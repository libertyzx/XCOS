/**
 * @file        XC_Core.h
 * @brief       内核基础
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  框架内核的基础操作(框架实例锁等);
 *  锁操作原位于"XC_SchInternal.h"(调度锁),因"XC_Task"与"XC_Sch"模块共用而下沉至此;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_Core_h
#define XC_Core_h
//=== 头文件
#include "XC_TypeInternal.h" // XCOS_t:锁操作的框架实例类型

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 框架实例锁 */

/**
 * @brief       [内部]锁定框架实例
 * @param[in]   phXCOS [struct XCOS_tag*]框架句柄(可为"XCOS_t*"或"XC_OSHandle_t")
 * @details
 *  主要用来锁定任务切换(包括链表操作,状态切换),防止中断调用时资源竞争;
 */
#define XC_Core_Lock(phXCOS)                      \
    do {                                          \
        ((struct XCOS_tag*)(phXCOS))->Lock = 1U;  \
    } while(0)

/**
 * @brief       [内部]解锁框架实例
 * @param[in]   phXCOS [struct XCOS_tag*]框架句柄(可为"XCOS_t*"或"XC_OSHandle_t")
 * @details
 *  主要用来锁定任务切换(包括链表操作,状态切换),防止中断调用时资源竞争;
 */
#define XC_Core_Unlock(phXCOS)                    \
    do {                                          \
        ((struct XCOS_tag*)(phXCOS))->Lock = 0U;  \
    } while(0)

/**
 * @brief       [内部]获取框架实例锁状态
 * @param[in]   phXCOS [struct XCOS_tag*]框架句柄(可为"XCOS_t*"或"XC_OSHandle_t")
 * @return      uint8_t
 * @retval      0 : 解锁
 * @retval      1 : 锁定
 * @details
 *  主要用来锁定任务切换(包括链表操作,状态切换),防止中断调用时资源竞争;
 */
#define XC_Core_GetLockState(phXCOS) (((struct XCOS_tag*)(phXCOS))->Lock)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
