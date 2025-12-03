/**
 * @file        XC_Sch.c
 * @brief       调度器实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2025/11/26
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     用于调度任务的调度器实现;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 头文件
#include "XC_Sch.h"
#include "Internal/XC_Internal.h"
#include "Internal/XC_List.h"
#include "XC_Time.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 外部函数 */

extern void XCTask_InsertTaskToTimeList(XCListNode_t* pList, XC_TaskHandle_t phTCB); // [内部]将任务插入到时间表

/************************************************ 我是分割线 ************************************************/
/** 私有宏 */

/**
 * @brief   [私有]获取下个任务唤醒的时间
 * @return  XC_Tick_t    下个唤醒的Tick值;
 * @details
 *  下个任务唤醒的时间为:时间表首节点唤醒时间;
 */
#define XCSch_GetNextTaskWakeTick() (((XC_TaskHandle_t)(XCList_GetListStartNode(&phXCOS->TimeList)))->TaskWakeTick)

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
 *  时间调度处理:
 *  - 处理系统Tick溢出后的任务调度(时间表移动到就绪表,溢出表移动到时间表);
 *  - 处理任务时间到达后的任务调度(到达任务移动到就绪表);
 *  注意:
 *  > 此函数只处理在时间表和溢出时间表的任务的调度;
 *  > 操作此函数需要"XCSch_Lock";
 */
static void XCSch_TimeSched(XC_OSHandle_t phXCOS)
{
    volatile XCListNode_t* pIterator;
    XC_TaskHandle_t        phTCB;
    XC_Tick_t              Tick;

    Tick = XCTime_GetTick(); // 得到当前系统Tick
    /**
     *  Tick时间溢出处理
     *  比较保存的Tick("PrevTick")和当前的Tick值,保存的值大于当前的值,则表示计数溢出
     *  溢出后处理如下:
     *  - 遍历时间表("TimeList"),更改节点装,并将节点移动到就绪表("ReadyList");
     *  - 将时间溢出表("TimeOverflowList")的所有节点移动到时间表("TimeList");
     *  - 从时间表得到最新的下个唤醒时间(时间表首节点);
     */
    if(phXCOS->PrevTick > Tick) { // 保存的Tick大于当前Tick,时间溢出,需要处理
        // 时间表处理
        if(XCList_ListValid(&phXCOS->TimeList)) { // 时间表有节点
            // 所有节点移动到就绪表;
            pIterator = XCList_GetListStartNode(&phXCOS->TimeList); // 得到时间链表初始节点
            do {
                ((XC_TaskHandle_t)pIterator)->TaskState = XC_TASK_READY;    // 任务状态:就绪
                ((XC_TaskHandle_t)pIterator)->WakeType  = XC_WAKE_TIME;     // 是时间唤醒
                pIterator                               = pIterator->pNext; // 指向下个节点
            } while(!XCList_ReachEndNode(&phXCOS->TimeList, pIterator)); // 到达结尾则结束
            XCList_MoveListToNodeBefore(phXCOS->pNextReadyNode, &phXCOS->TimeList); // 时间表所有节点移动到就绪表
        }
        // 时间溢出表处理(溢出表全部节点移动时间表)
        if(XCList_ListValid(&phXCOS->TimeOverflowList)) {                              // 时间溢出表有节点
            XCList_MoveListToNodeBefore(&phXCOS->TimeList, &phXCOS->TimeOverflowList); // 时间溢出表所有节点移动到时间表
        }
        phXCOS->PrevTick = Tick; // 更新保存Tick
    }

    /**
     *  时间表处理
     *  时间表("TimeList")有节点,且下个唤醒任务的时间到达,则调用;
     *  需要以下操作;
     *  - 遍历时间表("TimeList"),唤醒时间到达的任务都移动到就绪表("ReadyList");
     *  注意:
     *      - 必须先处理"Tick时间溢出处理";
     *      - 安全操作中,必须另存任务TCB并立刻将迭代器移动至下个节点,
     *          因为当前节点会移动到就绪表,若是直接移动迭代器,链表指向就会错误;
     */
    if((XCList_ListValid(&phXCOS->TimeList)) && (Tick >= XCSch_GetNextTaskWakeTick())) { // 时间表有效 && 任务唤醒时间已经到达
        pIterator = XCList_GetListStartNode(&phXCOS->TimeList);                          // 得到时间链表初始节点
        // 轮询链表,将时间到达的节点都移动到就绪表;
        while(Tick >= ((XC_TaskHandle_t)pIterator)->TaskWakeTick) { // 唤醒时间到达
            phTCB     = (XC_TaskHandle_t)pIterator;                 // 安全操作1,得到当前任务的TCB
            pIterator = pIterator->pNext;                           // 安全操作2,指向下个任务
            XCTask_MoveTaskToReadyList(phTCB);                      // 唤醒任务,移到就绪表
            phTCB->TaskState = XC_TASK_READY;                       // 任务状态:就绪
            phTCB->WakeType  = XC_WAKE_TIME;                        // 时间唤醒
            // 轮询结束判断
            if(XCList_ReachEndNode(&phXCOS->TimeList, pIterator)) { // 到达结尾
                break;                                              // 结束轮询
            }
        }
        phXCOS->PrevTick = Tick; // 更新保存Tick
    }
}

