# XCOS 示例说明

<div align="center">

![示例](https://img.shields.io/badge/示例-完整测试-blue.svg)
![版本](https://img.shields.io/badge/版本-2.1.0-green.svg)
![环境](https://img.shields.io/badge/环境-MDK_V5.39-orange.svg)
![芯片](https://img.shields.io/badge/芯片-STM32F103ZE-green.svg)

**XCOS框架完整功能演示和性能测试**

[测试环境](#-测试环境) • [资源占用](#-资源占用分析) • [示例配置](#-示例配置说明) • [功能演示](#-功能演示)

</div>

## 🎯 测试环境

### 开发环境配置

| 项目          | 配置                      |
| ---           | ---                       |
| **IDE**       | Keil MDK                  |
| **版本**      | V5.39.0.0                 |
| **工具链**    | ARM Compiler V5.06        |
| **目标芯片**  | STM32F103ZE (软件模拟)    |
| **优化等级**  | -O0                       |

### MDK配置截图

![芯片选择配置](./图/MDK-芯片配置.jpg)
**芯片选择配置**

![编译器配置](./图/MDK-C配置.jpg)
**编译器配置**

![调试器配置](./图/MDK-调试配置.jpg)
**调试器配置**

## 📊 资源占用分析

> **测量条件**: 与当前 MDK 工程一致 —— ARM Compiler **V5.06 update 7**，优化 **-O1（优化大小）**，C99，STM32F103ZE；基准（配置0）已包含 HAL 库启用模块的全部代码。

### 内存占用对比表 (MDK -O1)

| 配置      | Code  | RO-data | RW-data | ZI-data | Flash增量     | RAM增量       |
| ---       | ---   | ---     | ---     | ---     | ---           | ---           |
| **基准**  | 4700  | 380     | 24      | 1168    | -             | -             |
| **配置1** | 5576  | 380     | 28      | 1212    | +876 Bytes    | +48 Bytes     |
| **配置2** | 8720  | 380     | 40      | 1944    | +4020 Bytes   | +792 Bytes    |

- 注:增量为和基准比较的值;Code 为含 HAL 库在内的工程代码量,增量即 XCOS 框架与测试任务的额外开销;

### 资源占用总结

基于上述测试数据（V2.1.0，当前 MDK 工程全量实测，AC5 V5.06 -O1），XCOS框架的资源占用特点如下:

- **框架核心**: 约 **0.9 KB Flash**（+876B）+ **48 Bytes RAM**（`XCOS_t` 与 `g_SysTickCount` 实测，与结构体定义吻合）
- **任务区**（12 任务 A0~A11，含协程嵌套）: 约 **3.1 KB Flash**（+3144B）+ **744 Bytes RAM**
  - 单个任务平均: 约 **262 Bytes Flash** + **62 Bytes RAM**（含 TCB 44B）
  - 协程嵌套任务: 每层额外 **8 Bytes 帧栈**（`XC_CorFrame_t`）

**资源效率**: 12 个测试任务（含中断通知、协程嵌套、分步注册）增加约 3.1KB Flash + 744B RAM，适合资源受限的嵌入式环境。相比 V2.0.0（TCB 36B），V2.1.0 每个任务增加 8B（协程嵌套字段）。

## ⚙️ 示例配置说明

### 配置0 - 基准测试
**用途**: 建立无框架代码的内存占用基准
- 无任何XCOS框架函数和变量(注:文件内有4Byte的全局变量)
- 仅包含基础硬件初始化和HAL库

### 配置1 - 框架核心
**用途**: 测试框架核心开销
- 1个框架句柄 (`XC_OSHandle_t`)
- 嘀嗒中断调用 `XC_Time_TickInc()`
- 框架初始化和启动

### 配置2 - 完整功能测试 (V2.1.0)
**用途**: 全面测试框架所有功能（含 V2.1.0 协程嵌套）
- 12个不同功能的任务（A0~A11）
- 涵盖所有API和中断操作
- 包含空闲回调处理
- 包含协程嵌套（`XC_Task_RegExt`/`XC_Cor_Call`）与分步注册（`XC_Task_SetEntry`/`XC_Task_Add`）测试

## 🛠️ 功能演示

**请查看代码文件**

💡 完整示例代码请查看工程目录 `Examples/CortexM3_Test/`

### 任务清单 (V2.1.0)

| 任务 | 测试内容 | 关键API |
| --- | --- | --- |
| `Task_A0` | 基础延时 | `XC_Cor_Enter`/`XC_Cor_DelayMs`/`XC_Cor_Leave` |
| `Task_A1`/`Task_A2` | 任务通知（提前通知 + 正常通知 + 超时） | `XC_Task_SendNotify`/`XC_Cor_WaitNotifyMs`/`XC_Cor_IsNotifyTimeout`/`XC_Cor_GetNotifyData` |
| `Task_A3`/`Task_A4` | 任务挂起与恢复 | `XC_Task_Suspend`/`XC_Task_Resume` |
| `Task_A5` | 任务复位 | `XC_Cor_Reset` |
| `Task_A6` | 任务移除 | `XC_Cor_Remove` |
| `Task_A7` | 中断通知唤醒（TIM2 随机周期） | `XC_Task_SendNotify`（中断中） |
| `Task_A8` | 中断通知唤醒（TIM3 4参数轮换） | `XC_Task_SendNotify`（中断中，携带数据） |
| `Task_A9` | 外部中断（EXTI0~3）手动通知唤醒 | `XC_Task_SendNotify`（中断中） |
| `Task_A10` | **协程嵌套**：顶层任务 `XC_Cor_Call` 子协程，子层延时/等通知后同轮切父返回 | `XC_Task_RegExt`/`XC_Cor_Call`/`XC_Cor_DelayMs` |
| `Task_A11` | **分步注册 + 帧栈设置** | `XC_Task_SetEntry`/`XC_Task_SetCorStack`/`XC_Task_Add` |

### 演示功能完整性
- ✅ 协程基本操作(进入/离开/让出)
- ✅ 精确时间管理(延时/超时)
- ✅ 任务间通信(通知机制)
- ✅ 任务状态管理(挂起/恢复/复位)
- ✅ 动态任务管理(注册/移除)
- ✅ 中断集成(外部/定时器)
- ✅ 低功耗支持(空闲回调)
- ✅ **协程嵌套**(子协程调用/返回/同轮切父)
- ✅ **协程帧栈配置**(RegExt/SetCorStack)

---

<div align="center">

**📖 完整 API 参考: [API文档](./XCOS.md) • 配置: [配置文档](./XC_Config.md)**

</div>
