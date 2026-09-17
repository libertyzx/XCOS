# Tests —— 仓库内回归 / 体积 / 周期基线

> **一句话**：`Tests/` 是**自测工具** —— 改完代码跑一条命令，它会告诉你三件事：**功能有没有被改坏 / 固件大小有没有意外变化 / 跑得有没有变慢**。
> 本文是用法说明（怎么跑、怎么加用例、怎么更新基准值）；文中术语（marker / snapshot / 通道 / 阈值）在下面各节解释。

> 目标：**一条命令**得到「功能 0 失败 + 体积不漂移（±N 字节）」，把"每次改动靠人工敲命令 + 手工量体积"变成可复现的固定动作。

## 1. 怎么跑

```powershell
# 全部（功能 + 体积 + 周期）
powershell -File Tests/run_all.ps1
powershell -File Tests/run_all.ps1 -SkipCycles            # 跳过周期通道(无 MDK/嫌慢)

# 只跑功能回归
powershell -File Tests/run_tests.ps1
powershell -File Tests/run_tests.ps1 -Filter B4          # 按名字过滤(正则)

# 只跑体积通道
powershell -File Tests/run_size.ps1                      # 与 Tests/baseline.json 对比
powershell -File Tests/run_size.ps1 -SkipImage           # 跳过 MDK 镜像级

# 只跑周期通道（µVision 模拟器；约 10~25 s）
powershell -File Tests/run_cycles.ps1                    # 与 baseline.json 的 cycles 段对比(±5%)
powershell -File Tests/run_cycles.ps1 -WriteBaseline     # 重建周期基线

# 只跑"让出往返"补充测点（热路径：自环 / 非自环 两条让出路径）
powershell -File Tests/run_yield.ps1
powershell -File Tests/run_yield.ps1 -WriteBaseline      # 只合并 yield/delay 两个键

# 表格排版自检（代码注释 / 文档里的 Markdown 表格是否"源码对齐"）
powershell -File Tests/check_tables.ps1
powershell -File Tests/check_tables.ps1 -Verbose           # 逐块列出宽度与跳过说明


# 怀疑环境不同 / 工具链不在默认路径
powershell -File Tests/run_all.ps1 -Gcc "C:\Software\CLion\bin\mingw\bin\gcc.exe" -Mdk "C:\Software\Keil\MDK5"
```

退出码：`0` 全通过；`1` 有失败；`2` 找不到工具链（宿主通道）。

## 2. 通道与判定

| 通道                   | 脚本               | 判定方式                                                                                         | 说明                                                                                                          |
| ---------------------- | ------------------ | ------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------- |
| 功能回归               | `run_tests.ps1`    | **marker**（输出正则，如 `failures 0`）/ **snapshot**（输出与 `snapshots/*.txt` 逐行一致）/ 超时 | 单条 30s 超时（防止挂死）                                                                                     |
| 体积·对象级           | `run_size.ps1`     | 与 `baseline.json` 比，阈值 **±0 B**                                                            | armcc AC5 `-O1 --cpu=Cortex-M3` + fromelf                                                                     |
| 体积·镜像级           | `run_size.ps1`     | 与 `baseline.json` 比，阈值 **±8 B**                                                            | 镜像 `Code/Examples` 到临时目录后用 `UV4 -b` 全量构建                                                         |
| 周期（关键路径）       | `run_cycles.ps1`   | 与 `baseline.json` 的 `cycles` 段比，阈值 **±5%**                                               | µVision 模拟器 + SysTick 计数（技能文档 §4.1 口径）；测点 `Tests/bench/`                                    |
| 周期·让出往返（补充） | `run_yield.ps1`    | 同上（`cycles` 段），阈值 **±5%**                                                               | 让出路径往返周期：`yield_roundtrip`（自环）/ `delay_roundtrip`（非自环）；测点 `Tests/bench/xc_yield_bench.c` |
| 表格对齐（静态检查）   | `check_tables.ps1` | 每张表**各行显示宽度必须相等**（宽度口径：ASCII = 1 列 / 中文·全角 = 2 列）                     | 代码注释不渲染 Markdown ⇒ 表格须**源码对齐**；`-Verbose` 逐块列出（含被跳过的"单元格内含未转义 `\|`"行）     |

* 基线数值与口径说明：见 [`baseline.md`](./baseline.md)（机器口径 = `baseline.json`）。
* 运行产物（exe/日志/镜像工作区）全部写在 `%TEMP%\xcos_tests_*` / `%TEMP%\xcos_size_*`，**不污染仓库**。

## 3. 环境要求

| 项             | 默认值                                                                                | 可否覆盖           |
| -------------- | ------------------------------------------------------------------------------------- | ------------------ |
| 宿主编译器     | `C:\Software\CLion\bin\mingw\bin\gcc.exe`（其次 `C:\mingw64`、`C:\msys64`、PATH）     | `-Gcc <path>`      |
| gcc 运行库目录 | `C:\Software\qalculate`（本机 mingw 缺 `libgmp/libmpfr/zlib`，不加会 `cc1` 启动失败） | `-ExtraPath <dir>` |
| MDK 根目录     | `C:\Software\Keil\MDK5`（`ARM\ARMCC\bin\armcc.exe`、`UV4\UV4.exe`）                   | `-Mdk <path>`      |

