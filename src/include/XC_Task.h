/**
 * @file        XC_Task.h
 * @brief       任务的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/11/12
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  - 实现了所有协程相关的操作;
 *  - 实现了任务处理需要的操作;
 * **********************************************
 *  修改日志
 *  - 见"XC_UpdateInfo.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_Task_H_
#define _XC_Task_H_
//=== 头文件
#include "BinData.h"
#include "XC_List.h"
#include "XC_Sch.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/*协程底层实现("ANSI-C"和"GNU-C"区分)*/
#ifdef __GNUC__
#include "COR_GNU.h" //运行"GNU-C"库
#else
#include "COR_ANSI.h" //运行"ANSI-C"库
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 宏/枚举 */

/**
 * @brief   [用户]函数返回值
 */
typedef enum {
    _XC_R_Fail = -1, // 失败
    _XC_R_OK   = 0,  // 成功
    _XC_R_Continue,  // 继续
} XCRetuen_t;

/**
 * @brief   [用户]任务状态值(当前协程状态)
 */
typedef enum {
    _XC_S_Void = 0,   // 空,被移除后的状态
    _XC_S_Run,        // 运行
    _XC_S_Ready,      // 就绪(注册后的状态)
    _XC_S_Delay,      // [阻塞]延时
    _XC_S_WaitNotify, // [阻塞]等待通知
    _XC_S_Suspend,    // [阻塞]挂起
} XCState_t;

/**
 * @brief   [内部]阻塞类型
 */
typedef enum {
    _XC_B_NonBlocked = _B0000_0000, // 没有阻塞(清除阻塞)
    _XC_B_Blocked    = _B0000_0001, // 阻塞
    _XC_B_WaitTime   = _B0000_0010, // 等待时间
} XCBlocked_t;

/**
 * @brief   [内部]唤醒状态
 */
typedef enum {
    _XC_Wake_Non = 0,    // 无唤醒
    _XC_Wake_Time,       // 时间到达唤醒
    _XC_Wake_Notify,     // 通知到达唤醒
    _XC_Wake_TaskResume, // 任务挂起后恢复
} XCWake_t;

/************************************************ 我是分割线 ************************************************/
/** 类型 */

/**
 * @brief       [内部]XCOS核心断点类型
 * @details     用于创建上下文切换的断点标记
 */
typedef COR_BP_t XCBP_t;

/**
 * @brief   [用户]协程任务控制块(Task Control Block)
 * @details
 *  用于记录任务控制相关的数据,每个任务都需要一个独立的TCB; \n
 *  类型占字节数(32bit): 8+4*6+8=40Byte,补齐占:40Byte
 */
typedef struct XCTCB_t {
    XCListNode_t   ListNode;        // 链表节点
    struct XCOS_t* phXCOS;          // 任务所属的框架句柄
    void (*fTask)(struct XCTCB_t*); // 函数运行入口(任务入口)
    XCuint_t TaskWakeTick;          // 任务下个唤醒的时间(0则一直阻塞)
    XCBP_t   BP;                    // 协程断点(Break Point)

    void* pParam;      // 传递的参数
    void* pNotifyData; // 通知数据

    uint8_t Blocked;   // 阻塞状态("XCBlocked_t"类型数据),用于阻塞调用时判断当前任务执行完成后是什么阻塞状态;
    uint8_t WakeType;  // 任务唤醒的类型("XCWake_t"类型数据),用于判断是谁唤醒或者超时;
    uint8_t TaskState; // 任务状态("XCState_t"类型数据);

    uint8_t NotifyTrigger;   // 通知触发
    uint8_t NotifyProcessed; // 通知触发处理

    /**
     *  以下三个变量用于处理任务挂起恢复; \n
     *  主要用于任务挂起和恢复的异步操作; \n
     *  "StateChangeType"的值为"XCState_t"类型中的: \n
     *  - "_XC_S_Void"    : 未挂起
     *  - "_XC_S_Suspend" : 挂起
     */
    uint8_t StateChangeTrigger;   // 状态改变触发
    uint8_t StateChangeProcessed; // 状态改变处理
    uint8_t StateChangeType;      // 最后一次改变的是什么状态(_XC_S_Suspend/_XC_S_Void)
} XCTCB_t;

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 函数声明-[内部] */

void XC_TaskBasicInit(XCTCB_t* phTCB); //[内部]任务基本初始化

/** 函数声明-[内部]任务链表节点处理 */

