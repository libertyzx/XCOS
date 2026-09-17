/**
 * @file        XC_Task.c
 * @brief       任务的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
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
#include "XC_Task.h"
#include "Internal/XC_List.h"
#include "Internal/XC_TaskInternal.h"
#include "XC_Diag.h" //诊断能力(断言/运行统计) + 错误上报宏(见其内含的 XC_Err.h)
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
 *      - "NotifyPending"   通知-待处理标志
 *  在任务注册,移除,复位,添加时调用
 */
static void XC_Task_BasicInit(XC_TaskHandle_t phTCB)
{
    COR_Init(phTCB->BP);                      // 初始化断点
    phTCB->TaskWakeupTick = ~0U;              // 任务下个唤醒的时间(初始设置最大)
    phTCB->pNotifyData    = NULL;             // 通知数据清零
    phTCB->NotifyState    = XC_NOTIFY_WAKEUP; // 通知状态:通知唤醒(没有通知)
    // 清除通知(待处理标志清0; 生产侧只写1/消费侧只写0,此处为消费侧)
    phTCB->NotifyPending = 0U;
#if (XC_CFG_COR_NESTING != 0)
    phTCB->CorDepth      = 0U; // 清协程深度(pCorStack/CorDepthMax 保留)
#endif
}

/* 协程嵌套: 帧入栈/出栈 (仅 `XC_CFG_COR_NESTING != 0`; 关闭时本段不编译) */
#if (XC_CFG_COR_NESTING != 0)
/**
 * @brief       [内部]协程帧入栈
 * @param[in]   phTCB     任务句柄
 * @param[in]   fn        子协程函数入口
 * @param[in]   RetLine   父层返回点(ANSI:行号 / GNU:标签地址)
 * @details
 *  仅由"XC_Cor_Call"调用:保存父层入口+返回点,入口切换为子函数;
 *  不做真实 C 嵌套调用(C 栈 O(1)),由调度器下一轮进入子层;
 *  越界保护(未配栈调用 Call / 栈容量不足)统一复位任务安全降级;
 *  **可诊断**: 越界分支会先"错误上报"再降级 —— 打开 `XC_CFG_ERR_HOOK` 时
 *  调用用户钩子 `XC_Err_Hook(phTCB, XC_ERR_FRAME_OVERFLOW)`, 钩子内可读到出错现场
 *  (`CorDepth`/`CorDepthMax`/`fTask`); 开关默认关闭 ⇒ 上报为**空操作, 0 开销**;
 */
void XC_Task_PushFrame(XC_TaskHandle_t phTCB, XC_CorFn_t fn, XC_BP_t RetLine)
{
    /**
     *  越界保护:同一分支覆盖两种情形——
     *  1. 未设栈调用 Call:CorDepthMax==0U 且 pCorStack==NULL(用户编程错误);
     *  2. 栈容量不足:CorDepth 达到 CorDepthMax(嵌套过深);
     *  处理策略:复位任务安全降级(HandleReset 内部清栈并恢复顶层入口 fTask=[0].pfn);
     *  错误上报:先上报再降级 —— 打开 XC_CFG_ERR_HOOK 时调用用户钩子
     *  (现场未破坏:钩子内 CorDepth/CorDepthMax 可区分上述两种情形);关闭时为空操作;
     */
    if(phTCB->CorDepth >= phTCB->CorDepthMax) {
        XC_Err_Report(phTCB, XC_ERR_FRAME_OVERFLOW); // 错误上报(开关关闭时为空操作)
        XC_Task_HandleReset(phTCB);
        return;
    }
    XC_CorFrame_t* pf = &phTCB->pCorStack[phTCB->CorDepth];
    pf->pfn           = phTCB->fTask; // 父层入口(弹帧恢复用;栈底帧恒为顶层任务)
    pf->BP            = RetLine;      // 父层返回点(ANSI:行号 / GNU:标签地址)
    phTCB->fTask      = fn;           // 入口切换为子函数(下轮/切父循环进入)
    COR_Init(phTCB->BP);              // 子层首次从头(ANSI:0 / GNU:NULL);用 COR_Init 跨版本统一
    phTCB->CorDepth++;
}

