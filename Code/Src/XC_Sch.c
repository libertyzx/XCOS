/**
 * @file        XC_Sch.c
 * @brief       调度器实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.1
 * @date        2026/08/14
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
#include "Internal/XC_List.h"
#include "Internal/XC_TaskInternal.h"
#include "XC_Diag.h" //诊断能力(断言/运行统计埋点)
#include "XC_Time.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 私有宏 */

/**
 * @brief   [私有]摘就绪表首节点(内联宏, 等价 XC_List_Remove 内联展开)
 * @param[in]   phTCB   就绪表首任务 TCB(由调用处先取)
 * @details
 *  摘除首节点并自环化(游离标记); 4 步链表操作, 无函数调用;
 *  调用处先取表首任务: phTCB = XC_LIST_TO_TCB(XC_List_GetListStartNode(&phXCOS->ReadyList));
 */
#define XC_SCH_TAKE_FIRST(phTCB)                                  \
    do {                                                          \
        (phTCB)->ListNode.pNext->pPrev = (phTCB)->ListNode.pPrev; \
        (phTCB)->ListNode.pPrev->pNext = (phTCB)->ListNode.pNext; \
        (phTCB)->ListNode.pNext        = &(phTCB)->ListNode;      \
        (phTCB)->ListNode.pPrev        = &(phTCB)->ListNode;      \
    } while(0)

/**
 * @brief   [私有]任务插回就绪表尾(内联宏, 4 步链表操作)
 * @param[in]   phReady   就绪表根(&phXCOS->ReadyList)
 * @param[in]   phTCB     要插回的任务(必游离/自环)
 * @details
 *  利用不变量: 任务必游离(免摘除) + 表尾 pNext 恒为 Root(免读);
 *  就绪表空时原表尾==Root, 结果单节点表, 统一成立。
 */
#define XC_SCH_PUT_TAIL(phReady, phTCB)               \
    do {                                              \
        (phTCB)->ListNode.pNext = (phReady);          \
        (phTCB)->ListNode.pPrev = (phReady)->pPrev;   \
        (phReady)->pPrev->pNext = &(phTCB)->ListNode; \
        (phReady)->pPrev        = &(phTCB)->ListNode; \
    } while(0)

/**
 * @brief   [私有]获取下个任务唤醒的时间
 * @return  XC_Tick_t    下个唤醒的Tick值;
 * @details 下个任务唤醒的时间为:时间表首节点唤醒时间;
 */
