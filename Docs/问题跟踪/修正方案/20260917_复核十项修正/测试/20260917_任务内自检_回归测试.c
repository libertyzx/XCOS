/* B5 回归测试: 运行期自检在**任务上下文**误报 TASKNUM
 * ===========================================================================
 * 缺陷(B5): 调度器取出任务后会把它**摘出任何表**(`XC_SCH_TAKE_FIRST` 使节点自环游离),
 *   因此"四张表节点总数 = `TaskNum` - 1"; 而自检原先要求**严格相等**
 *   ⇒ 在任务里调用(`XC_Config.h` 的推荐用法①"每个 API 调用之后调一次")**必然**误报
 *   `XC_CHK_TASKNUM` —— 推荐用法与实现互相矛盾;
 *   修复: 自检允许"节点总数恰好少 1"(其余差值/超上限一律照报)。
 *
 * 用例三段:
 *   Part 1: 任务上下文调用 ⇒ 修复后 `XC_CHK_OK`(修复前 `XC_CHK_TASKNUM`);
 *           同时记录现场(就绪表空 + `TaskNum=1`)证明"游离"前提;
 *   Part 2: 任务上下文 + 注入 `TaskNum++`(计数虚高 1)⇒ 仍须报 `XC_CHK_TASKNUM`
 *           (容忍值**恰为 1**, 没把自检放水);
 *   Part 3: 收工后把 `TaskNum`/状态复原(用例自身不留副作用)。
 *
 * 编译(PowerShell; 行尾续行用反引号):
 *   gcc -std=gnu99 -O1 -Wall -Wextra -DXC_CFG_DEBUG_CHECK=1 -I Code\Inc -o b5.exe `
 *       Code\Src\XC_Diag.c Code\Src\XC_List.c Code\Src\XC_Sch.c Code\Src\XC_Task.c Code\Src\XC_Time.c `
 *       "Docs\问题跟踪\修正方案\20260917_复核十项修正\测试\20260917_任务内自检_回归测试.c"
 */
#include <setjmp.h>
#include <stdio.h>

#include "XCOS.h"
#include "Internal/XC_TaskInternal.h"

#if (XC_CFG_DEBUG_CHECK == 0)
#error "本用例必须在 -DXC_CFG_DEBUG_CHECK=1 档编译(见 Tests/manifest.txt)"
#endif

static int g_fail = 0;

#define CHK(cond, msg)                     \
    do {                                   \
        if(cond) {                         \
            printf("  PASS  %s\n", (msg)); \
        } else {                           \
            printf("  FAIL  %s\n", (msg)); \
            g_fail++;                      \
        }                                  \
    } while(0)

static XC_ChkResult_t g_RcInTask  = (XC_ChkResult_t)99; /* 任务上下文自检结果 */
static XC_ChkResult_t g_RcDrift   = (XC_ChkResult_t)99; /* 任务上下文 + TaskNum 虚高 1 */
static int            g_TaskNum   = -1;                /* 自检时框架记的任务数 */
static int            g_ReadyBusy = -1;                /* 自检时就绪表是否非空 */
static int            g_Ran       = 0;                 /* 任务是否已跑过"检查段" */
static int            g_IdleN     = 0;
static jmp_buf        g_Env;

/**
 * @brief   任务: **在被调度的任务上下文里**做自检(这正是"每个 API 调用之后"的用法)
 * @details
 *  进入时本任务已由调度器摘出表外 ⇒ 四表节点数 = `TaskNum - 1`;
 *  之后阻塞(默认 Tick 为 0, 不会到点) ⇒ 主循环进入空闲, 由空闲回调收工跳出。
 */
static void TaskA(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    if(g_Ran == 0) {
        g_Ran       = 1;
        g_TaskNum   = (int)t->phXCOS->TaskNum;
        g_ReadyBusy = (XC_List_ListValid(&t->phXCOS->ReadyList) != 0) ? 1 : 0;

        g_RcInTask = XC_Diag_CheckInvariants(t->phXCOS); /* Part 1: 修复前 = XC_CHK_TASKNUM */

        t->phXCOS->TaskNum++;                          /* Part 2: 注入"计数虚高 1" */
        g_RcDrift = XC_Diag_CheckInvariants(t->phXCOS);
        t->phXCOS->TaskNum--;                          /* 复原(用例不留副作用) */
    }
    XC_Cor_DelayTick(5U); /* 阻塞(Tick 不前进 ⇒ 不会到点) ⇒ 让主循环进入空闲 */
    goto A_IDLE_FOREVER;
A_IDLE_FOREVER:
    goto A_IDLE_FOREVER;
    XC_Cor_Leave();
}

/** 空闲回调: 任务已跑过检查段 ⇒ 收工跳出调度器(兜底 8 轮) */
static void Idle(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)
{
    (void)phXCOS;
    (void)IdleTick;
    g_IdleN++;
    if((g_Ran != 0) || (g_IdleN > 8)) {
        longjmp(g_Env, 1);
    }
}

int main(void)
{
    static XCOS_t      s_os;
    static XC_TaskCB_t s_tcb;

    if(setjmp(g_Env) == 0) {
        XC_Sch_Init(&s_os);
        XC_Sch_SetIdleCallback(&s_os, Idle);
        (void)XC_Task_Reg(&s_os, &s_tcb, TaskA, NULL);
        XC_Sch_Start(&s_os); /* 不返回: 由 Idle 回调 longjmp 跳出 */
    }

    printf("=== B5: 运行期自检在任务上下文的判定 ===\n");
    printf("  现场: TaskNum=%d ; 就绪表非空=%d(运行中的任务已被摘出表外) ; 空闲回调次数=%d\n",
           g_TaskNum, g_ReadyBusy, g_IdleN);
    printf("  Part1 任务内自检 = %d ; Part2 注入 TaskNum+1 = %d\n", (int)g_RcInTask, (int)g_RcDrift);

    CHK(g_Ran == 1, "任务上下文确实执行了自检(检查段已运行)");
    CHK(g_TaskNum == 1, "机制: 只有 1 个任务(TaskNum=1)");
    CHK(g_ReadyBusy == 0, "机制: 运行中的任务不在就绪表 ⇒ 四表节点总数 = TaskNum - 1");
    CHK(g_RcInTask == XC_CHK_OK,
        "任务上下文自检: 返回 XC_CHK_OK(修复前误报 XC_CHK_TASKNUM=4)");
    CHK(g_RcDrift == XC_CHK_TASKNUM,
        "注入 TaskNum 虚高 1: 仍报 XC_CHK_TASKNUM(容忍值恰为 1, 自检未被放水)");

    printf("\n===== B5 用例完成: failures %d =====\n", g_fail);
    return ((g_fail == 0) ? 0 : 1);
}
