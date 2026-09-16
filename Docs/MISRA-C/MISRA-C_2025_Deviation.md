# XCOS — MISRA C:2025 Deviation 记录

> **版本**: 1.0
> **创建日期**: 2026-08-14
> **适用范围**: XCOS v2.1.0 代码库（`Code/` 目录）
> **依据**: MISRA C:2025 规则清单（`Docs/MISRA-C/MISRA_C_2025_Rules_Complete.md`）
> **性质**: 本文件为 Deviation **通知记录**，用于归档代码库对 MISRA 规则的偏差事实及理由，供后续审查、审计与合规性追溯使用。

---

## Deviation 汇总

| ID | 规则 | 级别 | 主题 | 记录日期 |
|----|------|------|------|:----:|
| DEV-XCOS-001 | Rule 11.5 / Rule 20.10 / Rule 15.1 | Advisory | 协程底层设计约束（`void*`过渡 + `##` + `goto`） | 2026-08-14 |
| DEV-XCOS-002 | Rule 8.7 | Advisory | 全局滴答计数外部链接 | 2026-08-14 |
| DEV-XCOS-003 | Rule 2.3 | Advisory | 未使用的类型声明（用户 API） | 2026-08-14 |
| DEV-XCOS-004 | Rule 8.13 | Advisory/Undecidable | 指针 const 限定 | 2026-08-14 |

---

## DEV-XCOS-001 — 协程底层设计约束

### 基本信息

| 字段 | 内容 |
|------|------|
| Deviation ID | DEV-XCOS-001 |
| 规则 | Rule 11.5（void 指针转换对象指针）/ Rule 20.10（`##` 运算符）/ Rule 15.1（goto 语句） |
| 级别 | 🟡 Advisory |
| 文件 | `Code/Inc/Internal/XC_List.h`、`Code/Inc/Internal/XC_CorGNU.h`、`Code/Inc/Internal/XC_CorANSI.h`（嵌套协程 v2.1.0 起） |
| 记录日期 | 2026-08-14 |

### 违规描述

**Rule 11.5** — `void*` 过渡转换：

```c
// XC_List.h:155
#define XC_LIST_TO_TCB(pNode) ((struct XC_TaskCB_t*)(void*)(pNode))
```

**Rule 20.10** — `##` 预处理运算符：

```c
// XC_CorGNU.h:53
#define COR_BP2(S1, S2) S1##S2
```

**Rule 15.1** — goto 语句（协程底层，GNU 版 + ANSI 版）：

```c
// XC_CorGNU.h:95, 106, 125 / XC_CorANSI.h:74, 83, 104(嵌套协程 v2.1.0 回退)
goto COR_GOTO_END;
// XC_Cor.h(用户可见宏):XC_Cor_Leave 内的编译器可达性提示(2026-08 起,消除 armcc #111-D)
if(0) { goto COR_DONE_L; } COR_DONE_L:;
```

### 偏差理由

| 规则 | 理由 |
|------|------|
| Rule 11.5 | `XC_LIST_TO_TCB` 是 container_of 专用宏。C 标准（C89~C23 §6.7.2.1）保证结构体指针与首成员指针地址相同，`void*` 过渡语义完全安全。且已从原 Rule 11.3（Required）降级为 Rule 11.5（Advisory） |
| Rule 20.10 | GNU 协程必须用 `##` 拼接 `__func__` 与 `__LINE__` 生成唯一 goto 标签名，这是计算跳转协程的核心机制，无替代方案 |
| Rule 15.1 | 协程使用 `goto *BP` 计算跳转实现上下文切换（GNU），`goto COR_GOTO_END` 是向前跳转至函数末尾（cleanup 模式），符合 Rule 15.2 要求，无替代方案。**ANSI 版自嵌套协程 v2.1.0 起回退使用同一 `goto COR_GOTO_END` 模式**（`COR_Break`/`COR_SetBPBreak`/`COR_End`/`COR_BreakLabel`）：`break` 在循环体内跳出的是循环而非协程 `switch`，控制流会落入 `XC_Cor_Leave` 误置 `CorState=DONE`，使调度器"同轮切父"误判弹帧（方案附例 `for(;;)+Call` 实测死循环、子协程从不运行），故 ANSI 版必须与 GNU 版一致使用 `goto`。**另**：`XC_Cor_Leave` 宏内 `if(0){ goto COR_DONE_L; } COR_DONE_L:;` 为编译器可达性提示（`goto` 在常量假分支内、永不执行），用于消除 armcc 对协程标准写法 `while(1){...}XC_Cor_Leave()` 的 #111-D 告警，与协程挂起/完成 `goto` 同类，一并纳入本 Deviation |

