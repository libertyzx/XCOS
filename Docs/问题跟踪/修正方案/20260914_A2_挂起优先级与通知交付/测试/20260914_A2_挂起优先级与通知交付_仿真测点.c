/**
 * @file 20260914_A2_挂起优先级与通知交付_sim.c
 * @brief [仿真通道] A2 语义 + 周期 对照测点 (AC5 -O1 / SARMCM3 仿真)
 * @details 同一份测点分别链接 before(原版) / after(A2 补丁) 两版 Code/, 结果写 g_res[] + SimDone()。
 *  只使用两版都存在的 API 与 TCB 公开字段 => 两版 bench 等长(体积可比)。
 *  g_res[]:
 *   [0]检查数 [1]失败数
 *   [2]T1状态(exp4) [3]T1目标运行次数(exp0)
 *   [4]T2 Resume返回(exp0) [5]T2恢复后超时标志(exp0) [6]T2收到挂起前通知(exp1) [7]T2恢复后运行次数(exp1)
 *   [8]T3挂起期SendNotify返回(exp1) [9]T3挂起期数据未被覆盖(exp1)
 *   [10]T5无通知挂起后状态(exp4) [11]T5无通知恢复超时标志(exp1)
 *   [12]T6恢复后通知返回(exp0) [13]T6正常通知超时标志(exp0) [14]T6收到新通知(exp1) [15]A总运行次数(exp3)
 *   [16]一轮成本(阻塞表 8 个"挂起+待通知"节点, 从让出到下一轮) [17]SendNotify(锁内异步)单次周期(100次摊销)
 *   [18]Resume(挂起+待通知)单次周期 [19]计时自校准开销 [20]sizeof(XC_TaskCB_t)
 */
#include <stddef.h>
#include <string.h>
#include "XCOS.h"
#include "XC_Cor.h"
#include "XC_Sch.h"
#include "XC_Task.h"
#include "Internal/XC_Core.h"
#include "Internal/XC_List.h"

/* 手写 SysTick 寄存器: 免 CMSIS 依赖(核内外设, 模拟器已建模) */
typedef struct { volatile uint32_t CTRL; volatile uint32_t LOAD; volatile uint32_t VAL; volatile uint32_t CALIB; } XC_SysTick_t;
#define XC_SYSTICK  ((XC_SysTick_t*)0xE000E010UL)
#define XC_CYC_LOAD 0x00FFFFFFUL

XCOS_t            g_xc;
volatile uint32_t g_res[24];

static void     CycInit(void) { XC_SYSTICK->LOAD = XC_CYC_LOAD; XC_SYSTICK->VAL = 0U; XC_SYSTICK->CTRL = 5U; }
static uint32_t CycGet(void)  { return (XC_CYC_LOAD - XC_SYSTICK->VAL); }

void SimDone(void) { }

static XC_TaskCB_t s_z[8];   /* 阻塞表: 8 个 挂起 + 待处理通知 节点 */
static XC_TaskCB_t s_A;      /* 场景目标任务 */
static XC_TaskCB_t s_B;      /* 场景驱动任务(线性步进) */

static int         s_N1 = 0x5A5A; /* 挂起前登记的通知数据 */
static int         s_N2 = 0x1234; /* 挂起期间被拒的通知数据 */
static int         s_N3 = 0x7A7A; /* 恢复后正常通知数据   */
static int         s_A_RunCnt = 0;
static int         s_A_T1 = -1, s_A_T2 = -1, s_A_T3 = -1;
static const void* s_A_D1 = NULL;
static const void* s_A_D3 = NULL;
static int         s_T1_state = -1, s_T1_run = -1;
static int         s_T2_resume = -1, s_T2_to = -1, s_T2_dataok = -1, s_T2_run = -1;
static int         s_T3_send = -1, s_T3_dataok = -1;
static int         s_T5_state = -1, s_T5_resume = -1, s_T5_to = -1;
static int         s_T6_send = -1, s_T6_to = -1, s_T6_dataok = -1, s_T6_run = -1;static void DummyYield(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) { XC_Cor_Yield(); }
    XC_Cor_Leave();
}

