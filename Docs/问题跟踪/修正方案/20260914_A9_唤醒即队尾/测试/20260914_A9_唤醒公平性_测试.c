/* A9 验证用例: 唤醒/入表一律"排队尾"(回归 V2.0 语义) —— 顺序可确定性断言
 *  ---------------------------------------------------------------------------
 *  场景1(定时唤醒, TimeSched): 周期任务 P(延时2 tick) + 普通任务 R(每次推进 2 tick 再让出)
 *      修复前(P 插表首): trace = P R | P R | ...      ← P 被唤醒后**抢到 R 前面**
 *      修复后(P 排队尾): trace = P R R | P R R | ...  ← P 被唤醒后排在 R 之后(公平轮转)
 *  场景2(通知唤醒, SendNotify): R1(通知者) + R2(观察者) + Z(等待通知)
 *      修复前(Z 插表首): trace = Z 1 Z 2 ...          ← Z 抢到 R2 前面(插队)
 *      修复后(Z 排队尾): trace = 1 2 Z 1 2 Z ...      ← Z 排队尾(不插队)
 *  ---------------------------------------------------------------------------
 *  说明: 本用例用 trace(每个被调度任务的首字符) + 固定调度次数 + longjmp 跳出调度器,
 *        因此**完全确定**(无需线程/真实中断); "过载⇒独占饿死"是插队的极端后果, 需真实
 *        ISR/线程源才能确定性复现(见 Tests/README.md 的跳过项说明与文档分析)。
 *  编译: gcc -std=gnu99 -O1 -Wall -Wextra -I Code/Inc ^
 *        Code/Src/XC_Diag.c Code/Src/XC_List.c Code/Src/XC_Sch.c Code/Src/XC_Task.c Code/Src/XC_Time.c ^
 *        <本文件> -o a9.exe   (PowerShell 用反引号续行; cmd 用 ^)
 */
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "XCOS.h"

static int g_fail = 0;

#define CHK(cond, msg)                          \
    do {                                        \
        if(cond) {                              \
            printf("  PASS  %s\n", (msg));      \
        } else {                                \
            printf("  FAIL  %s\n", (msg));      \
            g_fail++;                           \
        }                                       \
    } while(0)

/* ---- 调度轨迹 ---- */
static char g_trace[64];
static int  g_traceN = 0;
static int  g_disp   = 0;
static int  g_stop   = 0;
static jmp_buf g_env;

static void Tr(char c)
{
    if(g_traceN < ((int)sizeof(g_trace) - 1)) {
        g_trace[g_traceN++] = c;
        g_trace[g_traceN]   = '\0';
    }
}

static void Tick(char c, int stopN)
{
    Tr(c);
    if(++g_disp >= stopN) {
        longjmp(g_env, 1); /* 跳出调度器(仅测试用; 本次调度槽未走"归位") */
    }
}

/* ================= 场景 1: 定时唤醒(TimeSched) ================= */
static XCOS_t      g_os1;
static XC_TaskCB_t g_tP, g_tR;
static int         g_pRuns = 0, g_rRuns = 0;

/** 周期任务: 每次延时 2 tick(由 R 推进 tick 至到期) */
static void TaskP(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
P_LOOP:
    g_pRuns++;
    Tick('P', 9);
    XC_Cor_DelayTick(2U);
    goto P_LOOP;
    XC_Cor_Leave();
}

/** 普通任务: 每次推进 2 tick(模拟"时间流逝")后让出 */
static void TaskR(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
R_LOOP:
    g_rRuns++;
    Tick('R', 9);
    g_SysTickCount += 2U; /* 推进 tick: 使 P 的到期在本轮内可见 */
    XC_Cor_Yield();
    goto R_LOOP;
    XC_Cor_Leave();
}

/* ================= 场景 2: 通知唤醒(SendNotify) ================= */
static XCOS_t      g_os2;
static XC_TaskCB_t g_t1, g_t2, g_tZ;
static int         g_zWait = 0, g_zWakes = 0;

/** 通知者: Z 进入等待后, 由它发通知(每次运行都发) */
static void TaskN1(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
N1_LOOP:
    Tick('1', 9);
    if(g_zWait != 0) {
        (void)XC_Task_SendNotify(&g_tZ, NULL); /* 通知唤醒 Z */
    }
    XC_Cor_Yield();
    goto N1_LOOP;
    XC_Cor_Leave();
}

