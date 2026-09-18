/**
 * @file        XC_DiagInternal.h
 * @brief       诊断能力-内部声明(断言宏 / 调度槽计时埋点)
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.1
 * @date        2026/09/15
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  本头只放"**实现侧 / 内核内部使用**"的诊断声明(用户侧声明见 `XC_Diag.h`):
 *  - `XC_DIAG_ASSERT`: 内核 API 入口 / 状态前提的参数断言(开关 `XC_CFG_ASSERT`, 须同时开 `XC_CFG_ERR_HOOK`);
 *  - `XC_Diag_RunStatsBegin` / `XC_Diag_RunStatsEnd`: **调度槽计时**两端, 只由调度器主循环调用(开关 `XC_CFG_TASK_STATS`);
 *  ---
 *  用户**不要**直接使用本头内容; `XC_Diag.h` 会自动包含本头(用户只需包含 `XCOS.h`/`XC_Diag.h`);
 *  开关关闭时上面两者都不编译 ⇒ **0 代码 / 0 RAM**; 回归/体积基线见 `Tests/baseline.md`;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_DiagInternal_h
#define XC_DiagInternal_h
//=== 头文件
#include "XC_Config.h"
#include "XC_Err.h" //错误上报设施(断言经其 XC_Err_Report 上报 XC_ERR_ASSERT)
#include "XC_Type.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 1) 参数/状态断言(开关: XC_CFG_ASSERT; 依赖 XC_CFG_ERR_HOOK) */

/**
 * @brief   [内部]断言(仅检查并上报, **不改变控制流**)
 * @param   cond    布尔条件(关档时**不求值**)
 * @details
 *  - 打开 `XC_CFG_ASSERT`(须同时打开 `XC_CFG_ERR_HOOK`)时: 条件为假 ⇒ 经钩子上报
 *    `XC_ERR_ASSERT`(在钩子内对 `XC_ERR_ASSERT` 打断点, 由调用栈回溯即可定位调用点);
 *  - 关闭(默认)时展开为 `((void)0)` ⇒ **0 代码 / 0 RAM**, 且条件表达式**不求值**;
 *  - 纪律: 断言**不替代**返回值校验 —— 内核现有 `return XC_FAIL` 等检查一律保留;
 *    断言语义是"本不该发生" ⇒ 只上报, 不 return、不止损、不改变行为;
 *  - 建议用法: `XC_DIAG_ASSERT(phTCB != NULL);` 放在 API 入口或状态前提处;
 *  - 边界规则(断言 vs 返回码)见 `Docs/架构概览` §11.6;
 */
#if (XC_CFG_ASSERT != 0)
#if (XC_CFG_ERR_HOOK == 0)
#error "XC_CFG_ASSERT 需要同时打开 XC_CFG_ERR_HOOK(断言失败经钩子上报, 否则不可见)"
#endif
#define XC_DIAG_ASSERT(cond)                    \
    do {                                        \
        if(!(cond)) {                           \
            XC_Err_Report(NULL, XC_ERR_ASSERT); \
        }                                       \
    } while(0)
#else
#define XC_DIAG_ASSERT(cond) ((void)0)
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 2) 每任务运行统计-埋点两端(开关: XC_CFG_TASK_STATS; 查询 API 见 `XC_Diag.h`) */

#if (XC_CFG_TASK_STATS != 0)

/**
 * @brief       [内部]调度槽计时起点(由调度器主循环调用)
 * @return      XC_Tick_t  当前系统 Tick(交给 `XC_Diag_RunStatsEnd` 计算差值)
 * @details
 *  起始时刻由调用方以**局部变量**保存(不占任务/框架 RAM); 差值为**无符号减法**(Tick 回绕安全);
 *  > 与 `XC_Diag_RunStatsEnd` 成对使用; 也供统计功能的测试用例直接驱动(免跑死循环);
 */
XC_Tick_t XC_Diag_RunStatsBegin(void);

/**
 * @brief       [内部]调度槽计时终点: 累计"运行次数 / 最长一次耗时"
 * @param[in]   phTCB       本轮运行的任务
 * @param[in]   RunStart    本轮起始 Tick(`XC_Diag_RunStatsBegin` 的返回值)
 * @details
 *  - `RunCnt++`(饱和, 不回绕); `MaxRunTick` 只增不减;
 *  - 写者只有主循环 ⇒ 无需加锁(字段并发等级为"主上下文");
 */
void XC_Diag_RunStatsEnd(XC_TaskHandle_t phTCB, XC_Tick_t RunStart);

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
