# 更新说明

> **这份文件记什么**：只写「**改了什么 · 为什么改 · 对使用者有什么影响**」，尽量用大白话说清；具体数值与测试口径统一放在
> [`../Tests/baseline.md`](../Tests/baseline.md)（机器可读基线 `Tests/baseline.json`），逐条的来龙去脉与实测证据见 `Docs/问题跟踪/`。
>
> **每次改动后的自测（三件事）**：① **功能** —— 20 条回归用例（通知 / 挂起 / 调度 / 诊断开关…）；② **体积** —— 编译出的固件大小有没有意外变化；
> ③ **速度** —— 关键路径耗时有没有变慢。一条命令跑完：`powershell -File Tests/run_all.ps1`。

---

## T2026/09/16
- 版本:2.1.0

**内核缺陷修复（需重新编译）**
- **空闲回调拿到"巨大休眠值"**（A17）：空闲分支改为"**锁内取值**（"就绪表是否空"与"最近唤醒时刻"同一次原子观察）+ 已过点钳位为 0"；
  修复前 `IdleTick` 会算成 `0xFFFFFFFF`（≈49.7 天）或一段无关字节（表根被当任务读）⇒ 按提示值休眠的应用会**睡过头 / 假死**
  （只写 `__WFI()` 的不受影响）；`XC_CFG_TICKS_PER_SEC` 越高越容易命中。
  **`IdleTick` 语义**：`0` = **不要休眠**（别把 0 当"睡 0 个 tick"丢给平台）、`~0U` = 无待唤醒任务、其余 = 建议休眠上限（是快照）。
- **调度更公平**：任何任务"进入就绪表"都排在**队尾**（唤醒 / 定时到点 / 复位 / 恢复 / 注册一致），回到 V2.0 的既定规则。
- **通知不再丢**：通知机制由"生产者/消费者计数对"改为**待处理标志位**（中断侧只写 1、消费侧只写 0）。
- **挂起的任务不会被误唤醒**：唤醒条件排除"已挂起"；`XC_Task_Resume()` 会把挂起期间收到的通知交给它。
- 其它：同一任务**重复注册**返回失败；`XC_Task_SetEntry` 换入口先清协程栈；`>1000 Hz` 的时间换算公式改正；
  `XC_Task_Reg()` 传空指针按统一约定处理（**参数写错 = 编程错误 ⇒ 只由断言上报**，返回码只留给"任务表满 / 重复注册 / 未挂起"）。
- **影响**：需重新编译；旧工程一般无需改代码；**无 API / 无数据结构变化**（`sizeof(TCB)` / `sizeof(XCOS_t)` 仍 44 / 44）。

**编译期防护（0 代码 / 0 数据，写错即编译报错）**：Tick 计数类型 ≥32 位；`XC_CFG_TICKS_PER_SEC ≤1000` 时必须是 1000 的因子；`XC_CFG_MAX_TASKS ∈ 1~255`。

**新增可选诊断能力（默认全关 ⇒ 发布版 0 开销）**：`XC_Err.h`（错误上报钩子）+ `XC_Diag.h` / `XC_Diag.c`（参数断言 / 运行期自检 / 每任务运行统计）；发布工程**可以不加 `XC_Diag.c`**。

**新增自测工具 `Tests/`**：一条命令跑「功能回归 / 固件体积 / 关键路径耗时」—— `powershell -File Tests/run_all.ps1`。

**文档与仓库整理**：手册分层（`Docs/XCOS.md` = 用户 API，`Docs/架构概览` = 实现细节）；标签统一 `[配置]` / `[用户]` / `[内部]`；架构图改 **ASCII**；问题跟踪归并到 `Docs/问题跟踪/`；新增《XCOS 内核开发技能》。

**评估后决定不做**：断言附带"文件 + 行号"（+152 B / +92 B）；私有宏空表保护、时间表插入 / 事件扫描优化等（实测见 `Docs/问题跟踪/修正方案/20260911_O3_唤醒任务表优化/`）。

**影响与验证**：功能 **PASS 16 / FAIL 0 / SKIP 3**（19 条用例）；体积 `default` / `stats` / `diag` = **8768 / 8920 / 9868**（RAM 0）；周期 9 项 **PASS**。

