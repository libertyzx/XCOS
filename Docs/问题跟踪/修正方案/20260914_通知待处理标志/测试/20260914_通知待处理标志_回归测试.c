/**
 * @file       20260914_通知待处理标志_回归测试.c
 * @brief      回归测试: 通知"待处理标志"改造(A1 彻底修复)
 * @details
 *  关联: Docs/问题跟踪/修正方案/20260914_通知待处理标志/修正方案.md
 * ---
 *  用例:
 *   1) **[A1 核心]** 300 次"只记录"型发送(跨过原 255 回绕点)后,等待处理必须**立即返回**(不阻塞);
 *      旧实现(双计数): 此处回绕 => Produced==Consumed => 误判"无通知" => 入表阻塞 = 丢唤醒 => 本用例应 [FAIL];
 *   2) `ClrNotify` 清标志后,等待必须**阻塞**(进入阻塞表,且锁已释放);
 *   3) 等待中被通知(`Lock==0` 直接路径)必须**唤醒**并搬入就绪表;
 *   4) 锁窗口内发送**只登记**: `NotifyPending`/`EventPending` 置位、**不移表**、目标仍在阻塞表;
 *   5) 端到端: 锁窗口异步通知 -> 主循环事件门控(先清后处理) -> 目标被唤醒。
 * ---
 *  编译(宿主, gcc):
 *    gcc -std=c99 -Wall -Wextra -I Code/Inc ^
 *        Docs/问题跟踪/修正方案/20260914_通知待处理标志/测试/20260914_通知待处理标志_回归测试.c ^
 *        Code/Src/XC_*.c -o t_notify.exe && t_notify.exe
 *  期望: 全部 [PASS], 退出码 0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "XCOS.h"
#include "XC_Cor.h"
#include "Internal/XC_Core.h"
#include "Internal/XC_List.h"

/* ===== 自我超时看门狗(防止死等/卡死) ===== */
static clock_t s_WD_Start = 0;
static clock_t s_WD_TimeoutMs = 10000;

static void WD_Init(clock_t timeoutMs)
{
    s_WD_Start     = clock();
    s_WD_TimeoutMs = timeoutMs;
}
static void WD_Check(void)
{
    if((clock() - s_WD_Start) >= (clock_t)(s_WD_TimeoutMs * (clock_t)CLOCKS_PER_SEC / 1000)) {
        printf("[FAIL] watchdog timeout (dead wait / lost wakeup?)\n");
        fflush(stdout);
        exit(2);
    }
}

/* ===== 断言 ===== */
static int s_Fail = 0;
#define CHECK(cond, msg)                                         \
    do {                                                         \
        if((cond)) {                                             \
            printf("[PASS] %s\n", (msg));                        \
        }                                                        \
        else {                                                   \
            printf("[FAIL] %s  <-- line %d\n", (msg), __LINE__); \
            s_Fail++;                                            \
        }                                                        \
    } while(0)

XCOS_t g_xc;

/* ===================================================================
 * 白盒用例(不用调度器): 直接调内部处理函数, 检查状态与链表
 * =================================================================== */
static XC_TaskCB_t s_tcb;

/** 复位被测 TCB(先摘除节点, 再清零) */
static void TCB_Reset(void)
{
    if((s_tcb.ListNode.pNext != NULL) && (s_tcb.ListNode.pNext != &s_tcb.ListNode)) {
        XC_List_Remove(&s_tcb.ListNode); /* 从原表摘除, 避免悬挂 */
    }
    memset(&s_tcb, 0, sizeof(s_tcb));
    s_tcb.phXCOS        = &g_xc;
    s_tcb.TaskState     = XC_TASK_READY;
    s_tcb.NotifyState   = XC_NOTIFY_WAKEUP;
    s_tcb.NotifyPending = 0U;
    XC_List_Init(&s_tcb.ListNode); /* 自环 = 游离(不在任何表) */
}

/** 被测 TCB 是否为该表首节点 */
static int TCB_IsListHead(XC_ListNode_t* pList)
{
    return ((XC_List_ListValid(pList) != 0) && (XC_List_GetListStartNode(pList) == &s_tcb.ListNode)) ? 1 : 0;
}

