/**
 * @file        XC_Diag.h
 * @brief       诊断能力: 断言 / 运行期自检 / 运行统计(错误上报设施见 `XC_Err.h`)
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/09/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  本模块把框架的"**诊断能力**"集中在一处(与内核功能解耦);
 *  「**错误上报设施**」(错误码 / 用户钩子 / 上报宏)在独立头 `XC_Err.h` 里 —— 本头包含它(断言失败即经其上
 *  报 `XC_ERR_ASSERT`), 但**只做错误上报**的用户/模块无需包含本头;
 *
 *  | 子功能 | 开关(见 `XC_Config.h`) | 关闭时 | 打开时成本 | 头/实现 |
 *  | --- | --- | --- | --- | --- |
 *  | 错误上报钩子 | `XC_CFG_ERR_HOOK` | 0 | `XC_Task.o` +12 B(对象级) | `XC_Err.h`(无实现) |
 *  | 参数/状态断言 | `XC_CFG_ASSERT`(须同时开 `XC_CFG_ERR_HOOK`) | 0 | 每处 1 次判空 + 调用 | `Internal/XC_DiagInternal.h`(宏) |
 *  | 运行期不变量自检 | `XC_CFG_DEBUG_CHECK` | 0 | `XC_Diag.o` 约 +0.5 KB | 本头 + `XC_Diag.c` |
 *  | 每任务运行统计 | `XC_CFG_TASK_STATS` | 0 | **TCB +8 B/任务** | 本头 + `Internal/XC_DiagInternal.h`(埋点) + `XC_Diag.c` |
 *  ---
 *  **三条纪律**:
 *  1. **只上报/只读检查**: 不改变内核控制流(帧栈越界仍复位、断言不 return);
 *  2. **全部默认关闭**: 关档时字段与代码都不编译 ⇒ 0 代码 / 0 RAM / 0 耗时;
 *  3. **可物理裁剪**: 发布工程**不添加** `Code/Src/XC_Diag.c` 时, 诊断实现完全不进固件
 *     (前提: 所有诊断开关仍为 0; 开关为 1 却未加入该文件 ⇒ **链接报错**, 属预期);
 *  ---
 *  **埋点(调用点)仍在内核里, 不在本模块**:
 *  - 帧栈越界上报: `Code/Src/XC_Task.c` §`XC_Task_PushFrame`;
 *  - 调度槽计时两端: `Code/Src/XC_Sch.c` §`XC_Sch_Start`(运行块);
 *  - 注册/移除时清零统计: `Code/Src/XC_Task.c` §`XC_Task_AddInternal` / §`XC_Task_HandleRemove`;
 *  ---
 *  用法/口径详见 `Docs/XC_Config.md` 与 `Docs/XCOS.md`(「错误上报」「运行期自检」「运行统计」节);
 *  **回归/体积基线**见 `Tests/baseline.md`(每次改动跑 `powershell -File Tests/run_all.ps1`);
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_Diag_h
#define XC_Diag_h
//=== 头文件
#include "Internal/XC_DiagInternal.h"
#include "XC_Config.h"
#include "XC_Err.h"
#include "XC_Type.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 1) 运行期不变量自检(开关: XC_CFG_DEBUG_CHECK) */

/**
 * @brief   [调试]自检结果(返回值)
 * @details  自检遇到**第一个**不一致即返回对应编号; 全部一致返回 `XC_CHK_OK`;
 */
typedef enum {
    XC_CHK_OK               = 0, // 一致(无异常)
    XC_CHK_LINK_BROKEN      = 1, // 链表结构损坏(前后指针不互指, 或成环: 遍历步数超过任务数上限)
    XC_CHK_STATE_TABLE      = 2, // 任务状态与所在表不匹配(该游离的却在表里 / 表与状态不符)
    XC_CHK_NODE_IN_TWO_LIST = 3, // 同一任务被同时挂在两张表
    XC_CHK_TASKNUM          = 4, // "TaskNum" 与四张表的节点总数不一致(或超出最大任务数)
    XC_CHK_NESTING          = 5, // 协程帧栈/深度自洽性被破坏(有栈无容量、深度越界等)
    XC_CHK_HANDLE           = 6, // 表内任务的 "phXCOS" 不是本次检查的框架实例
} XC_ChkResult_t;

