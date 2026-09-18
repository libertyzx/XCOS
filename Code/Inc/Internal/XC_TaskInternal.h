/**
 * @file        XC_TaskInternal.h
 * @brief       任务的实现-内部函数
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.1
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     框架任务内部实现的代码
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_TaskInternal_h
#define XC_TaskInternal_h
//=== 头文件
#include "Internal/XC_Core.h"
#include "Internal/XC_List.h"
#include "XC_Type.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 函数声明-协程内部处理函数 */

/**
 * @brief       [内部]等待通知处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details     用于协程内部"等待通知"处理;
 */
void XC_Task_HandleWaitNotify(XC_TaskHandle_t phTCB, uint32_t TickCount);

/**
 * @brief       [内部]延时处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details     用于协程内部"延时"处理;
 */
void XC_Task_HandleDelay(XC_TaskHandle_t phTCB, uint32_t TickCount);

/**
 * @brief       [内部]挂起处理
 * @param[in]   phTCB       任务句柄
 * @details     用于协程内部"挂起自身"和"挂起任务"使用;
 */
void XC_Task_HandleSuspend(XC_TaskHandle_t phTCB);

/**
 * @brief       [内部]复位处理
 * @param[in]   phTCB       任务句柄
 * @details     用于协程内部"复位自身"和"复位任务"使用;
 */
void XC_Task_HandleReset(XC_TaskHandle_t phTCB);

/**
 * @brief       [内部]移除处理
 * @param[in]   phTCB 协程控制块
 * @details     用于协程内部"移除自身"和"任务移除"使用;
 */
void XC_Task_HandleRemove(XC_TaskHandle_t phTCB);

/**
 * @brief       [内部]协程帧入栈
 * @param[in]   phTCB     任务句柄
 * @param[in]   fn        子协程函数入口
 * @param[in]   RetLine   父层返回点(ANSI:行号 / GNU:标签地址)
 * @details     仅由 XC_Cor_Call 调用:保存父层入口+返回点,入口切换为子函数;
 *              越界时复位任务安全降级;
 */
void XC_Task_PushFrame(XC_TaskHandle_t phTCB, XC_CorFn_t fn, XC_BP_t RetLine);

/**
 * @brief       [内部]协程帧出栈
 * @param[in]   phTCB   [XC_TaskHandle_t]任务句柄
 * @details     弹帧恢复父层入口与返回点(调度器同轮切父使用);
 */
void XC_Task_PopFrame(XC_TaskHandle_t phTCB);

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 任务链表节点处理 */

/**
 * @brief       [内部]把"游离任务节点"插入就绪表尾(注册路径专用)
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @details
 *  节点必须**不在任何表中**(新注册的 TCB 满足);
 *  **统一排队尾**(回归 V2.0 语义) ⇒ 新注册任务不插队、严格轮转;
 *  空表时(`pPrev` 指向表根)等价于插表首(即唯一节点);
 */
#define XC_Task_InsertToReadyListTail(phTCB) XC_List_InsertNodeAfter((&(phTCB)->phXCOS->ReadyList)->pPrev, &(phTCB)->ListNode)

/**
 * @brief       [内部]把任务移动到就绪表尾(唤醒/复位/恢复等路径)
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @details
 *  **统一规则** —— "**入就绪表即排队尾**"(回归 V2.0 的单一规则):
 *  - 历史(V2.1.0): 此处插入**表首**, 使"等待中通知 / 定时到点 / 复位 / 恢复"**插队** ——
 *    既与"提前通知(归位表尾)"不一致, 又会在事件率 ≥ 1/调度槽的**过载**下让该任务
 *    **独占调度器**(其它任务与 idle 被饿死);
 *  - 插尾**不做任何优先级补偿** ⇒ 所有任务(含 idle)按轮转公平获得 CPU;
 *  - 语义依据: V2.0 的入表宏(`XCList_MoveNodeAfter(phXCOS->pPrevReadyNode, ...)`)即为"排在
 *    本轮已排队者之后" = 队尾; V2.1.0 调度器重构时锚点由"游标"改为"表根", 语义漂移为表首;
 *  ---
 *  **实现纪律(务必遵守)**: 必须"**先摘除(自环) 再插表尾**", **不可**直接用
 *  `XC_List_MoveNodeAfter(ReadyList.pPrev, node)` —— 当该节点**恰为就绪表尾节点**(或仍在
 *  表中)时, "移动到'自己'之后"会**自引用破坏链表**(实测: 表根变自环 ⇒ `ListValid` 假);
 *  先 `XC_List_Remove` 再取 `pPrev` 可覆盖全部情形:
 *  - 游离(调度器摘出的运行中任务): 同上, Remove 是空操作(自环);
 *  - 已在就绪表(如 `XC_Task_Reset` 一个 READY 任务) / 恰为表尾: 先摘除 ⇒ `pPrev` 转为前驱 ⇒ 结果仍为表尾;
 *  - 节点在阻塞/时间表(唤醒路径): 摘除后插尾 ⇒ 队列正确;
 */
#define XC_Task_MoveToReadyListTail(phTCB)        \
    do {                                          \
        XC_List_Remove(&(phTCB)->ListNode);       \
        XC_List_InsertNodeAfter(                  \
            (&(phTCB)->phXCOS->ReadyList)->pPrev, \
            &(phTCB)->ListNode);                  \
    } while(0)

/**
 * @brief       [内部]将任务移动到阻塞表
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @details     将任务移动到阻塞表头
 */
#define XC_Task_MoveToBlockedList(phTCB) XC_List_MoveNodeAfter(&(phTCB)->phXCOS->BlockedList, &(phTCB)->ListNode)

/**
 * @brief       [内部]移除任务节点
 * @param[in]   phTCB   [XC_TaskHandle_t]协程控制块
 * @return      void
 * @details     从XCOS中删除TCB节点;
 */
#define XC_Task_RemoveNode(phTCB)        XC_List_Remove(&(phTCB)->ListNode)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
