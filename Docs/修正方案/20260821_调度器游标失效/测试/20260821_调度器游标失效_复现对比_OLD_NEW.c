#include "XCOS.h"
#include <stdio.h>


/* ===== 测试自我超时看门狗(防止程序卡死; C标准 clock(), 全平台可编译) ===== */
#include <time.h>
static clock_t s_WD_Start = 0;
static clock_t s_WD_TimeoutMs = 30000;
static const char* s_WD_Tag = "test";
static void WD_Init(clock_t timeoutMs, const char* tag)
{
    s_WD_Start = clock();
    s_WD_TimeoutMs = timeoutMs;
    s_WD_Tag = tag;
}
static void WD_Check(void)
{
    if((clock() - s_WD_Start) >= (clock_t)(s_WD_TimeoutMs * (clock_t)CLOCKS_PER_SEC / 1000)) {
        printf("[WATCHDOG] %s: timeout (>%u ms), kill!\n", s_WD_Tag, (unsigned)s_WD_TimeoutMs);
        fflush(stdout);
        exit(2);
    }
}

XCOS_t g_xc;
XC_TaskCB_t gA, gB, gC;
static int b_c = 0, c_c = 0;

void TaskA(XC_TaskHandle_t t) {
    XC_Cor_Enter(t);
    XC_Task_Suspend(&gB);   /* A 挂起 B(就绪表 A 的后继) */
    while(1) { XC_Cor_Yield(); }
    XC_Cor_Leave();
}
void TaskB(XC_TaskHandle_t t) {
    b_c++;
    XC_Cor_Enter(t);
    while(1) { XC_Cor_Yield(); }
    XC_Cor_Leave();
}
void TaskC(XC_TaskHandle_t t) {
    c_c++;
    XC_Cor_Enter(t);
    while(1) { XC_Cor_Yield(); }
    XC_Cor_Leave();
}

static XC_ListNode_t* g_iter;

/* OLD: 跨轮游标(原 XC_Sch_Start) */
static void OldRound(XC_OSHandle_t os) {
    if(XC_List_ListValid(&os->ReadyList)) {
        if(XC_List_ReachEndNode(&os->ReadyList, g_iter)) g_iter = g_iter->pNext;
        XC_TaskHandle_t t = XC_LIST_TO_TCB(g_iter);
        g_iter = g_iter->pNext;
        t->TaskState = XC_TASK_RUN;
        t->fTask(t);
        while((t->CorState == XC_COR_DONE) && (t->CorDepth > 0U)) { XC_Task_PopFrame(t); t->fTask(t); }
        if((t->CorState == XC_COR_DONE) && (t->CorDepth == 0U)) t->TaskState = XC_TASK_READY;
    }
    XC_Time_TickInc();
}

/* NEW: 修复后(首节点取走+游离检测插回, 与 XC_Sch_Start 新实现一致) */
static void NewRound(XC_OSHandle_t os) {
    XC_TaskHandle_t t;
    if(XC_List_ListValid(&os->ReadyList)) {
        t = XC_LIST_TO_TCB(XC_List_GetListStartNode(&os->ReadyList));
        XC_List_Remove(&t->ListNode);
        t->TaskState = XC_TASK_RUN;
        t->fTask(t);
        while((t->CorState == XC_COR_DONE) && (t->CorDepth > 0U)) { XC_Task_PopFrame(t); t->fTask(t); }
        if((t->CorState == XC_COR_DONE) && (t->CorDepth == 0U)) t->TaskState = XC_TASK_READY;
        if((t->TaskState != (uint8_t)XC_TASK_VOID) && (t->ListNode.pNext == &t->ListNode)) {
            XC_List_MoveNodeAfter(os->ReadyList.pPrev, &t->ListNode);
        }
    }
    XC_Time_TickInc();
}

int run(int is_new) {
    XC_Sch_Init(&g_xc);
    b_c = 0; c_c = 0;
    g_iter = &g_xc.ReadyList;
    /* Reg 顺序: B, A, C -> 就绪表 Root->C->A->B, A 位于 B 之前 */
    XC_Task_Reg(&g_xc, &gB, TaskB, NULL);
    XC_Task_Reg(&g_xc, &gA, TaskA, NULL);
    XC_Task_Reg(&g_xc, &gC, TaskC, NULL);
    for(int i = 0; i < 8; i++) { WD_Check(); if(is_new) NewRound(&g_xc); else { OldRound(&g_xc); if(i >= 2) break; } }
    printf("%s: b_c=%d c_c=%d | B_state=%d", is_new ? "NEW" : "OLD", b_c, c_c, (int)XC_Task_GetState(&gB));
    return b_c;
}

int main(void) {
    WD_Init(10000, "游标失效复现对比");
    int old_b = run(0);
    printf("  (挂起任务被运行次数; 期望 0)\n");
    int new_b = run(1);
    printf("  (挂起任务被运行次数; 期望 0)\n");
    printf("RESULT: OLD=%d NEW=%d\n", old_b, new_b);
    if(old_b == 0) { printf("NOTE: OLD 未触发(顺序不符)"); return 2; }
    return (new_b == 0) ? 0 : 1;
}