#define XC_Sch_GetNextTaskWakeupTick() ((XC_LIST_TO_TCB(XC_List_GetListStartNode(&phXCOS->TimeList)))->TaskWakeupTick)

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
static void XC_Sch_TimeSched(XC_OSHandle_t phXCOS)
{
    XC_ListNode_t*  pIterator;
    XC_TaskHandle_t phTCB;
    XC_Tick_t       Tick;
    XC_ListNode_t*  pTimeList;

    pTimeList = &phXCOS->TimeList;
    Tick      = XC_Time_GetTick(); // 得到当前系统Tick
    XC_Core_Lock(phXCOS);          // 锁
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
        if(XC_List_ListValid(pTimeList)) { // 时间表有节点
            // 所有节点移动到就绪表(尾部);
            pIterator = XC_List_GetListStartNode(pTimeList); // 得到时间链表初始节点
            do {
                XC_LIST_TO_TCB(pIterator)->TaskState = XC_TASK_READY;    // 任务状态:就绪
                pIterator                            = pIterator->pNext; // 指向下个节点
            } while(!XC_List_ReachEndNode(pTimeList, pIterator)); // 到达结尾则结束
            // 时间表所有节点追加到就绪表尾(入表统一排队尾; 锚点 = 旧就绪表尾节点)
            XC_List_MoveListToNodeAfter(phXCOS->ReadyList.pPrev, pTimeList);
        }
        // 时间溢出表处理(溢出表全部节点移动时间表)
        if(XC_List_ListValid(&phXCOS->TimeOverflowList)) {                     // 时间溢出表有节点
            XC_List_MoveListToNodeAfter(pTimeList, &phXCOS->TimeOverflowList); // 时间溢出表所有节点移动到时间表
        }
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
    if((XC_List_ListValid(pTimeList)) && (Tick >= XC_Sch_GetNextTaskWakeupTick())) { // 时间表有效 && 任务唤醒时间已经到达
        /**轮询链表,将时间到达的节点都移动到就绪表 */
        pIterator = XC_List_GetListStartNode(pTimeList);           // 得到时间链表初始节点
        while(Tick >= XC_LIST_TO_TCB(pIterator)->TaskWakeupTick) { // 唤醒时间到达
            phTCB     = XC_LIST_TO_TCB(pIterator);                 // 得到当前任务的TCB
            pIterator = pIterator->pNext;                          // 指向下个任务(必须在操作链表前)
            XC_Task_MoveToReadyListTail(phTCB);                    // 唤醒任务,移到就绪表尾(统一排队尾)
            phTCB->TaskState = XC_TASK_READY;                      // 任务状态:就绪
            // 轮询结束判断
            if(XC_List_ReachEndNode(pTimeList, pIterator)) { // 到达结尾
                break;                                       // 结束轮询
            }
        }
    }
    /**
     *  **每轮都更新**"上次观察到的 Tick" —— 它是回绕判定(`PrevTick > Tick`)的唯一依据;
     *  若只在"检测到回绕"或"有时间表到点"时才更新, 会在极端场景下**长期不更新**:
     *  启动后从未有定时到点(例如纯事件驱动的应用), 期间却有一笔跨回绕的延时进了溢出表 ⇒
     *  回绕真正发生时 `PrevTick` 还是初值(0) ⇒ 判定不成立 ⇒ **溢出表整表搬移被跳过**(该任务永不唤醒,
     *  最长要再等一个完整回绕周期, 若期间仍无到点则永不);
     *  每轮更新"观察值"不改变判定语义(`PrevTick > Tick` ⇔ 相邻两次观察之间发生过回绕),
     *  只会让判定更及时;
     */
    phXCOS->PrevTick = Tick; // 更新保存Tick
    XC_Core_Unlock(phXCOS);  // 解锁
}

/**
 * @brief       [私有]事件调度
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数用于:
 *      - 任务唤醒"XC_Task_SendNotify"函数触发;
 *  注意:
 *      - 在调用此函数前必须先判断"phXCOS->EventPending != 0U"(主循环门控:先清标志后处理);
 *      - 此函数的实现主要是循环搜索链表(时间表,溢出表,阻塞表),判断是否有需要处理的任务:
 */
