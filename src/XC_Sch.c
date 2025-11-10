/**
 * @file        XC_Sch.c
 * @brief       调度器实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/10/30
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     用于调度任务的调度器实现;
 * **********************************************
 *  修改日志
 *  - 见"XC_UpdateInfo.md"的更新说明;
 */
//=== 头文件
#include "XC_Sch.h"
#include "XC_Task.h"
#include "XC_Time.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**任务-私有宏定义*/

/**
 * @brief       [私有]获取任务的唤醒的Tick
 * @param[in]   _phTCB      [XCTCB_t*]任务TCB(会强制转为"XCTCB_t*"类型)
 * @return      XCuint_t    返回任务唤醒的Tick
 * @details     得到当前链表的开始地址
 */
#define XCSch_GetTaskWakeTick(_phTCB) (((XCTCB_t*)(_phTCB))->TaskWakeTick)

/**
 * @brief       [私有]更新任务的唤醒的Tick
 * @param[in]   _phTCB      [XCTCB_t*]任务TCB(会强制转为"XCTCB_t*"类型)
 * @param[in]   _WakeTick   [XCuint_t]任务唤醒的Tick
 * @details     更新任务中的下个唤醒时刻Tick
 */
#define XCSch_UpdataTaskWakeTick(_phTCB, _WakeTick)       \
    {                                                     \
        ((XCTCB_t*)(_phTCB))->TaskWakeTick = (_WakeTick); \
    }

/**
 * @brief       [私有]更新任务唤醒类型
 * @param[in]   _phTCB      [XCTCB_t*]任务TCB(会强制转为"XCTCB_t*"类型)
 * @param[in]   _WakeType   [uint8_t]唤醒类型(_XC_Wake_**)
 * @details     更新任务唤醒类型
 */
#define XCSch_UpdataTaskWakeType(_phTCB, _WakeType)   \
    {                                                 \
        ((XCTCB_t*)(_phTCB))->WakeType = (_WakeType); \
    }

/**
 * @brief       [私有]更新任务状态
 * @param[in]   _phTCB      [XCTCB_t*]任务TCB(会强制转为"XCTCB_t*"类型)
 * @param[in]   _TaskState  [uint8_t]任务状态(_XC_S_**)
 * @details     更新任务状态
 */
#define XCSch_UpdataTaskState(_phTCB, _TaskState)       \
    {                                                   \
        ((XCTCB_t*)(_phTCB))->TaskState = (_TaskState); \
    }

/************************************************ 我是分割线 ************************************************/
/**框架-私有宏定义*/

/**
 * @brief       [私有]更新框架中下个唤醒任务的Tick
 * @param[in]   _phXCOS             [XCOS_t*]框架句柄
 * @param[in]   _NextWakeTaskTick   [XCuint_t]更新的下个唤醒任务的Tick
 * @details     更新的是下次有唤醒任务的Tick;当系统Tick大于这个值,则需要开始处理唤醒任务;
 */
#define XCSch_UpdataNextWakeTaskTick(_phXCOS, _NextWakeTaskTick) \
    {                                                            \
        (_phXCOS)->NextTaskWakeTick = (_NextWakeTaskTick);       \
    }

/**
 * @brief       [私有]获取框架中下个唤醒任务的Tick
 * @param[in]   _phXCOS     [XCOS_t*]框架句柄
 * @return      XCuint_t    返回下个唤醒任务的Tick
 * @details     获取到的值主要用来对比系统Tick,当系统Tick大于这个值,则需要开始处理唤醒任务;
 */
#define XCSch_GetNextWakeTaskTick(_phXCOS) ((_phXCOS)->NextTaskWakeTick)

/**
 * @brief       [私有]更新上个Tick
 * @param[in]   _phXCOS [XCOS_t*]框架句柄
 * @param[in]   _Tick   [XCuint_t]更新的Tick值
 * @details     更新上个Tick,用于下次调度使用;  \n
 *  只有在下面2个条件下才可更新:
 *  - 在遍历所有时间到达处理后可更新;
 *  - 在1完成情况下,有新的任务需要入时间阻塞时可更新;
 */
#define XCSch_UpdataPreviousTick(_phXCOS, _Tick) \
    {                                            \
        (_phXCOS)->PreviousTick = (_Tick);       \
    }

