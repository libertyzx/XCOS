/**
 * @file  a2_fix_test2.c  (A2 fix verification: suspend wins over event-sched wakeup + resume delivers pending notify)
 * @brief covers A2 semantics:
 *   [T1] suspended task must NOT be woken by EventSched (state stays SUSPEND, never runs)
 *   [T2] Resume delivers the notify registered before suspend (wait side sees "notified") and returns OK
 *   [T3] SendNotify while suspended must FAIL (no data overwrite, pending flag kept)
 *   [T5] suspend with no pending notify -> resume: wait side sees "not notified" (timeout branch)
 *   [T6] regression: normal SendNotify still wakes a resumed task
 * build: gcc -std=gnu99 -Wall -Wextra -I <Code/Inc> a2_fix_test2.c <Code/Src>/XC_*.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "XCOS.h"
#include "XC_Cor.h"
#include "Internal/XC_Core.h"

static clock_t     s_WD_Start = 0;
static clock_t     s_WD_Ms    = 30000;
static const char* s_WD_Tag   = "t";
static void WD_Init(clock_t ms, const char* tag) { s_WD_Start = clock(); s_WD_Ms = ms; s_WD_Tag = tag; }
static void WD_Check(void)
{
    if((clock() - s_WD_Start) >= (clock_t)(s_WD_Ms * (clock_t)CLOCKS_PER_SEC / 1000)) {
        printf("[WATCHDOG] timeout"); putchar(10); fflush(stdout); exit(2);
    }
}
static void NL(void) { putchar(10); }

XCOS_t g_xc;
static XC_TaskCB_t s_TCB_A;
static XC_TaskCB_t s_TCB_B;

static int s_N1 = 0x5A5A; /* notify data registered before suspend */
static int s_N2 = 0x1234; /* notify data rejected during suspend  */
static int s_N3 = 0x7A7A; /* notify data after resume             */

static int        s_A_RunCnt = 0;
static int        s_A_Timeout1 = -1, s_A_Timeout2 = -1, s_A_Timeout3 = -1;
static const int* s_A_Data1 = NULL;
static const int* s_A_Data3 = NULL;

static int        s_Send1 = -1, s_Pend1 = -1, s_Sus1 = -1;
static int        s_T1_state = -1, s_T1_run = -1;
static int        s_T3_send = -1, s_T3_pend = -1;
static const int* s_T3_data = NULL;
static int        s_T2_resume = -1, s_T2_timeout = -1, s_T2_run = -1;
static const int* s_T2_data = NULL;
static int        s_T5_sus = -1, s_T5_state = -1, s_T5_resume = -1, s_T5_timeout2 = -1;
static int        s_T6_send = -1, s_T6_timeout3 = -1, s_T6_run = -1;
static const int* s_T6_data = NULL;
static int        g_preYield = 0; /**< [A9] 前导同步用的静态计数器(不可用局部变量: 跨让出点不保存) */

void Task_A(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Cor_ClrNotify();
    XC_Cor_WaitNotify(0U); /* phase0: dead wait (suspend target) */
    s_A_RunCnt++;
    s_A_Timeout1 = XC_Cor_IsNotifyTimeout() ? 1 : 0;
    s_A_Data1    = (const int*)XC_Cor_GetNotifyData();
    XC_Cor_WaitNotify(0U); /* phase1: suspend without notify -> resume */
    s_A_RunCnt++;
    s_A_Timeout2 = XC_Cor_IsNotifyTimeout() ? 1 : 0;
    XC_Cor_WaitNotify(0U); /* phase2: normal notify after resume */
    s_A_RunCnt++;
    s_A_Timeout3 = XC_Cor_IsNotifyTimeout() ? 1 : 0;
    s_A_Data3    = (const int*)XC_Cor_GetNotifyData();
    while(1) { WD_Check(); XC_Cor_Yield(); }
    XC_Cor_Leave();
}

