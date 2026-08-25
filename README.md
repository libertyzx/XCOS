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
| 框架核心      | 52 Bytes      | < 700 Bytes   |
| +1个任务      | +44 Bytes     | +~230 Bytes   |
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
- [Agent开发技能](./Docs/XCOS_Agent_Skill.md) - AI Agent 使用 XCOS 2.1.0 的开发指南（编程模型、API 速查、代码模式与陷阱）
- [框架概览](./Docs/架构概览/XCOS_V2.1.0_架构概览.md) - 完整实现说明（数据结构、调度流程、内存开销、边界限制）
- [使用示例](./Docs/Examples.md) - 丰富的示例代码
- [配置说明](./Docs/XC_Config.md) - 详细的配置选项说明
- [更新日志](./Docs/CHANGELOG.md) - 版本更新记录
- [修正方案](./Docs/修正方案/) - 问题修正方案（含问题说明、根因分析、修复方案与测试）
- [缺陷记录](./Docs/缺陷记录/) - 已知缺陷记录（含触发条件、实测复现与使用注意）

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
|   |   |   |-- XC_List.h           # 链表实现头文件
|   |   |   |-- XC_TaskInternal.h   # 任务相关的内部代码
|   |   |   |-- XC_TimeInternal.h   # 时间相关的内部代码
|   |   |   |-- XC_TypeInternal.h   # 类型相关的内部代码
|   |   |-- XC_BitPatterns.h        # 二进制数值宏定义
|   |   |-- XC_Compat.h             # 向后兼容宏(旧名映射,后续版本删除)
|   |   |-- XC_Config.h             # 用于存放"XCOS"的配置参数
|   |   |-- XC_Cor.h                # 协程块宏(进入/离开/延时/通知/嵌套调用)
|   |   |-- XC_Sch.h                # 调度处理头文件
|   |   |-- XC_Task.h               # 任务处理头文件
|   |   |-- XC_Time.h               # 时间处理头文件
|   |   |-- XC_Type.h               # 框架中所有用户类型
|   |   |-- XCOS.h                  # 总头文件,包含版本信息
|   |-- Src/                         # 源码文件
|   |   |-- XC_List.c                # 内部链表实现
|   |   |-- XC_Sch.c                 # 调度处理
|   |   |-- XC_Task.c                # 任务处理
|   |   |-- XC_Time.c                # 时间处理
|-- Docs/                           # 文档
|   |-- XCOS.md                     # API参考手册(完整)
|   |-- XCOS_Agent_Skill.md        # AI Agent 开发技能(编程模型/API速查/代码模式/陷阱)
|   |-- XC_Config.md                # 配置说明
|   |-- Examples.md                 # 示例说明
|   |-- ROADMAP.md                  # 项目路线图
|   |-- CHANGELOG.md                # 更新日志
|   |-- MISRA-C/                    # MISRA C:2025 合规文档
|   |-- 架构概览/                    # 架构实现说明（Markdown 思维导图 + 完整实现）
|   |-- 修正方案/                    # 问题修正方案(问题说明/根因/修复/测试)
|   |-- 缺陷记录/                    # 已知缺陷记录(触发条件/复现/使用注意)
|-- Examples/                       # 示例
|-- .clang-format                   # 格式化配置
|-- .gitignore
|-- LICENSE
|-- README.md
```

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