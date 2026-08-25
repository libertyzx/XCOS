/**
 * @file  repro_bugs.c
 * @brief 复现 XCOS 深度审查发现的 2 个 bug(当前内核源码, 未修改)
 *  Bug#1: 父层 XC_Cor_Call 让出后 TaskState 保持 RUN(应为 READY)
 *  Bug#2: 挂起任务被 EventSched 误唤醒(应保持 SUSPEND)
 * 编译: gcc -std=c99 -Wall -Wextra -I <Code/Inc> repro_bugs.c <Code/Src>/XC_*.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "XCOS.h"
#include "XC_Cor.h"
#include "Internal/XC_Core.h"


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

/* ================= Bug#1: 父层 Call 让出后 TaskState 保持 RUN ================= */
static XC_TaskCB_t    s_TCB_Parent;
static XC_TaskCB_t    s_TCB_Check;
static XC_CorFrame_t  s_ParentStack[2];
static int s_Bug1_StateAtCheck = -1; /* Check 观测到的 Parent 状态 */

void Task_Sub(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Cor_Leave();
}

void Task_Parent(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Cor_Call(Task_Sub); /* 让出点: 登记子层, 无 Delay/Yield, 任务进入就绪表等待 */
    XC_Cor_Leave();
}

void Task_Check1(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    /* 此时 Parent 已完成 Call 让出, 正在就绪表等待 -> 应为 READY */
    s_Bug1_StateAtCheck = (int)XC_Task_GetState(&s_TCB_Parent);
    printf("[Bug#1] Check: Parent state after Call-yield = %d (expect READY=%d, RUN=%d)\n",
           s_Bug1_StateAtCheck, (int)XC_TASK_READY, (int)XC_TASK_RUN);
    exit(0);
    XC_Cor_Leave();
}

static void RunBug1(void)
{
    memset(&g_xc, 0, sizeof(g_xc));
    XC_Sch_Init(&g_xc);
    XC_Task_Reg(&g_xc, &s_TCB_Check, Task_Check1, NULL);      /* 就绪表 [Check] */
    XC_Task_RegExt(&g_xc, &s_TCB_Parent, Task_Parent, NULL, s_ParentStack, 2U); /* [Parent, Check] */
    /* 轮1: Parent 运行 -> Call 让出(游离, TaskState=RUN) -> 归位插就绪表尾
     * 轮2: Check 运行 -> 检查 Parent 状态 */
    XC_Sch_Start(&g_xc);
}

/* ================= Bug#2: 挂起任务被 EventSched 误唤醒 ================= */
static XC_TaskCB_t s_TCB_A;
static XC_TaskCB_t s_TCB_B;
static int s_Bug2_AWokeUp = 0; /* A 被非 Resume 方式唤醒次数(期望 0) */

void Task_A(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Cor_WaitNotify(0U); /* 死等通知 -> 阻塞表, NotifyState=WAIT */
    s_Bug2_AWokeUp++;      /* 若执行到此处 = 被误唤醒(挂起被破坏) */
    while(1) { WD_Check(); XC_Cor_Yield(); }
    XC_Cor_Leave();
}

void Task_B(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    /* 模拟: 中断在"调度器锁窗口"SendNotify(A)(异步, EventProduced++), 随后 Suspend(A) */
    XC_Core_Lock(&g_xc);                       /* 锁=1: 模拟主循环锁区 */
    XC_Task_SendNotify(&s_TCB_A, NULL);        /* A WAIT -> 异步 EventProduced++ (不移表) */
    XC_Core_Unlock(&g_xc);
    XC_Task_Suspend(&s_TCB_A);                 /* A -> 阻塞表 + SUSPEND(仍 WAIT, Produced=1) */
    XC_Cor_Yield();                            /* 让出 -> 主循环 EventSched 触发 */
    /* 再次调度: 检查 A 是否被 EventSched 误唤醒 */
    printf("[Bug#2] A woke-up count = %d (expect 0); A state = %d (expect SUSPEND=%d)\n",
           s_Bug2_AWokeUp, (int)XC_Task_GetState(&s_TCB_A), (int)XC_TASK_SUSPEND);
    exit(0);
    XC_Cor_Leave();
}

static void RunBug2(void)
{
    memset(&g_xc, 0, sizeof(g_xc));
    XC_Sch_Init(&g_xc);
    XC_Task_Reg(&g_xc, &s_TCB_B, Task_B, NULL); /* 就绪表 [B] */
    XC_Task_Reg(&g_xc, &s_TCB_A, Task_A, NULL); /* 就绪表 [A, B] */
    /* 轮1: A 运行 WaitNotify 死等 -> 阻塞表
     * 轮2: B 运行(锁窗口 SendNotify + Suspend) -> 让出
     *      主循环 EventSched -> 若误唤醒 A 则下一轮 A 运行(记录)
     * 轮3: A(若被唤醒) / 轮4: B 检查 */
    XC_Sch_Start(&g_xc);
}

int main(int argc, char* argv[])
{
    WD_Init(10000, "复现测试(状态一致性)");
    printf("===== XCOS Bug Repro (current kernel, unmodified) =====\n");
    if((argc > 1) && (strcmp(argv[1], "bug2") == 0)) {
        RunBug2();
        return 0;
    }
    RunBug1();
    return 0;
}
