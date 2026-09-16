---
name: xcos-dev-2.1.0
description: 开发与维护 XCOS 2.1.0 内核的工程技能（改代码 / 加开关 / 调语义 / 同步文档 / 跑三通道回归与重建基线）。给出"改哪里、怎么改、改完必须同步什么、如何验收"的可执行清单，以及本项目真实踩过的坑。适用：C（ANSI/GNU 双底层）+ PowerShell 脚本。面向内核维护者与 AI 编码代理。
---

# XCOS 内核开发技能（Agent Skill）

> 本技能面向 **要改 XCOS 本身的人**（维护者 / AI 编码代理）：动 `Code/` 之前先读本文件，按 **"改动类别 → 影响面 → 验收闭环"** 执行。
>
> **三份技能的分工**：

| 技能文档 | 解决什么 |
| --- | --- |
| [`XCOS_Agent_Skill.md`](./XCOS_Agent_Skill.md) | **用** XCOS 写应用：编程模型、API 速查、标准代码模式、用户侧陷阱 |
| [`MDK_Sim_Agent_Skill.md`](./MDK_Sim_Agent_Skill.md) | 只有 Keil MDK 时，用 **µVision 无界面仿真**实测周期 / 体积 / RAM |
| **本文件** | **开发 / 维护 XCOS 内核**：改哪、怎么改、改完同步什么、如何用 `Tests/` 验收与重建基线 |

> **路径约定**：技能文档内一律用**仓库相对路径**；**不引用** `Docs/问题跟踪/` 的内部编号（只引用 `Docs/XCOS.md`、`Docs/架构概览/`、`Tests/` 等公开材料）。

---

## 一、开工前（5 分钟）

1. **先备份**：`%TEMP%\xcos_<批次>_<时间戳>\`（`Code/` + `Docs/` + `Tests/` + `README.md`）。
   > 文档类改动也照备：本项目曾因批量替换脚本的 bug **写坏过两个文档**（见 §十二 陷阱 1）。
2. **读三条流程**：`Tests/README.md`（怎么跑 / 怎么加用例）、`Docs/问题跟踪/README.md`（问题→方案→验证的登记规范）、本文件 §八（标准动作）。
3. **判定改动类别**，决定影响面与必须做的验收（见 §八.1 的对照表）。
4. **记住四条"不要"**：
   - 不在热路径加断言 / 统计（如每次调度槽都会调用的函数）；
   - 不对**同一形参 + 同一失效条件**同时写"断言 + 返回校验"（§四 单一强制点）；
   - 不让**公开文档**引用问题跟踪编号（只写技术描述）；
   - 不用"整文件覆盖"的方式写 `Tests/baseline.json`（§十二 陷阱 2）。

---

## 二、仓库结构与文件职责

```
Code/
├─ Inc/                       # 公共头（用户可见）
│   ├─ XCOS.h                 # 总入口：版本宏 + 汇总包含（含 XC_Compat.h）
│   ├─ XC_Config.h            # ★ 所有配置开关 + XC_Tick_t 定义 + 编译期断言
│   ├─ XC_Type.h              # 用户类型（不透明句柄、枚举）
│   ├─ XC_BitPatterns.h       # BIN_* 常量（体积大、无依赖）
│   ├─ XC_Err.h               # 错误上报设施：错误码 + 用户钩子 + 上报宏（**无实现文件**）
│   ├─ XC_Diag.h              # 诊断能力(用户/调试可见: 自检 + 统计查询; 实现可物理裁剪)
│   ├─ XC_Cor.h               # 协程块宏（Enter/Leave/Call/Yield/Delay*/WaitNotify…）
│   ├─ XC_Task.h / XC_Sch.h / XC_Time.h   # 任务 / 调度 / 时间 API
│   ├─ XC_Compat.h            # 旧名兼容层（计划 V2.2.0 删除）
│   └─ Internal/              # 内部头（用户不直接用）
│       ├─ XC_Core.h          # 框架实例锁（Lock/Unlock/GetLockState 三宏 + 契约）
│       ├─ XC_DiagInternal.h  # 诊断能力(实现侧: 断言宏 XC_DIAG_ASSERT + 调度槽计时埋点)
│       ├─ XC_List.h          # 双向链表 + XC_LIST_TO_TCB
│       ├─ XC_TypeInternal.h  # XCOS_t / TCB 定义（**并发契约写在这里**）
│       ├─ XC_TaskInternal.h  # 入表/出表宏（入表规则集中处）
│       ├─ XC_TimeInternal.h  # Tick 换算宏 + 静态断言
│       └─ XC_CorANSI.h / XC_CorGNU.h    # 协程双底层（`__GNUC__` 自动选）
└─ Src/
    ├─ XC_Diag.c              # 自检 + 统计实现（全关时为空编译单元，可整文件裁剪）
    ├─ XC_List.c / XC_Time.c / XC_Task.c / XC_Sch.c
