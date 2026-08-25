/**
 * @file  20260824_事件调度误唤醒挂起任务_复现测试.c
 * @brief 复现缺陷 DEF-20260824-02: 挂起任务被 EventSched 误唤醒(应保持 SUSPEND)
 *  触发链: 任务A WaitNotify(WAIT) → 锁窗口异步 SendNotify(Produced++ 未移表)
 *          → 任务B 挂起 A(阻塞表+SUSPEND, 挂起不清除通知)
 *          → 主循环 EventSched 的 WAKEUP_NOTIFY 误唤醒 A
 * 当前内核: 不修复(已知缺陷), 预期复现 woke-up=1 / state=READY
 * 备选修复后: 预期 woke-up=0 / state=SUSPEND(见 Docs/缺陷记录/20260824_事件调度误唤醒挂起任务.md)
 * 编译: gcc -std=c99 -Wall -Wextra -I <Code/Inc> 本文件 <Code/Src>/XC_*.c
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
    /* 模拟: 中断在"调度器锁窗口"SendNotify(A)(异步, EventProduced++), 随后任务上下文 Suspend(A) */
    XC_Core_Lock(&g_xc);                       /* 锁=1: 模拟主循环锁区(中断) */
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

int main(void)
{
    WD_Init(10000, "缺陷复现测试");
    printf("===== XCOS DEF-20260824-02 Repro =====\n");
    memset(&g_xc, 0, sizeof(g_xc));
    XC_Sch_Init(&g_xc);
    XC_Task_Reg(&g_xc, &s_TCB_B, Task_B, NULL); /* 就绪表 [B] */
    XC_Task_Reg(&g_xc, &s_TCB_A, Task_A, NULL); /* 就绪表 [A, B] */
    /* 轮1: A 运行 WaitNotify 死等 -> 阻塞表
     * 轮2: B 运行(锁窗口 SendNotify + Suspend) -> 让出
     *      主循环 EventSched -> 若误唤醒 A 则下一轮 A 运行(记录)
     * 轮3: A(若被唤醒) / 轮4: B 检查 */
    XC_Sch_Start(&g_xc);
    return 0;
}
