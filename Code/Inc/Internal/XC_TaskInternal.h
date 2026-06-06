/**
 * @file        XC_TaskInternal.h
 * @brief       任务的实现-内部函数
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2026/01/05
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     框架任务内部实现的代码
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_TaskInternal_H_
#define _XC_TaskInternal_H_
//=== 头文件
#include "Internal/XC_List.h"
#include "XC_Type.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 函数声明-协程内部处理函数 */

/**
 * @brief       [内部]等待通知处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details     用于协程内部"等待通知"处理;
 */
void XCTask_HandleWaitNotify(XC_TaskHandle_t phTCB, uint32_t TickCount);

/**
 * @brief       [内部]延时处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details     用于协程内部"延时"处理;
 */
void XCTask_HandleDelay(XC_TaskHandle_t phTCB, uint32_t TickCount);

/**
 * @brief       [内部]挂起处理
 * @param[in]   phTCB       任务句柄
 * @details     用于协程内部"挂起自身"和"挂起任务"使用;
 */
void XCTask_HandleSuspend(XC_TaskHandle_t phTCB);

/**
 * @brief       [内部]复位处理
 * @param[in]   phTCB       任务句柄
 * @details     用于协程内部"复位自身"和"复位任务"使用;
 */
void XCTask_HandleReset(XC_TaskHandle_t phTCB);

/**
 * @brief       [内部]移除处理
 * @param[in]   phTCB 协程控制块
 * @details     用于协程内部"移除自身"和"任务移除"使用;
 */
void XCTask_HandleRemove(XC_TaskHandle_t phTCB);

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 任务链表节点处理 */

/**
 * @brief       [内部]将任务移动到就绪表
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @details     将任务移动上个就绪节点后,确保最后调用;
 */
#define XCTask_MoveTaskToReadyList(_phTCB)   XCList_MoveNodeAfter((_phTCB)->phXCOS->pPrevReadyNode, &(_phTCB)->ListNode)

/**
 * @brief       [内部]将任务移动到阻塞表
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @details     将任务移动到阻塞表头
 */
#define XCTask_MoveTaskToBlockedList(_phTCB) XCList_MoveNodeAfter(&(_phTCB)->phXCOS->BlockedList, &(_phTCB)->ListNode)

/**
 * @brief       [内部]移除任务节点
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @return      void
 * @details     从XCOS中删除TCB节点;
 */
#define XCTask_RemoveTaskNode(_phTCB)        XCList_Remove(&(_phTCB)->ListNode)

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
