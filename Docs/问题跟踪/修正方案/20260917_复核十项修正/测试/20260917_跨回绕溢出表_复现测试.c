/* A19 复现测试: "跨回绕延时"可能**永不唤醒**(溢出表整表搬移被跳过)
 * ===========================================================================
 * 缺陷(A19): 时间调度只在"检测到 Tick 回绕"或"时间表有节点到点"时才更新 `PrevTick`;
 *   若启动后**从未有过定时到点**(纯事件驱动的应用), 期间却有一笔**跨回绕**的延时进了溢出表,
 *   那么回绕真正发生时 `PrevTick` 还是初值(0) ⇒ 回绕判定 `PrevTick > Tick` 不成立
 *   ⇒ **溢出表整表搬入时间表被跳过** ⇒ 该任务再也醒不过来
 *   (最长要等下一个完整回绕周期, 若期间仍无到点则永久);
 *   修复: 时间调度**每轮**都把"当前 Tick"记为观察值(`PrevTick = Tick`, 置于函数末尾)。
 *
 * 手段: 把 Tick 源切到"方式2: 直接计数器模式"(-DXC_SYS_TICK_COUNT=XC_TestTickRead()),
 *   用例脚本化推动时间轴, 分两个场景(覆盖回绕判定的**两条**下游路径):
 *     Part 1(溢出来源): 时间表为空、任务延时**跨回绕** ⇒ 节点进**溢出表**;
 *       期望: 回绕被发现 ⇒ 溢出表整表搬回时间表 ⇒ 任务唤醒;
 *     Part 2(时间表来源): 延时**不跨回绕**(唤醒时刻在回绕之后才到期) ⇒ 节点在**时间表**;
 *       期望: 回绕被发现 ⇒ 时间表整表搬就绪表(与唤醒时刻无关) ⇒ 任务唤醒 —— 这条路正是
 *       "每轮更新 PrevTick 后, `PrevTick > Tick` 是否还能进入"的直接验证(用户关切点);
 *     两段都在第 1 次空闲回调里把时间推到"回绕之后";
 * 判定:
 *   - `PrevTick` 在第 1 次空闲前应已等于"当时的 Tick"(修复前恒为初值 0) —— 机理断言;
 *   - 推进时间后任务必须被唤醒(修复前回绕未被发现 ⇒ 节点滞留 ⇒ 永不唤醒), 由"最多 8 轮空闲"兜底跳出并判 FAIL;
 * 预期: **修复前两段都 FAIL**, 修复后 PASS ⇒ 转为回归用例(已入 `Tests/manifest.txt`: `A19-跨回绕溢出表`)。
 * ---------------------------------------------------------------------------
 * 注: 脚本化 Tick 头文件复用 A17 用例的 `xc_test_tick.h`(声明 `XC_TestTickRead`);
 *     本用例的读函数**不做读计数注入**, 只是把固定值返回(时间由空闲回调推进)。
 * ---------------------------------------------------------------------------
 * 编译(PowerShell; 行尾续行用反引号):
 *   gcc -std=gnu99 -O1 -Wall -Wextra `
 *       -include "Docs\问题跟踪\修正方案\20260916_A17_空闲Tick钳位与取值纪律\测试\xc_test_tick.h" `
 *       -DXC_SYS_TICK_COUNT=XC_TestTickRead() -I Code\Inc -o a19.exe `
 *       Code\Src\XC_Diag.c Code\Src\XC_List.c Code\Src\XC_Sch.c Code\Src\XC_Task.c Code\Src\XC_Time.c `
 *       "Docs\问题跟踪\修正方案\20260917_复核十项修正\测试\20260917_跨回绕溢出表_复现测试.c"
 */
#include <setjmp.h>
#include <stdio.h>

#include "XCOS.h"
#include "Internal/XC_List.h"
#include "Internal/XC_TaskInternal.h"

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