static void Task_A(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    XC_Cor_ClrNotify();
    XC_Cor_WaitNotify(0U);            /* 阶段0: 死等(被挂起的目标) */
    s_A_RunCnt++;
    s_A_T1 = XC_Cor_IsNotifyTimeout() ? 1 : 0;
    s_A_D1 = XC_Cor_GetNotifyData();
    XC_Cor_WaitNotify(0U);            /* 阶段1: 无通知挂起 -> 恢复 */
    s_A_RunCnt++;
    s_A_T2 = XC_Cor_IsNotifyTimeout() ? 1 : 0;
    XC_Cor_WaitNotify(0U);            /* 阶段2: 恢复后正常通知 */
    s_A_RunCnt++;
    s_A_T3 = XC_Cor_IsNotifyTimeout() ? 1 : 0;
    s_A_D3 = XC_Cor_GetNotifyData();
    while(1) { XC_Cor_Yield(); }
    XC_Cor_Leave();
}

static void Task_B(XC_TaskHandle_t t)
{
    uint32_t t0, t1;

    XC_Cor_Enter(t);

    /* R1: 锁窗口异步通知(挂起前登记) + 挂起 A */
    XC_Core_Lock(&g_xc);
    (void)XC_Task_SendNotify(&s_A, (void*)&s_N1);
    XC_Core_Unlock(&g_xc);
    (void)XC_Task_Suspend(&s_A);
    t0 = CycGet();
    XC_Cor_Yield();

    /* R2: 轮边界成本(含阻塞表 8 个"挂起+待通知"节点的事件补偿) */
    t1 = CycGet();
    g_res[16] = (t1 - t0) - g_res[19];

    s_T1_state  = (int)XC_Task_GetState(&s_A);
    s_T1_run    = s_A_RunCnt;
    s_T3_send   = (int)XC_Task_SendNotify(&s_A, (void*)&s_N2);
    s_T3_dataok = (XC_Task_ReadNotifyData(&s_A) == (void*)&s_N1) ? 1 : 0;
    t0 = CycGet();
    s_T2_resume = (int)XC_Task_Resume(&s_A);
    t1 = CycGet();
    g_res[18] = (t1 - t0) - g_res[19];
    XC_Cor_Yield();

    /* R3: A 已恢复并记录阶段0 */
    s_T2_to     = s_A_T1;
    s_T2_dataok = (s_A_D1 == (const void*)&s_N1) ? 1 : 0;
    s_T2_run    = s_A_RunCnt;
    (void)XC_Task_Suspend(&s_A);
    XC_Cor_Yield();

    /* R4: A 应仍为 SUSPEND */
    s_T5_state  = (int)XC_Task_GetState(&s_A);
    s_T5_resume = (int)XC_Task_Resume(&s_A);
    XC_Cor_Yield();

    /* R5: A 记录阶段1(应报未通知) */
    s_T5_to   = s_A_T2;
    s_T6_send = (int)XC_Task_SendNotify(&s_A, (void*)&s_N3);
    XC_Cor_Yield();

    /* R6: A 记录阶段2; 汇总 + 评判 */
    s_T6_to     = s_A_T3;
    s_T6_dataok = (s_A_D3 == (const void*)&s_N3) ? 1 : 0;
    s_T6_run    = s_A_RunCnt;

    g_res[2]  = (uint32_t)s_T1_state;
    g_res[3]  = (uint32_t)s_T1_run;
    g_res[4]  = (uint32_t)s_T2_resume;
    g_res[5]  = (uint32_t)s_T2_to;
    g_res[6]  = (uint32_t)s_T2_dataok;
    g_res[7]  = (uint32_t)s_T2_run;
    g_res[8]  = (uint32_t)s_T3_send;
    g_res[9]  = (uint32_t)s_T3_dataok;
    g_res[10] = (uint32_t)s_T5_state;
    g_res[11] = (uint32_t)s_T5_to;
    g_res[12] = (uint32_t)s_T6_send;
    g_res[13] = (uint32_t)s_T6_to;
    g_res[14] = (uint32_t)s_T6_dataok;
    g_res[15] = (uint32_t)s_T6_run;

    g_res[0] = 14U;
    g_res[1] = 0U;
    if(g_res[2]  != 4U) { g_res[1]++; } /* T1 挂起后状态 SUSPEND */
    if(g_res[3]  != 0U) { g_res[1]++; } /* T1 未被事件调度唤醒 */
    if(g_res[4]  != 0U) { g_res[1]++; } /* T2 Resume 返回 OK */
    if(g_res[5]  != 0U) { g_res[1]++; } /* T2 视为已通知 */
    if(g_res[6]  != 1U) { g_res[1]++; } /* T2 数据=挂起前通知 */
    if(g_res[7]  != 1U) { g_res[1]++; } /* T2 恢复后运行一次 */
    if(g_res[8]  != 1U) { g_res[1]++; } /* T3 挂起期发送 FAIL */
    if(g_res[9]  != 1U) { g_res[1]++; } /* T3 数据未被覆盖 */
    if(g_res[10] != 4U) { g_res[1]++; } /* T5 状态 SUSPEND */
    if(g_res[11] != 1U) { g_res[1]++; } /* T5 报未通知 */
    if(g_res[12] != 0U) { g_res[1]++; } /* T6 通知 OK */
    if(g_res[13] != 0U) { g_res[1]++; } /* T6 已通知 */
    if(g_res[14] != 1U) { g_res[1]++; } /* T6 数据=新通知 */
    if(g_res[15] != 3U) { g_res[1]++; } /* A 总运行 3 次 */

    SimDone();
    while(1) { XC_Cor_Yield(); }
    XC_Cor_Leave();
}