/**
 * @brief       [私有]获取上个Tick
 * @param[in]   _phXCOS     [XCOS_t*]框架句柄
 * @return      XCuint_t    返回上个Tick
 * @details     获取上次处理时间时保存的Tick;这个值主要用来处理当系统Tick溢出时的操作;
 */
#define XCSch_GetPreviousTick(_phXCOS) ((_phXCOS)->PreviousTick)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**调度处理*/

/**
 * @brief       [私有]时间调度
 * @param[in]   phXCOS  框架句柄
 * @param[in]   Tick    当前系统Tick
 * @details
 *  时间调度处理:
 *  - 处理系统Tick溢出任务的调度(时间表移动到就绪表,溢出表移动到时间表);
 *  - 处理时间到达后将任务的调度(到达任务移动到就绪表);
 *  注意,这个函数是处理任务时间调度的对当前结束的任务不做调度;
 */
static void XCSch_TimeSched(XCOS_t* phXCOS, XCuint_t Tick)
{
    XCListNode_t* pIndex;
    XCTCB_t*      phTCB;

    /**
     *  上个保存的Tick大于当前当前Tick,表示Tick溢出
     *  需要做以下处理:
     *  1.将"TimeList"表所有节点移动到"ReadyList"表;
     *  2.将"TimeOverflowList"表所有节点移动到"TimeList"表;
     *  3.更新下个唤醒的时间;
     */
    if(XCSch_GetPreviousTick(phXCOS) > Tick) {
        // 上个保存的Tick大于当前当前Tick,表示Tick已经溢出,判断表中是否有节点需要处理;
        XCSch_ListOperationStart(phXCOS); // 链表操作开始
        if(XCList_ListValid(&phXCOS->TimeList)) {
            //"TimeList"表中有节点,将所有节点移动到"ReadyList"表
            pIndex = XCList_GetListStartNode(&phXCOS->TimeList); // 得到时间链表初始节点
            while(!XCList_ReachEndNode(&phXCOS->TimeList, pIndex)) {
                // 节点没有到达结尾,将节点状态改变(这里只改变状态,不移动节点)
                XCSch_UpdataTaskState(pIndex, _XC_S_Ready);      // 任务状态:就绪
                XCSch_UpdataTaskWakeType(pIndex, _XC_Wake_Time); // 时间唤醒
                pIndex = pIndex->pNext;                          // 指向下个节点
            }
            // 将"TimeList"链表移动到节点"pReadyListNodeIndex"前,并清除"TimeList"链表
            XCList_SwapListToNodePrevious(phXCOS->pReadyListNodeIndex, &phXCOS->TimeList);
        }
        // 判断"TimeOverflowList"表中是否有节点
        if(XCList_ListValid(&phXCOS->TimeOverflowList)) { // 表中有节点
            //"TimeList"表节点处理完成后,直接和"TimeOverflowList"表换,同等于"TimeOverflowList"表所有节点移动到"TimeList"表;
            XCList_SwapList(&phXCOS->TimeList, &phXCOS->TimeOverflowList);
            // 从"TimeList"链表根节点更新下个唤醒时间
            XCSch_UpdataNextWakeTaskTick(phXCOS, XCSch_GetTaskWakeTick(XCList_GetListStartNode(&phXCOS->TimeList)));
        }
        else {
            XCSch_UpdataNextWakeTaskTick(phXCOS, ~0); // 若是没有节点,将唤醒时间调整为最大
        }
        XCSch_ListOperationEnd(phXCOS);         // 链表操作结束
        XCSch_UpdataPreviousTick(phXCOS, Tick); // 更新保存Tick
    }

    /**
     *  时间表处理
     *  "TimeList"表有节点,且下个唤醒任务的时间到达,则调用;
     *  需要以下操作;
     *  1.遍历"TimeList"表,唤醒时间到达的任务都移动到"ReadyList"表;
     *  2.更新下个唤醒的时间;
     */
    if(XCList_ListValid(&phXCOS->TimeList)) {
        // 链表中是有节点的
        if(Tick >= XCSch_GetNextWakeTaskTick(phXCOS)) {
            // 下个任务的唤醒时间到达,处理
            XCSch_ListOperationStart(phXCOS);                    // 链表操作开始
            pIndex = XCList_GetListStartNode(&phXCOS->TimeList); // 得到时间链表初始节点
            while((!XCList_ReachEndNode(&phXCOS->TimeList, pIndex)) && (Tick >= XCSch_GetTaskWakeTick(pIndex))) {
                // 节点没有到达结尾 && 当前索引任务唤醒时间到达
                phTCB  = (XCTCB_t*)pIndex; // 得到索引任务的TCB
                pIndex = pIndex->pNext;    // 指向下个任务
                // 唤醒任务移到就绪表
                XC_ListNodeRemove(phTCB);                       // 原链表移除节点
                XC_ListNodeInsertIndexPrevious(phTCB);          // 插入就绪表
                XCSch_UpdataTaskState(phTCB, _XC_S_Ready);      // 任务状态:就绪
                XCSch_UpdataTaskWakeType(phTCB, _XC_Wake_Time); // 时间唤醒
            }
            // 轮询结束,判断索引是否到达链表的结尾
            if(XCList_ReachEndNode(&phXCOS->TimeList, pIndex)) {
                // 到达链表结尾,标示链表中已经没有节点;
                XCSch_UpdataNextWakeTaskTick(phXCOS, ~0); // 若是没有节点,将唤醒时间调整为最大
            }
            else {
                // 没有到达链表的结尾,链表还有节点,将当前节点的唤醒时间设置为下个需要唤醒的时间;
                XCSch_UpdataNextWakeTaskTick(phXCOS, XCSch_GetTaskWakeTick(pIndex)); // 更新下个唤醒时间
            }
            XCSch_ListOperationEnd(phXCOS);         // 链表操作结束
            XCSch_UpdataPreviousTick(phXCOS, Tick); // 更新保存的Tick
        }
    }
}

