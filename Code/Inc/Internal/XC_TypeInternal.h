/**
 * @file        XC_TypeInternal.h.h
 * @brief       内部类型
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2026/01/05
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     存放框架内部类型的代码
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_TypeInternal_h_
#define _XC_TypeInternal_h_
//=== 头文件
#include "Internal/XC_List.h"
#include "XC_Config.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/** 协程底层实现("ANSI-C"和"GNU-C"区分) */
#ifdef __GNUC__
#include "Internal/XC_CorGNU.h" //运行"GNU-C"库
#else
#include "Internal/XC_CorANSI.h" //运行"ANSI-C"库
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 枚举类型 */

/**
 * @brief   [私有]通知唤醒状态
 */
typedef enum {
    XC_NOTIFY_WAKEUP = 0, // 通知唤醒
    XC_NOTIFY_WAIT,       // 等待通知
} XC_NotifyState_t;

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 数据类型 */

/**
 * @brief   [内部]XCOS句柄
 * @details
 *  用于记录XCOS实例的数据,一个工程中开源有多个XCOS实例,用此句柄区分;
 *  字节数说明(32bit): 8*4+4*2+4+4 = 48Byte
 */
struct XCOS_t {
    // 链表
    XCListNode_t ReadyList;        // 就绪链表
    XCListNode_t TimeList;         // 延时/超时/等待的链表
    XCListNode_t TimeOverflowList; // 时间溢出的链表
    XCListNode_t BlockedList;      // 阻塞链表

    XCListNode_t* pPrevReadyNode; // 上个就绪的节点,位于就绪表

    XC_Tick_t PrevTick; // 上个Tick,用于判断Tick溢出,在时间处理中时间表更新时更新

    uint8_t          TaskNum;       // 任务数量
    volatile uint8_t Lock;          // 锁(1锁定;0解锁),用于中断处理
    volatile uint8_t EventProduced; // 事件-生产者(用于通知操作触发)
    volatile uint8_t EventConsumed; // 事件-消费者(用于通知异步操作触发后处理)

    /**
     * @brief       框架空闲处理回调
     * @param[in]   phXCOS      [XC_OSHandle_t]框架句柄
     * @param[in]   IdleTick    [XC_Tick_t]空闲的Tick值(空闲多少个Tick)
     * @details
     *  在此回调函数中处理空闲相关事宜;
     *  当函数被调用时,必定没有任务是就绪的,框架是空闲的;
     *  若是在空闲时休眠系统,则需配置系统在"IdleTick"后唤醒框架;
     *  若是系统Tick计数也停止了则需要更新Tick值:
     *  - 系统Tick是定时器中断计数运行的,可以使用以下方式更新:
     *      ```
     *      XC_Tick_t Tick;
     *      Tick = XCTime_GetTick() + IdleTick;
     *      XCTime_TickSet(Tick)
     *      ```
     *  - 系统Tick是一个计数器,则计数器等于"XCTime_GetTick() + IdleTick";
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
 * @brief   [内部]协程任务控制块(Task Control Block)
 * @details
 *  用于记录任务控制相关的数据,每个任务都需要一个独立的TCB;
 *  类型占字节数(32bit): 8+4*6+4=36Byte;
 *  ---
 *  基础数据(在"XCTask_BasicInit"中被初始化)
 *      - "TaskWakeupTick"  任务下个唤醒的时间
 *      - "BP"              协程断点
 *      - "pNotifyData"     通知数据
 *      - "NotifyState"     通知状态
 *      - "NotifyProduced"  通知-生产者
 *      - "NotifyConsumed"  通知-消费者
 *  非基础数据
 *      - "ListNode"    链表节点
 *      - "phXCOS"      任务所属的框架句柄
 *      - "fTask"       函数运行入口
 *      - "pParam"      传递的参数
 *      - "TaskState"   任务状态
 */
struct XC_TaskCB_t {
    XCListNode_t   ListNode;            // 链表节点
    struct XCOS_t* phXCOS;              // 任务所属的框架句柄
    void (*fTask)(struct XC_TaskCB_t*); // 函数运行入口(任务入口)
    XCBP_t    BP;                       // 协程断点(Break Point)
    XC_Tick_t TaskWakeupTick;           // 任务下个唤醒的时间(0则一直阻塞)

    void*            pParam;      // 传递的参数
    void*            pNotifyData; // 通知数据
    volatile uint8_t TaskState;   // 任务状态("XC_TaskState_t"类型数据)
    volatile uint8_t NotifyState; // 通知状态("XC_NotifyState_t"类型数据)

    /**
     * 框架的通知使用生产者和消费者计数实现;
     * - 单生产者单消费者(SPSC)情况下线程是安全的;
     * - 生产者和单消费者只在创建任务时清0;
     */
    volatile uint8_t NotifyProduced; // 通知-生产者
    volatile uint8_t NotifyConsumed; // 通知-消费者
};

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
