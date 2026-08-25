---
name: xcos-2.1.0
description: 使用 XCOS 2.1.0 协作式协程操作系统（Cooperative Coroutine RTOS）进行裸机/嵌入式任务开发。提供任务、协程块、调度器、时间、通知的完整 API 指南、标准代码模式与陷阱规避，帮助 AI 直接生成正确、可编译的 XCOS 应用代码。适用语言：C。
---

# XCOS 2.1.0 开发技能（Agent Skill）

> 本技能面向 **AI 编码代理**：在生成任何使用 XCOS 2.1.0 的 C 代码前，先读取本文件掌握编程模型、API 约定与陷阱，再动手写代码。

---

## 一、框架概述

- **XCOS** 是**协作式协程操作系统**：单线程、**非抢占**，一次只运行一个任务，任务主动让出（Yield/Delay/WaitNotify/Leave）后调度器才切换下一个；
- 任务以普通 C 函数编写，内部用**协程块宏**（`XC_Cor_*`）表达执行流程，靠 `switch-case`（ANSI）或 `computed goto`（GNU）保存/恢复断点，**无独立任务栈**（仅 TCB + 可选协程帧栈）；
- 系统组成：**调度器（XC_Sch）**、**任务（XC_Task）**、**协程块（XC_Cor）**、**时间（XC_Time）**、底层**链表（XC_List）**与**内核锁（XC_Core，内部）**；
- 调度模型：主循环轮询**就绪表**（取首 → 游离化 → 运行 → 游离任务插回表尾轮转）；时间表/溢出表管理延时；阻塞表管理挂起/死等；事件表（Event）做中断通知补偿；
- 版本宏：`XCOS_API_LEVEL`（=2）、`XCOS_VER_MAJOR/MINOR/PATCH/STRING/NUMBER`。

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

| API | 说明 |
| --- | --- |
| `void XC_Sch_Init(XC_OSHandle_t phXCOS)` | 初始化锁、四表、事件计数；启动前必须调用 |
| `void XC_Sch_Start(XC_OSHandle_t phXCOS)` | 启动调度器（阻塞，不返回） |
| `uint8_t XC_Sch_GetTaskNum(XC_OSHandle_t phXCOS)` | 获取任务数 |
| `void XC_Sch_SetIdleCallback(XC_OSHandle_t, void(*)(XC_OSHandle_t, XC_Tick_t))` | 设置空闲回调；参数 IdleTick 为可休眠 Tick 数（`~0U` 表示可无限休眠） |

### 任务（XC_Task）

| API | 调用等级 | 说明 |
| --- | --- | --- |
| `XC_Return_t XC_Task_Reg(phXCOS, phTCB, fTask, pParam)` | 初始化/非协程 | 注册任务（无嵌套） |
| `XC_Return_t XC_Task_RegExt(phXCOS, phTCB, fTask, pParam, pCorStack, CorDepthMax)` | 初始化/非协程 | 注册任务（支持协程嵌套，需配帧栈） |
| `XC_Return_t XC_Task_SetCorStack(phTCB, pCorStack, CorDepthMax)` | 非协程 | 配置/撤销嵌套帧栈 |
| `void XC_Task_SetEntry(phTCB, fTask, pParam)` / `XC_Return_t XC_Task_Add(phXCOS, phTCB)` | 非协程 | 分步注册（先设入口再添加） |
| `void XC_Task_Reset(phTCB)` | 任意（非协程内） | 复位任务（清断点/清栈，从头运行）；复位自身用 `XC_Cor_Reset` |
| `XC_Return_t XC_Task_Suspend(phTCB)` | 任意（非协程内，**不可中断**） | 挂起任务（挂起后不调度） |
| `XC_Return_t XC_Task_Resume(phTCB)` | 任意（非协程内，**不可中断**） | 恢复挂起任务 |
| `void XC_Task_Remove(phTCB)` | 任意（非协程内） | 移除任务；移除自身用 `XC_Cor_Remove` |
| `XC_Return_t XC_Task_SendNotify(phTCB, void* pData)` | 任意 / **中断** | 发送通知唤醒任务（SPSC） |
| `XC_Task_ClrNotify(phTCB)`（宏） | 任意 | 清除通知 |
| `XC_Task_UpdateNotifyData(phTCB, data)` / `XC_Task_ReadNotifyData(phTCB)`（宏） | 任意 | 通知数据读写 |
| `XC_Task_GetParam(phTCB)` / `XC_Task_GetState(phTCB)`（宏） | 任意 | 参数 / 状态查询 |

### 协程块（XC_Cor，全部为宏，必须在协程块内使用）

