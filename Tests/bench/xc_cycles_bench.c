/**
 * @file  xc_cycles_bench.c
 * @brief [周期通道] 关键路径周期测点（**替换镜像工程的 `main.c`**，由 `Tests/run_cycles.ps1` 驱动）
 * @details
 *  - 计时: **SysTick 递减计数器**（技能文档 `Docs/Skill/MDK_Sim_Agent_Skill.md` §4.1）:
 *          `SysTick->LOAD=0xFFFFFF; VAL=0; CTRL=5`，读 `LOAD - VAL`；并**校准计时自身开销**(连续两次取值差)后逐项扣除;
 *  - 结果写 `volatile uint32_t g_res[]`，由 `cycles.ini`（`g, SimDone` + `DumpRes()`）打印;
 *  - **只调用"线程模式可调用"的 API**：不启动调度器、不依赖外设与中断 ⇒ 在 µVision 模拟器里确定可复现;
 *  - 口径: 均为"**单次调用**的处理器周期数"（AC5 `-O1`，Cortex-M3 模拟器；绝对值仅供同口径对比）。
 */
#include "main.h" /* 带来 XCOS.h + CMSIS(HAL) */
#include <stdint.h>

#define CYC_LOAD 0x00FFFFFFU
#define N_DELAY  20U

volatile uint32_t g_res[16]; /* [ini 打印] 结果数组 */
static XCOS_t      g_os;
static XC_TaskCB_t g_tcb[N_DELAY + 4U];

void SimDone(void); /* [ini 断点符号] */

static void CycInit(void)
{
    SysTick->LOAD = CYC_LOAD;
    SysTick->VAL  = 0U;
    SysTick->CTRL = 5U; /* CLKSOURCE(CPU) | ENABLE */
}

static uint32_t CycGet(void)
{
    return (CYC_LOAD - SysTick->VAL);
}

/** 任务体（本测点**不启动调度器**(`XC_Sch_Start`)，任务体不会被运行 ⇒ 仅占位;
 *  刻意不用协程宏 `XC_Cor_Enter/Leave`：避免 armcc 在"未被引用的协程标签"上报 #177-D 告警） */
static void TaskBody(XC_TaskHandle_t phTCB)
{
    (void)phTCB;
}

/** 测点结束标记(供 .ini 的 `g, SimDone` 定位; 内含副作用以免空函数被优化消除) */
void SimDone(void)
{
    g_res[15] = g_res[15] + 0U;
}

int main(void)
{
    uint32_t i;
    uint32_t t0;
    uint32_t t1;
    uint32_t ovh;
    volatile uint32_t spin = 0U;

    CycInit();
    t0       = CycGet();
    t1       = CycGet();
    ovh      = t1 - t0; /* 计时自身开销(每项扣除) */
    g_res[15] = ovh;

    XC_Sch_Init(&g_os);
    for(i = 0U; i < (N_DELAY + 4U); i++) {
        (void)XC_Task_Reg(&g_os, &g_tcb[i], TaskBody, NULL);
    }

    /* r0: 时间表插入(锁内有序插入) —— **表首(最好情况)**: 新节点截止时刻最小 ⇒ 首次比较即定位 */
    for(i = 0U; i < N_DELAY; i++) {
        XC_Task_HandleDelay(&g_tcb[i], 1000U + i);
    }
    t0 = CycGet();
    XC_Task_HandleDelay(&g_tcb[N_DELAY], 1U);
    t1 = CycGet();
    g_res[0] = (t1 - t0) - ovh;

    /* r5: 时间表插入(锁内有序插入) —— **表尾(最坏情况)**: 新节点截止时刻最大 ⇒ 遍历全表
     *  (O3 性能专题的历史口径即此: 20 节点表尾插入 ≈335 周期) */
    t0 = CycGet();
    XC_Task_HandleDelay(&g_tcb[N_DELAY + 4U - 1U], 1000000U);
    t1 = CycGet();
    g_res[5] = (t1 - t0) - ovh;

    /* r1: 搬表到"就绪表尾"(入表统一排队尾的路径: 先摘除(自环) 再插尾) */
    t0 = CycGet();
    XC_Task_MoveToReadyListTail(&g_tcb[N_DELAY]);
    t1 = CycGet();
    g_res[1] = (t1 - t0) - ovh;

    /* r2: 唤醒 API(目标正在等待): `SendNotify` 直接路径(含移表 + 置状态) */
    (void)XC_Task_HandleWaitNotify(&g_tcb[N_DELAY + 1U], 0U);
    t0 = CycGet();
    (void)XC_Task_SendNotify(&g_tcb[N_DELAY + 1U], NULL);
    t1 = CycGet();
    g_res[2] = (t1 - t0) - ovh;

    /* r3: 锁窗口登记路径(主循环持锁 ⇒ ISR 发通知只登记, 不移表) */
    (void)XC_Task_HandleWaitNotify(&g_tcb[N_DELAY + 2U], 0U);
    XC_Core_Lock(&g_os);
    t0 = CycGet();
    (void)XC_Task_SendNotify(&g_tcb[N_DELAY + 2U], NULL);
    t1 = CycGet();
    XC_Core_Unlock(&g_os);
    g_res[3] = (t1 - t0) - ovh;

    /* r4: 恢复挂起(含移表) */
    (void)XC_Task_Suspend(&g_tcb[N_DELAY + 3U]);
    t0 = CycGet();
    (void)XC_Task_Resume(&g_tcb[N_DELAY + 3U]);
    t1 = CycGet();
    g_res[4] = (t1 - t0) - ovh;

    /* r13/r14: 结构体尺寸(与体积基线交叉校验) */
    g_res[13] = (uint32_t)sizeof(XC_TaskCB_t);
    g_res[14] = (uint32_t)sizeof(XCOS_t);

    SimDone();
    /**
     * 不写 `return 0`: 调试脚本在 `SimDone()` 处接管(断点 + DumpRes);
     * 若断点异常(未命中), 保持在死循环里 ⇒ 既不返回(避免跑进启动代码), 也不产生"不可达语句"告警。
     */
    for(;;) {
        spin++;
    }
}
