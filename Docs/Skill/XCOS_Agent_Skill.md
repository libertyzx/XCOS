---
name: xcos-2.1.1
description: 使用 XCOS 2.1.1 协作式协程操作系统（Cooperative Coroutine RTOS）进行裸机/嵌入式任务开发。提供任务、协程块、调度器、时间、通知的完整 API 指南、标准代码模式与陷阱规避，帮助 AI 直接生成正确、可编译的 XCOS 应用代码。适用语言：C。
---

# XCOS 2.1.1 开发技能（Agent Skill）

> 本技能面向 **AI 编码代理**：在生成任何使用 XCOS 2.1.1 的 C 代码前，先读取本文件掌握编程模型、API 约定与陷阱，再动手写代码。
>
> **要改 XCOS 内核本身**（而不是写应用）？请先读 [`XCOS_Dev_Agent_Skill.md`](./XCOS_Dev_Agent_Skill.md)：改动分类 → 影响面 → 改完怎么自测（编译三档、功能/体积/速度三件事、必要时重建基线）、文档同步矩阵、文档自检（§十，3 项）、登记流程与实战陷阱。

---

## 一、框架概述

- **XCOS** 是**协作式协程操作系统**：单线程、**非抢占**，一次只运行一个任务，任务主动让出（Yield/Delay/WaitNotify/Leave）后调度器才切换下一个；
- 任务以普通 C 函数编写，内部用**协程块宏**（`XC_Cor_*`）表达执行流程，靠 `switch-case`（ANSI）或 `computed goto`（GNU）保存/恢复断点，**无独立任务栈**（仅 TCB + 可选协程帧栈）；
- 系统组成：**调度器（XC_Sch）**、**任务（XC_Task）**、**协程块（XC_Cor）**、**时间（XC_Time）**、底层**链表（XC_List）**与**内核锁（XC_Core，内部）**；
- 调度模型：主循环轮询**就绪表**（取首 → 游离化 → 运行 → 游离任务插回表尾轮转）；时间表/溢出表管理延时；阻塞表管理挂起/死等；事件表（Event）做中断通知补偿；
- 版本宏：`XCOS_API_LEVEL`（=2）、`XCOS_VER_MAJOR/MINOR/PATCH/STRING/NUMBER`。
- **命名规范（生成代码时必须遵守）**：
    - 函数 / 函数式宏：`XC_<模块>_<功能>`（`XC_Task_Reg` / `XC_Cor_Yield` / `XC_Err_Report` / **`XC_Err_Hook`**）—— **模块与功能之间必须有下划线**；
    - 类型：`XC_<模块><名词>_t`（`XC_TaskHandle_t` / `XC_CorFrame_t` / `XC_ErrCode_t`）—— 模块与名词之间**不加**下划线；
    - 枚举值 / 常量宏：全大写 `XC_<模块>_<名>`（`XC_ERR_NONE` / `XC_TASK_READY`）；
    - **有意保留的例外**：断言宏 `XC_DIAG_ASSERT(cond)` 全大写（C 惯例，与 `assert` 一致）；
    - **旧名**一律放 `XC_Compat.h`（随 `XCOS.h` 自动包含，计划 V2.2.0 删除）：如任务旧名 `XCTask_Reg` ⇒ 规范名 `XC_Task_Reg`；**新代码只用规范名**（旧名仅供老代码迁移）。

## 二、核心编程模型（必须理解）

### 1. 任务函数结构（协程块）

```c
void MyTask(XC_TaskHandle_t t) {
    XC_Cor_Enter(t);              // 协程块头：置 CorState=SUSPENDED
    while(1) {
        // ... 业务代码 ...
        XC_Cor_DelayMs(10);       // 让出，10ms 后从此处继续
    }
    XC_Cor_Leave();               // 协程块尾：置 DONE（无限循环任务可省略语义，但需保留）
}
```

- `XC_Cor_Enter` / `XC_Cor_Leave` 必须成对包裹任务主体；
- **协程内局部变量在让出后不保存**（上下文切换不保存 C 栈），跨让出点需保留的数据放 TCB/静态/全局区；
- 任务函数不能中途 `return`（会跳过 Leave 导致协程块语法不闭合）；让出靠宏内部的 `goto`。

### 2. 启动流程

```c
#include "XCOS.h"

XCOS_t      g_xc;                  // 框架实例（全局）
XC_TaskCB_t s_Task;                // 任务 TCB

void TimerISR(void) { XC_Time_TickInc(); }   // 定时器中断中递增 Tick（必须）

int main(void) {
    XC_Sch_Init(&g_xc);                       // 1. 初始化调度器
    XC_Task_Reg(&g_xc, &s_Task, MyTask, NULL); // 2. 注册任务（进就绪表）
    XC_Sch_Start(&g_xc);                      // 3. 启动（阻塞，不返回）
    return 0;
}
```

### 3. Tick 与时间基准

- `XC_Time_TickInc()` 在定时器中断中按 `XC_CFG_TICKS_PER_SEC`（默认 1000Hz=1ms）递增；
- 所有延时/超时/定时器都基于 Tick 计数，时间换算宏自动处理（`XC_Time_MsToTicks` 等），**时间比较是溢出安全的**（无符号回绕），直接比较 `GetTick() >= deadline` 是错误的。

