/**
 * @file        XC_Compat.h
 * @brief       向后兼容宏
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  集中存放各模块的旧版函数/宏名(向后兼容),仅供旧代码迁移期使用;
 *  后续版本将直接删除,不建议新代码使用;
 *  各旧名按来源文件分组,便于迁移时对照;
 * **********************************************
 * @deprecated 本文件整体为"迁移期过渡层": **计划在 V2.2.0 删除**;
 *  - 新代码请使用新名(命名规范 "XC_<模块>_<功能>");
 *  - 迁移对照表见 "Docs/XCOS.md" 的「🔁 向后兼容（XC_Compat.h）」一节;
 *  - 如需"尽早暴露旧名使用", 建议在应用侧自检(如对旧名做 "#ifdef ... #error ..." 或源码检索);
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_Compat_h
#define XC_Compat_h
//=== 头文件
#include "XC_Cor.h"
#include "XC_Sch.h"
#include "XC_Task.h"
#include "XC_Time.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 旧版宏名:来自"XC_Cor.h"(旧前缀"XC_", 2.1.0版本迁移到"XC_Cor_") */

#define XC_Enter                    XC_Cor_Enter
#define XC_Leave                    XC_Cor_Leave
#define XC_GetParam                 XC_Cor_GetParam
#define XC_Yield                    XC_Cor_Yield
#define XC_Suspend                  XC_Cor_Suspend
#define XC_Reset                    XC_Cor_Reset
#define XC_Remove                   XC_Cor_Remove
#define XC_DelayTick                XC_Cor_DelayTick
#define XC_DelayUs                  XC_Cor_DelayUs
#define XC_DelayMs                  XC_Cor_DelayMs
#define XC_DelaySec                 XC_Cor_DelaySec
#define XC_WaitForNotify            XC_Cor_WaitNotify
#define XC_WaitForNotifyMs          XC_Cor_WaitNotifyMs
#define XC_ClrNotify                XC_Cor_ClrNotify
#define XC_CheckNotifyWakeupTimeout XC_Cor_IsNotifyTimeout
#define XC_GetNotifyData            XC_Cor_GetNotifyData

/************************************************ 我是分割线 ************************************************/
/** 旧版函数名:来自"XC_Sch.h"(旧前缀"XCSch_", 2.1.0版本迁移到"XC_Sch_") */

#define XCSch_Init                  XC_Sch_Init
#define XCSch_Start                 XC_Sch_Start
#define XCSch_GetTaskNum            XC_Sch_GetTaskNum
#define XCSch_SetIdleCallback       XC_Sch_SetIdleCallback

/************************************************ 我是分割线 ************************************************/
/** 旧版函数/宏名:来自"XC_Task.h"(旧前缀"XCTask_", 2.1.0版本迁移到"XC_Task_") */

#define XCTask_Reg                  XC_Task_Reg
#define XCTask_Remove               XC_Task_Remove
#define XCTask_SetEntry             XC_Task_SetEntry
#define XCTask_Add                  XC_Task_Add
#define XCTask_Reset                XC_Task_Reset
#define XCTask_Suspend              XC_Task_Suspend
#define XCTask_Resume               XC_Task_Resume
#define XCTask_SendNotify           XC_Task_SendNotify
#define XCTask_ClrNotify            XC_Task_ClrNotify
#define XCTask_UpdateNotifyData     XC_Task_UpdateNotifyData
#define XCTask_ReadNotifyData       XC_Task_ReadNotifyData
#define XCTask_GetParam             XC_Task_GetParam
#define XCTask_GetState             XC_Task_GetState

/************************************************ 我是分割线 ************************************************/
/** 旧版函数/宏名:来自"XC_Time.h"(旧前缀"XCTime_", 2.1.0版本迁移到"XC_Time_") */

#define XCTime_GetTickUnit          XC_Time_GetTickUnit
#define XCTime_GetTick              XC_Time_GetTick
#define XCTime_GetMs                XC_Time_GetMs
#define XCTime_TickInc              XC_Time_TickInc
#define XCTime_TickSet              XC_Time_TickSet
#define XCTime_CheckTimeout         XC_Time_CheckTimeout
#define XCTime_CheckTimeoutMs       XC_Time_CheckTimeoutMs
#define XCTime_CheckTimeoutSec      XC_Time_CheckTimeoutSec
#define XCTime_GetRemain            XC_Time_GetRemain
#define XCTime_GetElapsed           XC_Time_GetElapsed
#define XCTime_TimerSet             XC_Time_TimerSet
#define XCTime_TimerSetMs           XC_Time_TimerSetMs
#define XCTime_TimerSetSec          XC_Time_TimerSetSec
#define XCTime_TimerRepeat          XC_Time_TimerRepeat
#define XCTime_TimerCheck           XC_Time_TimerCheck
#define XCTime_TimerClr             XC_Time_TimerClr
#define XCTime_TimerGetRemain       XC_Time_TimerGetRemain
#define XCTime_TimerGetElapsed      XC_Time_TimerGetElapsed
#define XCTime_TimerGetElapsedMs    XC_Time_TimerGetElapsedMs
#define XCTime_BlockDelay           XC_Time_BlockDelay
#define XCTime_BlockDelayMs         XC_Time_BlockDelayMs

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
