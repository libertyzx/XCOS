/**
 * @file       20260914_通知待处理标志_回归测试_sim.c
 * @brief      [仿真通道] 通知"待处理标志"回归测试(MDK Cortex-M3 模拟器)
 * @details
 *  覆盖与宿主版(20260914_通知待处理标志_回归测试.c)相同的用例, 但适配仿真环境:
 *  - 不使用 printf/host I/O: 结果写入 g_res[], 由 sim.ini 在 SimDone 断点处打印;
 *  - 不使用 exit(): 结束时调用 SimDone()(空函数, 供 .ini 用 "g, SimDone" 命中);
 *  ---
 *  用法(详见 Docs/Skill/MDK_Sim_Agent_Skill.md):
 *   1) 镜像工程到临时工作区(Code/ + Examples/CortexM3_Test/), 用本文件替换 Code/Main/main.c;
 *   2) .uvoptx: <uSim>1</uSim>, <sGomain>0</sGomain>, <sIfile> = <sim.ini 绝对路径>;
 *   3) uVision.com -b 构建 => UV4 -d 跑仿真(超时 Kill) => 读 sim_log.txt;
 *  ---
 *  g_res[] 槽位:
 *   [0] 已执行用例数(自检) [1] 失败数(0=全通过)
 *   [2] 用例1: 300 次"只记录"发送后 NotifyPending=1        (1=OK)
 *   [3] 用例1: 等待处理立即返回(未阻塞) —— A1 核心          (1=OK)
 *   [4] 用例1: 消费后标志清 0                              (1=OK)
 *   [5] 用例2: ClrNotify 后标志=0                          (1=OK)
 *   [6] 用例2: 无通知时等待阻塞(TaskState=BLOCKED)         (1=OK)
 *   [7] 用例2: 目标进入阻塞表                              (1=OK)
 *   [8] 用例2: 退出后锁已释放(0)                           (1=OK)
 *   [9] 用例3: 等待中通知 => NotifyState=WAKEUP            (1=OK)
 *  [10] 用例3: 等待中通知 => TaskState=READY               (1=OK)
 *  [11] 用例3: 目标被搬入就绪表                            (1=OK)
 *  [12] 用例3: 目标已离开阻塞表                            (1=OK)
 *  [13] 用例4: 锁窗口内发送 => NotifyPending=1(只登记)      (1=OK)
 *  [14] 用例4: 锁窗口内发送 => EventPending=1              (1=OK)
 *  [15] 用例4: 锁窗口内不移表(TaskState 仍 BLOCKED)        (1=OK)
 *  [16] 用例4: 目标仍在阻塞表                              (1=OK)
 *  [17] 用例5: 端到端(锁窗口异步 -> 事件门控唤醒)           (1=OK)
 *  [20] offsetof(XC_TaskCB_t, NotifyPending)  期望 34
 *  [21] offsetof(XCOS_t,      EventPending)   期望 38
 *  [22] sizeof(XC_TaskCB_t)                   期望 44
 *  [23] sizeof(XCOS_t)                        期望 44
 */
#include <stddef.h>
#include <string.h>
#include "XCOS.h"
#include "XC_Cor.h"
#include "XC_Sch.h"
#include "Internal/XC_Core.h"
#include "Internal/XC_List.h"

XCOS_t           g_xc;
volatile uint32_t g_res[32];

/* 结果槽位 */
#define R_CNT      0U  /* 已执行用例数 */
#define R_FAIL     1U  /* 失败数 */

#define CHK(slot, cond)                        \
    do {                                       \
        g_res[R_CNT]++;                        \
        if(cond) {                             \
            g_res[(slot)] = 1U;                \
        }                                      \
        else {                                 \
            g_res[(slot)] = 0xDEADU;           \
            g_res[R_FAIL]++;                   \
        }                                      \
    } while(0)

/** 供 .ini 用 "g, SimDone" 命中的空函数 */
void SimDone(void)
{
}

/* ===================================================================
 * 白盒用例(不用调度器)
 * =================================================================== */
static XC_TaskCB_t s_tcb;

static void TCB_Reset(void)
{
    if((s_tcb.ListNode.pNext != NULL) && (s_tcb.ListNode.pNext != &s_tcb.ListNode)) {
        XC_List_Remove(&s_tcb.ListNode);
    }
    memset(&s_tcb, 0, sizeof(s_tcb));
    s_tcb.phXCOS        = &g_xc;
    s_tcb.TaskState     = (uint8_t)XC_TASK_READY;
    s_tcb.NotifyState   = (uint8_t)XC_NOTIFY_WAKEUP;
    s_tcb.NotifyPending = 0U;
    XC_List_Init(&s_tcb.ListNode);
}

static uint32_t TCB_IsListHead(XC_ListNode_t* pList)
{
    return ((XC_List_ListValid(pList) != 0) && (XC_List_GetListStartNode(pList) == &s_tcb.ListNode)) ? 1U : 0U;
}