/**
 * @brief       [内部]协程帧出栈
 * @param[in]   phTCB   [XC_TaskHandle_t]任务句柄
 * @details
 *  弹帧恢复父层入口与返回点(调度器同轮切父使用);
 *  仅当"CorDepth > 0U"时调用(由调度器保证);
 */
void XC_Task_PopFrame(XC_TaskHandle_t phTCB)
{
    phTCB->CorDepth--;                                    // 先减后取(弹出当前层)
    phTCB->fTask = phTCB->pCorStack[phTCB->CorDepth].pfn; // 恢复父层入口
    phTCB->BP    = phTCB->pCorStack[phTCB->CorDepth].BP;  // 恢复父层返回点
}
#endif

/**
 * @brief       [私有]添加任务内部实现
 * @param[in]   phTCB 协程控制块
 * @details
 *  除"phXCOS","fTask","pParam"外的其他参数处理;
 *  调用此函数前需要先调用"phTCB->TaskState = XC_TASK_VOID;"任务状态清除;
 */
static void XC_Task_AddInternal(XC_TaskHandle_t phTCB)
{
    XC_Task_BasicInit(phTCB); // 基础初始化
#if (XC_CFG_TASK_STATS != 0)
    XC_Diag_ClrRunStats(phTCB); // 运行统计清零(新生命周期)
#endif
    XC_Core_Lock(phTCB->phXCOS);          // 锁
    XC_Task_InsertToReadyListTail(phTCB); // 插入就绪表尾(统一排队尾, 新任务不插队)
    XC_Core_Unlock(phTCB->phXCOS);        // 解锁
    phTCB->TaskState = XC_TASK_READY;     // 任务更新状态为就绪
    phTCB->phXCOS->TaskNum++;             // 任务数+1
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
    XC_Core_Lock(phTCB->phXCOS);     // 锁
    XC_Task_RemoveNode(phTCB);       // 移除任务节点(内部会"自环化")
    {
        /* 节点标记: 出表后置 NULL, **不再保留"自环"**
         *  - "自环" = 调度器摘出运行中任务后的"游离"标记, 语义是"让出后需插回就绪表";
         *  - 已移除任务与该语义相反(永不回表) ⇒ 置 NULL 以示区分:
         *    调度器归位处只判"自环"即可排除已移除任务(省一次状态判定, 见 "XC_Sch.c" 归位处);
         *  - 约束: 已移除任务的节点**不得再被任何链表操作触碰**
         *    (移表类入口均有"任务不存在"拦截: Suspend/Reset/Remove/SendNotify 判 VOID; Resume 要求 SUSPEND) */
        phTCB->ListNode.pNext = NULL;
        phTCB->ListNode.pPrev = NULL;
    }
    XC_Core_Unlock(phTCB->phXCOS); // 解锁
    /* 清栈并恢复最外层入口(栈底帧 [0].pfn 恒为最外层) */
#if (XC_CFG_COR_NESTING != 0)
    if(phTCB->CorDepth > 0U) {
        phTCB->fTask = phTCB->pCorStack[0].pfn;
    }
    phTCB->CorDepth = 0U;
#endif
    XC_Task_BasicInit(phTCB); // 基本数据初始化
#if (XC_CFG_TASK_STATS != 0)
    XC_Diag_ClrRunStats(phTCB); // 运行统计清零(移除=生命周期结束)
#endif
    phTCB->phXCOS->TaskNum--; // 任务数-1
    phTCB->phXCOS = NULL;     // 清除任务的所属框架句柄
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
    XC_Core_Lock(phTCB->phXCOS);        // 锁
    XC_Task_MoveToReadyListTail(phTCB); // 移动到就绪表尾(统一排队尾; 复位不插队)
    XC_Core_Unlock(phTCB->phXCOS);      // 解锁
    /* 清栈并恢复最外层入口(栈底帧 [0].pfn 恒为最外层) */
#if (XC_CFG_COR_NESTING != 0)
    if(phTCB->CorDepth > 0U) {
        phTCB->fTask = phTCB->pCorStack[0].pfn;
    }
    phTCB->CorDepth = 0U;
#endif
    XC_Task_BasicInit(phTCB); // 基本数据初始化
}

