# 20260914 B4 每任务运行统计（可选功能）

> **条目**：[`Docs/问题跟踪/问题清单/20260914_诊断能力与工程化.md`](../../问题清单/20260914_诊断能力与工程化.md)（✅ 已修复）
> **状态**：已实施并归档（2026-09-14；用户决策"本版本实现"，采用方案 A = TCB 字段）

## 做了什么

打开 `XC_CFG_TASK_STATS`（默认关）后，调度器在每个"**调度槽**"前后取时刻，为每个任务累计：

| 字段（TCB，仅开关打开时存在） | 含义 |
| --- | --- |
| `RunCnt`（`uint32_t`） | 运行次数（**饱和计数**，不回绕） |
| `MaxRunTick`（`XC_Tick_t`） | **最长一次**运行耗时（单位 Tick） |

| 文件 | 内容 |
| --- | --- |
| `Code/Inc/XC_Config.h` | 开关 `XC_CFG_TASK_STATS`（默认 `0`）+ 口径/体积/边界注释 |
| `Code/Inc/Internal/XC_TypeInternal.h` | TCB 字段（`[并发]` 契约：主上下文，仅调度器写） |
| `Code/Inc/XC_Sch.h` / `Code/Src/XC_Sch.c` | 内部包裹 `XC_Diag_RunStatsBegin()` / `XC_Diag_RunStatsEnd(phTCB, start)` + 主循环两处调用 |
| `Code/Inc/XC_Task.h` / `Code/Src/XC_Task.c` | 查询/清零 API + 注册/移除时清零 |

**统计口径**：从任务被摘出就绪表 → 到它让出（延时/等待/挂起/让出）为一次调度槽；**含"同轮切父"的父层时间，不含等待时间**；差值用无符号减法（Tick 回绕安全）。

## 怎么跑这个用例

```powershell
# 关闭档（应正常编译/运行, 统计字段与统计代码都不编译）
gcc -std=gnu99 -O1 -Wall -Wextra -I Code/Inc `
    Code/Src/XC_List.c Code/Src/XC_Sch.c Code/Src/XC_Task.c Code/Src/XC_Time.c `
    "Docs/问题跟踪/修正方案/20260914_B4_运行统计/测试/20260914_B4_运行统计_测试.c" -o b4_off.exe

# 统计档（13 项断言: 尺寸/精度/回绕/清零/真实调度器端到端/生命周期）
gcc -std=gnu99 -O1 -Wall -Wextra -DXC_CFG_TASK_STATS=1 -I Code/Inc `
    Code/Src/XC_List.c Code/Src/XC_Sch.c Code/Src/XC_Task.c Code/Src/XC_Time.c `
    "Docs/问题跟踪/修正方案/20260914_B4_运行统计/测试/20260914_B4_运行统计_测试.c" -o b4_on.exe
```

期望：两档都输出 `===== B4 用例完成: failures 0 =====`。

> 用例要点：直接驱动内核 Tick 源 `g_SysTickCount`（无需定时器中断）；用 `setjmp/longjmp` 从任务内**跳出调度器死循环** ⇒ 覆盖**真实主循环**的统计路径；
> 「第 3 次调度因 longjmp 跳出、未走统计终点」故不计入，用例按 2 次断言（口径说明见用例内注释）。

## 实测结果（2026-09-14）

| 项 | 结果 |
| --- | --- |
| 宿主（关 / 开 / 三开关全开） | 关：TCB 尺寸未变 + 调度器照常运行；开：**13 项 PASS**；三开关(B1+B3+B4)同开：**failures 0** |
| 对象级（armcc AC5 `-O1`，诊断模块化后） | `XC_Diag.o` 关 **0** → 开 **56**；`XC_Sch.o` 600 → **616(+16)**；`XC_Task.o` 884 → **896(+12)**（合计 **+84 B**） |
| 镜像级（`Examples/CortexM3_Test`，12 任务） | 改前 `Code 8684 / ZI 1944` → 关 **8684 / 1944（一致，0 Error/0 Warning）** → 开 **8764(+80) / 2040(+96 = 12×8B)** |
| 回归 | A1 通知回归 ALL PASS、A2、状态一致性与挂起唤醒、B1/B3 用例 均 failures = 0 |

## 怎么在工程里用

```c
#define XC_CFG_TASK_STATS (1U)   /* 排障/调优期打开; 发布可关闭 */
#include "XCOS.h"

uint32_t  runs = XC_Diag_GetRunCnt(&s_TaskA);     /* 运行次数 */
XC_Tick_t peak = XC_Diag_GetMaxRunTick(&s_TaskA); /* 最长一次(Tick; 默认 1ms/tick) */
/* peak 偏大 ⇒ 该任务需分片(周期性 XC_Cor_Yield)或拆分 */
```

> 注意：1 ms 时基下**短于 1 tick 记 0**（要更细用计数器模式或 `>1000Hz` 高频档）；耗时**含运行期间的中断时间**（是"调度槽耗时"，要看指令周期用 MDK 仿真测量）。
