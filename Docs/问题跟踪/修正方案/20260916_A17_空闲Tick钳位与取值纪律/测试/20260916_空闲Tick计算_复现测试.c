/* A17 复现测试: 空闲分支取值缺陷(两条隐患) —— **同一用例含两段**
 * ===========================================================================
 * Part 1(**下溢**): 主循环空闲分支把"下一个任务唤醒时刻 − 当前 Tick"直接做**无符号减法**;
 *       一旦"该唤醒时刻已经过去"(窗口内 Tick 前进了), 结果为负 ⇒ 按 uint32 回绕成
 *       4294967295 ⇒ 交给 fIdle; 而真实含义是"**别休眠, 立刻回主循环干活**"。
 * Part 2(**陈旧快照**): 空闲分支的"判空"与"取首节点"原本是**两次读表根**(且在**锁外**); 若这期间中断把
 *       节点搬去就绪表(通知直接唤醒路径), 取首节点时就会把**表根当 TCB** 读(Garbage Tick);
 *       且"任务其实已就绪"却给了可长睡的 `~0U` 提示。
 *       修复: 该段取值**并入锁域** —— "就绪表是否空"与"最近唤醒时刻"读成一次原子观察。
 * 手段: 把 Tick 源切到"方式2: 直接计数器模式"(-DXC_SYS_TICK_COUNT=XC_TestTickRead()),
 *       于是框架各编译单元的每次"读 Tick"都走本用例的函数 ⇒ 可在**一次迭代内**、
 *       精确在"空闲分支那次读"上注入:
 *         - Part 1: 让 Tick +2(等价"TimeSched 读完 Tick 后, 主循环又忙了 2 个 tick");
 *         - Part 2: 本函数此刻**充当"中断"** —— 把时间表首节点搬到就绪表(并清 `Lock`: 空闲分支未持锁 ⇒ ISR 走"直接搬表")。
 * 判定: 修复后两段的"注入轮"都必须为 **0**(= 不要休眠); 修复前分别是 `0xFFFFFFFF` 与 `~0U`。
 * 预期: **修复前本用例 FAIL**, 修复后 PASS ⇒ 转为回归用例(已入 `Tests/manifest.txt`: `A17-空闲Tick`)。
 * ---------------------------------------------------------------------------
 * 编译(PowerShell; 行尾续行用反引号):
 *   gcc -std=gnu99 -O1 -Wall -Wextra `
 *       -include "Docs\问题跟踪\修正方案\20260916_A17_空闲Tick钳位与取值纪律\测试\xc_test_tick.h" `
 *       -DXC_SYS_TICK_COUNT=XC_TestTickRead() -I Code\Inc -o a17.exe `
 *       Code\Src\XC_Diag.c Code\Src\XC_List.c Code\Src\XC_Sch.c Code\Src\XC_Task.c Code\Src\XC_Time.c `
 *       "Docs\问题跟踪\修正方案\20260916_A17_空闲Tick钳位与取值纪律\测试\20260916_空闲Tick计算_复现测试.c"
 */
#include <setjmp.h>
#include <stdio.h>

#include "XCOS.h"
#include "Internal/XC_TaskInternal.h" /* [Part2] XC_Task_MoveToReadyListTail(用例充当"中断"用) */

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

/* ================= 脚本化 Tick 源(方式2: 计数器模式) ================= */

#define A17_PART1 1U /* 注入: 窗口内 Tick 前进 2(下溢) */
#define A17_PART2 2U /* 注入: 窗口内"中断"把时间表首节点搬到就绪表(陈旧快照) */

static XC_Tick_t g_Now      = 0U; /* 当前 Tick(由注入推进; 等价于定时器中断累加) */
static unsigned  g_ReadN    = 0U; /* 本轮迭代已读 Tick 次数(空闲回调里复位) */
static int       g_TaskRan  = 0;  /* 本轮迭代任务是否运行过(任务内部会读 1 次 Tick) */
static int       g_Inject   = 0;  /* 1 = 允许注入 */
static unsigned  g_Part     = 0U; /* A17_PART1 / A17_PART2 */
static XCOS_t*   g_Os       = NULL; /* 本轮被注入的框架实例 */
static int       g_IsrMoved = 0;  /* [Part2] "中断"是否搬走了表首节点 */
static XC_Tick_t g_ValNew   = 0U; /* [Part2] 该任务(节点)的真实唤醒时刻 */
static XC_Tick_t g_ValOld   = 0U; /* [Part2] 旧写法取到的值(表已空后再读一次表根) */
static XC_Tick_t g_IlObs[4];      /* fIdle 收到的值(采样) */
static int       g_IlN = 0;
static jmp_buf   g_Env;

