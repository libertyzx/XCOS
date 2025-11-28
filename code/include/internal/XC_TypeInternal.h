/**
 * @file        XC_TypeInternal.h.h
 * @brief       内部类型
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/11/26
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     存放框架内部类型的代码
 * **********************************************
 *  修改日志
 *  - 见"XC_UpdateInfo.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_TypeInternal_h_
#define _XC_TypeInternal_h_
//=== 头文件
#include "XC_Config.h"
#include "internal/XC_List.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/** 协程底层实现("ANSI-C"和"GNU-C"区分) */
#ifdef __GNUC__
#include "internal/XC_CorGNU.h" //运行"GNU-C"库
#else
#include "internal/XC_CorANSI.h" //运行"ANSI-C"库
#endif

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
 *  字节数说明(32bit): 8*4+4*2+4+4 = 48Byte
 */
struct XCOS_t {
    // 链表
    XCListNode_t ReadyList;        // 就绪链表
    XCListNode_t TimeList;         // 延时/超时/等待的链表
    XCListNode_t TimeOverflowList; // 时间溢出的链表
    XCListNode_t BlockedList;      // 阻塞链表

    XCListNode_t* pNextReadyNode; // 下个就绪的节点,位于就绪表

    XC_Tick_t PrevTick; // 上个Tick,用于判断Tick溢出,在时间处理中时间表更新时更新

    uint8_t TaskNum; // 任务数量
    uint8_t Lock;    // 锁(1锁定;0解锁),用于中断处理

    uint8_t TaskSchedTrigger;   // 任务调度触发(用于通知,挂起,恢复异步操作触发)
    uint8_t TaskSchedProcessed; // 任务调度处理(用于通知,挂起,恢复异步操作触发后处理)
    /**
     * @brief       框架空闲处理回调
     * @param[in]   phXCOS      [XC_OSHandle_t]框架句柄
     * @param[in]   IdleTick    [XC_Tick_t]空闲的Tick值(空闲多少个Tick)
     * @details
     *  在此回调函数中处理空闲相关事宜; \n
     *  当函数被调用时,必定没有任务是就绪的,框架是空闲的; \n
     *  若是在空闲时休眠系统,则需配置系统在"IdleTick"后唤醒框架; \n
     *  若是系统Tick计数也停止了则需要更新Tick值: \n
     *  - 系统Tick是定时器中断计数运行的,可以使用以下方式更新: \n
     *      ```
     *      volatile XC_Tick_t Tick;
     *      Tick = XCTime_GetTick() + IdleTick;
     *      XCTime_TickSet(Tick)
     *      ```
     *  - 系统Tick是一个计数器,则计数器等于"XCTime_GetTick() + IdleTick"; \n
     */
    void (*fIdle)(struct XCOS_t*, XC_Tick_t);
};

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   [内部]XCOS核心断点类型
 * @details 用于创建上下文切换的断点标记
 */
typedef COR_BP_t XCBP_t;

/**
 * @brief   [用户]协程任务控制块(Task Control Block)
 * @details
 *  用于记录任务控制相关的数据,每个任务都需要一个独立的TCB; \n
 *  类型占字节数(32bit): 8+4*6+4+3=39Byte,补齐占:40Byte
 */
typedef struct XC_TaskCB_t {
    XCListNode_t   ListNode;            // 链表节点
    struct XCOS_t* phXCOS;              // 任务所属的框架句柄
    void (*fTask)(struct XC_TaskCB_t*); // 函数运行入口(任务入口)
    XC_Tick_t TaskWakeTick;             // 任务下个唤醒的时间(0则一直阻塞)
    XCBP_t    BP;                       // 协程断点(Break Point)

    void* pParam;      // 传递的参数
    void* pNotifyData; // 通知数据

    uint8_t WakeType;  // 任务唤醒的类型("XC_WakeType_t"类型数据),用于判断是谁唤醒或者超时;
    uint8_t TaskState; // 任务状态("XC_TaskState_t"类型数据);

    uint8_t NotifyTrigger;   // 通知触发
    uint8_t NotifyProcessed; // 通知触发处理

    /**
     *  以下三个变量用于处理任务挂起恢复; \n
     *  主要用于任务挂起和恢复的异步操作; \n
     *  "StateChangeType"的值为"XC_TaskState_t"类型中的: \n
     *  - "XC_TASK_VOID"    : 未挂起
     *  - "XC_TASK_SUSPEND" : 挂起
     */
    uint8_t StateChangeTrigger;   // 状态改变触发
    uint8_t StateChangeProcessed; // 状态改变处理
    uint8_t StateChangeType;      // 最后一次改变的是什么状态(XC_TASK_SUSPEND/XC_TASK_VOID)
} XC_TaskCB_t;

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