| 宏 | 说明 |
| --- | --- |
| `XC_Cor_Enter(phTCB)` / `XC_Cor_Leave()` | 协程块头 / 尾（必须成对） |
| `XC_Cor_Call(fn)` | 嵌套调用子协程（需 `XC_Task_RegExt`/`SetCorStack` 配置帧栈；未配栈则触发任务复位保护） |
| `XC_Cor_Yield()` | 让出 CPU（下一次从此处继续） |
| `XC_Cor_Suspend()` | 挂起自身（只能被 `XC_Task_Resume` 恢复） |
| `XC_Cor_Reset()` | 复位自身（从头运行） |
| `XC_Cor_Remove()` | 移除自身 |
| `XC_Cor_GetParam()` | 获取任务注册参数 |
| `XC_Cor_DelayTick(n)` / `DelayUs` / `DelayMs` / `DelaySec` | 延时后继续（时间单位宏自动换算） |
| `XC_Cor_WaitNotify(timeout)` / `XC_Cor_WaitNotifyMs(ms)` | 等待通知（timeout=0 为死等；超时后 `IsNotifyTimeout` 为真） |
| `XC_Cor_ClrNotify()` | 清除通知（等待前调用防提前通知） |
| `XC_Cor_IsNotifyTimeout()` | 等待通知后判断是否超时（1=超时，0=收到通知） |
| `XC_Cor_GetNotifyData()` | 获取通知数据指针 |

### 时间（XC_Time）

| API | 说明 |
| --- | --- |
| `XC_Time_GetTick()` | 当前 Tick |
| `XC_Time_TickInc()` / `XC_Time_TickSet(t)` | 中断递增 / 设置 Tick（休眠唤醒后校正） |
| `XC_Time_GetTickUnit()` / `XC_Time_GetMs()` | Tick 最小单位(us) / 运行 ms |
| `XC_Time_CheckTimeout(last, count)` / `Ms` / `Sec` | 溢出安全超时判断（`GetTick()-last >= count`） |
| `XC_Time_GetRemain(last, count)` / `GetElapsed(last, count)` | 剩余 / 已运行 Tick（溢出安全） |
| `XC_Time_MsToTicks/UsToTicks/SecToTicks`、`TicksToMs/Us/Sec` | 时间单位换算宏 |
| `XC_Time_TimerSet/TimerSetMs/TimerSetSec/TimerRepeat/TimerCheck/TimerClr` | 软件定时器宏 |
| `XC_Time_TimerGetRemain/TimerGetElapsed/TimerGetElapsedMs` | 定时器查询 |
| `XC_Time_BlockDelay(ticks)` / `BlockDelayMs(ms)` | 死循环阻塞延时（非协程，会占住 CPU） |

### 配置（XC_Config.h）

| 宏 | 默认 | 说明 |
| --- | --- | --- |
| `XC_CFG_TICK_TYPE` | `uint32_t` | Tick 数据类型 |
| `XC_CFG_MAX_TASKS` | 100 | 最大任务数（10~255） |
| `XC_CFG_TICKS_PER_SEC` | 1000 | Tick 频率（Hz），1ms/tick |
| `XC_SYS_TICK_COUNT` / `XC_SYS_TICK_INT_INC_MODE` | 中断累加 | Tick 计数方式（中断累加 / 外部计数器） |
| `XCOS_CFG` / `XCOS_Cfg.h` | — | 独立配置文件方式 |

### 类型与枚举

- 框架：`XCOS_t`、`XC_OSHandle_t`；任务：`XC_TaskCB_t`、`XC_TaskHandle_t`、`XC_CorFn_t`、`XC_CorFrame_t`、`XC_BP_t`；时间：`XC_Tick_t`、`XC_TimerTick_t`；链表：`XC_ListNode_t`；
- `XC_Return_t`：`XC_OK` / `XC_FAIL` / `XC_CONTINUE`；
- `XC_TaskState_t`：`XC_TASK_VOID` / `XC_TASK_RUN` / `XC_TASK_READY` / `XC_TASK_BLOCKED` / `XC_TASK_SUSPEND`；
- 内部：`XC_NotifyState_t`（WAKEUP/WAIT）、`XC_CorState_t`（DONE/SUSPENDED）。

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


## 五、必须遵守的约定与陷阱（生成代码时逐条检查）