static void XC_Sch_EventSched(XC_OSHandle_t phXCOS)
{

    /**
     * [私有]通知唤醒遍历处理宏
     * @param[in]   pCList  需要遍历查找"等待通知"任务的链表指针
     * @details
     *  刻意使用宏(而非静态函数)的实现说明:
     *  - 本段代码位于事件调度的热路径上(每次事件触发会对 3 个链表各调用一次),
     *    宏内联展开可避免函数调用/入栈/返回等开销,同时迭代器与指针缓存在展开点直接生成,不额外占用栈帧;
     *  - 宏体只使用传入参数和全局宏(链表/任务操作宏),不隐式引用外部局部变量;
     *  - 宏在函数内定义(局部宏),作用域仅限"XC_Sch_EventSched",不污染全局命名空间;
     *  - 宏体执行期间依赖外层已持有的调度锁("XC_Core_Lock"),保证链表遍历与节点移动的原子性;
     *  注意:
     *  - 被唤醒的节点会移动(移除)到就绪表,所以必须先保存/推进迭代器再操作节点,
     *    否则链表指针将失效(见宏体内操作顺序);
     *  - **挂起优先级(最高)**: 唤醒条件**已排除** `TaskState==SUSPEND`
     *    ⇒ 挂起任务不会被事件调度唤醒, 只能由 `XC_Task_Resume` 恢复(挂起期间到达的通知不登记);
     *  - **入表位置**: 唤醒后**追加到就绪表尾**
     *    ⇒ 与"提前通知 / 普通让出"一致, 唤醒不做优先级补偿, 保持严格轮转(回归 V2.0 语义);
     */
#define WAKEUP_NOTIFY(pCList)                                                                                                        \
    {                                                                                                                                \
        XC_ListNode_t* pList     = pCList;                          /*缓存链表*/                                                     \
        XC_ListNode_t* pIterator = XC_List_GetListStartNode(pList); /*得到链表初始节点*/                                             \
        if(!XC_List_ReachEndNode(pList, pIterator)) {               /*判断是否有节点*/                                               \
            do {                                                                                                                     \
                if((XC_LIST_TO_TCB(pIterator)->NotifyState == (uint8_t)XC_NOTIFY_WAIT) && /*是等待唤醒*/                             \
                   (XC_LIST_TO_TCB(pIterator)->NotifyPending != 0U) &&                    /*有待处理通知*/                           \
                   (XC_LIST_TO_TCB(pIterator)->TaskState != (uint8_t)XC_TASK_SUSPEND)) {  /*挂起优先级最高:不唤醒挂起任务*/          \
                    /*是等待唤醒 && 有待处理通知*/                                                                                   \
                    XC_LIST_TO_TCB(pIterator)->NotifyPending = 0U;                        /*消费:清待处理标志(常量0写)*/             \
                    XC_TaskHandle_t phTCB                    = XC_LIST_TO_TCB(pIterator); /*得到当前任务TCB*/                        \
                    pIterator                                = pIterator->pNext;          /*指向下个节点(必须在操作前指向下个节点)*/ \
                    /** 通知唤醒 */                                                                                                  \
                    XC_Task_MoveToReadyListTail(phTCB);    /*任务移动到就绪表尾(唤醒不插队)*/                                        \
                    phTCB->NotifyState = XC_NOTIFY_WAKEUP; /*通知状态:通知唤醒*/                                                     \
                    phTCB->TaskState   = XC_TASK_READY;    /*任务状态:就绪*/                                                         \
                }                                                                                                                    \
                else {                                                                                                               \
                    pIterator = pIterator->pNext; /*指向下个节点*/                                                                   \
                }                                                                                                                    \
            } while(!XC_List_ReachEndNode(pList, pIterator));                                                                        \
        }                                                                                                                            \
    }

    XC_Core_Lock(phXCOS);                     // 锁
    WAKEUP_NOTIFY(&phXCOS->TimeList);         // "TimeList"延时/超时/等待的链表
    WAKEUP_NOTIFY(&phXCOS->TimeOverflowList); // "TimeOverflowList"时间溢出的链表
    WAKEUP_NOTIFY(&phXCOS->BlockedList);      // "BlockedList"阻塞链表
    XC_Core_Unlock(phXCOS);                   // 解锁
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
 *  本函数会把 `Lock` 置 1(锁定) —— 目的是让**初始化窗口**内的中断"直接路径"只登记、不改表
 *  (此刻四张表刚建好, 还没有任务);
 *  **注意**: 随后的注册会**解锁**(`XC_Task_Reg`/`XC_Task_Add` 内部按"锁范围最小化"成对 Lock/Unlock)
 *  ⇒ 这里的锁定**不代表**"到 `XC_Sch_Start` 之前一直持锁";
 *  之所以仍然安全: 启动前没有任务在运行 ⇒ `NotifyState` 恒为 `WAKEUP`(`XC_Task_BasicInit` 置),
 *  中断侧 `XC_Task_SendNotify` 只登记 `NotifyPending`(**不移表**) ⇒ 与主循环之间没有并发表操作;
 */
void XC_Sch_Init(XC_OSHandle_t phXCOS)
{
    XC_DIAG_ASSERT(phXCOS != NULL);
    /** 初始第一步上锁: 初始化窗口内中断"直接路径"只登记、不改表(注册阶段会成对解锁) */
    phXCOS->Lock = 1U; // 锁(1锁,0解锁),用于中断处理

    phXCOS->PrevTick     = 0U;   // 保存上个Tick值
    phXCOS->TaskNum      = 0U;   // 任务数量
    phXCOS->EventPending = 0U;   // 异步调度事件-待处理标志(通知/挂起/恢复的异步触发)
    phXCOS->fIdle        = NULL; // 框架空闲处理

    XC_List_Init(&phXCOS->ReadyList);
    XC_List_Init(&phXCOS->TimeList);
    XC_List_Init(&phXCOS->TimeOverflowList);
    XC_List_Init(&phXCOS->BlockedList);
}

/**
 * @brief       [用户]调度器启动
 * @param[in]   phXCOS  框架句柄
 * @details
 *  此函数无返回,阻塞运行;
 *  注意:阻塞下要是有看门狗,记得在任务中喂狗;
 */
void XC_Sch_Start(XC_OSHandle_t phXCOS)
{
    XC_DIAG_ASSERT(phXCOS != NULL);
    XC_TaskHandle_t phTCB;
    XC_Tick_t       Tick;
#if (XC_CFG_TASK_STATS != 0)
    XC_Tick_t RunStart; // [统计] 本调度槽起始 Tick(仅统计档存在; 局部变量不占任务/框架 RAM)
#endif

    for(;;) {
        /* 取任务: ListValid 锁外判断(中断仅通知类, 只加不减 -> 判断非空后不会变空, 不会取到根节点) */
        if(XC_List_ListValid(&phXCOS->ReadyList)) {                               // 就绪表有节点,处理
            XC_Core_Lock(phXCOS);                                                 // 锁: 保护取任务链表操作
            phTCB = XC_LIST_TO_TCB(XC_List_GetListStartNode(&phXCOS->ReadyList)); // 表首节点 = Root->pNext
            XC_SCH_TAKE_FIRST(phTCB);                                             // ★ 摘除: 节点自环 => 游离标记
            phTCB->TaskState = XC_TASK_RUN;                                       // 任务状态:运行
            XC_Core_Unlock(phXCOS);                                               // 解锁: 运行阶段中断可正常直接操作

            /* 运行(锁外) */
#if (XC_CFG_TASK_STATS != 0)
            /* [统计] 统计起点: 从"摘出就绪表"到"让出"为一次调度槽(含下面同轮切父的父层) */
            RunStart = XC_Diag_RunStatsBegin();
#endif
            phTCB->fTask(phTCB); // 恒调当前层(顶层或子层)
                                 /* 同轮切父:当前层完成(走到 Leave 置 DONE)且栈非空时,弹帧并立即运行父层
                                  * (仅在 XC_CFG_COR_NESTING=1 时存在; 关闭后 TCB 无 CorState/CorDepth 字段) */
#if (XC_CFG_COR_NESTING != 0)
            while((phTCB->CorState == XC_COR_DONE) && (phTCB->CorDepth > 0U)) {
                XC_Task_PopFrame(phTCB); // 出栈:恢复父层入口+返回点
                phTCB->fTask(phTCB);     // 同轮切父:运行父层
            }
#endif
#if (XC_CFG_TASK_STATS != 0)
            /* [统计] 统计终点(归位之前): 累计运行次数与最长耗时; 任务等待期间不计时;
             * 本槽内"移除自身"的任务(TaskState==VOID, 统计已在 HandleRemove 清零)不再累计,
             * 否则会把"移除时清零"覆盖成 RunCnt=1 / MaxRunTick=本槽耗时 */
            if(phTCB->TaskState != (uint8_t)XC_TASK_VOID) {
                XC_Diag_RunStatsEnd(phTCB, RunStart);
            }
#endif
            /* 归位: 仅"游离"(自环)任务插回就绪表尾
             * 自环 = 调度器摘出后没被任何 API 移表(Yield/Leave/直接return); 非自环 = 已入别的表或已移除, 不干预;
             * 插回时同步置 READY(不覆盖运行中被中断挂起任务的 SUSPEND, 否则出现"在阻塞表却报 READY"的僵尸状态)
             * ⚠️ 依赖: **任务被移除时其节点被置 NULL**(见 "XC_Task_HandleRemove") ⇒ 已移除任务不会被误插回;
             *          改这里或改那里时必须成对检查 */
            if(phTCB->ListNode.pNext == &phTCB->ListNode) {     // 锁外快判(可过期): 决定"是否进锁"
                XC_Core_Lock(phXCOS);                           // 锁
                if(phTCB->ListNode.pNext == &phTCB->ListNode) { // 锁内复判(生效): 防中断在两次判定之间搬表
                    XC_SCH_PUT_TAIL(&phXCOS->ReadyList, phTCB); // 插就绪表尾(轮转)
                    phTCB->TaskState = XC_TASK_READY;           // 状态一致性: 锁内同步(与链表操作原子)
                }
                XC_Core_Unlock(phXCOS);
            }
        }
        /* 就绪表空: 直接跳过, 进入时间/事件/空闲处理 */

        XC_Sch_TimeSched(phXCOS); // 时间调度处理

        // 任务异步调度处理(先清标志再处理: 处理期间中断再置位 => 下轮再处理, 不丢事件)
        if(phXCOS->EventPending != 0U) {
            phXCOS->EventPending = 0U;
            XC_Sch_EventSched(phXCOS);
        }

        // 判断就绪表是否有任务(空闲处理); 锁外快判依据"中断侧只加不减"⇒ 判空后不会变空(锁内会再判一次)
        if((XC_List_ListValid(&phXCOS->ReadyList) == 0) && (phXCOS->fIdle != NULL)) {
            /** 就绪表没有节点 && 有空闲处理函数*/
            /**
             *  计算"建议休眠上限"(距最近唤醒的 Tick 数):
             *  - 时间表1,溢出表0: 取时间表首节点唤醒时刻(最近唤醒的那个任务);
             *  - 时间表1,溢出表1: 取时间表首节点唤醒时刻(溢出表任务在**下一个 Tick 回绕之后**, 必然更晚);
             *  - 时间表0,溢出表1: 取溢出表首节点唤醒时间;
             *  - 时间表0,溢出表0: 给 ~0U(无待唤醒任务, 按最大时间休眠);
             *  - 已过点 / 窗口内 Tick 回绕 / 就绪表已有任务 ⇒ 给 0(不要休眠);
             *  - "当前 Tick"与表内取值在**同一个锁域内读**(一次一致快照): 锁外读 Tick 会被中断推进 ⇒ 提示值偏大;
             */
            XC_Core_Lock(phXCOS);         // 锁: "当前 Tick 快照"、"就绪表是否空"、"下个唤醒时刻"读成一次原子观察
            Tick = XC_Time_GetTick();     // 得到当前Tick(锁内取快照: 锁外取会被中断推进 ⇒ 提示值偏大)
            if(phXCOS->PrevTick > Tick) { // 窗口内 Tick 回绕(时间调度待处理)
                Tick = 0U;
            }
            else if(XC_List_ListValid(&phXCOS->ReadyList)) { // 已有任务就绪
                Tick = 0U;
            }
            else if(XC_List_ListValid(&phXCOS->TimeList)) {          // 时间表有节点
                XC_Tick_t Next = XC_Sch_GetNextTaskWakeupTick();     // 首节点 = 最近唤醒时刻
                Tick           = (Next > Tick) ? (Next - Tick) : 0U; // 已过点 ⇒ 0(直接相减会回绕成大数)
            }
            else if(XC_List_ListValid(&phXCOS->TimeOverflowList)) {                                                // 溢出表有节点
                Tick = XC_LIST_TO_TCB(XC_List_GetListStartNode(&phXCOS->TimeOverflowList))->TaskWakeupTick - Tick; // 模块差(未回绕) = 距回绕 + 唤醒时刻
            }
            else {          // 时间表,溢出表都没有节点;
                Tick = ~0U; // 按最大时间休眠
            }
            XC_Core_Unlock(phXCOS);
            phXCOS->fIdle(phXCOS, Tick); // 框架空闲处理(锁外: 回调可能休眠/阻塞)
        }
    }
}

/**
 * @brief       [用户]获取任务数
 * @param[in]   phXCOS  框架句柄
 * @return      uint8_t 返回任务数量
 * @details     直接获取框架句柄中的任务数字段;
 */
uint8_t XC_Sch_GetTaskNum(XC_OSHandle_t phXCOS)
{
    XC_DIAG_ASSERT(phXCOS != NULL);
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
 * @note
 *  回调的第二个参数"IdleTick"是"**建议休眠上限**"(距下一个任务唤醒的 Tick 数);
 *  取值语义见 "Internal/XC_TypeInternal.h" 里 "fIdle" 字段的说明;
 */
void XC_Sch_SetIdleCallback(XC_OSHandle_t phXCOS, void (*fIdle)(XC_OSHandle_t, XC_Tick_t))
{
    XC_DIAG_ASSERT(phXCOS != NULL);
    phXCOS->fIdle = fIdle;
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