/**
 * @brief       [私有]阻塞调度
 * @param[in]   phXCOS  框架句柄
 * @param[in]   phTCB   有阻塞操作的任务TCB句柄
 * @details
 *  处理任务的阻塞状态;
 *  - 处理任务唤醒时间,由唤醒时间决定任务是进时间表还是时间溢出表;
 *  - 处理任务阻塞,挂起或者死等的任务进入阻塞表;
 *  注意:
 *  > 调用此函数前先判断函数是否需要阻塞处理;
 *  > 此调度函数主要是分配阻塞的任务;
 *  > 操作此函数需要"XCSch_Lock";
 */
static void XCSch_BlockedSched(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB)
{
    XC_Tick_t TaskWakeTick;
    XC_Tick_t Tick;

    Tick = XCTime_GetTick(); // 得到当前系统Tick
    /**
     *  处理任务操作中的延时,超时,阻塞;
     *  在任务中只是将需要延时或者超时的值做保存,在此处处理;
     *  延时或者超时的值加上当前的Tick后得到下次需要唤醒的Tick;
     *  但是要注意,计算下次唤醒的Tick时计数值可能会溢出,所以需要做溢出判断,
     *  将任务分别挂载到两个链表中:时间表("TimeList")和溢出表("TimeOverflowList");
     */
    if(phTCB->TaskWakeTick != 0) {                        // 有延时或超时时间("TaskWakeTick"!=0)
        TaskWakeTick        = phTCB->TaskWakeTick + Tick; // 得到此任务下次唤醒的的时刻
        phTCB->TaskWakeTick = TaskWakeTick;               // 保存下次唤醒时刻
        XCTask_RemoveTaskNode(phTCB);                     // 将当前节点从就绪表移除
        /**
         * 判断任务下个唤醒时刻是否是在Tick溢出后:
         *  (TaskWakeTick < Tick) ? 溢出 : 非溢出
         * 由溢出和非溢出选择任务入溢出表或者时间表;
         */
        XCTask_InsertTaskToTimeList((TaskWakeTick < Tick) ? (&phXCOS->TimeOverflowList) : (&phXCOS->TimeList), phTCB);
    }
    else {                                   // 无时间,阻塞处理
        XCTask_MoveTaskToBlockedList(phTCB); // 任务移动到阻塞表(任务挂起或者死等,进入阻塞表;)
    }
}

/**
 * @brief       [私有]任务调度
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数用于:
 *      - 任务唤醒XCTask_NotifySendfy"函数触发;
 *      - 任务挂起XCTask_Suspendnd"函数触发;
 *      - 任务挂起恢XCTask_Resumeume"函数触发;
 *  只有在以上3个函数在中断不安全的情况下被调用,才会触发运行此函数;
 *  注意:
 *  > 在调用此函数前必须先判断"phXCOS->TaskSchedTrigger != phXCOS->TaskSchedProcessed";
 *  > 此函数的实现主要是循环搜索链表(全部),判断是否有需要处理的任务:
 *  > 操作此函数需要"XCSch_Lock";
 */
