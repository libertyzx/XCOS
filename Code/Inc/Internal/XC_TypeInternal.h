/**
 * @file        XC_TypeInternal.h
 * @brief       内部类型
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
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
#ifndef XC_TypeInternal_h
#define XC_TypeInternal_h
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
struct XCOS_tag {
    // 链表
    XC_ListNode_t ReadyList;        // 就绪链表
    XC_ListNode_t TimeList;         // 延时/超时/等待的链表
    XC_ListNode_t TimeOverflowList; // 时间溢出的链表
    XC_ListNode_t BlockedList;      // 阻塞链表

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
     *      Tick = XC_Time_GetTick() + IdleTick;
     *      XC_Time_TickSet(Tick)
     *      ```
     *  - 系统Tick是一个计数器,则计数器等于"XC_Time_GetTick() + IdleTick";
     */
    void (*fIdle)(struct XCOS_tag*, XC_Tick_t);
};

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   [内部]XCOS核心断点类型
 * @details 用于创建上下文切换的断点标记
 */
typedef COR_BP_t XC_BP_t;

/**
 * @brief   [内部]协程状态(存 TCB.CorState,调度器据此决定是否同轮切父)
 * @details
 *  - "XC_COR_DONE":      本层完成(执行到"XC_Cor_Leave");
 *  - "XC_COR_SUSPENDED": 本层挂起(延时/通知/让出/Call 登记后跳出);
 *  枚举值存 TCB.CorState(uint8_t),与 TaskState/NotifyState 同为"枚举值存 uint8_t 字段"模式;
 */
typedef enum {
    XC_COR_DONE = 0,  // 本层完成
    XC_COR_SUSPENDED, // 本层挂起(延时/通知/让出)
} XC_CorState_t;

/**
 * @brief   [内部]协程函数类型(顶层任务与子协程统一)
 * @details
 *  与现有 TCB.fTask 类型等价(void 返回):参数经"struct XC_TaskCB_tag*"引用,
 *  即用户侧"XC_TaskHandle_t"(XC_TaskCB_t* = struct XC_TaskCB_tag*),任务函数签名
 *  保持 void 不变(非破坏性 API);
 */
typedef void (*XC_CorFn_t)(struct XC_TaskCB_tag*);

/**
 * @brief   [内部]协程帧(任务私有帧栈的一个元素)
 * @details
 *  8 字节/层(32位):父层函数入口 + 父层返回点;
 *  栈底帧恒为顶层任务(只在第一次"XC_Cor_Call"时写入);
 */
typedef struct {
    XC_CorFn_t pfn; // 父层函数入口(弹帧恢复 fTask 用;栈底帧恒为顶层任务)
    XC_BP_t    BP;  // 父层返回点(ANSI:行号 / GNU:标签地址)
} XC_CorFrame_t;

/**
 * @brief   [内部]协程任务控制块(Task Control Block)
 * @details
 *  用于记录任务控制相关的数据,每个任务都需要一个独立的TCB;
 *  类型占字节数(32bit): 36→44Byte(+8B;7B 字段 + 1B 对齐 padding,整体仍 4B 对齐);
 *  ---
 *  基础数据(在"XC_Task_BasicInit"中被初始化)
 *      - "TaskWakeupTick"  任务下个唤醒的时间
 *      - "BP"              协程断点
 *      - "pNotifyData"     通知数据
 *      - "NotifyState"     通知状态
 *      - "NotifyProduced"  通知-生产者
 *      - "NotifyConsumed"  通知-消费者
 *      - "CorDepth"        协程深度
 *  非基础数据
 *      - "ListNode"    链表节点
 *      - "phXCOS"      任务所属的框架句柄
 *      - "fTask"       当前层入口(顶层或子层,由 XC_Cor_Call 切换;无嵌套时恒为顶层)
 *      - "pParam"      传递的参数
 *      - "TaskState"   任务状态
 *      - "pCorStack"   用户按需提供的帧栈(NULL=无嵌套)
 *      - "CorDepthMax" 栈容量(越界检查)
 *      - "CorState"    本轮结果
 */
struct XC_TaskCB_tag {
    XC_ListNode_t    ListNode;       // 链表节点
    struct XCOS_tag* phXCOS;         // 任务所属的框架句柄
    XC_CorFn_t       fTask;          // 当前层入口(顶层或子层,由 XC_Cor_Call 切换;无嵌套时恒为顶层)
    XC_BP_t          BP;             // 当前执行层断点(唯一,顶层/子层共用;不占栈)
    XC_Tick_t        TaskWakeupTick; // 任务下个唤醒的时间(0则一直阻塞)

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

    /* 协程嵌套支持 */
    XC_CorFrame_t* pCorStack;   // 用户按需提供的帧栈(NULL=无嵌套)
    uint8_t        CorDepth;    // 当前子层深度(0=在顶层)
    uint8_t        CorDepthMax; // 栈容量(越界检查)
    uint8_t        CorState;    // 本轮结果(值域:XC_CorState_t;填补对齐padding)
};

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