/**
 * @brief       [内部]挂起处理
 * @param[in]   phTCB       任务句柄
 * @details     用于协程内部"挂起自身"和"挂起任务"使用;
 */
void XC_Task_HandleSuspend(XC_TaskHandle_t phTCB)
{
    XC_Core_Lock(phTCB->phXCOS);        // 锁(状态写与移表同锁域: 防中断 SendNotify 直接路径并发覆盖)
    phTCB->TaskState = XC_TASK_SUSPEND; // 任务状态:挂起(优先级最高: 任何唤醒路径见此值均不移表)
    XC_Task_MoveToBlockedList(phTCB);   // 将任务移动到阻塞表
    XC_Core_Unlock(phTCB->phXCOS);      // 解锁
}

/**
 * @brief       [私有]阻塞处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details     处理阻塞任务("等待通知"和"延时"的最终处理函数)
 *
 *  **锁契约(务必遵守)**: 本函数按"锁范围最小化"设计 ——
 *  - 内部**自行 Lock/Unlock**(两条分支都保证解锁) ⇒ **可被"已持锁"的调用者调用**;
 *  - "XC_Task_HandleWaitNotify" 正是这样用的: 它先 Lock,再在 else 分支进入本函数,
 *    由本函数内部的 Unlock **隐式释放外层锁**(锁是单一标志位、不可重入,故为"隐式配对");
 *  - 因此: ①**不得**新增"提前 return 且不解锁"的分支; ②**不得**改变两条分支的解锁保证;
 *    ③若要把所有权显式化,应改为传参/拆函数(显式声明"是否代为解锁"),否则一旦失配,锁会**永久为 1**
 *    ⇒ 所有中断通知退化为"只登记"(直接唤醒静默失效,症状分散、极难定位)。
 */
static void XC_Task_HandleBlocking(XC_TaskHandle_t phTCB, uint32_t TickCount)
{
    XC_Tick_t      Tick;
    XC_ListNode_t* pIterator;
    XC_ListNode_t* pList;

    if(TickCount != 0U) {                  // 有延时或超时时间("TickCount"!=0)
        Tick = XC_Time_GetTick();          // 得到当前系统Tick
        TickCount += Tick;                 // 得到此任务下次唤醒的的时刻
        phTCB->TaskWakeupTick = TickCount; // 保存下次唤醒时刻
        /**
         *  判断任务下个唤醒时刻是否是在Tick溢出后:
         *      (TickCount < Tick) ? 溢出 : 非溢出
         *  由溢出和非溢出选择任务入溢出表或者时间表;
         *  - 本判定与"64 位精确比较(Tick + TickCount 是否 >= 2^32)"**完全等价**
         *    (无"半量程"限制),溢出表承接的正是"唤醒时刻落在下一个 Tick 回绕之后"的任务;
         *  - 溢出表在下一次 Tick 回绕时由时间调度**整表搬入时间表**(见 "XC_Sch_TimeSched")
         *    ⇒ **跨回绕的延时是支持的**;
         *  - 量程: 本函数的 "TickCount" 是**已换算好的 Tick 数**,可用上限即
         *    "XC_Tick_t" 的最大值(默认 uint32_t = 4294967295);
         *    "秒 → Tick"换算宏另有更小的秒数上限(见 "XC_TimeInternal.h" 的 "XC_Time_SecToTicks");
         */
        pList = (TickCount < Tick) ? (&phTCB->phXCOS->TimeOverflowList) : (&phTCB->phXCOS->TimeList); // 得到链表
        XC_Core_Lock(phTCB->phXCOS);                                                                  // 锁
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
        XC_Core_Lock(phTCB->phXCOS);      // 锁
        XC_Task_MoveToBlockedList(phTCB); // 任务移动到阻塞表(任务挂起或者死等,进入阻塞表;)
    }
    XC_Core_Unlock(phTCB->phXCOS); // 解锁
}