## 三、API 速查（分模块）

### 调度器（XC_Sch）

| API                                                                             | 说明                                                                 |
| ------------------------------------------------------------------------------- | -------------------------------------------------------------------- |
| `void XC_Sch_Init(XC_OSHandle_t phXCOS)`                                        | 初始化锁、四表、事件计数；启动前必须调用                             |
| `void XC_Sch_Start(XC_OSHandle_t phXCOS)`                                       | 启动调度器（阻塞，不返回）                                           |
| `uint8_t XC_Sch_GetTaskNum(XC_OSHandle_t phXCOS)`                               | 获取任务数                                                           |
| `void XC_Sch_SetIdleCallback(XC_OSHandle_t, void(*)(XC_OSHandle_t, XC_Tick_t))` | 设置空闲回调；参数 IdleTick 为可休眠 Tick 数（`~0U` 表示可无限休眠） |

### 任务（XC_Task）

| API                                                                                        | 调用等级                                     | 说明                                                                           |
| ------------------------------------------------------------------------------------------ | -------------------------------------------- | ------------------------------------------------------------------------------ |
| `XC_Return_t XC_Task_Reg(phXCOS, phTCB, fTask, pParam)`                                    | 初始化/非协程                                | 注册任务（无嵌套）                                                             |
| `XC_Return_t XC_Task_RegExt(phXCOS, phTCB, fTask, pParam, pCorStack, CorDepthMax)`         | 初始化/非协程                                | 注册任务（支持协程嵌套，需配帧栈）                                             |
| `XC_Return_t XC_Task_SetCorStack(phTCB, pCorStack, CorDepthMax)`                           | 非协程                                       | 配置/撤销嵌套帧栈                                                              |
| `void XC_Task_SetEntry(phTCB, fTask, pParam)` / `XC_Return_t XC_Task_Add(phXCOS, phTCB)`   | 非协程                                       | 分步注册（先设入口再添加）；嵌套中换入口会自动清栈（按新入口从头运行）         |
| `void XC_Task_Reset(phTCB)`                                                                | 任务/主循环（非协程内，**不可中断**）        | 复位任务（清断点/清栈，从头运行）；复位自身用 `XC_Cor_Reset`                   |
| `XC_Return_t XC_Task_Suspend(phTCB)`                                                       | 任务/主循环（非协程内，**不可中断**）        | 挂起任务（挂起后不调度）                                                       |
| `XC_Return_t XC_Task_Resume(phTCB)`                                                        | 任务/主循环（非协程内，**不可中断**）        | 恢复挂起任务（并交付挂起前已登记的通知）                                       |
| `void XC_Task_Remove(phTCB)`                                                               | 任务/主循环（非协程内，**不可中断**）        | 移除任务；移除自身用 `XC_Cor_Remove`                                           |
| `XC_Return_t XC_Task_SendNotify(phTCB, void* pData)`                                       | 任意 / **中断（ISR 唯一合法的框架调用）**    | 发送通知唤醒任务（SPSC）；ISR 只走这一条路，见 §四.7                          |
| `XC_Task_ClrNotify(phTCB)`（宏）                                                           | **任务上下文（消费侧；不可中断）**           | 清除通知（写消费侧字段，ISR 调用会丢唤醒）                                     |
| `XC_Task_UpdateNotifyData(phTCB, data)`（宏）                                              | **生产者上下文（与 `SendNotify` 同上下文）** | 写通知数据（生产侧字段）                                                       |
| `XC_Task_ReadNotifyData(phTCB)`（宏）                                                      | 任意（只读）                                 | 读通知数据；可能读到瞬时旧值                                                   |
| `XC_Task_GetParam(phTCB)` / `XC_Task_GetState(phTCB)`（宏）                                | 任意                                         | 参数 / 状态查询                                                                |
| `XC_Diag_GetRunCnt(phTCB)` / `XC_Diag_GetMaxRunTick(phTCB)` / `XC_Diag_ClrRunStats(phTCB)` | 任意（读）/ 非协程（清零）                   | **可选统计**（`XC_CFG_TASK_STATS`）：运行次数 / 最长一次耗时 / 清零；见陷阱 20 |

### 协程块（XC_Cor，全部为宏，必须在协程块内使用）

