/**
 * @file        XC_Task.h
 * @brief       任务的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
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
#ifndef XC_Task_h
#define XC_Task_h
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
 * @retval      XC_FAIL :    注册失败(任务太多, 或该任务已注册)
 * @details
 *  注册一个任务;
 *  **同一任务只能注册一次**: 重复注册返回 XC_FAIL (移除后可再次注册)
 *  **前置契约(违背 = 未定义行为)**: `phXCOS` / `phTCB` / `fTask` 均须非空 —— 属"编程错误"，
 *  按"单一强制点"规则只由断言上报(DEBUG 档 `XC_DIAG_ASSERT`)，**发布档不检查、不返回错误**(0 成本)；
 *  边界规则见 `Docs/架构概览/XCOS_V2.1.0_架构概览.md`「断言 vs 返回码：边界规则」§11.6;
 */
XC_Return_t XC_Task_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam);

/**
 * @brief       [用户]任务注册(扩展:支持协程嵌套)
 * @param[in]   phXCOS        框架句柄
 * @param[in]   phTCB         协程任务控制块
 * @param[in]   fTask         任务的函数指针(任务入口)
 * @param[in]   pParam        传递给任务的参数
 * @param[in]   pCorStack     协程帧栈(用户按需提供;无嵌套传NULL)
 * @param[in]   CorDepthMax   帧栈容量(最大嵌套层数;无嵌套传0)
 * @return      XC_Return_t
 * @retval      XC_OK :      注册成功
 * @retval      XC_FAIL :    注册失败(任务太多, 或该任务已注册)—— 同 `XC_Task_Reg`
 * @details
 *  在"XC_Task_Reg"基础上增加协程帧栈配置,等价于"XC_Task_Reg"+"XC_Task_SetCorStack";
 *  "pCorStack"/"CorDepthMax"必须同为有/同为无(都传NULL/0表示无嵌套任务);
 *  需要嵌套的任务必须用本函数或"XC_Task_SetCorStack"配置帧栈;
 *  **前置契约(违背 = 未定义行为)**: 入参非空 + "帧栈/容量同为有或同为无"(DEBUG 档由断言上报, 发布档不检查);
 */
XC_Return_t XC_Task_RegExt(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, XC_CorFn_t fTask, void* pParam, XC_CorFrame_t* pCorStack, uint8_t CorDepthMax);

/**
 * @brief       [用户]设置协程帧栈(支持/撤销协程嵌套能力)
 * @param[in]   phTCB         协程任务控制块
 * @param[in]   pCorStack     协程帧栈(用户按需提供;撤销嵌套能力传NULL)
 * @param[in]   CorDepthMax   帧栈容量(最大嵌套层数;撤销传0)
 * @return      XC_Return_t
 * @retval      XC_OK :      设置成功(当前实现**恒返回 XC_OK**)
 * @details
 *  "pCorStack"/"CorDepthMax"必须同为有/同为无;
 *  传"(NULL, 0U)"撤销嵌套能力;重新设置会清空当前深度("CorDepth=0");
 *  任务嵌套中更换配置前应先"XC_Task_Reset"/"XC_Cor_Reset"清栈;
 *  **前置契约(违背 = 未定义行为)**: 入参非空 + "帧栈/容量同为有或同为无"
 *  (属"编程错误" ⇒ 只由断言上报, 发布档不检查、不返回错误);
 */
XC_Return_t XC_Task_SetCorStack(XC_TaskHandle_t phTCB, XC_CorFrame_t* pCorStack, uint8_t CorDepthMax);

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
 *  一般配合"XC_Task_Add"使用(分步注册);
 *  **嵌套中换入口**: 会自动"清栈 + 重置断点", 语义 = **丢弃当前嵌套层, 按新入口从头运行**
 *  (否则栈底帧 pCorStack[0].pfn 会与新入口不一致, 复位/移除时把 fTask 恢复成旧入口);
 */
void XC_Task_SetEntry(XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam);