void XC_MoveTaskToReadyList(XCTCB_t* phTCB);                       // [内部]将任务移动到就绪表
void XC_MoveTaskToBlockedList(XCTCB_t* phTCB);                     // [内部]将任务移动到阻塞表
void XC_InsertTaskToTimeList(XCListNode_t* pList, XCTCB_t* phTCB); // [内部]将任务插入到时间表
void XC_RemoveTaskNode(XCTCB_t* phTCB);                            // [内部]移除任务节点

/**
 * @brief       [内部]将任务插入到就绪表
 * @param[in]   phTCB   [XCTCB_t*]协程控制块
 * @details
 *  将任务插入当前就绪任务前,任务节点在就绪表;
 */
#define XC_InsertTaskToReadyList(_phTCB) XCList_InsertNodeBefore((_phTCB)->phXCOS->pReadyNode, &(_phTCB)->ListNode)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 函数声明-[用户]基本任务操作 */

/**
 * @brief       [用户]任务注册
 * @param[in]   phXCOS  框架句柄
 * @param[in]   phTCB   协程任务控制块
 * @param[in]   fTask   任务的函数指针(任务入口)
 * @param[in]   pParam  传递给任务的参数
 * @return      int32_t
 * @retval      _XC_R_OK :      注册成功
 * @retval      _XC_R_Fail :    注册失败,任务太多
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 *  **不可多次注册同个任务(没有做重复判断,会发生未知情况)** \n
 */
int32_t XC_TaskReg(XCOS_t* phXCOS, XCTCB_t* phTCB, void (*fTask)(XCTCB_t*), void* pParam);

/**
 * @brief       [用户]任务移除
 * @param[in]   phTCB 协程控制块
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 */
void XC_TaskRemove(XCTCB_t* phTCB);

/**
 * @brief       [用户]任务复位
 * @param[in]   phTCB   协程控制块
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 *  **不可复位自身(复位自身使用"XC_Reset")** \n
 */