| 宏                                                         | 说明                                                                                   |
| ---------------------------------------------------------- | -------------------------------------------------------------------------------------- |
| `XC_Cor_Enter(phTCB)` / `XC_Cor_Leave()`                   | 协程块头 / 尾（必须成对）                                                              |
| `XC_Cor_Call(fn)`                                          | 嵌套调用子协程（需 `XC_Task_RegExt`/`SetCorStack` 配置帧栈；未配栈则触发任务复位保护） |
| `XC_Cor_Yield()`                                           | 让出 CPU（下一次从此处继续）                                                           |
| `XC_Cor_Suspend()`                                         | 挂起自身（只能被 `XC_Task_Resume` 恢复）                                               |
| `XC_Cor_Reset()`                                           | 复位自身（从头运行）                                                                   |
| `XC_Cor_Remove()`                                          | 移除自身                                                                               |
| `XC_Cor_GetParam()`                                        | 获取任务注册参数                                                                       |
| `XC_Cor_DelayTick(n)` / `DelayUs` / `DelayMs` / `DelaySec` | 延时后继续（时间单位宏自动换算）                                                       |
| `XC_Cor_WaitNotify(timeout)` / `XC_Cor_WaitNotifyMs(ms)`   | 等待通知（timeout=0 为死等；超时后 `IsNotifyTimeout` 为真）                            |
| `XC_Cor_ClrNotify()`                                       | 清除通知（等待前调用防提前通知）                                                       |
| `XC_Cor_IsNotifyTimeout()`                                 | 等待通知后判断是否超时（1=超时，0=收到通知）                                           |
| `XC_Cor_GetNotifyData()`                                   | 获取通知数据指针                                                                       |

### 时间（XC_Time）

| API                                                                       | 说明                                          |
| ------------------------------------------------------------------------- | --------------------------------------------- |
| `XC_Time_GetTick()`                                                       | 当前 Tick                                     |
| `XC_Time_TickInc()` / `XC_Time_TickSet(t)`                                | 中断递增 / 设置 Tick（休眠唤醒后校正）        |
| `XC_Time_GetTickUnit()` / `XC_Time_GetMs()`                               | Tick 最小单位(us) / 运行 ms                   |
| `XC_Time_CheckTimeout(last, count)` / `Ms` / `Sec`                        | 溢出安全超时判断（`GetTick()-last >= count`） |
| `XC_Time_GetRemain(last, count)` / `GetElapsed(last, count)`              | 剩余 / 已运行 Tick（溢出安全）                |
| `XC_Time_MsToTicks/UsToTicks/SecToTicks`、`TicksToMs/Us/Sec`              | 时间单位换算宏                                |
| `XC_Time_TimerSet/TimerSetMs/TimerSetSec/TimerRepeat/TimerCheck/TimerClr` | 软件定时器宏                                  |
| `XC_Time_TimerGetRemain/TimerGetElapsed/TimerGetElapsedMs`                | 定时器查询                                    |
| `XC_Time_BlockDelay(ticks)` / `BlockDelayMs(ms)`                          | 死循环阻塞延时（非协程，会占住 CPU）          |

### 配置（XC_Config.h）

| 宏                                                                                   | 默认         | 说明                                                                                                                                                                                           |
| ------------------------------------------------------------------------------------ | ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `XC_Tick_t`（**固定 32 位无符号，V2.1.1 起不可配**）                                 | `uint32_t`   | Tick 数据类型（固定 32 位无符号 `uint32_t`，定义在类型层；**不可配置**）                                                                                                                       |
| `XC_CFG_MAX_TASKS`                                                                   | 100          | 最大任务数（**1~255**；上限由**编译期断言**拦截，建议 ≥10）                                                                                                                                   |
| `XC_CFG_TICKS_PER_SEC`                                                               | 1000         | Tick 频率（Hz），1ms/tick                                                                                                                                                                      |
| `XC_SYS_TICK_COUNT` / `XC_SYS_TICK_INT_INC_MODE`                                     | 中断累加     | Tick 计数方式（中断累加 / 外部计数器）                                                                                                                                                         |
| `XC_CFG_ERR_HOOK`                                                                    | 0（关）      | 错误上报钩子（诊断）：置 1 后**必须实现** `XC_Err_Hook(phTCB, ErrCode)`；见陷阱 18                                                                                                             |
| `XC_CFG_DEBUG_CHECK`                                                                 | 0（关）      | 运行期不变量自检（调试）：置 1 后可用 `XC_Diag_CheckInvariants`；见陷阱 19                                                                                                                     |
| `XC_CFG_TASK_STATS`                                                                  | 0（关）      | 每任务运行统计（可选）：置 1 后 TCB **+8B/任务**，可用 `XC_Diag_GetRunCnt/GetMaxRunTick`；见陷阱 20                                                                                            |
| `XC_CFG_ASSERT`                                                                      | 0（关）      | 参数/状态断言（诊断）：置 1 后可用 `XC_DIAG_ASSERT(cond)`；**必须同时开 `XC_CFG_ERR_HOOK`**；见陷阱 21                                                                                         |
| `XC_CFG_COR_NESTING`                                                                 | 1（开）      | 协程嵌套支持：置 **0** 后 TCB **44→36 B/任务**、无 `RegExt`/`SetCorStack`/`Cor_Call`/同轮切父（基本协程不变）                                                                                 |
| `XCOS_CFG` / `XCOS_Cfg.h`                                                            | —           | 独立配置文件方式                                                                                                                                                                               |

### 类型与枚举

