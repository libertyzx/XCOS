# 20260914 A9 唤醒/入表统一「排队尾」（回归 V2.0 语义）

> **条目**：[`Docs/问题跟踪/问题清单/20260914_内核缺陷-通知与调度语义.md`](../../问题清单/20260914_内核缺陷-通知与调度语义.md)（✅ 已修复）
> **状态**：已实施并归档（2026-09-14）；用户决策：**照 V2.0 全统一表尾**（协作式应以公平轮转为主，含 `fIdle`）

## 做了什么

把"**进入就绪表**"的位置**统一为队尾**（严格轮转），覆盖：等待中通知 / 提前通知 / 定时到点唤醒 / 复位 / 恢复 / 注册 / Tick 溢出回绕整表。

| 文件 | 内容 |
| --- | --- |
| `Code/Inc/Internal/XC_TaskInternal.h` | 删除表首语义的 `XC_Task_MoveToReadyList`；新增 `XC_Task_InsertToReadyListTail`（注册）与 `XC_Task_MoveToReadyListTail`（唤醒/复位/恢复） |
| `Code/Src/XC_Task.c` | 注册 / 复位 / 恢复 / 等待中通知 4 处改为排队尾 |
| `Code/Src/XC_Sch.c` | 到点唤醒 / `EventSched` 落地 / 溢出回绕整表 3 处改为排队尾；顺带修正 `WAKEUP_NOTIFY` 上方**过期注释**（"未排除 SUSPEND"实为 A2 已修复） |

**实现纪律（踩坑后固化）**：`XC_Task_MoveToReadyListTail` 必须"**先 `XC_List_Remove` 再插表尾**"——直接 `XC_List_MoveNodeAfter(ReadyList.pPrev, node)` 在该节点**恰为表尾**（或仍在表中）时会**自引用破坏链表**（实测：表根变自环 ⇒ `XC_List_ListValid` 假）。

## 为什么是"回归 V2.0"

| 版本 | 就绪表机制 | 入表规则 |
| --- | --- | --- |
| **V2.0** | 跨轮游标 `pPrevReadyNode`；**只有一个**入表宏 `XCTask_MoveTaskToReadyList` = `XCList_MoveNodeAfter(phXCOS->pPrevReadyNode, ...)` | **统一：排队尾**（排在"本轮已排队者之后"；无 `PUT_TAIL` 之类第二宏） |
| V2.1.0（修复前） | 去掉游标（取表首 + 摘除游离化 + 自环判定归位） | **分裂**：归位/提前通知=表尾；唤醒/定时/复位/恢复/注册=**表首**（宏名沿用、锚点由"游标"换成"表根" ⇒ 语义漂移） |

⇒ 修复 = 保留 V2.1.0 的**无游标结构**（避免 DEF-20260821 游标失效），把"入表"收敛回 V2.0 的**单一规则**。

## 怎么跑这个用例

```powershell
gcc -std=gnu99 -O1 -Wall -Wextra -I Code/Inc `
    Code/Src/XC_Diag.c Code/Src/XC_List.c Code/Src/XC_Sch.c Code/Src/XC_Task.c Code/Src/XC_Time.c `
    "Docs/问题跟踪/修正方案/20260914_A9_唤醒即队尾/测试/20260914_A9_唤醒公平性_测试.c" -o a9.exe
```

用例特点：**确定性**（`trace` 记录每个被调度任务 + 固定调度次数 + `longjmp` 跳出调度器），**不依赖线程/真实中断**；两个场景各断言一段 trace。

## 实测（before / after）

| 场景 | 修复前（插队） | 修复后（排队尾） |
| --- | --- | --- |
| 定时唤醒：周期任务 `P`（延时 2 tick）+ 普通任务 `R`（推进 tick 后让出） | `RPRPRPRPR`（**P 被唤醒后抢到 R 前面**） | **`PRRPRRPRR`**（P 排队尾，P:R ≈ 1:2 ✓） |
| 通知唤醒：`R1`（通知者）+ `R2`（观察者）+ `Z`（等待者） | `Z21Z21Z21`（**Z 插队**到观察者之前） | **`12Z12Z12Z`**（Z 排队尾 ✓） |

> 忽略 `[trace]` 前缀以外的一切；两段 trace 的差异就是 A9 的行为差异。
> 「过载 ⇒ 独占 ⇒ 饿死」是"插队"的极端后果，需要**真实中断/线程源**才能确定性复现，故本用例只断言**顺序**（结论与推演见条目与 `Docs/XCOS.md`）。

## 连带修改

- `20260914_A2_挂起优先级与通知交付_测试.c`：补**前导同步**（让出直到 A 进入 `BLOCKED`）—— 原用例隐含依赖"注册插表首 ⇒ 后注册的 A 先运行"；适配后**与原快照 0 差异**；
- `Tests/snapshots/S2-游标失效对照.txt`：`B_state` 2(READY) → **4(SUSPEND)**（更符合"挂起保持"预期；判据 `b_c=0` 不变）。

## 体积/内存（E1 通道）

| 项 | 结果 |
| --- | --- |
| 对象级（armcc AC5 `-O1 --cpu=Cortex-M3`） | `XC_Sch.o` 600→**632**(off)、`XC_Task.o` 884→**912**(off)（"先摘除再插尾"每处约 +8 B） |
| 镜像级（示例工程） | `default` 8684→**8768（+84 B）**、`stats` 8764→**8852（+88 B）**；**RO/RW/ZI 全不变 ⇒ RAM 0** |
| 回归 | E1 宿主 **PASS 14 / FAIL 0 / SKIP 4**；体积通道全 delta=0（基线已重建） |