> 逐项来龙去脉、复现用例与实测证据见 **`Docs/问题跟踪/`**（问题清单 **A17** 及其修正方案）；数值口径见 `Tests/baseline.md`。

---

## T2026/08/24
- 版本:2.1.0

- 协程嵌套支持(架构详见"Docs/架构概览/XCOS_V2.1.0_架构概览.md"):
    - 新增 `XC_Cor_Enter`/`XC_Cor_Leave`/`XC_Cor_Call` 与 TCB 嵌套字段(`pCorStack`/`CorDepth`/`CorDepthMax`/`CorState`);
    - 底层新增 `COR_BreakLabel`/`COR_RetPoint` 原语,GNU `COR_Start` 增加锚点标签;
    - 调度器支持"同轮切父"(while 循环);任务复位/移除时清栈并恢复最外层入口;
- 修复 ANSI 底层缺陷:`goto` 误改为 `break` 后,`break` 在循环体内跳出的是循环而非协程 switch,
  控制流落入 `XC_Cor_Leave` 误置 `CorState=DONE`,导致调度器"同轮切父"死循环/子协程被跳过;
  已回退 `goto` 并恢复 `COR_GOTO_END` 标签(MISRA-C Rule 15.1 豁免见 `Docs/MISRA-C/MISRA-C_2025_Deviation.md`);
- 类型命名统一(遵循 `XC_<名称>_t` 规范):
    - `XCBP_t` -> `XC_BP_t`;
    - `XCListNode_t` -> `XC_ListNode_t`(struct 标签统一为 `XC_ListNode_tag`);
- 文档与示例同步更新:
    - **MISRA-C 文档独立归档**:`MISRA-C_2025_Deviation.md`、`MISRA_C_2025_Rules_Complete.md` 移至 `Docs/MISRA-C/`;历史合规检查分析草稿(v2/v3/v4)结论已全部并入 Deviation 文件后清理删除;
    - **XCOS.md 重写**:由概述升级为完整 API 参考手册(含原型/参数/返回值/说明/调用等级/向后兼容映射表),补充协程嵌套新 API(`XC_Task_RegExt`/`XC_Task_SetCorStack`/`XC_Cor_Call` 等);
    - **XC_Config.md 更新**:配置项与代码对齐(`XC_CFG_TICK_TYPE`/`XC_CFG_MAX_TASKS`/`XC_CFG_TICKS_PER_SEC`/`XC_SYS_TICK_COUNT`),补充 `XCOS_Cfg.h` 独立配置方式;
    - **ROADMAP.md 重写**:修复编码乱码,更新为 v2.1.0 完成状态;
    - **Examples.md 更新**:V2.1.0 测试覆盖说明(12任务 A0~A11,含协程嵌套与分步注册);
    - **README.md 更新**:版本徽章 v2.1.0,新增协程嵌套特性,示例代码升级新 API 命名,文件结构补充 `XC_Cor.h`/`XC_Compat.h` 与 `Docs/MISRA-C/`;