- 框架：`XCOS_t`、`XC_OSHandle_t`；任务：`XC_TaskCB_t`、`XC_TaskHandle_t`、`XC_CorFn_t`、`XC_CorFrame_t`、`XC_BP_t`；时间：`XC_Tick_t`、`XC_TimerTick_t`；链表：`XC_ListNode_t`；
- `XC_Return_t`：`XC_OK` / `XC_FAIL` / `XC_CONTINUE`；
- `XC_TaskState_t`：`XC_TASK_VOID` / `XC_TASK_RUN` / `XC_TASK_READY` / `XC_TASK_BLOCKED` / `XC_TASK_SUSPEND`；
- 内部：`XC_NotifyState_t`（WAKEUP/WAIT）、`XC_CorState_t`（DONE/SUSPENDED）；
- 错误上报：`XC_ErrCode_t`（`XC_ERR_NONE` / `XC_ERR_FRAME_OVERFLOW` / `XC_ERR_ASSERT`）—— 上报钩子 `XC_Err_Hook` 用（声明见 `Code/Inc/XC_Err.h`），需 `XC_CFG_ERR_HOOK`；
- 调试：`XC_ChkResult_t`（`XC_CHK_OK` / `..._STATE_TABLE` 等）—— 运行期自检返回值，需 `XC_CFG_DEBUG_CHECK`。

## 四、标准代码模式

### 1. 最小系统（见“核心编程模型”第 2 节）

### 2. 任务延时

```c
void BlinkTask(XC_TaskHandle_t t) {
    XC_Cor_Enter(t);
    while(1) {
        LedToggle();
        XC_Cor_DelayMs(500);   // 500ms 后恢复
    }
    XC_Cor_Leave();
}
```

### 3. 任务通知（SPSC，中断可调用）

```c
/* 任务 A：等待通知 */
void TaskA(XC_TaskHandle_t t) {
    XC_Cor_Enter(t);
    while(1) {
        XC_Cor_ClrNotify();            // 防止残留通知
        XC_Cor_WaitNotify(0);          // 死等通知
        if(!XC_Cor_IsNotifyTimeout()) {
            void* data = XC_Cor_GetNotifyData();
            /* 处理通知数据 */
        }
    }
    XC_Cor_Leave();
}

/* 任意上下文（含中断）：发送通知 */
XC_Task_SendNotify(&s_TaskA, (void*)0x01);
```

### 4. 协程嵌套（V2.1.0）

```c
static XC_TaskCB_t    s_Task;
static XC_CorFrame_t  s_Stack[4];      // 帧栈容量 = 最大嵌套层数

void SubA(XC_TaskHandle_t t) { XC_Cor_Enter(t); XC_Cor_DelayMs(10); XC_Cor_Leave(); }
void SubB(XC_TaskHandle_t t) { XC_Cor_Enter(t); XC_Cor_Yield();      XC_Cor_Leave(); }

void ParentTask(XC_TaskHandle_t t) {
    XC_Cor_Enter(t);
    while(1) {
        XC_Cor_Call(SubA);             // 登记子协程，调度器下一轮进入
        XC_Cor_Call(SubB);             // 子协程完成后同轮切父返回此处
        XC_Cor_DelayMs(100);
    }
    XC_Cor_Leave();
}

/* 注册（必须配帧栈） */
XC_Task_RegExt(&g_xc, &s_Task, ParentTask, NULL, s_Stack, 4U);
```

### 5. 挂起 / 恢复 / 复位 / 移除

```c
XC_Task_Suspend(&s_Task);   // 挂起（其他任务中调用）
XC_Task_Resume(&s_Task);    // 恢复
XC_Task_Reset(&s_Task);     // 复位（从头运行）
XC_Task_Remove(&s_Task);    // 移除
/* 自身操作用协程内宏：XC_Cor_Suspend / XC_Cor_Reset / XC_Cor_Remove */
```

### 6. 空闲回调与休眠

```c
void Idle(XC_OSHandle_t os, XC_Tick_t IdleTick) {
    if(IdleTick == (XC_Tick_t)~0U) { /* 可无限休眠 */ }
    else {
        /* 休眠 IdleTick 后唤醒: 需校正 Tick */
        XC_Time_TickSet(XC_Time_GetTick() + IdleTick);
    }
}
XC_Sch_SetIdleCallback(&g_xc, Idle);
```

### 7. 中断里要"挂起 / 恢复 / 复位 / 移除"任务怎么办（**ISR 只走通知**）

**中断里唯一允许的框架调用是 `XC_Task_SendNotify`**；ISR 想产生"移表类"效果时，正确写法是**通知一个控制任务，由该任务（任务上下文）执行 API**：

```c
/* ---- ISR 侧：只发通知，不碰任务状态/链表 ---- */
void EXTI0_IRQHandler(void)
{
    (void)XC_Task_SendNotify(&s_Ctrl, (void*)&s_Event);   /* 生产侧: 只写常量 1 */
}

/* ---- 控制任务（任务上下文）：真正动表 ---- */
void Task_Ctrl(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) {
        XC_Cor_WaitNotify(0U);
        if(!XC_Cor_IsNotifyTimeout()) {
            XC_Task_Resume(&s_Worker);   /* 挂起/恢复/复位/移除都在这里做 */
        }
    }
    XC_Cor_Leave(t);
}
```

对照表（**为什么不能在 ISR 里直接调**）：

