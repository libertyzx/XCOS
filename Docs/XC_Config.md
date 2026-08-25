# XCOS 配置说明

<div align="center">

![配置](https://img.shields.io/badge/配置-参数-orange.svg)
![版本](https://img.shields.io/badge/版本-2.1.0-green.svg)
![兼容性](https://img.shields.io/badge/兼容性-ANSI/GNU-blue.svg)

**XCOS框架配置参数详细说明**

[快速配置](#-快速配置) • [配置参数](#-配置参数) • [参数详解](#-参数详解) • [滴答计数](#-滴答计数实现)

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

| 参数名称              | 类型  | 默认值            | 范围      | 说明              |
| ---                   | ---   | ---               | ---       | ---               |
| `XC_CFG_TICK_TYPE`    | 宏    | `uint32_t`        | 整数类型  | 嘀嗒计数基础类型  |
| `XC_CFG_MAX_TASKS`    | 宏    | `100U`            | 10~255    | 最大任务数量      |
| `XC_CFG_TICKS_PER_SEC`| 宏    | `1000U`           | 1~1000000 | 系统Tick频率(Hz)  |
| `XC_SYS_TICK_COUNT`   | 宏    | `g_SysTickCount`  | -         | 系统Tick计数取值  |

## 📋 参数详解

### 🔧 XC_CFG_TICK_TYPE - 嘀嗒计数基础类型

**用途:** 定义 `XC_Tick_t` 的基础类型（`typedef XC_CFG_TICK_TYPE XC_Tick_t;`）;

**说明:**
- ✅ 用于系统滴答计数, 延时时间等所有时间相关变量
- ✅ 默认 `uint32_t`, 可支持约49天的连续计数(1ms Tick)
- ✅ 一般无需修改, 保持默认即可

### 🎯 XC_CFG_MAX_TASKS - 最大任务数

**用途:** 限制系统支持的最大任务数量

**说明:**
- ✅ 只限制最大能注册的任务;
- ✅ 任务只会在注册的时候占用资源, 任务数量不会影响框架占用的资源大小;
- ✅ 协作式调度中, 任务数过多会影响实时性;
- ✅ 注册超出上限时 `XC_Task_Reg`/`XC_Task_RegExt`/`XC_Task_Add` 返回 `XC_FAIL`;

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
  - `≤ 1000000`（时基 <1ms）：us→Tick 向上取整, ms/s→Tick 直接乘频率;
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

---

<div align="center">

**📖 完整 API 参考: [API文档](./XCOS.md) • 示例: [示例文档](./Examples.md)**

</div>