/**
 * @brief   [测试]读当前 Tick + 在**空闲分支那次读**上注入
 * @details
 *  注入点 = 本轮"**空闲分支**"的那次读, 读序如下:
 *  - 任务运行过的迭代: ①任务(HandleBlocking 读) ②TimeSched 读 ③空闲分支读;
 *  - 任务没运行的迭代: ①TimeSched 读 ②空闲分支读;
 *  ⇒ 命中"空闲分支读"时按 `g_Part` 注入:
 *    - Part1: Tick +2(等价"这轮干活期间, 定时器中断又走了 2 个 tick");
 *    - Part2: 本函数**充当"中断"**: 把时间表首节点搬到就绪表(等价 `SendNotify` 的直接唤醒路径),
 *             并顺手记录"新/旧两种写法"此刻分别能取到什么值(机理对照)。
 */
XC_Tick_t XC_TestTickRead(void)
{
    XC_Tick_t t      = g_Now;
    unsigned  target = (g_TaskRan != 0) ? 3U : 2U; /* 本轮"空闲分支读"的序号 */

    g_ReadN++;
    if((g_Inject == 0) || (g_ReadN != target)) {
        return (t);
    }

    if(g_Part == A17_PART1) {
        g_Now = (XC_Tick_t)(g_Now + 2U);
        t     = g_Now;
    }
    else { /* A17_PART2: 此刻充当"中断" */
        XC_ListNode_t* pHead = XC_List_GetListStartNode(&g_Os->TimeList); /* 表首节点(此刻还在表里) */
        if(pHead != &g_Os->TimeList) {
            XC_TaskHandle_t phTCB = XC_LIST_TO_TCB(pHead);

            g_ValNew = phTCB->TaskWakeupTick; /* 任务真实唤醒时刻(对照: 旧写法把表根当 TCB ⇒ 读到的不是这个值) */
            g_Os->Lock = 0U;                  /* 空闲分支未持锁 ⇒ ISR 读到 0 会走"直接搬表"分支(最坏情形) */
            XC_Task_MoveToReadyListTail(phTCB); /* 摘出时间表 → 插就绪表尾(中断直接唤醒) */
            phTCB->TaskState = XC_TASK_READY;
            g_IsrMoved = 1;
            /* 旧写法: "判空"之后再读一次表根(此刻表已空) ⇒ 读到的是表根字节
             * (&TimeList + offsetof(TaskWakeupTick) = &XCOS_t + 28 = BlockedList.pPrev, 不是任何任务的唤醒时刻) */
            g_ValOld = XC_LIST_TO_TCB(&g_Os->TimeList)->TaskWakeupTick;
        }
    }
    return (t);
}

/* ================= 任务 / 空闲回调 ================= */

static XCOS_t      s_os1;
static XC_TaskCB_t s_tcb1;
static XCOS_t      s_os2;
static XC_TaskCB_t s_tcb2;

/** 单任务: 每次运行延时 1 tick(⇒ 时间表首节点唤醒时刻 = 当前 Tick + 1) */
static void TaskA(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
A_LOOP:
    g_TaskRan = 1;
    XC_Cor_DelayTick(1U);
    goto A_LOOP;
    XC_Cor_Leave();
}

/** 空闲回调: 记录框架给的提示值; 第 1 次采样后开启注入; 采到"注入轮"后跳出调度器 */
static void Idle(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)
{
    (void)phXCOS;
    if(g_IlN < 4) {
        g_IlObs[g_IlN] = IdleTick;
    }
    g_IlN++;

    g_ReadN   = 0U; /* 迭代边界: 复位本轮读计数 */
    g_TaskRan = 0;

    if(g_IlN < 2) {
        g_Inject = 1; /* 下一轮开始注入(本轮采样 = 正常值) */
        return;
    }
    /* [Part2] 若还没轮到"表首节点在时间表里"的那一轮, 继续跑(最多 8 轮, 防挂死) */
    if((g_Part == A17_PART2) && (g_IsrMoved == 0) && (g_IlN < 8)) {
        return;
    }
    longjmp(g_Env, 1); /* 已采到"注入轮"的值, 跳出调度器 */
}