| ISR 里想做的事       | 禁止直接调用                                                           | 正确做法                                        |
| -------------------- | ---------------------------------------------------------------------- | ----------------------------------------------- |
| 让某任务"恢复/就绪"  | `XC_Task_Resume`                                                       | 通知目标任务或控制任务，由任务上下文调用        |
| 挂起/复位/移除某任务 | `XC_Task_Suspend` / `Reset` / `Remove`                                 | 同上（用通知把"意图"传出去）                    |
| 清除通知             | `XC_Task_ClrNotify`（消费侧字段）                                      | 由目标任务自身在等待前 `XC_Cor_ClrNotify()`     |
| 传递数据             | `XC_Task_UpdateNotifyData`（生产侧字段，仅可与 `SendNotify` 同上下文） | 在 ISR 内紧随 `SendNotify` 之前写（同一生产者） |

> 机理：`XC_Core_Lock/Unlock` 只是"主循环 ↔ 中断"的**单一标志位**（不可重入、不关中断）。ISR 里调用 `Suspend/Resume` 这类 API 会**把主循环正在持有的锁提前打开**，造成**静默**的链表损坏（不立刻报错，之后某次调度才崩）。


## 五、必须遵守的约定与陷阱（生成代码时逐条检查）

1. **协程块包裹**：任务函数主体必须在 `XC_Cor_Enter` / `XC_Cor_Leave` 之间；**不能在协程块内 `return`**；
2. **局部变量不跨让出点保存**：`XC_Cor_Delay*` / `XC_Cor_Yield` / `XC_Cor_WaitNotify` / `XC_Cor_Call` 之后的局部变量会被重置；跨让出的状态放 **TCB 字段 / 静态变量 / 全局变量**；
3. **Tick 递增**：`XC_Time_TickInc()` 必须在定时器中断中调用；否则所有延时/超时永远不触发；
4. **移表类 API 不可在中断中使用**：`XC_Task_Suspend` / `XC_Task_Resume` / `XC_Task_Reset` / `XC_Task_Remove` 仅限**任务/主循环上下文**（非协程内亦可）；ISR 中唯一合法的框架调用是 `XC_Task_SendNotify` —— 需要"挂起/恢复"就通知控制任务，由任务上下文执行（范式见 §四.7）；`XC_Task_ClrNotify` 写消费侧字段，同样不可在中断中调用；
5. **通知 SPSC**：`XC_Task_SendNotify` 可中断调用，但同一任务只能有**一个生产者**；多生产者需用户自加锁；`pNotifyData` 在唤醒前更新、唤醒后读取；
6. **任务不可重复注册**：同一 TCB 重复 `XC_Task_Reg` / `XC_Task_Add` 会返回 **`XC_FAIL`**（框架检测：`phXCOS` 非空即为已注册）；**移除后可再次注册**；移除/复位不释放 TCB；
7. **嵌套必须配帧栈**：使用 `XC_Cor_Call` 的任务必须用 `XC_Task_RegExt` 或 `XC_Task_SetCorStack` 配置 `pCorStack`/`CorDepthMax`；`CorDepth` 超过容量会触发**任务复位保护**（安全降级 ⇒ **静默复位**：任务从头运行、无任何上报）；最大嵌套层数不超过 `CorDepthMax`；排查"任务莫名从头运行"时请打开诊断开关（见陷阱 18）；
8. **延时/超时参数**：`XC_Cor_WaitNotify(0)` = 死等（无超时）；`XC_Cor_DelayMs(0)` 不阻塞（立即让出并回就绪）；`UsToTicks` 误差大（us 按 1 tick 处理）；
9. **时间比较用宏**：判断超时使用 `XC_Time_CheckTimeout`（溢出安全），不要写 `GetTick() >= deadline`；
10. **空闲回调**：`IdleTick == ~0U` 表示可无限休眠；若休眠停止 Tick 计数，唤醒时用 `XC_Time_TickSet` 校正；
11. **任务状态语义**：注册/唤醒后 `READY`，调度运行 `RUN`，延时/等待 `BLOCKED`，挂起 `SUSPEND`，移除 `VOID`；调度器运行完游离任务会插回就绪表尾并置 `READY`（轮转公平）；
12. **无抢占**：任务不能依赖"运行期间不被其他任务打断"之外的抢占；长时间计算应主动 `XC_Cor_Yield` 分片执行。
13. **新增/修改共享字段必须登记并发契约**：`XCOS_t` / `XC_TaskCB_t` 的字段改动要在 `Docs/架构概览/XCOS_V2.1.0_架构概览.md`「并发模型与字段所有权」矩阵中声明"写者集合 + 并发模型 + 安全依据"；**禁止"多写者 + 读-改-写"**（`++`、位或赋值、位域），确需计数用差值法；
14. **每个字段的并发等级以矩阵为准**：ISR 只允许写"生产侧常量"（`NotifyPending`），消费侧字段（`ClrNotify`）与移表类 API 均限主循环上下文；
15. **协作式时延约束（无优先级）**：本内核**无时间片/无优先级** ⇒ **任一任务的执行时间 = 其它所有任务的最坏额外延迟**；单次执行建议控制在**百微秒量级**，长计算必须**分片**（循环内周期性 `XC_Cor_Yield()`）；缓冲型外设（串口等）用 DMA/环形缓冲吸收；需要确定性/抢占 ⇒ 应改用抢占式 RTOS（详见 `Docs/架构概览/XCOS_V2.1.0_架构概览.md`「并发模型与字段所有权」§12.5）；
16. **`XC_Core_Lock` / `Unlock` 不是互斥锁**：它是"**主循环处于关键区**"的**单一标志位**（不可重入、不关中断、无等待队列）⇒ **不得嵌套、必须配对、不得在中断里 `Unlock`**；另外 `XC_Task_HandleWaitNotify` 会**持锁**进入 `XC_Task_HandleBlocking`，由后者内部解锁（**隐式配对**）——改动这两个函数时必须保持"**每条路径恰好解锁一次**"，否则锁永久为 1、所有中断通知退化为"只登记"（直接唤醒静默失效）；
17. **`XC_Time_SecToTicks(s)` 有溢出上限**：上限 ≈ `XC_TICK_MAX / f`（1000 Hz ⇒ 约 49.7 天；10000 Hz ⇒ 约 4.97 天）；大延时请**分段**或改用 `XC_Cor_DelayTick()`（详见 `Docs/XC_Config.md` 的 `XC_CFG_TICKS_PER_SEC` 节）；
18. **"任务莫名从头运行" ⇒ 开错误上报钩子定位**：帧栈越界（未配栈就 `XC_Cor_Call` / 嵌套超 `CorDepthMax`）默认是**静默复位**（`XC_Task_PushFrame` ⇒ `HandleReset`），现场没有任何痕迹；排查时**打开 `XC_CFG_ERR_HOOK`(1)** 并实现钩子即可捕获：

    ```c
    #define XC_CFG_ERR_HOOK (1U)   /* 包含 XCOS.h 之前 */
    #include "XCOS.h"

    void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)   /* 必须实现 */
    {
        if(ErrCode == XC_ERR_FRAME_OVERFLOW) {
            /* CorDepthMax==0 ⇒ 忘了配帧栈; CorDepth==CorDepthMax ⇒ 嵌套过深 */
        }
    }
    ```

    - 默认关闭 ⇒ **0 代码 / 0 RAM / 0 开销**（开启后 `XC_Task.o` 对象级 +12 B），关闭时无需实现该函数；
    - 钩子内**不得**调用会移表的 API、不得阻塞；详见 `Docs/XCOS.md`「错误上报」节与 `Docs/XC_Config.md`；
