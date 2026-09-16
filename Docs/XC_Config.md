# XCOS 配置说明

<div align="center">

![配置](https://img.shields.io/badge/配置-参数-orange.svg)
![版本](https://img.shields.io/badge/版本-2.1.0-green.svg)
![兼容性](https://img.shields.io/badge/兼容性-ANSI/GNU-blue.svg)

**XCOS框架配置参数详细说明**

[快速配置](#-快速配置) • [配置参数](#-配置参数) • [参数详解](#-参数详解) • [滴答计数](#-滴答计数实现) • [诊断开关](#-xc_cfg_err_hook---错误上报钩子开关)

</div>

## 🚀 快速配置

默认配置已针对大多数应用场景优化, 开箱即用;

如需自定义, 有两种方式:

### 方式1: 编译期宏定义（推荐）

在包含 `XCOS.h` 前定义相应宏:

```c
// 在包含 XCOS.h 前自定义配置
#define XC_CFG_TICKS_PER_SEC  1000    // 修改Tick频率(1kHz=1ms)
#define XC_CFG_MAX_TASKS      32      // 修改最大任务数

#include "XCOS.h"
```

### 方式2: 独立配置文件 `XCOS_Cfg.h`

定义宏 `XCOS_CFG` 后，`XC_Config.h` 会自动包含用户提供的 `XCOS_Cfg.h`：

```c
// 编译器定义: XCOS_CFG (或在包含 XCOS.h 前)
#define XCOS_CFG
#include "XCOS.h"

// XCOS_Cfg.h 内容示例:
#ifndef XC_CFG_TICKS_PER_SEC
#define XC_CFG_TICKS_PER_SEC (1000U)
#endif
```

## ⚙️ 配置参数

> 代码里这些参数统一标注为 **`[配置]`**（`Code/Inc/XC_Config.h`），以区别于 `[用户]`（用户调用的 API）与 `[内部]`（实现侧，用户不必写）。

| 参数名称              | 类型  | 默认值            | 范围      | 说明              |
| ---                   | ---   | ---               | ---       | ---               |
| `XC_CFG_TICK_TYPE`    | 宏    | `uint32_t`        | 整数类型  | 嘀嗒计数基础类型  |
| `XC_CFG_MAX_TASKS`    | 宏    | `100U`            | 1~255（上限有编译期断言） | 最大任务数量      |
| `XC_CFG_TICKS_PER_SEC`| 宏    | `1000U`           | 见下方详解 | 系统Tick频率(Hz)  |
| `XC_SYS_TICK_COUNT`   | 宏    | `g_SysTickCount`  | -         | 系统Tick计数取值  |
| `XC_CFG_ERR_HOOK`     | 宏    | `0U`（关）        | 0 / 1     | 错误上报钩子(诊断) |
| `XC_CFG_DEBUG_CHECK`  | 宏    | `0U`（关）        | 0 / 1     | 运行期不变量自检(调试) |
| `XC_CFG_TASK_STATS`   | 宏    | `0U`（关）        | 0 / 1     | 每任务运行统计(可选) |
| `XC_CFG_ASSERT`       | 宏    | `0U`（关）        | 0 / 1     | 参数/状态断言(诊断; 须同开钩子) |

## 📋 参数详解

### 🔧 XC_CFG_TICK_TYPE - 嘀嗒计数基础类型

**用途:** 定义 `XC_Tick_t` 的基础类型（`typedef XC_CFG_TICK_TYPE XC_Tick_t;`）;

**说明:**
- ✅ 用于系统滴答计数, 延时时间等所有时间相关变量
- ✅ 默认 `uint32_t`, 可支持约49天的连续计数(1ms Tick)
- ⛔ **必须 ≥ 32 位**：16 位在 1000Hz 下约 **65.5s** 回绕，会被调度器判为"时间溢出"而**把整张时间表一次性搬到就绪表**（所有延时任务被提前唤醒，语义破坏），且 `TicksToMs`/`GetMs` 等换算随之错乱；
  框架在 `XC_Config.h` 对此有**编译期断言**（`sizeof(XC_Tick_t) >= 4U`，不满足时直接编译报错，0 运行时/体积开销）
- ✅ 一般无需修改, 保持默认即可

### 🎯 XC_CFG_MAX_TASKS - 最大任务数

**用途:** 限制系统支持的最大任务数量

**说明:**
- ✅ 只限制最大能注册的任务;
- ✅ 任务只会在注册的时候占用资源, 任务数量不会影响框架占用的资源大小;
- ✅ 协作式调度中, 任务数过多会影响实时性;
- ✅ 注册超出上限时 `XC_Task_Reg`/`XC_Task_RegExt`/`XC_Task_Add` 返回 `XC_FAIL`;
- ⛔ **上限 255 是硬约束**：框架实例的任务计数 `TaskNum` 是 `uint8_t`，配置超过 255 会让计数**回绕**（注册数涨不上去）、
  `XC_Task_Remove` 的 `TaskNum--` 还会**下溢** ⇒ 诊断/统计全部失真；框架在 `XC_Config.h` 对此有**编译期断言**
  （`1 <= XC_CFG_MAX_TASKS <= 255`，不满足直接编译报错，0 运行时/体积开销）；
- 💡 **建议 ≥ 10**：协作式调度下任务太少时"轮转粒度"粗，实时性反而不稳（该下限**只建议、不强制**）；

### ⏱️ XC_CFG_TICKS_PER_SEC - 系统Tick频率

**用途:** 定义系统时间基准精度

**频率选择参考:**

| 频率值    | 周期  | 适用场景              |
| ---       | ---   | ---                   |
| 100Hz     | 10ms  | 响应要求低            |
| 1kHz      | 1ms   | **通用应用**(默认)    |
| 10kHz     | 100us | 高精度定时需求        |
| 100kHz    | 10us  | 极高精度需求          |

**说明:**
- ✅ 时基转换（ms/us/s ↔ Tick）在 `XC_TimeInternal.h` 中按频率档位自动选择公式:
  - `≤ 1000`（时基 ≥1ms）：us/ms→Tick 向上取整, 秒→Tick 直接乘频率;
  - `≤ 1000000`（时基 <1ms）：us/ms→Tick **向上取整**（ms 经 us 中间量换算, 与上一档取整语义一致）, 秒→Tick 直接乘频率;
- ⛔ **取值 ≤1000 Hz 时，必须是 1000 的因子**（框架有**编译期断言**，非因子直接编译报错）：

  | 档位 | 支持的取值 |
  | --- | --- |
  | **≤ 1000 Hz** | **1 / 2 / 4 / 5 / 8 / 10 / 20 / 25 / 40 / 50 / 100 / 125 / 200 / 250 / 500 / 1000** |
  | **> 1000 Hz**（≤ 1000000） | 任意值（走 **us** 时基） |

  失真机理：该档位 `XC_TICK_PERIOD_MS = 1000 / f` 是**整型除法**——非因子频率（如 800/600 Hz）会把"每 tick"截断成**整 ms**
  ⇒ `XC_Time_TicksToMs()`/`XC_Time_GetMs()` **系统性偏小**（800 Hz 偏小 20%、600 Hz 偏小 40%），
  而 `XC_Time_MsToTicks()` **向上取整到整 tick** ⇒ 延时**偏长**（300 Hz 下请求 1 ms 实际 3.33 ms）；
  > 与 `f` 无关的运算不受影响：`XC_Time_CheckTimeout()`/`GetRemain()`/`GetElapsed()` 与 `XC_Cor_DelayTick()` 均为 **tick 差值**运算 ✅
- ⚠️ **`XC_Time_SecToTicks(s)` 的取值上限**：它是 `s × f`（32 位 `XC_Tick_t`）⇒ 可用秒数上限 ≈ **`XC_TICK_MAX / f`**：

  | 频率 | `SecToTicks` 秒数上限 | 说明 |
  | --- | --- | --- |
  | 1000 Hz（默认） | ≈ **4 294 967 s（49.7 天）** | 与 Tick 自身 49.7 天回绕一致 |
  | 2000 Hz | ≈ 2 147 483 s（24.8 天） | |
  | 500 Hz | ≈ 8 589 934 s（99.4 天） | 但 Tick 自身 49.7 天回绕 ⇒ 实际受 Tick 限制 |
  | 10000 Hz | ≈ 429 496 s（4.97 天） | |

  > 超过上限会**溢出**（得到错误 Tick）；且回绕结果**必然小于**请求值 ⇒ 延时/超时都**偏短**
  > （例：1000 Hz 下 `XC_Time_SecToTicks(4295000)` = **32704** Tick ≈ 32.7 s，而非 49.7 天）；
  > 大延时请**分段**（多次 `DelayMs/Sec`）或改用 `XC_Cor_DelayTick()`（直接给 Tick）。
  > 另注：**跨 Tick 回绕的延时本身是支持的**（由时间溢出表处理），受限的只是「秒 × 频率」这一次乘法。
  > 本框架**不提供**饱和版宏（如需可自行用 64 位中间量包一层）。
- ⚠️ us 级延时（`XC_Cor_DelayUs`）仅在 Tick 时基为 us 级（高频率）时精确;

### 🔄 XC_SYS_TICK_COUNT - 系统Tick计数取值

**用途:** 系统时间基准的计数来源（框架通过此宏读取当前 Tick）

**支持两种实现方式（默认中断累加）:**

#### 方式1: 中断累加模式（默认）

定义 `XC_SYS_TICK_INT_INC_MODE` 并使用全局变量 `g_SysTickCount`，推荐用于大多数应用。

**实现步骤:**
1. 配置硬件定时器, 频率为 `XC_CFG_TICKS_PER_SEC`;
2. 在定时器中断中调用 `XC_Time_TickInc()`（等价于 `g_SysTickCount++`）;

#### 方式2: 直接计数器模式

自定义 `XC_SYS_TICK_COUNT` 宏指向硬件计数器，推荐用于性能要求极高的应用。

**实现步骤:**
1. 配置硬件定时器为计数器模式;
2. 在包含 `XCOS.h` 前定义:
   ```c
   // 直接映射到硬件计数器寄存器(只读)
   #define XC_SYS_TICK_COUNT (TIM2->CNT)
   ```

**性能优势:**
- ⚡ 零函数调用开销
- ⚡ 直接读取硬件计数器
- ⚡ 适合高频Tick应用

**注意事项:**
- 🔧 需要32位硬件计数器（与 `XC_Tick_t` 同宽）
- 🔧 注意寄存器访问权限(volatile)
- 🔧 确保计数器溢出后会从0开始计数（满足无符号回绕语义）

### 🩺 XC_CFG_ERR_HOOK - 错误上报钩子开关

**用途:** 让框架在"**用户可修复的错误**"发生时回调用户钩子（现场记录日志 / 点亮指示灯 / 打断点定位）

**说明:**
- ✅ 默认 `0`（关闭）：所有上报点编译为**空操作** ⇒ **0 代码 / 0 RAM / 0 开销**，且**无需**实现任何函数；
- ✅ 置 `1`（开启）：框架在错误点调用用户实现的钩子（声明与错误码见 `Code/Inc/XC_Err.h`）：

  ```c
  #define XC_CFG_ERR_HOOK (1U)     /* 必须在包含 XCOS.h 之前 */

  void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)
  {
      /* 只上报、不救场：框架原有的安全降级（如帧栈越界复位任务）照旧执行 */
  }
  ```
- ⛔ 开启后**必须提供 `XC_Err_Hook` 实现**，否则链接报未定义引用；
- 当前上报点：**帧栈越界**（`XC_ERR_FRAME_OVERFLOW`：未配帧栈就 `XC_Cor_Call`，或嵌套超过 `CorDepthMax`）—— 原行为是"静默复位任务"，打开本开关即可捕获；
- 错误码：`XC_ERR_NONE` / `XC_ERR_FRAME_OVERFLOW` / `XC_ERR_ASSERT`；**`phTCB` 可能为 `NULL`**（无任务上下文，如断言场景）⇒ 使用前判空；

### 🧪 XC_CFG_DEBUG_CHECK - 运行期不变量自检开关（调试用）

**用途:** 开启后提供 `XC_Diag_CheckInvariants(phXCOS)`：把"四张任务表 + 每个任务的状态标签"逐条盘点，检查内核依赖的不变量

**说明:**
- ✅ 默认 `0`（关闭）：自检实现**不编译** ⇒ 0 代码 / 0 RAM / 0 耗时；
- ✅ 置 `1`（开启）：返回 `XC_CHK_OK` 或**第一个失败项编号**（`XC_CHK_LINK_BROKEN` / `..._STATE_TABLE` / `..._NODE_IN_TWO_LIST` / `..._TASKNUM` / `..._NESTING` / `..._HANDLE`）；
- 检查项：状态↔所在表匹配、链表双向指针完好（防环）、同一任务不重复挂表、`TaskNum` 与四表节点总数一致、协程帧栈与深度自洽；
- ⚠️ **只读检查**：不修改任何状态、不改变内核行为，也**不会**自动调用；内部短暂持锁 ⇒ **不可在已持锁的上下文**中调用；
- 典型用法：开发/回归期在关键 API 之后调用一次（定位最强）；或在空闲回调里每隔若干轮抽查一次。
- 体积/内存：多出来的代码**全部在诊断模块**（`Code/Src/XC_Diag.c`；内核侧不变）；**RAM 不增加**（不使用静态/堆内存，仅少量栈）；具体数值见 [`Tests/baseline.md`](../Tests/baseline.md)。

### 📊 XC_CFG_TASK_STATS - 每任务运行统计开关（可选功能）

**用途:** 让调度器为每个任务累计"运行次数 / 最长一次运行耗时"，回答"**哪个任务是耗时大户**"

**说明:**
- ✅ 默认 `0`（关闭）：统计字段与统计代码都不编译 ⇒ **0 代码 / 0 RAM**；
- ✅ 置 `1`（开启）：**TCB 44 → 52 Byte（+8 Byte/任务）**（100 任务 ⇒ +800 Byte RAM）；
- 查询/清零 API：`XC_Diag_GetRunCnt` / `XC_Diag_GetMaxRunTick` / `XC_Diag_ClrRunStats`（声明见 `Code/Inc/XC_Diag.h`）；
- 计数口径（一次"调度槽"）：从任务被摘出就绪表开始，到它让出（延时/等待/挂起/让出）为止，含"同轮切父"的父层运行时间；**任务等待期间不计时**；
- 精度与边界：单位是**系统 Tick**（默认 1ms ⇒ 短于 1 tick 记 0）；测量含运行期间的**中断时间** ⇒ 是"调度槽耗时"，不等价于纯任务指令数；
- 生命周期：**注册/移除时清零**（任务在**本调度槽内移除自身**时不再累计），`XC_Task_Reset` **保留**（排障场景不丢证据）。

### ✅ XC_CFG_ASSERT - 参数/状态断言开关（诊断用）

**用途:** 开启后可用 `XC_DIAG_ASSERT(cond)` 在 API 入口/状态前提处做断言：**只上报、不改行为**

**说明:**
- ✅ 默认 `0`（关闭）：宏展开为 `((void)0)` ⇒ **0 代码 / 0 RAM**，且条件**不求值**；
- ✅ 置 `1`（开启）：条件为假 ⇒ 经错误上报钩子上报 `XC_ERR_ASSERT`（错误码/钩子见 `Code/Inc/XC_Err.h`）：

  ```c
  void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)
  {
      if(ErrCode == XC_ERR_ASSERT) { /* 打断点 ⇒ 调用栈直接指到断言处 */ }
  }
  ```
- ⛔ **依赖 `XC_CFG_ERR_HOOK`**：本开关为 1 而钩子开关为 0 时**编译报错**（断言失败将不可见）⇒ 调试档建议 `-DXC_CFG_ERR_HOOK=1 -DXC_CFG_ASSERT=1`；
- 纪律：**单一强制点** —— 同一形参 + 同一失效条件只能**二选一**（参数合法性 ⇒ 断言；运行期前提 / 契约承诺的结果 ⇒ 返回码），不同形参或不同条件可并存；**热路径不加**（如每调度槽调用的统计函数）；边界规则全文见 [`架构概览` §11.6](./架构概览/XCOS_V2.1.0_架构概览.md)；
- 用法示例：`XC_DIAG_ASSERT(phTCB != NULL);`、`XC_DIAG_ASSERT((phTCB->pCorStack == NULL) == (phTCB->CorDepthMax == 0U));`

**体积（打开开关后多出来的代码；armcc AC5 `-O1`，Cortex-M3）:**

| 配置 | Code | 说明 |
| --- | --- | --- |
| 关闭（默认） | **0 B** | 宏展开为空操作 ⇒ 连条件都不求值 |
| 开启（`ASSERT` + `ERR_HOOK`） | 多 **360 字节**（相对"只开错误上报"） | 内核内 **26 处断言**（`XC_Task.c` 19 / `XC_Sch.c` 4 / `XC_Diag.c` 3），平均 ≈ **14 字节/处** |

**当前调用点（内核内）:**

| 文件 | 调用点 | 断言内容 |
| --- | --- | --- |
| `XC_Task.c`（19 处） | `XC_Task_Reg` / `RegExt` / `SetEntry` / `Add` / `SetCorStack` / `Remove` / `Reset` / `Suspend` / `Resume` / `SendNotify` | `phTCB` / `phXCOS` / `fTask` 非空；`(pCorStack == NULL) == (CorDepthMax == 0)` |
| `XC_Sch.c`（4 处） | `XC_Sch_Init` / `Start` / `GetTaskNum` / `SetIdleCallback` | `phXCOS != NULL` |
| `XC_Diag.c`（3 处） | `XC_Diag_GetRunCnt` / `GetMaxRunTick` / `ClrRunStats` | `phTCB != NULL`（`XC_Diag_CheckInvariants` 的"句柄为空"改用返回值 `XC_CHK_HANDLE` 报告 ⇒ 不重复断言） |

> 新增调用点请同步本表与 `Tests/baseline.md` 的"断言档"体积。

---

<div align="center">

**📖 完整 API 参考: [API文档](./XCOS.md) • 示例: [示例文档](./Examples.md)**

</div>