/**
 * @brief       [内部]等待通知处理
 * @param[in]   phTCB       任务句柄
 * @param[in]   TickCount   阻塞的超时时间(单位:Tick)
 * @details
 *  用于协程内部"等待通知"处理;
 *  被发送通知唤醒后待处理标志会被清除;
 *  **注意**:
 *  - 此函数配合"XC_Task_SendNotify"函数,需要一起协同;
 *  **锁契约**: 本函数开头 Lock,两条路径的解锁归属不同 ——
 *  - 命中待处理标志(提前通知,任务不阻塞): 本函数**显式** Unlock;
 *  - 走阻塞分支: **不解锁**,由 "XC_Task_HandleBlocking" 内部的 Unlock **隐式释放**
 *    (锁为单一标志位、不可重入 ⇒ 隐式配对);
 *  改动此处必须保证"**每条路径恰好解锁一次**",否则锁永久为 1(后果见 "XC_Core.h" 的契约说明)。
 */
void XC_Task_HandleWaitNotify(XC_TaskHandle_t phTCB, uint32_t TickCount)
{
    /**
     *  先锁
     *  在"XC_NOTIFY_WAIT"状态前在"通知唤醒"->不进入等待通知
     *  在"XC_NOTIFY_WAIT"状态后在"通知唤醒"->异步调度触发;
     */
    XC_Core_Lock(phTCB->phXCOS); // 锁
    /*语句[1]前中断,"异步调度"不会触发,只置"待处理标志"*/
    phTCB->NotifyState = XC_NOTIFY_WAIT; // [1]通知状态:等待通知
    /**
     *  语句[1]后中断,"异步调度"会触发;
     *  若是中断了,这里直接处理消费者,但是注意因为框架是协同调度,只要等待通知了,必定会触发调度;
     *  注意:这里处理了消费者,异步调度还是会被触发,但异步调度性能会有略微提升;
     *  这里不需要临时变量存放生产者,因为"待处理标志"就是"有/无"语义,醒了一次即全部消费;
     */
    if(phTCB->NotifyPending != 0U) {             // 出现通知
        phTCB->NotifyPending = 0U;               // 消费:清待处理标志(常量0写)
        phTCB->NotifyState   = XC_NOTIFY_WAKEUP; // 通知状态:通知唤醒
        XC_Core_Unlock(phTCB->phXCOS);           // 解锁
        phTCB->TaskState = XC_TASK_READY;        // 任务状态:就绪
    }
    else {
        /* 语句[1]后没有中断,这里直接处理阻塞:
         * - 先置 BLOCKED 再入表:若中断在 "XC_Task_HandleBlocking" 解锁后直接唤醒任务,
         *   其设置的 READY 会覆盖此处 BLOCKED,保证状态与链表一致;
         * - 锁设计:外层锁(本函数开头)须持续到入表完成,防止解锁窗口内中断直接唤醒任务
         *   导致"已唤醒却再次入表阻塞";"XC_Task_HandleBlocking"按"锁范围最小化"自持锁,
         *   其内部 Unlock 即隐式释放本外层锁(锁为单一标志位,不支持嵌套,此即隐式配对) */
        phTCB->TaskState = XC_TASK_BLOCKED;       // 任务状态:阻塞
        XC_Task_HandleBlocking(phTCB, TickCount); // 处理阻塞(内部锁保护入表,锁范围最小)
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
 * @retval      XC_FAIL :    注册失败(任务太多, 或该任务已注册)
 * @details
 *  **同一任务只能注册一次**: 重复注册返回 XC_FAIL (判据: "phXCOS" 非空; 移除后清空, 可再次注册)
 *  任务注册完成后会挂载到就绪表;
 */
XC_Return_t XC_Task_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    XC_DIAG_ASSERT(phXCOS != NULL);
    XC_DIAG_ASSERT(fTask != NULL);
    /** 重复注册检测: 已注册(phXCOS 非空)的任务再次注册会重复入表 => 破坏链表/任务数虚高 */
    if(phTCB->phXCOS != NULL) {
        return (XC_FAIL);
    }

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

/* 协程嵌套 API (仅 `XC_CFG_COR_NESTING != 0`; 关闭时本段不编译 ⇒ 使用即编译报错) */
#if (XC_CFG_COR_NESTING != 0)
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
 * @retval      XC_FAIL :    注册失败(任务太多, 或该任务已注册)—— 同 "XC_Task_Reg"
 * @details
 *  先写帧栈配置,再走"XC_Task_Reg"流程(等价于 Reg + SetCorStack);
 *  "pCorStack"/"CorDepthMax"必须同为有/同为无;
 */
XC_Return_t XC_Task_RegExt(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, XC_CorFn_t fTask, void* pParam, XC_CorFrame_t* pCorStack, uint8_t CorDepthMax)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    XC_DIAG_ASSERT(phXCOS != NULL);
    XC_DIAG_ASSERT(fTask != NULL);
    XC_DIAG_ASSERT((pCorStack == NULL) == (CorDepthMax == 0U));
    /* "帧栈与容量同为有/同为无"是**前置契约**: 违背属未定义行为(DEBUG 档由断言上报);
     * 按"单一强制点"规则, 此处不再重复 `return XC_FAIL`(边界规则见 Docs/架构概览/XCOS_V2.1.0_架构概览.md §11.6) */
    phTCB->pCorStack   = pCorStack;   // 帧栈
    phTCB->CorDepthMax = CorDepthMax; // 栈容量
    return (XC_Task_Reg(phXCOS, phTCB, fTask, pParam));
}

/**
 * @brief       [用户]设置协程帧栈(支持/撤销协程嵌套能力)
 * @param[in]   phTCB         协程任务控制块
 * @param[in]   pCorStack     协程帧栈(用户按需提供;撤销嵌套能力传NULL)
 * @param[in]   CorDepthMax   帧栈容量(最大嵌套层数;撤销传0)
 * @return      XC_Return_t
 * @retval      XC_OK :      设置成功
 * @retval      XC_FAIL :    (不返回: 形参违背前置契约属未定义行为, 见 @details)
 * @details
 *  写配置并置"CorDepth = 0U";传"(NULL, 0U)"撤销嵌套能力;
 */
XC_Return_t XC_Task_SetCorStack(XC_TaskHandle_t phTCB, XC_CorFrame_t* pCorStack, uint8_t CorDepthMax)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    XC_DIAG_ASSERT((pCorStack == NULL) == (CorDepthMax == 0U));
    /* 前置契约(违背=未定义行为; DEBUG 档由断言上报) —— 不再重复返回 XC_FAIL(单一强制点) */
    phTCB->pCorStack   = pCorStack;   // 帧栈
    phTCB->CorDepthMax = CorDepthMax; // 栈容量
    phTCB->CorDepth    = 0U;          // 清当前深度
    return (XC_OK);
}
#endif

