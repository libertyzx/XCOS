/**
 * @file  20260916_空闲Tick计算_仿真测点.c
 * @brief [A17 仿真测点] 在 µVision 模拟器（Cortex-M3 / armcc AC5 -O1）上**真跑调度器**，统计 `fIdle` 收到的 `IdleTick`
 * @details
 *  **用法**（本文件**替换镜像工程的 `main.c`**，由 `Tests/run_cycles.ps1` 驱动；**不改仓库内工程文件**）：
 *  ```
 *  powershell -File Tests\run_cycles.ps1 `
 *      -BenchFile "Docs\问题跟踪\修正方案\20260916_A17_空闲Tick钳位与取值纪律\测试\20260916_空闲Tick计算_仿真测点.c" `
 *      -IniFile   "Docs\问题跟踪\修正方案\20260916_A17_空闲Tick钳位与取值纪律\测试\20260916_空闲Tick计算_仿真测点.ini" `
 *      -ProjectDefine "XC_CFG_TICKS_PER_SEC=100000U" `
 *      -ConfigTickSource "XC_SimTickRead"
 *  ```
 *  - `XC_CFG_TICKS_PER_SEC=100000U`：以**高频档**（100 kHz，`>1000Hz` 的 us 时基分支）构建；
 *  - `-ConfigTickSource XC_SimTickRead`：脚本把"声明 + `#define XC_SYS_TICK_COUNT XC_SimTickRead()`"
 *    写进**镜像副本**的 `XC_Config.h`（Tick 源切"**计数器模式**"；仓库文件不动）；
 *  - **不启动中断/不依赖外设**（µVision 仿真无外设模型 ⇒ 中断跑不出来）：任务只用
 *    `XC_Cor_Enter / XC_Cor_DelayTick` 让出，Tick 完全由本文件脚本化推进 ⇒ **定点注入**：
 *    在"空闲分支那次读 Tick"上 `+2`，等价于"`TimeSched` 读完 Tick 后、空闲分支读 Tick 之前又过了 2 个 tick"
 *    ⇒ 复现 A17 隐患①（"时间表首节点已过点"）的触发条件；
 *  - 收尾：采满 `SIM_SAMPLES` 次后先 `FillRes()` 写 `g_res[]`，再调 `SimDone()`（由 `.ini` 的 `g, SimDone` 命中并打印）。
 *  ---
 *  **实测（2026-09-16，armcc AC5 -O1 / Cortex-M3 模拟器 / 100 kHz）**：
 *
 *  | 指标                                | 修复前                         | 修复后       |
 *  | ----------------------------------- | ------------------------------ | ------------ |
 *  | 采样数                              | 300                            | 300          |
 *  | **异常大值次数（≥ `0x80000000`）** | **300**                        | **0**        |
 *  | 提示为 `0` 的次数                   | 0                              | **300**      |
 *  | 见过的最大值                        | **4294967295（`0xFFFFFFFF`）** | **0**        |
 *  | 注入次数 / `ticks_per_sec`          | 300 / 100000                   | 300 / 100000 |
 *
 *  > 复跑注意：`.ini` 需与 `Tests/bench/cycles.ini` **同构**（`LOG` → `FUNC DumpRes` → `g, SimDone` → `DumpRes()` → `D &g_res, &g_res+63`）；
 *  > 早期自写的 ini 写法会让 µVision 脚本**静默中止**（表现为"日志已建但无任何输出"）。
 *  ---
 *  `g_res[]`：`[0]` 采样数 / `[1]` **异常大值（≥ `0x80000000`）次数** / `[2]` `0` 次数 / `[3]` 最大值 /
 *             `[4]` 注入次数 / `[5]` `XC_CFG_TICKS_PER_SEC` / `[6]` 正常小值次数
 *
 *  **判读**：修复前 `[1] ≈ [0]`（`IdleTick` 被回绕成 ≈`0xFFFFFFFF`）；修复后 `[1] == 0` 且 `[2] ≈ [0]`（"已过点 ⇒ 0"）。
 */
#include "main.h" /* 带来 XCOS.h + CMSIS(HAL) */
#include <stdint.h>

#define SIM_SAMPLES 300U /* 采满这么多次空闲回调就结束仿真 */

volatile uint32_t g_res[8]; /* [ini 打印] 结果数组 */

static XCOS_t      s_os;
static XC_TaskCB_t s_tcb;