static void WhiteBoxCases(void)
{
    uint32_t i;

    /* 用例1: A1 —— 300 次"只记录"发送后, 等待应立即返回 */
    TCB_Reset();
    for(i = 0U; i < 300U; i++) {
        (void)XC_Task_SendNotify(&s_tcb, NULL);
    }
    CHK(2U, s_tcb.NotifyPending == 1U);
    XC_Task_HandleWaitNotify(&s_tcb, 0U);
    CHK(3U, s_tcb.TaskState == (uint8_t)XC_TASK_READY);
    CHK(4U, s_tcb.NotifyPending == 0U);

    /* 用例2: ClrNotify 后等待必须阻塞 */
    TCB_Reset();
    (void)XC_Task_SendNotify(&s_tcb, NULL);
    XC_Task_ClrNotify(&s_tcb);
    CHK(5U, s_tcb.NotifyPending == 0U);
    XC_Task_HandleWaitNotify(&s_tcb, 0U);
    CHK(6U, s_tcb.TaskState == (uint8_t)XC_TASK_BLOCKED);
    CHK(7U, TCB_IsListHead(&g_xc.BlockedList) == 1U);
    CHK(8U, XC_Core_GetLockState(&g_xc) == 0U);

    /* 用例3: 等待中被通知(Lock==0 直接路径) => 唤醒 + 搬入就绪表 */
    TCB_Reset();
    s_tcb.NotifyState = (uint8_t)XC_NOTIFY_WAIT;
    s_tcb.TaskState   = (uint8_t)XC_TASK_BLOCKED;
    XC_List_InsertNodeAfter(&g_xc.BlockedList, &s_tcb.ListNode);
    (void)XC_Task_SendNotify(&s_tcb, NULL);
    CHK(9U, s_tcb.NotifyState == (uint8_t)XC_NOTIFY_WAKEUP);
    CHK(10U, s_tcb.TaskState == (uint8_t)XC_TASK_READY);
    CHK(11U, TCB_IsListHead(&g_xc.ReadyList) == 1U);
    CHK(12U, TCB_IsListHead(&g_xc.BlockedList) == 0U);

    /* 用例4: 锁窗口内发送 => 只登记(不移表) */
    TCB_Reset();
    s_tcb.NotifyState = (uint8_t)XC_NOTIFY_WAIT;
    s_tcb.TaskState   = (uint8_t)XC_TASK_BLOCKED;
    XC_List_InsertNodeAfter(&g_xc.BlockedList, &s_tcb.ListNode);
    XC_Core_Lock(&g_xc);
    (void)XC_Task_SendNotify(&s_tcb, NULL);
    XC_Core_Unlock(&g_xc);
    CHK(13U, s_tcb.NotifyPending == 1U);
    CHK(14U, g_xc.EventPending == 1U);
    CHK(15U, s_tcb.TaskState == (uint8_t)XC_TASK_BLOCKED);
    CHK(16U, TCB_IsListHead(&g_xc.BlockedList) == 1U);

    /* 清理: 白盒 TCB 不参与后续端到端调度 */
    TCB_Reset();
    s_tcb.TaskState = (uint8_t)XC_TASK_VOID;
}

/* ===================================================================
 * 用例5(端到端): 锁窗口异步通知 -> 事件门控(先清后处理) -> 唤醒
 * =================================================================== */
static XC_TaskCB_t s_TCB_A;
static XC_TaskCB_t s_TCB_B;
static XC_TaskCB_t s_TCB_C;

void Task_E2E_Sender(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Core_Lock(&g_xc); /* 模拟"中断在主循环锁窗口内发送通知" */
    (void)XC_Task_SendNotify(&s_TCB_A, NULL);
    XC_Core_Unlock(&g_xc);
    while(1) {
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

void Task_E2E_Target(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Cor_WaitNotify(0U); /* 死等 => 阻塞表 + WAIT */
    CHK(17U, 1);           /* 走到这里 = 被唤醒 */
    while(1) {
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
    if(g_res[17U] != 1U) { /* 目标未被唤醒 => 记一次失败 */
        g_res[R_CNT]++;
        g_res[R_FAIL]++;
    }
    SimDone(); /* 断点命中点: sim.ini 在此打印 g_res[] */
    while(1) {
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

int main(void)
{
    memset(&g_xc, 0, sizeof(g_xc));
    XC_Sch_Init(&g_xc);

    /* 静态检查: 字段偏移与结构大小(设计: 34 / 38 / 44 / 44) */
    g_res[20] = (uint32_t)offsetof(struct XC_TaskCB_tag, NotifyPending);
    g_res[21] = (uint32_t)offsetof(struct XCOS_tag, EventPending);
    g_res[22] = (uint32_t)sizeof(XC_TaskCB_t);
    g_res[23] = (uint32_t)sizeof(XCOS_t);

    WhiteBoxCases();

    /* 就绪表为"表首插入", 注册顺序与运行顺序相反 => 期望运行顺序 A -> B -> C */
    (void)XC_Task_Reg(&g_xc, &s_TCB_C, Task_E2E_Check, NULL);
    (void)XC_Task_Reg(&g_xc, &s_TCB_B, Task_E2E_Sender, NULL);
    (void)XC_Task_Reg(&g_xc, &s_TCB_A, Task_E2E_Target, NULL);

    XC_Sch_Start(&g_xc); /* 不返回; 由 Task_E2E_Check 调用 SimDone() 结束 */
    return 0;
}
