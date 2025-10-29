/**
 * @file        XC_Task.c
 * @brief       任务的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/10/20
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     实现了任务处理相关的操作
 * **********************************************
 *  修改日志
 *  - 2024/03/18
 *      - 初始编写
 *  - 2024/04/09
 *      - 版本:1.01
 *      - "XC_SendNotify"函数形参通知数据改为"void*"指针;
 *      - "XC_TaskBasicInit"函数增加传入形参初始化;
 *  - 2025/10/20
 *      - 见"XC_UpdateInfo.md"的"T2025/10/20"更新说明;
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
    phTCB->Lock        = _XC_Lock_Unlock;  // 没有锁
}

/************************************************ 我是分割线 ************************************************/
/** 任务链表节点处理 */

/**
 * @brief       [内部]链表节点移除
 * @param[in]   phTCB   协程控制块
 * @return      void
 * @details
 *  从XCOS中删除TCB节点;
 *  > 注意:若是删除的节点是当前就绪节点,则就绪节点将切换成上个节点;
 */
void XC_ListNodeRemove(XCTCB_t* phTCB)
{
    // 若移除的是索引指向的节点,则将索引指向上个节点
    if(phTCB->phXCOS->pReadyListNodeIndex == &phTCB->ListNode) {
        phTCB->phXCOS->pReadyListNodeIndex = phTCB->ListNode.pPrevious;
    }
    XCList_Remove(&phTCB->ListNode); // 移除节点
}

/**
 * @brief       [内部]将节点插入索引前
 * @param[in]   phTCB   协程控制块
 * @details
 *  将节点插入当前就绪节点前,节点在就绪表;
 *  > 注意:索引指向的是就绪表,所以节点是插入就绪表的;
 */
void XC_ListNodeInsertIndexPrevious(XCTCB_t* phTCB)
{
    XCList_InsertNodePrevious(phTCB->phXCOS->pReadyListNodeIndex, &phTCB->ListNode); // 插入索引节点前
    phTCB->ListNode.pRootList = &phTCB->phXCOS->ReadyList;                           // 插在就绪表
}

/**
 * @brief       [内部]升序排列的插入节点
 * @param[in]   pList   需插入的链表
 * @param[in]   phTCB   协程控制块
 * @details
 *  从根节点向下(Next)查询"任务下个唤醒的时间"(TaskWakeTick),根据查询值从小到大排列;
 *  > 注意:若查询值相同,新节点插入在旧节点的前面;
 */