```

**"新东西放哪"判定**：

| 要加的东西 | 放哪 |
| --- | --- |
| 新 API（用户可见） | `Inc/XC_<模块>.h` 声明 + `Src/XC_<模块>.c` 实现（`@brief [用户]`） |
| 内部函数/宏 | `Inc/Internal/XC_<模块>Internal.h`（`@brief [内部]`） |
| 新配置开关 | `Inc/XC_Config.h`（**唯一定义处**，含默认值 + 成本/边界说明） |
| 新诊断能力 | 用户/调试可见 → `Inc/XC_Diag.h`；实现侧(断言宏/埋点) → `Inc/Internal/XC_DiagInternal.h`（+ `Src/XC_Diag.c`，默认关、可裁剪） |
| 新错误码 | `Inc/XC_Err.h` 的 `XC_ErrCode_t` |
| 新类型（用户可声明变量） | `Inc/XC_Type.h`；内部类型 → `Inc/Internal/XC_TypeInternal.h` |
| 测试用例 | `Docs/问题跟踪/修正方案/<日期>_<主题>/测试/*.c` + `Tests/manifest.txt` 登记（**不要**复制进 `Tests/`） |
| 决策/问题记录 | `Docs/问题跟踪/问题清单/<日期>_<类别>.md` + 索引登记 |

---

## 三、命名与代码风格

1. **命名（强制，用户侧 Skill 同款）**：函数/函数式宏 `XC_<模块>_<功能>`；类型 `XC_<模块><名词>_t`；枚举值/常量宏全大写；
   断言宏 `XC_DIAG_ASSERT` 是有意例外；**旧名只进 `XC_Compat.h`**，新代码只用规范名。
2. **文件头模板**（新建文件照抄；`@details` 写清楚"这个文件负责什么/边界"）：
   ```c
   /**
    * @file        XC_Xxx.c
    * @brief       一句话
    * @author      libertyzx (libertyzx@163.com)
    * @version     2.1.0
    * @date        YYYY/MM/DD
    * **********************************************
    * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
    * @license     This project is released under the MIT License.
    * **********************************************
    * @details     细节/边界/并发级别
    * **********************************************
    *  修改日志
    *  - 见"CHANGELOG.md"的更新说明;
    */
   ```
   > "修改日志"段**统一为一行**（细节一律进 `CHANGELOG.md`），全仓 24 个文件已统一。
3. **注释密度**：公共 API 的 `@details` 必须写清 **语义 / 边界 / 前置契约 / 并发级别**；容易踩的地方用 `> 注意:` 或 `⚠️`；
   内部实现注释解释"**为什么这样写**"（例：`XC_Task_MoveToReadyListTail` 为什么必须"先摘除再插尾"）。
4. **分隔线**：沿用仓库既有 `/* *** **我是分割线** *** */` 分节风格，便于与既有文件保持一致。
5. **脚本（`Tests/*.ps1`）必须纯 ASCII**：Windows PowerShell 5.1 会把"无 BOM 的 UTF-8"脚本按 ANSI 解码 ⇒ 脚本内的中文会被破坏；
   中文一律放 `manifest.txt` / `*.md`（脚本用显式 UTF-8 读取）。

---

## 四、断言 vs 返回码：**单一强制点**（改内核最常踩的规则）

> **规则一句话**：**同一形参 + 同一失效条件，只允许一个强制点** —— 要么断言，要么返回错误码，**不得叠加**；
> 不同形参、或同一形参的**不同**条件可并存（那是两条独立契约）。

| 规则 | 内容 |
| --- | --- |
| **R1 互斥** | 同一形参 + 同一失效条件 ⇒ **二选一**（`XC_DIAG_ASSERT` 与 `if(...) return` 不得同时出现） |
| **R2 允许并存** | 不同形参 / 同一形参的不同条件 ⇒ 可混用（例：`assert(p != NULL)` + `if(len > MAX) return XC_FAIL;`） |
| **R3 禁止部分重叠** | 复合 `if` 中不得再 `OR` 已被断言覆盖的子条件（例：`if((n >= MAX) \|\| (p == NULL))` 的 `p == NULL` 若已断言 ⇒ 删掉该子条件） |
| **R4 方向判据** | **前置契约 / 编程错误**（参数 `NULL`、非法参数组合）⇒ **断言**（发布档不检查，`@details` 标注"违背 = 未定义行为"）；**运行期前提 / 契约承诺要报告的结果**（表满、重复注册、未挂起、自检失败项）⇒ **返回错误码** |
| **R5 归属层级** | 契约写在**最小公共入口**（如 `XC_Task_Reg`）；包装函数只断言自己新增的契约 |
| **R6 宏例外** | 宏类 API 无返回路径 ⇒ **只能断言**（`@details` 写明"调用方保证非空"） |
| **R7 跨 API 一致** | 同一类失效（如参数 `NULL`）在各 API 间必须**一致选择** |

**本内核的取向（性能优先）**：**参数合法性 = 断言**（发布档 0 开销）／**运行期前提与承诺结果 = 返回码**。

**写法样板**（公开 API 入口）：
```c
XC_Return_t XC_Task_Reg(XC_OSHandle_t phXCOS, XC_TaskHandle_t phTCB, void (*fTask)(XC_TaskHandle_t), void* pParam)
{
    XC_DIAG_ASSERT(phTCB != NULL);   /* 前置契约: 违背 = 未定义行为(只上报, 不 return) */
    XC_DIAG_ASSERT(phXCOS != NULL);
    XC_DIAG_ASSERT(fTask != NULL);
    if(phTCB->phXCOS != NULL)        { return (XC_FAIL); }   /* 运行期前提: 已注册 */
    if(phXCOS->TaskNum >= XC_CFG_MAX_TASKS) { return (XC_FAIL); }  /* 运行期前提: 表满 */
    ...
}
```

**开销（实测，armcc AC5 `-O1 --cpu=Cortex-M3`，仅断言档）**：

| 项 | 数值 |
| --- | --- |
| 断言落点 | **26 处**（`XC_Task.c` 19 / `XC_Sch.c` 4 / `XC_Diag.c` 3） |
| 断言档增量（相对"仅钩子"） | **+360 B**（≈14 B/处） |
| 关档 | **0 代码 / 0 RAM**（宏展开为空，**条件不求值**） |
| 若给断言加位置参数 | 带文件名 **+152 B**、仅行号 **+92 B**（结论：**不加**，用调试器断点 + 调用栈定位，见 `Docs/架构概览/XCOS_V2.1.0_架构概览.md` §11.6） |

> 新增断言落点后，**必须同步**：`Docs/XC_Config.md` 的调用点表、`Tests/baseline.md` 的"断言档"数值、`Docs/XCOS.md` 的"调用点"行。

---

## 五、并发与字段契约（改 TCB / XCOS_t 时必读）

**四种并发模型**（字段只能属于其中一种，写清"写者集合 + 模型 + 安全依据"）：

| 模型 | 含义 | 必须满足 |
| --- | --- | --- |
| **主上下文** | 只有主循环写，中断不碰 | 中断只读也谨慎（可能读到瞬时旧值） |
| **锁串行化** | 主循环 + 中断"直接路径"都写，靠 `Lock` 握手 | 写点必须在锁窗内；ISR 只允许 `SendNotify` 的直接搬表分支 |
| **SPSC（常量写）** | 单生产者 ↔ 单消费者，两侧**只写常量**（1/0） | 严禁 `++` / `|=` / 位域（退化成读-改-写） |
| **MPSC（常量写）** | 多生产者（所有中断）写同一常量、主循环清 0 | 生产侧只写常量 1；消费侧"先清后处理" |

**锁的三条纪律**（`Code/Inc/Internal/XC_Core.h`）：

1. 它**不是互斥锁**，是"主循环正处于关键区"的**单一标志位**：不可重入、无等待队列、无原子性；
2. **必须配对**：每个 `XC_Core_Lock` 恰有一次 `Unlock`（注意 `XC_Task_HandleWaitNotify` → `XC_Task_HandleBlocking` 的**隐式配对**，改这两处必须保持"每条路径恰好解锁一次"）；
3. 中断侧**只允许**"读到 `Lock == 0` 才获取、完成后配对释放"，**禁止在中断里 Unlock**（会把主循环的保护提前打开）。

**其它硬约束**：ISR 只走通知（会移表的 API 一律主循环调用）；入就绪表**统一排队尾**（严格轮转，无优先级补偿）；新增字段必须在 `Docs/架构概览/XCOS_V2.1.0_架构概览.md`「并发模型与字段所有权」矩阵登记（含 `Code/Inc/Internal/XC_TypeInternal.h` 的 `[并发]` 注释标签）。

---

## 六、功能开关的设计纪律

1. **开关定义只在 `Code/Inc/XC_Config.h`**（`#ifndef` + 默认值 + "关档成本/开档成本/边界"说明）；实现处只做 `#if` 分支。
2. **默认全关、关档 0 开销**：关档时字段与代码都不编译；"函数式宏"关档展开为 `((void)0)`（**实参不求值**）。
3. **可物理裁剪**：诊断实现集中在 `Code/Src/XC_Diag.c`，发布工程可不添加该文件（前提：所有诊断开关为 0；开关为 1 却未加入该文件 ⇒ **链接报错**，属预期）。
4. **组合依赖用 `#error` 守住**：例 `XC_CFG_ASSERT=1` 而 `XC_CFG_ERR_HOOK=0` ⇒ 编译报错。
5. **编译期断言范式**（负数组长度，不产生代码/数据）：
   ```c
   typedef char XC_StaticAssert_TickWidth[(sizeof(XC_Tick_t) >= 4U) ? 1 : -1];
   ```
   已用：Tick 宽度 ≥32 位、`XC_CFG_TICKS_PER_SEC ≤1000` 时必须是 1000 的因子、`XC_CFG_MAX_TASKS` 必须在 **1~255**（超上限会让 `XCOS_t.TaskNum`（`uint8_t`）回绕、`TaskNum--` 下溢）。
   **验证纪律**：凡写了"取值范围/必须满足"的配置，都要补一个**反向用例**（`-DXC_CFG_X=<越界值>` 必须**编译失败**，边界值如 `1`/`255` 必须通过）—— 光看"默认档能编过"抓不到漏断言。
