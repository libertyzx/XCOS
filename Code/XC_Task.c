/**
 * @file        XC_Task.c
 * @brief       任务的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2026/01/07
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     实现了任务处理相关的操作
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
/** 内部处理 */

/**
 * @brief       [私有]任务基本初始化
 * @param[in]   phTCB   任务控制块
 * @details
 *  用户不调用;
 *  "TCB"中以下数据会被初始化:
 *      - "BP"              协程断点
 *      - "TaskWakeupTick"  任务下个唤醒的时间
 *      - "pNotifyData"     通知数据
 *      - "NotifyState"     通知状态
 *      - "NotifyProduced"  通知-生产者
 *      - "NotifyConsumed"  通知-消费者
 *  在任务注册,移除,复位,添加时调用
 */
static void XC_Task_BasicInit(XC_TaskHandle_t phTCB)
{
    COR_Init(phTCB->BP);                      // 初始化断点
    phTCB->TaskWakeupTick = ~0U;              // 任务下个唤醒的时间(初始设置最大)
    phTCB->pNotifyData    = NULL;             // 通知数据清零
    phTCB->NotifyState    = XC_NOTIFY_WAKEUP; // 通知状态:通知唤醒(没有通知)
    // 清除通知
    phTCB->NotifyConsumed = phTCB->NotifyProduced;
}

/**
 * @brief       [私有]添加任务内部实现
 * @param[in]   phTCB 协程控制块
 * @details
 *  除"phXCOS","fTask","pParam"外的其他参数处理;
 *  调用此函数前需要先调用"phTCB->TaskState = XC_TASK_VOID;"任务状态清除;
 */
static void XC_Task_AddInternal(XC_TaskHandle_t phTCB)
{
    XC_Task_BasicInit(phTCB);                                                 // 基础初始化
    XC_Sch_Lock(phTCB->phXCOS);                                               // 锁
    XC_List_InsertNodeAfter(phTCB->phXCOS->pPrevReadyNode, &phTCB->ListNode); // 插入就续表
    XC_Sch_Unlock(phTCB->phXCOS);                                             // 解锁
    phTCB->TaskState = XC_TASK_READY;                                         // 任务更新状态为就绪
    phTCB->phXCOS->TaskNum++;                                                 // 任务数+1
}

/**
 * @brief       [内部]移除处理
 * @param[in]   phTCB 协程控制块
 * @details
 *  用于协程内部"移除自身"和"任务移除"使用;
 *  移除任务不会清除以下数据:
 *  - "fTask"   任务入口
 *  - "pParam"  任务参数
 */
void XC_Task_HandleRemove(XC_TaskHandle_t phTCB)
{
    phTCB->TaskState = XC_TASK_VOID; // 任务状态改为空
    XC_Sch_Lock(phTCB->phXCOS);      // 锁
    XC_Task_RemoveNode(phTCB);       // 移除任务节点
    XC_Sch_Unlock(phTCB->phXCOS);    // 解锁
    XC_Task_BasicInit(phTCB);        // 基本数据初始化
    phTCB->phXCOS->TaskNum--;        // 任务数-1
    phTCB->phXCOS = NULL;            // 清除任务的所属框架句柄
}

/**
 * @brief       [内部]复位处理
 * @param[in]   phTCB       任务句柄
 * @details     用于协程内部"复位自身"和"复位任务"使用;
 */
void XC_Task_HandleReset(XC_TaskHandle_t phTCB)
{
    phTCB->TaskState = XC_TASK_READY; // 任务更新状态为就绪
    /**这里不判断是否在就绪表,直接移动*/
    XC_Sch_Lock(phTCB->phXCOS);     // 锁
    XC_Task_MoveToReadyList(phTCB); // 任务移动到就绪表
    XC_Sch_Unlock(phTCB->phXCOS);   // 解锁
    XC_Task_BasicInit(phTCB);       // 基本数据初始化
}

/**
 * @brief       [内部]挂起处理
 * @param[in]   phTCB       任务句柄
 * @details     用于协程内部"挂起自身"和"挂起任务"使用;
 */
void XC_Task_HandleSuspend(XC_TaskHandle_t phTCB)
{
    phTCB->TaskState = XC_TASK_SUSPEND; // 任务状态:挂起
    XC_Sch_Lock(phTCB->phXCOS);         // 锁
    XC_Task_MoveToBlockedList(phTCB);   // 将任务移动到阻塞表
    XC_Sch_Unlock(phTCB->phXCOS);       // 解锁
}

/**
 * @brief       [私有]阻塞处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details     处理阻塞任务("等待通知"和"延时"的最终处理函数)
 */