int main(void)
{
    uint32_t i, c0, c1;

    memset(&g_xc, 0, sizeof(g_xc));
    memset(g_res, 0, sizeof(g_res));
    XC_Sch_Init(&g_xc);

    CycInit();
    c0 = CycGet();
    c1 = CycGet();
    g_res[19] = c1 - c0;                       /* 计时自校准 */
    g_res[20] = (uint32_t)sizeof(XC_TaskCB_t);

    /* 造 8 个"挂起 + 待处理通知"节点(与 A2 触发链一致, 全部走公开 API) */
    for(i = 0U; i < 8U; i++) {
        (void)XC_Task_Reg(&g_xc, &s_z[i], DummyYield, NULL);
    }
    for(i = 0U; i < 8U; i++) {
        XC_Task_HandleWaitNotify(&s_z[i], 2000U);   /* 进时间表 + WAIT */
        XC_Core_Lock(&g_xc);
        (void)XC_Task_SendNotify(&s_z[i], NULL);    /* 锁窗口: 异步登记 pending=1 */
        XC_Core_Unlock(&g_xc);
        (void)XC_Task_Suspend(&s_z[i]);             /* 挂起: 阻塞表 + SUSPEND */
    }

    /* SendNotify(锁内异步) 单次成本 */
    c0 = CycGet();
    for(i = 0U; i < 100U; i++) {
        XC_Core_Lock(&g_xc);
        (void)XC_Task_SendNotify(&s_z[0], NULL);
        XC_Core_Unlock(&g_xc);
    }
    c1 = CycGet();
    g_res[17] = ((c1 - c0) - g_res[19]) / 100U;

    /* 场景任务: 先注册 B 再注册 A => 就绪表 [A, B] */
    (void)XC_Task_Reg(&g_xc, &s_B, Task_B, NULL);
    (void)XC_Task_Reg(&g_xc, &s_A, Task_A, NULL);

    XC_Sch_Start(&g_xc);
    while(1) { }
    return 0;
}