6. **开关取值约束**：现有代码统一用 `!= 0` 判断 ⇒ 文档写"0/1"；若新增开关，建议同时加"取值只能 0/1"的静态断言，避免 `2` 之类的写法歧义。
7. 新增开关必须同步：`Docs/XC_Config.md`（参数表 + 详解）、`Docs/XCOS.md`（对应能力节）、`Docs/架构概览`（§1.4 与 §十一）、`Tests/manifest.txt`（"关/开"两条用例）。

---

## 七、语义变更纪律（改行为时）

- **入就绪表规则**：一律**排队尾**（唤醒 / 定时到点 / 复位 / 恢复 / 注册 / 时间表批量搬移）；改这条等于改"公平性契约"⇒ 必须同步
  `Docs/XCOS.md`「调度顺序与唤醒语义」、`Docs/Skill/XCOS_Agent_Skill.md` 陷阱、`CHANGELOG`；
- **失败模式**：能"安全降级"的路径要保留降级（例：帧栈越界 ⇒ 上报 + 复位任务），不要在中间插入"提前 return 且不解锁"的分支；
- **兼容性**：改 API 签名 / 语义 = **破坏性变更** ⇒ 要么提供 `XC_Compat.h` 别名（宏/函数名可映射，**签名不行**），要么并入 `V2.2.0`（与删除兼容层同批）并在 `CHANGELOG` 标注 BREAKING + 迁移说明。