- 框架核心修复(详见对应修正方案文档):
    - **修复通知唤醒竞态 bug**
        - 问题:`XC_Task_HandleWaitNotify` 的 else 分支在 `XC_Task_HandleBlocking`(内部自加锁/解锁)后无条件置 `TaskState=BLOCKED`;
        - 场景:中断恰在其解锁窗口内 `XC_Task_SendNotify`(Lock=0 直接调度路径)唤醒任务(移入就绪表+置 READY),该赋值会将其覆盖为 BLOCKED;
        - 后果:"任务已在就绪表却报 BLOCKED"的状态不一致(任务仍会被调度运行,不崩溃,但 `XC_Task_GetState` 短暂误报);
        - 修复:`BLOCKED` 前置到 `XC_Task_HandleBlocking` 之前(先置状态再入表,中断唤醒设置的 READY 自然覆盖,保证最终状态与链表一致);
        - 验证:经 hook 插桩测试确凿复现(bug 时 BLOCKED/修复后 READY)并回归通过;
    - **锁设计说明(代码注释 + 架构概览)**
        - `HandleWaitNotify` else 分支的外层锁由 `HandleBlocking` 内部 Unlock 隐式释放,属**有意的锁范围最小化设计**;
        - 外层锁须持续到入表完成防竞态,`HandleBlocking` 自持锁只保护链表操作;
        - 已在 `XC_Task.c` 与架构概览(4.4/4.6)补充设计说明,避免后来者误改;
    - **修复"顶层 DONE 后 TaskState 保持 RUN"状态不一致**
        - 问题:有限循环/条件退出任务走到 `XC_Cor_Leave` 后,任务已在就绪表却报 RUN(直到下次调度);
        - 溯源:V2.0.0 即存在(非嵌套引入,嵌套的同轮切父不影响顶层状态);
        - 修复:`XC_Sch_Start` 在任务运行(含同轮切父)结束后,若顶层完成(`CorState==DONE` 且 `CorDepth==0`)则回落 `TaskState=READY`;
        - 方案对比:调度器单点修复 Flash 固定 +16B 且与任务数无关(Leave 宏内按深度判断的方案每"Leave 可达任务"+14B 线性增长,100 任务达 +1.4KB);
        - 验证:无限循环任务从不 DONE,运行时短路判断开销可忽略;gcc 运行验证(有限循环任务 DONE 后 READY)+ 综合/A10/边界/锁检查全部回归通过;
    - **修复"任务运行期挂起/移除就绪表后继任务导致调度游标失效"**
        - 问题:`XC_Sch_Start` 原用跨轮游标 `pIterator` 遍历就绪表,当任务运行中通过 `XC_Task_Suspend`/`XC_Task_Remove` 操作就绪表中"后继"任务(即游标位置)时,游标指向已被移走的节点;
        - 后果:挂起任务被错误调度运行(状态被覆写)、`pPrevReadyNode` 被污染为阻塞表节点、严重时访问越界崩溃(实测 `0xC0000005`);
        - 溯源:V2.0 正式版调度器重构引入(V1.0 的"索引回退防护"在重构时被删除,V2.1.0 继承);
        - 修复:调度器改为"锁内取走就绪表首节点(游离化)+ 运行后按节点自环状态判定插回就绪表尾(轮转)";跨轮游标与 `pPrevReadyNode` 字段一并移除(`XCOS_t` -4B);新增私有特化内联宏 `XC_SCH_TAKE_FIRST`/`XC_SCH_PUT_TAIL`(免函数调用);
        - 代价:完整链接实测 6496→6532(+36B),RAM -8B;
        - 验证:挂起后继不再被调度、6 项调度综合测试、嵌套/锁/状态回归全部通过;
    - **调度模型调整为"唤醒优先 + 表尾轮转"(机制说明)**:
        - 取任务:调度器每轮取就绪表**首**节点(链表 O(1)),运行完**游离任务插回就绪表尾**(公平轮转);
        - 唤醒优先:被**唤醒/通知/延时到期/复位/新注册**的任务经 `XC_Task_MoveToReadyList` 插入就绪表**首**(下轮优先运行)——将"唤醒→运行"延迟从 **O(N)**(排到队尾等待整轮)降为 **O(1)**(下一轮即运行),优化周期任务抖动与事件响应;
        - 公平性:唤醒任务运行后要么**游离回表尾**(Yield/Leave 进入轮转)、要么**非游离移出就绪表**(Delay/等待通知),不持续占表首;实测高频唤醒(1~3ms 多任务)+ 无限 Yield 持久任务并发,持久任务计数持续增长(**无饿死**);
        - 溢出处理:`XC_List_MoveListToNodeAfter`(时间表/溢出表整表移就绪表首)仅在 **Tick 回绕**(约 49 天@1000Hz)补偿时发生,一次性,实测溢出后延时任务正常唤醒、持久任务无饿死;

    - **修复"父层 XC_Cor_Call 让出后 TaskState 保持 RUN / 阻塞表僵尸"状态一致性缺陷**:
        - 问题:归位逻辑仅对 `CorState==DONE` 置 READY,父层 Call(`XC_Cor_Enter` 置 SUSPENDED)让出后漏置,任务在就绪表却报 RUN;且"无条件置 READY"会覆盖运行中被挂起(阻塞表)任务的 SUSPEND,造成"在阻塞表却报 READY"的僵尸状态(Resume 无法恢复);
        - 修复:删除"DONE 无条件置 READY",状态置位移入归位块锁内(游离任务插回就绪表时置 READY),与链表插回原子,覆盖 DONE/SUSPENDED 两种让出且不再覆盖挂起状态;
        - 验证:复现测试 Call 让出后状态 RUN(1)→READY(2);5 场景回归(延时/通知/挂起恢复/嵌套/顶层 DONE)与原内核输出一致;armcc(ANSI/GNU)/armclang 0 警告,UV4 完整工程构建 0 错误 0 警告;
    - **已知缺陷(不修复,用户决策,已注释记录)**:`EventSched` 的 `WAKEUP_NOTIFY` 未排除挂起任务——等待通知(`WAIT`)中收到异步通知(中断锁窗口 SendNotify)后被挂起的任务,会被事件调度误唤醒,破坏"挂起只能 Resume 恢复"语义;已在宏注释中记录"使用注意",需用户保证"勿对等待通知中且可能收到异步通知的任务挂起";完整修复需配套 `XC_Task_Resume` 补消费通知;

