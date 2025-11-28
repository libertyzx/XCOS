/**
 * @file        XC_Task.h
 * @brief       任务的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/11/26
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
#include "XC_Type.h"

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
 * @return      XC_Retuen_t
 * @retval      XC_OK :      注册成功
 * @retval      XC_FAIL :    注册失败,任务太多
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 *  **不可多次注册同个任务(没有做重复判断,会发生未知情况)** \n
 */
XC_Retuen_t XCTask_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam);

/**
 * @brief       [用户]任务移除
 * @param[in]   phTCB 协程控制块
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 */
void XCTask_Remove(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]任务复位
 * @param[in]   phTCB   协程控制块
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 *  **不可复位自身(复位自身使用"XC_Reset")** \n
 */
void XCTask_Reset(XC_TaskHandle_t phTCB);

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
 *  **不可在中断中使用(有链表操作)** \n
 *  添加的任务必须先调用"XCTask_SetEntry"; \n
 *  设置好任务入口和传递的参数才可添加; \n
 */
XC_Retuen_t XCTask_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明-[用户]通知 | 挂起 | 挂起恢复 */

/**
 * @brief       [用户]发送通知(void*)
 * @param[in]   phTCB       任务控制块
 * @param[in]   pNotifyData 通知传递的数据(void*)类型
 * @return      XC_Retuen_t
 * @retval      XC_OK :          通知成功
 * @retval      XC_CONTINUE :    任务已经唤醒(已在就绪表)
 * @retval      XC_FAIL :        任务被挂起
 * @details     发送通知,唤醒任务;
 */
XC_Retuen_t XCTask_NotifySend(XC_TaskHandle_t phTCB, void* pNotifyData);

/**
 * @brief       [用户]任务挂起
 * @param[in]   phTCB   任务控制块
 * @return      XC_Retuen_t
 * @retval      XC_OK :          挂起成功
 * @retval      XC_CONTINUE :    已经被挂起
 * @details
 *  **可在中断中调用** \n
 *  将任务挂起,本次任务运行完成后暂停任务; \n
 */
XC_Retuen_t XCTask_Suspend(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]任务挂起恢复
 * @param[in]   phTCB   任务控制块
 * @return      XC_Retuen_t
 * @retval      XC_OK :          恢复成功
 * @retval      XC_CONTINUE :    任务没有挂起
 * @details
 *  **可在中断中调用** \n
 *  只能恢复被挂起的任务;
 */
XC_Retuen_t XCTask_Resume(XC_TaskHandle_t phTCB);

/************************************************ 我是分割线 ************************************************/
/** 以下是函数宏 */

/**
 * @brief       [用户]获取任务注册时传递的参数(void*)
 * @return      void*   返回空指针类型数据;
 * @details
 *  **任意位置可调用;** \n
 *  用来获取任务参数;
 *  > 注意: 因为协程内上下文切换局部变量是不保存的,
 *  > 所以若是要使用传递的参数需要再"XC_Enter"前将参数赋值给变量;
 */
#define XCTask_GetParam(_phTCB_) ((_phTCB_)->pParam)

/**
 * @brief       [用户]获取任务运行状态
 * @param[in]   _phTCB_  [XC_TaskHandle_t]任务控制块
 * @return      XC_TaskState_t
 * @retval      XC_TASK_VOID :          空,被移除后的状态
 * @retval      XC_TASK_RUN :           运行
 * @retval      XC_TASK_READY :         就绪(注册后的状态)
 * @retval      XC_TASK_DELAY :         [阻塞]延时
 * @retval      XC_TASK_WAIT_NOTIFY :   [阻塞]等待通知
 * @retval      XC_TASK_SUSPEND :       [阻塞]挂起
 * @details     说明
 *  **任意位置可调用;** \n
 *  获取任务运行的状态;
 */
#define XCTask_GetState(_phTCB_) ((_phTCB_)->TaskState)

/**
 * @brief       [用户]更新通知数据(void*)
 * @param[in]   _phTCB_          [XC_TaskHandle_t]任务控制块
 * @param[in]   _pNotifyData    [void*]通知数据
 * @details
 *  **任意位置可调用;** \n
 *  在不使用任务通知的时候,可以用通知数据来传递数据; \n
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
 *  **任意位置可调用;** \n
 *  在不使用任务通知的时候,可以用通知数据来传递数据; \n
 *  此函数用于读取通知数据;
 */
#define XCTask_ReadNotifyData(_phTCB_) ((_phTCB_)->pNotifyData)

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
 *  **协程块的开始;** \n
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
#define XC_Yield()                                               \
    {                                                            \
        _phXCTCB_->TaskState = XC_TASK_READY; /*任务状态:就绪*/  \
        _COR_SetBPBreak(*_pXCPB_)             /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]任务复位
 * @details
 *  **必须在协程块中使用;** \n
 *  复位当前在运行的任务;运行此宏后将立刻退出协程块
 */