### 缓解措施

1. **ANSI-C 替代方案（零 goto）不再成立**：嵌套协程方案（v2.1.0）要求 ANSI 版回退 `goto`。`XC_CorANSI.h` 原基于 `switch`/`case` 实现、`COR_Break`/`COR_SetBPBreak`/`COR_End` 用 `break` 替换 goto（`f4607ea`，MISRA-C:2025 修正），但 `break` 在循环体内跳出的是循环而非协程 `switch`，导致 `CorState=DONE` 被误置（调度器"同轮切父"误判弹帧、死循环/子协程被跳过）；嵌套协程要求挂起点可写在 `for`/`while` 体内（方案附例即 `for(;;)+Call`），故 ANSI 版必须恢复 `goto COR_GOTO_END`（前向跳转至 `DONE` 赋值之后的函数末尾标签，跳过 `DONE`），与 GNU 版语义对齐。`XC_TypeInternal.h:30-34` 仍按 `__GNUC__` 自动选择：`__GNUC__` 用 GNU 版，否则用 ANSI 版（两版本现均使用 `goto`）。
2. `XC_LIST_TO_TCB` 宏内已添加 MISRA 合规注释说明（`XC_List.h:151-154`）。

### 结论

**记录完成**。协程底层（GNU 版 + ANSI 版）实现机制不可避免：ANSI 版自嵌套协程 v2.1.0 起因 `break` 在循环体内语义缺陷回退 `goto`，两版本现均使用 `goto COR_GOTO_END`（前向跳转、cleanup 模式）。

---

## DEV-XCOS-002 — 全局滴答计数外部链接

### 基本信息

| 字段 | 内容 |
|------|------|
| Deviation ID | DEV-XCOS-002 |
| 规则 | Rule 8.7（外部链接但仅单 TU 引用） |
| 级别 | 🟡 Advisory |
| 文件 | `Code/Src/XC_Time.c`、`Code/Inc/XC_Config.h` |
| 记录日期 | 2026-08-14 |

### 违规描述

```c
// XC_Time.c:28 — 定义（外部链接）
volatile XC_Tick_t g_SysTickCount = 0U; // 用户系统Tick

// XC_Config.h:230 — extern 声明暴露给用户
extern volatile XC_Tick_t g_SysTickCount; // 外部声明全局滴答时间计数
```

从框架内部看，`g_SysTickCount` 仅在 `XC_Time.c` 单 TU 定义，其余 TU 通过宏 `XC_SYS_TICK_COUNT` 间接引用。按 Rule 8.7 应设为 `static`。

### 偏差理由

`g_SysTickCount` **不是纯内部变量，是用户 API**：

- 框架文档（`XC_Config.h:210-216`）明确要求用户**在自己的中断服务程序（ISR）中直接累加**此变量，实现系统滴答计数
- 用户代码位于框架之外，若设为 `static`，用户无法访问 → **破坏公开 API 与向后兼容性**
- 典型用户用法：
  ```c
  // 用户 ISR（框架之外的文件）
  void SysTick_Handler(void) {
      g_SysTickCount++;  // 需要 extern 链接
  }
  ```

### 缓解措施

已提供访问函数宏封装（`XC_Time.h`）：

```c
#define XC_Time_TickInc() do { g_SysTickCount++; } while(0)
#define XC_Time_TickSet(_Tick) do { g_SysTickCount = (_Tick); } while(0)
```

