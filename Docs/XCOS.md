# XCOS API 参考手册

<div align="center">

![API](https://img.shields.io/badge/API-参考手册-blue.svg)
![版本](https://img.shields.io/badge/版本-2.1.1-green.svg)
![标准](https://img.shields.io/badge/标准-ANSI/GNU-orange.svg)

**XCOS 框架完整 API 参考：数据类型、常量枚举、函数与宏定义（V2.1.1）**

[基础定义](#-基础定义) • [类型系统](#-类型系统) • [常量枚举](#-常量枚举) • [二进制模式](#-二进制模式) • [函数参考](#-函数参考) • [调用等级](#-函数调用等级) • [向后兼容](#-向后兼容)

</div>

## 🏗️ 架构概览

XCOS 为**协作式协程操作系统**：任务以函数形式编写，内部用协程块（`XC_Cor_*` 宏）表达执行流程；调度器在主循环中轮询就绪表，逐个运行任务。系统由**调度器（XC_Sch）**、**任务（XC_Task）**、**协程块（XC_Cor）**、**时间（XC_Time）** 与底层**链表（XC_List）**、**内核锁（XC_Core）** 组成；另有**默认关闭**的**诊断能力**（错误上报 / 断言 / 运行期自检 / 运行统计）。

**API 总览**（**纯 V2.1.1**；每项一句话说明；完整用法/参数见下文各节）：

| 分类                                                              | 项                          | 说明                                                                                                                                                                              |
| ----------------------------------------------------------------- | --------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **类型（`XC_Type.h`）**                                           | `XC_Tick_t`                 | 系统 Tick 计数类型（固定 `uint32_t`，1KHz下 1 tick = 1 ms）                                                                                                                       |
|                                                                   | `XC_TimerTick_t`            | 软件定时器计数（`TickCount` + `WaitCount`）                                                                                                                                       |
|                                                                   | `XCOS_t`                    | 框架实例（不透明，44 字节）                                                                                                                                                       |
|                                                                   | `XC_OSHandle_t`             | 框架句柄（`XCOS_t*`）                                                                                                                                                             |
|                                                                   | `XC_TaskCB_t`               | 任务控制块 TCB（不透明，44 字节）                                                                                                                                                 |
|                                                                   | `XC_TaskHandle_t`           | 任务句柄（`XC_TaskCB_t*`）                                                                                                                                                        |
|                                                                   | `XC_CorFrame_t`             | 协程帧（用户按需定义帧栈数组；**字段由框架内部维护**）                                                                                                                            |
|                                                                   |                             |                                                                                                                                                                                   |
| **枚举（取值见「常量枚举」节）**                                  | `XC_Return_t`               | 函数返回值（`XC_OK` / `XC_FAIL` / `XC_CONTINUE`）                                                                                                                                 |
|                                                                   | `XC_TaskState_t`            | 任务状态（`VOID` / `RUN` / `READY` / `BLOCKED` / `SUSPEND`）                                                                                                                      |
|                                                                   | `XC_ErrCode_t`              | 错误码（`XC_ERR_NONE` / `XC_ERR_FRAME_OVERFLOW` / `XC_ERR_ASSERT`）                                                                                                               |
|                                                                   | `XC_ChkResult_t`            | 自检结果（`XC_CHK_OK` 或首个失败项编号 1~6）                                                                                                                                      |
|                                                                   |                             |                                                                                                                                                                                   |
| **版本与标识宏（`XCOS.h`）**                                      | `XCOS_API_LEVEL`            | API 兼容性标识（= 主版本号，用于版本检查）                                                                                                                                        |
|                                                                   | `XCOS_VER_MAJOR`            | 主版本号（当前 `2`）                                                                                                                                                              |
|                                                                   | `XCOS_VER_MINOR`            | 次版本号（当前 `1`）                                                                                                                                                              |
|                                                                   | `XCOS_VER_PATCH`            | 修订号（当前 `0`）                                                                                                                                                                |
|                                                                   | `XCOS_VER_STRING`           | 字符串版本（`"2.1.1"`）                                                                                                                                                           |
|                                                                   | `XCOS_VER_NUMBER`           | 24 位版本数值（`0x020101`）                                                                                                                                                       |
|                                                                   |                             |                                                                                                                                                                                   |
| **配置（`XC_Config.h`，编译期定义；用户按需修改）**               | `XC_CFG_MAX_TASKS`          | 最大任务数（默认 100；**上限 255 由编译期断言拦截**，建议 ≥10；超上限时注册返回 `XC_FAIL`）                                                                                      |
|                                                                   | `XC_CFG_TICKS_PER_SEC`      | Tick 频率（默认 1000 Hz；≤1000 时必须是 1000 的因子，否则编译报错；>1000 Hz 走 µs 时基，**建议取 1000000 的因子**）；**档位推荐表 / 回绕周期 / 选档三步见 `Docs/XC_Config.md`** |
|                                                                   | `XC_SYS_TICK_COUNT`         | Tick 取值来源（默认中断累加的 `g_SysTickCount`；可指向硬件计数器）                                                                                                                |
|                                                                   | `XC_CFG_ERR_HOOK`           | 错误上报钩子开关（默认 0 = 关；关档 0 代码 / 0 RAM）                                                                                                                              |
|                                                                   | `XC_CFG_ASSERT`             | 断言开关（默认 0；**须同时打开钩子**，否则编译报错）                                                                                                                              |
|                                                                   | `XC_CFG_DEBUG_CHECK`        | 运行期自检开关（默认 0 = 关）                                                                                                                                                     |
|                                                                   | `XC_CFG_TASK_STATS`         | 每任务运行统计开关（默认 0；打开后每个任务多 8 字节 RAM）                                                                                                                         |
|                                                                   |                             |                                                                                                                                                                                   |
| **调度 `XC_Sch`（4）**                                            | `XC_Sch_Init`               | 初始化框架（建四张表、置锁、清任务数与事件标志）                                                                                                                                  |
|                                                                   | `XC_Sch_Start`              | 启动调度器（主循环，阻塞不返回）                                                                                                                                                  |
|                                                                   | `XC_Sch_GetTaskNum`         | 取当前已注册的任务数                                                                                                                                                              |
|                                                                   | `XC_Sch_SetIdleCallback`    | 设置空闲回调（就绪表为空时调用；`IdleTick` = **建议休眠上限**，`0` = **不要休眠**、`~0U` = 无待唤醒任务）                                                                         |
|                                                                   |                             |                                                                                                                                                                                   |
| **任务 `XC_Task`（15）**                                          | `XC_Task_Reg`               | 注册任务并挂到就绪表尾（重复注册 / 超上限返回 `XC_FAIL`）                                                                                                                         |
|                                                                   | `XC_Task_RegExt`            | 注册并配置协程帧栈（= `Reg` + `SetCorStack`）                                                                                                                                     |
|                                                                   | `XC_Task_SetCorStack`       | 设置或撤销协程帧栈（并把深度清 0）                                                                                                                                                |
|                                                                   | `XC_Task_Remove`            | 移除任务（出表、清 TCB、任务数 −1；不可移除自身）                                                                                                                                |
|                                                                   | `XC_Task_SetEntry`          | 设置任务入口与参数（嵌套中换入口会清栈）                                                                                                                                          |
|                                                                   | `XC_Task_Add`               | 分步创建：把已设入口的任务挂入就绪表尾                                                                                                                                            |
|                                                                   | `XC_Task_Reset`             | 复位任务（回就绪表尾、从头运行；不可复位自身）                                                                                                                                    |
|                                                                   | `XC_Task_Suspend`           | 挂起任务（本轮运行结束后暂停，覆盖一切阻塞状态）                                                                                                                                  |
|                                                                   | `XC_Task_Resume`            | 恢复被挂起的任务（并交付挂起期间已登记的通知）                                                                                                                                    |
|                                                                   | `XC_Task_SendNotify`        | 发送通知并唤醒（SPSC，**可在中断中调用**）                                                                                                                                        |
|                                                                   | `XC_Task_ClrNotify`         | 清除通知标志（消费侧；**勿在中断中调用**）                                                                                                                                        |
|                                                                   | `XC_Task_UpdateNotifyData`  | 只更新通知数据，不唤醒                                                                                                                                                            |
|                                                                   | `XC_Task_ReadNotifyData`    | 读取通知数据                                                                                                                                                                      |
|                                                                   | `XC_Task_GetParam`          | 取注册时传入的参数（`void*`）                                                                                                                                                     |
|                                                                   | `XC_Task_GetState`          | 取任务状态（`XC_TaskState_t`）                                                                                                                                                    |
|                                                                   |                             |                                                                                                                                                                                   |
| **协程块 `XC_Cor`（17，均须在协程块内）**                         | `XC_Cor_Enter`              | 进入协程块（与 `Leave` 配对）                                                                                                                                                     |
|                                                                   | `XC_Cor_Leave`              | 离开协程块（标记本层完成）                                                                                                                                                        |
|                                                                   | `XC_Cor_Call`               | 嵌套调用子协程（需配帧栈；**必挂起点**）                                                                                                                                          |
|                                                                   | `XC_Cor_GetParam`           | 协程块内取任务参数                                                                                                                                                                |
|                                                                   | `XC_Cor_Yield`              | 让出 CPU，下一轮从此继续                                                                                                                                                          |
|                                                                   | `XC_Cor_Suspend`            | 挂起自身（只能由 `XC_Task_Resume` 恢复）                                                                                                                                          |
|                                                                   | `XC_Cor_Reset`              | 复位自身（丢弃断点，从头运行）                                                                                                                                                    |
|                                                                   | `XC_Cor_Remove`             | 移除自身                                                                                                                                                                          |
|                                                                   | `XC_Cor_DelayTick`          | 延时 n 个 Tick                                                                                                                                                                    |
|                                                                   | `XC_Cor_DelayUs`            | 延时 us（Tick 时基 ≥us 时才准确）                                                                                                                                                |
|                                                                   | `XC_Cor_DelayMs`            | 延时 ms                                                                                                                                                                           |
|                                                                   | `XC_Cor_DelaySec`           | 延时 s                                                                                                                                                                            |
|                                                                   | `XC_Cor_WaitNotify`         | 等待通知（Tick 超时；0 = 死等）                                                                                                                                                   |
|                                                                   | `XC_Cor_WaitNotifyMs`       | 等待通知（ms 超时）                                                                                                                                                               |
|                                                                   | `XC_Cor_ClrNotify`          | 清除通知（等待前调用，防"提前到达"）                                                                                                                                              |
|                                                                   | `XC_Cor_IsNotifyTimeout`    | 判断本次唤醒是否超时                                                                                                                                                              |
|                                                                   | `XC_Cor_GetNotifyData`      | 取通知携带的数据                                                                                                                                                                  |
|                                                                   |                             |                                                                                                                                                                                   |
| **时间 `XC_Time`（21）**                                          | `XC_Time_GetTickUnit`       | Tick 的最小时间单位（us）                                                                                                                                                         |
|                                                                   | `XC_Time_GetTick`           | 取当前 Tick                                                                                                                                                                       |
|                                                                   | `XC_Time_GetMs`             | 取系统已运行的毫秒数                                                                                                                                                              |
|                                                                   | `XC_Time_TickInc`           | 中断里递增 Tick（默认模式下**必须调用**）                                                                                                                                         |
|                                                                   | `XC_Time_TickSet`           | 设置 Tick（休眠唤醒后校正；仅中断累加模式）                                                                                                                                       |
|                                                                   | `XC_Time_CheckTimeout`      | 判断距 `last` 是否已过 n 个 Tick（**回绕安全**）                                                                                                                                  |
|                                                                   | `XC_Time_CheckTimeoutMs`    | 同上，单位 ms                                                                                                                                                                     |
|                                                                   | `XC_Time_CheckTimeoutSec`   | 同上，单位 s                                                                                                                                                                      |
|                                                                   | `XC_Time_GetRemain`         | 距目标还差多少 Tick（0 = 已到）                                                                                                                                                   |
|                                                                   | `XC_Time_GetElapsed`        | 距目标已经过多少 Tick（封顶为目标值）                                                                                                                                             |
|                                                                   | `XC_Time_TimerSet`          | 设置软件定时器目标（Tick）                                                                                                                                                        |
|                                                                   | `XC_Time_TimerSetMs`        | 同上，单位 ms                                                                                                                                                                     |
|                                                                   | `XC_Time_TimerSetSec`       | 同上，单位 s                                                                                                                                                                      |
|                                                                   | `XC_Time_TimerRepeat`       | 以当前时刻重设上次目标（周期定时）                                                                                                                                                |
|                                                                   | `XC_Time_TimerCheck`        | 定时器是否到点                                                                                                                                                                    |
|                                                                   | `XC_Time_TimerClr`          | 清空定时器计数                                                                                                                                                                    |
|                                                                   | `XC_Time_TimerGetRemain`    | 定时器还差多少 Tick                                                                                                                                                               |
|                                                                   | `XC_Time_TimerGetElapsed`   | 定时器已经过多少 Tick                                                                                                                                                             |
|                                                                   | `XC_Time_TimerGetElapsedMs` | 定时器已经过多少 ms                                                                                                                                                               |
|                                                                   | `XC_Time_BlockDelay`        | 死循环阻塞延时（Tick；非协程场景，会占住 CPU）                                                                                                                                    |
|                                                                   | `XC_Time_BlockDelayMs`      | 死循环阻塞延时（ms；非协程场景）                                                                                                                                                  |
|                                                                   |                             |                                                                                                                                                                                   |
| **时间换算宏（`XC_Time.h`）**                                     | `XC_Time_UsToTicks`         | us → Tick（向上取整）                                                                                                                                                            |
|                                                                   | `XC_Time_MsToTicks`         | ms → Tick（向上取整）                                                                                                                                                            |
|                                                                   | `XC_Time_SecToTicks`        | s → Tick（乘频率；**秒数上限 ≈ Tick 最大值/f**，超限回绕后偏短 ⇒ 见 `XC_Config.md` 的 `XC_CFG_TICKS_PER_SEC`）                                                                 |
|                                                                   | `XC_Time_TicksToUs`         | Tick → us                                                                                                                                                                        |
|                                                                   | `XC_Time_TicksToMs`         | Tick → ms（向下取整）                                                                                                                                                            |
|                                                                   | `XC_Time_TicksToSec`        | Tick → s                                                                                                                                                                         |
|                                                                   |                             |                                                                                                                                                                                   |
| **诊断（`XC_Err.h` / `XC_Diag.h`；**默认全关 ⇒ 发布 0 开销**）** | `XC_Err_Hook`               | 错误上报钩子（**由用户实现**；断言场景 `phTCB == NULL`）                                                                                                                          |
|                                                                   | `XC_Diag_CheckInvariants`   | 运行期自检（返回首个失败项；**不可在已持锁上下文**调用）（**仅调试档**：`XC_CFG_DEBUG_CHECK=1`）                                                                                  |
|                                                                   | `XC_Diag_GetRunCnt`         | 取任务运行次数（饱和计数，不回绕）                                                                                                                                                |
|                                                                   | `XC_Diag_GetMaxRunTick`     | 取任务最长一次"调度槽"耗时（Tick）                                                                                                                                                |
|                                                                   | `XC_Diag_ClrRunStats`       | 清零任务运行统计                                                                                                                                                                  |
|                                                                   |                             |                                                                                                                                                                                   |
| **二进制常量（`XC_BitPatterns.h`）**                              | `BIN_0` … `BIN_1111_1111`  | 1~8 位二进制常量（如 `BIN_1010_0011` = `0xA3`）；本版本只提供 `BIN_*` 一种写法                                                                                                    |

> 说明：本表只列**用户可见 / 可用**的 API、类型与配置；实现侧私有项（`XC_List_*`、`XC_Core_*`、`XC_Task_Handle*`、`XC_Diag_RunStats*`、`XC_Err_Report`、`XC_DIAG_ASSERT` 等）不在表内。

**调度主循环**（就绪表轮转 + 入表排队尾；图中箭头与线均为 ASCII 码）：

```text
主循环 XC_Sch_Start
  |
  +--> [1] 就绪表非空? --否(空)--> [5] 空闲回调 fIdle(可休眠 IdleTick)
  |         |
  |         | 是
  |         v
  |    [2] 取表首任务(摘除 => 游离标记)
  |         |
  |         v
  |    [3] 运行 fTask(协程块内让出: Delay / WaitNotify / Yield / Leave)
  |         |
  |         v
  |    [4] 仍游离(自环)? --是--> [4a] 插回就绪表尾(统一排队尾)
  |         | 否(已被移入 时间表/阻塞表)      |
  |         +<------------------------------+
  |         |
  |         v
  |    [5] 时间调度 XC_Sch_TimeSched + 事件调度 XC_Sch_EventSched
  |         |
  +<--------+
```

> **一句话**：主循环反复做三件事 —— ① **有就绪任务就按轮转跑一个**（取表首 → 运行 → 仍游离就插回**队尾**）；
> ② **跑完做时间调度与事件调度**（延时到点 / 通知落地）；③ **没有就绪任务就进空闲回调**（可休眠）。

> 完整实现细节（数据结构、调度流程、内存开销、边界限制）请参考 [架构概览](./架构概览/XCOS_V2.1.0_架构概览.md)。

---

## 📋 基础定义

### 框架标识宏

| 宏名             | 值  | 文件   | 说明                                                |
| ---------------- | --- | ------ | --------------------------------------------------- |
| `XCOS_API_LEVEL` | `2` | XCOS.h | 框架 API 兼容性标识（指向主版本号，用于兼容性检查） |

### 版本信息

| 宏名              | 值         | 文件   | 说明                           |
| ----------------- | ---------- | ------ | ------------------------------ |
| `XCOS_VER_MAJOR`  | `2`        | XCOS.h | 主版本号                       |
| `XCOS_VER_MINOR`  | `1`        | XCOS.h | 次版本号                       |
| `XCOS_VER_PATCH`  | `0`        | XCOS.h | 修订号                         |
| `XCOS_VER_STRING` | `"2.1.0"`  | XCOS.h | 字符串版本                     |
| `XCOS_VER_NUMBER` | `0x020100` | XCOS.h | 版本数值（24位：主·次·修订） |

---

## 🏗️ 类型系统

### 核心数据类型

| 类型名              | 大小(32位)   | 文件          | 说明                                                                                                                               |
| ------------------- | ------------ | ------------- | ------------------------------------------------------------------------------------------------------                             |
| `XC_Tick_t`         | 4 Bytes      | XC_Type.h     | 系统 Tick 计数类型（**固定 32 位无符号**；定义在 `Internal/XC_TypeInternal.h`，**不可配置**）                                      |
| `XC_TimerTick_t`    | 8 Bytes      | XC_Type.h     | 软件定时器计数类型（`TickCount` + `WaitCount`）                                                                                    |
| `XCOS_t`            | 44 Bytes     | XC_Type.h     | XCOS 框架实例结构体（不透明；4 张链表 32B + `PrevTick` + `TaskNum`/`Lock`/`EventPending` + `fIdle`）                               |
| `XC_OSHandle_t`     | 4 Bytes      | XC_Type.h     | 框架操作句柄（`XCOS_t*`）                                                                                                          |
| `XC_TaskCB_t`       | 44 Bytes     | XC_Type.h     | 任务控制块实例（不透明，V2.1.0 含协程嵌套字段；**必须静态/零初始化**）                                                             |
| `XC_TaskHandle_t`   | 4 Bytes      | XC_Type.h     | 任务操作句柄（`XC_TaskCB_t*`）                                                                                                     |

### 句柄与不透明指针

- `XC_OSHandle_t` = `XCOS_t*`：框架句柄，所有模块操作的第一参数；
- `XC_TaskHandle_t` = `XC_TaskCB_t*`：任务句柄；
- 两者均为**不透明指针**：内部结构只在 `Internal/` 头文件定义，用户不可直接访问成员，通过 API/宏操作；
- 一个工程可创建多个 XCOS 实例（多个框架句柄），互不影响。

### 协程相关类型（V2.1.0 新增）

> 以下类型供协程嵌套使用（用户可声明变量，不直接访问内部字段）：
> `XC_CorFrame_t` 的**类型名**在 `Code/Inc/XC_Type.h`（用户按需定义帧栈数组，如 `XC_CorFrame_t s_stack[3]`），
> 结构体定义与另两个类型（`XC_CorFn_t` / `XC_CorState_t`）在 `Internal/XC_TypeInternal.h`。

| 类型名          | 大小(32位) | 说明                                                                  |
| --------------- | ---------- | --------------------------------------------------------------------- |
| `XC_CorFn_t`    | 4 Bytes    | 协程函数指针：`void (*)(XC_TaskHandle_t)`（顶层任务与子协程统一签名） |
| `XC_CorFrame_t` | 8 Bytes    | 协程帧（每层一个：父层入口 `pfn` + 父层返回点 `BP`）                  |
| `XC_CorState_t` | 1 Byte     | 本轮结果：`XC_COR_DONE`（本层完成）/ `XC_COR_SUSPENDED`（本层挂起）   |

---

## 🎯 常量枚举

### 函数返回值类型（`XC_Return_t`）

| 名称          | 值  | 说明                           |
| ------------- | --- | ------------------------------ |
| `XC_OK`       | 0   | 成功                           |
| `XC_FAIL`     | 1   | 失败或无效                     |
| `XC_CONTINUE` | 2   | 操作继续或未完成（异步操作中） |

### 任务状态类型（`XC_TaskState_t`）

| 名称              | 值  | 说明                                           |
| ----------------- | --- | ---------------------------------------------- |
| `XC_TASK_VOID`    | 0   | 空（任务创建前或被移除后的状态）               |
| `XC_TASK_RUN`     | 1   | 运行（正在运行的任务）                         |
| `XC_TASK_READY`   | 2   | 就绪（在就绪表中的任务状态，任务注册后为就绪） |
| `XC_TASK_BLOCKED` | 3   | 阻塞（延时、等待通知后的状态）                 |
| `XC_TASK_SUSPEND` | 4   | 挂起                                           |

**状态转换**（图中箭头与线均为 ASCII 码）：

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

### 错误码类型（`XC_ErrCode_t`）

> 用于「**错误上报钩子**」（`Code/Inc/XC_Err.h`；开关 `XC_CFG_ERR_HOOK` **默认关闭**，关闭时零开销）；

| 名称                    | 值  | 说明                                                                                |
| ----------------------- | --- | ----------------------------------------------------------------------------------- |
| `XC_ERR_NONE`           | 0   | 无错误（保留值，框架不会上报此值）                                                  |
| `XC_ERR_FRAME_OVERFLOW` | 1   | 帧栈越界：未配帧栈调用 `XC_Cor_Call`，或嵌套层数超过 `CorDepthMax`                  |
| `XC_ERR_ASSERT`         | 2   | 断言失败（`XC_DIAG_ASSERT` 条件为假；**需同时打开 `XC_CFG_ASSERT`**，见「断言」节） |

> 值域 `0~2`；**每次只上报一个错误码**（框架内不保存历史），与 `Code/Inc/XC_Err.h` 的 `XC_ErrCode_t` 一致。

### 自检结果类型（`XC_ChkResult_t`）

> 用于「**运行期不变量自检**」（`XC_Diag_CheckInvariants`；开关 `XC_CFG_DEBUG_CHECK` **默认关闭**，关闭时该函数不编译）；

| 名称                      | 值  | 说明                                                           |
| ------------------------- | --- | -------------------------------------------------------------- |
| `XC_CHK_OK`               | 0   | 全部一致                                                       |
| `XC_CHK_LINK_BROKEN`      | 1   | 链表结构损坏（前后指针不互指，或成环：遍历步数超过任务数上限） |
| `XC_CHK_STATE_TABLE`      | 2   | 任务状态与所在表不匹配                                         |
| `XC_CHK_NODE_IN_TWO_LIST` | 3   | 同一任务被同时挂在两张表                                       |
| `XC_CHK_TASKNUM`          | 4   | `TaskNum` 与四表节点总数不一致                                 |
| `XC_CHK_NESTING`          | 5   | 协程帧栈/深度自洽性被破坏                                      |
| `XC_CHK_HANDLE`           | 6   | 表内任务的 `phXCOS` 不是本次检查的框架实例                     |

> **内部枚举**（非用户 API，定义于 `Code/Inc/Internal/XC_TypeInternal.h`，此处仅备查）：
> `XC_NotifyState_t` = `XC_NOTIFY_WAKEUP`(0，通知唤醒) / `XC_NOTIFY_WAIT`(1，等待通知)；
> `XC_CorState_t` = `XC_COR_DONE`(0，本层完成) / `XC_COR_SUSPENDED`(1，本层挂起)，已列于「协程相关类型」。

---

## 🔢 二进制模式

### 二进制常量定义（`XC_BitPatterns.h`）

提供可视化的二进制模式宏，便于位操作；支持 1~8 位任意二进制常量，可使用下划线 `_` 按“4位_4位”分组提高可读性，直接映射为十六进制值。

| 常量名          | 值   | 说明            |
| --------------- | ---- | --------------- |
| `BIN_0`         | 0x00 | 二进制 0        |
| `BIN_0000_0001` | 0x01 | 二进制 00000001 |
| `BIN_1010_0011` | 0xA3 | 二进制 10100011 |
| `BIN_1111_1001` | 0xF9 | 二进制 11111001 |

**特点**:
- 前缀为 `BIN_`（V2.1.0 起统一使用；本版本 `Code/Inc/XC_BitPatterns.h` **只提供 `BIN_*` 一种写法**）；
- 支持 1~8 位任意二进制常量；
- 可使用下划线提高可读性（只支持“4位_4位”分组）；
- 直接映射为十六进制值。

## 🛠️ 函数参考

> **约定**：
> - 类型列：`函数`（实函数，位于 `.c`）或 `宏`（头文件内联宏）；
> - 调用等级：`协程内` = 必须在协程块（`XC_Cor_Enter`/`XC_Cor_Leave` 之间）使用；`中断` = 可在中断服务程序中调用（**仅通知类与 Tick 类 API**，SPSC 线程安全）；详见 [函数调用等级](#-函数调用等级)。

### 调度器模块（XC_Sch，4 个）

#### `XC_Sch_Init` — 调度器初始化

| 项       | 内容                                                                                                |
| -------- | --------------------------------------------------------------------------------------------------- |
| 原型     | `void XC_Sch_Init(XC_OSHandle_t phXCOS)`                                                            |
| 参数     | `phXCOS` 框架句柄                                                                                   |
| 返回     | 无                                                                                                  |
| 说明     | 创建好 XCOS 实例后调用，初始化框架锁、就绪/时间/溢出/阻塞四表、事件计数等；**调度器启动前必须调用** |
| 调用等级 | 初始化阶段（main 中）                                                                               |

#### `XC_Sch_Start` — 调度器启动（阻塞）

| 项       | 内容                                                                                     |
| -------- | ---------------------------------------------------------------------------------------- |
| 原型     | `void XC_Sch_Start(XC_OSHandle_t phXCOS)`                                                |
| 参数     | `phXCOS` 框架句柄                                                                        |
| 返回     | 无                                                                                       |
| 说明     | 阻塞运行调度器主循环：轮询就绪表运行任务、时间调度、事件调度、空闲回调；**调用后不返回** |
| 调用等级 | 初始化阶段（main 中，最后一步）                                                          |

#### `XC_Sch_GetTaskNum` — 获取任务数

| 项       | 内容                                              |
| -------- | ------------------------------------------------- |
| 原型     | `uint8_t XC_Sch_GetTaskNum(XC_OSHandle_t phXCOS)` |
| 参数     | `phXCOS` 框架句柄                                 |
| 返回     | `uint8_t` 当前框架中的任务数量                    |
| 说明     | 直接读取框架句柄中的任务数字段                    |
| 调用等级 | 任意（非协程内）                                  |

#### `XC_Sch_SetIdleCallback` — 设置空闲处理回调

| 项       | 内容                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| -------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型     | `void XC_Sch_SetIdleCallback(XC_OSHandle_t phXCOS, void (*fIdle)(XC_OSHandle_t, XC_Tick_t))`                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 参数     | `phXCOS` 框架句柄；`fIdle` 空闲回调函数指针（传 `NULL` 清除回调）                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 返回     | 无                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 说明     | 就绪表为空时调用回调。回调参数：`phXCOS` 框架句柄、`IdleTick` **空闲提示值**（距下一个任务唤醒还有多少 Tick，是**建议休眠上限**且为**快照**）：<br>• **`0` = 不要休眠**（有到点/待处理工作，含“窗口内 Tick 前进/回绕”“计算期间刚有任务就绪”）⇒ 回调应**立刻返回**；⚠️ **不要把 0 直接交给“设置休眠时长”类接口**——有些平台把 0 当“无限”会睡死；<br>• **`~0U` = 无待唤醒任务**（时间表与溢出表都空）⇒ 按平台最大能力处理（休眠期间 Tick 停走的平台别当成“睡 49.7 天”）；<br>• 其余 = 建议休眠上限。若休眠会停止 Tick 计数，须在唤醒时更新 `XC_Time_TickSet` |
| 回调原型 | `void fIdle(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)`                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 调用等级 | 初始化阶段（main 中）                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |

---

### 任务管理模块（XC_Task，15 个）

#### 任务注册与移除

##### `XC_Task_Reg` — 任务注册

| 项         | 内容                                                                                                                                                                                                                                                                       |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型       | `XC_Return_t XC_Task_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)`                                                                                                                                                       |
| 参数       | `phXCOS` 框架句柄；`phTCB` 任务控制块；`fTask` 任务函数指针；`pParam` 传递给任务的参数                                                                                                                                                                                     |
| 返回       | `XC_OK` 注册成功；`XC_FAIL` 注册失败（任务太多，或该任务已注册）                                                                                                                                                                                                           |
| 说明       | 注册一个普通任务（无协程嵌套）。**同一任务只能注册一次**（重复注册返回 `XC_FAIL`；移除后可再次注册）；注册时任务进入就绪表；**TCB 必须静态分配（BSS 零初始化）或先清零**（判重读 `phXCOS`）；本函数会清空帧栈配置（需嵌套请用 `XC_Task_RegExt` / `XC_Task_SetCorStack`）   |
| 调用等级   | 初始化阶段 / 运行期（非协程内）                                                                                                                                                                                                                                            |

##### `XC_Task_RegExt` — 任务注册（扩展：支持协程嵌套）

| 项         | 内容                                                                                                                                                                                                                                               |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型       | `XC_Return_t XC_Task_RegExt(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, XC_CorFn_t fTask, void* pParam, XC_CorFrame_t* pCorStack, uint8_t CorDepthMax)`                                                                                           |
| 参数       | `phXCOS` 框架句柄；`phTCB` 任务控制块；`fTask` 任务函数指针；`pParam` 任务参数；`pCorStack` 协程帧栈（无嵌套传 `NULL`）；`CorDepthMax` 帧栈容量/最大嵌套层数（无嵌套传 `0`）                                                                       |
| 返回       | `XC_OK` 成功；`XC_FAIL` 失败（任务太多，或帧栈/容量参数不匹配）                                                                                                                                                                                    |
| 说明       | V2.1.0 新增。等价于 `XC_Task_Reg` + `XC_Task_SetCorStack`；`pCorStack`/`CorDepthMax` 必须同为有/同为无；需要嵌套的任务必须用本函数或 `XC_Task_SetCorStack` 配置帧栈；**实现上"先注册成功、再写帧栈"**（`XC_Task_Reg` 会清空这两个字段）            |
| 调用等级   | 初始化阶段 / 运行期（非协程内）                                                                                                                                                                                                                    |

##### `XC_Task_SetCorStack` — 设置协程帧栈（支持/撤销嵌套）

| 项         | 内容                                                                                                                                                                                                          |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型       | `XC_Return_t XC_Task_SetCorStack(XC_TaskHandle_t phTCB, XC_CorFrame_t* pCorStack, uint8_t CorDepthMax)`                                                                                                       |
| 参数       | `phTCB` 任务控制块；`pCorStack` 协程帧栈（撤销传 `NULL`）；`CorDepthMax` 容量（撤销传 `0`）                                                                                                                   |
| 返回       | `XC_OK` 成功；`XC_FAIL` 帧栈/容量参数不匹配                                                                                                                                                                   |
| 说明       | V2.1.0 新增。传 `(NULL, 0U)` 撤销嵌套能力；重新设置会清空当前深度（`CorDepth=0`）与"本轮结果"；任务嵌套中更换配置前应先复位清栈；**分步注册顺序**：`XC_Task_SetEntry` →(需要嵌套再)本函数 → `XC_Task_Add`   |
| 调用等级   | 非协程内                                                                                                                                                                                                      |

##### `XC_Task_Remove` — 任务移除

| 项       | 内容                                                                                                                                                                 |
| -------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型     | `void XC_Task_Remove(XC_TaskHandle_t phTCB)`                                                                                                                         |
| 参数     | `phTCB` 任务控制块                                                                                                                                                   |
| 返回     | 无                                                                                                                                                                   |
| 说明     | 移除一个任务：出表、清栈、`TaskNum--`、状态回 `XC_TASK_VOID`、`phXCOS=NULL`。**不可移除自身**（移除自身使用 `XC_Cor_Remove`）；与 `XC_Task_Reset` 类似，唤醒源不清除 |
| 调用等级 | 任意（非协程内；**不可在中断中调用**）                                                                                                                               |

#### 分步创建

##### `XC_Task_SetEntry` — 设置任务入口

| 项       | 内容                                                                                                                                                                                                                |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型     | `void XC_Task_SetEntry(XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)`                                                                                                                        |
| 参数     | `phTCB` 任务控制块；`fTask` 任务函数指针；`pParam` 任务参数                                                                                                                                                         |
| 返回     | 无                                                                                                                                                                                                                  |
| 说明     | 只设置任务入口和参数，配合 `XC_Task_Add` 分步注册使用。**嵌套中换入口**：函数会自动"清栈 + 重置断点"，语义 = **丢弃当前嵌套层、按新入口从头运行**（保证"栈底帧 = 顶层入口"不变量，避免复位/移除时把入口恢复成旧值） |
| 调用等级 | 非协程内                                                                                                                                                                                                            |

##### `XC_Task_Add` — 添加任务

| 项       | 内容                                                                                   |
| -------- | -------------------------------------------------------------------------------------- |
| 原型     | `XC_Return_t XC_Task_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB)`                 |
| 参数     | `phXCOS` 框架句柄；`phTCB` 任务控制块                                                  |
| 返回     | `XC_OK` 成功；`XC_FAIL` 任务太多                                                       |
| 说明     | 将已设置入口的任务加入调度器（就绪表）。与 `XC_Task_SetEntry` 组合可替代 `XC_Task_Reg` |
| 调用等级 | 非协程内                                                                               |

#### 复位 | 挂起 | 恢复

##### `XC_Task_Reset` — 任务复位

| 项       | 内容                                                                                                |
| -------- | --------------------------------------------------------------------------------------------------- |
| 原型     | `void XC_Task_Reset(XC_TaskHandle_t phTCB)`                                                         |
| 参数     | `phTCB` 任务控制块                                                                                  |
| 返回     | 无                                                                                                  |
| 说明     | 复位指定任务：清断点、清帧栈、从任务函数头重新开始；**不可复位自身**（复位自身使用 `XC_Cor_Reset`） |
| 调用等级 | 任意（非协程内；**不可在中断中调用**）                                                              |

##### `XC_Task_Suspend` — 任务挂起

| 项       | 内容                                                                       |
| -------- | -------------------------------------------------------------------------- |
| 原型     | `XC_Return_t XC_Task_Suspend(XC_TaskHandle_t phTCB)`                       |
| 参数     | `phTCB` 任务控制块                                                         |
| 返回     | `XC_OK` 挂起成功；`XC_FAIL` 失败（任务不存在）                             |
| 说明     | 将任务挂起，本次任务运行完成后暂停；挂起后不再被调度（延时的任务也可挂起） |
| 调用等级 | 任意（非协程内；**不可在中断中调用**）                                     |

##### `XC_Task_Resume` — 任务挂起恢复

| 项         | 内容                                                                                                                                                                                                       |
| ---------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型       | `XC_Return_t XC_Task_Resume(XC_TaskHandle_t phTCB)`                                                                                                                                                        |
| 参数       | `phTCB` 任务控制块                                                                                                                                                                                         |
| 返回       | `XC_OK` 恢复成功；`XC_FAIL` 失败（任务未挂起）                                                                                                                                                             |
| 说明       | 只能恢复被挂起的任务；恢复后回到就绪表。**"恢复"不等于"收到通知"**：挂起期间没有通知时，恢复后 `XC_Cor_IsNotifyTimeout()` 仍为真（判据是"本次等待不是被通知结束的"）⇒ 要区分"超时 / 被恢复"请自行加标志   |
| 调用等级   | 任意（非协程内；**不可在中断中调用**）                                                                                                                                                                     |

#### 通知（SPSC，线程安全）

##### `XC_Task_SendNotify` — 发送通知

| 项         | 内容                                                                                                                                                                                                                                                                                                                                                            |
| ---------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型       | `XC_Return_t XC_Task_SendNotify(XC_TaskHandle_t phTCB, void* pNotifyData)`                                                                                                                                                                                                                                                                                      |
| 参数       | `phTCB` 需要发送通知的任务 TCB；`pNotifyData` 通知传递的参数                                                                                                                                                                                                                                                                                                    |
| 返回       | `XC_OK` 通知成功；`XC_FAIL` 任务不存在或任务被挂起；`XC_CONTINUE` 异步操作中                                                                                                                                                                                                                                                                                    |
| 说明       | 发送通知并唤醒任务。**线程安全（SPSC）**：单生产者单消费者模型，可在中断中调用（生产侧只写常量 1，不做读-改-写）；**发布次序保证**：先写数据（`pNotifyData` 为 `volatile`）再置标志 ⇒ 消费侧见到标志后读到的一定是新数据（编译器不得跨 `volatile` 重排）；可在“等待通知”前发送（提前通知，等待将立即结束）；唤醒后待处理标志清零；多生产者时须用户加锁保护   |
| 调用等级   | 任意 / **中断（ISR 唯一合法的框架调用）**                                                                                                                                                                                                                                                                                                                       |

> **中断侧唯一入口**：ISR 中只允许调用本函数（其余仅"只读查询"可酌情用于调试）；"挂起 / 恢复 / 复位 / 移除 / 延时 / 等待"等动作**必须在任务上下文执行**。
>
> ISR 想产生这些效果的**正确写法** —— 通知一个控制任务，由该任务执行对应 API：
>
> ```c
> /* ISR 侧 */               (void)XC_Task_SendNotify(&s_Ctrl, (void*)&s_Event);
>
> /* 控制任务(任务上下文) */
> XC_Cor_WaitNotify(0U);
> if(!XC_Cor_IsNotifyTimeout()) { XC_Task_Resume(&s_Worker); }   /* 真正动表在这里 */
> ```
>
> 违反此约定（在 ISR 中调用 `Suspend/Resume/Reset/Remove` 等"会移表"的 API）会破坏框架的锁握手（`XC_Core_Lock/Unlock` 只是"主循环 ↔ 中断"的**单一标志位**，不可重入），造成**静默**的链表损坏。

##### `XC_Task_ClrNotify` — 清除通知（宏）

| 项       | 内容                                                                                                                 |
| -------- | -------------------------------------------------------------------------------------------------------------------- |
| 原型     | `#define XC_Task_ClrNotify(phTCB)`                                                                                   |
| 参数     | `phTCB` 任务控制块                                                                                                   |
| 说明     | 清除通知（待处理标志清 0）；可在“等待通知”前调用，防止通知提前到达；须在主循环（目标任务自身）调用，勿在中断中调用 |
| 调用等级 | **任务上下文（消费侧；不可在中断中调用）**                                                                           |

##### `XC_Task_UpdateNotifyData` — 更新通知数据（宏）

| 项       | 内容                                                                                                                                                                                 |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 原型     | `#define XC_Task_UpdateNotifyData(phTCB, _pNotifyData)`                                                                                                                              |
| 参数     | `phTCB` 任务控制块；`_pNotifyData` 通知数据                                                                                                                                          |
| 说明     | 不使用任务通知时，可用通知数据字段传递数据（写）；写的是**生产者侧字段**，须与 `XC_Task_SendNotify` 处于**同一生产者上下文**（多个 ISR 都写同一任务的该字段 = 多生产者，需自行加锁） |
| 调用等级 | **生产者上下文（与 `XC_Task_SendNotify` 同一上下文）**                                                                                                                               |

##### `XC_Task_ReadNotifyData` — 读取通知数据（宏）

| 项       | 内容                                                                             |
| -------- | -------------------------------------------------------------------------------- |
| 原型     | `#define XC_Task_ReadNotifyData(phTCB)`                                          |
| 参数     | `phTCB` 任务控制块                                                               |
| 返回     | `void*` 通知数据                                                                 |
| 说明     | 读取通知数据字段（纯读，任意上下文可调；可能读到瞬时旧值，勿作跨上下文业务决策） |
| 调用等级 | 任意（只读）                                                                     |

#### 参数与状态（宏）

##### `XC_Task_GetParam` — 获取任务参数（宏）

| 项       | 内容                                                                                                                                                            |
| -------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型     | `#define XC_Task_GetParam(phTCB)`                                                                                                                               |
| 参数     | `phTCB` 任务控制块                                                                                                                                              |
| 返回     | `void*` 注册时传递的参数                                                                                                                                        |
| 说明     | 获取任务注册时传递的参数。**注意**：协程内上下文切换局部变量不保存，若要在协程块内使用参数，应在 `XC_Cor_Enter` 前赋值给静态/全局变量，或使用 `XC_Cor_GetParam` |
| 调用等级 | 任意                                                                                                                                                            |

##### `XC_Task_GetState` — 获取任务状态（宏）

| 项       | 内容                                                                  |
| -------- | --------------------------------------------------------------------- |
| 原型     | `#define XC_Task_GetState(phTCB)`                                     |
| 参数     | `phTCB` 任务控制块                                                    |
| 返回     | `XC_TaskState_t` 任务状态（`XC_TASK_VOID/RUN/READY/BLOCKED/SUSPEND`） |
| 说明     | 获取任务运行状态                                                      |
| 调用等级 | 任意                                                                  |

### 协程块模块（XC_Cor，17 个，均须在协程块内使用）

> **协程块**是 XCOS 的核心执行模型：`XC_Cor_Enter` 开始，`XC_Cor_Leave` 结束；块内的延时/等待通知/让出/挂起等操作会自动“挂起+恢复”。所有 `XC_Cor_*` 宏**必须在协程块内调用**。

#### 进入与离开

##### `XC_Cor_Enter` — 进入协程块（宏）

| 项       | 内容                                                                                     |
| -------- | ---------------------------------------------------------------------------------------- |
| 原型     | `#define XC_Cor_Enter(phTCB)`                                                            |
| 参数     | `phTCB` 任务控制块                                                                       |
| 说明     | 协程块的开始；必须搭配 `XC_Cor_Leave` 使用。首次执行从函数头开始，恢复时从上次挂起点继续 |
| 调用等级 | 协程内（函数开头）                                                                       |

##### `XC_Cor_Leave` — 离开协程块（宏）

| 项       | 内容                                                                                                                                                                                                                                                                                                                                                                                 |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 原型     | `#define XC_Cor_Leave()`                                                                                                                                                                                                                                                                                                                                                             |
| 说明     | 协程块的结束；走到此处表示本层正常完成（`CorState=DONE`）。若本层被挂起（延时/等通知/让出），恢复前不会执行到此处。**宏内含编译器可达性提示**（`if(0){goto}` 常量假分支，永不执行，-O0~-O3 均被优化消除，零开销）：使任务可保持 `while(1){...}XC_Cor_Leave()` 标准写法而免除 armcc #111-D 不可达告警（详见 `XC_Cor.h` 注释与 `Docs/MISRA-C/MISRA-C_2025_Deviation.md` DEV-XCOS-001） |
| 调用等级 | 协程内（函数结尾）                                                                                                                                                                                                                                                                                                                                                                   |

#### 嵌套调用（V2.1.0）

##### `XC_Cor_Call` — 嵌套调用子协程（宏）

| 项       | 内容                                                                                                                                                                                                                                                                                                                                                   |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 原型     | `#define XC_Cor_Call(fn)`                                                                                                                                                                                                                                                                                                                              |
| 参数     | `fn` 子协程函数（`XC_CorFn_t`，签名同任务函数）                                                                                                                                                                                                                                                                                                        |
| 说明     | **登记**子协程，不做真实 C 嵌套调用（C 栈 O(1)）：入栈保存“父层入口+返回点”，入口切换为子函数后跳出，下一轮由调度器进入子层；子层完成后调度器弹帧恢复父层，**同轮切父**继续运行父层。需要嵌套的任务必须用 `XC_Task_RegExt`/`XC_Task_SetCorStack` 配置帧栈；未配栈时调用触发越界保护（复位任务）。**注意**：`Call` 是必挂起点，调用前后局部变量不保存 |
| 调用等级 | 协程内                                                                                                                                                                                                                                                                                                                                                 |

#### 协程控制

##### `XC_Cor_GetParam` — 获取任务参数（宏）

| 项       | 内容                                       |
| -------- | ------------------------------------------ |
| 原型     | `#define XC_Cor_GetParam()`                |
| 返回     | `void*` 任务注册时传递的参数               |
| 说明     | 协程块内获取任务参数（顶层与子层均可使用） |
| 调用等级 | 协程内                                     |

##### `XC_Cor_Yield` — 让出控制（宏）

| 项       | 内容                                                                          |
| -------- | ----------------------------------------------------------------------------- |
| 原型     | `#define XC_Cor_Yield()`                                                      |
| 说明     | 让出 CPU 使用权，跳出协程块；任务仍留在就绪表，下次调度后从**下一行**继续执行 |
| 调用等级 | 协程内                                                                        |

##### `XC_Cor_Suspend` — 挂起自身（宏）

| 项       | 内容                                                  |
| -------- | ----------------------------------------------------- |
| 原型     | `#define XC_Cor_Suspend()`                            |
| 说明     | 将当前任务挂起；恢复需由其他任务调用 `XC_Task_Resume` |
| 调用等级 | 协程内                                                |

##### `XC_Cor_Reset` — 复位自身（宏）

| 项       | 内容                                                                                       |
| -------- | ------------------------------------------------------------------------------------------ |
| 原型     | `#define XC_Cor_Reset()`                                                                   |
| 说明     | 复位当前任务：清断点、清帧栈、恢复最外层入口，立刻退出协程块；下次调度从任务函数头重新执行 |
| 调用等级 | 协程内                                                                                     |

##### `XC_Cor_Remove` — 移除自身（宏）

| 项       | 内容                                                                                                     |
| -------- | -------------------------------------------------------------------------------------------------------- |
| 原型     | `#define XC_Cor_Remove()`                                                                                |
| 说明     | 移除当前任务：出表、清栈、`TaskNum--`、状态回 `VOID`；立刻退出协程块。注意唤醒源不会被清除，直到下次唤醒 |
| 调用等级 | 协程内                                                                                                   |

#### 延时

##### `XC_Cor_DelayTick` — 延时 Tick（宏）

| 项       | 内容                                                                                 |
| -------- | ------------------------------------------------------------------------------------ |
| 原型     | `#define XC_Cor_DelayTick(Tick)`                                                     |
| 参数     | `Tick` 延时 Tick 数                                                                  |
| 说明     | 让出 CPU，进入时间表，延时 `Tick` 个基础时钟后自动回到就绪表；恢复后从**下一行**继续 |
| 调用等级 | 协程内                                                                               |

##### `XC_Cor_DelayUs` / `XC_Cor_DelayMs` / `XC_Cor_DelaySec` — 延时（us/ms/s）（宏）

| 项       | 内容                                                                                                     |
| -------- | -------------------------------------------------------------------------------------------------------- |
| 原型     | `#define XC_Cor_DelayUs(Us)` / `#define XC_Cor_DelayMs(Ms)` / `#define XC_Cor_DelaySec(Sec)`             |
| 参数     | `Us`/`Ms`/`Sec` 延时时间                                                                                 |
| 说明     | 基于 `XC_Cor_DelayTick` 做单位换算；**us 延时需 Tick 时基为 us 级才精确**（`XC_CFG_TICKS_PER_SEC` 高时） |
| 调用等级 | 协程内                                                                                                   |

#### 等待通知

##### `XC_Cor_WaitNotify` — 等待通知（Tick，宏）

| 项       | 内容                                                                                                                     |
| -------- | ------------------------------------------------------------------------------------------------------------------------ |
| 原型     | `#define XC_Cor_WaitNotify(TickTimeout)`                                                                                 |
| 参数     | `TickTimeout` 超时 Tick 数；`0` 表示死等（不超时）                                                                       |
| 说明     | 任务等待通知，进入阻塞表；被 `XC_Task_SendNotify` 唤醒或超时后回到就绪表。唤醒后用 `XC_Cor_IsNotifyTimeout` 判断是否超时 |
| 调用等级 | 协程内                                                                                                                   |

##### `XC_Cor_WaitNotifyMs` — 等待通知（ms，宏）

| 项       | 内容                                     |
| -------- | ---------------------------------------- |
| 原型     | `#define XC_Cor_WaitNotifyMs(MsTimeout)` |
| 参数     | `MsTimeout` 超时时间（ms）；`0` 表示死等 |
| 说明     | `XC_Cor_WaitNotify` 的 ms 版本           |
| 调用等级 | 协程内                                   |

##### `XC_Cor_ClrNotify` — 清除通知（宏）

| 项       | 内容                                                           |
| -------- | -------------------------------------------------------------- |
| 原型     | `#define XC_Cor_ClrNotify()`                                   |
| 说明     | 清除通知（待处理标志清 0）；在等待通知前调用，防止通知提前到达 |
| 调用等级 | 协程内                                                         |

##### `XC_Cor_IsNotifyTimeout` — 检查是否超时（宏）

| 项       | 内容                                        |
| -------- | ------------------------------------------- |
| 原型     | `#define XC_Cor_IsNotifyTimeout()`          |
| 返回     | `0` 未超时（被通知唤醒）；`1` 超时          |
| 说明     | 用于 `XC_Cor_WaitNotify` 唤醒后判断唤醒原因 |
| 调用等级 | 协程内（等待通知返回后）                    |

##### `XC_Cor_GetNotifyData` — 获取通知数据（宏）

| 项       | 内容                             |
| -------- | -------------------------------- |
| 原型     | `#define XC_Cor_GetNotifyData()` |
| 返回     | `void*` 通知传递的数据           |
| 说明     | 被通知唤醒后获取通知数据         |
| 调用等级 | 协程内                           |

---

### 时间处理模块（XC_Time，21 个）

#### 基础时间（宏）

##### `XC_Time_GetTickUnit` — 获取 Tick 最小时间单位（us）

| 项       | 内容                                  |
| -------- | ------------------------------------- |
| 原型     | `#define XC_Time_GetTickUnit()`       |
| 返回     | `uint32_t` 一次 Tick 计数的时间（us） |
| 说明     | 即 `1000000 / XC_CFG_TICKS_PER_SEC`   |
| 调用等级 | 任意                                  |

##### `XC_Time_GetTick` — 获取系统 Tick

| 项       | 内容                                    |
| -------- | --------------------------------------- |
| 原型     | `#define XC_Time_GetTick()`             |
| 返回     | `XC_Tick_t` 当前系统 Tick 值            |
| 说明     | 读取全局滴答计数（`XC_SYS_TICK_COUNT`） |
| 调用等级 | 任意 / **中断**                         |

##### `XC_Time_GetMs` — 获取系统运行时间（ms）

| 项       | 内容                                    |
| -------- | --------------------------------------- |
| 原型     | `#define XC_Time_GetMs()`               |
| 返回     | `XC_Tick_t` 系统已运行毫秒数            |
| 说明     | 32 位下 ms 计数最长约 49 天（1ms Tick） |
| 调用等级 | 任意 / **中断**                         |

##### `XC_Time_TickInc` — 递增系统 Tick（宏，默认模式）

| 项       | 内容                                                                                               |
| -------- | -------------------------------------------------------------------------------------------------- |
| 原型     | `#define XC_Time_TickInc()`                                                                        |
| 说明     | 在定时器中断中按 `XC_CFG_TICKS_PER_SEC` 频率调用；仅中断累加模式（`XC_SYS_TICK_INT_INC_MODE`）有效 |
| 调用等级 | **中断**（系统 Tick 源）                                                                           |

##### `XC_Time_TickSet` — 设置系统 Tick（宏，默认模式）

| 项       | 内容                                          |
| -------- | --------------------------------------------- |
| 原型     | `#define XC_Time_TickSet(_Tick)`              |
| 参数     | `_Tick` 要设置的 Tick 值                      |
| 说明     | 特殊情况下使用，如休眠唤醒后重新校准系统 Tick |
| 调用等级 | 任意                                          |

#### 时间比较（溢出安全）

> 以下比较基于无符号减法，**允许 Tick 溢出**；限制：`延时 + 每次查询间隔` 不能超过 Tick 类型值域。

##### `XC_Time_CheckTimeout` — 比较 Tick 是否到达（宏）

| 项       | 内容                                           |
| -------- | ---------------------------------------------- |
| 原型     | `#define XC_Time_CheckTimeout(_Lc, _c)`        |
| 参数     | `_Lc` 上次记录的 Tick；`_c` 需要比较的 Tick 数 |
| 返回     | `1` 时间到达；`0` 未到达                       |
| 说明     | 判断是否已运行 `_c` 个 Tick                    |
| 调用等级 | 任意                                           |

##### `XC_Time_CheckTimeoutMs` / `XC_Time_CheckTimeoutSec` — 时间是否到达（ms/s，宏）

| 项       | 内容                                                                                   |
| -------- | -------------------------------------------------------------------------------------- |
| 原型     | `#define XC_Time_CheckTimeoutMs(_Lc, _t)` / `#define XC_Time_CheckTimeoutSec(_Lc, _t)` |
| 参数     | `_Lc` 上次记录的 Tick；`_t` 时间（ms/s）                                               |
| 返回     | `1` 时间到达；`0` 未到达                                                               |
| 说明     | `XC_Time_CheckTimeout` 的时间单位版本                                                  |
| 调用等级 | 任意                                                                                   |

##### `XC_Time_GetRemain` — 获取剩余 Tick（函数）

| 项       | 内容                                                                     |
| -------- | ------------------------------------------------------------------------ |
| 原型     | `XC_Tick_t XC_Time_GetRemain(XC_Tick_t LastTick, XC_Tick_t CompareTick)` |
| 参数     | `LastTick` 上次记录的 Tick；`CompareTick` 等待到达的 Tick 数             |
| 返回     | 距离到达还差的 Tick 数；`0` 表示已到达或早已到达                         |
| 调用等级 | 任意                                                                     |

##### `XC_Time_GetElapsed` — 获取已运行 Tick（函数）

| 项       | 内容                                                                      |
| -------- | ------------------------------------------------------------------------- |
| 原型     | `XC_Tick_t XC_Time_GetElapsed(XC_Tick_t LastTick, XC_Tick_t CompareTick)` |
| 参数     | `LastTick` 上次记录的 Tick；`CompareTick` 等待到达的 Tick 数              |
| 返回     | 已运行的 Tick 数；等于 `CompareTick` 表示已完成                           |
| 调用等级 | 任意                                                                      |

#### 软件定时器（`XC_TimerTick_t`，非协程场景）

##### `XC_Time_TimerSet` — 更新定时器 Tick（宏）

| 项       | 内容                                                      |
| -------- | --------------------------------------------------------- |
| 原型     | `#define XC_Time_TimerSet(_tC, _c)`                       |
| 参数     | `_tC` `XC_TimerTick_t` 变量；`_c` 需要比较/延时的 Tick 数 |
| 说明     | 记录当前 Tick 并设置等待 Tick 数                          |
| 调用等级 | 任意                                                      |

##### `XC_Time_TimerSetMs` / `XC_Time_TimerSetSec` — 更新定时器时间（ms/s，宏）

| 项       | 内容                                                                           |
| -------- | ------------------------------------------------------------------------------ |
| 原型     | `#define XC_Time_TimerSetMs(_tC, _t)` / `#define XC_Time_TimerSetSec(_tC, _t)` |
| 说明     | 定时器时间单位版本                                                             |
| 调用等级 | 任意                                                                           |

##### `XC_Time_TimerRepeat` — 重复定时器（宏）

| 项       | 内容                                                   |
| -------- | ------------------------------------------------------ |
| 原型     | `#define XC_Time_TimerRepeat(_tC)`                     |
| 说明     | 以当前 Tick 重新计时（保留等待 Tick 数），实现周期定时 |
| 调用等级 | 任意                                                   |

##### `XC_Time_TimerCheck` — 判断定时器是否到达（宏）

| 项       | 内容                              |
| -------- | --------------------------------- |
| 原型     | `#define XC_Time_TimerCheck(_tC)` |
| 返回     | `1` 时间到达；`0` 未到达          |
| 调用等级 | 任意                              |

##### `XC_Time_TimerClr` — 清除定时器（宏）

| 项       | 内容                            |
| -------- | ------------------------------- |
| 原型     | `#define XC_Time_TimerClr(_tC)` |
| 说明     | `XC_TimerTick_t` 变量全部清零   |
| 调用等级 | 任意                            |

##### `XC_Time_TimerGetRemain` — 定时器剩余 Tick（函数）

| 项       | 内容                                                     |
| -------- | -------------------------------------------------------- |
| 原型     | `XC_Tick_t XC_Time_TimerGetRemain(XC_TimerTick_t tTime)` |
| 参数     | `tTime` `XC_TimerTick_t` 变量（按值传）                  |
| 返回     | 剩余 Tick；`0` 表示已到达或早已到达                      |
| 调用等级 | 任意                                                     |

##### `XC_Time_TimerGetElapsed` — 定时器已运行 Tick（函数）

| 项       | 内容                                                      |
| -------- | --------------------------------------------------------- |
| 原型     | `XC_Tick_t XC_Time_TimerGetElapsed(XC_TimerTick_t tTime)` |
| 返回     | 已运行 Tick；等于 `WaitCount` 表示时间到达                |
| 调用等级 | 任意                                                      |

##### `XC_Time_TimerGetElapsedMs` — 定时器已运行时间（ms，宏）

| 项       | 内容                                     |
| -------- | ---------------------------------------- |
| 原型     | `#define XC_Time_TimerGetElapsedMs(_tC)` |
| 返回     | 已运行毫秒数                             |
| 调用等级 | 任意                                     |

#### 死循环延时（阻塞式，非协程）

##### `XC_Time_BlockDelay` / `XC_Time_BlockDelayMs` — 死循环延时（Tick/ms，函数）

| 项       | 内容                                                                                             |
| -------- | ------------------------------------------------------------------------------------------------ |
| 原型     | `void XC_Time_BlockDelay(XC_Tick_t DelayTick)` / `void XC_Time_BlockDelayMs(XC_Tick_t Delay_ms)` |
| 参数     | `DelayTick`/`Delay_ms` 延时值                                                                    |
| 说明     | while 循环忙等待，**会阻塞整个系统**；仅在非协程场景（如初始化、临界段）使用                     |
| 调用等级 | 任意（非协程内推荐用 `XC_Cor_Delay*`）                                                           |

### 诊断模块（XC_Err / XC_Diag，5 个）

> 全部**默认关闭**（开关见 `Code/Inc/XC_Config.h`）⇒ 关档 0 代码 / 0 RAM；本节是"用户实现 / 用户调用"的诊断 API 详表。

#### `XC_Err_Hook` — 错误上报钩子（**由用户实现**）

| 项       | 内容                                                                                                                                                                  |
| -------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 原型     | `void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode);`（声明见 `Code/Inc/XC_Err.h`）                                                                        |
| 参数     | `phTCB` 出错的任务控制块（**断言上报时为 `NULL`**）；`ErrCode` 错误码（`XC_ErrCode_t`）                                                                               |
| 返回     | 无                                                                                                                                                                    |
| 说明     | 框架在"**安全降级之前**"调用它（当前上报点：帧栈越界 `XC_ERR_FRAME_OVERFLOW`、断言 `XC_ERR_ASSERT`）；开关 `XC_CFG_ERR_HOOK=1` 时**必须由用户提供实现**，否则链接报错 |
| 要求     | 可重入、不阻塞、**不得调用会移表的框架 API**；取 `phTCB` 现场前**先判空**                                                                                             |
| 调用等级 | 任意（由框架调用，可能在任务或中断上下文）                                                                                                                            |

#### `XC_Diag_CheckInvariants` — 运行期不变量自检（**调试档**）

| 项       | 内容                                                                                                                                                                                       |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 原型     | `XC_ChkResult_t XC_Diag_CheckInvariants(XC_OSHandle_t phXCOS);`                                                                                                                            |
| 参数     | `phXCOS` 框架句柄                                                                                                                                                                          |
| 返回     | `XC_CHK_OK` 或**首个失败项编号**（见 `XC_ChkResult_t`）                                                                                                                                    |
| 说明     | 盘点四张任务表与每个任务的状态标签：状态↔表匹配、链表双向指针、同一任务不重复挂表、`TaskNum` 与节点总数一致、协程嵌套自洽、表内任务归属；**只读检查**，不改变内核行为，也**不会自动调用** |
| 开关     | `XC_CFG_DEBUG_CHECK=1`（关档时本函数**不编译**）                                                                                                                                           |
| 调用等级 | 任务 / 主循环；**不可在已持锁的上下文中调用**（内部会短暂持锁）                                                                                                                            |

#### `XC_Diag_GetRunCnt` — 取任务运行次数（可选档 `XC_CFG_TASK_STATS`）

| 项       | 内容                                                                                 |
| -------- | ------------------------------------------------------------------------------------ |
| 原型     | `uint32_t XC_Diag_GetRunCnt(XC_TaskHandle_t phTCB);`                                 |
| 参数     | `phTCB` 任务控制块                                                                   |
| 返回     | 该任务被"调度槽"运行的累计次数（**饱和计数**，到 `0xFFFFFFFF` 后保持）               |
| 说明     | 计数口径：任务被摘出就绪表 → 让出为 1 次（含"同轮切父"的父层运行）；注册/移除时清零 |
| 调用等级 | 任意（只读；由调度器在主循环更新）                                                   |

#### `XC_Diag_GetMaxRunTick` — 取任务最长一次运行耗时（可选档 `XC_CFG_TASK_STATS`）

| 项       | 内容                                                                                      |
| -------- | ----------------------------------------------------------------------------------------- |
| 原型     | `XC_Tick_t XC_Diag_GetMaxRunTick(XC_TaskHandle_t phTCB);`                                 |
| 参数     | `phTCB` 任务控制块                                                                        |
| 返回     | 最长一次"调度槽"耗时（单位 Tick；**只增不减**）                                           |
| 说明     | 1 ms 时基下**短于 1 tick 记 0**；含运行期间的**中断时间** ⇒ 是"调度槽耗时"，不是纯指令数 |
| 调用等级 | 任意（只读）                                                                              |

#### `XC_Diag_ClrRunStats` — 清零任务运行统计（可选档 `XC_CFG_TASK_STATS`）

| 项       | 内容                                                                                                      |
| -------- | --------------------------------------------------------------------------------------------------------- |
| 原型     | `void XC_Diag_ClrRunStats(XC_TaskHandle_t phTCB);`                                                        |
| 参数     | `phTCB` 任务控制块                                                                                        |
| 返回     | 无                                                                                                        |
| 说明     | 把 `RunCnt` 与 `MaxRunTick` 都清 0；**注册/移除时框架自动调用**，`XC_Task_Reset` **保留**（排障不丢证据） |
| 调用等级 | 任意                                                                                                      |

---

## 💡 函数调用等级

按调用场景分为 4 个等级，违反调用等级将产生未定义行为：

| 等级                                | 可调用 API                                                                                                                                                                       | 说明                                                                                                                                                           |
| ----------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|                                     |                                                                                                                                                                                  |                                                                                                                                                                |
| **协程内**                          | 全部 `XC_Cor_*` 宏（Enter/Leave/Call/Yield/Suspend/Reset/Remove/Delay*/WaitNotify/ClrNotify/IsNotifyTimeout/GetNotifyData/GetParam）                                             | 必须在 `XC_Cor_Enter`/`XC_Cor_Leave` 之间；`XC_Cor_Call` 需要任务配置帧栈                                                                                      |
|                                     |                                                                                                                                                                                  |                                                                                                                                                                |
| **任意（只读查询 / 通知类）**       | `XC_Time_*`（时间比较/定时器/延时）、`XC_Task_GetState`/`GetParam`/`ReadNotifyData`（只读）、`XC_Task_UpdateNotifyData`（**须与 `SendNotify` 同一生产者**）、`XC_Sch_GetTaskNum` | 普通任务/主流程代码                                                                                                                                            |
|                                     |                                                                                                                                                                                  |                                                                                                                                                                |
| **任务/主循环（移表类，不可中断）** | `XC_Task_Suspend` / `Resume` / `Reset` / `Remove`、`XC_Task_ClrNotify`（消费侧字段）                                                                                             | 会移动链表或写共享字段；**在中断中调用会破坏锁握手**（静默链表损坏）——ISR 若要"挂起/恢复"请走"通知控制任务"（见「并发约束」与 `Skill` §四.7）               |
|                                     |                                                                                                                                                                                  |                                                                                                                                                                |
| **中断**                            | `XC_Task_SendNotify`（通知）、`XC_Time_GetTick`、`XC_Time_GetMs`、`XC_Time_TickInc`（Tick 源）                                                                                   | **仅通知类与 Tick 读取可在中断中调用**（SPSC 线程安全）；挂起/恢复/移除/复位/注册等链表操作 API **不可**在中断中调用；多生产者/CPU 内存重排序需用户加锁/加屏障 |
|                                     |                                                                                                                                                                                  |                                                                                                                                                                |
| **诊断（可选）**                    | `XC_Err_Hook`（**由用户实现**）、`XC_Diag_CheckInvariants`、`XC_Diag_GetRunCnt` / `XC_Diag_GetMaxRunTick` / `XC_Diag_ClrRunStats`                                                | 诊断开关默认全关；`XC_Diag_CheckInvariants` **不可在已持锁的上下文**调用；详见「函数参考 · 诊断模块」                                                         |
|                                     |                                                                                                                                                                                  |                                                                                                                                                                |
| **初始化**                          | `XC_Sch_Init`、`XC_Sch_Start`、`XC_Sch_SetIdleCallback`、`XC_Task_Reg*`/`Add`                                                                                                    | main 中或系统启动阶段                                                                                                                                          |

### 使用建议

1. **协程块内变量**：协程挂起时函数局部变量不保存；需要跨挂起保留的数据存 TCB/静态区/全局区；
2. **任务参数**：若要在协程块内使用注册参数，用 `XC_Cor_GetParam()`（或进入前转存到静态区）；
3. **中断通知**：`XC_Task_SendNotify` 可在中断中调用（SPSC 线程安全），是推荐的 ISR→任务通信方式；
4. **Tick 计数**：确保系统 Tick 持续运行（中断累加模式记得在定时器中断调 `XC_Time_TickInc`）；
5. **错误处理**：返回 `XC_Return_t` 的 API 建议检查返回值；协程块内宏无返回值，通过 `XC_Cor_IsNotifyTimeout`/`XC_Task_GetState` 判断状态。

---

## 🔒 并发约束（用户须知）

框架**不关中断、不做原子操作**：并发相关的**实现细节与字段级契约**（四种并发模型、全字段所有权矩阵、锁的内部语义、字段准入规则）
见 [架构概览 §十二](./架构概览/XCOS_V2.1.0_架构概览.md)。**用户需要遵守的约束**如下：

| 约束              | 内容                                                                                                                                               |
| ----------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| 中断里**只能**调  | `XC_Task_SendNotify`（发通知）、`XC_Time_GetTick` / `XC_Time_GetMs` / `XC_Time_TickInc`（读/递增 Tick）                                            |
| 中断里**禁止**调  | 一切"会移表"的 API：`XC_Task_Reg` / `RegExt` / `Add` / `Remove` / `Reset` / `Suspend` / `Resume`；以及 `XC_Task_ClrNotify`（消费侧字段，会丢唤醒） |
| 发通知是 **SPSC** | 每个任务只允许**一个生产者**（某一个中断**或**某一个任务）；多生产者需用户自己加锁；`XC_Task_UpdateNotifyData` 必须与 `SendNotify` 同一生产者      |
| 挂起优先级最高    | 挂起期间到达的通知**不登记**：`XC_Task_SendNotify` 直接返回 `XC_FAIL`；恢复只能由 `XC_Task_Resume` 完成                                            |
| 无优先级（时延）  | 任一任务的执行时间 = 其它任务的最坏额外延迟；单次执行建议**百微秒量级**，长计算要**分片**（`XC_Cor_Yield`），串口等缓冲型外设用 DMA/环形缓冲       |
| 数据可见性        | 通知数据 `pNotifyData` **不是 volatile**（可见性靠"标志先写 / 后读"的顺序）；若 CPU 会重排序内存访问，需用户自行加**内存屏障**                     |
| 命名              | 新代码只用 V2.1.0 规范名（`XC_<模块>_<功能>`）；旧名见「🔁 向后兼容」                                                                            |

（并发相关的**实现细节**——四种并发模型、全字段矩阵、ISR 合法/禁止、锁的内部语义与字段准入规则——已迁至
[架构概览 §十二](./架构概览/XCOS_V2.1.0_架构概览.md)；本手册只保留上表的**用户约束**。）

---

## 🩺 错误上报（错误上报设施，`XC_Err.h`）

**定位**：框架把「**用户可修复的错误**」上报给用户，便于现场定位；**默认关闭、零开销**（用于替代"出错后毫无痕迹"的场景）。

| 项       | 内容                                                                                                                                                 |
| -------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| 开关     | `XC_CFG_ERR_HOOK`（`0`=关，**默认**；`1`=开）—— 配置细节见 `Docs/XC_Config.md`                                                                     |
| 钩子     | `void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode);`（**由用户实现**）                                                                   |
| 错误码   | `XC_ErrCode_t`（见「常量枚举」节）                                                                                                                   |
| 上报点   | `XC_ERR_FRAME_OVERFLOW`：`XC_Task_PushFrame` 帧栈越界分支（**框架原行为 = 静默复位任务**）                                                           |
| 调用时机 | 在框架**安全降级之前** ⇒ 钩子内可读到出错现场（`CorDepth` / `CorDepthMax` / `fTask`）                                                               |
| 开销     | 关闭：**0 代码 / 0 RAM**；开启：代码约多 **12 字节**（成本在 `XC_Task_PushFrame` 的调用点）—— 精确值见 [`Tests/baseline.md`](../Tests/baseline.md) |
| 并发     | 上报点在**任务上下文**（当前唯一上报点）；钩子实现须**可重入、不阻塞**                                                                               |

```c
/* 1) 打开开关（在包含 XCOS.h 之前定义） */
#define XC_CFG_ERR_HOOK (1U)
#include "XCOS.h"

/* 2) 实现钩子（名字与签名固定） */
void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)
{
    if(ErrCode == XC_ERR_FRAME_OVERFLOW) {
        g_errCnt++;              /* 计数 / 写日志 / 点亮指示灯 */
        (void)phTCB->CorDepthMax; /* ==0 ⇒ 未配帧栈; ==CorDepth ⇒ 嵌套过深 */
    }
}
```

**约束**：钩子内**不得**调用会移表的框架 API（`Reg`/`Remove`/`Reset`/`Suspend`/`Resume` 等），不得长时间阻塞；关闭开关时**无需**实现该函数（也不会产生未定义引用）。

> **示例里的 `phTCB->CorDepthMax` 是"调试期只读内部字段"**（字段定义在 `Internal/`，属实现细节，**不承诺跨版本稳定**）；
> 正式代码请**只按 `ErrCode` 处理**（`XC_ERR_FRAME_OVERFLOW` 即帧栈越界），需要现场信息时用调试器查看 TCB。

---

## 🧩 断言（诊断，`XC_CFG_ASSERT`）

**定位**：把"本不该发生"的前提（参数非空、状态前提）写成**可上报**的断言；**只上报，不改变控制流**（不 return、不止损）。**默认关闭 ⇒ 0 代码 / 0 RAM**。

| 项                                 | 内容                                                                                                                                                                                                                                                                                                                          |
| ---------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 开关                               | `XC_CFG_ASSERT`（`0`=关，**默认**）+ **必须同时开 `XC_CFG_ERR_HOOK`**（只开断言 ⇒ **编译报错**）                                                                                                                                                                                                                             |
| 宏                                 | `XC_DIAG_ASSERT(cond)`（`Code/Inc/Internal/XC_DiagInternal.h`，由 `XC_Diag.h` 自动包含；条件为假时**只上报**）                                                                                                                                                                                                                |
|                                    |                                                                                                                                                                                                                                                                                                                               |
| **失败时怎么处理**（用户最关心的） | ① 框架调用**你实现的** `XC_Err_Hook(phTCB, XC_ERR_ASSERT)` ⇒ 在钩子里对 `XC_ERR_ASSERT` **打断点**，调用栈直接指到出错的那一行（还能看当时的变量）；<br>② **不影响控制流**：断言之后程序继续按原逻辑走（框架原有的 `if(...) return XC_FAIL;` 校验照旧）；<br>③ 断言上报时 **`phTCB == NULL`**（帧栈越界上报才有任务现场） |
| 开销                               | 关：**0 B**（宏展开为空、条件不求值）；开：内核内 26 处断言，**体积见 [`Tests/baseline.md`](../Tests/baseline.md)**                                                                                                                                                                                                           |
| 你要做的                           | ① 打开两个开关；② 实现钩子（见下例）；③ 在钩子里**先判空**，再打点/记录                                                                                                                                                                                                                                                    |

```c
#define XC_CFG_ERR_HOOK (1U)
#define XC_CFG_ASSERT    (1U)     /* 需与钩子同开 */
#include "XCOS.h"

void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)
{
    if(ErrCode == XC_ERR_ASSERT) {
        /* 断点放这里: 调用栈 = 断言位置 */
        (void)phTCB;    /* 断言上报时 phTCB == NULL */
    }
}
```

> **前置契约**：参数写错（`NULL`、非法参数组合）属**调用方编程错误** ⇒ 框架**只在调试档断言**、发布档不检查，违背 = **未定义行为**；
> 而"表满 / 重复注册 / 未挂起"这类**运行期前提**一律**返回错误码**（照旧检查）。
> 断言与返回码的**分工规则（维护者视角）**、以及"为什么断言不带文件/行号"的实测结论，见 [架构概览 §11.6](./架构概览/XCOS_V2.1.0_架构概览.md)。

---

## 📊 运行统计（可选，`XC_CFG_TASK_STATS`）

**定位**：协作式内核**无优先级** ⇒ 任一任务的执行时间就是其它任务的额外延迟（见 [架构概览](./架构概览/XCOS_V2.1.0_架构概览.md) 的「并发模型与字段所有权」）；本开关让框架**自动记录每任务的运行次数与最长一次耗时**，用于回答"**谁拖慢了系统**"。**默认关闭 ⇒ 发布 0 代码 / 0 RAM**。

| 项       | 内容                                                                                                                                                                                           |
| -------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 开关     | `XC_CFG_TASK_STATS`（`0`=关，**默认**；`1`=开）—— 细节见 `Docs/XC_Config.md`                                                                                                                 |
| TCB 字段 | `RunCnt`（`uint32_t`，饱和计数）、`MaxRunTick`（`XC_Tick_t`）—— 仅开关打开时存在                                                                                                             |
| API      | `XC_Diag_GetRunCnt()` / `XC_Diag_GetMaxRunTick()` / `XC_Diag_ClrRunStats()`                                                                                                                    |
| 统计口径 | 一次"**调度槽**"= 从任务被摘出就绪表 → 到它让出（延时/等待/挂起/让出），含"同轮切父"的父层；**等待期间不计时**                                                                                |
| 开销     | 关：**0 代码 / 0 RAM**；开：**每个任务多 8 字节 RAM**（12 任务示例共 +96 字节）、代码约多 **88 字节**（示例工程含演示代码 +152 字节）—— 精确值见 [`Tests/baseline.md`](../Tests/baseline.md) |
| 生命周期 | 注册/移除清零（本槽内移除自身不再累计）；`XC_Task_Reset` **保留**；`ClrRunStats` 手工清零                                                                                                      |
| 并发     | **主上下文**（仅调度器写；用户/调试只读）                                                                                                                                                      |

```c
#define XC_CFG_TASK_STATS (1U)
#include "XCOS.h"

uint32_t  runs = XC_Diag_GetRunCnt(&s_TaskA);     /* 运行次数 */
XC_Tick_t peak = XC_Diag_GetMaxRunTick(&s_TaskA); /* 最长一次(Tick; 默认 1ms/tick) */
/* peak 偏大 ⇒ 该任务需分片(XC_Cor_Yield)/拆分; 结合 runs 可估平均占用 */
```

**边界**：1 ms 时基下**短于 1 tick 记 0**（要更细用计数器模式/高频档）；耗时**含运行期间的中断时间** ⇒ 是"调度槽耗时"而非纯指令数（指令周期请用 MDK 仿真测量，二者互补）；`RunCnt` 饱和不回绕。

---

## 🧪 运行期自检（调试，`XC_CFG_DEBUG_CHECK`）

**定位**：内核多处依赖「任务状态 ↔ 所在表一致」等**不变量**（唤醒路径先看 `TaskState` 再决定移表……），但**运行期默认不做任何检查**；历史上两个缺陷（僵尸状态、挂起任务被误唤醒）都属于不变量被破坏。本开关提供 `XC_Diag_CheckInvariants()`，在开发/回归期把它们**在更早的时刻**暴露出来。**默认关闭 ⇒ 发布 0 代码 / 0 RAM**。

| 项       | 内容                                                                                                                                                                                       |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 开关     | `XC_CFG_DEBUG_CHECK`（`0`=关，**默认**；`1`=开）—— 配置细节见 `Docs/XC_Config.md`                                                                                                        |
| 函数     | `XC_ChkResult_t XC_Diag_CheckInvariants(XC_OSHandle_t phXCOS);`（仅开关打开时编译）                                                                                                        |
| 返回值   | `XC_CHK_OK`（一致）或**第一个失败项编号**，见「常量枚举」节的 `XC_ChkResult_t`                                                                                                             |
| 检查项   | ① 状态↔表匹配；② 链表双向指针 + 步数上限（防环）；③ 同一任务不重复挂表；④ `TaskNum`==四表节点总数（**运行中的任务不在任何表内 ⇒ 允许恰好少 1**）；⑤ 协程嵌套自洽；⑥ 表内任务归属   |
| 开销     | 关闭：**0 代码 / 0 RAM**；开启：代码约多 **444 字节**（都在诊断模块 `XC_Diag.c`；内核侧不增加），Flash/RAM 其它项不变 —— 精确值见 [`Tests/baseline.md`](../Tests/baseline.md)            |
| 并发     | **只读检查**，内部**短暂持锁** ⇒ **不可在已持锁的上下文**调用；**任务上下文可调用**（推荐用法"每个 API 调用之后"在任务里同样有效，见下例）；不改变内核行为、不会自动调用                  |

```c
/* 1) 打开开关 */
#define XC_CFG_DEBUG_CHECK (1U)
#include "XCOS.h"

/* 2) 在"每个 API 调用之后"(回归用例)或"空闲回调里定期"调用 */
XC_ChkResult_t rc = XC_Diag_CheckInvariants(&g_xc);
if(rc != XC_CHK_OK) {
    /* rc 即第一个失败项编号: 2=状态↔表不符(僵尸状态类); 3=重复挂表; 4=计数不符; 5=嵌套; 6=归属 */
}
```

**典型用法**：
1. **回归用例**里每个 API 调用之后调一次 ⇒ 在"出错的那一步"立刻报出不一致（定位最强，推荐）；
2. **运行期守护**：`XC_Sch_SetIdleCallback` 注册的空闲回调里每隔若干轮抽查一次；
3. **排障**：怀疑"任务卡死 / 莫名重启"时，在可疑点手工插入一次。

**注意**：关闭开关时该函数**不编译**，调用点应放在 `#if(XC_CFG_DEBUG_CHECK != 0)` 内；失败后如何上报由用户决定（可直接复用错误上报钩子 `XC_Err_Hook`）。

---

## 🔄 调度顺序与唤醒语义（就绪表插入规则）

> **一句话规则**：**任何任务「进入就绪表」都排在队尾**（严格轮转）—— 不论它是被唤醒、定时到点、复位、恢复，还是新注册。
> 语义依据 = **V2.0 的既定设计**（V2.1.0 调度器重构时曾漂移为"唤醒插队到表首"，**已按 V2.0 语义回归**，见 `Docs/CHANGELOG.md`）。

| 事件                                                         | 落点                                       | 说明                             |
| ------------------------------------------------------------ | ------------------------------------------ | -------------------------------- |
| 任务让出（`Yield`/`Delay`/`WaitNotify`/`Leave`/直接 return） | **就绪表尾**                               | 调度器"归位"（自环判定）         |
| 等待中收到通知（`SendNotify` 直接路径 / 事件调度落地）       | **就绪表尾**                               | 改前为表首（插队）               |
| 提前收到通知（任务尚未等待 ⇒ 不阻塞）                       | **就绪表尾**                               | 与上一条**统一**（本规则的核心） |
| 延时/超时到点唤醒（`XC_Sch_TimeSched`）                      | **就绪表尾**                               | 改前为表首                       |
| `XC_Task_Reset` / 帧栈越界降级（`HandleReset`）              | **就绪表尾**                               | 改前为表首                       |
| `XC_Task_Resume`（恢复挂起，含交付挂起前的通知）             | **就绪表尾**                               | 改前为表首                       |
| 新注册（`XC_Task_Reg` / `XC_Task_Add`）                      | **就绪表尾**                               | 改前为表首（"后注册先跑"）       |
| `XC_Task_Suspend` / 阻塞（延时、等待通知）                   | 阻塞表 / 时间表                            | 不涉及就绪表                     |
| Tick 溢出回绕（时间表批量搬移）                              | **就绪表尾**（整表追加，保持原有相对顺序） | 改前为表首                       |

**后果（用户须知）**

* ✅ **公平**：所有任务（含 `fIdle` 的运行机会）按轮转均分 CPU；**过载时不会出现"某任务独占调度器、其它任务与 idle 饿死"**（这正是"唤醒插队"在"事件率 ≥ 1/调度槽"时的问题）；
* ⚠️ **唤醒最坏延迟 = 一轮**（≤ 就绪任务数 `N` 个调度槽）—— 被唤醒的任务需等其它就绪任务各跑一次；对本框架（**协作式、无优先级**）这是**设计取舍**（时延建议见「🔒 并发约束」）；
* ⚠️ **注册顺序 = 首次执行顺序**（不再"后注册的先跑"）；
* ⚠️ 需要"某任务更快响应"时：**减少就绪任务数 / 缩短单次执行时间（分片 `XC_Cor_Yield`）**，而**不要**依赖入表位置；
* ℹ️ 过载（事件率 ≥ 1/调度槽）时的行为：通知被"**1 bit 待处理标志**"合并（**不丢**），各任务按轮转变慢 ⇒ **优雅降级**，而非"只跑一个任务"。

---

## 🔁 向后兼容（XC_Compat.h）

V2.1.0 起函数/宏统一命名规范（`XC_<模块>_<功能>`）。旧名称仍可通过 `XC_Compat.h`（`XCOS.h` 已包含）使用，**仅供迁移期，后续版本将删除，新代码请使用新名称**：

### 弃用策略与时间表（2026-09-14 定）

| 项                               | 内容                                                                                                                                                     |
| -------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 当前状态                         | `XC_Compat.h` 提供旧名（`XCTask_*` / `XCSch_*` / `XCTime_*` 等），随 `XCOS.h` **自动包含**（框架不提供开/关宏）                                          |
|                                  |                                                                                                                                                          |
| **计划删除版本**                 | **V2.2.0**（届时删除 `XC_Compat.h` 及其包含）                                                                                                            |
| 迁移期建议                       | 新代码只用新名；旧代码按下面的映射表逐个替换（`Examples/` 与 `Docs/Skill/` 已全部使用新名，可作对照）                                                    |
| 提前暴露旧名用法（可选，应用侧） | 框架**不提供开关**（保持 `XCOS.h` 的包含行为不变）；如需要，可在自己的工程侧自检：对旧名做编译期检查（如 `#ifdef XCTask_Reg` → `#error`）或直接源码检索 |
| 删除时要求                       | 须在 `CHANGELOG.md` 记录为**破坏性变更**并附迁移指引；`XC_Compat.h` 文件头已标注 `@deprecated`                                                           |

### 协程块（旧前缀 `XC_`）

| 旧名称                        | 新名称                   |
| ----------------------------- | ------------------------ |
| `XC_Enter`                    | `XC_Cor_Enter`           |
| `XC_Leave`                    | `XC_Cor_Leave`           |
| `XC_GetParam`                 | `XC_Cor_GetParam`        |
| `XC_Yield`                    | `XC_Cor_Yield`           |
| `XC_Suspend`                  | `XC_Cor_Suspend`         |
| `XC_Reset`                    | `XC_Cor_Reset`           |
| `XC_Remove`                   | `XC_Cor_Remove`          |
| `XC_DelayTick`                | `XC_Cor_DelayTick`       |
| `XC_DelayUs`                  | `XC_Cor_DelayUs`         |
| `XC_DelayMs`                  | `XC_Cor_DelayMs`         |
| `XC_DelaySec`                 | `XC_Cor_DelaySec`        |
| `XC_WaitForNotify`            | `XC_Cor_WaitNotify`      |
| `XC_WaitForNotifyMs`          | `XC_Cor_WaitNotifyMs`    |
| `XC_ClrNotify`                | `XC_Cor_ClrNotify`       |
| `XC_CheckNotifyWakeupTimeout` | `XC_Cor_IsNotifyTimeout` |
| `XC_GetNotifyData`            | `XC_Cor_GetNotifyData`   |

### 调度器（旧前缀 `XCSch_`）

| 旧名称                  | 新名称                   |
| ----------------------- | ------------------------ |
| `XCSch_Init`            | `XC_Sch_Init`            |
| `XCSch_Start`           | `XC_Sch_Start`           |
| `XCSch_GetTaskNum`      | `XC_Sch_GetTaskNum`      |
| `XCSch_SetIdleCallback` | `XC_Sch_SetIdleCallback` |

### 任务（旧前缀 `XCTask_`）

| 旧名称                    | 新名称                     |
| ------------------------- | -------------------------- |
| `XCTask_Reg`              | `XC_Task_Reg`              |
| `XCTask_Remove`           | `XC_Task_Remove`           |
| `XCTask_SetEntry`         | `XC_Task_SetEntry`         |
| `XCTask_Add`              | `XC_Task_Add`              |
| `XCTask_Reset`            | `XC_Task_Reset`            |
| `XCTask_Suspend`          | `XC_Task_Suspend`          |
| `XCTask_Resume`           | `XC_Task_Resume`           |
| `XCTask_SendNotify`       | `XC_Task_SendNotify`       |
| `XCTask_ClrNotify`        | `XC_Task_ClrNotify`        |
| `XCTask_UpdateNotifyData` | `XC_Task_UpdateNotifyData` |
| `XCTask_ReadNotifyData`   | `XC_Task_ReadNotifyData`   |
| `XCTask_GetParam`         | `XC_Task_GetParam`         |
| `XCTask_GetState`         | `XC_Task_GetState`         |

### 时间（旧前缀 `XCTime_`）

| 旧名称                     | 新名称                      |
| -------------------------- | --------------------------- |
| `XCTime_GetTickUnit`       | `XC_Time_GetTickUnit`       |
| `XCTime_GetTick`           | `XC_Time_GetTick`           |
| `XCTime_GetMs`             | `XC_Time_GetMs`             |
| `XCTime_TickInc`           | `XC_Time_TickInc`           |
| `XCTime_TickSet`           | `XC_Time_TickSet`           |
| `XCTime_CheckTimeout`      | `XC_Time_CheckTimeout`      |
| `XCTime_CheckTimeoutMs`    | `XC_Time_CheckTimeoutMs`    |
| `XCTime_CheckTimeoutSec`   | `XC_Time_CheckTimeoutSec`   |
| `XCTime_GetRemain`         | `XC_Time_GetRemain`         |
| `XCTime_GetElapsed`        | `XC_Time_GetElapsed`        |
| `XCTime_TimerSet`          | `XC_Time_TimerSet`          |
| `XCTime_TimerSetMs`        | `XC_Time_TimerSetMs`        |
| `XCTime_TimerSetSec`       | `XC_Time_TimerSetSec`       |
| `XCTime_TimerRepeat`       | `XC_Time_TimerRepeat`       |
| `XCTime_TimerCheck`        | `XC_Time_TimerCheck`        |
| `XCTime_TimerClr`          | `XC_Time_TimerClr`          |
| `XCTime_TimerGetRemain`    | `XC_Time_TimerGetRemain`    |
| `XCTime_TimerGetElapsed`   | `XC_Time_TimerGetElapsed`   |
| `XCTime_TimerGetElapsedMs` | `XC_Time_TimerGetElapsedMs` |
| `XCTime_BlockDelay`        | `XC_Time_BlockDelay`        |
| `XCTime_BlockDelayMs`      | `XC_Time_BlockDelayMs`      |

> **其他变更**（V2.0 → V2.1.0 的**重命名**，均**不在** `XC_Compat.h` 中提供别名 ⇒ 升级旧工程需**手工替换**）：
> 类型 `XCBP_t` → `XC_BP_t`；`XCListNode_t` → `XC_ListNode_t`（struct 标签统一 `XC_ListNode_tag`）；
> 二进制宏 `_B*` → `BIN_*`（`Code/Inc/XC_BitPatterns.h` 现只提供 `BIN_*`）；框架兼容标识 `_XCOS_` → `XCOS_API_LEVEL`。

---

<div align="center">

**📖 完整示例请参考: [示例文档](./Examples.md) • ⚙️ 配置说明请参考: [配置文档](./XC_Config.md) • 架构实现: [架构概览](./架构概览/XCOS_V2.1.0_架构概览.md)**

</div>
