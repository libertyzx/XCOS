# XCOS API 参考手册

<div align="center">

![API](https://img.shields.io/badge/API-参考手册-blue.svg)
![版本](https://img.shields.io/badge/版本-2.1.0-green.svg)
![标准](https://img.shields.io/badge/标准-ANSI/GNU-orange.svg)

**XCOS 框架完整 API 参考：数据类型、常量枚举、函数与宏定义（V2.1.0）**

[基础定义](#-基础定义) • [类型系统](#-类型系统) • [常量枚举](#-常量枚举) • [二进制模式](#-二进制模式) • [函数参考](#-函数参考) • [调用等级](#-函数调用等级) • [向后兼容](#-向后兼容)

</div>

## 🏗️ 架构概览

![XCOS框架架构图](./架构概览/XCOS.png)
**XCOS框架架构图**

XCOS 为**协作式协程操作系统**：任务以函数形式编写，内部用协程块（`XC_Cor_*` 宏）表达执行流程；调度器在主循环中轮询就绪表，逐个运行任务。系统由**调度器（XC_Sch）**、**任务（XC_Task）**、**协程块（XC_Cor）**、**时间（XC_Time）** 与底层**链表（XC_List）**、**内核锁（XC_Core）** 组成。

> 完整实现细节（数据结构、调度流程、内存开销、边界限制）请参考 [架构概览](./架构概览/XCOS_V2.1.0_架构概览.md)。

---

## 📋 基础定义

### 框架标识宏

| 宏名              | 值      | 文件     | 说明                              |
| ---               | ---     | ---      | ---                               |
| `XCOS_API_LEVEL`  | `2`     | XCOS.h   | 框架 API 兼容性标识（指向主版本号，用于兼容性检查） |

### 版本信息

| 宏名              | 值            | 文件     | 说明          |
| ---               | ---           | ---      | ---           |
| `XCOS_VER_MAJOR`  | `2`           | XCOS.h   | 主版本号      |
| `XCOS_VER_MINOR`  | `1`           | XCOS.h   | 次版本号      |
| `XCOS_VER_PATCH`  | `0`           | XCOS.h   | 修订号        |
| `XCOS_VER_STRING` | `"2.1.0"`     | XCOS.h   | 字符串版本    |
| `XCOS_VER_NUMBER` | `0x020100`    | XCOS.h   | 版本数值（24位：主·次·修订） |

---

## 🏗️ 类型系统

### 核心数据类型

| 类型名            | 大小(32位) | 文件           | 说明                           |
| ---               | ---        | ---            | ---                            |
| `XC_Tick_t`       | 4 Bytes    | XC_Config.h    | 系统 Tick 计数类型（可配置，默认 `uint32_t`） |
| `XC_TimerTick_t`  | 8 Bytes    | XC_Type.h      | 软件定时器计数类型（`TickCount` + `WaitCount`） |
| `XCOS_t`          | 48 Bytes   | XC_Type.h      | XCOS 框架实例结构体（不透明）   |
| `XC_OSHandle_t`   | 4 Bytes    | XC_Type.h      | 框架操作句柄（`XCOS_t*`）       |
| `XC_TaskCB_t`     | 44 Bytes   | XC_Type.h      | 任务控制块实例（不透明，V2.1.0 含协程嵌套字段） |
| `XC_TaskHandle_t` | 4 Bytes    | XC_Type.h      | 任务操作句柄（`XC_TaskCB_t*`）  |

### 句柄与不透明指针

- `XC_OSHandle_t` = `XCOS_t*`：框架句柄，所有模块操作的第一参数；
- `XC_TaskHandle_t` = `XC_TaskCB_t*`：任务句柄；
- 两者均为**不透明指针**：内部结构只在 `Internal/` 头文件定义，用户不可直接访问成员，通过 API/宏操作；
- 一个工程可创建多个 XCOS 实例（多个框架句柄），互不影响。

### 协程相关类型（V2.1.0 新增）

> 以下类型供协程嵌套使用，定义于 `Internal/XC_TypeInternal.h`（用户可声明变量，不访问内部）。

| 类型名            | 大小(32位) | 说明                           |
| ---               | ---        | ---                            |
| `XC_CorFn_t`      | 4 Bytes    | 协程函数指针：`void (*)(XC_TaskHandle_t)`（顶层任务与子协程统一签名） |
| `XC_CorFrame_t`   | 8 Bytes    | 协程帧（每层一个：父层入口 `pfn` + 父层返回点 `BP`） |
| `XC_CorState_t`   | 1 Byte     | 本轮结果：`XC_COR_DONE`（本层完成）/ `XC_COR_SUSPENDED`（本层挂起） |

---

## 🎯 常量枚举

### 函数返回值类型（`XC_Return_t`）

| 名称          | 值    | 说明              |
| ---           | ---   | ---               |
| `XC_OK`       | 0     | 成功              |
| `XC_FAIL`     | 1     | 失败或无效        |
| `XC_CONTINUE` | 2     | 操作继续或未完成（异步操作中） |

### 任务状态类型（`XC_TaskState_t`）

| 名称                  | 值    | 说明                                          |
| ---                   | ---   | ---                                           |
| `XC_TASK_VOID`        | 0     | 空（任务创建前或被移除后的状态）               |
| `XC_TASK_RUN`         | 1     | 运行（正在运行的任务）                         |
| `XC_TASK_READY`       | 2     | 就绪（在就绪表中的任务状态，任务注册后为就绪） |
| `XC_TASK_BLOCKED`     | 3     | 阻塞（延时、等待通知后的状态）                 |
| `XC_TASK_SUSPEND`     | 4     | 挂起                                          |

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

---

## 🔢 二进制模式

### 二进制常量定义（`XC_BitPatterns.h`）

提供可视化的二进制模式宏，便于位操作；支持 1~8 位任意二进制常量，可使用下划线 `_` 按“4位_4位”分组提高可读性，直接映射为十六进制值。

| 常量名        | 值    | 说明              |
| ---           | ---   | ---               |
| `BIN_0`       | 0x00  | 二进制 0          |
| `BIN_0000_0001` | 0x01 | 二进制 00000001   |
| `BIN_1010_0011` | 0xA3 | 二进制 10100011   |
| `BIN_1111_1001` | 0xF9 | 二进制 11111001   |

**特点**:
- 前缀为 `BIN_`（V2.1.0 起，旧前缀 `_B` 已废弃）；
- 支持 1~8 位任意二进制常量；
- 可使用下划线提高可读性（只支持“4位_4位”分组）；
- 直接映射为十六进制值。

## 🛠️ 函数参考

> **约定**：
> - 类型列：`函数`（实函数，位于 `.c`）或 `宏`（头文件内联宏）；
> - 调用等级：`协程内` = 必须在协程块（`XC_Cor_Enter`/`XC_Cor_Leave` 之间）使用；`中断` = 可在中断服务程序中调用（**仅通知类与 Tick 类 API**，SPSC 线程安全）；详见 [函数调用等级](#-函数调用等级)。

### 调度器模块（XC_Sch，4 个）

#### `XC_Sch_Init` — 调度器初始化

| 项     | 内容 |
| ---    | ---  |
| 原型   | `void XC_Sch_Init(XC_OSHandle_t phXCOS)` |
| 参数   | `phXCOS` 框架句柄 |
| 返回   | 无 |
| 说明   | 创建好 XCOS 实例后调用，初始化框架锁、就绪/时间/溢出/阻塞四表、事件计数等；**调度器启动前必须调用** |
| 调用等级 | 初始化阶段（main 中） |

#### `XC_Sch_Start` — 调度器启动（阻塞）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `void XC_Sch_Start(XC_OSHandle_t phXCOS)` |
| 参数   | `phXCOS` 框架句柄 |
| 返回   | 无 |
| 说明   | 阻塞运行调度器主循环：轮询就绪表运行任务、时间调度、事件调度、空闲回调；**调用后不返回** |
| 调用等级 | 初始化阶段（main 中，最后一步） |

#### `XC_Sch_GetTaskNum` — 获取任务数

| 项     | 内容 |
| ---    | ---  |
| 原型   | `uint8_t XC_Sch_GetTaskNum(XC_OSHandle_t phXCOS)` |
| 参数   | `phXCOS` 框架句柄 |
| 返回   | `uint8_t` 当前框架中的任务数量 |
| 说明   | 直接读取框架句柄中的任务数字段 |
| 调用等级 | 任意（非协程内） |

#### `XC_Sch_SetIdleCallback` — 设置空闲处理回调

| 项     | 内容 |
| ---    | ---  |
| 原型   | `void XC_Sch_SetIdleCallback(XC_OSHandle_t phXCOS, void (*fIdle)(XC_OSHandle_t, XC_Tick_t))` |
| 参数   | `phXCOS` 框架句柄；`fIdle` 空闲回调函数指针（传 `NULL` 清除回调） |
| 返回   | 无 |
| 说明   | 就绪表为空时调用回调。回调参数：`phXCOS` 框架句柄、`IdleTick` 空闲的 Tick 数（`~0U` 表示可无限休眠）；若休眠会停止 Tick 计数，须在唤醒时更新 `XC_Time_TickSet` |
| 回调原型 | `void fIdle(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)` |
| 调用等级 | 初始化阶段（main 中） |

---

### 任务管理模块（XC_Task，15 个）

#### 任务注册与移除

##### `XC_Task_Reg` — 任务注册

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Return_t XC_Task_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)` |
| 参数   | `phXCOS` 框架句柄；`phTCB` 任务控制块；`fTask` 任务函数指针；`pParam` 传递给任务的参数 |
| 返回   | `XC_OK` 注册成功；`XC_FAIL` 注册失败（任务太多） |
| 说明   | 注册一个普通任务（无协程嵌套）。**不可多次注册同一个任务**（无重复判断，会引发未知行为）；注册时任务进入就绪表 |
| 调用等级 | 初始化阶段 / 运行期（非协程内） |

##### `XC_Task_RegExt` — 任务注册（扩展：支持协程嵌套）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Return_t XC_Task_RegExt(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, XC_CorFn_t fTask, void* pParam, XC_CorFrame_t* pCorStack, uint8_t CorDepthMax)` |
| 参数   | `phXCOS` 框架句柄；`phTCB` 任务控制块；`fTask` 任务函数指针；`pParam` 任务参数；`pCorStack` 协程帧栈（无嵌套传 `NULL`）；`CorDepthMax` 帧栈容量/最大嵌套层数（无嵌套传 `0`） |
| 返回   | `XC_OK` 成功；`XC_FAIL` 失败（任务太多，或帧栈/容量参数不匹配） |
| 说明   | V2.1.0 新增。等价于 `XC_Task_Reg` + `XC_Task_SetCorStack`；`pCorStack`/`CorDepthMax` 必须同为有/同为无；需要嵌套的任务必须用本函数或 `XC_Task_SetCorStack` 配置帧栈 |
| 调用等级 | 初始化阶段 / 运行期（非协程内） |

##### `XC_Task_SetCorStack` — 设置协程帧栈（支持/撤销嵌套）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Return_t XC_Task_SetCorStack(XC_TaskHandle_t phTCB, XC_CorFrame_t* pCorStack, uint8_t CorDepthMax)` |
| 参数   | `phTCB` 任务控制块；`pCorStack` 协程帧栈（撤销传 `NULL`）；`CorDepthMax` 容量（撤销传 `0`） |
| 返回   | `XC_OK` 成功；`XC_FAIL` 帧栈/容量参数不匹配 |
| 说明   | V2.1.0 新增。传 `(NULL, 0U)` 撤销嵌套能力；重新设置会清空当前深度（`CorDepth=0`）；任务嵌套中更换配置前应先复位清栈 |
| 调用等级 | 非协程内 |

##### `XC_Task_Remove` — 任务移除

| 项     | 内容 |
| ---    | ---  |
| 原型   | `void XC_Task_Remove(XC_TaskHandle_t phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 返回   | 无 |
| 说明   | 移除一个任务：出表、清栈、`TaskNum--`、状态回 `XC_TASK_VOID`、`phXCOS=NULL`。**不可移除自身**（移除自身使用 `XC_Cor_Remove`）；与 `XC_Task_Reset` 类似，唤醒源不清除 |
| 调用等级 | 任意（非协程内） |

#### 分步创建

##### `XC_Task_SetEntry` — 设置任务入口

| 项     | 内容 |
| ---    | ---  |
| 原型   | `void XC_Task_SetEntry(XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)` |
| 参数   | `phTCB` 任务控制块；`fTask` 任务函数指针；`pParam` 任务参数 |
| 返回   | 无 |
| 说明   | 只设置任务入口和参数，配合 `XC_Task_Add` 分步注册使用；**嵌套中换入口会使栈底帧与新入口不一致，换前须先清栈** |
| 调用等级 | 非协程内 |

##### `XC_Task_Add` — 添加任务

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Return_t XC_Task_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB)` |
| 参数   | `phXCOS` 框架句柄；`phTCB` 任务控制块 |
| 返回   | `XC_OK` 成功；`XC_FAIL` 任务太多 |
| 说明   | 将已设置入口的任务加入调度器（就绪表）。与 `XC_Task_SetEntry` 组合可替代 `XC_Task_Reg` |
| 调用等级 | 非协程内 |

#### 复位 | 挂起 | 恢复

##### `XC_Task_Reset` — 任务复位

| 项     | 内容 |
| ---    | ---  |
| 原型   | `void XC_Task_Reset(XC_TaskHandle_t phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 返回   | 无 |
| 说明   | 复位指定任务：清断点、清帧栈、从任务函数头重新开始；**不可复位自身**（复位自身使用 `XC_Cor_Reset`） |
| 调用等级 | 任意（非协程内） |

##### `XC_Task_Suspend` — 任务挂起

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Return_t XC_Task_Suspend(XC_TaskHandle_t phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 返回   | `XC_OK` 挂起成功；`XC_FAIL` 失败（任务不存在）；`XC_CONTINUE` 异步操作中 |
| 说明   | 将任务挂起，本次任务运行完成后暂停；挂起后不再被调度（延时的任务也可挂起） |
| 调用等级 | 任意（非协程内；**不可在中断中调用**） |

##### `XC_Task_Resume` — 任务挂起恢复

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Return_t XC_Task_Resume(XC_TaskHandle_t phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 返回   | `XC_OK` 恢复成功；`XC_FAIL` 失败（任务未挂起） |
| 说明   | 只能恢复被挂起的任务；恢复后回到就绪表 |
| 调用等级 | 任意（非协程内；**不可在中断中调用**） |

#### 通知（SPSC，线程安全）

##### `XC_Task_SendNotify` — 发送通知

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Return_t XC_Task_SendNotify(XC_TaskHandle_t phTCB, void* pNotifyData)` |
| 参数   | `phTCB` 需要发送通知的任务 TCB；`pNotifyData` 通知传递的参数 |
| 返回   | `XC_OK` 通知成功；`XC_FAIL` 任务不存在或任务被挂起；`XC_CONTINUE` 异步操作中 |
| 说明   | 发送通知并唤醒任务。**线程安全（SPSC）**：单生产者单消费者模型，可在中断中调用；可在“等待通知”前发送（提前通知，等待将立即结束）；唤醒后通知计数清零；多生产者时须用户加锁保护 |
| 调用等级 | 任意 / **中断** |

##### `XC_Task_ClrNotify` — 清除通知（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Task_ClrNotify(phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 说明   | 清除通知计数；可在“等待通知”前调用，防止通知提前到达 |
| 调用等级 | 任意 |

##### `XC_Task_UpdateNotifyData` — 更新通知数据（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Task_UpdateNotifyData(phTCB, _pNotifyData)` |
| 参数   | `phTCB` 任务控制块；`_pNotifyData` 通知数据 |
| 说明   | 不使用任务通知时，可用通知数据字段传递数据（写） |
| 调用等级 | 任意 |

##### `XC_Task_ReadNotifyData` — 读取通知数据（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Task_ReadNotifyData(phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 返回   | `void*` 通知数据 |
| 说明   | 读取通知数据字段 |
| 调用等级 | 任意 |

#### 参数与状态（宏）

##### `XC_Task_GetParam` — 获取任务参数（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Task_GetParam(phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 返回   | `void*` 注册时传递的参数 |
| 说明   | 获取任务注册时传递的参数。**注意**：协程内上下文切换局部变量不保存，若要在协程块内使用参数，应在 `XC_Cor_Enter` 前赋值给静态/全局变量，或使用 `XC_Cor_GetParam` |
| 调用等级 | 任意 |

##### `XC_Task_GetState` — 获取任务状态（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Task_GetState(phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 返回   | `XC_TaskState_t` 任务状态（`XC_TASK_VOID/RUN/READY/BLOCKED/SUSPEND`） |
| 说明   | 获取任务运行状态 |
| 调用等级 | 任意 |

### 协程块模块（XC_Cor，17 个，均须在协程块内使用）

> **协程块**是 XCOS 的核心执行模型：`XC_Cor_Enter` 开始，`XC_Cor_Leave` 结束；块内的延时/等待通知/让出/挂起等操作会自动“挂起+恢复”。所有 `XC_Cor_*` 宏**必须在协程块内调用**。

#### 进入与离开

##### `XC_Cor_Enter` — 进入协程块（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_Enter(phTCB)` |
| 参数   | `phTCB` 任务控制块 |
| 说明   | 协程块的开始；必须搭配 `XC_Cor_Leave` 使用。首次执行从函数头开始，恢复时从上次挂起点继续 |
| 调用等级 | 协程内（函数开头） |

##### `XC_Cor_Leave` — 离开协程块（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_Leave()` |
| 说明   | 协程块的结束；走到此处表示本层正常完成（`CorState=DONE`）。若本层被挂起（延时/等通知/让出），恢复前不会执行到此处。**宏内含编译器可达性提示**（`if(0){goto}` 常量假分支，永不执行，-O0~-O3 均被优化消除，零开销）：使任务可保持 `while(1){...}XC_Cor_Leave()` 标准写法而免除 armcc #111-D 不可达告警（详见 `XC_Cor.h` 注释与 `Docs/MISRA-C/MISRA-C_2025_Deviation.md` DEV-XCOS-001） |
| 调用等级 | 协程内（函数结尾） |

#### 嵌套调用（V2.1.0）

##### `XC_Cor_Call` — 嵌套调用子协程（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_Call(fn)` |
| 参数   | `fn` 子协程函数（`XC_CorFn_t`，签名同任务函数） |
| 说明   | **登记**子协程，不做真实 C 嵌套调用（C 栈 O(1)）：入栈保存“父层入口+返回点”，入口切换为子函数后跳出，下一轮由调度器进入子层；子层完成后调度器弹帧恢复父层，**同轮切父**继续运行父层。需要嵌套的任务必须用 `XC_Task_RegExt`/`XC_Task_SetCorStack` 配置帧栈；未配栈时调用触发越界保护（复位任务）。**注意**：`Call` 是必挂起点，调用前后局部变量不保存 |
| 调用等级 | 协程内 |

#### 协程控制

##### `XC_Cor_GetParam` — 获取任务参数（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_GetParam()` |
| 返回   | `void*` 任务注册时传递的参数 |
| 说明   | 协程块内获取任务参数（顶层与子层均可使用） |
| 调用等级 | 协程内 |

##### `XC_Cor_Yield` — 让出控制（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_Yield()` |
| 说明   | 让出 CPU 使用权，跳出协程块；任务仍留在就绪表，下次调度后从**下一行**继续执行 |
| 调用等级 | 协程内 |

##### `XC_Cor_Suspend` — 挂起自身（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_Suspend()` |
| 说明   | 将当前任务挂起；恢复需由其他任务调用 `XC_Task_Resume` |
| 调用等级 | 协程内 |

##### `XC_Cor_Reset` — 复位自身（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_Reset()` |
| 说明   | 复位当前任务：清断点、清帧栈、恢复最外层入口，立刻退出协程块；下次调度从任务函数头重新执行 |
| 调用等级 | 协程内 |

##### `XC_Cor_Remove` — 移除自身（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_Remove()` |
| 说明   | 移除当前任务：出表、清栈、`TaskNum--`、状态回 `VOID`；立刻退出协程块。注意唤醒源不会被清除，直到下次唤醒 |
| 调用等级 | 协程内 |

#### 延时

##### `XC_Cor_DelayTick` — 延时 Tick（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_DelayTick(Tick)` |
| 参数   | `Tick` 延时 Tick 数 |
| 说明   | 让出 CPU，进入时间表，延时 `Tick` 个基础时钟后自动回到就绪表；恢复后从**下一行**继续 |
| 调用等级 | 协程内 |

##### `XC_Cor_DelayUs` / `XC_Cor_DelayMs` / `XC_Cor_DelaySec` — 延时（us/ms/s）（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_DelayUs(Us)` / `#define XC_Cor_DelayMs(Ms)` / `#define XC_Cor_DelaySec(Sec)` |
| 参数   | `Us`/`Ms`/`Sec` 延时时间 |
| 说明   | 基于 `XC_Cor_DelayTick` 做单位换算；**us 延时需 Tick 时基为 us 级才精确**（`XC_CFG_TICKS_PER_SEC` 高时） |
| 调用等级 | 协程内 |

#### 等待通知

##### `XC_Cor_WaitNotify` — 等待通知（Tick，宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_WaitNotify(TickTimeout)` |
| 参数   | `TickTimeout` 超时 Tick 数；`0` 表示死等（不超时） |
| 说明   | 任务等待通知，进入阻塞表；被 `XC_Task_SendNotify` 唤醒或超时后回到就绪表。唤醒后用 `XC_Cor_IsNotifyTimeout` 判断是否超时 |
| 调用等级 | 协程内 |

##### `XC_Cor_WaitNotifyMs` — 等待通知（ms，宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_WaitNotifyMs(MsTimeout)` |
| 参数   | `MsTimeout` 超时时间（ms）；`0` 表示死等 |
| 说明   | `XC_Cor_WaitNotify` 的 ms 版本 |
| 调用等级 | 协程内 |

##### `XC_Cor_ClrNotify` — 清除通知（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_ClrNotify()` |
| 说明   | 清除通知计数；在等待通知前调用，防止通知提前到达 |
| 调用等级 | 协程内 |

##### `XC_Cor_IsNotifyTimeout` — 检查是否超时（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_IsNotifyTimeout()` |
| 返回   | `0` 未超时（被通知唤醒）；`1` 超时 |
| 说明   | 用于 `XC_Cor_WaitNotify` 唤醒后判断唤醒原因 |
| 调用等级 | 协程内（等待通知返回后） |

##### `XC_Cor_GetNotifyData` — 获取通知数据（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Cor_GetNotifyData()` |
| 返回   | `void*` 通知传递的数据 |
| 说明   | 被通知唤醒后获取通知数据 |
| 调用等级 | 协程内 |

---

### 时间处理模块（XC_Time，21 个）

#### 基础时间（宏）

##### `XC_Time_GetTickUnit` — 获取 Tick 最小时间单位（us）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_GetTickUnit()` |
| 返回   | `uint32_t` 一次 Tick 计数的时间（us） |
| 说明   | 即 `1000000 / XC_CFG_TICKS_PER_SEC` |
| 调用等级 | 任意 |

##### `XC_Time_GetTick` — 获取系统 Tick

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_GetTick()` |
| 返回   | `XC_Tick_t` 当前系统 Tick 值 |
| 说明   | 读取全局滴答计数（`XC_SYS_TICK_COUNT`） |
| 调用等级 | 任意 / **中断** |

##### `XC_Time_GetMs` — 获取系统运行时间（ms）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_GetMs()` |
| 返回   | `XC_Tick_t` 系统已运行毫秒数 |
| 说明   | 32 位下 ms 计数最长约 49 天（1ms Tick） |
| 调用等级 | 任意 / **中断** |

##### `XC_Time_TickInc` — 递增系统 Tick（宏，默认模式）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_TickInc()` |
| 说明   | 在定时器中断中按 `XC_CFG_TICKS_PER_SEC` 频率调用；仅中断累加模式（`XC_SYS_TICK_INT_INC_MODE`）有效 |
| 调用等级 | **中断**（系统 Tick 源） |

##### `XC_Time_TickSet` — 设置系统 Tick（宏，默认模式）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_TickSet(_Tick)` |
| 参数   | `_Tick` 要设置的 Tick 值 |
| 说明   | 特殊情况下使用，如休眠唤醒后重新校准系统 Tick |
| 调用等级 | 任意 |

#### 时间比较（溢出安全）

> 以下比较基于无符号减法，**允许 Tick 溢出**；限制：`延时 + 每次查询间隔` 不能超过 Tick 类型值域。

##### `XC_Time_CheckTimeout` — 比较 Tick 是否到达（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_CheckTimeout(_Lc, _c)` |
| 参数   | `_Lc` 上次记录的 Tick；`_c` 需要比较的 Tick 数 |
| 返回   | `1` 时间到达；`0` 未到达 |
| 说明   | 判断是否已运行 `_c` 个 Tick |
| 调用等级 | 任意 |

##### `XC_Time_CheckTimeoutMs` / `XC_Time_CheckTimeoutSec` — 时间是否到达（ms/s，宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_CheckTimeoutMs(_Lc, _t)` / `#define XC_Time_CheckTimeoutSec(_Lc, _t)` |
| 参数   | `_Lc` 上次记录的 Tick；`_t` 时间（ms/s） |
| 返回   | `1` 时间到达；`0` 未到达 |
| 说明   | `XC_Time_CheckTimeout` 的时间单位版本 |
| 调用等级 | 任意 |

##### `XC_Time_GetRemain` — 获取剩余 Tick（函数）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Tick_t XC_Time_GetRemain(XC_Tick_t LastTick, XC_Tick_t CompareTick)` |
| 参数   | `LastTick` 上次记录的 Tick；`CompareTick` 等待到达的 Tick 数 |
| 返回   | 距离到达还差的 Tick 数；`0` 表示已到达或早已到达 |
| 调用等级 | 任意 |

##### `XC_Time_GetElapsed` — 获取已运行 Tick（函数）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Tick_t XC_Time_GetElapsed(XC_Tick_t LastTick, XC_Tick_t CompareTick)` |
| 参数   | `LastTick` 上次记录的 Tick；`CompareTick` 等待到达的 Tick 数 |
| 返回   | 已运行的 Tick 数；等于 `CompareTick` 表示已完成 |
| 调用等级 | 任意 |

#### 软件定时器（`XC_TimerTick_t`，非协程场景）

##### `XC_Time_TimerSet` — 更新定时器 Tick（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_TimerSet(_tC, _c)` |
| 参数   | `_tC` `XC_TimerTick_t` 变量；`_c` 需要比较/延时的 Tick 数 |
| 说明   | 记录当前 Tick 并设置等待 Tick 数 |
| 调用等级 | 任意 |

##### `XC_Time_TimerSetMs` / `XC_Time_TimerSetSec` — 更新定时器时间（ms/s，宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_TimerSetMs(_tC, _t)` / `#define XC_Time_TimerSetSec(_tC, _t)` |
| 说明   | 定时器时间单位版本 |
| 调用等级 | 任意 |

##### `XC_Time_TimerRepeat` — 重复定时器（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_TimerRepeat(_tC)` |
| 说明   | 以当前 Tick 重新计时（保留等待 Tick 数），实现周期定时 |
| 调用等级 | 任意 |

##### `XC_Time_TimerCheck` — 判断定时器是否到达（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_TimerCheck(_tC)` |
| 返回   | `1` 时间到达；`0` 未到达 |
| 调用等级 | 任意 |

##### `XC_Time_TimerClr` — 清除定时器（宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_TimerClr(_tC)` |
| 说明   | `XC_TimerTick_t` 变量全部清零 |
| 调用等级 | 任意 |

##### `XC_Time_TimerGetRemain` — 定时器剩余 Tick（函数）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Tick_t XC_Time_TimerGetRemain(XC_TimerTick_t tTime)` |
| 参数   | `tTime` `XC_TimerTick_t` 变量（按值传） |
| 返回   | 剩余 Tick；`0` 表示已到达或早已到达 |
| 调用等级 | 任意 |

##### `XC_Time_TimerGetElapsed` — 定时器已运行 Tick（函数）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `XC_Tick_t XC_Time_TimerGetElapsed(XC_TimerTick_t tTime)` |
| 返回   | 已运行 Tick；等于 `WaitCount` 表示时间到达 |
| 调用等级 | 任意 |

##### `XC_Time_TimerGetElapsedMs` — 定时器已运行时间（ms，宏）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `#define XC_Time_TimerGetElapsedMs(_tC)` |
| 返回   | 已运行毫秒数 |
| 调用等级 | 任意 |

#### 死循环延时（阻塞式，非协程）

##### `XC_Time_BlockDelay` / `XC_Time_BlockDelayMs` — 死循环延时（Tick/ms，函数）

| 项     | 内容 |
| ---    | ---  |
| 原型   | `void XC_Time_BlockDelay(XC_Tick_t DelayTick)` / `void XC_Time_BlockDelayMs(XC_Tick_t Delay_ms)` |
| 参数   | `DelayTick`/`Delay_ms` 延时值 |
| 说明   | while 循环忙等待，**会阻塞整个系统**；仅在非协程场景（如初始化、临界段）使用 |
| 调用等级 | 任意（非协程内推荐用 `XC_Cor_Delay*`） |

---

## 💡 函数调用等级

按调用场景分为 4 个等级，违反调用等级将产生未定义行为：

| 等级 | 可调用 API | 说明 |
| ---  | --- | --- |
| **协程内** | 全部 `XC_Cor_*` 宏（Enter/Leave/Call/Yield/Suspend/Reset/Remove/Delay*/WaitNotify/ClrNotify/IsNotifyTimeout/GetNotifyData/GetParam） | 必须在 `XC_Cor_Enter`/`XC_Cor_Leave` 之间；`XC_Cor_Call` 需要任务配置帧栈 |
| **任意** | `XC_Time_*`（时间比较/定时器/延时）、`XC_Task_*`（注册/移除/复位/挂起/恢复/通知/参数/状态）、`XC_Sch_GetTaskNum` | 普通任务/主流程代码 |
| **中断** | `XC_Task_SendNotify`（通知）、`XC_Time_GetTick`、`XC_Time_GetMs`、`XC_Time_TickInc`（Tick 源） | **仅通知类与 Tick 读取可在中断中调用**（SPSC 线程安全）；挂起/恢复/移除/复位/注册等链表操作 API **不可**在中断中调用；多生产者/CPU 内存重排序需用户加锁/加屏障 |
| **初始化** | `XC_Sch_Init`、`XC_Sch_Start`、`XC_Sch_SetIdleCallback`、`XC_Task_Reg*`/`Add` | main 中或系统启动阶段 |

### 使用建议

1. **协程块内变量**：协程挂起时函数局部变量不保存；需要跨挂起保留的数据存 TCB/静态区/全局区；
2. **任务参数**：若要在协程块内使用注册参数，用 `XC_Cor_GetParam()`（或进入前转存到静态区）；
3. **中断通知**：`XC_Task_SendNotify` 可在中断中调用（SPSC 线程安全），是推荐的 ISR→任务通信方式；
4. **Tick 计数**：确保系统 Tick 持续运行（中断累加模式记得在定时器中断调 `XC_Time_TickInc`）；
5. **错误处理**：返回 `XC_Return_t` 的 API 建议检查返回值；协程块内宏无返回值，通过 `XC_Cor_IsNotifyTimeout`/`XC_Task_GetState` 判断状态。

---

## 🔁 向后兼容（XC_Compat.h）

V2.1.0 起函数/宏统一命名规范（`XC_<模块>_<功能>`）。旧名称仍可通过 `XC_Compat.h`（`XCOS.h` 已包含）使用，**仅供迁移期，后续版本将删除，新代码请使用新名称**：

### 协程块（旧前缀 `XC_`）

| 旧名称 | 新名称 |
| --- | --- |
| `XC_Enter` | `XC_Cor_Enter` |
| `XC_Leave` | `XC_Cor_Leave` |
| `XC_GetParam` | `XC_Cor_GetParam` |
| `XC_Yield` | `XC_Cor_Yield` |
| `XC_Suspend` | `XC_Cor_Suspend` |
| `XC_Reset` | `XC_Cor_Reset` |
| `XC_Remove` | `XC_Cor_Remove` |
| `XC_DelayTick` | `XC_Cor_DelayTick` |
| `XC_DelayUs` | `XC_Cor_DelayUs` |
| `XC_DelayMs` | `XC_Cor_DelayMs` |
| `XC_DelaySec` | `XC_Cor_DelaySec` |
| `XC_WaitForNotify` | `XC_Cor_WaitNotify` |
| `XC_WaitForNotifyMs` | `XC_Cor_WaitNotifyMs` |
| `XC_ClrNotify` | `XC_Cor_ClrNotify` |
| `XC_CheckNotifyWakeupTimeout` | `XC_Cor_IsNotifyTimeout` |
| `XC_GetNotifyData` | `XC_Cor_GetNotifyData` |

### 调度器（旧前缀 `XCSch_`）

| 旧名称 | 新名称 |
| --- | --- |
| `XCSch_Init` | `XC_Sch_Init` |
| `XCSch_Start` | `XC_Sch_Start` |
| `XCSch_GetTaskNum` | `XC_Sch_GetTaskNum` |
| `XCSch_SetIdleCallback` | `XC_Sch_SetIdleCallback` |

### 任务（旧前缀 `XCTask_`）

| 旧名称 | 新名称 |
| --- | --- |
| `XCTask_Reg` | `XC_Task_Reg` |
| `XCTask_Remove` | `XC_Task_Remove` |
| `XCTask_SetEntry` | `XC_Task_SetEntry` |
| `XCTask_Add` | `XC_Task_Add` |
| `XCTask_Reset` | `XC_Task_Reset` |
| `XCTask_Suspend` | `XC_Task_Suspend` |
| `XCTask_Resume` | `XC_Task_Resume` |
| `XCTask_SendNotify` | `XC_Task_SendNotify` |
| `XCTask_ClrNotify` | `XC_Task_ClrNotify` |
| `XCTask_UpdateNotifyData` | `XC_Task_UpdateNotifyData` |
| `XCTask_ReadNotifyData` | `XC_Task_ReadNotifyData` |
| `XCTask_GetParam` | `XC_Task_GetParam` |
| `XCTask_GetState` | `XC_Task_GetState` |

### 时间（旧前缀 `XCTime_`）

| 旧名称 | 新名称 |
| --- | --- |
| `XCTime_GetTickUnit` | `XC_Time_GetTickUnit` |
| `XCTime_GetTick` | `XC_Time_GetTick` |
| `XCTime_GetMs` | `XC_Time_GetMs` |
| `XCTime_TickInc` | `XC_Time_TickInc` |
| `XCTime_TickSet` | `XC_Time_TickSet` |
| `XCTime_CheckTimeout` | `XC_Time_CheckTimeout` |
| `XCTime_CheckTimeoutMs` | `XC_Time_CheckTimeoutMs` |
| `XCTime_CheckTimeoutSec` | `XC_Time_CheckTimeoutSec` |
| `XCTime_GetRemain` | `XC_Time_GetRemain` |
| `XCTime_GetElapsed` | `XC_Time_GetElapsed` |
| `XCTime_TimerSet` | `XC_Time_TimerSet` |
| `XCTime_TimerSetMs` | `XC_Time_TimerSetMs` |
| `XCTime_TimerSetSec` | `XC_Time_TimerSetSec` |
| `XCTime_TimerRepeat` | `XC_Time_TimerRepeat` |
| `XCTime_TimerCheck` | `XC_Time_TimerCheck` |
| `XCTime_TimerClr` | `XC_Time_TimerClr` |
| `XCTime_TimerGetRemain` | `XC_Time_TimerGetRemain` |
| `XCTime_TimerGetElapsed` | `XC_Time_TimerGetElapsed` |
| `XCTime_TimerGetElapsedMs` | `XC_Time_TimerGetElapsedMs` |
| `XCTime_BlockDelay` | `XC_Time_BlockDelay` |
| `XCTime_BlockDelayMs` | `XC_Time_BlockDelayMs` |

> **其他变更**：类型 `XCBP_t` → `XC_BP_t`；`XCListNode_t` → `XC_ListNode_t`（struct 标签统一 `XC_ListNode_tag`）；二进制宏 `_B*` → `BIN_*`；框架兼容标识 `_XCOS_` → `XCOS_API_LEVEL`。

---

<div align="center">

**📖 完整示例请参考: [示例文档](./Examples.md) • ⚙️ 配置说明请参考: [配置文档](./XC_Config.md) • 架构实现: [架构概览](./架构概览/XCOS_V2.1.0_架构概览.md)**

</div>
