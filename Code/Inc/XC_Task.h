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
 * @return      XC_Return_t
 * @retval      XC_OK :      注册成功
 * @retval      XC_FAIL :    注册失败,任务太多
 * @details
 *  注册一个任务;
 *  **不可多次注册同个任务(没有做重复判断,会发生未知情况)**
 */
XC_Return_t XC_Task_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam);

/**
 * @brief       [用户]任务移除
 * @param[in]   phTCB 协程控制块
 * @details     移除一个任务
 */
void XC_Task_Remove(XC_TaskHandle_t phTCB);

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
void XC_Task_SetEntry(XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam);

/**
 * @brief   [用户]添加任务
 * @param   phXCOS  框架句柄
 * @param   phTCB   协程任务控制块
 * @return  XC_Return_t
 * @retval  XC_OK :      注册成功
 * @retval  XC_FAIL :    注册失败,任务太多
 * @details
 *  添加的任务必须先调用"XCTask_SetEntry";
 *  设置好任务入口和传递的参数才可添加;
 */
XC_Return_t XC_Task_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明-[用户]复位 | 挂起 | 挂起恢复 */

/**
 * @brief       [用户]任务复位
 * @param[in]   phTCB   协程控制块
 * @details
 *  服务一个任务;
 *  **不可复位自身(复位自身使用"XC_Reset")**
 */
void XC_Task_Reset(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]任务挂起
 * @param[in]   phTCB   任务控制块
 * @return      XC_Return_t
 * @retval      XC_OK :          挂起成功
 * @retval      XC_FAIL:         失败(任务不存在)
 * @retval      XC_CONTINUE :    异步操作中
 * @details     将任务挂起,本次任务运行完成后暂停任务;
 */
XC_Return_t XC_Task_Suspend(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]任务挂起恢复
 * @param[in]   phTCB   任务控制块
 * @return      XC_Return_t
 * @retval      XC_OK :          恢复成功
 * @retval      XC_FAIL:         失败(任务未挂起)
 * @details     只能恢复被挂起的任务;
 */
XC_Return_t XC_Task_Resume(XC_TaskHandle_t phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明/函数宏-[用户]通知相关 */

/**
 * @brief       [用户]发送通知
 * @param[in]   phTCB           需要发送通知的任务TCB
 * @param[in]   pNotifyData     通知传递的参数
 * @return      XC_Return_t
 * @retval      XC_OK :          通知成功
 * @retval      XC_FAIL :        任务不存在或者任务被挂起
 * @retval      XC_CONTINUE :    异步操作中
 * @details
 *  **线程安全(SPSC)**
 *  发送通知,唤醒任务;
 */
XC_Return_t XC_Task_SendNotify(XC_TaskHandle_t phTCB, void* pNotifyData);

/**
 * @brief       [用户]清除通知
 * @param[in]   phTCB       [XC_TaskHandle_t]任务控制块
 * @details
 *  用于清除通知;
 *  可在"等待通知"前调用,防止通知提前到达;
 */
#define XC_Task_ClrNotify(_phTCB_)                             \
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
#define XC_Task_UpdateNotifyData(_phTCB_, _pNotifyData) \
    {                                                   \
        (_phTCB_)->pNotifyData = (_pNotifyData);        \
    }

/**
 * @brief       [用户]读取通知数据(void*)
 * @param[in]   _phTCB_  [XC_TaskHandle_t]任务控制块
 * @return      void*   返回通知数据
 * @details
 *  在不使用任务通知的时候,可以用通知数据来传递数据;
 *  此函数用于读取通知数据;
 */
#define XC_Task_ReadNotifyData(_phTCB_) ((_phTCB_)->pNotifyData)

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
#define XC_Task_GetParam(_phTCB_)       ((_phTCB_)->pParam)

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
#define XC_Task_GetState(_phTCB_)       ((XC_TaskState_t)((_phTCB_)->TaskState))

/************************************************ 我是分割线 ************************************************/
//=== 向后兼容:保留旧版函数/宏名(已弃用,建议迁移到 XC_Task_ 前缀)
#define XCTask_Reg                      XC_Task_Reg
#define XCTask_Remove                   XC_Task_Remove
#define XCTask_SetEntry                 XC_Task_SetEntry
#define XCTask_Add                      XC_Task_Add
#define XCTask_Reset                    XC_Task_Reset
#define XCTask_Suspend                  XC_Task_Suspend
#define XCTask_Resume                   XC_Task_Resume
#define XCTask_SendNotify               XC_Task_SendNotify
#define XCTask_ClrNotify                XC_Task_ClrNotify
#define XCTask_UpdateNotifyData         XC_Task_UpdateNotifyData
#define XCTask_ReadNotifyData           XC_Task_ReadNotifyData
#define XCTask_GetParam                 XC_Task_GetParam
#define XCTask_GetState                 XC_Task_GetState

//=== 文件结束
#endif