/** 观察者: 只让出(用于判定"Z 是否插到它前面") */
static void TaskN2(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
N2_LOOP:
    Tick('2', 9);
    XC_Cor_Yield();
    goto N2_LOOP;
    XC_Cor_Leave();
}

/** 等待通知的任务 */
static void TaskZ(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
Z_LOOP:
    Tick('Z', 9);
    g_zWait = 1;
    XC_Cor_WaitNotify(0U); /* 死等通知(阻塞) */
    g_zWait = 0;
    g_zWakes++;
    goto Z_LOOP;
    XC_Cor_Leave();
}

static void RunScheduler(XCOS_t* pOs, int stopN)
{
    g_disp  = 0;
    g_stop  = stopN;
    g_traceN = 0;
    g_trace[0] = '\0';
    if(setjmp(g_env) == 0) {
        XC_Sch_Start(pOs); /* 死循环: 由任务内 longjmp 返回 */
    }
}

int main(void)
{
    printf("== A9 用例: 唤醒/入表是否排队尾(V2.0 语义) ==\n");

    /* ---------- 场景 1: 定时唤醒 ---------- */
    printf("\n[场景1] 定时唤醒(TimeSched): 期望 trace = \"PRRPRRPRR\"(P 排队尾)\n");
    (void)memset(&g_os1, 0, sizeof(g_os1));
    (void)memset(&g_tP, 0, sizeof(g_tP));
    (void)memset(&g_tR, 0, sizeof(g_tR));
    g_SysTickCount = 0U;
    XC_Sch_Init(&g_os1);
    CHK(XC_Task_Reg(&g_os1, &g_tP, TaskP, NULL) == XC_OK, "注册周期任务 P");
    CHK(XC_Task_Reg(&g_os1, &g_tR, TaskR, NULL) == XC_OK, "注册普通任务 R");
    RunScheduler(&g_os1, 9);
    printf("       实测 trace = \"%s\"  (P=%d 次, R=%d 次)\n", g_trace, g_pRuns, g_rRuns);
    CHK(g_pRuns >= 2, "周期任务有被唤醒运行");
    CHK(g_rRuns >= 4, "普通任务未被饿死(与 P 轮转)");
    CHK(strcmp(g_trace, "PRRPRRPRR") == 0, "场景1 trace == \"PRRPRRPRR\"(P 唤醒后排在 R 之后)");

    /* ---------- 场景 2: 通知唤醒 ---------- */
    printf("\n[场景2] 通知唤醒(SendNotify): 期望 trace = \"12Z12Z12Z\"(Z 排队尾)\n");
    (void)memset(&g_os2, 0, sizeof(g_os2));
    (void)memset(&g_t1, 0, sizeof(g_t1));
    (void)memset(&g_t2, 0, sizeof(g_t2));
    (void)memset(&g_tZ, 0, sizeof(g_tZ));
    g_zWait = 0;
    g_zWakes = 0;
    g_SysTickCount = 0U;
    XC_Sch_Init(&g_os2);
    CHK(XC_Task_Reg(&g_os2, &g_t1, TaskN1, NULL) == XC_OK, "注册任务 N1(通知者)");
    CHK(XC_Task_Reg(&g_os2, &g_t2, TaskN2, NULL) == XC_OK, "注册任务 N2(观察者)");
    CHK(XC_Task_Reg(&g_os2, &g_tZ, TaskZ, NULL) == XC_OK, "注册任务 Z(等待通知)");
    RunScheduler(&g_os2, 9);
    printf("       实测 trace = \"%s\"  (Z 被唤醒 %d 次)\n", g_trace, g_zWakes);
    CHK(g_zWakes >= 2, "Z 被通知唤醒");
    CHK(strcmp(g_trace, "12Z12Z12Z") == 0, "场景2 trace == \"12Z12Z12Z\"(Z 排队尾, 不插队)");

    printf("\n===== A9 用例完成: failures %d =====\n", g_fail);
    return ((g_fail == 0) ? 0 : 1);
}
