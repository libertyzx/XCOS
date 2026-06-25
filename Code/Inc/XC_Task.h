/**
 * @file        XC_Task.h
 * @brief       任务的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2026/01/06
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  - 实现了所有协程相关的操作;
 *  - 实现了任务处理需要的操作;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_Task_H_
#define _XC_Task_H_
//=== 头文件
#include "Internal/XC_TaskInternal.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 函数声明-[用户]任务注册和删除 */

/**
 * @brief       [用户]任务注册
 * @param[in]   phXCOS  框架句柄
 * @param[in]   phTCB   协程任务控制块
 * @param[in]   fTask   任务的函数指针(任务入口)
 * @param[in]   pParam  传递给任务的参数
 * @return      XC_Retuen_t
 * @retval      XC_OK :      注册成功
 * @retval      XC_FAIL :    注册失败,任务太多
 * @details
 *  注册一个任务;
 *  **不可多次注册同个任务(没有做重复判断,会发生未知情况)**
 */
XC_Retuen_t XCTask_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam);

/**
 * @brief       [用户]任务移除
 * @param[in]   phTCB 协程控制块
 * @details     移除一个任务
 */
void XCTask_Remove(XC_TaskHandle_t phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明-[用户]任务分步创建相关的用户函数 */

/**
 * @brief       [用户]设置任务入口
 * @param[in]   phTCB   协程任务控制块
 * @param[in]   fTask   任务的函数指针(任务入口)
 * @param[in]   pParam  传递给任务的参数
 * @details
 *  只设置任务入口和传递给任务的参数;
 *  一般配合XCTask_Addk"使用;
 */
void XCTask_SetEntry(XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam);

/**
 * @brief   [用户]添加任务
 * @param   phXCOS  框架句柄
 * @param   phTCB   协程任务控制块
 * @return  XC_Retuen_t
 * @retval  XC_OK :      注册成功
 * @retval  XC_FAIL :    注册失败,任务太多
 * @details
 *  添加的任务必须先调用"XCTask_SetEntry";
 *  设置好任务入口和传递的参数才可添加;
 */
XC_Retuen_t XCTask_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明-[用户]复位 | 挂起 | 挂起恢复 */

/**
 * @brief       [用户]任务复位
 * @param[in]   phTCB   协程控制块
 * @details
 *  服务一个任务;
 *  **不可复位自身(复位自身使用"XC_Reset")**
 */
void XCTask_Reset(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]任务挂起
 * @param[in]   phTCB   任务控制块
 * @return      XC_Retuen_t
 * @retval      XC_OK :          挂起成功
 * @retval      XC_FAIL:         失败(任务不存在)
 * @retval      XC_CONTINUE :    异步操作中
 * @details     将任务挂起,本次任务运行完成后暂停任务;
 */
XC_Retuen_t XCTask_Suspend(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]任务挂起恢复
 * @param[in]   phTCB   任务控制块
 * @return      XC_Retuen_t
 * @retval      XC_OK :          恢复成功
 * @retval      XC_FAIL:         失败(任务未挂起)
 * @details     只能恢复被挂起的任务;
 */
XC_Retuen_t XCTask_Resume(XC_TaskHandle_t phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明/函数宏-[用户]通知相关 */

/**
 * @brief       [用户]发送通知
 * @param[in]   phTCB           需要发送通知的任务TCB
 * @param[in]   pNotifyData     通知传递的参数
 * @return      XC_Retuen_t
 * @retval      XC_OK :          通知成功
 * @retval      XC_FAIL :        任务不存在或者任务被挂起
 * @retval      XC_CONTINUE :    异步操作中
 * @details
 *  **线程安全(SPSC)**
 *  发送通知,唤醒任务;
 */
XC_Retuen_t XCTask_SendNotify(XC_TaskHandle_t phTCB, void* pNotifyData);

/**
 * @brief       [用户]清除通知
 * @param[in]   phTCB       [XC_TaskHandle_t]任务控制块
 * @details
 *  用于清除通知;
 *  可在"等待通知"前调用,防止通知提前到达;
 */
#define XCTask_ClrNotify(_phTCB_)                              \
    {                                                          \
        (_phTCB_)->NotifyConsumed = (_phTCB_)->NotifyProduced; \
    }

/**
 * @brief       [用户]更新通知数据(void*)
 * @param[in]   _phTCB_         [XC_TaskHandle_t]任务控制块
 * @param[in]   _pNotifyData    [void*]通知数据
 * @details
 *  在不使用任务通知的时候,可以用通知数据来传递数据;
 *  此函数用于更新(写)通知数据;
 */
#define XCTask_UpdateNotifyData(_phTCB_, _pNotifyData) \
    {                                                  \
        (_phTCB_)->pNotifyData = (_pNotifyData);       \
    }

/**
 * @brief       [用户]读取通知数据(void*)
 * @param[in]   _phTCB_  [XC_TaskHandle_t]任务控制块
 * @return      void*   返回通知数据
 * @details
 *  在不使用任务通知的时候,可以用通知数据来传递数据;
 *  此函数用于读取通知数据;
 */
#define XCTask_ReadNotifyData(_phTCB_) ((_phTCB_)->pNotifyData)

/************************************************ 我是分割线 ************************************************/
/** 函数宏-[用户]参数 | 状态 */

/**
 * @brief       [用户]获取任务注册时传递的参数(void*)
 * @return      void*   返回空指针类型数据;
 * @details
 *  用来获取任务参数;
 *  > 注意: 因为协程内上下文切换局部变量是不保存的,
 *  > 所以若是要使用传递的参数需要再"XC_Enter"前将参数赋值给变量;
 */
#define XCTask_GetParam(_phTCB_)       ((_phTCB_)->pParam)

/**
 * @brief       [用户]获取任务运行状态
 * @param[in]   _phTCB_  [XC_TaskHandle_t]任务控制块
 * @return      XC_TaskState_t
 * @retval      XC_TASK_VOID :          空(任务创建前或被移除后的状态)
 * @retval      XC_TASK_RUN :           运行(正在运行的任务)
 * @retval      XC_TASK_READY :         就绪(在就绪表中的任务状态,任务注册后为就绪)
 * @retval      XC_TASK_BLOCKED :       阻塞(延时,等待通知后的状态)
 * @retval      XC_TASK_SUSPEND :       挂起
 * @details     获取任务运行的状态;
 */
#define XCTask_GetState(_phTCB_)       ((XC_TaskState_t)((_phTCB_)->TaskState))

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 协程块:开始/结束 */

/**
 * @brief       [用户][协程]进入
 * @param[in]   _phTCB_  [XC_TaskHandle_t]任务控制块
 * @details
 *  **协程块的开始;**
 *  必须搭配"XC_Leave"使用;
 */
#define XC_Enter(_phTCB_)                                        \
    {                                                            \
        /*全局变量转局部变量可加快运行速度*/                     \
        XC_TaskHandle_t _phXCTCB_ = (_phTCB_);      /*得到PCB*/  \
        XCBP_t*         _pXCPB_   = &_phXCTCB_->BP; /*得到断点*/ \
        /*启动协程*/                                             \
        _COR_Start(*_pXCPB_); /*启动*/

/**
 * @brief       [用户][协程]离开
 * @details
 *  **协程块的结束;**
 *  必须搭配"XC_Enter"使用;
 */
#define XC_Leave()       \
    _COR_End(); /*结束*/ \
    }

/************************************************ 我是分割线 ************************************************/
/** 协程块:协程控制,必须在协程块中调用 */

/**
 * @brief       [用户][协程]协程块内获取任务注册时传递的参数(void*)
 * @return      void*   返回空指针类型数据;
 * @details
 *  **必须在协程块中使用;**
 *  用来获取参数;
 *  > 注意: 因为协程内上下文切换局部变量是不保存的,
 *  > 所以若是要使用传递的参数需要再"XC_Enter"前将参数赋值给变量;
 */
#define XC_GetParam() (_phXCTCB_->pParam)

/**
 * @brief       [用户][协程]让出控制
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,跳出协程,下次调度后从此处继续处理;
 */
#define XC_Yield()                                               \
    {                                                            \
        _phXCTCB_->TaskState = XC_TASK_READY; /*任务状态:就绪*/  \
        _COR_SetBPBreak(*_pXCPB_)             /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]将自身挂起
 * @details
 *  **必须在协程块中使用;**
 *  将自身挂起;
 */
#define XC_Suspend()                                        \
    {                                                       \
        XCTask_HandleSuspend(_phXCTCB_); /*任务挂起*/       \
        _COR_SetBPBreak(*_pXCPB_);       /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]任务复位
 * @details
 *  **必须在协程块中使用;**
 *  复位当前在运行的任务;运行此宏后将立刻退出协程块
 */
#define XC_Reset()                     \
    {                                  \
        XCTask_HandleReset(_phXCTCB_); \
        _COR_Break(*_pXCPB_);          \
    }

/**
 * @brief   [用户][协程]移除自身
 * @details
 *  **必须在协程块中使用;**
 *  将自身移除;
 *  注意,唤醒源不会被清除,直到下次唤醒;
 */
#define XC_Remove()                                  \
    {                                                \
        XCTask_HandleRemove(_phXCTCB_); /*移除任务*/ \
        _COR_Break(*_pXCPB_);           /*直接跳出*/ \
    }

/************************************************ 我是分割线 ************************************************/
/** 协程块:延时处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]延时n个Tick
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时_n个基础时钟;
 */
#define XC_DelayTick(_n)                              \
    {                                                 \
        XCTask_HandleDelay(_phXCTCB_, _n);            \
        _COR_SetBPBreak(*_pXCPB_); /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]延时us
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时_n个时间;
 *  > 注意:us延时必须要Tick计数在us时基上才能准确的调用;
 */
#define XC_DelayUs(_n)  XC_DelayTick(XCTime_UsToTicks(_n))

/**
 * @brief       [用户][协程]延时ms
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时_n个时间;
 */
#define XC_DelayMs(_n)  XC_DelayTick(XCTime_MsToTicks(_n))

/**
 * @brief       [用户][协程]延时s
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时_n个时间;
 */
#define XC_DelaySec(_n) XC_DelayTick(XCTime_SecToTicks(_n))

/************************************************ 我是分割线 ************************************************/
/** 协程块:任务通知处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]等待通知(单位:系统Tick)
 * @param[in]   _TickTimeout    [XC_Tick_t]超时时间,单位:系统Tick;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;**
 *  任务等待通知;
 *  唤醒后用"XC_CheckNotifyWakeupTimeout"判断是否超时;
 */
#define XC_WaitForNotify(_TickTimeout)                    \
    {                                                     \
        XCTask_HandleWaitNotify(_phXCTCB_, _TickTimeout); \
        _COR_SetBPBreak(*_pXCPB_); /*设置断点并跳出*/     \
    }

/**
 * @brief       [用户][协程]等待通知(单位:ms)
 * @param[in]   _msTimeout  [XC_Tick_t]超时时间,单位:ms;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;**
 *  任务等待通知;
 *  唤醒后用"XC_CheckNotifyWakeupTimeout"判断是否超时;
 */
#define XC_WaitForNotifyMs(_msTimeout) XC_WaitForNotify(XCTime_MsToTicks(_msTimeout))

/**
 * @brief       [用户]清除通知
 * @param[in]   phTCB       [XC_TaskHandle_t]任务控制块
 * @details
 *  **必须在协程块中使用;**
 *  用于清除通知;
 *  可在"等待通知"前调用,防止通知提前到达;
 */
#define XC_ClrNotify()                                         \
    {                                                          \
        _phXCTCB_->NotifyConsumed = _phXCTCB_->NotifyProduced; \
    }

/**
 * @brief       [用户][协程]检查通知唤醒是否超时
 * @return      boot
 * @retval      0 : 没有超时
 * @retval      1 : 超时
 * @details
 *  **必须在协程块中使用;**
 *  用于判断任务通知阻塞唤醒后是否超时;
 *  > 通知函数:XC_WaitForNotify;
 */
#define XC_CheckNotifyWakeupTimeout() (_phXCTCB_->NotifyState != XC_NOTIFY_WAKEUP)

/**
 * @brief       [用户][协程]获取通知的数据(void*)
 * @return      void*   返回通知数据
 * @details
 *  **必须在协程块中使用;**
 *  被通知唤醒后获取通知传递的数据;
 */
#define XC_GetNotifyData()            (_phXCTCB_->pNotifyData)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