static void XCSch_TaskSched(XC_OSHandle_t phXCOS)
{
    volatile uint8_t Trigger;
    volatile uint8_t StateChangeType;
    XCListNode_t*    pIterator;
    XCListNode_t*    pList[4];
    XC_TaskHandle_t  phTCB;
    uint8_t          i;

    pList[0] = &phXCOS->TimeList;         //"TimeList"延时/超时/等待的链表
    pList[1] = &phXCOS->TimeOverflowList; //"TimeOverflowList"时间溢出的链表
    pList[2] = &phXCOS->BlockedList;      //"BlockedList"阻塞链表
    pList[3] = &phXCOS->ReadyList;        //"ReadyList"就绪链表

    /**
     * 链表循环搜索判断
     */
    for(i = 0; i < 4; i++) {
        pIterator = XCList_GetListStartNode(pList[i]);      // 得到链表初始节点
        while(!XCList_ReachEndNode(pList[i], pIterator)) {  // 判断是否到达结尾
            phTCB           = (XC_TaskHandle_t)(pIterator); // 得到当前任务TCB
            pIterator       = pIterator->pNext;             // 指向下个节点(必须在操作前指向下个节点)
            StateChangeType = phTCB->StateChangeType;       // 原子操作,状态改变的类型
            Trigger         = phTCB->StateChangeTrigger;    // 原子操作,状态改变触发
            /** 优先处理挂起和挂起恢复,再处理通知 */
            if(Trigger != phTCB->StateChangeProcessed) { // 状态被改变,挂起挂起恢复处理
                phTCB->StateChangeProcessed = Trigger;   // 更新状态改变处理
                if(StateChangeType == XC_TASK_VOID) {
                    /** 任务恢复 */
                    XCTask_MoveTaskToReadyList(phTCB); // 任务移动到就绪表
                    phTCB->WakeType  = XC_WAKE_RESUME; // 被任务恢复唤醒
                    phTCB->TaskState = XC_TASK_READY;  // 任务状态:就绪
                }
                else {
                    /** 任务挂起 */
                    phTCB->TaskState = XC_TASK_SUSPEND;  // 任务状态:挂起
                    XCTask_MoveTaskToBlockedList(phTCB); // 任务移动到阻塞表
                }
            }
            else {                                      // 通知处理
                Trigger = phTCB->NotifyTrigger;         // 原子操作,通知触发
                if(Trigger != phTCB->NotifyProcessed) { // 状态被改变
                    phTCB->NotifyProcessed = Trigger;   // 更新状态改变处理
                    /** 通知唤醒 */
                    XCTask_MoveTaskToReadyList(phTCB); // 任务移动到就绪表
                    phTCB->WakeType  = XC_WAKE_NOTIFY; // 被通知唤醒
                    phTCB->TaskState = XC_TASK_READY;  // 任务状态:就绪
                }
            }
        }
    }
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
 * @details
 *  用于初始化框架的调度器;
 *  任务启动默认就是调度锁定状态;
 */
void XCSch_Init(XC_OSHandle_t phXCOS)
{
    /** 初始第一步为锁,防止出现以为中断调度的情况; */
    phXCOS->Lock = 1; // 锁(1锁,0解锁),用于中断处理

    phXCOS->pNextReadyNode     = &phXCOS->ReadyList; // 下个就绪节点指向就绪表根
    phXCOS->PrevTick           = 0;                  // 保存上个Tick值
    phXCOS->TaskNum            = 0;                  // 任务数量
    phXCOS->TaskSchedTrigger   = 0;                  // 任务调度触发(用于通知,挂起,恢复异步操作触发)
    phXCOS->TaskSchedProcessed = 0;                  // 任务调度处理(用于通知,挂起,恢复异步操作触发后处理)
    phXCOS->fIdle              = NULL;               // 框架空闲处理

    XCList_Init(&phXCOS->ReadyList);
    XCList_Init(&phXCOS->TimeList);
    XCList_Init(&phXCOS->TimeOverflowList);
    XCList_Init(&phXCOS->BlockedList);
}

/**
 * @brief       [用户]调度器启动(阻塞)
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数无返回,阻塞运行;
 *  > 注意:阻塞下要是有看门狗,记得在任务中喂狗;
 */
void XCSch_Start(XC_OSHandle_t phXCOS)
{
    volatile uint8_t Trigger;
    XCListNode_t*    pIterator; // 节点迭代器
    XC_TaskHandle_t  phTCB;
    XC_Tick_t        Tick;

    pIterator = &phXCOS->ReadyList; // 就绪节点指向就绪表根
    XCSch_Lock(phXCOS);             // 锁
    do {
        if(XCList_ListValid(&phXCOS->ReadyList)) {                   // 就绪表有节点,处理
            if(XCList_ReachEndNode(&phXCOS->ReadyList, pIterator)) { // 到达结尾(是根节点)
                pIterator = pIterator->pNext;                        // 再次向下更新就绪节点
            }
            phTCB                  = (XC_TaskHandle_t)(pIterator); // 得到任务TCB
            pIterator              = pIterator->pNext;             // 向下更新就绪节点
            phXCOS->pNextReadyNode = pIterator;                    // 得到下个就绪节点
            phTCB->TaskState       = XC_TASK_RUN;                  // 任务状态:运行
            XCSch_Unlock(phXCOS);                                  // 解锁
            phTCB->fTask(phTCB);                                   // 运行任务
            XCSch_Lock(phXCOS);                                    // 锁
            // 任务阻塞处理
            if(phTCB->TaskState >= XC_TASK_DELAY) {
                XCSch_BlockedSched(phXCOS, phTCB); // 当前任务阻塞调度处理
            }
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
             *  - 时间表1,溢出表0: 取数据"XCSch_GetNextTaskWakeTick()"(时间表首节点);
             *  - 时间表1,溢出表1: 取数据"XCSch_GetNextTaskWakeTick()"(时间表首节点);
             *  - 时间表0,溢出表1: 取溢出表首节点唤醒时间;
             *  - 时间表0,溢出表0: 系统休眠;
             */
            Tick = XCTime_GetTick();                  // 得到当前Tick
            if(XCList_ListValid(&phXCOS->TimeList)) { // 时间表有节点
                Tick = XCSch_GetNextTaskWakeTick() - Tick;
            }
            else if(XCList_ListValid(&phXCOS->TimeOverflowList)) { // 溢出表有节点
                Tick = ((XC_TaskHandle_t)(XCList_GetListStartNode(&phXCOS->TimeOverflowList)))->TaskWakeTick - Tick;
            }
            else {         // 时间表,溢出表都没有节点;
                Tick = ~0; // 按最大时间休眠
            }
            XCSch_Unlock(phXCOS);        // 解锁
            phXCOS->fIdle(phXCOS, Tick); // 框架空闲处理
            XCSch_Lock(phXCOS);          // 锁
        }
    } while(1);
}

/**
 * @brief       [用户]获取任务数
 * @param[in]   phXCOS  框架句柄
 * @return      uint8_t 返回任务数量
 * @details     直接获取框架句柄中的任务数字段;
 */
uint8_t XCSch_GetTaskNum(XC_OSHandle_t phXCOS)
{
    return (phXCOS->TaskNum);
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       [用户]设置空闲处理回调
 * @param[in]   phXCOS  框架句柄
 * @param[in]   fIdle  框架空闲处理回调
 * @details
 *  用于设置框架空闲处理回调;
 *  若是需要清除回调则"fIdle"值为NULL即可;
 */
void XCSch_SetIdleCallback(XC_OSHandle_t phXCOS, void (*fIdle)(XC_OSHandle_t, XC_Tick_t))
{
    phXCOS->fIdle = fIdle;
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