19. **改内核/锁之后跑一次"不变量自检"**：置 `XC_CFG_DEBUG_CHECK=1` 后可调用 `XC_Diag_CheckInvariants(&g_xc)` —— 它盘点「任务状态 ↔ 所在表」等内核依赖的不变量，返回 `XC_CHK_OK` 或**第一个失败项编号**（1=链表损坏/成环、2=状态↔表不符〔僵尸状态类〕、3=同一任务挂两张表、4=`TaskNum` 不符、5=嵌套不自洽、6=任务归属错）；

    ```c
    #define XC_CFG_DEBUG_CHECK (1U)   /* 包含 XCOS.h 之前 */
    #include "XCOS.h"

    XC_ChkResult_t rc = XC_Diag_CheckInvariants(&g_xc);
    if(rc != XC_CHK_OK) { /* rc 即第一个失败项编号 */ }
    ```

    - 怎么用：**回归用例里每个 API 之后调一次**（定位最强）／空闲回调里定期抽查／排障时手工插入；
    - 默认关闭 ⇒ 该函数**不编译**（发布 0 代码/0 RAM）；调试档实测 `XC_Diag.o` **+436 B**（实现在诊断模块, 内核侧不增加）；
    - 只读检查、内部短暂持锁 ⇒ **不可在已持锁的上下文**调用；详见 `Docs/XCOS.md`「运行期自检」节；
20. **"谁拖慢了系统" ⇒ 打开每任务运行统计**：协作式内核无优先级 ⇒ **任一任务的执行时间就是其它任务的额外延迟**；置 `XC_CFG_TASK_STATS=1` 后框架自动记录每任务的**运行次数**与**最长一次耗时**（"调度槽"口径：从摘出就绪表到让出，含同轮切父，**不含等待时间**）：

    ```c
    #define XC_CFG_TASK_STATS (1U)   /* 包含 XCOS.h 之前 */
    #include "XCOS.h"

    uint32_t  runs = XC_Diag_GetRunCnt(&s_TaskA);
    XC_Tick_t peak = XC_Diag_GetMaxRunTick(&s_TaskA);   /* 单位 Tick(默认 1ms) */
    /* peak 偏大 ⇒ 该任务要分片(周期 XC_Cor_Yield)或拆分 */
    ```

    - 成本：**每个任务多 8 字节 RAM**（100 个任务 = +800 字节），代码约多 88 字节（示例工程含演示代码 +152 字节）；**默认关闭 ⇒ 0 代码 / 0 RAM**；
    - 口径提醒：1 ms 时基下**短于 1 tick 记 0**（要更细用计数器模式/高频档）；耗时**含运行期间的中断时间**（不是纯指令数，要指令周期用 MDK 仿真测量）；
    - 生命周期：注册/移除清零，`XC_Task_Reset` 保留；详见 `Docs/XCOS.md`「运行统计」节；
