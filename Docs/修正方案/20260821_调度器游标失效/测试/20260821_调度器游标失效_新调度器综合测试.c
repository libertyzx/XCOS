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

/* ===== 测试驱动: 复制仓库 TimeSched/EventSched/主循环(与 XC_Sch.c 修复后一致) ===== */
#define GET_NEXT_WAKEUP(os) ((XC_LIST_TO_TCB(XC_List_GetListStartNode(&(os)->TimeList)))->TaskWakeupTick)

static void TestTimeSched(XC_OSHandle_t os)
{
    XC_ListNode_t* pIterator;
    XC_TaskHandle_t phTCB;
    XC_Tick_t Tick;
    XC_ListNode_t* pTimeList;
    pTimeList = &os->TimeList;
    Tick = XC_Time_GetTick();
    XC_Core_Lock(os);
    if(os->PrevTick > Tick) {
        if(XC_List_ListValid(pTimeList)) {
            pIterator = XC_List_GetListStartNode(pTimeList);
            do {
                XC_LIST_TO_TCB(pIterator)->TaskState = XC_TASK_READY;
                pIterator = pIterator->pNext;
            } while(!XC_List_ReachEndNode(pTimeList, pIterator));
            XC_List_MoveListToNodeAfter(&os->ReadyList, pTimeList);
        }
        if(XC_List_ListValid(&os->TimeOverflowList)) {
            XC_List_MoveListToNodeAfter(pTimeList, &os->TimeOverflowList);
        }
        os->PrevTick = Tick;
    }
    if((XC_List_ListValid(pTimeList)) && (Tick >= GET_NEXT_WAKEUP(os))) {
        pIterator = XC_List_GetListStartNode(pTimeList);
        while(Tick >= XC_LIST_TO_TCB(pIterator)->TaskWakeupTick) {
            phTCB = XC_LIST_TO_TCB(pIterator);
            pIterator = pIterator->pNext;
            XC_Task_MoveToReadyList(phTCB);
            phTCB->TaskState = XC_TASK_READY;
            if(XC_List_ReachEndNode(pTimeList, pIterator)) break;
        }
        os->PrevTick = Tick;
    }
    XC_Core_Unlock(os);
}

static void TestEventSched(XC_OSHandle_t os)
{
#define WAKEUP_NOTIFY(pCList)                                                                  \
    {                                                                                          \
        XC_ListNode_t* pList     = pCList;                                                     \
        XC_ListNode_t* pIterator = XC_List_GetListStartNode(pList);                            \
        if(!XC_List_ReachEndNode(pList, pIterator)) {                                          \
            do {                                                                               \
                if((XC_LIST_TO_TCB(pIterator)->NotifyState == (uint8_t)XC_NOTIFY_WAIT) &&      \
                   (XC_LIST_TO_TCB(pIterator)->NotifyProduced != XC_LIST_TO_TCB(pIterator)->NotifyConsumed)) { \
                    XC_LIST_TO_TCB(pIterator)->NotifyConsumed = XC_LIST_TO_TCB(pIterator)->NotifyProduced;    \
                    XC_TaskHandle_t phTCB = XC_LIST_TO_TCB(pIterator);                         \
                    pIterator = pIterator->pNext;                                              \
                    XC_Task_MoveToReadyList(phTCB);                                            \
                    phTCB->NotifyState = XC_NOTIFY_WAKEUP;                                     \
                    phTCB->TaskState   = XC_TASK_READY;                                        \
                } else {                                                                       \
                    pIterator = pIterator->pNext;                                              \
                }                                                                              \
            } while(!XC_List_ReachEndNode(pList, pIterator));                                  \
        }                                                                                      \
    }
    XC_Core_Lock(os);
    WAKEUP_NOTIFY(&os->TimeList);
    WAKEUP_NOTIFY(&os->TimeOverflowList);
    WAKEUP_NOTIFY(&os->BlockedList);
    XC_Core_Unlock(os);
}

