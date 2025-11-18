/**
 * @file        XC_Sch.c
 * @brief       调度器实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/11/12
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
/**调度处理*/

/**
 * @brief       [私有]时间调度
 * @param[in]   phXCOS  框架句柄
 * @details
 *  时间调度处理: \n
 *  - 处理系统Tick溢出后的任务调度(时间表移动到就绪表,溢出表移动到时间表); \n
 *  - 处理任务时间到达后的任务调度(到达任务移动到就绪表); \n \n
 *  注意: \n
 *  >   此函数只处理在时间表和溢出时间表的任务的调度; \n
 */
static void XCSch_TimeSched(XCOS_t* phXCOS)
{
    XCListNode_t* pIterator;
    XCuint_t      Tick;
    XCTCB_t*      phTCB;

    Tick = XCTime_GetTick(); // 得到当前系统Tick
    XCSch_ListLock(phXCOS);  // 链表锁
    /**
     *  Tick时间溢出处理
     *  比较保存的Tick("PrevTick")和当前的Tick值,保存的值大于当前的值,则表示计数溢出
     *  溢出后处理如下:
     *  - 遍历时间表("TimeList"),更改节点装,并将节点移动到就绪表("ReadyList");
     *  - 将时间溢出表("TimeOverflowList")的所有节点移动到时间表("TimeList");
     *  - 从时间表得到最新的下个唤醒时间("NextTaskWakeTick");
     */
    if(phXCOS->PrevTick > Tick) { // 保存的Tick大于当前Tick,时间溢出,需要处理
        // 时间表处理
        if(XCList_ListValid(&phXCOS->TimeList)) { // 时间表有节点
            // 所有节点移动到就绪表;
            pIterator = XCList_GetListStartNode(&phXCOS->TimeList); // 得到时间链表初始节点
            do {
                ((XCTCB_t*)pIterator)->TaskState = _XC_S_Ready;      // 任务状态:就绪
                ((XCTCB_t*)pIterator)->WakeType  = _XC_Wake_Time;    // 是时间唤醒
                pIterator                        = pIterator->pNext; // 指向下个节点
            } while(!XCList_ReachEndNode(&phXCOS->TimeList, pIterator));
            XCList_MoveListToNodeBefore(phXCOS->pReadyNode, &phXCOS->TimeList); // 时间表所有节点移动到就绪表
        }
        // 时间溢出表处理(时间溢出表->时间表)
        if(XCList_ListValid(&phXCOS->TimeOverflowList)) {                                                      // 时间溢出表有节点
            XCList_MoveListToNodeBefore(&phXCOS->TimeList, &phXCOS->TimeOverflowList);                         // 时间溢出表所有节点移动到时间表
            phXCOS->NextTaskWakeTick = ((XCTCB_t*)(XCList_GetListStartNode(&phXCOS->TimeList)))->TaskWakeTick; // 首节点的下次唤醒的Tick
        }
        else {
            /** 时间溢出表没有节点,时间表在之前已经全部移动到就绪表,所以时间表也没有节点 */
            phXCOS->NextTaskWakeTick = ~0; // 更新下个唤醒时间,若是没有节点,将唤醒时间调整为最大
        }
        phXCOS->PrevTick = Tick; // 更新保存Tick
    }

    /**
     *  时间表处理
     *  时间表("TimeList")有节点,且下个唤醒任务的时间到达,则调用;
     *  需要以下操作;
     *  - 遍历时间表("TimeList"),唤醒时间到达的任务都移动到就绪表("ReadyList");
     *  - 更新下个唤醒的时间;
     *  注意:
     *      安全操作中,必须另存任务TCB并立刻将迭代器移动至下个节点,
     *      因为当前节点会移动到就绪表,若是直接移动迭代器,链表指向就会错误;
     */
    if((XCList_ListValid(&phXCOS->TimeList)) && (Tick >= phXCOS->NextTaskWakeTick)) { // 时间表有效 && 任务唤醒时间已经到达
        pIterator = XCList_GetListStartNode(&phXCOS->TimeList);                       // 得到时间链表初始节点
        // 轮寻链表,将时间到达的节点都移动到就绪表;
        while(Tick >= ((XCTCB_t*)pIterator)->TaskWakeTick) { // 唤醒时间到达
            phTCB     = (XCTCB_t*)pIterator;                 // 安全操作1,得到当前任务的TCB
            pIterator = pIterator->pNext;                    // 安全操作2,指向下个任务
            XC_MoveTaskToReadyList(phTCB);                   // 唤醒任务,移到就绪表
            phTCB->TaskState = _XC_S_Ready;                  // 任务状态:就绪
            phTCB->WakeType  = _XC_Wake_Time;                // 时间唤醒
            // 轮询结束判断
            if(XCList_ReachEndNode(&phXCOS->TimeList, pIterator)) { // 到达结尾
                phXCOS->NextTaskWakeTick = ~0;                      // 更新下个唤醒时间,没有节点了,将唤醒时间调整为最大
                goto GOTO_EndPolling;                               // 结束轮询
            }
        }
        phXCOS->NextTaskWakeTick = ((XCTCB_t*)(XCList_GetListStartNode(&phXCOS->TimeList)))->TaskWakeTick; // 更新下个唤醒时间
    GOTO_EndPolling:                                                                                       // 结束轮询
        phXCOS->PrevTick = Tick;                                                                           // 更新保存Tick
    }

    XCSch_ListUnlock(phXCOS); // 链表解锁
}