/* ---------------- 脚本化 Tick 源（计数器模式；由工程宏指向本函数） ---------------- */
static volatile XC_Tick_t s_tick;    /* 当前 Tick(只由本文件推进) */
static unsigned           s_readN;   /* 本轮迭代已读 Tick 次数(任务开始/空闲回调里复位) */
static int                s_taskRan; /* 本轮迭代任务是否运行过(其内部会读 1 次 Tick) */
static uint32_t           s_inject;  /* 注入次数 */

/* ---------------- 统计 ---------------- */
static uint32_t s_ilN;    /* 空闲回调次数 */
static uint32_t s_absurd; /* IdleTick >= 0x80000000 ⇒ 异常大值 */
static uint32_t s_zero;   /* IdleTick == 0  ⇒ "不要休眠" */
static uint32_t s_norm;   /* 其余(正常小值) */
static uint32_t s_max;    /* 见过的最大 IdleTick */

void        SimDone(void); /* [ini 断点符号] */
static void FillRes(void); /* 结果汇总(在 SimDone() 之前调用) */

/**
 * @brief [测试钩子] 读当前 Tick + **定点注入**
 * @details
 *  注入点 = 本轮"**空闲分支**"的那次读。读序：
 *  - 任务运行过的迭代：①任务(`HandleBlocking` 读) ②`TimeSched` 读 ③空闲分支读；
 *  - 任务未运行的迭代：①`TimeSched` 读 ②空闲分支读；
 *  ⇒ 命中"空闲分支读"时 `+2`（等价于"这轮干活期间, 定时器又走了 2 个 tick"）。
 */
XC_Tick_t XC_SimTickRead(void)
{
    XC_Tick_t t      = s_tick;
    unsigned  target = (s_taskRan != 0) ? 3U : 2U;

    s_readN++;
    if(s_readN == target) {
        s_tick = (XC_Tick_t)(s_tick + 2U); /* 注入: 窗口内前进 2 个 tick */
        t      = s_tick;
        s_inject++;
    }
    return (t);
}

/** 周期任务: 每次运行都延时 1 tick（工程既有 `while(1)` 写法；armcc 走 ANSI 协程实现） */
static void TaskTick(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) {
        s_taskRan = 1;
        s_readN   = 0U; /* 迭代边界(就绪表非空的那轮不会进空闲回调, 故这里也要复位) */
        XC_Cor_DelayTick(1U);
    }
    XC_Cor_Leave();
}

/** 空闲回调: 统计框架给的提示值 */
static void Idle(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)
{
    (void)phXCOS;
    s_ilN++;
    if(IdleTick > s_max) {
        s_max = IdleTick;
    }
    if(IdleTick >= 0x80000000U) {
        s_absurd++; /* 异常大值: 回绕(下溢)或垃圾值 */
    }
    else if(IdleTick == 0U) {
        s_zero++; /* 正确语义: "已过点/有工作 ⇒ 不要休眠" */
    }
    else {
        s_norm++; /* 正常: 距下一个任务唤醒还有这么多个 tick */
    }

    s_readN   = 0U; /* 迭代边界: 复位本轮读计数 */
    s_taskRan = 0;

    if(s_ilN >= SIM_SAMPLES) {
        FillRes(); /* 先写结果, 再进断点符号 ⇒ 断点命中即可读到 */
        SimDone();
    }
}

/** 汇总结果到 `g_res`（必须在断点符号 `SimDone()` 之前调用） */
static void FillRes(void)
{
    g_res[0] = s_ilN;
    g_res[1] = s_absurd;
    g_res[2] = s_zero;
    g_res[3] = s_max;
    g_res[4] = s_inject;
    g_res[5] = (uint32_t)XC_CFG_TICKS_PER_SEC; /* 打印配置, 确认"高频档"生效 */
    g_res[6] = s_norm;
}

/** [ini 断点符号] 结束标记（带副作用，防被优化消除） */
void SimDone(void)
{
    g_res[7] = 0xA17U; /* 断点命中时该值尚未写入 ⇒ 用 [0..6] 判读 */
}

int main(void)
{
    XC_Sch_Init(&s_os);
    XC_Sch_SetIdleCallback(&s_os, Idle);
    (void)XC_Task_Reg(&s_os, &s_tcb, TaskTick, NULL);

    XC_Sch_Start(&s_os); /* 不返回: 由 Idle 回调内的 SimDone()(.ini 断点)结束仿真 */
    return (0);
}