static void SchedStep(XC_OSHandle_t os)
{
    XC_TaskHandle_t t;
    /* 复刻修改后 XC_Sch_Start: ListValid 锁外判断 + 锁内取首节点(游离化) + 自环判定插回 */
    if(XC_List_ListValid(&os->ReadyList)) {
        XC_Core_Lock(os);
        t = XC_LIST_TO_TCB(XC_List_GetListStartNode(&os->ReadyList));
        XC_List_Remove(&t->ListNode);            /* 等价 XC_SCH_TAKE_FIRST: 摘除, 节点自环(游离) */
        t->TaskState = XC_TASK_RUN;
        XC_Core_Unlock(os);
        t->fTask(t);
        while((t->CorState == XC_COR_DONE) && (t->CorDepth > 0U)) { XC_Task_PopFrame(t); t->fTask(t); }
        /* 状态一致性(与当前 XC_Sch.c 一致): 归位(锁内)插回就绪表尾时置 READY,
         * 不再使用"仅 CorState==DONE 置 READY"的旧逻辑(父层 Call 让出漏置/覆盖挂起) */
        if((t->TaskState != (uint8_t)XC_TASK_VOID) && (t->ListNode.pNext == &t->ListNode)) {
            XC_Core_Lock(os);
            XC_List_MoveNodeAfter(os->ReadyList.pPrev, &t->ListNode);   /* 等价 XC_SCH_PUT_TAIL: 插表尾 */
            t->TaskState = XC_TASK_READY;                                /* 归位锁内置 READY */
            XC_Core_Unlock(os);
        }
    }
    TestTimeSched(os);
    if(os->EventProduced != os->EventConsumed) {
        os->EventConsumed = os->EventProduced;
        TestEventSched(os);
    }
    XC_Time_TickInc();
}

/* ===== 场景任务 ===== */
static XCOS_t g;
static XC_TaskCB_t gA, gB, gC, gD;
static int g_bc=0, g_cc=0, g_dc=0, g_rx=0, g_ac=0;

void TaskY(XC_TaskHandle_t t) { XC_Cor_Enter(t); while(1){ XC_Cor_Yield(); } XC_Cor_Leave(); }  /* 让出 */
void TaskA_SuspendB(XC_TaskHandle_t t) { XC_Cor_Enter(t); XC_Task_Suspend(&gB); while(1){ XC_Cor_Yield(); } XC_Cor_Leave(); }
void TaskA_RemoveB(XC_TaskHandle_t t)  { XC_Cor_Enter(t); XC_Task_Remove(&gB); while(1){ XC_Cor_Yield(); } XC_Cor_Leave(); }
void TaskB(XC_TaskHandle_t t)          { g_bc++; XC_Cor_Enter(t); while(1){ XC_Cor_Yield(); } XC_Cor_Leave(); }
void TaskC(XC_TaskHandle_t t)          { g_cc++; XC_Cor_Enter(t); while(1){ XC_Cor_Yield(); } XC_Cor_Leave(); }
void TaskD(XC_TaskHandle_t t)          { g_dc++; XC_Cor_Enter(t); while(1){ XC_Cor_Yield(); } XC_Cor_Leave(); }
void TaskA(XC_TaskHandle_t t)          { g_ac++; XC_Cor_Enter(t); while(1){ XC_Cor_Yield(); } XC_Cor_Leave(); }

void TaskDelay3(XC_TaskHandle_t t) { XC_Cor_Enter(t); while(1){ XC_Cor_DelayMs(3); g_bc++; } XC_Cor_Leave(); }
void TaskDelay5(XC_TaskHandle_t t) { XC_Cor_Enter(t); while(1){ XC_Cor_DelayMs(5); g_cc++; } XC_Cor_Leave(); }
void TaskNotify(XC_TaskHandle_t t)  { XC_Cor_Enter(t); while(1){ XC_Cor_ClrNotify(); XC_Cor_WaitNotifyMs(0); if(!XC_Cor_IsNotifyTimeout()){ g_rx++; } } XC_Cor_Leave(); }

