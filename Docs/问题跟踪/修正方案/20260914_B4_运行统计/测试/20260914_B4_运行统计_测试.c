/* B4 验证用例: 每任务运行统计(XC_CFG_TASK_STATS)
 *  - 关档: 断言 TCB 仍为 44 Byte(0 RAM 增量), 且调度器照常运行(guards 未破坏主循环);
 *  - 开档: ① 包裹函数级(RunStatsBegin/End)精度与"只记最长"语义; ② Tick 回绕安全;
 *          ③ **真实调度器端到端**(setjmp/longjmp 跳出死循环); ④ 生命周期(Reset 保留 / Remove 清零 / 重注册清零);
 *          ⑤ TCB 为 52 Byte(+8B/任务);
 * 编译(关/开):
 *   gcc -std=gnu99 -O1 -Wall -Wextra [-DXC_CFG_TASK_STATS=1] -I Code/Inc \
 *       Code/Src/XC_List.c Code/Src/XC_Sch.c Code/Src/XC_Task.c Code/Src/XC_Time.c <本文件> -o b4.exe
 * 说明: 测试直接驱动内核 Tick 源(g_SysTickCount, 默认"中断累加模式"下由定时器 ISR 递增)
 *       ⇒ 无需定时器/中断, 即可精确构造"调度槽耗时"与 Tick 回绕场景;
 */
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "XCOS.h"

/** 测试用 Tick 操作(g_SysTickCount 由 Code/Src/XC_Time.c 定义; XCOS.h 已带 extern 声明) */
#define TICK_INC(n) (g_SysTickCount += (XC_Tick_t)(n))
#define TICK_SET(v) (g_SysTickCount = (XC_Tick_t)(v))

/**
 * TCB 期望尺寸(用于验证"关档 0 RAM 增量 / 开档 +8B/任务"):
 *  - 32 位目标(真实 MCU, armcc/gcc-arm): 关 = 44, 开 = 52;
 *  - 64 位宿主(gcc 在 PC 上跑回归): 指针 8B ⇒ 结构体更大, 关 = 88, 开 = 96;
 */
#if(defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8))
#define TCB_SIZE_OFF (88U)
#else
#define TCB_SIZE_OFF (44U)
#endif
#define TCB_SIZE_ON  (TCB_SIZE_OFF + 8U) /* 统计字段: RunCnt(4B) + MaxRunTick(4B) */

#if(XC_CFG_ERR_HOOK != 0)
/** B1 错误上报钩子空实现(本用例不校验上报; 仅为"多个开关同时打开"时能链接) */
void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)
{
    (void)phTCB;
    (void)ErrCode;
}
#endif

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

static XCOS_t      g_os;
static XC_TaskCB_t g_task;
static jmp_buf     g_env;  /* 用于从调度器死循环中跳出(仅测试用) */
static int         g_disp; /* 任务被调度的次数 */

/**
 * 任务体: 每次调度推进 Tick 后让出;
 *  - 第 1 次调度: Tick +1 ⇒ 该调度槽应记 1;
 *  - 第 2 次调度: Tick +2 ⇒ 该调度槽应记 2(最长仍为 2);
 *  - 第 3 次调度: longjmp 跳出调度器(该次未走"统计终点" ⇒ 不计入, 见下"端到端"断言说明);
 */
static void ProfTask(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
FIRST:
    g_disp++;
    if(g_disp == 1) {
        TICK_INC(1U);
        XC_Cor_Yield();
        goto FIRST;
    }
    if(g_disp == 2) {
        TICK_INC(2U);
        XC_Cor_Yield();
        goto FIRST;
    }
    longjmp(g_env, 1); /* 第 3 次: 跳出 for(;;) 调度循环(仅测试用) */
    XC_Cor_Leave();
}

/** 跑一次调度器(靠任务内 longjmp 返回) */
static void RunSchedulerUntilLongjmp(void)
{
    g_disp = 0;
    if(setjmp(g_env) == 0) {
        XC_Sch_Start(&g_os); /* 死循环: 由任务内 longjmp 返回 */
    }
}