/**
 * @brief       [私有]阻塞调度
 * @param[in]   phXCOS  框架句柄
 * @param[in]   phTCB   有阻塞操作的任务TCB句柄
 * @details
 *  处理任务的阻塞状态; \n
 *  - 处理任务唤醒时间,由唤醒时间决定任务是进时间表还是时间溢出表; \n
 *  - 处理任务阻塞,挂起或者死等的任务进入阻塞表; \n
 *  注意: \n
 *  > 此调度函数主要是分配阻塞的任务; \n
 */
static void XCSch_BlockedSched(XCOS_t* phXCOS, XCTCB_t* phTCB)
{
    XCuint_t TaskWakeTick;
    XCuint_t Tick;

    Tick = XCTime_GetTick(); // 得到当前系统Tick
    XCSch_ListLock(phXCOS);  // 链表锁
    /**
     *  处理任务操作中的延时,超时,阻塞;
     *  在任务中只是将需要延时或者超时的值做保存,在此处处理;
     *  延时或者超时的值加上当前的Tick后得到下次需要唤醒的Tick;
     *  但是要注意,计算下次唤醒的Tick时计数值可能会溢出,所以需要做溢出判断,
     *  将任务分别挂载到两个链表中:时间表("TimeList")和溢出表("TimeOverflowList");
     */
    if((phTCB->Blocked & _XC_B_WaitTime) && phTCB->TaskWakeTick) { // 有时间处理 && 有延时或超时时间("TaskWakeTick"!=0)
        TaskWakeTick        = phTCB->TaskWakeTick + Tick;          // 得到此任务下次唤醒的的时刻
        phTCB->TaskWakeTick = TaskWakeTick;                        // 保存下次唤醒时刻
        XC_RemoveTaskNode(phTCB);                                  // 将当前节点从就绪表移除
        // 判断唤醒时刻是否是在Tick溢出后,
        if(TaskWakeTick < Tick) {                                      // 下次唤醒是Tick溢出后
            XC_InsertTaskToTimeList(&phXCOS->TimeOverflowList, phTCB); // 任务入时间溢出表
        }
        else {                                                                                                 // 下次唤醒在正常的Tick时刻
            XC_InsertTaskToTimeList(&phXCOS->TimeList, phTCB);                                                 // 任务入时间表
            phXCOS->NextTaskWakeTick = ((XCTCB_t*)(XCList_GetListStartNode(&phXCOS->TimeList)))->TaskWakeTick; // 更新下个唤醒时间,为首节点
        }
    }
    else if(phTCB->Blocked & _XC_B_Blocked) { // 是阻塞处理
        XC_MoveTaskToBlockedList(phTCB);      // 任务移动到阻塞表(任务挂起,或者死等,进入阻塞表;)
    }
    phTCB->Blocked = _XC_B_NonBlocked; // 清除阻塞标志

    XCSch_ListUnlock(phXCOS); // 链表解锁
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
    XCListNode_t* pIter;
    XCListNode_t* pList[4];
    XCTCB_t*      phTCB;
    uint8_t       i;

    volatile uint8_t Trigger;
    volatile uint8_t StateChangeType;

    pList[0] = &phXCOS->TimeList;         //"TimeList"延时/超时/等待的链表
    pList[1] = &phXCOS->TimeOverflowList; //"TimeOverflowList"时间溢出的链表
    pList[2] = &phXCOS->BlockedList;      //"BlockedList"阻塞链表
    pList[3] = &phXCOS->ReadyList;        //"ReadyList"就绪链表

    XCSch_ListLock(phXCOS); // 链表锁

    /**
     * 链表循环搜索判断
     */
    for(i = 0; i < 4; i++) {
        pIter = XCList_GetListStartNode(pList[i]);       // 得到链表初始节点
        while(!XCList_ReachEndNode(pList[i], pIter)) {   // 判断是否到达结尾
            phTCB           = (XCTCB_t*)(pIter);         // 得到当前任务TCB
            pIter           = pIter->pNext;              // 指向下个节点(必须在操作前指向下个节点)
            Trigger         = phTCB->StateChangeTrigger; // 原子操作,状态改变触发
            StateChangeType = phTCB->StateChangeType;    // 原子操作,状态改变的类型
            if(Trigger != phTCB->StateChangeProcessed) { // 状态被改变
                phTCB->StateChangeProcessed = Trigger;   // 更新状态改变处理
                if(StateChangeType == _XC_S_Void) {
                    /** 任务恢复 */
                    XC_MoveTaskToReadyList(phTCB);          // 任务移动到就绪表
                    phTCB->WakeType  = _XC_Wake_TaskResume; // 被任务恢复唤醒
                    phTCB->TaskState = _XC_S_Ready;         // 任务状态:就绪
                }
                else {
                    /** 任务挂起 */
                    phTCB->TaskState = _XC_S_Suspend; // 任务状态:挂起
                    XC_MoveTaskToBlockedList(phTCB);  // 任务移动到阻塞表
                }
            }
            else {
                Trigger = phTCB->NotifyTrigger;         // 原子操作,通知触发
                if(Trigger != phTCB->NotifyProcessed) { // 状态被改变
                    phTCB->NotifyProcessed = Trigger;   // 更新状态改变处理
                    /** 通知唤醒 */
                    XC_MoveTaskToReadyList(phTCB);      // 任务移动到就绪表
                    phTCB->WakeType  = _XC_Wake_Notify; // 被通知唤醒
                    phTCB->TaskState = _XC_S_Ready;     // 任务状态:就绪
                }
            }
        }
    }

    XCSch_ListUnlock(phXCOS); // 链表解锁
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

    phXCOS->pReadyNode         = &phXCOS->ReadyList; // 就绪节点指向就绪表根
    phXCOS->NextTaskWakeTick   = ~0;                 // 下个任务唤醒时间为最大
    phXCOS->PrevTick           = 0;                  // 保存上个Tick值
    phXCOS->TaskNum            = 0;                  // 任务数量
    phXCOS->ListLock           = 0;                  // 链表锁(1锁,0解锁)
    phXCOS->TaskSchedTrigger   = 0;                  // 任务调度触发(用于通知,挂起,恢复异步操作触发)
    phXCOS->TaskSchedProcessed = 0;                  // 任务调度处理(用于通知,挂起,恢复异步操作触发后处理)
    phXCOS->fIdle              = NULL;               // 框架空闲处理
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
    XCListNode_t*    pIterator; // 节点迭代器
    volatile uint8_t Trigger;

    pIterator = &phXCOS->ReadyList; // 就绪节点指向就绪表根
    while(1) {
        XCSch_ListLock(phXCOS);                                      // 链表锁
        if(XCList_ListValid(&phXCOS->ReadyList)) {                   // 就绪表有节点处理
            if(XCList_ReachEndNode(&phXCOS->ReadyList, pIterator)) { // 到达结尾(是根节点)
                pIterator = pIterator->pNext;                        // 再次向下更新就绪节点
            }
            // phXCOS->pReadyNode = pIterator;             // 得到当前就绪节点
            phTCB              = (XCTCB_t*)(pIterator); // 得到任务TCB
            pIterator          = pIterator->pNext;      // 向下更新就绪节点
            phXCOS->pReadyNode = pIterator;             // 得到下个就绪节点
            XCSch_ListUnlock(phXCOS);                   // 链表解锁
            phTCB->TaskState = _XC_S_Run;               // 任务状态:运行
            phTCB->fTask(phTCB);                        // 运行任务

            // XCSch_TimeSched(phXCOS); // 时间调度处理
            // 任务阻塞处理
            if(phTCB->Blocked) {
                XCSch_BlockedSched(phXCOS, phTCB); // 当前任务阻塞调度处理
            }
        }
        else {                        // 没有节点处理
            XCSch_ListUnlock(phXCOS); // 链表解锁
            // XCSch_TimeSched(phXCOS);  // 时间调度处理
        }

        XCSch_TimeSched(phXCOS); // 时间调度处理

        // 任务异步调度处理
        Trigger = phXCOS->TaskSchedTrigger;
        if(Trigger != phXCOS->TaskSchedProcessed) {
            phXCOS->TaskSchedProcessed = Trigger;
            XCSch_TaskSched(phXCOS);
        }

        // 判断就绪表是否有任务(空闲处理)
        if((phXCOS->fIdle != NULL) &&                     // 有空闲处理函数
           (XCList_ListValid(&phXCOS->ReadyList) == 0)) { // 就绪表没有节点
            /**
             *  主要判断时间表和溢出表是否有效(1有效,0无效):
             *  - 时间表1,溢出表0: 取字段"NextTaskWakeTick"(时间表首节点);
             *  - 时间表1,溢出表1: 取字段"NextTaskWakeTick"(时间表首节点);
             *  - 时间表0,溢出表1: 取溢出表首节点唤醒时间;
             *  - 时间表0,溢出表0: 系统休眠;
             */
            Tick = XCTime_GetTick();                  // 得到当前Tick
            if(XCList_ListValid(&phXCOS->TimeList)) { // 时间表有节点
                // Tick = phXCOS->NextTaskWakeTick - Tick;
                Tick = ((XCTCB_t*)(XCList_GetListStartNode(&phXCOS->TimeList)))->TaskWakeTick - Tick;
            }
            else if(XCList_ListValid(&phXCOS->TimeOverflowList)) { // 溢出表有节点
                Tick = ((XCTCB_t*)(XCList_GetListStartNode(&phXCOS->TimeOverflowList)))->TaskWakeTick - Tick;
            }
            else { // 时间表,溢出表都没有节点;
                Tick = ~0;
            }
            phXCOS->fIdle(phXCOS, Tick); // 框架空闲处理

            // Tick = XCTime_GetTick();                            // 得到当前Tick
            // if((!XCList_ListValid(&phXCOS->TimeList)) &&        // 时间表没有有节点
            //    (XCList_ListValid(&phXCOS->TimeOverflowList))) { // 时间溢出表有节点
            //     /**
            //      *  在Tick溢出前可能会出现以下情况:
            //      *  "就绪表无节点 && 时间表无节点 && 时间溢出表有时间"
            //      *  此情况下需要取"时间溢出表"首节点的唤醒时间作为下次唤醒时间;
            //      *  注意:
            //      *      此处不能用"phXCOS->NextTaskWakeTick"为(~0)判断,因为下个溢出时间也可能为(~0);
            //      */
            //     Tick = ((XCTCB_t*)(XCList_GetListStartNode(&phXCOS->TimeOverflowList)))->TaskWakeTick - Tick;
            // }
            // else {
            //     Tick = phXCOS->NextTaskWakeTick - Tick;
            // }
            // phXCOS->fIdle(phXCOS, Tick); // 框架空闲处理
        }
    }
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

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