用户可通过宏或直接访问两种方式使用，保持最大兼容性。

### 结论

**记录完成**。作为用户 API，必须保持外部链接供用户中断访问。

---

## DEV-XCOS-003 — 未使用的类型声明

### 基本信息

| 字段 | 内容 |
|------|------|
| Deviation ID | DEV-XCOS-003 |
| 规则 | Rule 2.3（项目不应包含未使用的类型声明） |
| 级别 | 🟡 Advisory |
| 文件 | `Code/Inc/XC_Type.h` |
| 记录日期 | 2026-08-14 |

### 违规描述

```c
// XC_Type.h:70-73
typedef struct {
    XC_Tick_t TickCount;
    XC_Tick_t WaitCount;
} XC_TimerTick_t;
```

`XC_TimerTick_t` 在核心调度模块（XC_Sch/XC_Task）中未直接使用。

### 偏差理由

- `XC_TimerTick_t` 是**用户公开 API 类型**，用于扩展时间处理功能
- 实际上已被 `XC_Time.c` 的扩展时间处理函数使用：
  - `XC_Time_TimerGetRemain(XC_TimerTick_t)`
  - `XC_Time_TimerGetElapsed(XC_TimerTick_t)`
  - 以及 `XC_Time.h` 中的 `XC_Time_TimerSet` / `XC_Time_TimerCheck` 等宏
- 头文件文档明确标注为"[用户]软件定时器计数类型"，是 API 完整性的一部分

### 缓解措施

已在 `XC_Time.h`/`XC_Time.c` 提供完整的扩展时间处理 API 使用该类型，类型并非真正"未使用"。

### 结论

**记录完成**。作为用户 API 类型，内部未直接使用但对外提供，且已被扩展时间模块实际使用。

---

## DEV-XCOS-004 — 指针 const 限定

### 基本信息

| 字段 | 内容 |
|------|------|
| Deviation ID | DEV-XCOS-004 |
| 规则 | Rule 8.13（只要可能，指针应指向 const 限定类型） |
| 级别 | 🟡 Advisory / Undecidable |
| 文件 | `Code/Inc/XC_Sch.h`、`Code/Inc/XC_Task.h`、`Code/Inc/XC_Time.h` 等 API 头文件 |
| 记录日期 | 2026-08-14 |

### 违规描述

API 函数指针参数未加 const 限定，例如：

```c
void XC_Sch_Init(XC_OSHandle_t phXCOS);
void XC_Sch_Start(XC_OSHandle_t phXCOS);
uint8_t XC_Sch_GetTaskNum(XC_OSHandle_t phXCOS);
XC_Return_t XC_Task_SendNotify(XC_TaskHandle_t phTCB, void* pNotifyData);
```

### 偏差理由

1. **绝大多数 API 需修改数据，无法加 const**：
   - `XC_Sch_Init` / `XC_Sch_Start` / `XC_Sch_TimeSched`：写 TCB/XCOS 状态
   - `XC_Task_SendNotify` / `XC_Task_Remove` / `XC_Task_Reset`：写 TCB 字段
   - 只有极少数纯查询函数（如 `XC_Sch_GetTaskNum`）只读

2. **typedef 结构约束**：
   ```c
   // XC_Type.h:91
   typedef XCOS_t* XC_OSHandle_t;
   ```
   若全局改为 `const XCOS_t*`，所有**需要写数据**的函数将编译失败（向 const 指针指向的对象写入）；只能为只读查询单独定义 const 版本句柄，破坏 API 一致性。

3. **收益有限**：可加 const 的仅 `XC_Sch_GetTaskNum` 等个别纯查询函数，改造成本与 API 复杂度不成比例。

### 缓解措施

不改变句柄 typedef 设计。如未来需要，可为只读查询函数单独引入 `XC_OSHandleConst_t` 类型，但当前收益不匹配成本。

### 结论

**记录完成**。Undecidable 规则，结合句柄 typedef 设计约束，统一记录偏差。

---

## 记录清单

本文件所有 Deviation 均为**通知记录**，记录日期见各条目基本信息表与顶部汇总表。