#if (XC_CFG_DEBUG_CHECK != 0)

/**
 * @brief       [调试]运行期不变量自检(仅 `XC_CFG_DEBUG_CHECK` 打开时存在)
 * @param[in]   phXCOS  框架句柄
 * @return      XC_ChkResult_t
 * @retval      XC_CHK_OK : 全部一致
 * @retval      其他值    : 第一个失败项编号(类型见 `XC_ChkResult_t`)
 * @details
 *  **用途(调试)**: 把"四张任务表 + 每个任务的状态标签"盘点一遍, 检查内核依赖的不变量;
 *  内核多处依赖这些不变量(历史上两个缺陷——僵尸状态、误唤醒——都属于不变量被破坏),
 *  但**运行期默认不做任何检查** ⇒ 本函数用于在开发/回归期把它们**在更早的时刻**暴露出来;
 *  ---
 *  **检查项**:
 *  1. 就绪表内任务必须为 "READY"; 时间表/溢出表/阻塞表内必须为 "BLOCKED" 或 "SUSPEND",
 *     且 "SUSPEND" 只允许出现在阻塞表(挂起优先级最高的约定); "RUN"/"VOID" 不得出现在任何表;
 *  2. 链表双向指针完好("节点->后继->前驱 == 节点"等), 且遍历步数不超过 "XC_CFG_MAX_TASKS"(防环死循环);
 *  3. 同一任务不得同时挂在两张表(两两比对, 调试期允许 O(n^2));
 *  4. "phXCOS->TaskNum" 必须等于四张表的节点总数, 且不超过 "XC_CFG_MAX_TASKS";
 *  5. 协程嵌套自洽: "pCorStack" 与 "CorDepthMax" 必须同为"有"或同为"无", 且 "CorDepth <= CorDepthMax";
 *  6. 表内任务的 "phXCOS" 必须就是本次传入的框架实例;
 *  ---
 *  **注意**:
 *  - 本函数**只读检查**, 不修改任何状态, 也**不会**自动调用(由用户/回归用例显式调用);
 *  - 内部会**短暂持锁**(XC_Core_Lock/Unlock)完成遍历 ⇒ **不可在已持锁的上下文**中调用;
 *  - 关闭开关时本函数**不编译**(0 代码/0 RAM), 调用点也应随之用 "#if" 包住;
 *  - 失败时如何上报由用户决定(记录日志 / 点亮指示灯 / 调用 `XC_Err_Hook`);
 */
XC_ChkResult_t XC_Diag_CheckInvariants(XC_OSHandle_t phXCOS);

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 2) 每任务运行统计(开关: XC_CFG_TASK_STATS) */

#if (XC_CFG_TASK_STATS != 0)

/**
 * @brief       [用户]获取任务运行次数
 * @param[in]   phTCB   [XC_TaskHandle_t]任务控制块
 * @return      uint32_t  该任务被"调度槽"运行的累计次数(**饱和计数**, 不回绕)
 * @details
 *  计数口径: 每次任务被调度器取出并运行(到让出为止)记 1 次(含"同轮切父"的父层运行);
 *  清零: 注册/移除时清零(本槽内移除自身的任务不再累计), `XC_Task_Reset` 保留(见 `Docs/XC_Config.md`);
 */
uint32_t XC_Diag_GetRunCnt(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]获取任务"最长一次运行耗时"
 * @param[in]   phTCB   [XC_TaskHandle_t]任务控制块
 * @return      XC_Tick_t  最长一次调度槽耗时(单位: Tick)
 * @details
 *  只增不减; 单位是**系统 Tick**(默认 1ms ⇒ 短于 1ms 的运行记 0);
 *  > 该值是"调度槽耗时"(含运行期间的中断时间), 不是纯任务指令数(要指令数请看 MDK 仿真测量);
 */
XC_Tick_t XC_Diag_GetMaxRunTick(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]清零任务运行统计
 * @param[in]   phTCB   [XC_TaskHandle_t]任务控制块
 * @details     把 `RunCnt` 与 `MaxRunTick` 都置 0; 便于测量"某一段时间内"的任务耗时;
 */
void XC_Diag_ClrRunStats(XC_TaskHandle_t phTCB);

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