/**
 * @brief       [用户]任务移除
 * @param[in]   phTCB 协程控制块
 * @details
 *  会从链表中删除,并清空TCB数据;
 *  不可移除自身,移除自身使用"XC_Cor_Remove()"函数;
 */
void XC_Task_Remove(XC_TaskHandle_t phTCB)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    if(phTCB->phXCOS != NULL) {
        XC_Task_HandleRemove(phTCB);
    }
}

/************************************************ 我是分割线 ************************************************/
/**
 * 任务分步创建相关的用户函数
 *  "XC_Task_SetEntry"和"XC_Task_Add"函数需要配合使用;
 *  在"XC_Task_SetEntry"设置完成后在"XC_Task_Add"添加任务;
 *  注意:已注册(phXCOS 非空)的任务重复添加会返回 XC_FAIL;
 */

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
void XC_Task_SetEntry(XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    XC_DIAG_ASSERT(fTask != NULL);
    phTCB->TaskState = XC_TASK_VOID; // 任务状态清除
#if (XC_CFG_COR_NESTING != 0)
    /**
     *  嵌套中换入口: 必须先"清栈 + 重置断点", 否则栈底帧 pCorStack[0].pfn(旧入口)
     *  会在 Reset/Remove/嵌套越界保护 时被用来恢复 fTask => 新入口静默失效/跳回旧函数;
     *  语义: **嵌套中换入口 = 丢弃当前嵌套层, 按新入口从头运行**;
     */
    phTCB->CorDepth = 0U;   // 丢弃当前嵌套帧(栈底帧不变量: [0].pfn == 最外层入口)
#endif
    COR_Init(phTCB->BP);    // 重置断点(避免脏断点跳入新函数的非法位置)
    phTCB->fTask  = fTask;  // 更新任务入口
    phTCB->pParam = pParam; // 传递给任务的参数
}

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
 *  添加后挂载到就绪表;
 */
