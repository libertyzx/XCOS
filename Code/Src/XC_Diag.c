/**
 * @file        XC_Diag.c
 * @brief       诊断实现: 运行期自检 / 运行统计(错误上报与断言无实现: 前者在 `XC_Err.h`(用户提供钩子), 后者是宏)
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.1
 * @date        2026/09/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  本文件集中"诊断类"的**实现**(声明见 `XC_Diag.h`);
 *  埋点(调用点)仍在内核里: `XC_Task.c`(帧栈越界上报 / 统计清零)、`XC_Sch.c`(调度槽计时两端);
 *  ---
 *  编译裁剪:
 *  - 各子功能各自用开关包住 ⇒ 全部关闭时本文件为**空编译单元**(0 代码/0 RAM);
 *  - 发布工程也可**不添加本文件**(物理裁剪); 此时要求所有诊断开关为 0 —— 否则链接报未定义引用;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 头文件
#include "XC_Diag.h"
#include "Internal/XC_Core.h"
#include "Internal/XC_List.h"
#include "XC_Time.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 2) 运行期不变量自检(开关: XC_CFG_DEBUG_CHECK) */

#if (XC_CFG_DEBUG_CHECK != 0)

/**
 * @brief       [私有]节点是否在指定表中(带步数上限的遍历)
 * @param[in]   pRoot   表根节点
 * @param[in]   pNode   待查节点
 * @return      0: 不在表中; 1: 在表中
 * @details     仅用于调试自检; 步数上限 "XC_CFG_MAX_TASKS" 防止表成环时死循环;
 */
static uint8_t XC_Diag_ChkNodeInList(XC_ListNode_t* pRoot, XC_ListNode_t* pNode)
{
    XC_ListNode_t* pIter = pRoot->pNext;
    uint16_t       n     = 0U;

    while((pIter != pRoot) && (n <= (uint16_t)XC_CFG_MAX_TASKS)) {
        if(pIter == pNode) {
            return (1U);
        }
        pIter = pIter->pNext;
        n++;
    }
    return (0U);
}

/**
 * @brief       [私有]单表遍历 + 逐节点不变量校验
 * @param[in]   phXCOS          框架句柄
 * @param[in]   pRoot           表根节点
 * @param[in]   fBlockedFamily  0: 就绪表(状态须为 READY); 1: 阻塞族(状态须为 BLOCKED/SUSPEND)
 * @param[out]  pNodeCnt        本表节点数(累加到调用者的计数上)
 * @return      XC_ChkResult_t  首个失败项, 或 "XC_CHK_OK"
 * @details
 *  校验: 链表双向指针完好 + 任务归属 + 状态↔表匹配 + 协程嵌套自洽 + 步数上限(防环);
 *  详见 `XC_Diag.h` 中 "XC_Diag_CheckInvariants" 的检查项说明;
 */
static XC_ChkResult_t XC_Diag_ChkOneList(XC_OSHandle_t  phXCOS,
                                         XC_ListNode_t* pRoot,
                                         uint8_t        fBlockedFamily,
                                         uint16_t*      pNodeCnt)
{
    XC_ListNode_t*  pIter = pRoot->pNext;
    XC_TaskHandle_t phTCB;
    uint16_t        n = 0U;

    while(pIter != pRoot) {
        /** 1) 链表结构: 前后指针必须互指 */
        if((pIter->pNext->pPrev != pIter) || (pIter->pPrev->pNext != pIter)) {
            return (XC_CHK_LINK_BROKEN);
        }
        phTCB = XC_LIST_TO_TCB(pIter);
        /** 2) 归属: 表内任务必须属于本次检查的框架实例 */
        if(phTCB->phXCOS != phXCOS) {
            return (XC_CHK_HANDLE);
        }
        /** 3) 状态↔表: 就绪表=READY; 阻塞族=BLOCKED/SUSPEND, 且 SUSPEND 只允许出现在阻塞表 */
        if(fBlockedFamily == 0U) {
            if(phTCB->TaskState != (uint8_t)XC_TASK_READY) {
                return (XC_CHK_STATE_TABLE);
            }
        }
        else {
            if((phTCB->TaskState != (uint8_t)XC_TASK_BLOCKED) && (phTCB->TaskState != (uint8_t)XC_TASK_SUSPEND)) {
                return (XC_CHK_STATE_TABLE);
            }
            if((phTCB->TaskState == (uint8_t)XC_TASK_SUSPEND) && (pRoot != &phXCOS->BlockedList)) {
                return (XC_CHK_STATE_TABLE);
            }
        }
        /** 4) 协程嵌套不变量: 帧栈与容量必须同为"有"/同为"无"; 深度不得越界(仅 `XC_CFG_COR_NESTING=1`) */
#if (XC_CFG_COR_NESTING != 0)
        if((phTCB->pCorStack == NULL) != (phTCB->CorDepthMax == 0U)) {
            return (XC_CHK_NESTING);
        }
        if((phTCB->CorDepth > phTCB->CorDepthMax) ||
           ((phTCB->CorDepth > 0U) && (phTCB->pCorStack == NULL))) {
            return (XC_CHK_NESTING);
        }
#endif
        pIter = pIter->pNext;
        n++;
        /** 5) 步数上限: 超过最大任务数 => 表成环(结构损坏) */
        if(n > (uint16_t)XC_CFG_MAX_TASKS) {
            return (XC_CHK_LINK_BROKEN);
        }
    }
    *pNodeCnt = (uint16_t)(*pNodeCnt + n);
    return (XC_CHK_OK);
}

