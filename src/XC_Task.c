/**
 * @file        XC_Task.c
 * @brief       任务的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/11/12
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     实现了任务处理相关的操作
 * **********************************************
 *  修改日志
 *  - 见"XC_UpdateInfo.md"的更新说明;
 */
//=== 头文件
#include "XC_Task.h"
#include "XC_Sch.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 基本操作函数 */

/**
 * @brief       [内部]任务基本初始化
 * @param[in]   phTCB   任务控制块
 * @details
 *  用户不调用; \n
 *  "TCB"中以下数据不会被初始化:
 *      - "ListNode" :  链表节点
 *      - "phXCOS" :    任务所属的框架句柄
 *      - "fTask" :     任务入口
 *      - "pParam" :    传递给任务的参数
 *      - "TaskState" : 任务状态
 *  在任务注册,移除,复位,添加时调用
 */
void XC_TaskBasicInit(XCTCB_t* phTCB)
{
    phTCB->TaskWakeTick = ~0;              // 任务下个唤醒的时间
    _COR_Init(phTCB->BP);                  // 初始化断点
    phTCB->pNotifyData = NULL;             // 通知数据清零
    phTCB->Blocked     = _XC_B_NonBlocked; // 没有阻塞
    phTCB->WakeType    = _XC_Wake_Non;     // 没有唤醒

    phTCB->NotifyTrigger   = 0; // 通知触发
    phTCB->NotifyProcessed = 0; // 通知触发处理

    phTCB->StateChangeTrigger   = 0;          // 状态改变触发
    phTCB->StateChangeProcessed = 0;          // 状态改变处理
    phTCB->StateChangeType      = _XC_S_Void; // 最后一次改变的是什么状态(_XC_S_Suspend/_XC_S_Void)
}

/************************************************ 我是分割线 ************************************************/
/** 任务链表节点处理 */

/**
 * @brief       [内部]将任务移动到就绪表
 * @param[in]   phTCB   协程控制块
 * @details
 *  将任务移动到当前就绪节点前,确保最后调用;
 */
void XC_MoveTaskToReadyList(XCTCB_t* phTCB)
{
    // 若移动的是就绪节点,则将就绪节点指向上个节点
    if(phTCB->phXCOS->pReadyNode == &phTCB->ListNode) {
        phTCB->phXCOS->pReadyNode = phTCB->ListNode.pPrev;
    }
    // 将节点移动到就绪节点之前
    XCList_MoveNodeBefore(phTCB->phXCOS->pReadyNode, &phTCB->ListNode);
}

/**
 * @brief       [内部]将任务移动到阻塞表
 * @param[in]   phTCB   协程控制块
 * @details
 *  将任务移动到阻塞表尾部;
 */
void XC_MoveTaskToBlockedList(XCTCB_t* phTCB)
{
    // 若移动的是就绪节点,则将就绪节点指向上个节点
    if(phTCB->phXCOS->pReadyNode == &phTCB->ListNode) {
        phTCB->phXCOS->pReadyNode = phTCB->ListNode.pPrev;
    }
    // 将节点移动到阻塞表尾部(即根节点的上个节点)
    XCList_MoveNodeBefore(&phTCB->phXCOS->BlockedList, &phTCB->ListNode);
}

/**
 * @brief       [内部]将任务插入到时间表
 * @param[in]   pList   需插入的链表(TimeList | TimeOverflowList)
 * @param[in]   phTCB   协程控制块
 * @details
 *  插入的节点需确保纯净,此函数不会处理插入节点的上下的连接; \n
 *  插入时间表按升序排列; \n
 *  从根节点向下(Next)查询"任务下个唤醒的时间"(TaskWakeTick),根据查询值从小到大排列;
 *  > 注意:若查询值相同,新节点插入在旧节点的前面;
 */