- 测试与开发工具:
    - **测试程序加入自我超时看门狗(防卡死)**:5 个仓库测试文件统一加入 `WD_Init`/`WD_Check`(C 标准 `clock()`,跨平台可编译),在 tick 线程/任务循环/步进循环中检查超时,超时打印 `[WATCHDOG]` 并强制退出(码2);实测"死循环任务"场景 2s 自动退出,不再无限占用 CPU;
    - **调度器综合测试复刻逻辑同步**:`20260821_调度器游标失效_新调度器综合测试.c` 的 `SchedStep` 复刻与当前内核一致(归位锁内置 READY,移除旧的"仅 DONE 置 READY"),S1~S6 全部通过;
    - **XCOS.xmind 思维导图更新为 V2.1.0**:55 处 V2.0 旧命名 → V2.1.0 新命名(`XCSch_*`→`XC_Sch_*`、`XCTask_*`→`XC_Task_*`、`XC_*`→`XC_Cor_*`、`XCTime_*`→`XC_Time_*`),补充嵌套 API(`XC_Task_RegExt`/`XC_Task_SetCorStack`/`XC_Cor_Call`/`XC_Cor_ClrNotify`)、协程类型(`XC_CorFn_t`/`XC_CorFrame_t`/`XC_BP_t`)、`XC_NotifyState_t`,修正"函数返回值"枚举为 `XC_Return_t`;版本宏 `XCOS_API_LEVEL`/`XCOS_VER_*` 已包含;
    - **XCOS.png 架构图更新**:由 XCOS.xmind 导出(V2.1.0),`XCOS.md` 架构概览恢复图片引用模式;
      （2026-09-15 起架构图改为 **Mermaid 文本维护**，`XCOS.xmind` / `XCOS.png` 已移除 —— 历史版本见 git）
    - **新增 AI Agent 开发技能 `Docs/Skill/XCOS_Agent_Skill.md`**:含框架概述/编程模型/API 速查/标准代码模式/约定与陷阱/文件索引/已知缺陷,示例代码经 gcc 链接真实内核编译验证(`-Wall -Wextra` 0 错误);
    - **README.md 文档入口完善**:新增"框架概览"与"Agent 开发技能"引用,目录树补齐当时的各类记录目录与 `Docs/Skill/XCOS_Agent_Skill.md`;