static void WhiteBoxCases(void)
{
    int i;

    /* 用例1: A1 —— 300 次"只记录"发送后, 等待立即返回 */
    TCB_Reset();
    for(i = 0; i < 300; i++) {
        (void)XC_Task_SendNotify(&s_tcb, NULL);
    }
    CHECK(s_tcb.NotifyPending == 1U, "Case1: after 300 sends, pending flag = 1 (old impl wrapped to 0)");
    XC_Task_HandleWaitNotify(&s_tcb, 0U); /* 有通知: 立即消费, 不阻塞 */
    CHECK(s_tcb.TaskState == (uint8_t)XC_TASK_READY, "Case1: HandleWaitNotify returned immediately (not blocked) [A1]");
    CHECK(s_tcb.NotifyPending == 0U, "Case1: pending flag cleared after consume");
    CHECK(s_tcb.NotifyState == (uint8_t)XC_NOTIFY_WAKEUP, "Case1: NotifyState back to WAKEUP");
    CHECK(s_tcb.ListNode.pNext == &s_tcb.ListNode, "Case1: node still detached (not in any list)");

    /* 用例2: ClrNotify 后, 等待必须阻塞 */
    TCB_Reset();
    (void)XC_Task_SendNotify(&s_tcb, NULL); /* 提前通知 */
    XC_Task_ClrNotify(&s_tcb);              /* 清除通知 */
    CHECK(s_tcb.NotifyPending == 0U, "Case2: flag = 0 after ClrNotify");
    XC_Task_HandleWaitNotify(&s_tcb, 0U);   /* 无通知: 入阻塞表 */
    CHECK(s_tcb.TaskState == (uint8_t)XC_TASK_BLOCKED, "Case2: wait blocks when no notification");
    CHECK(TCB_IsListHead(&g_xc.BlockedList) == 1, "Case2: target inserted into BlockedList");
    CHECK(XC_Core_GetLockState(&g_xc) == 0U, "Case2: lock released (0) after return");

    /* 用例3: 等待中被通知(Lock==0 直接路径) => 唤醒 + 搬入就绪表 */
    TCB_Reset();
    s_tcb.NotifyState = (uint8_t)XC_NOTIFY_WAIT; /* 模拟"正在等待" */
    s_tcb.TaskState   = (uint8_t)XC_TASK_BLOCKED;
    XC_List_InsertNodeAfter(&g_xc.BlockedList, &s_tcb.ListNode);
    (void)XC_Task_SendNotify(&s_tcb, NULL); /* Lock==0 => 直接搬表 */
    CHECK(s_tcb.NotifyState == (uint8_t)XC_NOTIFY_WAKEUP, "Case3: notify while waiting -> WAKEUP");
    CHECK(s_tcb.TaskState == (uint8_t)XC_TASK_READY, "Case3: notify while waiting -> READY");
    CHECK(TCB_IsListHead(&g_xc.ReadyList) == 1, "Case3: target moved to ReadyList head (runs next)");
    CHECK(TCB_IsListHead(&g_xc.BlockedList) == 0, "Case3: target left BlockedList");

    /* 用例4: 锁窗口内发送 => 只登记(不移表) */
    TCB_Reset();
    s_tcb.NotifyState = (uint8_t)XC_NOTIFY_WAIT;
    s_tcb.TaskState   = (uint8_t)XC_TASK_BLOCKED;
    XC_List_InsertNodeAfter(&g_xc.BlockedList, &s_tcb.ListNode);
    XC_Core_Lock(&g_xc); /* 模拟主循环处于锁窗口(中断将走异步分支) */
    (void)XC_Task_SendNotify(&s_tcb, NULL);
    XC_Core_Unlock(&g_xc);
    CHECK(s_tcb.NotifyPending == 1U, "Case4: send inside lock window -> pending flag set");
    CHECK(g_xc.EventPending == 1U, "Case4: send inside lock window -> event flag set (deferred)");
    CHECK(s_tcb.TaskState == (uint8_t)XC_TASK_BLOCKED, "Case4: no list move inside lock window (still BLOCKED)");
    CHECK(TCB_IsListHead(&g_xc.BlockedList) == 1, "Case4: target still in BlockedList");

    /* 清理白盒 TCB: 不参与后续端到端调度 */
    TCB_Reset();
    s_tcb.TaskState = (uint8_t)XC_TASK_VOID;
}

/* ===================================================================
 * 用例5(端到端): 锁窗口异步通知 -> 事件门控(先清后处理) -> 唤醒
 * =================================================================== */
static XC_TaskCB_t s_TCB_A;
static XC_TaskCB_t s_TCB_B;
static XC_TaskCB_t s_TCB_C;

static int s_E2E_AWoke = 0;

void Task_E2E_Sender(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Core_Lock(&g_xc); /* 模拟"中断在主循环锁窗口内发送通知" */
    (void)XC_Task_SendNotify(&s_TCB_A, NULL);
    XC_Core_Unlock(&g_xc);
    while(1) {
        WD_Check();
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

void Task_E2E_Target(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Cor_WaitNotify(0U); /* 死等 => 阻塞表 + WAIT */
    s_E2E_AWoke = 1;       /* 走到这里 = 被唤醒 */
    while(1) {
        WD_Check();
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

void Task_E2E_Check(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Cor_Yield();
    XC_Cor_Yield();
    XC_Cor_Yield();
    XC_Cor_Yield();
    CHECK(s_E2E_AWoke == 1, "Case5: async notify (lock window) woken by EventSched (E2E)");
    printf("===== regression done: %s (failures %d) =====\n", (s_Fail == 0) ? "ALL PASS" : "FAILURES", s_Fail);
    fflush(stdout);
    exit((s_Fail == 0) ? 0 : 1);
    XC_Cor_Leave();
}

int main(void)
{
    WD_Init(10000);
    printf("===== XCOS notify-pending-flag regression test (A1) =====\n");

    memset(&g_xc, 0, sizeof(g_xc));
    XC_Sch_Init(&g_xc);

    WhiteBoxCases();

    /* 就绪表为"表首插入",注册顺序与运行顺序相反 => 期望运行顺序 A -> B -> C */
    XC_Task_Reg(&g_xc, &s_TCB_C, Task_E2E_Check, NULL);
    XC_Task_Reg(&g_xc, &s_TCB_B, Task_E2E_Sender, NULL);
    XC_Task_Reg(&g_xc, &s_TCB_A, Task_E2E_Target, NULL);

    XC_Sch_Start(&g_xc); /* 不返回(由 Task_E2E_Check 退出) */
    return 0;
}