/** 重置用例状态(两段之间调用) */
static void ResetState(void)
{
    g_Now      = 0U;
    g_ReadN    = 0U;
    g_TaskRan  = 0;
    g_Inject   = 0;
    g_IsrMoved = 0;
    g_ValNew   = 0U;
    g_ValOld   = 0U;
    g_IlN      = 0;
    g_IlObs[0] = g_IlObs[1] = 0U;
}

int main(void)
{
    /* ---------- Part 1: 窗口内 Tick 前进 2 ⇒ 下溢 ---------- */
    ResetState();
    g_Part = A17_PART1;
    g_Os   = &s_os1;
    if(setjmp(g_Env) == 0) {
        XC_Sch_Init(&s_os1);
        XC_Sch_SetIdleCallback(&s_os1, Idle);
        (void)XC_Task_Reg(&s_os1, &s_tcb1, TaskA, NULL);
        XC_Sch_Start(&s_os1); /* 不返回: 由 Idle 回调 longjmp 跳出 */
    }

    printf("=== A17 Part 1: 空闲 Tick 下溢(窗口内 Tick 前进) ===\n");
    printf("  采样1(正常轮) IdleTick=%u (期望 1)\n", (unsigned)g_IlObs[0]);
    printf("  采样2(注入轮) IdleTick=%u (0x%08X; 期望 0)\n", (unsigned)g_IlObs[1],
           (unsigned)g_IlObs[1]);
    CHK(g_IlN >= 2, "Part1: 采到 2 次空闲回调(正常轮 + 注入轮)");
    CHK(g_IlObs[0] == 1U, "Part1 正常轮: 空闲提示 = 1 tick(时间表首节点还有 1 tick 到期)");
    CHK(g_IlObs[1] == 0U, "Part1 注入轮: 已过点 ⇒ 空闲提示 = 0(修复前 0xFFFFFFFF)");

    /* ---------- Part 2: 窗口内"中断"把时间表首节点搬到就绪表 ⇒ 陈旧快照 ---------- */
    ResetState();
    g_Part = A17_PART2;
    g_Os   = &s_os2;
    if(setjmp(g_Env) == 0) {
        XC_Sch_Init(&s_os2);
        XC_Sch_SetIdleCallback(&s_os2, Idle);
        (void)XC_Task_Reg(&s_os2, &s_tcb2, TaskA, NULL);
        XC_Sch_Start(&s_os2);
    }

    printf("=== A17 Part 2: 窗口内\"中断\"搬走时间表首节点(陈旧快照) ===\n");
    printf("  采样1(正常轮) IdleTick=%u (期望 1)\n", (unsigned)g_IlObs[0]);
    printf("  采样2(注入轮) IdleTick=%u (0x%08X; 期望 0)\n", (unsigned)g_IlObs[1],
           (unsigned)g_IlObs[1]);
    printf("  机理对照: 任务真实唤醒时刻(节点上)=%u ; 旧写法(表空后读表根)取到=%u (0x%08X)\n",
           (unsigned)g_ValNew, (unsigned)g_ValOld, (unsigned)g_ValOld);
    CHK(g_IsrMoved == 1, "Part2: \"中断\"已把时间表首节点搬到就绪表");
    CHK(g_ValNew == 1U, "Part2 机理: 节点上读到的是任务真实唤醒时刻");
    CHK(g_ValOld != g_ValNew,
        "Part2 机理: 旧写法(表空后再读表根)取到的**不是**该任务的唤醒时刻 ⇒ 会得到垃圾 Tick");
    CHK(g_IlObs[1] == 0U, "Part2 注入轮: 任务已就绪 ⇒ 空闲提示 = 0(修复前给 ~0U 长睡)");

    printf("\n===== A17 用例完成: failures %d =====\n", g_fail);
    return ((g_fail == 0) ? 0 : 1);
}