/** 场景二选一: Part1 延时可跨回绕(⇒ 溢出表); Part2 不大于回绕距离(⇒ 时间表, 回绕时整表搬就绪) */
#define A19_PART1 (1U)
#define A19_PART2 (2U)

static unsigned  g_Part      = A19_PART1;    /* 当前场景 */
static XC_Tick_t g_TickStart = 0xFFFFFF00U;  /* 起始 Tick(回绕前 256 tick) */
static XC_Tick_t g_Delay     = 0x200U;       /* 延时: Part1 = 512(跨回绕) / Part2 = 2048(不跨回绕) */
static XC_Tick_t g_TickNext  = 0x00000200U;  /* 推进到时间(已过回绕, 也已过唤醒时刻) */
static int       g_ExpectOvf = 1;            /* 第 1 轮节点应在哪张表: 1=溢出表 / 0=时间表 */

static XC_Tick_t g_Now       = 0U;             /* 当前时间(由空闲回调推进) */
static int       g_Run       = 0;              /* 任务入口执行次数 */
static int       g_IdleN     = 0;              /* 空闲回调次数 */
static int       g_Woken     = 0;              /* 任务是否在"回绕之后"被唤醒 */
static XC_Tick_t g_Hint[3];                    /* 空闲提示值采样(0/1/2 次) */
static XC_Tick_t g_PrevTick1 = 0U;             /* 第 1 次空闲前的 PrevTick(观察值) */
static int       g_Overflow1 = 0;              /* 第 1 次空闲前: 溢出表非空 */
static int       g_TimeList1 = 0;              /* 第 1 次空闲前: 时间表非空 */
static jmp_buf   g_Env;

/** 脚本化 Tick 源(方式2: 直接计数器模式; 见 xc_test_tick.h) */
XC_Tick_t XC_TestTickRead(void)
{
    return (g_Now);
}

/** 单任务: 每轮延时 512 tick(第 1 次必然跨回绕 ⇒ 入溢出表) */
static void TaskA(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
A_LOOP:
    g_Run++;
    XC_Cor_DelayTick(g_Delay);
    goto A_LOOP;
    XC_Cor_Leave();
}

/**
 * @brief   空闲回调: 第 1 轮采样现场并把时间推过回绕; 之后等"任务被唤醒"后收工
 * @details
 *  - 第 1 轮: 任务已阻塞(溢出表), 记录 `PrevTick` / 溢出表状态 / 提示值, 再推进时间;
 *  - 之后: 若任务被唤醒并再次运行(`g_Run >= 2`) ⇒ 收工;
 *  - 兜底: 8 轮还没唤醒(修复前的表现) ⇒ 跳出并判 FAIL(不让用例挂死)。
 */
static void Idle(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)
{
    g_IdleN++;
    if(g_IdleN <= 3) {
        g_Hint[g_IdleN - 1] = IdleTick;
    }

    if(g_IdleN == 1) {
        g_PrevTick1 = phXCOS->PrevTick;
        g_Overflow1 = (XC_List_ListValid(&phXCOS->TimeOverflowList) != 0) ? 1 : 0;
        g_TimeList1 = (XC_List_ListValid(&phXCOS->TimeList) != 0) ? 1 : 0;
        g_Now       = g_TickNext; /* 时间推进: 已过回绕, 也已过该任务的唤醒时刻 */
        return;
    }

    if(g_Run >= 2) {
        g_Woken = 1;
        longjmp(g_Env, 1); /* 已唤醒并再次运行 ⇒ 收工 */
    }
    if(g_IdleN > 8) {
        longjmp(g_Env, 1); /* 兜底: 永不唤醒(修复前的表现) */
    }
}