int main(void)
{
    printf("== B4 用例: XC_CFG_TASK_STATS = %u ==\n", (unsigned)XC_CFG_TASK_STATS);
    (void)memset(&g_os, 0, sizeof(g_os));
    (void)memset(&g_task, 0, sizeof(g_task));
    g_SysTickCount = 0U;
    XC_Sch_Init(&g_os);
    CHK(XC_Task_Reg(&g_os, &g_task, ProfTask, NULL) == XC_OK, "注册任务");

#if(XC_CFG_TASK_STATS == 0)
    /** ---------- 关闭档 ---------- */
    printf("  [info] sizeof(XC_TaskCB_t) = %u Byte(期望 %u)\n",
           (unsigned)sizeof(XC_TaskCB_t), (unsigned)TCB_SIZE_OFF);
    CHK(sizeof(XC_TaskCB_t) == TCB_SIZE_OFF, "关档: TCB 未变(0 RAM 增量)");
    RunSchedulerUntilLongjmp();
    CHK(g_disp == 3, "关档: 调度器照常运行(任务被调度 3 次)");
    printf("  [info] 关档: 统计字段/统计代码均未编译(0 代码/0 RAM)\n");
#else
    /** ---------- 开启档 ---------- */
    printf("  [info] sizeof(XC_TaskCB_t) = %u Byte(期望 %u = 关档 +8)\n",
           (unsigned)sizeof(XC_TaskCB_t), (unsigned)TCB_SIZE_ON);
    CHK(sizeof(XC_TaskCB_t) == TCB_SIZE_ON, "开档: TCB 增加 8 Byte(RunCnt + MaxRunTick)");

    /* 0) 新注册的任务统计从 0 开始 */
    CHK((XC_Diag_GetRunCnt(&g_task) == 0U) && (XC_Diag_GetMaxRunTick(&g_task) == 0U),
        "注册后统计为 0");

    /* 1) 包裹函数级: 精度与"只记最长"语义 */
    {
        XC_Tick_t st;

        st = XC_Diag_RunStatsBegin();
        TICK_INC(5U);
        XC_Diag_RunStatsEnd(&g_task, st);
        CHK((XC_Diag_GetRunCnt(&g_task) == 1U) && (XC_Diag_GetMaxRunTick(&g_task) == 5U),
            "包裹级: 第 1 次(+5) ⇒ RunCnt=1, Max=5");

        st = XC_Diag_RunStatsBegin();
        TICK_INC(3U);
        XC_Diag_RunStatsEnd(&g_task, st);
        CHK((XC_Diag_GetRunCnt(&g_task) == 2U) && (XC_Diag_GetMaxRunTick(&g_task) == 5U),
            "包裹级: 第 2 次(+3) ⇒ RunCnt=2, Max 仍为 5(只记最长)");

        st = XC_Diag_RunStatsBegin();
        TICK_INC(9U);
        XC_Diag_RunStatsEnd(&g_task, st);
        CHK((XC_Diag_GetRunCnt(&g_task) == 3U) && (XC_Diag_GetMaxRunTick(&g_task) == 9U),
            "包裹级: 第 3 次(+9) ⇒ RunCnt=3, Max=9");
    }

    /* 2) Tick 回绕边界: 起始 0xFFFFFFFE, 结束回绕到 2 ⇒ 无符号差值应为 4 */
    {
        XC_Tick_t st;

        XC_Diag_ClrRunStats(&g_task); /* 先清零, 使本项断言只反映回绕差值 */
        TICK_SET(0xFFFFFFFEU);
        st = XC_Diag_RunStatsBegin();
        TICK_SET(2U); /* 回绕 */
        XC_Diag_RunStatsEnd(&g_task, st);
        CHK(XC_Diag_GetMaxRunTick(&g_task) == 4U, "回绕安全: 差值按无符号计算(=4)");
    }

    /* 3) 清零 API */
    XC_Diag_ClrRunStats(&g_task);
    CHK((XC_Diag_GetRunCnt(&g_task) == 0U) && (XC_Diag_GetMaxRunTick(&g_task) == 0U),
        "XC_Diag_ClrRunStats 清零");

    /* 4) 真实调度器端到端: 前两次调度由主循环统计(第 1 次 +1、第 2 次 +2) */
    TICK_SET(0U);
    RunSchedulerUntilLongjmp();
    CHK(g_disp == 3, "端到端: 任务被调度 3 次(longjmp 跳出)");
    CHK(XC_Diag_GetRunCnt(&g_task) == 2U, "端到端: RunCnt=2(第 3 次跳出未计入, 符合口径)");
    CHK(XC_Diag_GetMaxRunTick(&g_task) == 2U, "端到端: MaxRunTick=2(第 2 次调度耗时最长)");

    /* 5) 生命周期: Reset 保留, Remove 清零 */
    XC_Task_Reset(&g_task);
    CHK(XC_Diag_GetRunCnt(&g_task) == 2U, "Reset 保留统计(排障不丢证据)");
    XC_Task_Remove(&g_task);
    CHK((XC_Diag_GetRunCnt(&g_task) == 0U) && (XC_Diag_GetMaxRunTick(&g_task) == 0U),
        "Remove 清零统计(生命周期结束)");

    /* 6) 重注册后从 0 开始 */
    CHK(XC_Task_Reg(&g_os, &g_task, ProfTask, NULL) == XC_OK, "移除后可再次注册");
    CHK((XC_Diag_GetRunCnt(&g_task) == 0U) && (XC_Diag_GetMaxRunTick(&g_task) == 0U),
        "重注册后统计从 0 开始");
#endif

    printf("\n===== B4 用例完成: failures %d =====\n", g_fail);
    return ((g_fail == 0) ? 0 : 1);
}
