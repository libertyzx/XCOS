/**
 * @file  xc_yield_bench.c
 * @brief [周期通道·补充测点] "让出路径"往返周期: 自环路径(Yield) 与 非自环路径(Delay)
 * @details
 *  用途: 量化"调度器归位判定"这一热路径的成本(每次任务调用/让出都会走到);
 *        A/B = 纯 Yield 任务(归位走"自环"路径); C/D = DelayMs(1) 任务(归位走"非自环"路径);
 *  口径: 各自的"上次被调度 -> 本次被调度"平均周期(含 CycGet 与调度器固定开销) => 仅用于同口径对比;
 *  时间源: 每读一次 Tick 前进 1(使 Delay 能到点, 确定性; 不依赖中断/外设);
 *  驱动: `Tests/run_yield.ps1`（内部调用 `run_cycles.ps1 -BenchFile/-IniFile/-ConfigTickSource TestTick`）;
 *        结果键: `yield_roundtrip` / `delay_roundtrip`（写入 `Tests/baseline.json` 的 `cycles` 段）。
 */
#include "main.h" /* XCOS.h + CMSIS */
#include <stdint.h>

#define CYC_LOAD 0x00FFFFFFU
#define N_RT     1000U

volatile uint32_t g_res[16];
static XCOS_t      g_os;
static XC_TaskCB_t g_tcbA;
static XC_TaskCB_t g_tcbB;
static XC_TaskCB_t g_tcbC;
static XC_TaskCB_t g_tcbD;

void SimDone(void);      /* [ini 断点符号] */
XC_Tick_t TestTick(void); /* [脚本注入] XC_SYS_TICK_COUNT -> TestTick() */

static void CycInit(void)
{
    SysTick->LOAD = CYC_LOAD;
    SysTick->VAL  = 0U;
    SysTick->CTRL = 5U;
}

static uint32_t CycGet(void)
{
    return (CYC_LOAD - SysTick->VAL);
}

static volatile XC_Tick_t g_Tick = 1000U;

XC_Tick_t TestTick(void)
{
    g_Tick++; /* 每次读都前进 1 => DelayMs(1) 下一轮即到点 */
    return g_Tick;
}

static uint32_t g_SumY;
static uint32_t g_LastY;
static uint32_t g_CntY;
static uint32_t g_SumD;
static uint32_t g_LastD;
static uint32_t g_CntD;
static volatile uint32_t g_DoneD;

static void TaskA(XC_TaskHandle_t phTCB) /* 纯 Yield: 归位走"自环"路径 */
{
    uint32_t now;

    XC_Cor_Enter(phTCB);
    g_LastY = CycGet();
    g_SumY  = 0U;
    g_CntY  = 0U;
    for(;;) {
        now = CycGet();
        if(g_CntY != 0U) {
            g_SumY += (now - g_LastY);
        }
        g_LastY = now;
        g_CntY++;
        if(g_CntY > N_RT) {
            g_res[0] = g_SumY / (g_CntY - 1U);
            break;
        }
        XC_Cor_Yield();
    }
    while(g_DoneD == 0U) { /* 等 D 侧测完再收尾 */
        XC_Cor_Yield();
    }
    SimDone();
    for(;;) {
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

static void TaskC(XC_TaskHandle_t phTCB) /* Delay 1ms(=1 tick): 归位走"非自环"路径 */
{
    uint32_t now;

    XC_Cor_Enter(phTCB);
    g_LastD = CycGet();
    g_SumD  = 0U;
    g_CntD  = 0U;
    for(;;) {
        now = CycGet();
        if(g_CntD != 0U) {
            g_SumD += (now - g_LastD);
        }
        g_LastD = now;
        g_CntD++;
        if(g_CntD > N_RT) {
            g_res[2] = g_SumD / (g_CntD - 1U);
            g_DoneD  = 1U;
            break;
        }
        XC_Cor_DelayMs(1U);
    }
    for(;;) {
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

static void TaskB(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    for(;;) {
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

static void TaskD(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    for(;;) {
        XC_Cor_DelayMs(1U);
    }
    XC_Cor_Leave();
}

void SimDone(void)
{
    g_res[15] = g_res[15] + 0U;
}

int main(void)
{
    uint32_t t0;
    uint32_t t1;

    CycInit();
    t0 = CycGet();
    t1 = CycGet();
    g_res[1]  = t1 - t0; /* 计时自身开销(参考) */
    g_res[15] = N_RT;
    g_res[13] = (uint32_t)sizeof(XC_TaskCB_t);
    g_res[14] = (uint32_t)sizeof(XCOS_t);

    XC_Sch_Init(&g_os);
    (void)XC_Task_Reg(&g_os, &g_tcbA, TaskA, NULL);
    (void)XC_Task_Reg(&g_os, &g_tcbB, TaskB, NULL);
    (void)XC_Task_Reg(&g_os, &g_tcbC, TaskC, NULL);
    (void)XC_Task_Reg(&g_os, &g_tcbD, TaskD, NULL);
    XC_Sch_Start(&g_os); /* 阻塞,不返回 */

    for(;;) { }
}
