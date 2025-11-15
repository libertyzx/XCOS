/**
 * @file        XC_Sch.h
 * @brief       调度器实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/11/12
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     用于调度任务的代码
 * **********************************************
 *  修改日志
 *  - 见"XC_UpdateInfo.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_Sch_H_
#define _XC_Sch_H_
//=== 头文件
#include "XC_Cnf.h"
#include "XC_List.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 数据类型 */

/**
 * @brief   [用户]XCOS句柄
 * @details
 *  用于记录XCOS实例的数据,一个工程中开源有多个XCOS实例,用此句柄区分; \n
 *  字节数说明(32bit): 8*4+4*3+4+4 = 52Byte
 */
typedef struct XCOS_t {
    // 链表
    XCListNode_t ReadyList;        // 就绪链表
    XCListNode_t TimeList;         // 延时/超时/等待的链表
    XCListNode_t TimeOverflowList; // 时间溢出的链表
    XCListNode_t BlockedList;      // 阻塞链表

    XCListNode_t* pReadyNode; // 当前就绪节点(运行任务的节点),位于就绪表

    /**
     * @brief   下个任务唤醒的Tick
     * @details
     *  当系统Tick大于这个值,则需要开始处理唤醒任务;
     */
    XCuint_t NextTaskWakeTick;
    /**
     * @brief   上个Tick
     * @details
     *  只要任务被调度,则更新
     */
    XCuint_t PrevTick;

    uint8_t TaskNum;  // 任务数量
    uint8_t ListLock; // 链表锁(1锁定;0解锁)

    uint8_t TaskSchedTrigger;   // 任务调度触发(用于通知,挂起,恢复异步操作触发)
    uint8_t TaskSchedProcessed; // 任务调度处理(用于通知,挂起,恢复异步操作触发后处理)

    /**
     * @brief       框架空闲处理回调
     * @param[in]   phXCOS      [XCOS_t*]框架句柄
     * @param[in]   IdleTick    [XCuint_t]空闲的Tick值
     * @details
     *  在此回调函数中处理空闲相关事宜; \n
     *  当函数被调用时,必定没有任务是就绪的,框架是空闲的; \n
     *  若是在空闲时休眠系统,则需配置系统在"IdleTick"后唤醒框架; \n
     *  若是系统Tick计数也停止了则需要更新Tick值: \n
     *  - 系统Tick是定时器中断计数运行的,可以使用"XCSch_UpdateTickAfterWakeup"更新;
     *  - 系统Tick是一个计数器,则计数器需要更新为"phXCOS->NextTaskWakeTick";
     */
    void (*fIdle)(struct XCOS_t*, XCuint_t);
} XCOS_t;

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**函数宏*/

/**
 * @brief       [内部]链表锁
 * @param[in]   _phXCOS [XCOS_t*]框架句柄(会强制转换"XCOS句柄指针"类型)
 * @details     设置链表操作标志;
 */
#define XCSch_ListLock(_phXCOS)             \
    {                                       \
        ((XCOS_t*)(_phXCOS))->ListLock = 1; \
    }

/**
 * @brief       [内部]链表解锁
 * @param[in]   _phXCOS [XCOS_t*]框架句柄(会强制转换"XCOS句柄指针"类型)
 * @details     清除链表操作标志;
 */
#define XCSch_ListUnlock(_phXCOS)           \
    {                                       \
        ((XCOS_t*)(_phXCOS))->ListLock = 0; \
    }

/**
 * @brief       [内部]获取链表锁的状态
 * @param[in]   _phXCOS [XCOS_t*]框架句柄(会强制转换"XCOS句柄指针"类型)
 * @return      uint8_t
 * @retval      0 : 解锁
 * @retval      1 : 锁定
 * @details     判断当前链表是否在操作(是否被锁定);
 */
#define XCSch_GetListLockState(_phXCOS) (((XCOS_t*)(_phXCOS))->ListLock)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 函数声明-[用户]调度器处理 */

/**
 * @brief       [用户]调度器初始化
 * @param[in]   phXCOS  框架句柄
 * @details     在创建好"XCOS"句柄后,调用此函数初始化框架;
 */
void XCSch_Init(XCOS_t* phXCOS);

/**
 * @brief       [用户]调度器运行(阻塞)
 * @param[in]   phXCOS  框架句柄
 * @details     阻塞的运行调度器,调用此函数后不会返回;
 */
void XCSch_Run(XCOS_t* phXCOS);

/**
 * @brief       [用户]调度器运行(非阻塞)
 * @param[in]   phXCOS  框架句柄
 * @details     非阻塞运行调度器,需要循环调用此函数;
 */
void XCSch_RunNonBlocked(XCOS_t* phXCOS);

/**
 * @brief       [用户]获取任务数
 * @param[in]   phXCOS  框架句柄
 * @return      uint8_t 返回任务数量
 * @details     当前框架中有多少任务;
 */
uint8_t XCSch_GetTaskNum(XCOS_t* phXCOS);

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       [用户]设置空闲处理回调
 * @param[in]   phXCOS  框架句柄
 * @param[in]   fIdle  框架空闲处理回调
 * @details
 *  用于设置框架空闲处理回调; \n
 *  若是需要清除回调则"fIdle"值为NULL即可;
 */
void XCSch_SetIdleCallback(XCOS_t* phXCOS, void (*fIdle)(XCOS_t*, XCuint_t));

#ifdef __XC_SysTickIntAccMode__
/**
 * @brief       [用户]休眠唤醒后更新tick
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数是框架句柄中"fIdle"函数休眠框架,且定时器同时停止的情况下,在设备唤醒后调用;
 *  直接将Tick的值更新到框架句柄中的"NextTaskWakeTick"值
 *  注意:只有在系统Tick是定时器中断计数运行的情况下可以使用此函数;
 */
void XCSch_UpdateTickAfterWakeup(XCOS_t* phXCOS);
#endif

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