void XC_InsertTaskToTimeList(XCListNode_t* pList, XCTCB_t* phTCB)
{
    XCListNode_t* pIterator;
    XCuint_t      MaxTick = ~0; // Tick最大的值

    if(MaxTick == phTCB->TaskWakeTick) {
        pIterator = pList; // 若是唤醒值等于最大值,则迭代器设置为根节点
    }
    else {                                                                    // 查找(小->大)
        pIterator = pList->pNext;                                             // 获取根节点的下个节点地址
        while((pIterator != pList) &&                                         // 指向节点不是根节点
              (((XCTCB_t*)pIterator)->TaskWakeTick <= phTCB->TaskWakeTick)) { // 指向节点的值小于新节点的值
            pIterator = pIterator->pNext;                                     // 指向下个节点
        }
    }
    XCList_InsertNodeBefore(pIterator, &phTCB->ListNode); // 插入节点,在"pIterator"之前
}

/**
 * @brief       [内部]移除任务节点
 * @param[in]   phTCB   协程控制块
 * @return      void
 * @details
 *  从XCOS中删除TCB节点;
 */
void XC_RemoveTaskNode(XCTCB_t* phTCB)
{
    // 若移动的是就绪节点,则将就绪节点指向上个节点
    if(phTCB->phXCOS->pReadyNode == &phTCB->ListNode) {
        phTCB->phXCOS->pReadyNode = phTCB->ListNode.pPrev;
    }
    XCList_Remove(&phTCB->ListNode); // 移除节点
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 任务相关的用户函数 */

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
 *  **不可多次注册同个任务(没有做重复判断,会发生未知错误)** \n
 *  任务注册完成后会挂载到就绪表;
 */
int32_t XC_TaskReg(XCOS_t* phXCOS, XCTCB_t* phTCB, void (*fTask)(XCTCB_t*), void* pParam)
{
    if(phXCOS->TaskNum >= _XC_Cnf_TaskMaxNum) {
        return (_XC_R_Fail);
    }

    XC_TaskBasicInit(phTCB);
    XCList_InitNode(&phTCB->ListNode); // 初始化链表节点

    phTCB->phXCOS    = phXCOS;      // 保存任务的所属框架句柄
    phTCB->fTask     = fTask;       // 更新任务入口
    phTCB->pParam    = pParam;      // 传递给任务的参数
    phTCB->TaskState = _XC_S_Ready; // 任务更新状态为就绪
    phXCOS->TaskNum++;              // 任务数+1

    XCSch_ListLock(phXCOS);          // 链表锁
    XC_InsertTaskToReadyList(phTCB); // 插入就续表
    XCSch_ListUnlock(phXCOS);        // 链表解锁
    return (_XC_R_OK);
}

/**
 * @brief       [用户]任务移除
 * @param[in]   phTCB 协程控制块
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 *  会从链表中删除,并清空TCB数据;
 */
void XC_TaskRemove(XCTCB_t* phTCB)
{
    if(phTCB->phXCOS != NULL) {
        XC_TaskBasicInit(phTCB); // 基本数据初始化

        // 从表中删除任务
        XCSch_ListLock(phTCB->phXCOS);   // 链表锁
        XC_RemoveTaskNode(phTCB);        // 移除任务节点(节点将被初始化)
        phTCB->phXCOS->TaskNum--;        // 任务数-1
        XCSch_ListUnlock(phTCB->phXCOS); // 链表解锁

        phTCB->phXCOS    = NULL;       // 清除任务的所属框架句柄
        phTCB->fTask     = NULL;       // 清除任务入口
        phTCB->pParam    = NULL;       // 清除任务参数
        phTCB->TaskState = _XC_S_Void; // 任务状态改为空
    }
}

/**
 * @brief       [用户]任务复位
 * @param[in]   phTCB   协程控制块
 * @details
 *  **不可在中断中使用(有链表操作)** \n
 *  **不可复位自身(复位自身使用"XC_Reset")** \n
 *  会清除所有状态(包含挂起),任务复位; \n
 *  任务将重置到就绪表,然后从头运行; \n
 *  任务TCB中除了"phXCOS","fTask","Param"其他全部重置;
 */
void XC_TaskReset(XCTCB_t* phTCB)
{
    XC_TaskBasicInit(phTCB);        // 基本数据初始化
    phTCB->TaskState = _XC_S_Ready; // 任务更新状态为就绪

    // 重置到就绪表
    XCSch_ListLock(phTCB->phXCOS);   // 链表锁
    XC_MoveTaskToReadyList(phTCB);   // 任务移动到就绪表
    XCSch_ListUnlock(phTCB->phXCOS); // 链表解锁
}

/************************************************ 我是分割线 ************************************************/
/**
 * 任务分步创建相关的用户函数
 *  "XC_SetTaskEntryPoint"和"XCS_AddTask"函数需要配合使用;
 *  在"XC_SetTaskEntryPoint"设置完成后在调研"XC_AddTask"添加任务;
 *  注意:不要重复添加任务;
 */

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
void XC_SetTaskEntryPoint(XCTCB_t* phTCB, void (*fTask)(XCTCB_t*), void* pParam)
{
    phTCB->fTask  = fTask;  // 更新任务入口
    phTCB->pParam = pParam; // 传递给任务的参数
}

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
 *  添加后挂载到就绪表;
 */
int32_t XC_AddTask(XCOS_t* phXCOS, XCTCB_t* phTCB)
{
    if((phXCOS->TaskNum >= _XC_Cnf_TaskMaxNum) ||
       (phTCB->fTask == NULL)) {
        return (_XC_R_Fail);
    }

    XC_TaskBasicInit(phTCB);
    XCList_InitNode(&phTCB->ListNode); // 初始化链表

    phTCB->phXCOS = phXCOS; // 保存任务的所属框架句柄
    phXCOS->TaskNum++;      // 任务数+1

    XCSch_ListLock(phXCOS);          // 链表锁
    XC_InsertTaskToReadyList(phTCB); // 插入就续表
    XCSch_ListUnlock(phXCOS);        // 链表解锁
    return (_XC_R_OK);
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 任务通知的用户函数 */

/**
 * @brief       [用户]发送通知
 * @param[in]   phTCB           需要发送通知的任务TCB
 * @param[in]   pNotifyData     通知传递的参数
 * @return      int32_t
 * @retval      _XC_R_OK :          通知成功
 * @retval      _XC_R_Continue :    异步操作中
 * @retval      _XC_R_Fail :        任务不是在等待通知
 * @details
 *  **可在中断中调用** \n
 *  发送通知,唤醒任务; \n
 *  多次重复中断: \n
 *      - 异步操作: 参数是最后一次操作的值; \n
 *      - 同步操作: 参数是成功操作的值; \n
 */
int32_t XC_SendNotify(XCTCB_t* phTCB, void* pNotifyData)
{
    /**
     * 只有等待通知的任务才能被通知唤醒;
     */
    if(phTCB->TaskState != _XC_S_WaitNotify) {
        return (_XC_R_Fail);
    }

    /**
     *  若是任务运行中或者有链表操作,则异步操作;
     *  若是此处是"有链表操作",必定是中断调用了;
     *  这里是异步操作
     */
    // if((phTCB == ((XCTCB_t*)(phTCB->phXCOS->pReadyNode))) || // 判断任务是否是就绪节点
    //    (XCSch_GetListLockState(phTCB->phXCOS) == 1)) {       // 判断是否有链表操作
    if(XCSch_GetListLockState(phTCB->phXCOS) == 1) { // 判断是否有链表操作
        phTCB->pNotifyData = pNotifyData;            // 提前保存传递的通知数据
        // 触发异步操作
        phTCB->NotifyTrigger++;            // 通知触发
        phTCB->phXCOS->TaskSchedTrigger++; // 异步调度触发
        return (_XC_R_Continue);
    }

    /** 同步操作,没有运行中且没有操作链表,直接处理 */

    XCSch_ListLock(phTCB->phXCOS);        // 链表锁
    XC_MoveTaskToReadyList(phTCB);        // 移动到就绪表
    phTCB->pNotifyData = pNotifyData;     // 传递的通知数据
    phTCB->WakeType    = _XC_Wake_Notify; // 被通知唤醒
    phTCB->TaskState   = _XC_S_Ready;     // 任务状态:就绪
    XCSch_ListUnlock(phTCB->phXCOS);      // 链表解锁

    return (_XC_R_OK);
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 任务挂起&恢复 */

/**
 * @brief       [用户]任务挂起
 * @param[in]   phTCB   任务控制块
 * @return      int32_t
 * @retval      _XC_R_OK :          挂起成功
 * @retval      _XC_R_Continue :    异步操作中
 * @details
 *  **可在中断中调用** \n
 *  将任务挂起,**本次任务运行完成后暂停任务**; \n
 *  挂起可以覆盖任务的所有阻塞状态,优先级最高; \n
 *  挂起后只能被恢复任务唤醒; \n
 *  任务唤醒后原先的阻塞将失效,并以"_XC_Wake_TaskResume"作为唤醒类型继续运行;
 */
int32_t XC_TaskSuspend(XCTCB_t* phTCB)
{
    /**
     *  挂起操作优先级大于通知处理,在通知处理时可以挂起;
     *  但是要注意,通知状态下挂起并恢复,会导致通知被唤醒,且"获取唤醒超时"为超时;
     */
    if(phTCB->TaskState == _XC_S_Suspend) {
        return (_XC_R_OK); // 已经被挂起
    }

    /**
     *  若是任务运行中或者有链表操作,则异步操作
     *  若是此处是"有链表操作",必定是中断调用了
     */
    // if((phTCB == ((XCTCB_t*)(phTCB->phXCOS->pReadyNode))) || // 判断任务是否在运行中
    //    (XCSch_GetListLockState(phTCB->phXCOS) == 1)) {       // 判断是否有链表操作
    if(XCSch_GetListLockState(phTCB->phXCOS) == 1) { // 判断是否有链表操作
        // 触发异步操作
        phTCB->StateChangeTrigger++;            // 状态改变触发
        phTCB->StateChangeType = _XC_S_Suspend; // 状态改变类型:挂起
        phTCB->phXCOS->TaskSchedTrigger++;      // 异步调度触发
        return (_XC_R_Continue);
    }

    /** 同步操作,没有运行中且没有操作链表,直接处理 */

    XCSch_ListLock(phTCB->phXCOS);    // 链表锁
    phTCB->TaskState = _XC_S_Suspend; // 任务状态:挂起
    XC_MoveTaskToBlockedList(phTCB);  // 将任务移动到阻塞表
    XCSch_ListUnlock(phTCB->phXCOS);  // 链表解锁

    return (_XC_R_OK);
}

/**
 * @brief       [用户]任务挂起恢复
 * @param[in]   phTCB   任务控制块
 * @return      int32_t
 * @retval      _XC_R_OK :          恢复成功
 * @retval      _XC_R_Continue :    异步操作中
 * @details
 *  **可在中断中调用** \n
 *  只能恢复被挂起的任务;
 */
int32_t XC_TaskResume(XCTCB_t* phTCB)
{
    // 任务没有被挂起,直接成功
    if(phTCB->TaskState != _XC_S_Suspend) {
        return (_XC_R_OK); // 没有被挂起
    }

    /**
     *  若是任务运行中或者有链表操作,则异步操作
     *  若是此处是"有链表操作",必定是中断调用了
     */
    // if((phTCB == ((XCTCB_t*)(phTCB->phXCOS->pReadyNode))) || // 判断任务是否在运行中
    //    (XCSch_GetListLockState(phTCB->phXCOS) == 1)) {       // 判断是否有链表操作
    if(XCSch_GetListLockState(phTCB->phXCOS) == 1) { // 判断是否有链表操作
        // 触发异步操作
        phTCB->StateChangeTrigger++;         // 状态改变触发
        phTCB->StateChangeType = _XC_S_Void; // 状态改变类型:未挂起(恢复)
        phTCB->phXCOS->TaskSchedTrigger++;   // 异步调度触发
        return (_XC_R_Continue);
    }

    /** 同步操作,没有运行中且没有操作链表,直接处理 */

    XCSch_ListLock(phTCB->phXCOS);          // 链表锁
    XC_MoveTaskToReadyList(phTCB);          // 将任务移动到就绪表
    phTCB->WakeType  = _XC_Wake_TaskResume; // 被任务恢复唤醒
    phTCB->TaskState = _XC_S_Ready;         // 任务状态:就绪
    XCSch_ListUnlock(phTCB->phXCOS);        // 链表解锁

    return (_XC_R_OK);
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