/** 跑一个场景: 起始 Tick / 延时 / 推进目标 / 期望"第 1 轮"节点所在表(1=溢出表, 0=时间表) */
static void RunPart(unsigned part, XC_Tick_t tickStart, XC_Tick_t delay, XC_Tick_t tickNext, int expectOvf)
{
    static XCOS_t      s_os[2];    /* 每段一个框架实例(避免上一段的注册状态影响下一段) */
    static XC_TaskCB_t s_tcb[2];
    unsigned           k = (part == A19_PART1) ? 0U : 1U;

    g_Part = part; g_TickStart = tickStart; g_Delay = delay; g_TickNext = tickNext; g_ExpectOvf = expectOvf;
    g_Now = tickStart; g_Run = 0; g_IdleN = 0; g_Woken = 0; g_PrevTick1 = 0U;
    g_Overflow1 = 0; g_TimeList1 = 0;
    g_Hint[0] = g_Hint[1] = g_Hint[2] = 0U;

    if(setjmp(g_Env) == 0) {
        XC_Sch_Init(&s_os[k]);
        XC_Sch_SetIdleCallback(&s_os[k], Idle);
        (void)XC_Task_Reg(&s_os[k], &s_tcb[k], TaskA, NULL);
        XC_Sch_Start(&s_os[k]); /* 不返回: 由 Idle 回调 longjmp 跳出 */
    }

    if(g_Part == A19_PART1) {
        printf("=== A19 Part 1: 延时跨回绕 ⇒ 溢出表(回绕时整表搬回时间表) ===\n");
    }
    else {
        printf("=== A19 Part 2: 延时未跨回绕 ⇒ 时间表(回绕时整表搬就绪表) ===\n");
    }
    printf("  时间轴: 起始 0x%08X ; 延时 %u ⇒ 唤醒时刻 0x%08X ; 推进到 0x%08X\n",
           (unsigned)g_TickStart, (unsigned)g_Delay, (unsigned)(g_TickStart + g_Delay), (unsigned)g_TickNext);
    printf("  空闲回调=%d ; 任务入口执行=%d\n", g_IdleN, g_Run);
    printf("  第1次空闲: PrevTick=0x%08X(期望 0x%08X) 溢出表=%d 时间表=%d 提示值=%u ; 回绕后首轮提示值=%u(0x%08X)\n",
           (unsigned)g_PrevTick1, (unsigned)g_TickStart, g_Overflow1, g_TimeList1,
           (unsigned)g_Hint[0], (unsigned)g_Hint[1], (unsigned)g_Hint[1]);

    CHK(g_Overflow1 == g_ExpectOvf, "第1轮机制: 节点落在期望的表(溢出表=1 / 时间表=0)");
    CHK(g_TimeList1 == (1 - g_ExpectOvf), "第1轮机制: 另一张表为空");
    CHK(g_Hint[0] == g_Delay, "第1轮机制: 空闲提示值 = 距唤醒还剩的 tick 数(未回绕口径正确)");
    CHK(g_PrevTick1 == g_TickStart,
        "机理: 时间调度**每轮**都更新观察值 PrevTick(修复前恒为初值 0 ⇒ 回绕判定失效)");
    CHK(g_Hint[1] == g_Delay,
        "回绕后: 提示值正常(修复前回绕未被发现 ⇒ 节点滞留 ⇒ 提示值巨大 / 任务不运行)");
    CHK(g_Woken == 1, "回绕被发现 ⇒ 节点被搬到就绪表 ⇒ 任务被唤醒(修复前永不唤醒)");
}

int main(void)
{
    /* Part 1: 延时跨回绕 ⇒ 溢出表; 回绕时"溢出表整表搬回时间表" */
    RunPart(A19_PART1, 0xFFFFFF00U, 0x200U, 0x00000200U, 1);
    /* Part 2: 延时未跨回绕 ⇒ 时间表; 回绕时"时间表整表搬就绪表" —— 同一判定 `PrevTick > Tick` 的另一条路 */
    RunPart(A19_PART2, 0xFFFFF000U, 0x800U, 0x00000800U, 0);

    printf("\n===== A19 用例完成: failures %d =====\n", g_fail);
    return ((g_fail == 0) ? 0 : 1);
}