| Deviation ID | 规则 | 记录日期 |
|--------------|------|----------|
| DEV-XCOS-001 | Rule 11.5 / 20.10 / 15.1 | 2026-08-14 |
| DEV-XCOS-002 | Rule 8.7 | 2026-08-14 |
| DEV-XCOS-003 | Rule 2.3 | 2026-08-14 |
| DEV-XCOS-004 | Rule 8.13 | 2026-08-14 |

---

## 规则 → 代码位置索引（2026-09-14 新增）

> **目的**：把"代码改动后需人工复核 MISRA 偏差"变成**可勾选的索引 + 固定流程**，降低人工复核负担。
> **注意**：下表行号会随重构漂移，**复核时以"符号/宏名"为准**（行号仅作起点）。
> **脚本化**：下方流程第 2 步的锚点检查，已明确划归 **回归 / CI 通道**（顶层 `Tests/`：`run_all.ps1` / `run_tests.ps1`）做脚本化。

### A. Deviation → 代码位置

| Deviation | 规则 | 级别 | 代码位置（锚点） | 复核要点 |
| --- | --- | --- | --- | --- |
| DEV-XCOS-001 | Rule 11.5 | Advisory | `Code/Inc/Internal/XC_List.h`（`XC_LIST_TO_TCB`，`void*` 过渡；宏内已有 MISRA 合规注释） | 宏与合规注释仍在；新增 `void*`↔对象指针转换需先查本表 |
| DEV-XCOS-001 | Rule 20.10 | Advisory | `Code/Inc/Internal/XC_CorGNU.h`（`COR_BP2`，`##` 标签拼接） | `##` 仅用于协程标签拼接 |
| DEV-XCOS-001 | Rule 15.1 | Advisory | `Code/Inc/Internal/XC_CorGNU.h`、`Code/Inc/Internal/XC_CorANSI.h`（`goto COR_GOTO_END`）、`Code/Inc/XC_Cor.h`（`XC_Cor_Leave` 可达性提示） | `goto` 只允许出现在上述协程底层白名单位置 |
| DEV-XCOS-002 | Rule 8.7 | Advisory | `Code/Src/XC_Time.c`（`g_SysTickCount` 定义）、`Code/Inc/XC_Config.h`（`extern volatile XC_Tick_t g_SysTickCount`） | 该变量是**用户 API**（ISR 直接累加）⇒ 不得改 `static` |
| DEV-XCOS-003 | Rule 2.3 | Advisory | `Code/Inc/XC_Type.h`（`XC_TimerTick_t`） | 该类型仍被 `XC_Time.c` 的定时器 API 使用（非真正"未使用"） |
| DEV-XCOS-004 | Rule 8.13 | Advisory / Undecidable | `Code/Inc/XC_Sch.h`、`Code/Inc/XC_Task.h`、`Code/Inc/XC_Time.h`（句柄参数：`XC_OSHandle_t`/`XC_TaskHandle_t`） | 句柄 typedef 设计未变；如引入 `const` 句柄需整体评估 |

### B. 同步流程（每次改动 `Code/` 后 4 步）

| # | 步骤 | 做法 |
| --- | --- | --- |
| 1 | **编译 0 告警** | 用工程实际编译器（AC5/AC6）或 `armclang --target=arm-arm-none-eabi -mcpu=cortex-m3 -Wall -Wextra -c` 全量编译，确认 **0 告警** |
| 2 | **锚点抽查** | 按上表 A 核对"代码位置"的锚点（宏名/标签名而非行号），确认偏差条目仍成立 |
| 3 | **新偏差先登记** | 新增 `void*` 转换、`goto`、`##`、外部链接变量、位运算/联合体等构造前先查本表；构成新偏差 ⇒ 新增 `DEV-XCOS-00x`（规则/级别/代码位置/理由/缓解措施） |
| 4 | **记录与脚本化** | 偏差增减写入 `CHANGELOG.md`；把第 2 步固化为脚本，纳入**回归 / CI 通道**（顶层 `Tests/`） |

---

*文档结束*