void XC_TaskReset(XCTCB_t* phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明-[用户]任务分步创建相关的用户函数 */

/**
 * @brief       [用户]设置任务入口
 * @param[in]   phTCB   协程任务控制块
 * @param[in]   fTask   任务的函数指针(任务入口)
 * @param[in]   pParam  传递给任务的参数
 * @details
 *  **不可在中断中使用** \n
 *  只设置任务入口和传递给任务的参数; \n
 *  一般配合"XC_AddTask"使用;
 */
void XC_SetTaskEntryPoint(XCTCB_t* phTCB, void (*fTask)(XCTCB_t*), void* pParam);

/**
 * @brief   [用户]添加任务
 * @param   phXCOS  框架句柄
 * @param   phTCB   协程任务控制块
 * @return  int32_t
 * @retval  _XC_R_OK :      注册成功
 * @retval  _XC_R_Fail :    注册失败,任务太多
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 *  添加的任务必须先调用"XC_SetTaskEntryPoint"; \n
 *  设置好任务入口和传递的参数才可添加; \n
 */
int32_t XC_AddTask(XCOS_t* phXCOS, XCTCB_t* phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明-[用户]通知 | 挂起 | 挂起恢复 */

/**
 * @brief       [用户]发送通知(void*)
 * @param[in]   phTCB       任务控制块
 * @param[in]   pNotifyData 通知传递的数据(void*)类型
 * @return      int32_t
 * @retval      _XC_R_OK :          通知成功
 * @retval      _XC_R_Continue :    任务已经唤醒(已在就绪表)
 * @retval      _XC_R_Fail :        任务被挂起
 * @details     发送通知,唤醒任务;
 */
int32_t XC_SendNotify(XCTCB_t* phTCB, void* pNotifyData);

/**
 * @brief       [用户]任务挂起
 * @param[in]   phTCB   任务控制块
 * @return      int32_t
 * @retval      _XC_R_OK :          挂起成功
 * @retval      _XC_R_Continue :    已经被挂起
 * @details
 *  **可在中断中调用** \n
 *  将任务挂起,本次任务运行完成后暂停任务; \n
 */
int32_t XC_TaskSuspend(XCTCB_t* phTCB);

/**
 * @brief       [用户]任务挂起恢复
 * @param[in]   phTCB   任务控制块
 * @return      int32_t
 * @retval      _XC_R_OK :          恢复成功
 * @retval      _XC_R_Continue :    任务没有挂起
 * @details
 *  **可在中断中调用** \n
 *  只能恢复被挂起的任务;
 */
int32_t XC_TaskResume(XCTCB_t* phTCB);

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 协程块:开始/结束 */

/**
 * @brief       [用户][协程]进入
 * @param[in]   _phTCB_  [XCTCB_t*]任务控制块
 * @details
 *  **协程块的开始;** \n
 *  必须搭配"XC_Leave"使用;
 */
#define XC_Enter(_phTCB_)                                 \
    {                                                     \
        /*全局变量转局部变量可加快运行速度*/              \
        XCTCB_t* _phXCTCB_ = (_phTCB_);      /*得到PCB*/  \
        XCBP_t*  _pXCPB_   = &_phXCTCB_->BP; /*得到断点*/ \
        /*启动协程*/                                      \
        _COR_Start(*_pXCPB_); /*启动*/

/**
 * @brief       [用户][协程]离开
 * @details
 *  **协程块的结束;** \n
 *  必须搭配"XC_Enter"使用;
 */
#define XC_Leave()       \
    _COR_End(); /*结束*/ \
    }

/************************************************ 我是分割线 ************************************************/
/** 协程块:协程控制,必须在协程块中调用 */

/**
 * @brief       [用户][协程]让出控制
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,跳出协程,下次调度后从此处继续处理; \n
 */
#define XC_Yield()                                             \
    {                                                          \
        _phXCTCB_->TaskState = _XC_S_Ready; /*任务状态:就绪*/  \
        _COR_SetBPBreak(*_pXCPB_)           /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]任务复位
 * @details
 *  **必须在协程块中使用;** \n
 *  复位当前在运行的任务;运行此宏后将立刻退出协程块
 */
#define XC_Reset()               \
    {                            \
        XC_TaskReset(_phXCTCB_); \
        _COR_Break(*_pXCPB_);    \
    }

/**
 * @brief       [用户][协程]协程块内获取任务注册时传递的参数(void*)
 * @return      void*   返回空指针类型数据;
 * @details
 *  **必须在协程块中使用;** \n
 *  用来获取参数;
 *  > 注意: 因为协程内上下文切换局部变量是不保存的,
 *  > 所以若是要使用传递的参数需要再"XC_Enter"前将参数赋值给变量;
 */
#define XC_GetParam() (_phXCTCB_->pParam)

/**
 * @brief       [用户][协程]将自身挂起
 * @details
 *  必须在协程块中使用; \n
 *  将自身挂起;
 */
#define XC_Suspend()                                  \
    {                                                 \
        XC_TaskSuspend(_phXCTCB_); /*任务挂起*/       \
        _COR_SetBPBreak(*_pXCPB_); /*设置断点并跳出*/ \
    }

/************************************************ 我是分割线 ************************************************/
/** 协程块:延时处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]延时n个Tick
 * @param[in]   _n  [XCuint_t]延时的值
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,延时_n个基础时钟;
 */
#define XC_DelayTick(_n)                                                \
    {                                                                   \
        _phXCTCB_->TaskState    = _XC_S_Delay;    /*任务状态:延时*/     \
        _phXCTCB_->WakeType     = _XC_Wake_Non;   /*清唤醒类型*/        \
        _phXCTCB_->Blocked      = _XC_B_WaitTime; /*阻塞类型:等待时间*/ \
        _phXCTCB_->TaskWakeTick = (_n);           /*保存时间*/          \
        _COR_SetBPBreak(*_pXCPB_);                /*设置断点并跳出*/    \
    }

/**
 * @brief       [用户][协程]延时us
 * @param[in]   _n  [XCuint_t]延时的值
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,延时_n个时间; \n
 *  > 注意:us延时必须要Tick计数在us时基上才能准确的调用;
 */
#define XC_Delay_us(_n) XC_DelayTick(_Time_us2Tick(_n))

/**
 * @brief       [用户][协程]延时ms
 * @param[in]   _n  [XCuint_t]延时的值
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,延时_n个时间; \n
 */
#define XC_Delay_ms(_n) XC_DelayTick(_Time_ms2Tick(_n))

/**
 * @brief       [用户][协程]延时s
 * @param[in]   _n  [XCuint_t]延时的值
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,延时_n个时间; \n
 */
#define XC_Delay_s(_n)  XC_DelayTick(_Time_s2Tick(_n))

/************************************************ 我是分割线 ************************************************/
/** 协程块:任务通知处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]等待通知(单位:系统Tick)
 * @param[in]   _TickTimeout    [XCuint_t]超时时间,单位:系统Tick;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;** \n
 *  任务等待通知; \n
 *  唤醒后用"XC_GetWakeTimeout"判断是否超时;
 *  注意:"TaskState"任务状态参数需要最先改变,防止出现临界段;
 */
#define XC_WaitNotify(_TickTimeout)                                                          \
    {                                                                                        \
        _phXCTCB_->TaskState    = _XC_S_WaitNotify;               /*任务状态:等待通知*/      \
        _phXCTCB_->WakeType     = _XC_Wake_Non;                   /*唤醒类型:无唤醒*/        \
        _phXCTCB_->Blocked      = _XC_B_Blocked | _XC_B_WaitTime; /*阻塞类型:阻塞+等待时间*/ \
        _phXCTCB_->TaskWakeTick = (_TickTimeout);                 /*超时的时间*/             \
        _COR_SetBPBreak(*_pXCPB_);                                /*设置断点并跳出*/         \
    }

/**
 * @brief       [用户][协程]等待通知(单位:ms)
 * @param[in]   _msTimeout  [XCuint_t]超时时间,单位:ms;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;** \n
 *  任务等待通知; \n
 *  唤醒后用"XC_GetWakeTimeout"判断是否超时;
 */
#define XC_WaitNotify_ms(_msTimeout) XC_WaitNotify(_Time_ms2Tick(_msTimeout))

/**
 * @brief       [用户][协程]获取唤醒超时
 * @return      boot
 * @retval      0 : 没有超时
 * @retval      1 : 超时
 * @details
 *  **必须在协程块中使用;** \n
 *  用于判断任务通知阻塞唤醒后是否超时;
 *  > 通知函数:XC_WaitNotify
 */
#define XC_GetWakeTimeout()          (_phXCTCB_->WakeType == _XC_Wake_Time)

/**
 * @brief       [用户][协程]获取通知的数据(void*)
 * @return      void*   返回通知数据
 * @details
 *  **必须在协程块中使用;** \n
 *  被通知唤醒后获取通知传递的数据;
 */
#define XC_GetNotifyData()           (_phXCTCB_->pNotifyData)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 可在非协程块中调用 */

/**
 * @brief       [用户]获取任务注册时传递的参数(void*)
 * @return      void*   返回空指针类型数据;
 * @details
 *  **任意位置可调用;** \n
 *  用来获取任务参数;
 *  > 注意: 因为协程内上下文切换局部变量是不保存的,
 *  > 所以若是要使用传递的参数需要再"XC_Enter"前将参数赋值给变量;
 */
#define XC_GetTaskParam(_phTCB_)     ((_phTCB_)->pParam)

/**
 * @brief       [用户]获取任务运行状态
 * @param[in]   _phTCB_  [XCTCB_t*]任务控制块
 * @return      XCState_t
 * @retval      _XC_S_Void :        空,被移除后的状态
 * @retval      _XC_S_Run :         运行
 * @retval      _XC_S_Ready :       就绪(注册后的状态)
 * @retval      _XC_S_Delay :       [阻塞]延时
 * @retval      _XC_S_WaitNotify :  [阻塞]等待通知
 * @retval      _XC_S_Suspend :     [阻塞]挂起
 * @details     说明
 *  **任意位置可调用;** \n
 *  获取任务运行的状态;
 */
#define XC_GetTaskState(_phTCB_)     ((_phTCB_)->TaskState)

/**
 * @brief       [用户]更新通知数据(void*)
 * @param[in]   _phTCB_          [XCTCB_t*]任务控制块
 * @param[in]   _pNotifyData    [void*]通知数据
 * @details
 *  **任意位置可调用;** \n
 *  在不使用任务通知的时候,可以用通知数据来传递数据; \n
 *  此函数用于更新(写)通知数据;
 */
#define XC_UpdateNotifyData(_phTCB_, _pNotifyData) \
    {                                              \
        (_phTCB_)->pNotifyData = (_pNotifyData);   \
    }

/**
 * @brief       [用户]读取通知数据(void*)
 * @param[in]   _phTCB_  [XCTCB_t*]任务控制块
 * @return      void*   返回通知数据
 * @details
 *  **任意位置可调用;** \n
 *  在不使用任务通知的时候,可以用通知数据来传递数据; \n
 *  此函数用于读取通知数据;
 */
#define XC_ReadNotifyData(_phTCB_) ((_phTCB_)->pNotifyData)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