21. **断言只上报、不救场（`XC_CFG_ASSERT`）**：`XC_DIAG_ASSERT(cond)` 在条件为假时**只上报** `XC_ERR_ASSERT`（经 `XC_Err_Hook`），**不会 return/不止损**；因此：

    - **边界规则（单一强制点）**：同一形参 + 同一失效条件**只能二选一** —— 参数合法性（`NULL`、非法参数组合）用**断言**（违背 = 未定义行为，发布档不检查）；运行期前提 / 契约承诺要报告的结果（表满、重复注册、未挂起、自检失败项）用**返回码**；不同形参或不同条件可并存；复合 `if` 里不得再 `OR` 已被断言覆盖的子条件（边界规则全文见 `Docs/架构概览/XCOS_V2.1.0_架构概览.md` §11.6）；
    - **定位方式**：断言只上报 `XC_ERR_ASSERT` ⇒ 在钩子内对该码**打断点**，由**调用栈**定位 `文件:行号`（比"附加行号参数"信息更全且 0 额外开销）；实测若改为携带 `__FILE__`/`__LINE__`，**仅断言档**就要 **+152 B**（行号版 +92 B）⇒ 只在"无调试器留痕"场景才划算（口径见 `Docs/XCOS.md`「断言」）；
    - 断言**必须与钩子同开**（`XC_CFG_ASSERT=1` + `XC_CFG_ERR_HOOK=1`），只开断言 ⇒ **编译报错**；
    - 钩子里**取现场前必须判空**：断言上报时 `phTCB == NULL`（帧栈越界那类上报才有任务现场）；
      ```c
      void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)
      {
          if(ErrCode == XC_ERR_ASSERT) { /* 打断点 ⇒ 调用栈直接指到断言处 */ }
          if(phTCB != NULL) { /* 仅任务上下文才有现场 */ }
      }
      ```
    - 关档时宏为空操作、条件**不求值** ⇒ 发布固件 0 开销；开档（`ASSERT`+`ERR_HOOK`）内核内 **26 个调用点 ≈ +360 B**（≈14 B/处，2026-09-15 去重后）；
    - **已有落点**（内核内，2026-09-14 落点；2026-09-15 去重后 26 处）：`XC_Task_Reg/RegExt/SetEntry/Add/SetCorStack/Remove/Reset/Suspend/Resume/SendNotify`（`phTCB`/`phXCOS`/`fTask` 非空、帧栈⇔容量一致）、`XC_Sch_Init/Start/GetTaskNum/SetIdleCallback`、`XC_Diag_*` 查询（3 处）；**热路径不加**（如 `XC_Diag_RunStatsEnd`）；改内核时若新增落点，请同步 `Docs/XC_Config.md` 的调用点表与 `Tests/baseline.md` 的断言档体积；
22. **"唤醒 / 恢复 / 注册"一律排队尾（严格轮转）**：任何任务"进入就绪表"都排在**队尾** —— 等待中通知、提前通知、定时到点、复位、恢复、注册**都不插队**（**回归 V2.0 语义**；V2.1.0 曾漂移为"唤醒插队到表首"）：

    - ✅ **好处**：公平（含 `fIdle` 的运行机会）；**过载时不会独占**（改前"事件率 ≥ 1/调度槽"会让通知任务独占调度器、其它任务饿死）；时序可预期；
    - ⚠️ **代价**：**唤醒最坏延迟 = 一轮**（≤ 就绪任务数 `N` 个调度槽）；要更快 ⇒ 减少就绪任务数 / 分片（`XC_Cor_Yield`），**不要**指望入表位置；
    - ⚠️ **注册顺序 = 首次执行顺序**（不要依赖"后注册的先跑"）；
    - ℹ️ 过载时通知被"1 bit 待处理标志"合并（**不丢**），各任务按轮转变慢 = 优雅降级；
    - 详见 `Docs/XCOS.md`「调度顺序与唤醒语义」；

## 六、文件索引（仓库结构）

