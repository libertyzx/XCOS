/**
 * @file        XC_Sch.c
 * @brief       调度器实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2026/01/07
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
#include "Internal/XC_List.h"
#include "Internal/XC_SchInternal.h"
#include "Internal/XC_TaskInternal.h"
#include "XC_Time.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 私有宏 */

/**
 * @brief   [私有]获取下个任务唤醒的时间
 * @return  XC_Tick_t    下个唤醒的Tick值;
 * @details 下个任务唤醒的时间为:时间表首节点唤醒时间;
 */
#define XCSch_GetNextTaskWakeupTick() (((XC_TaskHandle_t)(XCList_GetListStartNode(&phXCOS->TimeList)))->TaskWakeupTick)

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
 */
static void XCSch_TimeSched(XC_OSHandle_t phXCOS)
{
    XCListNode_t*   pIterator;
    XC_TaskHandle_t phTCB;
    XC_Tick_t       Tick;
    XCListNode_t*   pTimeList;

    pTimeList = &phXCOS->TimeList;
    Tick      = XCTime_GetTick(); // 得到当前系统Tick
    XCSch_Lock(phXCOS);           // 锁
    /**
     *  Tick时间溢出处理
     *  比较保存的Tick("PrevTick")和当前的Tick值,保存的值大于当前的值,则表示计数溢出
     *  溢出后处理如下:
     *  - 遍历时间表("TimeList"),更改节点状态,并将节点移动到就绪表("ReadyList");
     *  - 将时间溢出表("TimeOverflowList")的所有节点移动到时间表("TimeList");
     *  - 从时间表得到最新的下个唤醒时间(时间表首节点);
     */
    if(phXCOS->PrevTick > Tick) { // 保存的Tick大于当前Tick,时间溢出,需要处理
        // 时间表处理
        if(XCList_ListValid(pTimeList)) { // 时间表有节点
            // 所有节点移动到就绪表;
            pIterator = XCList_GetListStartNode(pTimeList); // 得到时间链表初始节点
            do {
                ((XC_TaskHandle_t)pIterator)->TaskState = XC_TASK_READY;    // 任务状态:就绪
                pIterator                               = pIterator->pNext; // 指向下个节点
            } while(!XCList_ReachEndNode(pTimeList, pIterator)); // 到达结尾则结束
            XCList_MoveListToNodeAfter(phXCOS->pPrevReadyNode, pTimeList); // 时间表所有节点移动到就绪表
        }
        // 时间溢出表处理(溢出表全部节点移动时间表)
        if(XCList_ListValid(&phXCOS->TimeOverflowList)) {                     // 时间溢出表有节点
            XCList_MoveListToNodeAfter(pTimeList, &phXCOS->TimeOverflowList); // 时间溢出表所有节点移动到时间表
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
     *      - 必须先另存任务TCB并立刻将迭代器移动至下个节点,
     *          因为当前节点会移动到就绪表,若是直接移动迭代器,链表指向就会错误;
     */
    if((XCList_ListValid(pTimeList)) && (Tick >= XCSch_GetNextTaskWakeupTick())) { // 时间表有效 && 任务唤醒时间已经到达
        /**轮询链表,将时间到达的节点都移动到就绪表 */
        pIterator = XCList_GetListStartNode(pTimeList);               // 得到时间链表初始节点
        while(Tick >= ((XC_TaskHandle_t)pIterator)->TaskWakeupTick) { // 唤醒时间到达
            phTCB     = (XC_TaskHandle_t)pIterator;                   // 得到当前任务的TCB
            pIterator = pIterator->pNext;                             // 指向下个任务(必须在操作链表前)
            XCTask_MoveTaskToReadyList(phTCB);                        // 唤醒任务,移到就绪表
            phTCB->TaskState = XC_TASK_READY;                         // 任务状态:就绪
            // 轮询结束判断
            if(XCList_ReachEndNode(pTimeList, pIterator)) { // 到达结尾
                break;                                      // 结束轮询
            }
        }
        phXCOS->PrevTick = Tick; // 更新保存Tick
    }
    XCSch_Unlock(phXCOS); // 解锁
}

/**
 * @brief       [私有]事件调度
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数用于:
 *      - 任务唤醒"XCTask_SendNotify"函数触发;
 *  注意:
 *      - 在调用此函数前必须先判断"phXCOS->EventProduced != phXCOS->EventConsumed";
 *      - 此函数的实现主要是循环搜索链表(时间表,溢出表,阻塞表),判断是否有需要处理的任务:
 */
static void XCSch_EventSched(XC_OSHandle_t phXCOS)
{
    XCListNode_t*   pList;
    XCListNode_t*   pIterator;
    XC_TaskHandle_t phTCB;

#define WAKEUP_NOTIFY(pCList)                                                                                                                                        \
    {                                                                                                                                                                \
        pList     = pCList;                          /*缓存链表*/                                                                                                    \
        pIterator = XCList_GetListStartNode(pList);  /*得到链表初始节点*/                                                                                            \
        if(!XCList_ReachEndNode(pList, pIterator)) { /*判断是否有节点*/                                                                                              \
            do {                                                                                                                                                     \
                if((((XC_TaskHandle_t)(pIterator))->NotifyState == XC_NOTIFY_WAIT) &&                                    /*是等待唤醒*/                              \
                   (((XC_TaskHandle_t)(pIterator))->NotifyProduced != ((XC_TaskHandle_t)(pIterator))->NotifyConsumed)) { /*需要消费*/                                \
                    /*是等待唤醒 && 需要消费*/                                                                                                                       \
                    ((XC_TaskHandle_t)(pIterator))->NotifyConsumed = ((XC_TaskHandle_t)(pIterator))->NotifyProduced; /*更新状态改变处理(已经唤醒,可以不用临时变量)*/ \
                    phTCB                                          = (XC_TaskHandle_t)(pIterator);                   /*得到当前任务TCB*/                             \
                    pIterator                                      = pIterator->pNext;                               /*指向下个节点(必须在操作前指向下个节点)*/      \
                    /** 通知唤醒 */                                                                                                                                  \
                    XCTask_MoveTaskToReadyList(phTCB);     /*任务移动到就绪表*/                                                                                      \
                    phTCB->NotifyState = XC_NOTIFY_WAKEUP; /*通知状态:通知唤醒*/                                                                                     \
                    phTCB->TaskState   = XC_TASK_READY;    /*任务状态:就绪*/                                                                                         \
                }                                                                                                                                                    \
                else {                                                                                                                                               \
                    pIterator = pIterator->pNext; /*指向下个节点*/                                                                                                   \
                }                                                                                                                                                    \
            } while(!XCList_ReachEndNode(pList, pIterator));                                                                                                         \
        }                                                                                                                                                            \
    }

    XCSch_Lock(phXCOS);                       // 锁
    WAKEUP_NOTIFY(&phXCOS->TimeList);         // "TimeList"延时/超时/等待的链表
    WAKEUP_NOTIFY(&phXCOS->TimeOverflowList); // "TimeOverflowList"时间溢出的链表
    WAKEUP_NOTIFY(&phXCOS->BlockedList);      // "BlockedList"阻塞链表
    XCSch_Unlock(phXCOS);                     // 解锁
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

    phXCOS->pPrevReadyNode = &phXCOS->ReadyList; // 上个就绪节点指向就绪表根
    phXCOS->PrevTick       = 0;                  // 保存上个Tick值
    phXCOS->TaskNum        = 0;                  // 任务数量
    phXCOS->EventProduced  = 0;                  // 任务调度触发(用于通知,挂起,恢复异步操作触发)
    phXCOS->EventConsumed  = 0;                  // 任务调度处理(用于通知,挂起,恢复异步操作触发后处理)
    phXCOS->fIdle          = NULL;               // 框架空闲处理

    XCList_Init(&phXCOS->ReadyList);
    XCList_Init(&phXCOS->TimeList);
    XCList_Init(&phXCOS->TimeOverflowList);
    XCList_Init(&phXCOS->BlockedList);
}

/**
 * @brief       [用户]调度器启动
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数无返回,阻塞运行;
 *  注意:阻塞下要是有看门狗,记得在任务中喂狗;
 */
void XCSch_Start(XC_OSHandle_t phXCOS)
{
    uint8_t         Trigger;
    XCListNode_t*   pIterator;
    XC_TaskHandle_t phTCB;
    XC_Tick_t       Tick;

    pIterator = XCList_GetListStartNode(&phXCOS->ReadyList); // 迭代器初始指向就绪表首节点;
    while(1) {
        if(XCList_ListValid(&phXCOS->ReadyList)) {                   // 就绪表有节点,处理
            if(XCList_ReachEndNode(&phXCOS->ReadyList, pIterator)) { // 到达结尾(是根节点)
                pIterator = pIterator->pNext;                        // 再次向下更新就绪节点
            }
            phXCOS->pPrevReadyNode = pIterator->pPrev;             // 得到上个就绪节点
            phTCB                  = (XC_TaskHandle_t)(pIterator); // 得到任务TCB
            pIterator              = pIterator->pNext;             // 向下更新就绪节点
            phTCB->TaskState       = XC_TASK_RUN;                  // 任务状态:运行
            phTCB->fTask(phTCB);                                   // 运行任务
        }

        XCSch_TimeSched(phXCOS); // 时间调度处理

        // 任务异步调度处理
        Trigger = phXCOS->EventProduced;
        if(Trigger != phXCOS->EventConsumed) {
            phXCOS->EventConsumed = Trigger;
            XCSch_EventSched(phXCOS);
        }

        // 判断就绪表是否有任务(空闲处理)
        if((XCList_ListValid(&phXCOS->ReadyList) == 0) && (phXCOS->fIdle != NULL)) {
            /** 就绪表没有节点 && 有空闲处理函数*/
            /**
             *  主要判断时间表和溢出表是否有效(1有效,0无效):
             *  - 时间表1,溢出表0: 取数据"XCSch_GetNextTaskWakeupTick()"(时间表首节点);
             *  - 时间表1,溢出表1: 取数据"XCSch_GetNextTaskWakeupTick()"(时间表首节点);
             *  - 时间表0,溢出表1: 取溢出表首节点唤醒时间;
             *  - 时间表0,溢出表0: 系统休眠;
             */
            Tick = XCTime_GetTick();                  // 得到当前Tick
            if(XCList_ListValid(&phXCOS->TimeList)) { // 时间表有节点
                Tick = XCSch_GetNextTaskWakeupTick() - Tick;
            }
            else if(XCList_ListValid(&phXCOS->TimeOverflowList)) { // 溢出表有节点
                Tick = ((XC_TaskHandle_t)(XCList_GetListStartNode(&phXCOS->TimeOverflowList)))->TaskWakeupTick - Tick;
            }
            else {         // 时间表,溢出表都没有节点;
                Tick = ~0; // 按最大时间休眠
            }
            phXCOS->fIdle(phXCOS, Tick); // 框架空闲处理
        }
    }
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