- 示例工程(`Examples/CortexM3_Test`)升级 V2.1.0:
    - 全部示例代码改用新 API 命名(`XC_Cor_*`/`XC_Sch_*`/`XC_Task_*`/`XC_Time_*`);
    - 新增 `Task_A10`(协程嵌套:`XC_Task_RegExt` + `XC_Cor_Call`,子层延时/让出/完成,同轮切父返回);
    - 新增 `Task_A11`(分步注册:`XC_Task_SetEntry` + `XC_Task_SetCorStack` + `XC_Task_Add`);
    - 修复配置0(无框架)链接失败:`main.c` 的 `XC_Time_TickSet(溢出测试)` 加 `#if (_Cnf_Examples > 0)` 保护;
    - 修复 `stm32f1xx_it.c` 残留旧宏名 `XCTime_TickInc` -> `XC_Time_TickInc`;
    - `main.h` 配置开关 `_Cnf_Examples` 加 `#ifndef` 保护,支持编译选项 `-D_Cnf_Examples=N` 覆盖;
    - 全示例代码经 armclang(AC6/GNU底层)与 armcc(AC5/ANSI底层)双工具链编译 + armlink 完整链接验证通过;
    - **Examples.md 资源占用分析更新为实测数据**(AC5 V5.06 -O1,与工程一致,全量重建实测):框架核心 +876B Flash +48B RAM,12任务区 +3144B Flash +744B RAM,单任务平均约262B Flash +62B RAM;
    - **消除 armcc #111-D 不可达告警(根因修复)**:`XC_Cor_Leave` 的 `CorState=DONE` 赋值移入 `do{ if(0){goto COR_DONE_L;} COR_DONE_L:; ... }while(0)` 标签结构,使赋值成为"标签后的语句",armcc(AC5)不再判为不可达;任务代码零改动(保持 `while(1)` + `XC_Cor_Leave()` 写法);经 armcc/armclang 三路径编译 0 警告,且实测 -O0~-O3 各优化级别 Flash/性能与原版完全一致(+0);
    - **修复示例 A10/A11 子协程死循环 bug**:子协程不得在 `while(1)` 循环体内写 `XC_Cor_Leave()`(`DelayMs` 的 `goto COR_GOTO_END` 会跳到循环体内标签,导致无限循环、`fTask` 永不返回调度器、其余任务饿死),已改为顺序执行主体后自然 `Leave`(与架构概览 10.2 子协程写法一致);gcc 真实运行验证 `CallCount==SubRunCount==ParentAfterCount`;
    - 移除示例中临时的 `XC_LOOP_BREAK()` 宏(由框架层 Leave 修复取代,任务代码恢复纯 `while(1)` 写法);
    - MISRA-C Deviation DEV-XCOS-001 补充 `XC_Cor_Leave` 的 `if(0) goto`(常量假分支,永不执行,与协程底层 goto 同类,一并豁免);

---

## T2026/06/26
- 版本:2.1.0

- 修改文件编码,统一到utf-8;
- 修正返回类型拼写错误(`XC_Retuen_t`->`XC_Return_t`);
- "`_XCOS_`"版本兼容标识修改为"`XCOS_API_LEVEL`";
- 二进制宏"`_B*`"改为"`BIN_*`";

## 函数重命名
- 原协程相关命名修改:`XC_<功能>` --> `XC_Cor_<功能>`;
- 所有模块前缀添加`_`分割符:
    - `XCSch_`->`XC_Sch_`
    - `XCTask_`->`XC_Task_`
    - `XCTime_`->`XC_Time_`
    - `XCList_`->`XC_List_`
- 将协程块宏从`XC_Task.h`迁移至新文件`XC_Cor.h`,保留向后兼容宏;
- 向后兼容:所有旧名称均通过`#define`别名指向新名称;
- 影响范围:全部`.c`和`.h`文件,约150处符号修改;

## [MISRA-C:2025](./MISRA-C/MISRA_C_2025_Rules_Complete.md) 合规修正:

- Rule 1.3 - 未定义行为
- Rule 7.2 - unsigned 常量加 U 后缀
- Rule 5.10 / Rule 20.15 - 保留标识符禁令
- Rule 10.1 — 操作数基本类型不当
- Rule 11.3 - 跨类型指针强转
- Rule 13.2 - 表达式值和副作用应在所有求值顺序下保持一致
- Rule 14.3 - 不变控制表达式
- Rule 15.1 (ANSI 版) - 不使用goto
- Rule 15.6 - 迭代/选择语句体应为复合语句
- Rule 20.7 - 宏参数展开应恰当定界

### Deviation说明
见 [MISRA-C_2025_Deviation](./MISRA-C/MISRA-C_2025_Deviation.md)

---

## T2026/01/07
- 版本:2.0.0

### 修改
- 将所有注释改为"doxdocgen"形式;
- 重命名函数,宏,类型,使之标准化;
- 用户头文件中隐藏内部实现代码;
- 优化代码和内存占用,优化运行性能;
- 清除文件修改日志,以后都在此文件说明;
- 优化"通知唤醒"模块,使线程安全;

### 增加
- 增加"clang-format"格式化配置文件".clang-format";
- 增加系统嘀嗒计数的全局变量,默认使用中断计数模式;
- 增加配置宏"XCOS_CFG",和独立配置文件"XCOS_Cfg.h";

### 删除
- 保证内核纯净,将不使用或者其他通用库的文件删除,并删除所有兼容老版本的宏;