1. **协程块包裹**：任务函数主体必须在 `XC_Cor_Enter` / `XC_Cor_Leave` 之间；**不能在协程块内 `return`**；
2. **局部变量不跨让出点保存**：`XC_Cor_Delay*` / `XC_Cor_Yield` / `XC_Cor_WaitNotify` / `XC_Cor_Call` 之后的局部变量会被重置；跨让出的状态放 **TCB 字段 / 静态变量 / 全局变量**；
3. **Tick 递增**：`XC_Time_TickInc()` 必须在定时器中断中调用；否则所有延时/超时永远不触发；
4. **挂起/恢复不可在中断中使用**：`XC_Task_Suspend` / `XC_Task_Resume` 仅限任务/主循环上下文（非协程内亦可）；
5. **通知 SPSC**：`XC_Task_SendNotify` 可中断调用，但同一任务只能有**一个生产者**；多生产者需用户自加锁；`pNotifyData` 在唤醒前更新、唤醒后读取；
6. **任务不可重复注册**：同一 TCB 重复 `XC_Task_Reg` 会未知行为；移除/复位不释放 TCB；
7. **嵌套必须配帧栈**：使用 `XC_Cor_Call` 的任务必须用 `XC_Task_RegExt` 或 `XC_Task_SetCorStack` 配置 `pCorStack`/`CorDepthMax`；`CorDepth` 超过容量会触发**任务复位保护**（安全降级）；最大嵌套层数不超过 `CorDepthMax`；
8. **延时/超时参数**：`XC_Cor_WaitNotify(0)` = 死等（无超时）；`XC_Cor_DelayMs(0)` 不阻塞（立即让出并回就绪）；`UsToTicks` 误差大（us 按 1 tick 处理）；
9. **时间比较用宏**：判断超时使用 `XC_Time_CheckTimeout`（溢出安全），不要写 `GetTick() >= deadline`；
10. **空闲回调**：`IdleTick == ~0U` 表示可无限休眠；若休眠停止 Tick 计数，唤醒时用 `XC_Time_TickSet` 校正；
11. **任务状态语义**：注册/唤醒后 `READY`，调度运行 `RUN`，延时/等待 `BLOCKED`，挂起 `SUSPEND`，移除 `VOID`；调度器运行完游离任务会插回就绪表尾并置 `READY`（轮转公平）；
12. **无抢占**：任务不能依赖"运行期间不被其他任务打断"之外的抢占；长时间计算应主动 `XC_Cor_Yield` 分片执行。

## 六、文件索引（仓库结构）

| 路径 | 内容 |
| --- | --- |
| `Code/Inc/XCOS.h` | 总头文件（含版本宏 `XCOS_VER_*`、`XCOS_API_LEVEL`） |
| `Code/Inc/XC_Config.h` | 配置（`XC_CFG_*`、`XC_SYS_TICK_*`） |
| `Code/Inc/XC_Type.h` / `Internal/XC_TypeInternal.h` | 用户类型 / 内部类型（TCB、枚举、协程帧） |
| `Code/Inc/XC_Sch.h` / `XC_Task.h` / `XC_Cor.h` / `XC_Time.h` | 调度 / 任务 / 协程 / 时间 API 声明 |
| `Code/Inc/Internal/XC_List.h` / `XC_Core.h` | 内部链表 / 锁（用户一般不用） |
| `Code/Src/XC_Sch.c` / `XC_Task.c` / `XC_Time.c` / `XC_List.c` | 实现 |
| `Docs/XCOS.md` | API 参考手册（函数级文档） |
| `Docs/架构概览/XCOS_V2.1.0_架构概览.md` | 完整实现说明 |
| `Examples/CortexM3_Test/` | 12 任务示例工程（A0~A11，含嵌套/通知/分步注册） |

## 七、已知缺陷与限制

- **挂起任务可能被事件调度误唤醒**（DEF-20260824-02，不修复）：若任务在 `XC_Cor_WaitNotify`（`NotifyState==WAIT`）期间收到**异步通知**（中断在锁窗口 `XC_Task_SendNotify`，仅 `Produced++` 未移表）后**又被挂起**（挂起不清除通知），主循环事件调度会将其误唤醒。**使用约定**：不要对"等待通知中且可能收到异步通知"的任务执行挂起；若必须挂起，先 `XC_Cor_ClrNotify()` 或确认无未消费通知。详见 `Docs/缺陷记录/20260824_事件调度误唤醒挂起任务.md`；
- 通知采用 **SPSC 单生产者单消费者**模型，不支持多生产者并发（需外部锁）；
- 单任务不可重复注册；同一时刻任务只能位于一张表（就绪/时间/溢出/阻塞）。

---

*本技能配套：`Docs/XCOS.md`（API 参考）、`Docs/架构概览/XCOS_V2.1.0_架构概览.md`（实现）、`Docs/XC_Config.md`（配置）、`Examples/`（示例）。*

