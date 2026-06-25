/**
 * @file        XC_Cor.h
 * @brief       协程块实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/06/25
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  协程块（Coroutine Block）是 XCOS 的核心执行模型；
 *  所有协程控制宏必须在 `XC_Cor_Enter` / `XC_Cor_Leave` 块中使用；
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XCCOR_H_
#define XCCOR_H_
//=== 头文件
#include "XC_Task.h"
#include "XC_Type.h"

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
 *  必须搭配"XC_Cor_Leave"使用;
 */
#define XC_Cor_Enter(_phTCB_)                                    \
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
 *  必须搭配"XC_Cor_Enter"使用;
 */
#define XC_Cor_Leave()   \
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
 *  > 所以若是要使用传递的参数需要再"XC_Cor_Enter"前将参数赋值给变量;
 */
#define XC_Cor_GetParam() (_phXCTCB_->pParam)

/**
 * @brief       [用户][协程]让出控制
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,跳出协程,下次调度后从此处继续处理;
 */
#define XC_Cor_Yield()                                           \
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
#define XC_Cor_Suspend()                                     \
    {                                                        \
        XC_Task_HandleSuspend(_phXCTCB_); /*任务挂起*/       \
        _COR_SetBPBreak(*_pXCPB_);        /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]任务复位
 * @details
 *  **必须在协程块中使用;**
 *  复位当前在运行的任务;运行此宏后将立刻退出协程块
 */
#define XC_Cor_Reset()                  \
    {                                   \
        XC_Task_HandleReset(_phXCTCB_); \
        _COR_Break(*_pXCPB_);           \
    }

/**
 * @brief   [用户][协程]移除自身
 * @details
 *  **必须在协程块中使用;**
 *  将自身移除;
 *  注意,唤醒源不会被清除,直到下次唤醒;
 */
#define XC_Cor_Remove()                               \
    {                                                 \
        XC_Task_HandleRemove(_phXCTCB_); /*移除任务*/ \
        _COR_Break(*_pXCPB_);            /*直接跳出*/ \
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
#define XC_Cor_DelayTick(_n)                          \
    {                                                 \
        XC_Task_HandleDelay(_phXCTCB_, _n);           \
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
#define XC_Cor_DelayUs(_n)  XC_Cor_DelayTick(XC_Time_UsToTicks(_n))

/**
 * @brief       [用户][协程]延时ms
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时_n个时间;
 */
#define XC_Cor_DelayMs(_n)  XC_Cor_DelayTick(XC_Time_MsToTicks(_n))

/**
 * @brief       [用户][协程]延时s
 * @param[in]   _n  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时_n个时间;
 */
#define XC_Cor_DelaySec(_n) XC_Cor_DelayTick(XC_Time_SecToTicks(_n))

/************************************************ 我是分割线 ************************************************/
/** 协程块:任务通知处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]等待通知(单位:系统Tick)
 * @param[in]   _TickTimeout    [XC_Tick_t]超时时间,单位:系统Tick;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;**
 *  任务等待通知;
 *  唤醒后用"XC_Cor_CheckNotifyWakeupTimeout"判断是否超时;
 */
#define XC_Cor_WaitNotify(_TickTimeout)                    \
    {                                                      \
        XC_Task_HandleWaitNotify(_phXCTCB_, _TickTimeout); \
        _COR_SetBPBreak(*_pXCPB_); /*设置断点并跳出*/      \
    }

/**
 * @brief       [用户][协程]等待通知(单位:ms)
 * @param[in]   _msTimeout  [XC_Tick_t]超时时间,单位:ms;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;**
 *  任务等待通知;
 *  唤醒后用"XC_Cor_CheckNotifyWakeupTimeout"判断是否超时;
 */
#define XC_Cor_WaitNotifyMs(_msTimeout) XC_Cor_WaitNotify(XC_Time_MsToTicks(_msTimeout))

/**
 * @brief       [用户]清除通知
 * @param[in]   phTCB       [XC_TaskHandle_t]任务控制块
 * @details
 *  **必须在协程块中使用;**
 *  用于清除通知;
 *  可在"等待通知"前调用,防止通知提前到达;
 */
#define XC_Cor_ClrNotify()                                     \
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
 *  > 通知函数:XC_Cor_WaitNotify;
 */
#define XC_Cor_IsNotifyTimeout()    (_phXCTCB_->NotifyState != XC_NOTIFY_WAKEUP)

/**
 * @brief       [用户][协程]获取通知的数据(void*)
 * @return      void*   返回通知数据
 * @details
 *  **必须在协程块中使用;**
 *  被通知唤醒后获取通知传递的数据;
 */
#define XC_Cor_GetNotifyData()      (_phXCTCB_->pNotifyData)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 向后兼容:保留旧版宏名(已弃用,建议迁移到 XC_Cor_ 前缀)
#define XC_Enter                    XC_Cor_Enter
#define XC_Leave                    XC_Cor_Leave
#define XC_GetParam                 XC_Cor_GetParam
#define XC_Yield                    XC_Cor_Yield
#define XC_Suspend                  XC_Cor_Suspend
#define XC_Reset                    XC_Cor_Reset
#define XC_Remove                   XC_Cor_Remove
#define XC_DelayTick                XC_Cor_DelayTick
#define XC_DelayUs                  XC_Cor_DelayUs
#define XC_DelayMs                  XC_Cor_DelayMs
#define XC_DelaySec                 XC_Cor_DelaySec
#define XC_WaitForNotify            XC_Cor_WaitNotify
#define XC_WaitForNotifyMs          XC_Cor_WaitNotifyMs
#define XC_ClrNotify                XC_Cor_ClrNotify
#define XC_CheckNotifyWakeupTimeout XC_Cor_IsNotifyTimeout
#define XC_GetNotifyData            XC_Cor_GetNotifyData

//=== 文件结束
#endif