### 单独模块说明
- 信号量(XC_Sem)
    - 删除全部,任务通知可实现类似信号量的处理;
- 时间处理("XC_TimeCount"和"XC_Time")
    - 删除所有日历相关的代码;
    - 将所有代码从"XC_TimeCount"移动到"XC_Time";
    - 删除所有不常用的函数和宏,余下函数及宏见"XCOS.md"文件说明;
- 链表操作(XC_List)
    - 删除"XCListNode_t"链表类型中所属链表(pRootList)字段;
    - 删除"XCListBasic_t"类型;
    - 删除"XCListRoot_t"类型用"XCListNode_t"类型替代,根节点只是标记的普通节点,不在做特殊化;
    - 函数和宏做适配于此框架的优化,并删除不使用的通用函数;
    - 链表操作函数不能被外部调用;
- 调度及任务处理("XC_Sch"和"XC_Task")
    - 优化调度处理,增加"通知唤醒"可中断中调用处理;
    - 将任务调度(延时或者阻塞)切分到每个处理函数中,不做统一处理(这样优化性能和空间);
    - "通知唤醒"中断安全需要遍历链表,以优化代码,使遍历影响降到最低,触发概率约为:0.4%(双任务定时器随机中断得到);

### 注:
**V2.0.0** 版本改动略大,除了核心链表调度没有改变,其他处理都已经重构,包括函数和变量名称;
所以此版本可以当作独立的初始版本使用,从1.0上删除/修改/增加的函数(宏/变量等)不做独立说明;

---

## T2025/06/10
- 修改"XC_TaskBasicInit"函数,删除任务状态和形参的初始化;
- 修改"XCSch_TaskReg","XCSch_TaskRemove","XCSch_TaskReset"以适配新的"XC_TaskBasicInit"函数;
- 增加函数:
    - "void XCSch_SetTaskEntryPoint(XCTCB_t* phTCB, void(\*fTask)(XCTCB_t\*), void* pParam)" 设置任务入口;
    - "int32_t XCSch_AddTask(XCOS_t* phXCOS, XCTCB_t* phTCB)" 添加任务;
    两个函数配合调用,先"设置任务入口"在"添加任务",可以理解为"XCSch_TaskReg"的分步运行;

---

## T2024/12/19
- 完善说明文件,增加"XC_BitFlag"函数说明;

---

## T2024/12/06
- 更新说明文件;
- 修正部分代码注释;
- 增加"examples"例程说明;
#### XC_Task.h
- 增加函数:

| 函数名 | 说明 |
| --- | --- |
| XC_GetParam | 协程块内获取任务注册时传递的参数(指针); |
| XC_GetParamUint | 协程块内获取任务注册时传递的参数(32位无符号); |
| XC_Reset | 协程块内获复位自身; |
| XC_GetParam | 协程块内获取任务注册时传递的参数(指针); |
| XC_GetParamUint | 协程块内获取任务注册时传递的参数(32位无符号); |
| XC_Suspend | 协程块内挂起自身; |

- 将"XCSem_BinSemTake"和"XCSem_BinSemTake_ms"函数宏移动到"XC_Sem.h"文件中;

#### XC_Sem.h
- 增加从"XC_Sem.h"移来的函数宏"XCSem_BinSemTake"和"XCSem_BinSemTake_ms";

---

## T2024/08/28
#### XC_Sch
- 修改"XCSch_TaskReset"函数,任务复位不复位传递参数"Param";
#### XC_Time
- 修正"XCTime_GetRemain"函数,获取滴答计数变为ms的问题;
- 增加函数宏"XCTime_TimerGetElapsedMs"获取运行了多少ms;

---

## T2024/08/12
#### XC_Time
- 将原"XCAPI"中的Time处理(时间戳,日历,秒转换计算处理)转移至"XC_Time"中;
- "XC_Time.h"中编译相关宏独立到"XC_TimeCompile.h";
- "XC_Time.h"中系统时间计数相关宏独立到"XC_TimeCount.h";
- 将"XC_Time"中所有兼容性代码移至"XC_TimeCompatibility.h";
- 将"XCTime_CheckTimeout_t"重命名"XCTime_CheckTimeoutt",防止认为是变量类型;

---

## T2024/03/18
- 初始编写;
