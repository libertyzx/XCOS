# XCubicOS (XCOS)

<div align="center">

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Version](https://img.shields.io/badge/version-2.1.0-green.svg)
![C Language](https://img.shields.io/badge/language-C-orange.svg)
![Platform](https://img.shields.io/badge/platform-Embedded-lightgrey.svg)

**专为资源受限嵌入式环境设计的轻量级协程调度框架**

[特性](#-特性) • [快速开始](#-快速开始) • [文档](#-文档) • [文件结构](#-文件结构)

</div>

## ✨ 特性

### 🎯 极简设计
- **纯C语言实现** - 兼容ANSI-C和GNU-C标准
- **零依赖** - 可运行于裸机系统或任何支持C语言的RTOS环境
- **跨平台** - 无需特定硬件支持, 移植简单

### 💾 超低资源占用

| 场景          | RAM 占用      | ROM 占用      |
| ---           | ---           | ---           |
| 框架核心      | 48 Bytes（`XCOS_t` 44 + `g_SysTickCount` 4） | ≈ 0.9 KB（+876 B，见 [`Docs/Examples.md`](./Docs/Examples.md)） |
| +1个任务      | +44 Bytes（TCB）  | +~260 Bytes（12 任务平均，见 [`Docs/Examples.md`](./Docs/Examples.md)） |
| 无任务堆栈    | 共享系统堆栈  | 无额外开销    |
| +协程嵌套     | 8 Bytes/层     | 极少量指令     |

### ⚡ 高效协作式调度
- **纯协作式调度** - 无优先级抢占, 简化设计
- **零中断依赖** - 无需调度器中断, 无中断开关操作
- **快速上下文切换** - 无堆栈切换, 极速任务调度

### 🛠️ 功能丰富
- ✅ 阻塞式精确延时
- ✅ 轻量级任务通知机制
- ✅ 任务挂起与恢复管理
- ✅ 软件定时器支持
- ✅ 空闲任务回调
- ✅ **协程嵌套** - 子协程调用/返回, 帧栈实现, C 栈开销恒定 O(1)

## 📋 设计约束

### ⏱️ 实时性说明
- **非实时系统** - 任务响应时间依赖开发者代码风格
- **异步操作** - 中断中的任务操作为异步响应
- **执行建议** - 建议任务函数执行时间控制在毫秒级别

### 🔧 使用规范
- **变量管理** - 任务局部变量在协程让出后不保存, 需使用全局/静态变量
- **API调用** - 阻塞API必须在协程块内调用
- **编译建议** - 推荐GNU编译器以获得最佳性能

## 🚀 快速开始

### 1. 工程集成

将项目添加到您的工程中:

1. 将 `Code` 目录下所有 `.c` 文件加入工程
2. 将 `Code/Inc` 添加到工程包含路径
3. 使用时只需包含主头文件:

```c
#include "XCOS.h"
```

### 2. 基础使用示例

```c
#include "XCOS.h"

// 框架实例和任务控制块
XCOS_t s_hXCOS0 = {0};
XC_TaskCB_t s_hTCB0 = {0};

/**
 * @brief   用户任务函数
 * @param   phTCB 任务句柄
 */
void Task(XC_TaskHandle_t phTCB)
{
    /***/
    XC_Cor_Enter(phTCB);    // 进入协程块
    /***/
    while(1) {
        XC_Cor_DelayMs(20); // 阻塞延时20ms
    }
    /***/
    XC_Cor_Leave();         // 离开协程块
    /***/
}

/**
 * @brief   空闲处理回调
 * @param   phXCOS   框架句柄
 * @param   IdleTick 空闲Tick计数
 */
void IdleCallback(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)
{
    __WFI(); // 进入低功耗模式
}

/**
 * @brief   主函数
 * @return  不返回
 */
int main(void)
{
    // 硬件初始化(配置1ms定时器)
    // XCOS框架初始化
    XC_Sch_Init(&s_hXCOS0);
    XC_Sch_SetIdleCallback(&s_hXCOS0, IdleCallback);
    XC_Task_Reg(&s_hXCOS0, &s_hTCB0, Task, NULL);
    XC_Sch_Start(&s_hXCOS0);
    return(0);
}

/**
 * @brief   定时器中断服务函数
 */
void TimerISR(void)
{
    XC_Time_TickInc();  // 系统Tick递增
}
```

### 3. 配置说明

默认配置已优化, 开箱即用; 如需自定义配置, 请参考 [配置文档](./Docs/XC_Config.md) ;

## 📚 文档

- [API文档](./Docs/XCOS.md) - 完整的数据类型与函数说明
- [框架概览](./Docs/架构概览/XCOS_V2.1.0_架构概览.md) - 完整实现说明（数据结构、调度流程、内存开销、边界限制）
- [使用示例](./Docs/Examples.md) - 丰富的示例代码
- [配置说明](./Docs/XC_Config.md) - 详细的配置选项说明
- [更新日志](./Docs/CHANGELOG.md) - 版本更新记录
- [自测工具（`Tests/`）](./Tests/README.md) - **改完代码跑一条命令**：功能回归（20 条用例）/ 固件体积 / 关键路径耗时 —— `powershell -File Tests/run_all.ps1`；
  用法与注意事项见 [`Tests/README.md`](./Tests/README.md)，基线数值见 [`Tests/baseline.md`](./Tests/baseline.md)
- [Skill（AI Agent 技能）](./Docs/Skill/) - 面向**编码代理**的技能文档（3 份：**用**框架 / **仿真**实测 / **开发**内核）：
  - [`XCOS_Agent_Skill.md`](./Docs/Skill/XCOS_Agent_Skill.md) —— **用**框架（编程模型、API 速查、标准代码模式、约定与陷阱、文件索引、已知限制）；
  - [`MDK_Sim_Agent_Skill.md`](./Docs/Skill/MDK_Sim_Agent_Skill.md) —— 无 gcc/硬件、只有 Keil MDK 时用**无界面仿真**实测周期/体积/RAM；
  - [`XCOS_Dev_Agent_Skill.md`](./Docs/Skill/XCOS_Dev_Agent_Skill.md) —— **开发/维护内核**（文件职责、命名与风格、断言边界规则、并发契约、开关纪律、**七步验收闭环**、文档同步矩阵、文档自检、登记流程、实战陷阱）。
  约定：技能文档内路径一律用**仓库相对路径**；**不引用**内部问题跟踪记录（只引用 `Docs/XCOS.md`、`Docs/Skill/`、`Docs/架构概览/` 等公开文档）。
- 问题跟踪（**内部记录，不对外引用**）- 清单（按类别汇总）/ 修正方案（含测试与实测数据）/ 性能专题

## 🔧 文件结构

```dir
XCOS
|-- .vscode/
|-- Code/                           # 源码文件
|   |-- Inc/                        # 头文件
|   |   |-- Internal/               # 内部头文件
|   |   |   |-- XC_CorANSI.h        # 协程底层实现("ANSI-C"是由"switch case"实现)
|   |   |   |-- XC_CorGNU.h         # 协程底层实现("GNU-C" 是由"goto label" 实现)
|   |   |   |-- XC_Core.h           # 内核基础(框架实例锁等)
|   |   |   |-- XC_DiagInternal.h   # 诊断相关的内部代码(断言宏/调度槽计时埋点)
|   |   |   |-- XC_List.h           # 链表实现头文件
|   |   |   |-- XC_TaskInternal.h   # 任务相关的内部代码
|   |   |   |-- XC_TimeInternal.h   # 时间相关的内部代码
|   |   |   |-- XC_TypeInternal.h   # 类型相关的内部代码
|   |   |-- XC_BitPatterns.h        # 二进制数值宏定义
|   |   |-- XC_Compat.h             # 向后兼容宏(旧名映射,后续版本删除)
|   |   |-- XC_Config.h             # 用于存放"XCOS"的配置参数
|   |   |-- XC_Cor.h                # 协程块宏(进入/离开/延时/通知/嵌套调用)
|   |   |-- XC_Diag.h                # 诊断能力(运行期自检/运行统计查询; 开关 XC_CFG_*; 含 XC_Err.h 与 Internal/XC_DiagInternal.h)
|   |   |-- XC_Err.h                 # 错误上报(错误码/用户钩子/上报宏; 开关 XC_CFG_ERR_HOOK; 无实现文件)
|   |   |-- XC_Sch.h                # 调度处理头文件
|   |   |-- XC_Task.h               # 任务处理头文件
|   |   |-- XC_Time.h               # 时间处理头文件
|   |   |-- XC_Type.h               # 框架中所有用户类型
|   |   |-- XCOS.h                  # 总头文件,包含版本信息
|   |-- Src/                         # 源码文件
|   |   |-- XC_Diag.c                # 诊断实现(错误上报钩子无实现/自检/统计; 可物理裁剪)
|   |   |-- XC_List.c                # 内部链表实现
|   |   |-- XC_Sch.c                 # 调度处理
|   |   |-- XC_Task.c                # 任务处理
|   |   |-- XC_Time.c                # 时间处理
|-- Docs/                           # 文档
|   |-- XCOS.md                     # API参考手册(完整)
|   |-- XC_Config.md                # 配置说明
|   |-- Examples.md                 # 示例说明
|   |-- ROADMAP.md                  # 项目路线图
|   |-- CHANGELOG.md                # 更新日志
|   |-- MISRA-C/                    # MISRA C:2025 合规文档
|   |-- 架构概览/                    # 架构实现说明（API 总览表 + 调度主循环 ASCII 图 + 完整实现；无图片、无 XMind）
|   |-- 图/                          # 文档插图
|   |-- Skill/                      # AI Agent 技能文档
|   |   |-- XCOS_Agent_Skill.md     # 用户侧技能(编程模型/API速查/代码模式/用户陷阱)
|   |   |-- XCOS_Dev_Agent_Skill.md  # 内核开发技能(改代码/七步验收闭环/文档同步/文档自检/陷阱)
|   |   |-- MDK_Sim_Agent_Skill.md  # MDK µVision 无界面仿真实测技能
|   |-- 问题跟踪/                     # 发现问题 -> 定位缺陷 -> 修正方案 -> 落地验证
|   |   |-- 问题清单/                  # 问题清单: 按类别汇总(12 项 -> 3 个文件) + 索引
|   |   |   |-- README.md             #   索引(分类表 + 12 条状态一览 + 数据出处)
|   |   |   |-- 20260914_内核缺陷-通知与调度语义.md   #   内核缺陷(唤醒判据/入表语义)
|   |   |   |-- 20260914_内核缺陷-任务·时间·配置.md   #   内核缺陷(任务/时间/配置)
|   |   |   |-- 20260914_诊断能力与工程化.md         #   诊断能力(钩子/自检/统计)与回归基线
|   |   |-- 修正方案/                  # 修正方案(修正方案.md + 测试/附件; 含"未采用"方案)
|-- Examples/                       # 示例
|-- Tests/                          # 自测工具(回归/体积/周期基线; 用法见 Tests/README.md): powershell -File Tests/run_all.ps1
|-- .clang-format                   # 格式化配置
|-- .gitignore
|-- LICENSE
|-- README.md
```

> **改完代码怎么自测**：跑一条命令 `powershell -File Tests/run_all.ps1`，它会做三件事 ——
> ① **功能**：20 条用例（通知 / 挂起 / 调度 / 诊断开关等）是否全过；② **体积**：编译出来的固件大小有没有意外变化；
> ③ **速度**：关键路径耗时有没有变慢。用法见 [`Tests/README.md`](./Tests/README.md)，数值见 [`Tests/baseline.md`](./Tests/baseline.md)。

## 🔬 技术实现

协程实现原理详见技术博客:

📖 [C语言协程实现详解](https://blog.csdn.net/libertyzx/article/details/126186870)

## 🎯 适用场景
- ✅ 8/16/32位微控制器
- ✅ 内存受限的嵌入式设备
- ✅ 对功耗敏感的低功耗应用
- ✅ 需要简单任务管理的裸机系统
- ✅ 作为现有RTOS的轻量级补充

## 📄 许可证
本项目采用 MIT 许可证 - 详见 [LICENSE](./LICENSE) 文件;

---

<div align="center">

如果您觉得这个项目有帮助, 请给它一个 ⭐️

</div>