void XC_ListNodeInsertAsc(XCListRoot_t* pList, XCTCB_t* phTCB)
{
    XCListNode_t* pIterator;
    XCuint_t      MaxTick = ~0; // Tick最大的值

    if(MaxTick == phTCB->TaskWakeTick) {
        // 若是唤醒值等于最大值,则迭代器设置为根节点
        pIterator = (XCListNode_t*)&pList->RootNode;
    }
    else {
        // 查找(小->大)
        pIterator = pList->RootNode.pNext; // 获取根节点的下个节点地址
        while((pIterator != (XCListNode_t*)&pList->RootNode) && (((XCTCB_t*)pIterator)->TaskWakeTick <= phTCB->TaskWakeTick)) {
            // 指向节点不是根节点 && 指向节点的值小于新节点的值;
            pIterator = pIterator->pNext; // 指向下个节点
        }
    }
    XCList_InsertNodePrevious(pIterator, (XCListNode_t*)phTCB); // 插入节点,在"pIterator"之前
    phTCB->ListNode.pRootList = pList;                          // 保存根节点
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

    XCSch_ListOperationStart(phXCOS);      // 链表操作开始
    XC_ListNodeInsertIndexPrevious(phTCB); // 插入就续表
    XCSch_ListOperationEnd(phXCOS);        // 链表操作结束
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
        XCSch_ListOperationStart(phTCB->phXCOS); // 链表操作开始
        XC_ListNodeRemove(phTCB);
        phTCB->phXCOS->TaskNum--;              // 任务数-1
        XCSch_ListOperationEnd(phTCB->phXCOS); // 链表操作结束

        XCList_InitNode(&phTCB->ListNode); // 初始化链表
        phTCB->phXCOS    = NULL;           // 清除任务的所属框架句柄
        phTCB->fTask     = NULL;           // 清除任务入口
        phTCB->pParam    = NULL;           // 清除任务参数
        phTCB->TaskState = _XC_S_Void;     // 任务状态改为空
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
    XCSch_ListOperationStart(phTCB->phXCOS); // 链表操作开始
    XC_ListNodeRemove(phTCB);                // 原链表移除节点
    XC_ListNodeInsertIndexPrevious(phTCB);   // 重新插入就绪表
    XCSch_ListOperationEnd(phTCB->phXCOS);   // 链表操作结束
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

    XCSch_ListOperationStart(phXCOS);      // 链表操作开始
    XC_ListNodeInsertIndexPrevious(phTCB); // 插入就续表
    XCSch_ListOperationEnd(phXCOS);        // 链表操作结束
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
 *  发送通知,唤醒任务; \n
 *  只有在任务成功被通知(_XC_R_OK)时,传递的数据才有效; \n
 *  注意:通知,挂起,解除挂起,锁是同个;
 */
int32_t XC_SendNotify(XCTCB_t* phTCB, void* pNotifyData)
{
// 配置框架中断支持
#if (_XC_Cnf_IntSupport == 1)

    /**
     *  发送通知只会唤醒等待通知的任务,也就是状态为"_XC_S_WaitNotify"的任务;
     *  非"_XC_S_WaitNotify"的任务会直接返回失败(_XC_R_Fail);
     *  目前状态会被任务挂起所更新,更新为"_XC_S_Suspend";
     */
    if(phTCB->TaskState != _XC_S_WaitNotify) {
        return (_XC_R_Fail);
    }

    /**
     *  锁定处理
     *  这里锁处理,保证在唤醒时中断或多次中断嵌套中只有一次操作是有效的;
     *  这里的逻辑是:
     *      - Lock 赋值或判断时会在任意位置中断;
     *      - Lock 初值必须是"_XC_Lock_WaitLock"也就是"等待唤醒"开始,不然就作为无效处理直接跳出;
     *      - Lock 等于 _XC_Lock_WaitLock 在 Lock++ 后值为 _XC_Lock_Lock;
     *      - 在以下2个if和累加中任意时刻被中断(包括多次中断嵌套),
     *          在进入累加处理前Lock必定等于"_XC_Lock_WaitLock";
     *          若是在累加处被中断(或多次嵌套中断)必定有一个(且只有一个)会得到Lock == "_XC_Lock_Lock";
     *          在此函数运行完成后,Lock只会是"_XC_Lock_Unlock"或者大于"_XC_Lock_WaitUnlock",必定不等于"_XC_Lock_Lock";
     *          所以以下语句得到在多个嵌套处理时,此函数只会运行一次;
     */
    if(phTCB->Lock != _XC_Lock_WaitLock) {
        return (_XC_R_Continue); // 不是等待锁定状态,被中断了;
    }
    phTCB->Lock++; // 加锁(核心部分),正在情况下值为"_XC_Lock_Lock",若是在累加开始前被中断,则累加结束后为"_XC_Lock_Lock+1";
    if(phTCB->Lock != _XC_Lock_Lock) {
        return (_XC_R_Continue); // 被中断,其他地方唤醒
    }

    /** 以下代码已经确定是中断安全的 */

    phTCB->pNotifyData = pNotifyData; // 提前保存传递的通知数据
    /**
     *  设备在就绪表,表示被中断,等待通知已经处理,但是任务还没有切换到非就绪表;
     *  这里直接更新解锁标志,需要在调度中异步处理任务唤醒;
     * ---
     *  判断是否在就绪表后,还需要判断当前是否有链表操作(防链表操作被中断);
     *  若是有链表操作则标记,需要在调度中异步处理任务唤醒;
     */
    if((phTCB->ListNode.pRootList == &phTCB->phXCOS->ReadyList) || // 判断任务是否在就绪表
       (XCSch_GetListOperationState(phTCB->phXCOS) == 1)) {        // 判断是否有链表操作
        phTCB->Lock = _XC_Lock_WaitUnlockNotify;                   // 标记等待解锁,在调度中继续处理
        // 任务触发标记
        if(phTCB->phXCOS->TaskSchedFlag == 0) {
            phTCB->phXCOS->TaskSchedFlag++;
        }
        return (_XC_R_Continue);
    }

    /** 以下是非异步操作 */

    XCSch_ListOperationStart(phTCB->phXCOS); // 链表操作开始
    XC_ListNodeRemove(phTCB);                // 删除任务
    XC_ListNodeInsertIndexPrevious(phTCB);   // 插入就续表
    phTCB->WakeType  = _XC_Wake_Notify;      // 被通知唤醒
    phTCB->TaskState = _XC_S_Ready;          // 任务状态:就绪
    XCSch_ListOperationEnd(phTCB->phXCOS);   // 链表操作结束

    phTCB->Lock = _XC_Lock_Unlock; // 没有锁
    return (_XC_R_OK);

#else

    // 发送通知只会唤醒等待通知的任务
    if(phTCB->TaskState != _XC_S_WaitNotify) {
        return (_XC_R_Fail);
    }
    XCSch_ListOperationStart(phTCB->phXCOS); // 链表操作开始
    XC_ListNodeRemove(phTCB);                // 删除任务
    XC_ListNodeInsertIndexPrevious(phTCB);   // 插入就续表
    phTCB->WakeType  = _XC_Wake_Notify;      // 被通知唤醒
    phTCB->TaskState = _XC_S_Ready;          // 任务状态:就绪
    XCSch_ListOperationEnd(phTCB->phXCOS);   // 链表操作结束

    return (_XC_R_OK);

#endif
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
// 配置框架中断支持
#if (_XC_Cnf_IntSupport == 1)

    // 挂起判断
    if(phTCB->TaskState == _XC_S_Suspend) {
        return (_XC_R_OK); // 已经被挂起
    }

    // 锁定,详细说明见"XC_SendNotify"函数;
    if(phTCB->Lock != _XC_Lock_WaitLock) {
        return (_XC_R_Continue); // 不是等待锁定状态,被中断了;
    }
    phTCB->Lock++; // 加锁(核心部分),正在情况下值为"_XC_Lock_Lock",若是在累加开始前被中断,则累加结束后为"_XC_Lock_Lock+1";
    if(phTCB->Lock != _XC_Lock_Lock) {
        return (_XC_R_Continue); // 被中断,其他地方唤醒
    }

    /** 以下代码已经确定是中断安全的 */

    /**
     *  判断任务状态,任务状态在运行中,表示任务在运行中被中断;
     *  需要异步处理挂起;
     *  ---
     *  然后还需要判断当前是否有链表操作(防链表操作被中断);
     *  若是有链表操作则标记,需要在调度中异步处理任务唤醒;
     */
    if((phTCB->TaskState == _XC_S_Run) ||                   // 判断任务是否在运行中
       (XCSch_GetListOperationState(phTCB->phXCOS) == 1)) { // 判断是否有链表操作
        phTCB->Lock = _XC_Lock_WaitUnlockSuspend;           // 标记等待解锁,在调度中继续处理
        // 任务触发标记
        if(phTCB->phXCOS->TaskSchedFlag == 0) {
            phTCB->phXCOS->TaskSchedFlag++;
        }
        return (_XC_R_Continue);
    }

    /** 以下是非异步操作 */

    XCSch_ListOperationStart(phTCB->phXCOS);                         // 链表操作开始
    phTCB->TaskState = _XC_S_Suspend;                                // 任务状态:挂起
    XC_ListNodeRemove(phTCB);                                        // 删除任务
    XCList_InsertEnd(&phTCB->phXCOS->BlockedList, &phTCB->ListNode); // 插入阻塞表
    XCSch_ListOperationEnd(phTCB->phXCOS);                           // 链表操作结束

    phTCB->Lock = _XC_Lock_Unlock; // 没有锁
    return (_XC_R_OK);

#else

    // 挂起判断
    if(phTCB->TaskState == _XC_S_Suspend) {
        return (_XC_R_OK); // 已经被挂起
    }
    XCSch_ListOperationStart(phTCB->phXCOS);                         // 链表操作开始
    phTCB->TaskState = _XC_S_Suspend;                                // 任务状态:挂起
    XC_ListNodeRemove(phTCB);                                        // 删除任务
    XCList_InsertEnd(&phTCB->phXCOS->BlockedList, &phTCB->ListNode); // 插入阻塞表
    XCSch_ListOperationEnd(phTCB->phXCOS);                           // 链表操作结束

    return (_XC_R_OK);

#endif
}

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
int32_t XC_TaskResume(XCTCB_t* phTCB)
{
// 配置框架中断支持
#if (_XC_Cnf_IntSupport == 1)

    // 任务没有被挂起,直接成功
    if(phTCB->TaskState != _XC_S_Suspend) {
        return (_XC_R_OK); // 没有被挂起
    }

    // 锁定,详细说明见"XC_SendNotify"函数;
    if(phTCB->Lock != _XC_Lock_WaitLock) {
        return (_XC_R_Continue); // 不是等待锁定状态,被中断了;
    }
    phTCB->Lock++; // 加锁(核心部分),正在情况下值为"_XC_Lock_Lock",若是在累加开始前被中断,则累加结束后为"_XC_Lock_Lock+1";
    if(phTCB->Lock != _XC_Lock_Lock) {
        return (_XC_R_Continue); // 被中断,其他地方唤醒
    }

    /** 以下代码已经确定是中断安全的 */

    /**
     *  判断当前是否有链表操作(防链表操作被中断);
     *  若是有链表操作则标记,需要在调度中异步处理任务唤醒;
     */
    if(XCSch_GetListOperationState(phTCB->phXCOS) == 1) {
        phTCB->Lock = _XC_Lock_WaitUnlockResume; // 标记等待解锁,在调度中继续处理
        // 任务触发标记
        if(phTCB->phXCOS->TaskSchedFlag == 0) {
            phTCB->phXCOS->TaskSchedFlag++;
        }
        return (_XC_R_Continue);
    }

    /** 以下是非异步操作 */

    XCSch_ListOperationStart(phTCB->phXCOS); // 链表操作开始
    XC_ListNodeRemove(phTCB);                // 删除任务
    XC_ListNodeInsertIndexPrevious(phTCB);   // 插入就续表
    phTCB->WakeType  = _XC_Wake_TaskResume;  // 被任务恢复唤醒
    phTCB->TaskState = _XC_S_Ready;          // 任务状态:就绪
    XCSch_ListOperationEnd(phTCB->phXCOS);   // 链表操作结束

    phTCB->Lock = _XC_Lock_Unlock; // 没有锁
    return (_XC_R_OK);

#else

    // 任务没有被挂起,直接成功
    if(phTCB->TaskState != _XC_S_Suspend) {
        return (_XC_R_OK); // 没有被挂起
    }
    XCSch_ListOperationStart(phTCB->phXCOS); // 链表操作开始
    XC_ListNodeRemove(phTCB);                // 删除任务
    XC_ListNodeInsertIndexPrevious(phTCB);   // 插入就续表
    phTCB->WakeType  = _XC_Wake_TaskResume;  // 被任务恢复唤醒
    phTCB->TaskState = _XC_S_Ready;          // 任务状态:就绪
    XCSch_ListOperationEnd(phTCB->phXCOS);   // 链表操作结束

    return (_XC_R_OK);

#endif
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