static void XC_Task_HandleBlocking(XC_TaskHandle_t phTCB, uint32_t TickCount)
{
    XC_Tick_t     Tick;
    XCListNode_t* pIterator;
    XCListNode_t* pList;

    if(TickCount != 0U) {                  // 有延时或超时时间("TickCount"!=0)
        Tick = XC_Time_GetTick();          // 得到当前系统Tick
        TickCount += Tick;                 // 得到此任务下次唤醒的的时刻
        phTCB->TaskWakeupTick = TickCount; // 保存下次唤醒时刻
        /**
         *  判断任务下个唤醒时刻是否是在Tick溢出后:
         *      (TickCount < Tick) ? 溢出 : 非溢出
         *  由溢出和非溢出选择任务入溢出表或者时间表;
         */
        pList = (TickCount < Tick) ? (&phTCB->phXCOS->TimeOverflowList) : (&phTCB->phXCOS->TimeList); // 得到链表
        XC_Sch_Lock(phTCB->phXCOS);                                                                   // 锁
        /**
         *  插入时间表按升序排列;
         *  从根节点向下(Next)查询"任务下个唤醒的时间"(TaskWakeupTick),根据查询值从小到大排列;
         *  保证(指向节点的值 <= 新节点的值),确保新的阻塞任务在表的最后;
         */
        pIterator = XC_List_GetListStartNode(pList); // 获取链表的首节点
        while((pIterator != pList) && (XC_LIST_TO_TCB(pIterator)->TaskWakeupTick <= TickCount)) {
            /** 指向节点不是根节点 && (指向节点的值 <= 新节点的值) */
            pIterator = pIterator->pNext; // 指向下个节点
        }
        /**
         *  移动节点是移动到某个节点的后面,搜索结束时迭代器已经指向下个节点,
         *  所以移动点是迭代器的上个节点,在迭代器的上个节点的下个节点插入新节点;
         *  即为,新节点插入在迭代器的上个节点;
         */
        XC_List_MoveNodeAfter(pIterator->pPrev, &phTCB->ListNode);
    }
    else {                                // 无时间,阻塞处理
        XC_Sch_Lock(phTCB->phXCOS);       // 锁
        XC_Task_MoveToBlockedList(phTCB); // 任务移动到阻塞表(任务挂起或者死等,进入阻塞表;)
    }
    XC_Sch_Unlock(phTCB->phXCOS); // 解锁
}

/**
 * @brief       [内部]等待通知处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details
 *  用于协程内部"等待通知"处理;
 *  被发送通知唤醒后通知计数会被清零;
 *  **注意**:
 *  - 此函数配合"XCTask_SendNotify"函数,需要一起协同;
 */
void XC_Task_HandleWaitNotify(XC_TaskHandle_t phTCB, uint32_t TickCount)
{
    /**
     *  先锁
     *  在"XC_NOTIFY_WAIT"状态前在"通知唤醒"->不进入等待通知
     *  在"XC_NOTIFY_WAIT"状态后在"通知唤醒"->异步调度触发;
     */
    XC_Sch_Lock(phTCB->phXCOS); // 锁
    /*语句[1]前中断,"异步调度"不会触发,只增加通知生产者*/
    phTCB->NotifyState = XC_NOTIFY_WAIT; // [1]通知状态:等待通知
    /**
     *  语句[1]后中断,"异步调度"会触发;
     *  若是中断了,这里直接处理消费者,但是注意因为框架是协同调度,只要等待通知了,必定会触发调度;
     *  注意:这里处理了消费者,异步调度还是会被触发,但异步调度性能会有略微提升;
     *  这里不需要临时变量"NotifyProduced"存放生产者,只要唤醒了就可以直接全部消费;
     */
    if(phTCB->NotifyProduced != phTCB->NotifyConsumed) { // 出现通知
        phTCB->NotifyConsumed = phTCB->NotifyProduced;   // 清除通知
        phTCB->NotifyState    = XC_NOTIFY_WAKEUP;        // 通知状态:通知唤醒
        XC_Sch_Unlock(phTCB->phXCOS);                    // 解锁
        phTCB->TaskState = XC_TASK_READY;                // 任务状态:就绪
    }
    else {
        /**语句[1]后没有中断,这里直接处理阻塞 */
        XC_Task_HandleBlocking(phTCB, TickCount); // 处理阻塞
        phTCB->TaskState = XC_TASK_BLOCKED;       // 任务状态:阻塞
        /*"XC_Task_HandleBlocking"中已经解锁*/
    }
}

/**
 * @brief       [内部]延时处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details     用于协程内部"延时"处理
 */