> 找不到工具链时脚本**跳过该通道并给出提示**（不算失败），便于在没装 MDK 的机器上只跑功能回归。

## 4. 怎么加一个用例

1. 把 `.c` 放到对应专题目录 `Docs/问题跟踪/修正方案/<专题>/测试/`（**不要**复制到 `Tests/`，避免与文档里的相对链接重复）；
2. 在 `Tests/manifest.txt` 追加一行（TAB 分隔）：
   `名字 ⇥ marker|snapshot|skip ⇥ 用例路径 ⇥ 编译开关 ⇥ 判定(marker 正则 或 快照文件名) ⇥ 快照文件名 ⇥ 备注`
3. 快照类：先 `powershell -File Tests/run_tests.ps1 -UpdateSnapshots` 生成 `Tests/snapshots/<名>.txt`，**人工确认内容正确**后再提交；
4. 若用例输出含**时间/线程相关数值**（每次不同）⇒ 别用快照，改用 **marker**（用稳定不变量的正则），例如 `S1-状态一致性与挂起唤醒`。

## 5. 当前跳过项（3 条）

| 用例                                                                       | 原因                                                       |
| -------------------------------------------------------------------------- | ---------------------------------------------------------- |
| `20260914_A1_新旧代码对照_sim.c`、`20260914_通知待处理标志_回归测试_sim.c` | 需外部 tick 驱动（本机运行超时）⇒ 走仿真/带 tick 源的方式 |
| `20260914_A2_挂起优先级与通知交付_仿真测点.c`                              | MDK 仿真测点（bench），走仿真通道                          |

> **副本类用例（如 `S4-新调度器综合测试(V2副本)`）**：它把调度器逻辑**复刻**进测试（不调用 `XC_Sch_Start`），因此**没有线程/中断依赖、完全确定**；
> 但**必须与 `Code/Src/XC_Sch.c` 同步**（文件头写有"最后同步日期 + 对应的内核语义"）—— 不同步会"能编译但结论失真"。

## 6. 设计备注（**本工具自身**的注意，只在这里维护）

> `Tests/` 是**本版本新增的自测工具**；下面这些是**工具开发过程中的坑与约定** —— 属于正常的开发活动，**只写在本文（与 [`baseline.md`](./baseline.md)）**，
> 不进 `Docs/问题跟踪/`（问题清单只管框架自身的代码 / 配置缺陷）。

* **脚本为什么是纯 ASCII**：Windows PowerShell 5.1 会把"无 BOM 的 UTF-8"脚本按 ANSI 解码 ⇒ 脚本内的中文串会被破坏。因此中文只放在 `manifest.txt` / `baseline.md` / 本 README（脚本用显式 UTF-8 读取它们）。
* **为什么用 `Start-Process` 而不是管道调外部程序**：`$ErrorActionPreference='Stop'` 下，外部程序写 stderr 会被当作终止错误；改用 `Start-Process` + 文件重定向后行为稳定。
* **`-WriteBaseline` 的合并语义（勿再"整文件覆盖"）**：`run_size.ps1 -WriteBaseline` 只更新 `object`/`image` 段、**保留其它段**（如 `cycles`）；`run_cycles.ps1 -WriteBaseline` 同理只更新 `cycles`。两者都是"**读旧 json → 改本段 → 写回**"，避免互相删段（曾因覆盖写入导致周期通道报 `baseline has no "cycles" section`，2026-09-15 修复）。
* **诊断开关矩阵**：诊断开关(错误上报/自检/统计)的"关/开/全开"组合已固化进 manifest（8 条 marker 用例），保证"开关关闭时行为与体积都不变"这一点**每次都被验证**。
* **错误上报 vs 诊断能力（两个头，分层）**：`Code/Inc/XC_Err.h` = **错误上报设施**（错误码 `XC_ErrCode_t` + 用户钩子 `XC_Err_Hook` + 上报宏 `XC_Err_Report`，**纯声明无实现文件**）；`Code/Inc/XC_Diag.h`(+`Code/Src/XC_Diag.c`) = **诊断能力**（用户/调试可见：自检 + 统计查询；实现侧断言宏 `XC_DIAG_ASSERT` 与调度槽埋点声明在 `Code/Inc/Internal/XC_DiagInternal.h`）。只做上报的模块（如用户自己的驱动层）**只需包含 `XC_Err.h`**。
* **诊断模块（`XC_Diag.h` / `Internal/XC_DiagInternal.h` + `XC_Diag.c`）**：断言/自检/统计的实现都集中在诊断模块；内核侧只留**埋点调用**（`XC_Task.c` 帧栈越界上报与统计清零、`XC_Sch.c` 调度槽计时两端）。
  发布工程**不添加 `XC_Diag.c`** 即可物理裁掉全部诊断实现（要求诊断开关全为 0；开关为 1 却未加入该文件 ⇒ 链接报错，属预期）。新增诊断用例时，若用到诊断 API，记得在 `manifest.txt` 的 `SRC` 里已有 `Code\Src\XC_Diag.c`（E1 起已内置）。
