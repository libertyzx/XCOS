# XCOS V2.1.0 完整实现说明

> **定位**：本文档从**当前实现**视角，对 XCOS V2.1.0 的整个系统做完整、细致的说明——总体架构、链表、时间、任务、协程、调度、通知、锁等各模块的实现方式，以及内存开销、边界限制与使用示例。所有代码块均为 V2.1.0 的**当前定义**。
> **配套文档**：`Docs/XCOS.md`（用户手册）、`Docs/CHANGELOG.md`（版本记录）、`Docs/MISRA-C/MISRA-C_2025_Deviation.md`（合规豁免）。
> **关联代码**：`Code/Inc/`（`XC_Config.h`/`XC_Type.h`/`XC_TypeInternal.h`/`XC_List.h`/`XC_Time.h`/`XC_Task.h`/`XC_Sch.h`/`XC_Cor.h`/`XCOS.h`）、`Code/Inc/Internal/`（`XC_Core.h`/`XC_List.h`/`XC_TimeInternal.h`/`XC_TaskInternal.h`/`XC_CorANSI.h`/`XC_CorGNU.h`）、`Code/Src/`（`XC_List.c`/`XC_Time.c`/`XC_Task.c`/`XC_Sch.c`）。

---

## 目录

1. [总体架构](#一总体架构)
2. [双向链表模块（XC_List）](#二双向链表模块xc_list)
3. [时间模块（XC_Time）](#三时间模块xc_time)
4. [任务模块（XC_Task）](#四任务模块xc_task)
5. [协程机制（XC_Cor 与底层）](#五协程机制xc_cor-与底层)
6. [调度器（XC_Sch）](#六调度器xc_sch)
7. [内核基础：锁（XC_Core）](#七内核基础锁xc_core)
8. [内存开销](#八内存开销)
9. [边界与限制](#九边界与限制)
10. [使用示例](#十使用示例)

---

## 一、总体架构

### 1.1 系统组成

XCOS 是一个**协作式（非抢占）协程操作系统**：任务以函数形式编写，内部用协程块表达执行流程；调度器在主循环中轮询就绪表，逐个运行任务。系统由以下模块组成：

| 模块 | 文件 | 职责 |
| --- | --- | --- |
| 配置 | `XC_Config.h` | 编译期配置（滴答类型/频率、最大任务数、滴答计数方式） |
| 类型 | `XC_Type.h` + `XC_TypeInternal.h` | 基础类型、句柄类型、TCB、协程类型、断点类型 |
| 链表 | `XC_List.h` / `XC_List.c` | 双向链表（就绪表/时间表/溢出表/阻塞表的底层容器） |
| 时间 | `XC_Time.h` / `XC_Time.c` + `XC_TimeInternal.h` | 滴答计数、时间换算、溢出安全比较、软件定时器 |
| 任务 | `XC_Task.h` / `XC_Task.c` + `XC_TaskInternal.h` | 任务注册/控制、阻塞处理、通知、协程帧栈 |
| 协程 | `XC_Cor.h` + `Internal/XC_CorANSI.h` / `XC_CorGNU.h` | 协程块宏、挂起/延时/通知/嵌套 |
| 调度 | `XC_Sch.h` / `XC_Sch.c` | 主循环、时间调度、事件调度、空闲处理 |
| 内核基础 | `Internal/XC_Core.h` | 框架实例锁 |

### 1.2 设计要点

- **协作式调度**：任务不被打断，主动让出（延时/等通知/让出）后才轮到下一个任务；无需上下文切换指令，C 栈与嵌套深度无关；
- **无开关中断**：框架不做关中断、不做原子操作，任务同步只支持**通知**且为 **SPSC**（单生产者单消费者）模型；CPU 内存重排序时用户需自加屏障；
- **时间表按唤醒时刻升序排列**：时间调度只唤醒队首到期任务链，O(1)~O(n) 高效；
- **可配置**：滴答计数类型、每秒滴答数、最大任务数、滴答计数方式均可配置；
- **协程嵌套**：任务可嵌套调用子协程（帧栈实现），但仅是协程机制的一个能力点，不影响其他模块。

### 1.3 类型与命名约定

- 基础类型（`uint8_t`/`uint16_t`/`uint32_t`/`bool` 等）定义于 `XC_Type.h`；
- `XC_Tick_t` 特殊（可配置），在 `XC_Config.h` 定义（默认 `uint32_t`）；
- 类型命名遵循 `XC_<名称>_t`（typedef 名）/ `XC_<名称>_tag`（struct 标签），如 `XC_ListNode_t`/`XC_ListNode_tag`、`XC_TaskCB_tag`、`XCOS_tag`、`XC_BP_t`、`XC_CorFrame_t`、`XC_CorState_t`、`XC_CorFn_t`、`XC_TimerTick_t`、`XC_Return_t`、`XC_TaskState_t`、`XC_NotifyState_t`；
- 句柄为不透明指针：`XC_OSHandle_t`（`XCOS_t*`）、`XC_TaskHandle_t`（`XC_TaskCB_t*`）。

### 1.4 配置系统（`XC_Config.h`）

```c
/* 可选:用户工程定义 XCOS_CFG 时包含自定义配置头 */
#ifdef XCOS_CFG
#include "XCOS_Cfg.h"
#endif

/* 滴答计数数据类型(默认 uint32_t) */
#ifndef XC_CFG_TICK_TYPE
#define XC_CFG_TICK_TYPE uint32_t
#endif
typedef XC_CFG_TICK_TYPE XC_Tick_t;

/* 最大任务数(10~255,默认100) */
#ifndef XC_CFG_MAX_TASKS
#define XC_CFG_MAX_TASKS (100U)
#endif

/* 系统每秒滴答数(Hz;默认1000=1ms;最大1000000=1us) */
#ifndef XC_CFG_TICKS_PER_SEC
#define XC_CFG_TICKS_PER_SEC (1000U)
#endif
```

**滴答计数两种方式**（`XC_SYS_TICK_COUNT`）：

1. **中断累加模式（默认）**：`XC_SYS_TICK_INT_INC_MODE` + 全局 `g_SysTickCount`；用户定时器中断中调 `XC_Time_TickInc()`；
2. **计数器模式**：`XC_SYS_TICK_COUNT` 宏直接取硬件计数器（只读高频调用，须为 32 位计数器）。


---

## 二、双向链表模块（XC_List）

链表是 XCOS 的底层容器：就绪表（`ReadyList`）、时间表（`TimeList`）、时间溢出表（`TimeOverflowList`）、阻塞表（`BlockedList`）都由链表构成。链表地址即链表根节点地址，根节点只是标记的普通节点。

### 2.1 节点类型

```c
typedef struct XC_ListNode_tag {
    struct XC_ListNode_tag* pNext; // 指向下个节点
    struct XC_ListNode_tag* pPrev; // 指向上个节点
} XC_ListNode_t;
```

32 位下占 8 字节。TCB 的 `ListNode` 字段作为结构**首成员**，链表节点即任务在表中的"挂载点"。

### 2.2 初始化与判断宏

```c
#define XC_List_InitNode(pNode)   do { (pNode)->pNext = (pNode); (pNode)->pPrev = (pNode); } while(0)
#define XC_List_Init(pList)       XC_List_InitNode(pList)
#define XC_List_ListValid(pList)  ((pList) != ((pList)->pNext))        // 链表是否有节点
#define XC_List_ReachEndNode(pList, pNode) ((pList) == (pNode))        // 是否到达根节点
#define XC_List_GetListStartNode(pList)    ((pList)->pNext)            // 首节点
```

节点初始化后自指（环形链表）；遍历以回到根节点为结束条件。

### 2.3 节点操作（XC_List.c）

**插入（在某节点之后）**：

```c
void XC_List_InsertNodeAfter(XC_ListNode_t* pListNode, XC_ListNode_t* pNewNode)
{
    pNewNode->pNext        = pListNode->pNext;
    pNewNode->pNext->pPrev = pNewNode;
    pNewNode->pPrev        = pListNode;
    pListNode->pNext       = pNewNode;
}
```

**移动（从原链移除并移动到某节点之后）**：

```c
void XC_List_MoveNodeAfter(XC_ListNode_t* pDestNode, XC_ListNode_t* pSrcNode)
{
    pSrcNode->pNext->pPrev = pSrcNode->pPrev;
    pSrcNode->pPrev->pNext = pSrcNode->pNext;
    pSrcNode->pNext        = pDestNode->pNext;
    pSrcNode->pNext->pPrev = pSrcNode;
    pSrcNode->pPrev        = pDestNode;
    pDestNode->pNext       = pSrcNode;
}
```

**移除**：

```c
void XC_List_Remove(XC_ListNode_t* pNode)
{
    pNode->pNext->pPrev = pNode->pPrev;
    pNode->pPrev->pNext = pNode->pNext;
    XC_List_InitNode(pNode); // 移除后节点自指,便于复用
}
```

### 2.4 链表整体移动

将一个链表全部移动到另一个链表的某节点之后，移动后源链表被清空：

```c
void XC_List_MoveListToNodeAfter(XC_ListNode_t* pDestNode, XC_ListNode_t* pSrcList)
{
    pSrcList->pPrev->pNext  = pDestNode->pNext; // 链表尾 -> 目标节点下
    pDestNode->pNext->pPrev = pSrcList->pPrev;  // 目标节点下 -> 链表尾
    pDestNode->pNext        = pSrcList->pNext;  // 目标节点下 -> 链表首
    pSrcList->pNext->pPrev  = pDestNode;        // 链表首 -> 目标节点
    XC_List_Init(pSrcList);                     // 清空源链表
}
```

典型用途：时间调度中把时间表/溢出表整表移入就绪表。

### 2.5 从节点取回容器（container_of）

```c
#define XC_LIST_TO_TCB(pNode) ((struct XC_TaskCB_tag*)(void*)(pNode))
```

TCB 的 `ListNode` 是首成员，节点地址即 TCB 地址。直接 `XC_ListNode_t*` -> `XC_TaskCB_t*` 强转违反 MISRA Rule 11.3，经 `void*` 间接转换仅触发 Rule 11.5（Advisory），纳入合规豁免。



---

## 三、时间模块（XC_Time）

### 3.1 滴答计数（Tick）

- 系统时间以 **Tick** 为单位，频率为 `XC_CFG_TICKS_PER_SEC`（默认 1000Hz = 1ms/tick）；
- 当前 Tick 经 `XC_Time_GetTick()`（= `XC_SYS_TICK_COUNT`）读取，`volatile` 保证中断累加可见；
- 中断累加模式提供 `XC_Time_TickInc()`（中断中调用，`g_SysTickCount++`）与 `XC_Time_TickSet()`（休眠唤醒后设置）。

```c
#define XC_Time_GetTickUnit() (XC_TICK_PERIOD_US)   // 一个 Tick 的最小时间(us)
#define XC_Time_GetTick()     (XC_SYS_TICK_COUNT)   // 当前 Tick
#define XC_Time_GetMs()       (XC_Time_TicksToMs(XC_Time_GetTick())) // 运行时间(ms)
```

### 3.2 时间换算（`XC_TimeInternal.h`）

按 `XC_CFG_TICKS_PER_SEC` 分两档自动选择公式：

- **档位 1**（`≤ 1000`，时基 ≥1ms）：us/ms->Tick 用**向上取整**（除周期），秒->Tick 直接乘频率；
- **档位 2**（`≤ 1000000`，时基 <1ms）：us->Tick 向上取整，ms/s->Tick 直接乘频率；
- 超出范围编译报错 `#error`。

```c
/* 档位1示例:1个tick=XC_TICK_PERIOD_MS ms */
#define XC_Time_UsToTicks(Time)  (((Time) + XC_TICK_PERIOD_MS * 1000U - 1U) / (XC_TICK_PERIOD_MS * 1000U))
#define XC_Time_MsToTicks(Time)  (((Time) + XC_TICK_PERIOD_MS - 1U) / XC_TICK_PERIOD_MS)
#define XC_Time_SecToTicks(Time) ((Time) * XC_CFG_TICKS_PER_SEC)
#define XC_Time_TicksToUs(Tick)  ((Tick) * XC_TICK_PERIOD_MS * 1000U)
#define XC_Time_TicksToMs(Tick)  ((Tick) * XC_TICK_PERIOD_MS)
#define XC_Time_TicksToSec(Tick) ((Tick) / XC_CFG_TICKS_PER_SEC)
```

> 注意：时基为 ms 级时 us 时间按 1 个 tick 处理，误差大。

### 3.3 溢出安全的比较（`XC_Time.c`）

基于无符号回绕算术（`(Now - Last) >= Compare` 判到达），Tick 溢出后仍正确：

```c
XC_Tick_t XC_Time_GetRemain(XC_Tick_t LastTick, XC_Tick_t CompareTick)   // 剩余 Tick(0=已到)
XC_Tick_t XC_Time_GetElapsed(XC_Tick_t LastTick, XC_Tick_t CompareTick)  // 已运行 Tick
```

配套宏：`XC_Time_CheckTimeout(LastTick, CompareTick)`（到达判断）、`XC_Time_CheckTimeoutMs(LastTick, Ms)` 等。**限制**：等待时长 + 查询间隔不能超过 Tick 类型值域。

### 3.4 软件定时器（`XC_TimerTick_t`）

```c
typedef struct {
    XC_Tick_t TickCount; // 起始 Tick
    XC_Tick_t WaitCount; // 等待 Tick 数
} XC_TimerTick_t;
```

| 宏/函数 | 用途 |
| --- | --- |
| `XC_Time_TimerSet(_tC, _c)` / `SetMs` / `SetSec` | 设置起始 Tick 与等待数 |
| `XC_Time_TimerRepeat(_tC)` | 以当前 Tick 重置起始点（周期重复） |
| `XC_Time_TimerCheck(_tC)` | 判断是否到达 |
| `XC_Time_TimerClr(_tC)` | 清零 |
| `XC_Time_TimerGetRemain/Elapsed(_tC)` | 剩余/已运行 Tick |
| `XC_Time_TimerGetElapsedMs(_tC)` | 已运行 ms |

### 3.5 死循环延时（`XC_Time.c`）

```c
void XC_Time_BlockDelay(XC_Tick_t DelayTick);    // 阻塞延时 n Tick
void XC_Time_BlockDelayMs(XC_Tick_t Delay_ms);   // 阻塞延时 n ms
```

内部以 `CheckTimeout` 轮询实现，会占用 CPU（非协程挂起），适用于短暂延时。



---

## 四、任务模块（XC_Task）

### 4.1 任务控制块 TCB

```c
struct XC_TaskCB_tag {
    XC_ListNode_t     ListNode;            // 链表节点(挂入就绪/时间/溢出/阻塞表)
    struct XCOS_tag* phXCOS;              // 任务所属的框架句柄
    XC_CorFn_t       fTask;               // 当前层入口(顶层或子层,协程嵌套时由 Call 切换)
    XC_BP_t    BP;                         // 当前执行层断点(唯一,顶层/子层共用)
    XC_Tick_t TaskWakeupTick;             // 任务下个唤醒的时间(时间表排序键)

    void*            pParam;      // 传递的参数
    void*            pNotifyData; // 通知数据
    volatile uint8_t TaskState;   // 任务状态(VOID/RUN/READY/BLOCKED/SUSPEND)
    volatile uint8_t NotifyState; // 通知状态(WAKEUP/WAIT)

    /* SPSC 生产-消费者计数(线程安全;只在创建任务时清0) */
    volatile uint8_t NotifyProduced; // 通知-生产者
    volatile uint8_t NotifyConsumed; // 通知-消费者

    /* 协程嵌套支持 */
    XC_CorFrame_t* pCorStack;    // 用户按需提供的帧栈(NULL=无嵌套)
    uint8_t        CorDepth;     // 当前子层深度(0=在顶层)
    uint8_t        CorDepthMax;  // 帧栈容量(越界检查)
    uint8_t        CorState;     // 本轮结果(值域:XC_CorState_t;填补对齐padding)
};
/* sizeof: 44B(32bit) */
```

**任务状态机**：

```
XC_TASK_VOID ---Reg/Add---> XC_TASK_READY ---运行---> XC_TASK_RUN
     |                            <---- 让出/完成 ---------|
     |                            |                        | 挂起(延时/等通知)
     |                            |                        v
     |                            |                XC_TASK_BLOCKED
     |                            |                        |
     |                            <唤醒(时间到/通知)-------| Suspend
     |                            |                        v
     |                            |                 XC_TASK_SUSPEND
     |                            <----- Resume -----------|
     |                            |                        |
     +<---- Remove(任意状态)----- <---- Reset(任意状态)----+
```

### 4.2 任务注册

```c
/* 常规注册 */
XC_Return_t XC_Task_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB,
                        void (*fTask)(XC_TaskHandle_t), void* pParam);

/* 扩展注册(支持协程嵌套):等价于 Reg + SetCorStack */
XC_Return_t XC_Task_RegExt(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB,
                           XC_CorFn_t fTask, void* pParam,
                           XC_CorFrame_t* pCorStack, uint8_t CorDepthMax);

/* 分步创建:先设入口,后添加 */
void        XC_Task_SetEntry(XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam);
XC_Return_t XC_Task_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB);
```

**`XC_Task_Reg` 流程**：

```c
if(phXCOS->TaskNum >= XC_CFG_MAX_TASKS) return XC_FAIL;  // 数量上限
phTCB->TaskState = XC_TASK_VOID;
phTCB->phXCOS    = phXCOS;
phTCB->fTask     = fTask;
phTCB->pParam    = pParam;
XC_Task_AddInternal(phTCB);   // BasicInit + 锁 + 插入就绪表 + READY + TaskNum++
```

**`XC_Task_RegExt`**：先校验 `pCorStack`/`CorDepthMax` **同为有/同为无**（否则 `XC_FAIL`），写帧栈配置后走 `XC_Task_Reg` 流程。**`XC_Task_SetCorStack`**：独立配置帧栈并清 `CorDepth`，传 `(NULL, 0U)` 撤销嵌套能力。

### 4.3 任务控制

| API | 行为 |
| --- | --- |
| `XC_Task_Reset(phTCB)` | 任务复位到就绪表并从头运行；TCB 除 `phXCOS`/`fTask`/`pParam` 外全部重置；不可复位自身（自身用 `XC_Cor_Reset`） |
| `XC_Task_Suspend(phTCB)` | 挂起任务（本次运行完成后暂停），优先级最高、覆盖一切阻塞；`VOID` 返回 `XC_FAIL`，已挂起返回 `XC_OK` |
| `XC_Task_Resume(phTCB)` | 恢复被挂起的任务到就绪表（原阻塞失效）；未挂起返回 `XC_FAIL` |
| `XC_Task_Remove(phTCB)` | 移除任务（出表、清 TCB、`TaskNum--`、`phXCOS=NULL`）；不可移除自身（自身用 `XC_Cor_Remove`） |
| `XC_Task_GetState(phTCB)` | 获取任务状态 |



### 4.4 阻塞处理（`XC_Task_HandleBlocking`）

延时与等待通知的最终入表函数，**时间表按唤醒时刻升序插入**：

```c
static void XC_Task_HandleBlocking(XC_TaskHandle_t phTCB, uint32_t TickCount)
{
    if(TickCount != 0U) {              // 有超时时间
        Tick = XC_Time_GetTick();
        TickCount += Tick;             // 下次唤醒时刻
        phTCB->TaskWakeupTick = TickCount;
        /* 判断是否跨 Tick 溢出,选时间表或溢出表 */
        pList = (TickCount < Tick) ? (&phTCB->phXCOS->TimeOverflowList) : (&phTCB->phXCOS->TimeList);
        XC_Core_Lock(phTCB->phXCOS);
        /* 升序插入:从首节点向后找第一个唤醒时刻 > 新节点 的位置 */
        pIterator = XC_List_GetListStartNode(pList);
        while((pIterator != pList) && (XC_LIST_TO_TCB(pIterator)->TaskWakeupTick <= TickCount)) {
            pIterator = pIterator->pNext;
        }
        XC_List_MoveNodeAfter(pIterator->pPrev, &phTCB->ListNode); // 插入到其前节点之后
    }
    else {                              // 无时间:移入阻塞表(挂起/死等)
        XC_Core_Lock(phTCB->phXCOS);
        XC_Task_MoveToBlockedList(phTCB);
    }
    XC_Core_Unlock(phTCB->phXCOS);
}
```

- **升序时间表**保证时间调度只需处理队首到期链，且任务数量超限时（时间表非空）`XC_Sch_GetNextTaskWakeupTick` 取首节点即最近唤醒；
- **溢出表**承接"唤醒时刻已回绕"的任务，Tick 回绕时由时间调度整表处理；
- **锁设计（范围最小化）**：`HandleBlocking` 内部自持锁，锁范围**只覆盖链表临界操作**（时间表升序查找 + 插入 / 移入阻塞表），Tick 计算与表选择在锁外——保证高频路径（延时/等待通知入表）持锁时间最短；
- **锁配对（隐式）**：框架锁为单一标志位（`Lock=1/0`，不支持嵌套）。`HandleDelay` 调用 `HandleBlocking` 时无外层锁，由其内部 Lock/Unlock 成对；`HandleWaitNotify` 调用时已有外层锁，外层锁须持续到入表完成（防止"解锁窗口内中断直接唤醒导致已唤醒却再次入表阻塞"，见 4.6），由 `HandleBlocking` 内部 Unlock **隐式释放**（即外层锁委托内层释放）。

### 4.5 内部处理函数（`XC_TaskInternal.h` 声明）

| 函数 | 行为 |
| --- | --- |
| `XC_Task_HandleDelay(phTCB, TickCount)` | 延时：`TickCount==0` 置就绪，否则置 `BLOCKED` 并 `HandleBlocking` |
| `XC_Task_HandleWaitNotify(phTCB, TickCount)` | 等待通知：置 `WAIT`；已有通知则直接消费置 `READY`，否则 `HandleBlocking` + `BLOCKED` |
| `XC_Task_HandleSuspend(phTCB)` | 挂起：置 `SUSPEND` + 移入阻塞表 |
| `XC_Task_HandleReset(phTCB)` | 复位：置 `READY` + 移入就绪表 + **清栈恢复最外层入口** + `BasicInit` |
| `XC_Task_HandleRemove(phTCB)` | 移除：置 `VOID` + 出表 + **清栈恢复最外层入口** + `BasicInit` + 计数减一 |
| `XC_Task_PushFrame/PopFrame` | 协程帧入栈/出栈（见 4.7） |

链表操作宏：`XC_Task_MoveToReadyList`（移到上个就绪节点后，保证最后被调度）、`XC_Task_MoveToBlockedList`、`XC_Task_RemoveNode`。

### 4.6 通知机制（SPSC，线程安全）

通知为任务间/中断到任务间同步的唯一机制，**SPSC 模型**（每任务一个生产者 + 一个消费者）。

```c
XC_Return_t XC_Task_SendNotify(XC_TaskHandle_t phTCB, void* pNotifyData);
```

**发送流程**：

1. `TaskState` 为 `VOID`/`SUSPEND` -> 返回 `XC_FAIL`；
2. 写 `pNotifyData`；
3. 任务**非等待通知**：仅 `NotifyProduced++`（满则丢弃），返回 `XC_CONTINUE`——之后 `HandleWaitNotify` 会直接消费；
4. 任务**等待通知**且**调度运行中**（中断场景，`Lock==1`）：`NotifyProduced++` 且 `EventProduced++` 触发异步调度，返回 `XC_CONTINUE`；
5. 任务**等待通知**且**调度未运行**：加锁移动任务到就绪表、置 `WAKEUP`/`READY`，返回 `XC_OK`。

**等待流程（`HandleWaitNotify`）**：置 `WAIT` 后检查 `Produced != Consumed`——已有通知则立即消费置 `READY`（防止中断在置 `WAIT` 前发送的通知丢失）；否则阻塞等待，由事件调度或直接路径唤醒。

**并发保证**：生产者（中断）与消费者（调度器）经 `volatile` 计数 + 框架锁协作；单生产者约束下无竞争。**注意**：框架不关中断、不做原子操作，若出现多生产者需用户加锁；CPU 内存重排序时用户需内存屏障。

**锁设计说明（`HandleWaitNotify` 的隐式解锁）**：
- 进入函数时 `XC_Core_Lock`（外层锁），else 分支（无通知 -> 阻塞）**不显式释放**，由 `HandleBlocking` 内部 `Unlock` 隐式释放——这是**有意的性能设计**：外层锁须持续到任务入表完成，否则"检查无通知 -> 解锁 -> 中断 `SendNotify`（`Lock==0` 走直接唤醒路径，把任务移回就绪表并置 `READY`）-> `HandleBlocking` 又将其移入时间表"的竞态会重现，造成"已唤醒却再次阻塞、通知延迟"；
- 锁为单一标志位（`Lock=1/0`，见第七章），**不支持嵌套**，故外层锁的释放委托内层 `HandleBlocking` 的 `Unlock`（隐式配对）；若未来 `HandleBlocking` 的锁结构改变，须同步检查此处的隐式释放假设；
- `HandleDelay` 路径无外层锁，`HandleBlocking` 内部锁独立成对，锁范围最小（仅保护链表操作，见 4.4）。

### 4.7 协程帧栈操作（嵌套支撑，`XC_Task.c`）

**入栈 `XC_Task_PushFrame`**（仅 `XC_Cor_Call` 调用）：

```c
void XC_Task_PushFrame(XC_TaskHandle_t phTCB, XC_CorFn_t fn, XC_BP_t RetLine)
{
    if(phTCB->CorDepth >= phTCB->CorDepthMax) { // 越界保护:未配栈调用 / 嵌套过深
        XC_Task_HandleReset(phTCB);             // 复位任务安全降级
        return;
    }
    XC_CorFrame_t* pf = &phTCB->pCorStack[phTCB->CorDepth];
    pf->pfn      = phTCB->fTask;   // 父层入口(弹帧恢复用;栈底帧恒为顶层任务)
    pf->BP       = RetLine;        // 父层返回点(ANSI:行号 / GNU:标签地址)
    phTCB->fTask = fn;             // 入口切换为子函数
    COR_Init(phTCB->BP);           // 子层首次从头
    phTCB->CorDepth++;
}
```

**出栈 `XC_Task_PopFrame`**（调度器同轮切父调用）：

```c
void XC_Task_PopFrame(XC_TaskHandle_t phTCB)
{
    phTCB->CorDepth--;                                    // 先减后取
    phTCB->fTask = phTCB->pCorStack[phTCB->CorDepth].pfn; // 恢复父层入口
    phTCB->BP    = phTCB->pCorStack[phTCB->CorDepth].BP;  // 恢复父层返回点
}
```

**清栈恢复最外层**（`HandleReset`/`HandleRemove`/越界保护共用）：先判 `CorDepth > 0U` 再读 `pCorStack[0].pfn` 恢复 `fTask`，随后 `CorDepth = 0U`。**栈底帧 `[0].pfn` 恒为最外层任务函数**（只在第一次 `Call` 时写入，此后不再改写），因此任意深度触发复位/移除都能正确回到顶层。



---

## 五、协程机制（XC_Cor 与底层）

### 5.1 协程块模型

任务函数用 `XC_Cor_Enter` / `XC_Cor_Leave` 包成协程块，块内可放置挂起点。协程块是状态机：每次被调度从函数头进入，由底层按断点 `switch`/`goto` 到上次挂起处继续。

```c
#define XC_Cor_Enter(phTCB)                            \
    {                                                  \
        XC_TaskHandle_t phXCCorTCB = (phTCB);          \
        XC_BP_t*         pXCCorPB   = &phXCCorTCB->BP;  \
        phXCCorTCB->CorState = XC_COR_SUSPENDED;       \
        COR_Start(*pXCCorPB)
```

```c
#define XC_Cor_Leave()                      \
        phXCCorTCB->CorState = XC_COR_DONE; \
        COR_End(); /*结束*/                 \
    }
```

**`CorState` 语义**（调度器同轮切父的依据）：

- `XC_Cor_Enter` 置 `SUSPENDED`（"本轮是否完成" = "是否执行到 `Leave`"）；
- `XC_Cor_Leave` 置 `DONE`，**必须在 `COR_End()` 之前**——挂起路径经底层 `goto` 跳出时跳过 `DONE` 赋值，保持 `SUSPENDED`；
- 挂起/复位/移除宏经底层 `goto COR_GOTO_END` 跳出（标签在 `DONE` 赋值之后），天然跳过 `DONE`。

### 5.2 协程宏体系

| 宏 | 用途 | 底层原语 |
| --- | --- | --- |
| `XC_Cor_Enter` / `XC_Cor_Leave` | 协程块边界 | `COR_Start` / `COR_End` |
| `XC_Cor_DelayTick/Us/Ms/Sec` | 延时 | `XC_Task_HandleDelay` + `COR_SetBPBreak` |
| `XC_Cor_WaitNotify(Ms)` | 等待通知（`0` 死等） | `XC_Task_HandleWaitNotify` + `COR_SetBPBreak` |
| `XC_Cor_Yield()` | 让出 CPU | 置 `READY` + `COR_SetBPBreak` |
| `XC_Cor_Suspend()` | 挂起自身 | `XC_Task_HandleSuspend` + `COR_SetBPBreak` |
| `XC_Cor_Reset()` / `XC_Cor_Remove()` | 复位/移除自身 | `XC_Task_HandleReset`/`HandleRemove` + `COR_Break` |
| `XC_Cor_Call(fn)` | 嵌套登记子协程 | `XC_Task_PushFrame` + `COR_BreakLabel` |
| `XC_Cor_GetParam/ClrNotify/IsNotifyTimeout/GetNotifyData` | 参数/通知工具 | — |

### 5.3 协程嵌套（`XC_Cor_Call`）

```c
#define XC_Cor_Call(fn)                         \
    {                                           \
        XC_Task_PushFrame(phXCCorTCB, (fn), COR_RetPoint()); \
        COR_BreakLabel();                       \
    }
```

**机制**：不做真实 C 嵌套调用（C 栈 O(1)），`Call` 只登记子函数并退出：

1. `PushFrame`：帧存 `{父层入口=fTask, 返回点=COR_RetPoint()}`，`fTask` 切为子函数 `fn`，清 `BP`；
2. `COR_BreakLabel`：`goto` 跳出协程块（保持 `SUSPENDED`），放置返回标签；
3. 下一轮调度器调 `fTask`（子函数）进入子层（**活动层直达**）；
4. 子层完成 -> 调度器弹帧恢复父层入口+返回点，**同轮切父**继续运行父层。

**要点**：
- 需要嵌套的任务必须用 `XC_Task_RegExt`/`XC_Task_SetCorStack` 配置帧栈（`pCorStack`/`CorDepthMax` 同为有/同为无）；未配栈调用 `Call` 触发越界保护复位任务；
- `Call` 是**必挂起点**：本层在调用处退出、之后从函数头重新进入，调用前后局部变量一律不保存；
- 宏自带 `{}`，可写在 `if`/`while`/`for` 语句体内（ANSI 的 `case __LINE__:` 在宏内复合语句中合法）。

### 5.4 底层选型

```c
#ifdef __GNUC__
#include "Internal/XC_CorGNU.h" // GNU-C 库(computed goto)
#else
#include "Internal/XC_CorANSI.h" // ANSI-C 库(switch/case)
#endif
```

- armclang 恒定义 `__GNUC__`（即使 `-std=c99`）-> 恒走 GNU 底层；ANSI 路径仅 armcc 不带 `--gnu` 时触发；
- GNU 版约束：每个文件代码行数不超过 65535（标签由 `__func__`+`__LINE__` 组合）。


### 5.5 ANSI 底层（`XC_CorANSI.h`，switch/case + goto）

断点值为**行号**（`unsigned long`）：

```c
typedef unsigned long COR_BP_t;

#define COR_Init(BP) ((BP) = 0UL)

#define COR_Start(BP) \
    switch((BP)) {    \
        case 0:

#define COR_SetBP(BP)                 \
    (BP) = (unsigned long)(__LINE__); \
    case __LINE__:;

#define COR_Break(BP) \
    goto COR_GOTO_END;

#define COR_SetBPBreak(BP)            \
    (BP) = (unsigned long)(__LINE__); \
    goto COR_GOTO_END;                \
    case __LINE__:;

#define COR_End() \
    default:      \
        break;    \
        }         \
    COR_GOTO_END: /*结束跳转标志*/

#define COR_BreakLabel() \
    {                    \
        goto COR_GOTO_END; \
        case __LINE__:;  \
    }

#define COR_RetPoint() ((COR_BP_t)(__LINE__))
```

**工作机制**：

- **恢复**：`COR_Start` 展开为 `switch(BP) { case 0:`，`BP` 为上次挂起行号时直接跳到对应 `case` 继续；`BP==0` 从函数头执行；
- **挂起**：`COR_SetBPBreak` 先存行号再 `goto COR_GOTO_END` 跳出——**不用 `break`**。协程块内可含 `for(;;)`/`while(1)` 循环体，`break` 在循环体内跳出的是**循环**而非协程 `switch`，控制流会落入 `XC_Cor_Leave` 执行 `DONE`，把挂起误标为完成；`goto` 直接跳到 `switch` 之外、`DONE` 赋值之后的 `COR_GOTO_END` 标签，必然跳过 `DONE`；
- **`COR_GOTO_END`**：`COR_Break`/`COR_SetBPBreak`/`COR_BreakLabel` 的统一跳出目标；标签后空语句由上层 `COR_End();` 分号提供；
- **`default` 分支**：断点值异常兜底，同时满足 MISRA Rule 16.4；
- **`COR_BreakLabel`**：与 `SetBPBreak` 同构但不设断点（返回点已入帧）；**`COR_RetPoint`**：返回当前行号供入栈。

### 5.6 GNU 底层（`XC_CorGNU.h`，computed goto）

断点值为**标签地址**（`void*`）：

```c
typedef void* COR_BP_t;

#define COR_BP2(S1, S2) S1##S2
#define COR_BP(S1, S2)  COR_BP2(S1, S2)   // 生成标签 <函数名><行号>(## 需两次展开)

#define COR_Init(BP)    ((BP) = NULL)

#define COR_Start(BP) \
    { \
        (void)(&&COR_ANCHOR);  /* 锚点取地址(满足 goto* 的 &&label 约束,零指令) */ \
        COR_ANCHOR:;           /* 锚点标签定义(每函数仅一个 Enter,天然唯一) */ \
        if(BP != NULL) { goto* BP; } \
    }

#define COR_SetBP(BP)                        \
    {                                        \
        (BP) = &&COR_BP(__func__, __LINE__); \
        COR_BP(__func__, __LINE__) :;        \
    }

#define COR_Break(BP) \
    goto COR_GOTO_END;

#define COR_SetBPBreak(BP)                      \
    {                                           \
        (BP) = &&COR_BP(__func__, __LINE__);    \
        goto COR_GOTO_END;                      \
        COR_BP(__func__, __LINE__) :;           \
    }

#define COR_End() \
    COR_GOTO_END: /*结束跳转标志*/

#define COR_BreakLabel() \
    {                    \
        goto COR_GOTO_END; \
        COR_BP(__func__, __LINE__) :; \
    }

#define COR_RetPoint() (&&COR_BP(__func__, __LINE__))
```

**工作机制**：

- 每个挂起点/返回点生成唯一标签 `COR_BP(__func__, __LINE__)`；`COR_Start` 检查 `BP != NULL` 时 `goto* BP` 直达上次挂起标签，`BP==NULL` 顺序执行；
- **锚点标签 `COR_ANCHOR`**：`goto*` 要求函数内至少一个 `&&label` 表达式；纯逻辑协程（无挂起/`Call`/`Reset` 点）函数内没有，编译报错 `indirect goto in function with no address-of-label expressions`，锚点提供该表达式且零运行时开销；
- 跳出语义与 ANSI 相同（`goto COR_GOTO_END` 跳过 `DONE`）。

### 5.7 断点值与恢复标签的匹配

`COR_SetBPBreak`（设断点+标签）、`COR_RetPoint`/`COR_BreakLabel` 组合（`Call` 的返回点+返回标签）都位于 `XC_Cor.h` 的**同一个外层宏体内**——用户调用一次外层宏，预处理时内嵌 `__LINE__` 展开为同一用户行号、`__func__` 在同一函数内求值，断点值与恢复标签必然匹配。

### 5.8 MISRA 合规

- `goto COR_GOTO_END; case __LINE__:` 是**顺序流不可达标签**（协程恢复跳转目标，非死代码）；
- ANSI 版 `goto`（前向跳转至函数末尾、跳过 `DONE` 赋值，cleanup 模式）与 GNU 同类，纳入 Rule 15.1 Deviation（DEV-XCOS-001，见 `Docs/MISRA-C/MISRA-C_2025_Deviation.md`）；
- ANSI `default` 满足 Rule 16.4。



---

## 六、调度器（XC_Sch）

### 6.1 初始化 `XC_Sch_Init`

```c
void XC_Sch_Init(XC_OSHandle_t phXCOS)
{
    phXCOS->Lock = 1U;                 // 初始即锁,防止未初始化时被中断调度
    phXCOS->pPrevReadyNode = &phXCOS->ReadyList;  // 上个就绪节点指向根
    phXCOS->PrevTick       = 0U;                  // 上个 Tick
    phXCOS->TaskNum        = 0U;
    phXCOS->EventProduced  = 0U;      // 通知/挂起/恢复的异步调度触发计数
    phXCOS->EventConsumed  = 0U;
    phXCOS->fIdle          = NULL;
    XC_List_Init(&phXCOS->ReadyList);
    XC_List_Init(&phXCOS->TimeList);
    XC_List_Init(&phXCOS->TimeOverflowList);
    XC_List_Init(&phXCOS->BlockedList);
}
```

### 6.2 主循环 `XC_Sch_Start`

```c
void XC_Sch_Start(XC_OSHandle_t phXCOS)
{
    pIterator = XC_List_GetListStartNode(&phXCOS->ReadyList);
    for(;;) {
        if(XC_List_ListValid(&phXCOS->ReadyList)) {
            if(XC_List_ReachEndNode(&phXCOS->ReadyList, pIterator)) {
                pIterator = pIterator->pNext;          // 到达根节点则向前推进
            }
            phXCOS->pPrevReadyNode = pIterator->pPrev; // 记录上个就绪节点
            phTCB = XC_LIST_TO_TCB(pIterator);
            pIterator = pIterator->pNext;              // 先推进再运行(任务可能移动节点)
            phTCB->TaskState = XC_TASK_RUN;
            phTCB->fTask(phTCB);                       // 恒调当前层(顶层或子层)
            /* 同轮切父:当前层完成(DONE)且栈非空时,弹帧并立即运行父层 */
            while((phTCB->CorState == XC_COR_DONE) && (phTCB->CorDepth > 0U)) {
                XC_Task_PopFrame(phTCB);
                phTCB->fTask(phTCB);
            }
        }
        XC_Sch_TimeSched(phXCOS);                      // 时间调度
        Trigger = phXCOS->EventProduced;               // 事件调度(通知异步触发)
        if(Trigger != phXCOS->EventConsumed) {
            phXCOS->EventConsumed = Trigger;
            XC_Sch_EventSched(phXCOS);
        }
        /* 空闲处理:就绪表空时计算可休眠 Tick 并回调 fIdle */
        if((XC_List_ListValid(&phXCOS->ReadyList) == 0) && (phXCOS->fIdle != NULL)) {
            Tick = XC_Time_GetTick();
            if(XC_List_ListValid(&phXCOS->TimeList))       Tick = XC_Sch_GetNextTaskWakeupTick() - Tick;
            else if(XC_List_ListValid(&phXCOS->TimeOverflowList)) Tick = 溢出表首节点唤醒时刻 - Tick;
            else                                            Tick = ~0U;
            phXCOS->fIdle(phXCOS, Tick);
        }
    }
}
```

**任务运行语义**（调度器在每层函数返回后读 `TCB.CorState`）：

| `CorState` | 调度器行为 |
| --- | --- |
| `SUSPENDED` | 任务已被 Handle* 移出就绪表（或 Yield 留在就绪表），本轮结束 |
| `DONE` 且栈空 | 任务完成一轮，留在就绪表；下次从 `BP` 断点继续 |
| `DONE` 且栈非空 | 弹帧后立即运行父层（同轮切父） |

> **为什么同轮切父用 `while`**：存在"层层立即完成"的级联场景，须继续弹帧直到某层 `SUSPENDED` 或回到顶层；循环有界（每轮 `PopFrame` 深度递减）。

### 6.3 时间调度 `XC_Sch_TimeSched`

每轮主循环调用，处理时间表与溢出表：

1. **Tick 溢出处理**（`PrevTick > Tick`）：
   - 时间表所有节点状态置 `READY` 并整表移入就绪表；
   - 溢出表所有节点移入时间表；
   - 更新 `PrevTick`；
2. **时间表唤醒**（时间表有效且 `Tick >= 队首唤醒时刻`）：从首节点向后轮询，唤醒时刻已到达的任务移入就绪表（**先推进迭代器再移动节点**，避免指针失效），遇未到期或表尾停止。

```c
static void XC_Sch_TimeSched(XC_OSHandle_t phXCOS)
{
    XC_Core_Lock(phXCOS);
    if(phXCOS->PrevTick > Tick) {                 // 1. 溢出
        /* 时间表 -> 就绪表;溢出表 -> 时间表 */
        ...
    }
    if(时间表有效 && Tick >= XC_Sch_GetNextTaskWakeupTick()) { // 2. 唤醒
        while(Tick >= XC_LIST_TO_TCB(pIterator)->TaskWakeupTick) {
            phTCB = XC_LIST_TO_TCB(pIterator);
            pIterator = pIterator->pNext;          // 先推进
            XC_Task_MoveToReadyList(phTCB);
            phTCB->TaskState = XC_TASK_READY;
            if(XC_List_ReachEndNode(pTimeList, pIterator)) break;
        }
        phXCOS->PrevTick = Tick;
    }
    XC_Core_Unlock(phXCOS);
}
```

### 6.4 事件调度 `XC_Sch_EventSched`

由 `EventProduced != EventConsumed` 触发（中断中 `SendNotify` 等异步操作登记），在调度器上下文中执行实际唤醒。用**局部宏 `WAKEUP_NOTIFY(pCList)`** 遍历时间表/溢出表/阻塞表，将"等待通知且需要消费"的任务消费通知并移入就绪表（先推进迭代器再操作节点）。宏设计为热路径优化：内联展开避免函数调用开销，作用域仅限本函数。

### 6.5 空闲处理与休眠

就绪表为空时计算"距最近唤醒的 Tick 数"并回调 `fIdle(phXCOS, IdleTick)`：时间表/溢出表有节点 -> 队首唤醒时刻差；两表皆空 -> `~0U`（可按最大时间休眠）。回调内可休眠系统，唤醒后需按文档更新 Tick（`XC_Time_TickSet`）。



---

## 七、内核基础：锁（XC_Core）

框架用**框架实例锁**保护链表操作与状态切换，防止中断调用（如 `SendNotify`）与调度器并发时的资源竞争。

```c
#define XC_Core_Lock(phXCOS)     do { ((struct XCOS_tag*)(phXCOS))->Lock = 1U; } while(0)
#define XC_Core_Unlock(phXCOS)   do { ((struct XCOS_tag*)(phXCOS))->Lock = 0U; } while(0)
#define XC_Core_GetLockState(phXCOS) (((struct XCOS_tag*)(phXCOS))->Lock)
```

**使用方式**：

- 中断上下文（`SendNotify` 等）持锁时：只更新生产者计数并登记异步事件（`EventProduced++`），**不直接操作链表**——实际唤醒由调度器在 `EventSched` 中完成，避免中断与主循环的链表竞争；
- 调度器上下文（任务注册/阻塞/唤醒等）：加锁后直接操作链表；
- 锁是普通 `volatile` 标志（非原子），**主循环持锁期间若中断不访问共享链表则安全**；系统设计为单核中断场景。

---

## 八、内存开销

### TCB 开销

| 项 | 数值 |
| --- | --- |
| TCB 大小 | **44B**（32bit；嵌套字段 7B + 1B 对齐 padding，整体 4B 对齐） |
| 无嵌套任务 | 44B TCB + **0B 帧栈**（pCorStack=NULL） |
| 2 层嵌套任务 | 44B + 16B 帧栈（2 帧 × 8B） |
| 4 层嵌套任务 | 44B + 32B 帧栈（4 帧 × 8B） |
| 100 任务混合(60无/30二层/10四层) | 4400 + 480 + 320 = 5.2KB |

### 各模块数据

| 模块 | 内存 |
| --- | --- |
| `XCOS_tag`（框架实例） | 48B（32bit） |
| `XC_ListNode_t`（链表节点） | 8B |
| `XC_CorFrame_t`（协程帧） | 8B |
| `XC_TimerTick_t`（软件定时器） | 2 × Tick 大小（默认 8B） |

### 运行时开销

- 帧栈操作 O(1)：`Call` 入 1 帧、切父弹 1 帧；
- 同轮切父循环每轮深度递减，有界；
- 时间表升序插入 O(n)、唤醒 O(1)~O(n)（只处理队首到期链）；
- GNU `COR_Start` 锚点 `(void)(&&COR_ANCHOR)` 编译器优化为无指令。

---

## 九、边界与限制

1. **协作式、无抢占**：任务必须主动让出；长时间运行会阻塞其他任务（看门狗需在任务中喂狗）；
2. **无关中断/无原子操作**：任务同步仅支持 SPSC 通知；多生产者需用户加锁；CPU 内存重排序需用户内存屏障；
3. **最大任务数**：受 `XC_CFG_MAX_TASKS` 限制（10~255，默认 100），超出返回 `XC_FAIL`；
4. **Tick 值域**：`延时 + 查询间隔`不能超过 Tick 类型值域（默认 32 位约 49 天 @1ms）；
5. **局部变量不保存**：协程挂起时函数局部变量丢失，需跨挂起保留的数据存 TCB/静态区；
6. **协程嵌套**：
   - 深度超限 / 未配栈调用 `Call` -> `PushFrame` 越界保护**复位任务**（安全降级）；
   - `pCorStack`/`CorDepthMax` 是用户配置，`Reset`/`Remove`/`BasicInit` 只清 `CorDepth` 不清配置；
   - `XC_Task_SetEntry` 不动帧栈——嵌套中换入口会使栈底帧与新入口不一致，换前须先清栈；
   - `Call` 是必挂起点，调用前后局部变量一律不保存；
7. **`fTask` 可变**：`Call` 切换为子层入口，复位/移除从栈底帧 `[0].pfn` 恢复顶层；
8. **MISRA**：ANSI 版 `goto`（Rule 15.1）纳入 DEV-XCOS-001 豁免；`goto + case __LINE__` 为顺序流不可达标签（恢复跳转目标，非死代码）；
9. **GNU 文件行数**：GNU 版协程库要求每文件 ≤ 65535 行。

---

## 十、使用示例

### 10.1 最小系统（调度器 + 单任务）

```c
#include "XCOS.h"

static struct XCOS_tag   s_XCOS;   // 框架实例
static struct XC_TaskCB_tag s_Task; // 任务 TCB

static void TaskBlink(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    for(;;) {
        XC_Cor_DelayMs(10);
    }
    XC_Cor_Leave();
}

int main(void)
{
    /* 中断中:每 1ms 调用 XC_Time_TickInc() */
    XC_Sch_Init(&s_XCOS);
    XC_Task_Reg(&s_XCOS, &s_Task, TaskBlink, NULL);
    XC_Sch_Start(&s_XCOS);   // 阻塞,不返回
}
```

### 10.2 嵌套协程（子协程内延时/等通知）

```c
static XC_CorFrame_t s_TaskStack[3];   // 帧栈:容量3(顶层不占栈)

void HeartBeat(XC_TaskHandle_t phTCB)  // 子协程
{
    XC_Cor_Enter(phTCB);
    XC_Cor_DelayMs(20);
    XC_Cor_WaitNotify(100);
    XC_Cor_Leave();
}

void TaskDemo(XC_TaskHandle_t phTCB)   // 顶层任务
{
    XC_Cor_Enter(phTCB);
    for(;;) {
        XC_Cor_Call(HeartBeat);
    }
    XC_Cor_Leave();
}

XC_Task_RegExt(&s_XCOS, &s_Task, TaskDemo, NULL, s_TaskStack, 3);
```

### 10.3 通知使用（SPSC）

```c
/* 中断中:通知任务,携带数据 */
XC_Task_SendNotify(&s_Task, (void*)0x1234);

/* 任务内:等待通知 */
XC_Cor_Enter(phTCB);
XC_Cor_ClrNotify();
XC_Cor_WaitNotify(1000);             // 等 1000ms,期间可被通知唤醒
if(XC_Cor_IsNotifyTimeout()) {
    /* 超时处理 */
} else {
    void* data = XC_Cor_GetNotifyData();
}
XC_Cor_Leave();
```

### 10.4 软件定时器（非协程场景）

```c
XC_TimerTick_t tmr;
XC_Time_TimerSet(&tmr, XC_Time_MsToTicks(500));  /* 设置 500ms */
while(1) {
    if(XC_Time_TimerCheck(&tmr)) {
        XC_Time_TimerRepeat(&tmr);               /* 周期重复 */
        /* 周期任务 */
    }
}
```

---

*本文档为 XCOS V2.1.0 整个系统的完整实现说明；版本记录见 `Docs/CHANGELOG.md`，用户手册见 `Docs/XCOS.md`。*