| 路径                                                                                      | 内容                                                                                                                                                                                                                                                                                    |
| ----------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Code/Inc/XCOS.h`                                                                         | 总头文件（含版本宏 `XCOS_VER_*`、`XCOS_API_LEVEL`）                                                                                                                                                                                                                                     |
| `Code/Inc/XC_Config.h`                                                                    | 配置（`XC_CFG_*`、`XC_SYS_TICK_*`）                                                                                                                                                                                                                                                     |
| `Code/Inc/XC_Type.h` / `Internal/XC_TypeInternal.h`                                       | 用户类型 / 内部类型（TCB、枚举、协程帧）                                                                                                                                                                                                                                                |
| `Code/Inc/XC_Sch.h` / `XC_Task.h` / `XC_Cor.h` / `XC_Time.h`                              | 调度 / 任务 / 协程 / 时间 API 声明                                                                                                                                                                                                                                                      |
| `Code/Inc/XC_Err.h`                                                                       | **错误上报设施**（无实现文件）：错误码 `XC_ErrCode_t`、用户钩子 `XC_Err_Hook`、上报宏 `XC_Err_Report`（`XC_CFG_ERR_HOOK`，默认关）；只做上报的模块可只包含本头                                                                                                                          |
| `Code/Inc/XC_Diag.h` (+ `Code/Inc/Internal/XC_DiagInternal.h`) + `Code/Src/XC_Diag.c`     | **诊断能力**（与内核解耦，含 `XC_Err.h`）：自检 `XC_Diag_CheckInvariants`（`XC_CFG_DEBUG_CHECK`）、统计查询 `XC_Diag_GetRunCnt/GetMaxRunTick/ClrRunStats`（`XC_CFG_TASK_STATS`）；断言 `XC_DIAG_ASSERT`（`XC_CFG_ASSERT`）与调度槽埋点在**内部头**里；全部默认关；**可按工程裁剪该 .c** |
| `Code/Inc/Internal/XC_List.h` / `XC_Core.h`                                               | 内部链表 / 锁（用户一般不用）                                                                                                                                                                                                                                                           |
| `Code/Src/XC_Diag.c` / `XC_Sch.c` / `XC_Task.c` / `XC_Time.c` / `XC_List.c`               | 实现（诊断实现集中在 `XC_Diag.c`）                                                                                                                                                                                                                                                      |
| `Docs/XCOS.md`                                                                            | API 参考手册（函数级文档）                                                                                                                                                                                                                                                              |
| `Docs/架构概览/XCOS_V2.1.0_架构概览.md`                                                   | 完整实现说明                                                                                                                                                                                                                                                                            |
| `Examples/CortexM3_Test/`                                                                 | 12 任务示例工程（A0~A11，含嵌套/通知/分步注册）                                                                                                                                                                                                                                         |
| `Docs/Skill/XCOS_Agent_Skill.md` · `XCOS_Dev_Agent_Skill.md` · `MDK_Sim_Agent_Skill.md` | 三份技能文档：**用**框架 / **开发内核** / **MDK 仿真实测**                                                                                                                                                                                                                              |
| `Tests/`                                                                                  | 一条命令自测三件事：功能（20 条用例：**PASS 17 / FAIL 0 / SKIP 3**）/ 固件大小 / 关键路径耗时 —— `powershell -File Tests/run_all.ps1`（数值见 `Tests/baseline.md`）                                                                                                                   |

## 七、已知缺陷与限制

- **挂起优先级最高**：`WAKEUP_NOTIFY` 唤醒判据已排除 `TaskState==SUSPEND`，且 `XC_Task_HandleSuspend` 的状态写入与移表在同一锁域；**挂起只能由 `XC_Task_Resume` 唤醒**，`Resume` 会交付挂起前已登记的待处理通知（恢复后等待侧报"已通知"；无通知则报"超时"分支，不会死等）。**仍然成立的使用约定**：挂起期间到达的新通知一律返回 `XC_FAIL`（不登记、不覆盖 `pNotifyData`），不要依赖"挂起期间的通知"。
- 通知采用 **SPSC 单生产者单消费者**模型，不支持多生产者并发（需外部锁）；
- **帧栈越界默认是"静默复位"（2026-09-14 起可诊断）**：未配帧栈就 `XC_Cor_Call`、或嵌套超过 `CorDepthMax` 时，框架安全降级为"复位任务"（任务从头运行、**无任何上报**）；打开 `XC_CFG_ERR_HOOK` 并实现 `XC_Err_Hook` 即可捕获（见陷阱 18），默认关闭时**0 开销**；
- **不变量自检默认关闭**：`XC_CFG_DEBUG_CHECK` 为 0 时 `XC_Diag_CheckInvariants` 不编译；如需"运行期守护"请自行在空闲回调里调用（见陷阱 19），默认关闭下发布固件 0 开销；
- **运行统计默认关闭**：`XC_CFG_TASK_STATS` 为 0 时统计字段与统计代码都不编译（0 RAM/0 代码）；打开后 TCB +8 B/任务，且**精度受 Tick 时基限制**（1 ms 档下短于 1 ms 的运行记 0，见陷阱 20）；
- 单任务不可重复注册（重复注册返回 `XC_FAIL`，移除后可再次注册）；同一时刻任务只能位于一张表（就绪/时间/溢出/阻塞）。

---

*本技能配套：`Docs/XCOS.md`（API 参考）、`Docs/架构概览/XCOS_V2.1.0_架构概览.md`（实现）、`Docs/XC_Config.md`（配置）、`Examples/`（示例）；
要**改内核本身**请看 [`XCOS_Dev_Agent_Skill.md`](./XCOS_Dev_Agent_Skill.md)，要**用 MDK 仿真测周期/体积**请看 [`MDK_Sim_Agent_Skill.md`](./MDK_Sim_Agent_Skill.md)。*

