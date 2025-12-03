/**
 * @file        XC_Internal.h
 * @brief       内部实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2025/11/26
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     框架内部实现的代码
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_Internal_h_
#define _XC_Internal_h_

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 调度器调度锁 */

/**
 * @brief       [内部]锁
 * @param[in]   _phXCOS [XC_OSHandle_t]框架句柄(会强制转换"XCOS句柄指针"类型)
 * @details
 *  主要用来锁定任务切换(包括链表操作,状态切换),防止中断调用时资源竞争;
 */
#define XCSch_Lock(_phXCOS)                   \
    {                                         \
        ((XC_OSHandle_t)(_phXCOS))->Lock = 1; \
    }

/**
 * @brief       [内部]解锁
 * @param[in]   _phXCOS [XC_OSHandle_t]框架句柄(会强制转换"XCOS句柄指针"类型)
 * @details
 *  主要用来锁定任务切换(包括链表操作,状态切换),防止中断调用时资源竞争;
 */
#define XCSch_Unlock(_phXCOS)                 \
    {                                         \
        ((XC_OSHandle_t)(_phXCOS))->Lock = 0; \
    }

/**
 * @brief       [内部]获取锁的状态
 * @param[in]   _phXCOS [XC_OSHandle_t]框架句柄(会强制转换"XCOS句柄指针"类型)
 * @return      uint8_t
 * @retval      0 : 解锁
 * @retval      1 : 锁定
 * @details
 *  主要用来锁定任务切换(包括链表操作,状态切换),防止中断调用时资源竞争;
 */
#define XCSch_GetLockState(_phXCOS)          (((XC_OSHandle_t)(_phXCOS))->Lock)

/************************************************ 我是分割线 ************************************************/

/** 任务链表节点处理 */

/**
 * @brief       [内部]将任务移动到就绪表
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @details
 *  将任务移动到下个就绪节点前,确保最后调用;
 */
#define XCTask_MoveTaskToReadyList(_phTCB)   XCList_MoveNodeBefore((_phTCB)->phXCOS->pNextReadyNode, &(_phTCB)->ListNode);

/**
 * @brief       [内部]将任务移动到阻塞表
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @details
 *  将任务移动到阻塞表尾部;
 */
#define XCTask_MoveTaskToBlockedList(_phTCB) XCList_MoveNodeBefore(&(_phTCB)->phXCOS->BlockedList, &(_phTCB)->ListNode);

/**
 * @brief       [内部]移除任务节点
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @return      void
 * @details
 *  从XCOS中删除TCB节点;
 */
#define XCTask_RemoveTaskNode(_phTCB)        XCList_Remove(&(_phTCB)->ListNode);

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