/**
 * @brief   [用户]添加任务
 * @param   phXCOS  框架句柄
 * @param   phTCB   协程任务控制块
 * @return  XC_Return_t
 * @retval  XC_OK :      注册成功
 * @retval  XC_FAIL :    注册失败(任务太多, 或该任务已注册)
 * @details
 *  添加的任务必须先调用"XC_Task_SetEntry";
 *  设置好任务入口和传递的参数才可添加;
 *  **前置契约(违背 = 未定义行为)**: `phXCOS`/`phTCB` 非空, 且已用 `XC_Task_SetEntry` 配好非空入口
 *  (DEBUG 档由断言上报, 发布档不检查);
 */
XC_Return_t XC_Task_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB);

/************************************************ 我是分割线 ************************************************/
/** 函数声明-[用户]复位 | 挂起 | 挂起恢复 */

/**
 * @brief       [用户]任务复位
 * @param[in]   phTCB   协程控制块
 * @details
 *  服务一个任务;
 *  **不可复位自身(复位自身使用"XC_Cor_Reset")**
 */
void XC_Task_Reset(XC_TaskHandle_t phTCB);

/**
 * @brief       [用户]任务挂起
 * @param[in]   phTCB   任务控制块
 * @return      XC_Return_t
 * @retval      XC_OK :          挂起成功
 * @retval      XC_FAIL:         失败(任务不存在)
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
 *  用于清除通知(把"通知-待处理标志"清0);
 *  可在"等待通知"前调用,防止通知提前到达;
 *  **调用上下文**:本操作写的是"消费侧"字段,应在主循环(目标任务自身)调用,
 *  禁止在中断中调用(否则破坏单生产者单消费者前提);
 */
#define XC_Task_ClrNotify(phTCB)     \
    do {                             \
        (phTCB)->NotifyPending = 0U; \
    } while(0)

/**
 * @brief       [用户]更新通知数据(void*)
 * @param[in]   phTCB         [XC_TaskHandle_t]任务控制块
 * @param[in]   _pNotifyData    [void*]通知数据
 * @details
 *  在不使用任务通知的时候,可以用通知数据来传递数据;
 *  此函数用于更新(写)通知数据;
 */
#define XC_Task_UpdateNotifyData(phTCB, _pNotifyData) \
    do {                                              \
        (phTCB)->pNotifyData = (_pNotifyData);        \
    } while(0)

/**
 * @brief       [用户]读取通知数据(void*)
 * @param[in]   phTCB  [XC_TaskHandle_t]任务控制块
 * @return      void*   返回通知数据
 * @details
 *  在不使用任务通知的时候,可以用通知数据来传递数据;
 *  此函数用于读取通知数据;
 */
#define XC_Task_ReadNotifyData(phTCB) ((phTCB)->pNotifyData)

/************************************************ 我是分割线 ************************************************/
/** 函数宏-[用户]参数 | 状态 */

/**
 * @brief       [用户]获取任务注册时传递的参数(void*)
 * @return      void*   返回空指针类型数据;
 * @details
 *  用来获取任务参数;
 *  > 注意: 因为协程内上下文切换局部变量是不保存的,
 *  > 所以若是要使用传递的参数需要再"XC_Cor_Enter"前将参数赋值给变量;
 */
#define XC_Task_GetParam(phTCB)       ((phTCB)->pParam)

/**
 * @brief       [用户]获取任务运行状态
 * @param[in]   phTCB  [XC_TaskHandle_t]任务控制块
 * @return      XC_TaskState_t
 * @retval      XC_TASK_VOID :          空(任务创建前或被移除后的状态)
 * @retval      XC_TASK_RUN :           运行(正在运行的任务)
 * @retval      XC_TASK_READY :         就绪(在就绪表中的任务状态,任务注册后为就绪)
 * @retval      XC_TASK_BLOCKED :       阻塞(延时,等待通知后的状态)
 * @retval      XC_TASK_SUSPEND :       挂起
 * @details     获取任务运行的状态;
 */
#define XC_Task_GetState(phTCB)       ((XC_TaskState_t)((phTCB)->TaskState))

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