void XC_Task_HandleDelay(XC_TaskHandle_t phTCB, uint32_t TickCount)
{
    if(TickCount == 0U) {
        phTCB->TaskState = XC_TASK_READY; // 任务状态:就绪
    }
    else {
        phTCB->TaskState = XC_TASK_BLOCKED;       // 任务状态:阻塞
        XC_Task_HandleBlocking(phTCB, TickCount); // 处理阻塞
    }
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 任务注册相关 */

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
 *  **不可多次注册同个任务(没有做重复判断,会调度错误)**
 *  任务注册完成后会挂载到就绪表;
 */
XC_Return_t XC_Task_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)
{
    if(phXCOS->TaskNum >= XC_CFG_MAX_TASKS) {
        return (XC_FAIL);
    }

    phTCB->TaskState = XC_TASK_VOID; // 任务状态清除
    phTCB->phXCOS    = phXCOS;       // 保存任务的所属框架句柄
    phTCB->fTask     = fTask;        // 更新任务入口
    phTCB->pParam    = pParam;       // 传递给任务的参数
    XC_Task_AddInternal(phTCB);

    return (XC_OK);
}

/**
 * @brief       [用户]任务移除
 * @param[in]   phTCB 协程控制块
 * @details
 *  会从链表中删除,并清空TCB数据;
 *  不可移除自身,移除自身使用"XC_Remove()"函数;
 */
void XC_Task_Remove(XC_TaskHandle_t phTCB)
{
    if(phTCB->phXCOS != NULL) {
        XC_Task_HandleRemove(phTCB);
    }
}

/************************************************ 我是分割线 ************************************************/
/**
 * 任务分步创建相关的用户函数
 *  "XCTask_SetEntry"和"XCS_AddTask"函数需要配合使用;
 *  在"XCTask_SetEntry"设置完成后在XCTask_Addask"添加任务;
 *  注意:不要重复添加任务;
 */

/**
 * @brief       [用户]设置任务入口
 * @param[in]   phTCB   协程任务控制块
 * @param[in]   fTask   任务的函数指针(任务入口)
 * @param[in]   pParam  传递给任务的参数
 * @details
 *  只设置任务入口和传递给任务的参数;
 *  一般配合"XCTask_Add"使用;
 */
void XC_Task_SetEntry(XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)
{
    phTCB->TaskState = XC_TASK_VOID; // 任务状态清除
    phTCB->fTask     = fTask;        // 更新任务入口
    phTCB->pParam    = pParam;       // 传递给任务的参数
}

/**
 * @brief   [用户]添加任务
 * @param   phXCOS  框架句柄
 * @param   phTCB   协程任务控制块
 * @return  XC_Return_t
 * @retval  XC_OK :      注册成功
 * @retval  XC_FAIL :    注册失败,任务太多或者任务参数错误
 * @details
 *  添加的任务必须先调用"XCTask_SetEntry";
 *  设置好任务入口和传递的参数才可添加;
 *  添加后挂载到就绪表;
 */
XC_Return_t XC_Task_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB)
{
    if((phXCOS->TaskNum >= XC_CFG_MAX_TASKS) || (phTCB->fTask == NULL)) {
        return (XC_FAIL);
    }

    phTCB->phXCOS = phXCOS;     // 保存任务的所属框架句柄
    XC_Task_AddInternal(phTCB); // 添加任务
    return (XC_OK);
}

/************************************************ 我是分割线 ************************************************/
/** 任务复位 | 挂起 | 恢复 */

/**
 * @brief       [用户]任务复位
 * @param[in]   phTCB   协程控制块
 * @details
 *  **不可复位自身(复位自身使用"XC_Reset")**
 *  会清除所有状态(包含挂起),任务复位;
 *  任务将重置到就绪表,然后从头运行;
 *  任务TCB中除了"phXCOS","fTask","Param"其他全部重置;
 */
void XC_Task_Reset(XC_TaskHandle_t phTCB)
{
    if(phTCB->TaskState != (uint8_t)XC_TASK_VOID) {
        XC_Task_HandleReset(phTCB);
    }
}

/**
 * @brief       [用户]任务挂起
 * @param[in]   phTCB   任务控制块
 * @return      XC_Return_t
 * @retval      XC_OK :          挂起成功
 * @retval      XC_FAIL:         失败(任务不存在)
 * @retval      XC_CONTINUE :    异步操作中
 * @details
 *  将任务挂起,**本次任务运行完成后暂停任务**;
 *  需要在任务中立刻挂起,可以在协程块中调用"XC_Suspend";
 *  挂起可以覆盖任务的所有阻塞状态,优先级最高;
 *  挂起后只能被挂起恢复唤醒;
 */
XC_Return_t XC_Task_Suspend(XC_TaskHandle_t phTCB)
{
    /** 挂起操作只有在任务存在的时候才能运行 */
    if(phTCB->TaskState == (uint8_t)XC_TASK_VOID) {
        return (XC_FAIL);
    }
    else if(phTCB->TaskState == (uint8_t)XC_TASK_SUSPEND) {
        return (XC_OK); // 已经被挂起
    }

    XC_Task_HandleSuspend(phTCB);
    return (XC_OK);
}