XC_Return_t XC_Task_Add(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    XC_DIAG_ASSERT(phXCOS != NULL);
    XC_DIAG_ASSERT(phTCB->fTask != NULL);
    /** 重复添加检测: 已注册(phXCOS 非空)的任务再次入表会破坏链表/任务数虚高 */
    if(phTCB->phXCOS != NULL) {
        return (XC_FAIL);
    }

    if(phXCOS->TaskNum >= XC_CFG_MAX_TASKS) {
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
 *  **不可复位自身(复位自身使用"XC_Cor_Reset")**
 *  会清除所有状态(包含挂起),任务复位;
 *  任务将重置到就绪表,然后从头运行;
 *  任务TCB中除了"phXCOS","fTask","Param"其他全部重置;
 */
void XC_Task_Reset(XC_TaskHandle_t phTCB)
{
    XC_DIAG_ASSERT(phTCB != NULL);
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
 *  需要在任务中立刻挂起,可以在协程块中调用"XC_Cor_Suspend";
 *  挂起可以覆盖任务的所有阻塞状态,优先级最高;
 *  挂起后只能被挂起恢复唤醒;
 */
XC_Return_t XC_Task_Suspend(XC_TaskHandle_t phTCB)
{
    XC_DIAG_ASSERT(phTCB != NULL);
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
 *  任务唤醒后原先的阻塞将失效,任务状态为"XC_TASK_READY";
 */
XC_Return_t XC_Task_Resume(XC_TaskHandle_t phTCB)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    // 任务没有被挂起
    if(phTCB->TaskState != (uint8_t)XC_TASK_SUSPEND) {
        return (XC_FAIL); // 没有被挂起
    }

    XC_Core_Lock(phTCB->phXCOS);        // 锁
    XC_Task_MoveToReadyListTail(phTCB); // 将任务移动到就绪表尾(统一排队尾; 恢复不插队)
    /* 挂起期间已登记的通知(待处理标志)在恢复时交付: 消费标志并置"通知唤醒",
     * 否则恢复后 XC_Cor_IsNotifyTimeout() 会因 NotifyState 仍为 WAIT 而误报超时 */
    if((phTCB->NotifyState == (uint8_t)XC_NOTIFY_WAIT) && (phTCB->NotifyPending != 0U)) {
        phTCB->NotifyPending = 0U;                        // 消费:清待处理标志(常量0写)
        phTCB->NotifyState   = (uint8_t)XC_NOTIFY_WAKEUP; // 交付:通知唤醒(等待侧视为"已通知")
    }
    XC_Core_Unlock(phTCB->phXCOS);    // 解锁
    phTCB->TaskState = XC_TASK_READY; // 任务状态:就绪
    return (XC_OK);
}

/************************************************ 我是分割线 ************************************************/
/**
 *  任务通知的用户函数
 * ---
 *  因为框架不做开关中断,也不做原子操作(编译器特性等),
 *  所以目前任务同步只支持"通知",且只支持"SPSC"(单生产者单消费者,一个任务支持一个SCSP)模型;
 *  ---
 *  并发模型分层(2026-09-14 通知"待处理标志"改造后):
 *  - 任务级通知(TCB.NotifyPending): **SPSC** —— 单生产者(某一个中断或某一个任务) + 单消费者(目标任务自身);
 *  - 框架级事件门控(XCOS.EventPending): **MPSC** —— **所有中断都是生产者**,主循环是唯一消费者;
 *    因生产侧只写常量 1(不做"读-改-写"),MPSC 下任一交错结果都是 1,不会"丢更新";
 *    (改造前的 EventProduced++ 是"读-改-写",在多个中断并发/嵌套时属于丢更新竞争,
 *     仅因门控只要求"非零"才未暴露);
 *  - 框架级 Lock 与四张链表: 由 Lock **串行化**访问(非无锁); 中断仅在"读到 Lock==0"时直接移表,
 *    否则(含抢占嵌套的中断)只登记标志,由主循环落地;
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
 *  被发送通知唤醒后待处理标志会被清除;
 *  **注意**:
 *  - 一个任务对应一个通知处理;
 *  - 每个任务支持单生产者单消费者模型(SPSC)
 *  - 此函数为生产者,必须为单生产者;
 *      也就是说此函数运行时不能再被中断或者任务再次调用(每个任务都可以有一个,满足SPSC);
 *  - 若是有可能出现多次调用(多生产者),需要用户加锁保护;
 *  - 此函数不包含自锁,因为是通用C语言编写,不增加编译器和平台指令,也不关中断,
 *      这表示锁将不是原子操作,在特定的时刻必定出错,所以这里不在增加锁;
 *  - 此函数配合"XC_Task_HandleWaitNotify"函数,需要一起协同;
 */
XC_Return_t XC_Task_SendNotify(XC_TaskHandle_t phTCB, void* pNotifyData)
{
    XC_DIAG_ASSERT(phTCB != NULL);
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

    phTCB->pNotifyData   = pNotifyData; // 通知数据(先写数据)
    phTCB->NotifyPending = 1U;          // 通知-待处理标志(再置标志: 常量1写,无条件/幂等/无读改写)

    if(phTCB->NotifyState != (uint8_t)XC_NOTIFY_WAIT) {
        /** 不是等待通知状态:只做记录;等待侧进入等待时会先置 WAIT 再查标志,不会漏 */
        return (XC_CONTINUE);
    }

    /** 必须是等待通知唤醒 */
    if(XC_Core_GetLockState(phTCB->phXCOS)) {
        /** 调度运行中-异步,只会在中断中出现:仅登记事件,由主循环落地(不移表) */
        phTCB->phXCOS->EventPending = 1U; // 异步调度触发(常量1写)
        return (XC_CONTINUE);
    }

    /** 调度没有运行 */
    XC_Core_Lock(phTCB->phXCOS);           // 锁
    XC_Task_MoveToReadyListTail(phTCB);    // 移动到就绪表尾(唤醒不插队, 与"提前通知"一致)
    XC_Core_Unlock(phTCB->phXCOS);         // 解锁
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