/**
 * @brief       [私有]阻塞调度
 * @param[in]   phXCOS  框架句柄
 * @param[in]   phTCB   有阻塞操作的任务TCB句柄
 * @param[in]   Tick    当前系统Tick
 * @details
 *  处理任务的阻塞状态;
 *  - 处理任务唤醒时间,由唤醒时间决定任务是进时间表还是时间溢出表;
 *  - 处理任务阻塞,挂起或者死等的任务进入这个表;
 *  注意:
 *  > 此函数处理任务进入调度,但是不参与调度;
 *  > 调用此函数前必须先调用"XCSch_TimeSched"处理时间调度;
 */
static void XCSch_BlockedSched(XCOS_t* phXCOS, XCTCB_t* phTCB, XCuint_t Tick)
{
    XCuint_t TaskWakeTick;

    XCSch_ListOperationStart(phXCOS); // 链表操作开始
    /**
     * 任务时间阻塞处理(优先级最高)
     *  处理任务唤醒时间,由唤醒时间判断,任务是进时间表还是时间溢出表;
     */
    if((phTCB->Blocked & _XC_B_WaitTime) && XCSch_GetTaskWakeTick(phTCB)) {
        // 有时间处理 && 时间不为0
        TaskWakeTick = XCSch_GetTaskWakeTick(phTCB) + Tick; // 当前任务下次唤醒的时刻
        XCSch_UpdataTaskWakeTick(phTCB, TaskWakeTick);      // 更新任务下次唤醒的时刻
        XC_ListNodeRemove(phTCB);                           // 移除当前节点
        // 唤醒时间是否是Tick溢出;
        if(TaskWakeTick < Tick) {
            // 下次唤醒的Tick溢出
            XC_ListNodeInsertAsc(&phXCOS->TimeOverflowList, phTCB); // 任务入时间溢出表
        }
        else {
            // 下次唤醒的Tick没有溢出
            XC_ListNodeInsertAsc(&phXCOS->TimeList, phTCB); // 任务入时间表
            // 更新框架下次任务唤醒的时刻
            if(TaskWakeTick < XCSch_GetNextWakeTaskTick(phXCOS)) {
                // 当前任务下次唤醒值小于框架保存的唤醒值,则更新框架唤醒值
                XCSch_UpdataNextWakeTaskTick(phXCOS, TaskWakeTick); // 更新框架中的唤醒时刻
                XCSch_UpdataPreviousTick(phXCOS, Tick);             // 更新Tick
            }
        }
    }
    // 阻塞处理
    else if(phTCB->Blocked & _XC_B_Blocked) {
        /**
         * 任务挂起,或者死等,进入阻塞表;
         */
        XC_ListNodeRemove(phTCB);                                 // 移除当前节点
        XCList_InsertEnd(&phXCOS->BlockedList, &phTCB->ListNode); // 插入阻塞表
    }

    // 清除阻塞标志
    phTCB->Blocked = _XC_B_NonBlocked;
    XCSch_ListOperationEnd(phXCOS); // 链表操作结束
}