* **周期通道（`run_cycles.ps1` + `Tests/bench/`）**：按 `Docs/Skill/MDK_Sim_Agent_Skill.md` 的 7 步流程自动化——
  镜像工作区 → 用 `Tests/bench/xc_cycles_bench.c` **替换镜像的 `main.c`** → 把 `.uvoptx` 的 `<sIfile>` 指向 `Tests/bench/cycles.ini`
  （模板里的 `__SIM_LOG__` 占位符由脚本替换为绝对日志路径）→ **清 `Debug/`** 后 `uVision.com -b` 构建 →
  `UV4 -d -j0` 跑仿真（**轮询日志出现 `===== END =====` 提前退出**，超时则 Kill；会话未起会**清进程重试一次**）→ 解析 `r<idx> <name> = <cycles>`。
  **口径**：单次调用周期数，已扣除 `cyc_overhead`（计时自身开销，基线里亦记录）；测点只调用**线程模式可用**的 API（不启动调度器、不依赖外设/中断）⇒ 完全确定。
  **扩测点**：在 `xc_cycles_bench.c` 写 `g_res[n]` + `cycles.ini` 加一行 `printf` → `run_cycles.ps1 -WriteBaseline` 重建基线。
* **顺序/时序类用例的写法**：内核的"入表=排队尾"是**严格轮转**，用例**不要**假设"某个任务下一轮一定先跑"；需要等待某任务跑到某状态时，请写成"**让出直到条件成立（带上限）**"（如"让出直到条件成立"的循环），而不是固定 `Yield` 次数。
  同理，用例里跨让出点**不能依赖局部变量**（用静态/全局）。
* **编译器可能"无法运行"**：宿主通道启动时会做一次**冒烟编译**；失败会提示 `compiler cannot run` 并打印退出码（十六进制）。最常见原因是 `cc1.exe` 找不到运行库：
  ① 工具链**自己的 bin 目录**不在 `PATH`（缺 `libssp-0.dll` / `libgcc_s_*.dll`）⇒ 退出码 **`0xC0000135`**、**且没有任何输出**（`gcc --version` 却正常，极易误判为"代码坏了"）；
  ② 本机 mingw 缺 `libgmp/libmpfr/zlib` ⇒ 需要 `-ExtraPath`。
  脚本现已**自愈**：启动时自动把 **gcc 所在目录** 与 `-ExtraPath` 前置到 `PATH`；手工排查可直接跑 `<mingw>\libexec\gcc\...\cc1.exe --version`，退出码 `-1073741515`（`0xC0000135`）即是此因。
* **周期通道的仿真日志会被"占用"**：`UV4` 停止后，**写日志的进程可能仍持有 `sim_log.txt`**（`Stop-Process` 不保证立刻释放句柄）⇒ 直接读会抛"文件正由另一进程使用"，让通道**假失败**
  （`run_all` 里"体积 → 周期"连着跑时必现，单跑该通道常常侥幸通过）。脚本现用 `FileShare.ReadWrite` **共享读 + 最多 5 次重试**（每次 400 ms）。
* **`run_yield.ps1` 与 `run_cycles.ps1 -WriteBaseline` 的**顺序**：后者会**重建整个 `cycles` 段**（把 `yield_roundtrip` / `delay_roundtrip` 挤掉）⇒
  重建基线时**先跑 `run_cycles.ps1 -WriteBaseline`，再跑 `run_yield.ps1 -WriteBaseline`**（后者只**合并**那两个键，不动其它键）；`run_all.ps1` 已按此顺序串好（channel 3 → 3b）。
* 后续增强（**唯一未做项**）：**接入 CI**（本仓库当前以本地脚本为准）。—— 早先计划的"④ 周期通道脚本化"**已交付**：`Tests/run_cycles.ps1` + `Tests/bench/`（7 步仿真流程 + 日志解析 + `-WriteBaseline`）。
* **几条用例自带 1~2 条编译告警（不是回归、不影响判定）**：任务体里**没有任何挂起点**的用例会触发内核头 `Code/Inc/Internal/XC_CorGNU.h` 的
  `label 'COR_GOTO_END' defined but not used`（`-Wunused-label`：当前 `B1-关闭` / `B1-开启` / `B1-断言` / `B3-开启` / `S1` 各 1 条、`S3` 2 条，共 6 条）——
  该标签是"挂起/结束跳转"的目标，用例里没挂起时必然未被引用；内核与示例工程按严格档（`-Wall -Wextra -Wpedantic …`）编译时 **0 告警**，故不再为用例单独消音。