int main(void)
{
    WD_Init(10000, "调度器综合测试");
    int i, ok = 1;
    /* S1: Suspend 后继任务 */
    XC_Sch_Init(&g); g_bc=g_cc=g_ac=0;
    XC_Task_Reg(&g, &gB, TaskB, NULL); XC_Task_Reg(&g, &gA, TaskA_SuspendB, NULL); XC_Task_Reg(&g, &gC, TaskC, NULL);
    for(i=0;i<12;i++) { WD_Check(); SchedStep(&g); }
    printf("S1 Suspend后继: b_c=%d(exp0) B_state=%d(exp4) ", g_bc, (int)XC_Task_GetState(&gB));
    if(g_bc==0 && XC_Task_GetState(&gB)==XC_TASK_SUSPEND) printf("PASS\n"); else { printf("FAIL\n"); ok=0; }

    /* S2: Remove 后继任务 */
    XC_Sch_Init(&g); g_bc=g_cc=0;
    XC_Task_Reg(&g, &gB, TaskB, NULL); XC_Task_Reg(&g, &gA, TaskA_RemoveB, NULL); XC_Task_Reg(&g, &gC, TaskC, NULL);
    for(i=0;i<12;i++) { WD_Check(); SchedStep(&g); }
    printf("S2 Remove后继: b_c=%d(exp0) B_state=%d(exp0=VOID) ", g_bc, (int)XC_Task_GetState(&gB));
    if(g_bc==0 && XC_Task_GetState(&gB)==XC_TASK_VOID) printf("PASS\n"); else { printf("FAIL\n"); ok=0; }

    /* S3: 多任务轮转(公平) */
    XC_Sch_Init(&g); g_ac=g_bc=g_cc=0;
    XC_Task_Reg(&g, &gA, TaskA, NULL); XC_Task_Reg(&g, &gB, TaskB, NULL); XC_Task_Reg(&g, &gC, TaskC, NULL);
    for(i=0;i<12;i++) { WD_Check(); SchedStep(&g); }
    printf("S3 轮转: a=%d b=%d c=%d ", g_ac, g_bc, g_cc);
    if(g_ac==4 && g_bc==4 && g_cc==4) printf("PASS\n"); else { printf("FAIL\n"); ok=0; }

    /* S4: Delay 到期唤醒 */
    XC_Sch_Init(&g); g_bc=g_cc=0;
    XC_Task_Reg(&g, &gB, TaskDelay3, NULL); XC_Task_Reg(&g, &gC, TaskDelay5, NULL);
    for(i=0;i<60;i++) { WD_Check(); SchedStep(&g); }
    printf("S4 Delay唤醒: d3=%d(exp14~15) d5=%d(exp9~10) ", g_bc, g_cc);
    if(g_bc>=12 && g_bc<=16 && g_cc>=8 && g_cc<=11) printf("PASS\n"); else { printf("FAIL\n"); ok=0; }

    /* S5: 通知唤醒(阻塞表) */
    XC_Sch_Init(&g); g_rx=0;
    XC_Task_Reg(&g, &gA, TaskNotify, NULL);
    for(i=0;i<30;i++) {
        WD_Check();
        SchedStep(&g);
        if(i%3==0) XC_Task_SendNotify(&gA, (void*)1);
    }
    printf("S5 通知唤醒: rx=%d(exp>0) ", g_rx);
    if(g_rx>0) printf("PASS\n"); else { printf("FAIL\n"); ok=0; }

    /* S6: 全部阻塞->就绪表空(空闲稳定) */
    XC_Sch_Init(&g); g_bc=g_cc=0;
    XC_Task_Reg(&g, &gB, TaskDelay3, NULL); XC_Task_Reg(&g, &gC, TaskDelay5, NULL);
    for(i=0;i<2;i++) { WD_Check(); SchedStep(&g); }
    printf("S6 就绪表空: ready_valid=%d(exp0) ", XC_List_ListValid(&g.ReadyList));
    if(XC_List_ListValid(&g.ReadyList)==0) printf("PASS\n"); else { printf("FAIL\n"); ok=0; }

    printf(ok ? "ALL SCHED V2 TESTS PASS\n" : "SOME TESTS FAILED\n");
    return ok ? 0 : 1;
}