/**
 * @brief       [用户]任务挂起恢复
 * @param[in]   phTCB   任务控制块
 * @return      XC_Return_t
 * @retval      XC_OK :          恢复成功
 * @retval      XC_FAIL:         失败(任务未挂起)
 * @details
 *  只能恢复被挂起的任务;
 *  任务唤醒后原先的阻塞将失效,任务状态为"XC_TASK_RESUME";
 */
XC_Return_t XC_Task_Resume(XC_TaskHandle_t phTCB)
{
    // 任务没有被挂起
    if(phTCB->TaskState != (uint8_t)XC_TASK_SUSPEND) {
        return (XC_FAIL); // 没有被挂起
    }

    XC_Sch_Lock(phTCB->phXCOS);       // 锁
    XC_Task_MoveToReadyList(phTCB);   // 将任务移动到就绪表
    XC_Sch_Unlock(phTCB->phXCOS);     // 解锁
    phTCB->TaskState = XC_TASK_READY; // 任务状态:就绪
    return (XC_OK);
}

/************************************************ 我是分割线 ************************************************/
/**
 *  任务通知的用户函数
 * ---
 *  因为框架不做开关中断,也不做原子操作(编译器特性等),
 *  所以目前任务同步只支持"通知",且只支持"SPSC"(单生产者单消费者,一个任务支持一个SCSP)模型;
 *  另外若是CPU是"对内存传输进行重排序"的话,用户还需要处理内存屏障;
 */

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
 *  可以在"等待通知"前发送通知,这样"等待通知"将不阻塞,立刻结束;
 *  被发送通知唤醒后通知计数会被清零;
 *  **注意**:
 *  - 一个任务对应一个通知处理;
 *  - 每个任务支持单生产者单消费者模型(SPSC)
 *  - 此函数为生产者,必须为单生产者;
 *      也就是说此函数运行时不能再被中断或者任务再次调用(每个任务都可以有一个,满足SPSC);
 *  - 若是有可能出现多次调用(多生产者),需要用户加锁保护;
 *  - 此函数不包含自锁,因为是通用C语言编写,不增加编译器和平台指令,也不关中断,
 *      这表示锁将不是原子操作,在特定的时刻必定出错,所以这里不在增加锁;
 *  - 此函数配合"XCTask_HandleWaitNotify"函数,需要一起协同;
 */
XC_Return_t XC_Task_SendNotify(XC_TaskHandle_t phTCB, void* pNotifyData)
{
    /**
     *  说明
     *  # "任务不存在"和"任务挂起"的情况下,发送通知无效,返回失败;
     *  # 发送通知时的可能状态:
     *   - 任务在不进入等待通知状态下收到通知,只做记录,状态为:
     *       - 任务非等待通知状态
     *       - 注意:在此状态下,等待通知处理完成后需要再次判断通知状态,防止中间态被中断;
     *   - 任务在进入等待通知时需要判断框架是否在调度中:
     *       - 调度中:标记异步操作;
     *       - 非调度中:直接调度;
     */

    if((phTCB->TaskState == (uint8_t)XC_TASK_VOID) || (phTCB->TaskState == (uint8_t)XC_TASK_SUSPEND)) {
        /** 任务不存在 || 任务被挂起 */
        return (XC_FAIL);
    }

    phTCB->pNotifyData = pNotifyData; // 通知数据

    if(phTCB->NotifyState != (uint8_t)XC_NOTIFY_WAIT) {
        /** 不是等待通知状态 */
        if((phTCB->NotifyProduced + 1U) != phTCB->NotifyConsumed) {
            phTCB->NotifyProduced++; // 通知-生产者,生产者未满则+1
        }
        return (XC_CONTINUE);
    }

    /** 必须是等待通知唤醒 */
    if(XC_Sch_GetLockState(phTCB->phXCOS)) {
        /** 调度运行中-异步,只会在中断中出现 */
        if((phTCB->NotifyProduced + 1U) != phTCB->NotifyConsumed) {
            phTCB->NotifyProduced++; // 通知-生产者,生产者未满则+1
        }
        if((phTCB->phXCOS->EventProduced + 1U) != phTCB->phXCOS->EventConsumed) {
            phTCB->phXCOS->EventProduced++; // 异步调度触发,生产者未满则+1
        }
        return (XC_CONTINUE);
    }

    /** 调度没有运行 */
    XC_Sch_Lock(phTCB->phXCOS);            // 锁
    XC_Task_MoveToReadyList(phTCB);        // 移动到就绪表
    XC_Sch_Unlock(phTCB->phXCOS);          // 解锁
    phTCB->NotifyState = XC_NOTIFY_WAKEUP; // 通知状态:通知唤醒
    phTCB->TaskState   = XC_TASK_READY;    // 任务状态:就绪
    return (XC_OK);
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