#define XC_Reset()               \
    {                            \
        XCTask_Reset(_phXCTCB_); \
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
        XCTask_Suspend(_phXCTCB_); /*任务挂起*/       \
        _COR_SetBPBreak(*_pXCPB_); /*设置断点并跳出*/ \
    }

/**
 * @brief   [用户][协程]获取唤醒类型
 * @return  uint8_t   唤醒的类型,返回"XC_WakeType_t"类型数据;
 * @details
 *  必须在协程块中使用; \n
 *  在任务等待通知,延时,挂起等操作唤醒后确定唤醒源是什么; \n
 *  注意,唤醒源不会被清除,直到下次唤醒;
 */
#define XC_GetWakeType() (_phXCTCB_->WakeType)

/**
 * @brief   [用户][协程]移除自身
 * @details
 *  必须在协程块中使用; \n
 *  将自身移除; \n
 *  注意,唤醒源不会被清除,直到下次唤醒;
 */
#define XC_Remove()                            \
    {                                          \
        XCTask_Remove(_phXCTCB_); /*移除任务*/ \
        _COR_Break(*_pXCPB_);     /*直接跳出*/ \
    }

/************************************************ 我是分割线 ************************************************/
/** 协程块:延时处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]延时n个Tick
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,延时_n个基础时钟;
 */
#define XC_DelayTick(_n)                                            \
    {                                                               \
        _phXCTCB_->TaskState    = XC_TASK_DELAY; /*任务状态:延时*/  \
        _phXCTCB_->WakeType     = XC_WAKE_NONE;  /*清唤醒类型*/     \
        _phXCTCB_->TaskWakeTick = (_n);          /*保存时间*/       \
        _COR_SetBPBreak(*_pXCPB_);               /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]延时us
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,延时_n个时间; \n
 *  > 注意:us延时必须要Tick计数在us时基上才能准确的调用;
 */
#define XC_DelayUs(_n)  XC_DelayTick(XCTime_UsToTicks(_n))

/**
 * @brief       [用户][协程]延时ms
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,延时_n个时间; \n
 */
#define XC_DelayMs(_n)  XC_DelayTick(XCTime_MsToTicks(_n))

/**
 * @brief       [用户][协程]延时s
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;** \n
 *  让出CPU的使用权,延时_n个时间; \n
 */
#define XC_DelaySec(_n) XC_DelayTick(XCTime_SecToTicks(_n))

/************************************************ 我是分割线 ************************************************/
/** 协程块:任务通知处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]等待通知(单位:系统Tick)
 * @param[in]   _TickTimeout    [XC_Tick_t]超时时间,单位:系统Tick;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;** \n
 *  任务等待通知; \n
 *  唤醒后用"XC_GetNotifyWakeState"判断是否超时;
 *  注意:"TaskState"任务状态参数需要最先改变,防止出现临界段;
 */
#define XC_WaitNotify(_TickTimeout)                                          \
    {                                                                        \
        _phXCTCB_->TaskState    = XC_TASK_WAIT_NOTIFY; /*任务状态:等待通知*/ \
        _phXCTCB_->WakeType     = XC_WAKE_NONE;        /*唤醒类型:无唤醒*/   \
        _phXCTCB_->TaskWakeTick = (_TickTimeout);      /*超时的时间*/        \
        _COR_SetBPBreak(*_pXCPB_);                     /*设置断点并跳出*/    \
    }

/**
 * @brief       [用户][协程]等待通知(单位:ms)
 * @param[in]   _msTimeout  [XC_Tick_t]超时时间,单位:ms;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;** \n
 *  任务等待通知; \n
 *  唤醒后用"XC_GetNotifyWakeState"判断是否超时;
 */
#define XC_WaitNotifyMs(_msTimeout) XC_WaitNotify(XCTime_MsToTicks(_msTimeout))

/**
 * @brief       [用户][协程]获取通知唤醒状态
 * @return      boot
 * @retval      0 : 没有超时
 * @retval      1 : 超时
 * @details
 *  **必须在协程块中使用;** \n
 *  用于判断任务通知阻塞唤醒后是否超时;
 *  > 通知函数:XC_WaitNotify
 */
#define XC_GetNotifyWakeState()     (_phXCTCB_->WakeType != XC_WAKE_NOTIFY)

/**
 * @brief       [用户][协程]获取通知的数据(void*)
 * @return      void*   返回通知数据
 * @details
 *  **必须在协程块中使用;** \n
 *  被通知唤醒后获取通知传递的数据;
 */
#define XC_GetNotifyData()          (_phXCTCB_->pNotifyData)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
