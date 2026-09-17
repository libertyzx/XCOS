/**
 * @file       20260914_A1_新旧代码对照_sim.c
 * @brief      [仿真通道] A1 判据对照测点 —— 同一份代码分别跑"新(待处理标志) / 旧(双计数)"
 * @details
 *  只使用两版都存在的公开 API(不访问 TCB/XCOS_t 字段) => 同一份测点可分别链接旧/新 Code/。
 *  ---
 *  **判据(经实测修正)**: 旧实现的"回绕失效"需要同时满足
 *    (a) 消费者侧计数为 0(即上一次同步时生产者恰为 0; 新注册的 TCB 天然满足 P=C=0)
 *    (b) 自那以后未被消费的发送次数恰为 256 的整数倍(>=256)
 *  满足时 Produced 回绕成 0 == Consumed => 记录被抹 => WaitNotify 误阻塞(丢唤醒);
 *  因此本测点用**各自独立的 TCB**(避免 Reset 把 Consumed 带成非 0)分别跑 300/256/512 次。
 *  ---
 *  g_res[]: [0] 检查数 [1] 失败数 [2] 300 次未阻塞(对照组) [3] 256 次未阻塞(判据)
 *            [4] 512 次未阻塞(判据) [5] ClrNotify 后阻塞(对照组)
 */
#include <stddef.h>
#include <string.h>
#include "XCOS.h"
#include "XC_Cor.h"
#include "XC_Sch.h"
#include "XC_Task.h"
#include "Internal/XC_Core.h"
#include "Internal/XC_List.h"

XCOS_t            g_xc;
volatile uint32_t g_res[16];

static XC_TaskCB_t s_tcb300; /* 各用独立 TCB: 注册时 P=C=0 => 满足判据(a) */
static XC_TaskCB_t s_tcb256;
static XC_TaskCB_t s_tcb512;
static XC_TaskCB_t s_tcbClr;

void SimDone(void)
{
}

static void Dummy(XC_TaskHandle_t t)
{
    (void)t;
}

/* 发送 n 次后等待: 返回 1=未阻塞(正确) / 0=阻塞(丢唤醒) */
static uint32_t CaseSends(XC_TaskHandle_t pTCB, uint32_t n)
{
    uint32_t i;

    XC_Task_HandleDelay(pTCB, 1000U); /* 非等待 => 发送走"只记录"路径 */
    for(i = 0U; i < n; i++) {
        (void)XC_Task_SendNotify(pTCB, NULL);
    }
    XC_Task_HandleWaitNotify(pTCB, 0U);
    return (XC_Task_GetState(pTCB) == (uint8_t)XC_TASK_READY) ? 1U : 0U;
}

int main(void)
{
    memset(&g_xc, 0, sizeof(g_xc));
    XC_Sch_Init(&g_xc);

    (void)XC_Task_Reg(&g_xc, &s_tcb300, Dummy, NULL);
    (void)XC_Task_Reg(&g_xc, &s_tcb256, Dummy, NULL);
    (void)XC_Task_Reg(&g_xc, &s_tcb512, Dummy, NULL);
    (void)XC_Task_Reg(&g_xc, &s_tcbClr, Dummy, NULL);

    g_res[0]++;
    g_res[2] = CaseSends(&s_tcb300, 300U); /* 对照组: 300 mod 256 = 44 => 两版都应通过 */
    if(g_res[2] == 0U) { g_res[1]++; }

    g_res[0]++;
    g_res[3] = CaseSends(&s_tcb256, 256U); /* 判据: 旧实现应阻塞 */
    if(g_res[3] == 0U) { g_res[1]++; }

    g_res[0]++;
    g_res[4] = CaseSends(&s_tcb512, 512U); /* 判据: 旧实现应阻塞 */
    if(g_res[4] == 0U) { g_res[1]++; }

    /* 对照组: ClrNotify 后必须阻塞(两版一致) */
    XC_Task_HandleDelay(&s_tcbClr, 1000U);
    (void)XC_Task_SendNotify(&s_tcbClr, NULL);
    XC_Task_ClrNotify(&s_tcbClr);
    XC_Task_HandleWaitNotify(&s_tcbClr, 0U);
    g_res[0]++;
    g_res[5] = (XC_Task_GetState(&s_tcbClr) == (uint8_t)XC_TASK_BLOCKED) ? 1U : 0U;
    if(g_res[5] == 0U) { g_res[1]++; }

    SimDone();
    while(1) {
    }
    return 0;
}