/**
 * @brief       [私有]任务调度
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数用于:
 *      - 任务唤醒,"XC_SendNotify"函数触发;
 *      - 任务挂起,"XC_TaskSuspend"函数触发;
 *      - 任务挂起恢复,"XC_TaskResume"函数触发;
 *  只有在以上3个函数在中断不安全的情况下被调用,才会触发运行此函数; \n
 *  注意:在调用此函数前必须先判断"phXCOS->TaskSchedTrigger != phXCOS->TaskSchedProcessed"; \n
 *  此函数的实现主要是循环搜索链表,判断是否有需要处理的任务: \n
 *  搜索链表为: \n
 *      - ReadyList
 *      - TimeList
 *      - TimeOverflowList
 *      - BlockedList
 */
static void XCSch_TaskSched(XCOS_t* phXCOS)
{
    XCListNode_t* pIndex;
    XCListRoot_t* pList[4];
    XCTCB_t*      phTCB;
    uint8_t       i;

    volatile uint8_t Trigger;
    volatile uint8_t StateChangeType;

    pList[0] = &phXCOS->TimeList;         //"TimeList"延时/超时/等待的链表
    pList[1] = &phXCOS->TimeOverflowList; //"TimeOverflowList"时间溢出的链表
    pList[2] = &phXCOS->BlockedList;      //"BlockedList"阻塞链表
    pList[3] = &phXCOS->ReadyList;        //"ReadyList"就绪链表

    XCSch_ListOperationStart(phXCOS); // 链表操作开始

    /**
     * 链表循环搜索判断
     */
    for(i = 0; i < 4; i++) {
        pIndex = XCList_GetListStartNode(pList[i]);      // 得到链表初始节点
        while(!XCList_ReachEndNode(pList[i], pIndex)) {  // 判断是否到达结尾
            phTCB           = (XCTCB_t*)(pIndex);        // 得到当前任务TCB
            pIndex          = pIndex->pNext;             // 指向下个节点(必须在操作前指向下个节点)
            Trigger         = phTCB->StateChangeTrigger; // 原子操作,状态改变触发
            StateChangeType = phTCB->StateChangeType;    // 原子操作,状态改变的类型
            if(Trigger != phTCB->StateChangeProcessed) { // 状态被改变
                phTCB->StateChangeProcessed = Trigger;   // 更新状态改变处理
                if(StateChangeType == _XC_S_Void) {
                    /** 任务恢复 */
                    XC_ListNodeRemove(phTCB);               // 移除任务节点
                    XC_ListNodeInsertIndexPrevious(phTCB);  // 插入就续表
                    phTCB->WakeType  = _XC_Wake_TaskResume; // 被任务恢复唤醒
                    phTCB->TaskState = _XC_S_Ready;         // 任务状态:就绪
                }
                else {
                    /** 任务挂起 */
                    phTCB->TaskState = _XC_S_Suspend;                                // 任务状态:挂起
                    XC_ListNodeRemove(phTCB);                                        // 移除任务节点
                    XCList_InsertEnd(&phTCB->phXCOS->BlockedList, &phTCB->ListNode); // 插入阻塞表
                }
            }
            else {
                Trigger = phTCB->NotifyTrigger;         // 原子操作,通知触发
                if(Trigger != phTCB->NotifyProcessed) { // 状态被改变
                    phTCB->NotifyProcessed = Trigger;   // 更新状态改变处理
                    /** 通知唤醒 */
                    XC_ListNodeRemove(phTCB);              // 移除任务节点
                    XC_ListNodeInsertIndexPrevious(phTCB); // 插入就续表
                    phTCB->WakeType  = _XC_Wake_Notify;    // 被通知唤醒
                    phTCB->TaskState = _XC_S_Ready;        // 任务状态:就绪
                }
            }
        }
    }

    XCSch_ListOperationEnd(phXCOS); // 链表操作结束
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 用户函数 */

/**
 * @brief       [用户]调度器初始化
 * @param[in]   phXCOS  框架句柄
 * @details     用于初始化框架的调度器;
 */
void XCSch_Init(XCOS_t* phXCOS)
{
    XCList_Init(&phXCOS->ReadyList);
    XCList_Init(&phXCOS->TimeList);
    XCList_Init(&phXCOS->TimeOverflowList);
    XCList_Init(&phXCOS->BlockedList);

    phXCOS->pReadyListNodeIndex = (XCListNode_t*)&phXCOS->ReadyList.RootNode; // 就绪表的根链表
    phXCOS->NextTaskWakeTick    = ~0;                                         // 下个任务唤醒时间为最大
    phXCOS->PreviousTick        = 0;                                          // 保存上个Tick值
    phXCOS->TaskNum             = 0;                                          // 任务数量
    phXCOS->ListOperationFlag   = 0;                                          // 表操作标记(1操作中,0没有操作)
    phXCOS->TaskSchedTrigger    = 0;                                          // 任务调度触发(用于通知,挂起,恢复异步操作触发)
    phXCOS->TaskSchedProcessed  = 0;                                          // 任务调度处理(用于通知,挂起,恢复异步操作触发后处理)

#if (_XC_Cnf_IdleSupport == 1) // 配置框架空闲支持
    phXCOS->fIdle = NULL;      // 框架空闲处理
#endif
}

/**
 * @brief       [用户]调度器运行(阻塞)
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数无返回,阻塞运行;
 *  > 注意:阻塞下要是有看门狗,记得在任务中喂狗;
 */
void XCSch_Run(XCOS_t* phXCOS)
{
    XCTCB_t*         phTCB;
    XCuint_t         Tick;
    volatile uint8_t Trigger;

    while(1) {
        phXCOS->pReadyListNodeIndex = phXCOS->pReadyListNodeIndex->pNext; // 向下更新就绪表索引
        // 是否遍历到链表结尾(到根节点)
        if(XCList_ReachEndNode(&phXCOS->ReadyList, phXCOS->pReadyListNodeIndex)) { // 到达结尾(是根节点),只处理时间
            Tick = XCTime_GetTick();                                               // 得到当前系统Tick
            XCSch_TimeSched(phXCOS, Tick);                                         // 时间调度处理
        }
        else {                                               // 未到达结尾(不是根节点),运行任务;
            phTCB = (XCTCB_t*)(phXCOS->pReadyListNodeIndex); // 得到任务TCB
            XCSch_UpdataTaskState(phTCB, _XC_S_Run);         // 任务状态:运行
            phTCB->fTask(phTCB);                             // 运行任务
            Tick = XCTime_GetTick();                         // 得到当前系统Tick
            XCSch_TimeSched(phXCOS, Tick);                   // 时间调度处理
            if(phTCB->Blocked) {                             // 任务需要阻塞处理
                XCSch_BlockedSched(phXCOS, phTCB, Tick);     // 当前任务阻塞调度处理
            }
        }

        // 任务调度
        Trigger = phXCOS->TaskSchedTrigger;
        if(Trigger != phXCOS->TaskSchedProcessed) {
            phXCOS->TaskSchedProcessed = Trigger;
            XCSch_TaskSched(phXCOS);
        }

        // 配置框架空闲支持
#if (_XC_Cnf_IdleSupport == 1)
        // 判断就绪表是否有任务(空闲处理)
        if((phXCOS->fIdle != NULL) &&                                   // 有空闲处理函数
           (XCList_ListValid(&phXCOS->ReadyList) == 0)) {               // 就绪表没有节点
            Tick = XCTime_GetTick();                                    // 得到当前Tick
            if(Tick < phXCOS->NextTaskWakeTick) {                       // 当前Tick必须小于下次唤醒的Tick值
                phXCOS->fIdle(phXCOS, phXCOS->NextTaskWakeTick - Tick); // 框架空闲处理
            }
        }
#endif
    }
}

/**
 * @brief       [用户]调度器运行(非阻塞)
 * @param[in]   phXCOS  框架句柄
 * @details
 *  非阻塞运行,需要循环调度
 *  和"XCSch_Run"一样的实现,只是去掉了函数内的循环;
 */
void XCSch_RunNonBlocked(XCOS_t* phXCOS)
{
    XCTCB_t*         phTCB;
    XCuint_t         Tick;
    volatile uint8_t Trigger;

    phXCOS->pReadyListNodeIndex = phXCOS->pReadyListNodeIndex->pNext; // 向下更新就绪表索引
    // 是否遍历到链表结尾(到根节点)
    if(XCList_ReachEndNode(&phXCOS->ReadyList, phXCOS->pReadyListNodeIndex)) {
        // 到达结尾(是根节点),只处理时间
        Tick = XCTime_GetTick();       // 得到当前系统Tick
        XCSch_TimeSched(phXCOS, Tick); // 时间调度处理
    }
    else {
        // 不是根节点处理任务
        phTCB = (XCTCB_t*)(phXCOS->pReadyListNodeIndex); // 得到任务TCB
        XCSch_UpdataTaskState(phTCB, _XC_S_Run);         // 任务状态:运行
        phTCB->fTask(phTCB);                             // 运行任务
        Tick = XCTime_GetTick();                         // 得到当前系统Tick
        XCSch_TimeSched(phXCOS, Tick);                   // 时间调度处理
        if(phTCB->Blocked) {
            XCSch_BlockedSched(phXCOS, phTCB, Tick); // 阻塞调度处理
        }
    }
    // 任务调度
    Trigger = phXCOS->TaskSchedTrigger;
    if(Trigger != phXCOS->TaskSchedProcessed) {
        phXCOS->TaskSchedProcessed = Trigger;
        XCSch_TaskSched(phXCOS);
    }

    // 配置框架空闲支持
#if (_XC_Cnf_IdleSupport == 1)
    // 判断就绪表是否有任务(空闲处理)
    if((phXCOS->fIdle != NULL) &&                                   // 有空闲处理函数
       (XCList_ListValid(&phXCOS->ReadyList) == 0)) {               // 就绪表没有节点
        Tick = XCTime_GetTick();                                    // 得到当前Tick
        if(Tick < phXCOS->NextTaskWakeTick) {                       // 当前Tick必须小于下次唤醒的Tick值
            phXCOS->fIdle(phXCOS, phXCOS->NextTaskWakeTick - Tick); // 框架空闲处理
        }
    }
#endif
}

/**
 * @brief       [用户]获取任务数
 * @param[in]   phXCOS  框架句柄
 * @return      uint8_t 返回任务数量
 * @details     直接获取框架句柄中的任务数字段;
 */
uint8_t XCSch_GetTaskNum(XCOS_t* phXCOS)
{
    return (phXCOS->TaskNum);
}

/************************************************ 我是分割线 ************************************************/

// 配置框架空闲支持
#if (_XC_Cnf_IdleSupport == 1)

/**
 * @brief       [用户]设置空闲处理回调
 * @param[in]   phXCOS  框架句柄
 * @param[in]   fIdle  框架空闲处理回调
 * @details
 *  用于设置框架空闲处理回调; \n
 *  若是需要清除回调则"fIdle"值为NULL即可;
 */
void XCSch_SetIdleCallback(XCOS_t* phXCOS, void (*fIdle)(XCOS_t*, XCuint_t))
{
    phXCOS->fIdle = fIdle;
}

#ifdef __XC_SysTickIntAccMode__
/**
 * @brief       [用户]休眠唤醒后更新tick
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数是框架句柄中"fIdle"函数休眠框架,且定时器同时停止的情况下,在设备唤醒后调用;
 *  直接将Tick的值更新到框架句柄中的"NextTaskWakeTick"值
 *  注意:只有在系统Tick是定时器中断计数运行的情况下可以使用此函数;
 */
void XCSch_UpdateTickAfterWakeup(XCOS_t* phXCOS)
{
    // 只有在"中断累加形式"下有效;

    XCuint_t Tick;
    Tick             = phXCOS->NextTaskWakeTick;
    _XC_SysTickCount = Tick;
}
#endif

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