/**
 * @brief       [私有]同一任务不得同时挂在两张表
 * @param[in]   phXCOS  框架句柄
 * @return      XC_ChkResult_t
 * @details     四表两两比对(调试期允许 O(n^2)); 不分配临时数组 => RAM 0;
 */
static XC_ChkResult_t XC_Diag_ChkDupNode(XC_OSHandle_t phXCOS)
{
    XC_ListNode_t* pRoots[4];
    uint8_t        i;

    pRoots[0] = &phXCOS->ReadyList;
    pRoots[1] = &phXCOS->TimeList;
    pRoots[2] = &phXCOS->TimeOverflowList;
    pRoots[3] = &phXCOS->BlockedList;

    for(i = 0U; i < 4U; i++) {
        XC_ListNode_t* pIter = pRoots[i]->pNext;
        uint16_t       n     = 0U;

        while((pIter != pRoots[i]) && (n <= (uint16_t)XC_CFG_MAX_TASKS)) {
            uint8_t j;
            for(j = 0U; j < 4U; j++) {
                if((j != i) && (XC_Diag_ChkNodeInList(pRoots[j], pIter) != 0U)) {
                    return (XC_CHK_NODE_IN_TWO_LIST);
                }
            }
            pIter = pIter->pNext;
            n++;
        }
    }
    return (XC_CHK_OK);
}

/**
 * @brief       [调试]运行期不变量自检
 * @param[in]   phXCOS  框架句柄
 * @return      XC_ChkResult_t  首个失败项; "XC_CHK_OK" 表示全部一致
 * @details
 *  检查项/用法/注意事项见 `XC_Diag.h` 的声明注释 与 `Docs/XCOS.md`「运行期自检」;
 *  实现约束:
 *  - 全程持锁(防中断 "SendNotify" 直接路径在遍历途中改表) => **不可在已持锁上下文中调用**;
 *  - 不使用静态/堆内存(仅少量栈: 4 个表根指针 `pRoots[4]`); 所有遍历都有步数上限(防环);
 *  - 只读检查: 不修改任何任务状态/表结构, 也不改变内核运行行为;
 */
XC_ChkResult_t XC_Diag_CheckInvariants(XC_OSHandle_t phXCOS)
{
    /* 句柄为空是**本函数承诺要报告的结果**(XC_CHK_HANDLE) ⇒ 用返回值表达, 不再叠加断言(单一强制点) */
    XC_ChkResult_t rc      = XC_CHK_OK;
    uint16_t       NodeCnt = 0U;

    if(phXCOS == NULL) {
        return (XC_CHK_HANDLE);
    }
    XC_Core_Lock(phXCOS); // 锁: 自检期间与中断"直接路径"串行化

    rc = XC_Diag_ChkOneList(phXCOS, &phXCOS->ReadyList, 0U, &NodeCnt);
    if(rc == XC_CHK_OK) {
        rc = XC_Diag_ChkOneList(phXCOS, &phXCOS->TimeList, 1U, &NodeCnt);
    }
    if(rc == XC_CHK_OK) {
        rc = XC_Diag_ChkOneList(phXCOS, &phXCOS->TimeOverflowList, 1U, &NodeCnt);
    }
    if(rc == XC_CHK_OK) {
        rc = XC_Diag_ChkOneList(phXCOS, &phXCOS->BlockedList, 1U, &NodeCnt);
    }
    if(rc == XC_CHK_OK) {
        rc = XC_Diag_ChkDupNode(phXCOS);
    }
    if(rc == XC_CHK_OK) {
        /**
         *  "TaskNum" 与四表节点总数一致 —— 但**运行中的任务不在任何表内**
         *  (调度器取首后 `XC_SCH_TAKE_FIRST` 把节点自环游离, 运行结束才归位)
         *  ⇒ 在任务上下文里调用本函数时, "节点总数恰好少 1"是**正常现象**;
         *  故只容忍这一种差值(其余差值 / 超出上限一律报 XC_CHK_TASKNUM);
         *  注: 任务上下文下"TaskNum 虚高 1"(真实的计数漂移)仍会被抓到(差值会变成 2);
         */
        if((((NodeCnt != (uint16_t)phXCOS->TaskNum) && ((uint16_t)(NodeCnt + 1U) != (uint16_t)phXCOS->TaskNum))) ||
           (NodeCnt > (uint16_t)XC_CFG_MAX_TASKS)) {
            rc = XC_CHK_TASKNUM; // 任务计数与四表节点总数不一致
        }
    }

    XC_Core_Unlock(phXCOS); // 解锁(与上面的 Lock 配对)
    return (rc);
}

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 3) 每任务运行统计(开关: XC_CFG_TASK_STATS) */