void Task_B(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);

    /**
     * [A9, 2026-09-14] 前导同步: 唤醒/恢复/注册统一为"排队尾"后, **注册顺序不再保证 A 先运行**
     * (旧行为: 注册插表首 ⇒ 后注册的 A 反而先跑). 本用例的场景要求"A 已进入等待" ⇒ 先让出,
     * 直到 A 进入 BLOCKED(phase0 等待) 为止. (跨让出点不能依赖局部变量 ⇒ 用静态计数器)
     */
    for(g_preYield = 0; g_preYield < 8; g_preYield++) {
        if((int)XC_Task_GetState(&s_TCB_A) == (int)XC_TASK_BLOCKED) {
            break;
        }
        XC_Cor_Yield();
    }

    /* R1: async notify inside lock window, then suspend A */
    XC_Core_Lock(&g_xc);
    s_Send1 = (int)XC_Task_SendNotify(&s_TCB_A, (void*)&s_N1);
    s_Pend1 = (int)s_TCB_A.NotifyPending;
    XC_Core_Unlock(&g_xc);
    s_Sus1 = (int)XC_Task_Suspend(&s_TCB_A);
    XC_Cor_Yield();

    /* R2: A must still be SUSPEND; notify during suspend must FAIL; then Resume */
    s_T1_state  = (int)XC_Task_GetState(&s_TCB_A);
    s_T1_run    = s_A_RunCnt;
    s_T3_send   = (int)XC_Task_SendNotify(&s_TCB_A, (void*)&s_N2);
    s_T3_pend   = (int)s_TCB_A.NotifyPending;
    s_T3_data   = (const int*)XC_Task_ReadNotifyData(&s_TCB_A);
    s_T2_resume = (int)XC_Task_Resume(&s_TCB_A);
    XC_Cor_Yield();

    /* R3: A resumed and recorded phase0 (delivery); suspend it again (no pending notify) */
    s_T2_timeout = s_A_Timeout1;
    s_T2_data    = s_A_Data1;
    s_T2_run     = s_A_RunCnt;
    s_T5_sus     = (int)XC_Task_Suspend(&s_TCB_A);
    XC_Cor_Yield();

    /* R4: A still SUSPEND; resume -> wait side must report "not notified" */
    s_T5_state  = (int)XC_Task_GetState(&s_TCB_A);
    s_T5_resume = (int)XC_Task_Resume(&s_TCB_A);
    XC_Cor_Yield();

    /* R5: A recorded phase1; now a normal notify (direct wake) */
    s_T5_timeout2 = s_A_Timeout2;
    s_T6_send     = (int)XC_Task_SendNotify(&s_TCB_A, (void*)&s_N3);
    XC_Cor_Yield();

    /* R6: A recorded phase2; report */
    s_T6_timeout3 = s_A_Timeout3;
    s_T6_data     = s_A_Data3;
    s_T6_run      = s_A_RunCnt;

    printf("========== A2 fix verification =========="); NL();
    printf("[T1] state after suspend  = %d  (expect SUSPEND=%d)", s_T1_state, (int)XC_TASK_SUSPEND); NL();
    printf("[T1] A run count          = %d  (expect 0)", s_T1_run); NL();
    printf("[T2] Resume return        = %d  (expect OK=%d)", s_T2_resume, (int)XC_OK); NL();
    printf("[T2] timeout flag         = %d  (expect 0 = got notify)", s_T2_timeout); NL();
    printf("[T2] notify data          = %s  (expect 0x5A5A)", (s_T2_data == &s_N1) ? "0x5A5A(original)" : "WRONG!"); NL();
    printf("[T2] A run count          = %d  (expect 1)", s_T2_run); NL();
    printf("[T3] SendNotify suspended = %d  (expect FAIL=%d)", s_T3_send, (int)XC_FAIL); NL();
    printf("[T3] pending flag kept    = %d  (expect 1)", s_T3_pend); NL();
    printf("[T4] data not overwritten = %s  (expect 0x5A5A)", (s_T3_data == &s_N1) ? "0x5A5A(original)" : "OVERWRITTEN!"); NL();
    printf("[T5] state after suspend  = %d  (expect SUSPEND=%d)", s_T5_state, (int)XC_TASK_SUSPEND); NL();
    printf("[T5] timeout flag         = %d  (expect 1 = not notified)", s_T5_timeout2); NL();
    printf("[T6] SendNotify resumed   = %d  (expect OK=%d)", s_T6_send, (int)XC_OK); NL();
    printf("[T6] timeout flag         = %d  (expect 0 = got notify)", s_T6_timeout3); NL();
    printf("[T6] notify data          = %s  (expect 0x7A7A)", (s_T6_data == &s_N3) ? "0x7A7A(new)" : "WRONG!"); NL();
    printf("[T6] A run count          = %d  (expect 3)", s_T6_run); NL();
    printf("[pre] async send return   = %d  (expect CONTINUE=%d)", s_Send1, (int)XC_CONTINUE); NL();
    printf("[pre] pending after send  = %d  (expect 1)", s_Pend1); NL();
    printf("[pre] Suspend return      = %d  (expect OK=%d)", s_Sus1, (int)XC_OK); NL();
    fflush(stdout);
    exit(0);
    XC_Cor_Leave();
}

int main(void)
{
    WD_Init(15000, "A2 fix test");
    printf("===== XCOS A2 fix verification (DEF-20260824-02) ====="); NL();
    memset(&g_xc, 0, sizeof(g_xc));
    XC_Sch_Init(&g_xc);
    XC_Task_Reg(&g_xc, &s_TCB_B, Task_B, NULL);
    XC_Task_Reg(&g_xc, &s_TCB_A, Task_A, NULL);
    XC_Sch_Start(&g_xc);
    return 0;
}