---

## 八、改内核的标准动作（七步闭环）

### 8.1 先判定"改动类别 → 影响面"

| 改动类别 | 影响面 | 必做 |
| --- | --- | --- |
| **纯注释**（增删行） | **Code 可能变** —— ANSI 协程的断点常量取 `__LINE__` | 跑体积通道；有漂移就**重建基线** |
| 加/删分支、改函数体 | Code 变；热路径还影响周期 | 体积 **+** 周期 + 重建基线 + `CHANGELOG` |
| 改 `TCB` / `XCOS_t` 字段 | **RAM（ZI）变** | 重测 ZI + `架构概览` §八 + `XCOS.md` 字段矩阵 + `[并发]` 注释 |
| 加开关 / 加断言落点 | 对象级各档体积变 | 全档实测 + 调用点表 + `baseline.md` + 文档（§九） |
| 改公开 API 签名 | **破坏性** | 兼容层或并入 `V2.2.0` + 全套文档 + BREAKING |
| 改语义（入表规则等） | 用户可见行为 | `XCOS.md` 语义节 + 用户侧 Skill 陷阱 + `CHANGELOG` |
| 只改 `Tests/` | 回归结果 | 跑三通道 + `Tests/README.md` 同步 |

### 8.2 七步

1. **编码**：遵守 §三~§七（命名 / 单一强制点 / 并发契约 / 开关纪律）。
2. **严格编译 3 档**（每档都要 **0 告警**）：
   ```powershell
   $env:PATH += ';C:\Software\qalculate'   # mingw 运行库目录(不加会 cc1 启动失败)
   $gcc = 'C:\Software\CLion\bin\mingw\bin\gcc.exe'
   # 每文件单独编译(多文件不能带 -o); 3 档 = 默认 / 断言档 / 四开关全开
   & $gcc -c -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wcast-qual -Wmissing-prototypes `
          -Wold-style-definition -Wconversion -Wsign-conversion -Wundef -Wswitch-enum `
          -ICode/Inc [-DXC_CFG_ERR_HOOK=1 -DXC_CFG_ASSERT=1 -DXC_CFG_DEBUG_CHECK=1 -DXC_CFG_TASK_STATS=1] `
          Code/Src/XC_Task.c -o "$env:TEMP\x.o"
   ```
3. **功能回归**：`powershell -File Tests/run_tests.ps1`（当前基线：**PASS 17 / FAIL 0 / SKIP 3**，共 20 条用例）。
   - 不过 ⇒ 先看是不是"**删/改了别人依赖的行为**"（见 §十二 陷阱 5）；
   - 新增用例：把 `.c` 放 `Docs/问题跟踪/修正方案/<专题>/测试/`，再在 `Tests/manifest.txt` 登记（列定义见 `Tests/README.md` §4）。
4. **体积**：`powershell -File Tests/run_size.ps1`（对象级 **±0 B** / 镜像级 **±8 B**）。
   - 漂移是否"应有"？**应有** ⇒ 走第 6 步；**不应有** ⇒ 找出原因，别直接重建基线。
5. **周期**：`powershell -File Tests/run_cycles.ps1`（关键路径 **±5%**，µVision 模拟器）。
   - ⚠️ 跑周期时**不要并行**跑别的重命令（会占满机器导致仿真会话超时、`SimDone` 未到达 ⇒ 假失败）。
6. **重建基线**（仅当 delta 是预期的）：
   ```powershell
   powershell -File Tests/run_size.ps1  -WriteBaseline     # 只更新 object/image, 保留其它段(合并写入)
   powershell -File Tests/run_cycles.ps1 -WriteBaseline    # 只更新 cycles
   ```
   然后把关键数字与"**为什么变**"同步到 `Tests/baseline.md`（§2/§2.1/§3/§4）与 `CHANGELOG.md`。
7. **总验收 + 登记**：`powershell -File Tests/run_all.ps1` ⇒ **`=> ALL PASS`**；随后登记 `CHANGELOG.md` 与 `Docs/问题跟踪/`（§十一），并做 §九 的文档同步。

**当前基线快照**：数值一律以 `Tests/baseline.md`（机器口径 `Tests/baseline.json`）为准 ——
对象级逐文件 Code（off / diag）见 §2、单开关逐档见 §2.1、镜像 Code（`default` / `stats` / `diag`）见 §3、周期 9 项见 §4；
判定阈值：对象 **±0 B**、镜像 **±8 B**、周期 **±5%**。

---

## 九、文档同步矩阵（改完必须同步哪些）

| 改动 | 必须同步 |
| --- | --- |
| API 新增 / 签名 / 语义 | `Docs/XCOS.md`（函数表）、`Docs/Skill/XCOS_Agent_Skill.md`（API 速查）、`Docs/架构概览/`、`CHANGELOG.md` |
| **新增/修改枚举** | **`Docs/XCOS.md` 对应枚举表加/改行**（本项目曾漏 `XC_ERR_ASSERT` ⇒ 每次必查） + 用户侧 Skill「类型与枚举」 |
| 配置开关 | `Code/Inc/XC_Config.h`（注释与成本）、`Docs/XC_Config.md`（参数表 + 详解 + 体积表）、`Docs/XCOS.md`、`Docs/架构概览/` §1.4 与 §十一、`Tests/manifest.txt` |
| 体积 / 内存数字 | **不要在代码注释或公开文档里写死数字** —— 一律指向 `Tests/baseline.md`（机器口径 `Tests/baseline.json`）；非写不可时只写量级 + 指针 |
| 文件增删 / 改名 / 移动 | 根 `README.md` 目录树、`Docs/架构概览/`「关联代码」行、`Tests/README.md`、`Docs/Skill/` 文件索引、**`.gitignore`**（新增的构建产物 / IDE 本地产物要登记；产物优先写到 `%TEMP%`，别落仓库） |
| 行为 / 语义 | `XCOS.md` 相关节、用户侧 Skill 陷阱、`CHANGELOG.md`、`Docs/ROADMAP.md`（里程碑级才改） |
| **模块 / 公开 API / 调度语义** | **`Docs/XCOS.md`「🏗️ 架构概览」的 API 总览表**（只列有哪些函数/类型/常量，纯文本零渲染依赖）+ `Docs/架构概览/` §一的调度主循环 **ASCII 图** —— 架构图**只用文本维护**（ASCII 图/表格，不用 XMind / 图片 / Mermaid），改完顺带跑 §十 的坏链检查 |
| 诊断 / 断言规则 | `架构概览` §11.6（断言 vs 返回码边界规则 R1~R7）、`架构概览` §十一（诊断成本表）、`XC_Config.md`、用户侧 Skill 陷阱 21 |
| **文档 / 注释同步（强制）** | 改了代码就要同步：**代码注释**（`@brief`/`@details`/`@retval` + 示例代码）→ `Docs/XCOS.md`（API 与约束）→ `Docs/XC_Config.md`（配置）→ `Docs/架构概览/`（实现）→ `Docs/Skill/*`（用法/陷阱）→ `Tests/*` 的说明 → `CHANGELOG.md`（**一句话**说明"做了同步"，**明细不逐条记录**，问题跟踪里也不再登记这类） |
| 代码位置 | **文档里不写 `file:line` 行号锚点**（会随注释漂移）—— 只写符号名/字段名 + 文件名 |

> **公开文档红线**：`README.md` / `Docs/*.md`（除 `Docs/问题跟踪/` 外）**不引用问题跟踪编号**，只写技术描述。

---

## 十、改完文档的最低自检（3 项，够用就好）

> 只保留"**不查会真出事**"的三项；文档措辞/数字逐条比对的"六步法"**已精简掉**（这类内容不再逐条留档）。

1. **坏链 + 代码围栏配对**（一条命令可查）：
   - 坏链 = 扫 `]( )` 里的目标路径是否存在（含相对路径）；
   - 围栏 = **逐行 toggle** 代码围栏，出现"**块内又带语言标记的开栏**"就是多了/少了围栏。
     ⚠️ **"围栏总数是偶数"查不出来**（块内作为文本出现的围栏会让总数变奇数却完全正确）；
     ⚠️ 多余围栏的后果很重：从那一行起**全文代码块错位**（标题被吞进代码块、整章按代码渲染）。
2. **数字只指向 `Tests/baseline.md`**：正文不写死体积/尺寸/计数；非写不可时只写量级 + 指针，并保证"**合计 == 各分项之和**"
   （历史"修正备注"最容易残留旧数字）。
3. **标识符 / 枚举 / 开关双向比对**（抓"改了代码没改文档"）：`Code/**` 的 `XC_*` ↔ 公开文档；枚举逐成员；`XC_Config.h` 开关 ↔ `Docs/XC_Config.md` 参数表。
   并核对 `Docs/XCOS.md` 的「API 总览」表：① 表内名字**必须都能在 `Code/` 里找到**；② **不得出现实现侧私有项**
   （判据：`Code/` 头文件里 `@brief` 标 `[内部]` **且用户不必写**的符号 —— 如 `XC_Err_Report` / `XC_DIAG_ASSERT` / `XC_CorState_t` / `XC_BP_t` / `XC_List_*` / `XC_Core_*` / `XC_Task_Handle*` / `XC_Diag_RunStats*`）；
   ⚠️ **例外且必须保留**（用户要配置或要声明，虽也可能被标 `[内部]`）：`XC_CFG_*` / `XC_SYS_TICK_COUNT`（用户在配置里设）、`XC_Tick_t`（用户在变量/延时里用，定义处标 `[内部]`）、`XC_CorFn_t` / `XC_CorFrame_t`（用户写子协程与帧栈数组，如示例 `main.c` 的 `XC_CorFrame_t s_hA10Stack[3]`）。

---

## 十一、问题跟踪与登记（本仓库的既定流程）

1. **目录**：`Docs/问题跟踪/问题清单/`（`README.md` 索引 + 批次文件）+ `Docs/问题跟踪/修正方案/<日期>_<主题>/`（`修正方案.md` + `测试/`，长周期专题加 `README.md`）。
2. **新问题默认"新建批次文件"**：`<提出日期>_<类别>.md`（**不要并入旧批次**）；同批次内追加才写进该批次文件；随后在 `问题清单/README.md` 索引表补一行。
3. **每条要有**：位置 / 问题 / 处置（含**实测证据**）/ 状态（✅ 已修复 · 🔄 追踪中）。
4. **记录范围**：只登记**框架自身的代码 / 配置**类条目；以下三类**都不留档** ——
   ① **"文档 · 注释同步"类**；② **"评估后不处理 / 实测不建议"类**；③ **自测工具（`Tests/`）自身的开发过程与坑**（工具属本版新增 ⇒ 写进 `Tests/README.md` / `Tests/baseline.md`）。
   结论用**一句话**写进 `CHANGELOG.md`；若某项还留着有价值的实测数据，把它放进对应的 `修正方案/<专题>/`（而不是问题清单）。
5. **测试用例**：`修正方案/<专题>/测试/<日期>_<主题>_<类型>.c`（复现 / 回归 / 综合 / 新旧对照），并在 `Tests/manifest.txt` 登记；
   快照类先 `run_tests.ps1 -UpdateSnapshots` 生成，**人工确认内容**后再提交。
6. **`CHANGELOG.md` 登记**：`## T<YYYY/MM/DD>` 段，用**大白话**写「**改了什么 · 为什么 · 对使用者什么影响**」，按"修了什么 / 新增什么 / 文档 / 工具 / 评估后不做"分组；
   **不要在正文里贴会过期的数值或测试术语**（`PASS x/y/z`、`delta=`、`±N B` 这类）—— 数值统一指向 `Tests/baseline.md`，逐条证据放 `Docs/问题跟踪/`。
   文件开头的"这份文件记什么 + 自测三件事"说明要保持有效。

---

## 十二、陷阱清单（逐条都是本项目真实踩过的）

| # | 陷阱 | 防护 |
| --- | --- | --- |
| 1 | **PowerShell 批量替换脚本**把"规则数组"当字符串索引（单元素时取到首字符）⇒ 曾把两个文档里的 `（` 全替换成 `可`；（**2026-09-16 再次发生**：把 `Docs/Skill/XCOS_Agent_Skill.md` 的 `（` 全替换成 `1`、`1` 全替换成 `8`，如 `2.1.0`→`2.8.0`、`run_all.ps1`→`run_all.ps8`） | 先备份；规则放 **TSV 数据文件**；逐条长度校验；替换前后**括号计数比对**；**工作区与 git 索引副本对照**（本次即靠"`（x209 / ）x209` vs `（x0 / ）x209`"与"`1`/`8` 计数差"定位，并从索引恢复、零内容损失） |
| 2 | `-WriteBaseline` **整文件覆盖** `baseline.json` ⇒ 删掉 `cycles` 段 ⇒ 周期通道报 `baseline has no "cycles" section` | 一律"**读旧 json → 改本段 → 写回**"（现已改为合并写入） |
| 3 | 文档写 `file:line` 锚点 ⇒ 注释一改就**全漂移** | 只写符号名/字段名 + 文件名，**不写行号** |
| 4 | **注释增删行也会改体积**（ANSI 协程断点取 `__LINE__`） | 改注释后照样跑体积通道，必要时重建基线 |
| 5 | **删既有校验要扫依赖**：删掉 `XC_Task_SetCorStack` 的参数校验后，**报错钩子用例**的 2 处期望（原来依赖它返回 `XC_FAIL`）失效 ⇒ 回归 4 条 FAIL | 改前全仓 grep 用例/文档对该行为的依赖；改后跑回归 |
| 6 | 同一条件同时写"断言 + 返回校验" | §四 单一强制点（R1~R7） |
| 7 | 关档宏写成"有副作用/会求值" ⇒ 关档仍有开销 | 关档一律 `((void)0)`（实参**不求值**） |
| 8 | 在**热路径**加断言/统计（如每次调度槽调用的函数） | 只在 API 入口/状态前提加 |
| 9 | 锁的**隐式配对**被破坏（`HandleWaitNotify` → `HandleBlocking`）⇒ 锁永久为 1，中断通知全部静默失效 | 改这两处保证"**每条路径恰好解锁一次**" |
| 10 | 在中断里 `Unlock`、或在 ISR 调用会移表的 API | ISR 只走通知；只允许"读到 `Lock==0` 才获取、配对释放" |
| 11 | 多写者字段用 `++` / `|=` / 位域（读-改-写）⇒ 丢更新（"丢唤醒"历史根因） | 多写者只能用**常量写**；确需计数用差值法 |
| 12 | µVision **增量构建不重编**（改了配置/换了 `main.c` 却没生效） | 构建前清 `Debug/`（详见 `MDK_Sim_Agent_Skill.md` 陷阱 2） |
| 13 | 周期通道与其它重命令**并行** ⇒ 仿真超时、`SimDone` 未到达（假失败） | 周期通道单独跑 |
| 14 | `gcc -c` 多文件带 `-o` ⇒ `cannot specify '-o' with '-c' ... multiple files` | 每文件单独编译到独立 `.o` |
| 15 | `Tests/*.ps1` 里写中文（脚本无 BOM 时 PS 5.1 按 ANSI 解码 ⇒ 乱码/逻辑错）；公开文档引用问题编号 | 脚本**纯 ASCII**，中文放 `*.md`/`manifest.txt`（数据文件用 `[IO.File]::ReadAllText(..., utf8)` 显式解码）；公开文档不引用编号。**自检**：`Tests/*.ps1` 里 `[\\u4e00-\\u9fff]` 的计数必须**全 0** |
| 16 | **新增/修改 `XC_CFG_*` 却忘了配编译期断言** ⇒ 越界配置**静默损坏**（如 `MAX_TASKS>255` 让 `uint8_t TaskNum` 回绕、`TaskNum--` 下溢） | 每个"有取值范围"的配置配一个负数组长度断言；用**反向用例**（越界值必须编译失败）验证，见 §六 第 5 条 |
| 17 | 文档里的旧数字 / 历史备注残留（表已重算、备注还写旧值） | 改数字时**连备注一起改**；正文只指向 `Tests/baseline.md`，并核对"合计 == 分项和"（§十 第 2 条） |
| 18 | 自己的核对脚本**假阳性**（正则把"调用示例"当原型、统计漏计分组标题里的多名） | 报缺陷前**回看原文行**确认；统计类检查先看"匹配到几条"（为 0 说明正则写错了） |
| 19 | **PowerShell 内联单行脚本写 `if(){...}; elseif(){...}`** ⇒ `elseif` 被当独立命令，疯狂报错刷屏 | 稍复杂的逻辑一律**写成 `.ps1` 文件**再跑 |
| 20 | 文档**多余代码围栏**（块闭合后又出现孤立 ```` ``` ````）⇒ 之后**全文代码块配对错位**：标题被吞进代码块、整章按代码渲染；且"围栏总数是偶数"**查不出来** | 用**配对扫描**（§十 第 1 条），改完文档跑一次 |
| 21 | `FAIL: compiler cannot run`（**没有任何编译错误输出**）—— **根因：调用方 shell 的 `PATH` 里没有工具链自己的 bin 目录** ⇒ `cc1.exe` 加载不到 `libssp-0.dll` / `libgcc_s_*.dll`，以 **`0xC0000135`（DLL 未找到）** 退出且无输出（`gcc --version` 却正常，极易误判为"代码坏了/杀软"） | `Tests/run_tests.ps1` 已**自愈**（自动把 `<gcc 目录>` + `-ExtraPath` 前置到 PATH）；手工排查：直接运行 `<mingw>\libexec\gcc\...\cc1.exe --version`，若退出码为 `-1073741515`（`0xC0000135`）即是此因；写自己的编译脚本时记得补工具链 bin 目录 |
| 22 | 读**别的进程正在写的日志**（如 µVision 写的 `sim_log.txt`）用 `ReadAllLines` ⇒ 抛"文件正由另一进程使用"，表现为**通道假失败**（`run_all` 里"体积→周期"连着跑时必现，单跑却常常通过） | 用 `[IO.File]::Open(path, Open, Read, FileShare.ReadWrite)` + 重试（本项目：5 次 × 400 ms，见 `Tests/run_cycles.ps1`）；先 `Stop-Process UV4` **不保证**立刻释放文件句柄 |
| 23 | 用"**假设锁失效 / 有人忘锁**"的防御性写法替代锁（判空后再取值时捕获指针自保、遍历每轮重读表头、解锁后复检…）⇒ 代码被写歪，且**掩盖了真正的锁纪律问题** | **锁是链表访问的唯一互斥手段**：锁内一律按"临界区内表不会被中断改动"编写；确需锁外访问 ⇒ 依赖**写明的不变量**；"既没锁又没不变量" ⇒ **修法是补锁**（见 `Docs/架构概览` §七「锁纪律」） |

---

## 十三、命令速查

| 目的 | 命令 |
| --- | --- |
| **三通道总验收（提交前必跑）** | `powershell -File Tests/run_all.ps1` |
| 只跑功能回归 | `powershell -File Tests/run_tests.ps1 [-Filter 通知] [-UpdateSnapshots]` |
| 只跑体积 | `powershell -File Tests/run_size.ps1 [-SkipImage] [-WriteBaseline]` |
| 只跑周期 | `powershell -File Tests/run_cycles.ps1 [-WriteBaseline] [-TolPercent 5]` |
| 换工具链路径 | 上述脚本加 `-Gcc <path> -Mdk <path> -ExtraPath <dir>` |
| 严格编译（3 档，0 告警） | 见 §8.2 第 2 步 |
| 单开关逐档体积 | 同口径逐档编 `XC_Task.c`/`XC_Sch.c`/`XC_Diag.c` + `fromelf --text -z` |
| 看改动面 | `git --no-pager diff --stat`（**记得 `--no-pager`**） |
| 文档自检 | 见 §十（坏链 + 围栏配对 / 数字指向基线 / 标识符·枚举·开关比对） |

**环境路径（本机）**：MDK `C:\Software\Keil\MDK5`（`ARM\ARMCC\bin\armcc.exe`、`UV4\UV4.exe`）；
宿主 gcc `C:\Software\CLion\bin\mingw\bin\gcc.exe`（需 `-ExtraPath C:\Software\qalculate`）。

---

## 十四、维护说明

- 本文件**随仓库演进**：只要 §三~§九 的**规则/阈值/基线**有变，**必须同步更新本文件**（它是"改内核"的唯一操作手册）。
- 三份技能文档的分工见 §开头表格；新增/重命名 Skill 时，同步根 `README.md` 的 Skill 小节与目录树。
- 提交前自查（照着打勾）：☐ 三档编译 0 告警 ☐ `run_all.ps1` ALL PASS ☐ 基线数字与"为什么变"已写进 `baseline.md`/`CHANGELOG` ☐ 文档同步（§九）☐ 文档自检（§十）☐ 问题跟踪已登记（§十一）。

---

<div align="center">

**相关**：[用户侧技能](./XCOS_Agent_Skill.md) · [MDK 仿真实测技能](./MDK_Sim_Agent_Skill.md) · [API 手册](../XCOS.md) · [配置说明](../XC_Config.md) · [架构概览](../架构概览/XCOS_V2.1.0_架构概览.md) · [回归基线](../../Tests/baseline.md)

</div>