#if (XC_CFG_TASK_STATS != 0)

/**
 * @brief       [内部]调度槽计时起点
 * @return      当前系统 Tick
 * @details
 *  与 "XC_Diag_RunStatsEnd" 成对使用: 主循环在"运行任务之前"取一次, 结束时算差值;
 *  起始时刻由调用方的**局部变量**保存 ⇒ 不占任务/框架 RAM;
 *  声明与语义见 `Internal/XC_DiagInternal.h`; 单位/精度说明见 `Docs/XC_Config.md`「XC_CFG_TASK_STATS」;
 */
XC_Tick_t XC_Diag_RunStatsBegin(void)
{
    return (XC_Time_GetTick());
}

/**
 * @brief       [内部]调度槽计时终点: 累计"运行次数 / 最长一次耗时"
 * @param[in]   phTCB       本轮运行的任务
 * @param[in]   RunStart    本轮起始 Tick
 * @details
 *  - `RunCnt`: **饱和计数**(到 `0xFFFFFFFF` 保持, 不回绕);
 *  - `MaxRunTick`: 只增不减;
 *  - 差值用**无符号减法** ⇒ Tick 回绕安全(与 `XC_Time_CheckTimeout` 同思路);
 *  写者只有主循环 ⇒ 无需加锁;
 */
void XC_Diag_RunStatsEnd(XC_TaskHandle_t phTCB, XC_Tick_t RunStart)
{
    XC_Tick_t Elapsed = XC_Time_GetTick() - RunStart; // 无符号减法: Tick 回绕安全

    if(phTCB->RunCnt != 0xFFFFFFFFU) { // 饱和: 到顶保持, 不回绕
        phTCB->RunCnt++;
    }
    if(Elapsed > phTCB->MaxRunTick) { // 记录历史最长
        phTCB->MaxRunTick = Elapsed;
    }
}

/**
 * @brief       [用户]获取任务运行次数
 * @param[in]   phTCB   [XC_TaskHandle_t]任务控制块
 * @return      uint32_t  累计运行次数(饱和计数, 不回绕)
 * @details
 *  详细口径/生命周期见 `XC_Diag.h` 的声明注释 与 `Docs/XC_Config.md`「XC_CFG_TASK_STATS」;
 */
uint32_t XC_Diag_GetRunCnt(XC_TaskHandle_t phTCB)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    return (phTCB->RunCnt);
}

/**
 * @brief       [用户]获取任务"最长一次运行耗时"(单位 Tick)
 * @param[in]   phTCB   [XC_TaskHandle_t]任务控制块
 * @return      XC_Tick_t  历史最长一次调度槽耗时
 */
XC_Tick_t XC_Diag_GetMaxRunTick(XC_TaskHandle_t phTCB)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    return (phTCB->MaxRunTick);
}

/**
 * @brief       [用户]清零任务运行统计(RunCnt / MaxRunTick 都置 0)
 * @param[in]   phTCB   [XC_TaskHandle_t]任务控制块
 * @details
 *  注册/移除时会自动调用(见 "XC_Task_AddInternal"/"XC_Task_HandleRemove");
 *  `XC_Task_Reset` 不清零(排障场景保留历史证据);
 *  注: 任务在**本调度槽内移除自身**时调度器不再累计, 保证"移除后计数为 0";
 */
void XC_Diag_ClrRunStats(XC_TaskHandle_t phTCB)
{
    XC_DIAG_ASSERT(phTCB != NULL);
    phTCB->RunCnt     = 0U;
    phTCB->MaxRunTick = 0U;
}

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
