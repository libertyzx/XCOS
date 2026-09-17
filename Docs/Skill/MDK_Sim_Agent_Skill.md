---
name: mdk-uvision-sim
description: 用 MDK µVision 内置模拟器（SARMCM3 等 Cortex-M 核心模型）做**无界面、可脚本化、可复现**的嵌入式代码实测：构建、跑调试会话、在模拟器里量周期数/体积/RAM。适用于在无硬件、或无宿主编译器（gcc/clang）的环境下，用 Keil 工具链 + 调试脚本(.ini)得到真实指令级测量数据。适用平台：Windows + Keil MDK5。
---

# MDK µVision 模拟器实测技能（Agent Skill）

> 面向 **AI 编码代理**：当需要"跑起来量数据"而手上只有 Keil MDK（没有 gcc/硬件）时，按本技能操作。
> 核心结论：**µVision 支持无界面跑模拟器**（`UV4 -d ... -j0` + 调试初始化文件），可全程用命令行 + 脚本驱动，结果落到日志文件。

---

## 一、适用场景与前提

| 项       | 要求                                                                                                                                                            |
| -------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 工具     | Keil MDK5（本文以 `C:\Software\Keil\MDK5` 为例），含 `UV4\UV4.exe`、`UV4\uVision.com`、`ARM\ARMCLANG\bin\{armclang,armlink,fromelf}.exe`、`ARM\BIN\SarmCM3.dll` |
| 器件包   | 目标器件对应的 DFP 必须已安装，例如 `C:\Software\Keil\Arm\Packs\Keil\STM32F1xx_DFP`                                                                             |
| 工程     | 一个 µVision 工程（`.uvprojx` + `.uvoptx`），且调试目标选 **Use Simulator**（见 §2.1）                                                                        |
| 典型用途 | ① 两个实现方案的**周期数对比**；② **Flash/RAM 增量**实测；③ 时序/临界区时长实测；④ 无硬件时的功能验证                                                       |

> **不要**用 `UV4.exe -h`（它会拉起 GUI 并挂住）；命令行开关见 <UV4 目录>\uv4.chm 的 *Command Line* 一章，可用 `hh.exe -decompile <dir> uv4.chm` 解出 HTML 查阅。

---

## 二、无界面仿真流程（7 步）

### 2.0 关键开关速查

| 开关                         | 作用                                                             |
| ---------------------------- | ---------------------------------------------------------------- |
| `UV4 -b <proj> -j0 -o <log>` | 无界面构建（`-j0` 隐藏 GUI；`-o` 写日志）                        |
| `UV4 -r <proj>` / `-cr`      | 重新翻译 / 清理后重建                                            |
| `uVision.com -b ...`         | 控制台版，**进度会打到 stdout**（比 UV4.exe 更好捕获）           |
| `UV4 -d <proj> -j0`          | **无界面进入调试（仿真）会话**；配合调试初始化文件执行自动化测试 |
| `-x` / `-y`                  | 仅与 `-d` 搭配的 DDE 输出模式（返回完整/仅确认输出）             |
| `ERRORLEVEL`                 | 0 无错；1 有告警；2 有错误；3 致命；11~20 工程/导入类错误        |

### 2.1 确认工程是"仿真器 + 允许脚本接管"

* `.uvprojx`：`<TargetDllName>SARMCM3.DLL</TargetDllName>`（其他核心对应各自 DLL）、`<UseTargetDll>1</UseTargetDll>`；
* `.uvoptx`：`<uSim>1</uSim>`（Use Simulator）、`<sGomain>0</sGomain>`（**关掉 Run to main**，让 .ini 全权控制）、`<sIfile>…\sim.ini</sIfile>`（**仿真器调试初始化文件**；硬件目标用 `<tIfile>`）。

### 2.2 临时工作区（**必须镜像相对路径**）

`.uvprojx` 里的源文件/头文件路径都是**相对工程目录**的（如 `..\..\Code\Src\XC_Task.c`）。
把工程复制到临时目录时，必须同时把被引用的目录放到同一相对位置，否则报 `cannot open source input file`。

```powershell
$S = Join-Path $env:TEMP 'sim_ws'
New-Item -ItemType Directory -Force -Path "$S\Examples\MyProj" | Out-Null   # 放工程
Copy-Item -Recurse -Force '<repo>\Examples\MyProj\*' "$S\Examples\MyProj\"
Copy-Item -Recurse -Force '<repo>\Code' "$S\Code"                            # 满足 ..\..\Code
# 切换被测版本：改动只发生在 $S\Code，仓库不受影响
```

### 2.3 写测点程序（bench）

**替换一个已被工程引用的源文件**（例如工程的 `main.c`）最省事——不用改 `.uvprojx` 的文件列表。
测点程序要点见 §4（计时）与 §5（对比规范）。

### 2.4 写调试脚本 `.ini`（详见 §3）

### 2.5 构建（**先清输出目录，否则增量构建会跳过**）

```powershell
Remove-Item -Recurse -Force "$S\Examples\MyProj\Debug" -ErrorAction SilentlyContinue
cmd /c "`"C:\Software\Keil\MDK5\UV4\uVision.com`" -b `"$proj`" -o `"$S\build.log`" > `"$S\console.txt`" 2>&1"
Get-Content "$S\build.log" | Where-Object { $_ -match 'Program Size|Error\(s\)' }
```
`Program Size: Code=… RO-data=… RW-data=… ZI-data=…` 即为 **Flash/RAM 实测**（含链接器死代码消除）。

### 2.6 跑仿真（**超时回收 + 读日志**）

```powershell
Get-Process UV4 -ErrorAction SilentlyContinue | Stop-Process -Force   # 清残进程（否则第二次启动会失败）
Remove-Item -Force "$S\sim_log.txt" -ErrorAction SilentlyContinue
$p = Start-Process -FilePath 'C:\Software\Keil\MDK5\UV4\UV4.exe' `
                   -ArgumentList @('-d', $proj, '-j0') -PassThru
Start-Sleep -Seconds 20
if(-not $p.HasExited){ $p.Kill() }                                    # 脚本里的 EXIT 在调试脚本中不生效
Get-Content "$S\sim_log.txt" | Select-String -Pattern '^r\d+ '
```

### 2.7 收尾

* 结果文件：`sim_log.txt`（`printf` + `D` 内存转储）、`build.log`（体积）、`console.txt`；
* 若做两版对比，**同一份 bench、同一 `.ini`** 各跑一遍；
---

## 三、调试脚本 `.ini`（仿真器初始化文件）

### 3.1 语言与常用命令

* 脚本语言是 **C 风格**：可写 `FUNC void f(void){…}`、表达式、`printf`、`while`；也支持 `signal` 函数（模拟硬件行为）。
* 会话进入顺序：**加载镜像 → 恢复调试设置 → 执行 `.ini` → 运行到 main()**（若 `sGomain=1`，所以做自动化时把它关掉）。

| 命令                    | 用途                                     | 示例                      |
| ----------------------- | ---------------------------------------- | ------------------------- |
| `LOG >path` / `LOG OFF` | 把 Command 窗口输出（含 `printf`）写文件 | `LOG >C:\tmp\sim_log.txt` |
| `printf("…%u…", x)`   | 打印（配合 LOG 落盘）                    | 见模板                    |
| `g, <symbol>`           | 运行到某符号（等效临时断点）             | `g, SimDone`              |
| `BS <symbol>` / `G`     | 设断点 / 从当前 PC 运行                  | `BS SimDone` + `G`        |
| `D <start>,<end>`       | 内存显示（Command 窗口）                 | `D &g_res, &g_res+63`     |
| `SAVE file, start, end` | 内存转存到文件（二进制/hex）             | 备用取证手段              |
| `INCLUDE file.ini`      | 手动执行另一个脚本                       | 调试期临时用              |
| `ws <expr>`             | 加入 watch 窗口                          | `ws g_res[0]`             |
| `$ = <addr>`            | 改 PC                                    | `$ = 0x08000100`          |
| `twatch(<ticks>)`       | 延时若干时钟（signal 函数内用）          | 见 Keil 文档              |

> ⚠️ **`EXIT` 在调试脚本里不生效**（只能用于 Command 窗口）。因此驱动脚本用"**超时 + Kill 进程 + 读 LOG 文件**"的方式收尾。

### 3.2 模板（可直接复制）

```c
/* sim.ini —— 仿真批处理脚本(由 .uvoptx 的 <sIfile> 指定) */
LOG >C:\tmp\sim_log.txt                 /* 一进来就开日志 */

FUNC void DumpRes(void) {               /* 用 ASCII 标签, 中文在日志里可能乱码 */
    printf("\r\n===== SIM RESULT =====\r\n");
    printf("r0  opA cycles = %u\r\n", g_res[0]);
    printf("r1  opB cycles = %u\r\n", g_res[1]);
    printf("r15 timer overhead = %u\r\n", g_res[15]);
    printf("r13/r14 sizeof = %u %u\r\n", g_res[13], g_res[14]);
    printf("===== END =====\r\n");
}

g, SimDone                              /* 跑到测点程序里的空函数 SimDone() */
DumpRes()                               /* 打印结果 */
D &g_res, &g_res+63                     /* 内存转储兜底(printf 失败时还能取证) */
```

### 3.3 输出与编码

* `printf` 内容进入 Command 窗口 → 被 `LOG` 捕获；若不加 `LOG`，输出只在 GUI 里；
* **日志里避免中文标签**（脚本/日志编码不一致会乱码），用 ASCII；
* 用 `read_files` 或 .NET UTF-8 读结果文件。

---

## 四、计时与测量的正确姿势

### 4.1 用 SysTick 当周期计数器（核内外设，模拟器建模）

```c
#define CYC_LOAD 0x00FFFFFFU
static void CycInit(void) { SysTick->LOAD = CYC_LOAD; SysTick->VAL = 0U; SysTick->CTRL = 5U; /* EN|CPU */ }
static uint32_t CycGet(void) { return (CYC_LOAD - SysTick->VAL); }
```

* **必须校准计时自身开销**：连续两次取值的差就是"每次测量附加开销"（本次 AC5 -O1 实测 **12 周期**），从每个结果里扣掉；
* 被测区间内**不要**出现浮点/库调用（会引入不确定开销）；
* 结果写进 `volatile uint32_t g_res[N]`，由 `.ini` 打印；
* `DWT->CYCCNT` 是否被建模**未验证**，优先 SysTick。

### 4.2 别忘了"成本归属"

比较两版时，先问清"**这笔时间由谁付**"：
* 若 A 版在**调用者**里就把活干完（比如直接搬表），而 B 版把活**推到主循环**，那么"发送函数"的耗时不能直接对比——必须比**端到端**（发送 + 落地）。
* 本次就因漏掉这一点，连续两次算错结论；**正确做法：在测点里同时给出"发送"与"发送+落地"两个总时长**。

### 4.3 需要"中断上下文"时

* 直接调用 API 属于 **线程模式**；若被测路径区分上下文（如 `__get_IPSR()`），需让代码真正跑在 handler 模式：可用 **NVIC 软件触发中断**（`NVIC->STIR = IRQn` 或 CMSIS `__NVIC_SetPendingIRQ(IRQn)`），在对应 `*_IRQHandler` 里调用被测函数（**思路，未验证**）；
* 或者把"上下文判定"做成可配置宏，测量时强制成两个变体。

---

## 五、方案对比的规范（保证公平、可复现）

| #   | 规范                                                         | 原因                                                           |
| --- | ------------------------------------------------------------ | -------------------------------------------------------------- |
| 1   | **同一编译器 + 同一优化级别**（记录 AC5/AC6、`-O0/-O1/-O2`） | 优化级别会让指令数差 2 倍以上（本次 -O1 未内联导致测量被放大） |
| 2   | **两版测点代码等长**：差异用 `#ifdef` 对称分支               | 否则 `Code` 体积差被 bench 差异污染（本次 +52B 的干扰）        |
| 3   | 测点**强制引用所有被测 API**                                 | 否则链接器做死代码消除，体积不可比（或声明只比"存活部分"）     |
| 4   | 关键路径**用宏/inline 的代码风格保持一致**                   | "-O1 下函数 vs 宏"的差异能到 20~40 周期，会盖过设计本身的差异  |
| 5   | 报出**原始数据 + 口径**，标注估算/实测                       | 便于他人复核（本次报告分"实测/估算"两类）                      |
| 6   | 每轮**删 `Debug/` 目录**再构建                               | 增量构建会跳过（`.o` 比 `.c` 新时），拿到旧二进制 = 数据作废   |

---

## 六、驱动脚本模板（PowerShell 全流程）

```powershell
param(
    [string]$Proj  = "$env:TEMP\sim_ws\Examples\MyProj\MyProj.uvprojx",
    [string]$Work  = "$env:TEMP\sim_ws",
    [int]   $Wait  = 20,
    [string[]]$Variant = @('A','B')          # 要对比的源码目录名(位于 $Work\_src)
)
$UV = 'C:\Software\Keil\MDK5\UV4'
foreach($v in $Variant){
    Get-Process UV4 -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1
    Remove-Item -Recurse -Force "$Work\Code" -ErrorAction SilentlyContinue
    Copy-Item -Recurse -Force "$Work\_src\$v" "$Work\Code"          # 切换被测版本
    Remove-Item -Recurse -Force (Split-Path $Proj)\Debug -ErrorAction SilentlyContinue

    # 1) 构建
    cmd /c "`"$UV\uVision.com`" -b `"$Proj`" -o `"$Work\build_$v.log`" > `"$Work\console_$v.txt`" 2>&1"
    "===== $v =====" | Write-Host
    (Get-Content "$Work\build_$v.log" | Where-Object { $_ -match 'Program Size|Error\(s\)' }) -join ' | '

    # 2) 跑仿真
    Remove-Item -Force "$Work\sim_log.txt" -ErrorAction SilentlyContinue
    $p = Start-Process -FilePath "$UV\UV4.exe" -ArgumentList @('-d', $Proj, '-j0') -PassThru
    Start-Sleep -Seconds $Wait
    if(-not $p.HasExited){ $p.Kill() }

    # 3) 取结果
    if(Test-Path "$Work\sim_log.txt"){
        Get-Content "$Work\sim_log.txt" | Select-String -Pattern '^r\d+ ' | ForEach-Object { $_.Line }
    } else { Write-Host '(sim_log missing — 检查 .ini 的 LOG 路径 / 是否构建成功)' }
}
```

---

## 七、陷阱清单（逐条都是实战踩过的）

| #   | 陷阱                                 | 现象/后果                                                                                                                                        | 对策                                                                                                                                                                                       |
| --- | ------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 1   | 用 `UV4.exe -h` 查帮助               | 拉起 GUI 并**挂住**                                                                                                                              | 查 `uv4.chm`：`hh.exe -decompile <dir> uv4.chm` 后读 HTML                                                                                                                                  |
| 2   | 增量构建                             | 改了源码却"没生效"、`Build Time 00:00:00`                                                                                                        | 每轮**删 `Debug/`** 再构建                                                                                                                                                                 |
| 3   | `.o` 比 `.c` 新                      | 新加的函数/宏没进镜像（`undefined symbol`、`ZI` 未变）                                                                                           | 同上；并用**编译期断言**（`sizeof`）复核结构体是否真的改了                                                                                                                                 |
| 4   | `.uvprojx` 相对路径                  | `cannot open source input file`                                                                                                                  | 复制工程时**镜像目录结构**（`..\..\Code` 要真的存在）                                                                                                                                      |
| 5   | PowerShell 默认 ANSI 读写            | 中文注释乱码 → 之后中文锚点匹配失败                                                                                                             | 一律用 `[System.IO.File]::ReadAllText/WriteAllText($f, $enc)`，`$enc = New-Object System.Text.UTF8Encoding($false)`                                                                        |
| 6   | 用 PowerShell 正则替换"大段多行文本" | 转义地狱，**把源码写坏**                                                                                                                         | 用精确匹配的编辑工具/here-string；**改完立即 grep 校验**关键行；坏了就从原仓库重新拷贝再改                                                                                                 |
| 7   | `-D名字=$var`                        | 变量没展开（编译器报 `use of undeclared identifier '$var'`）                                                                                     | 显式拼接：`('-D名字=' + $var)`                                                                                                                                                             |
| 8   | `.ini` 里写中文                      | 日志乱码                                                                                                                                         | `printf` 标签用 ASCII                                                                                                                                                                      |
| 9   | 在 `.ini` 里用 `EXIT`                | 不生效（文档明确：仅 Command 窗口可用）                                                                                                          | 驱动脚本**超时 + Kill**，再读 LOG                                                                                                                                                          |
| 10  | 上一轮 UV4 未退出                    | 下次 `-d` 起不来 / 无日志                                                                                                                        | 每轮先 `Get-Process UV4 \| Stop-Process -Force`                                                                                                                                            |
| 11  | 两版 bench 不等长                    | `Code` 体积差被 bench 差异污染                                                                                                                   | 差异用 `#ifdef` **对称分支**                                                                                                                                                               |
| 12  | 用 -O1 的"函数 vs 宏"差异下结论      | 20~40 周期的假差异盖过设计差异                                                                                                                   | 两版**同风格**（都宏或都函数）；或统一 -O2                                                                                                                                                 |
| 13  | 日志被 `D` 转储淹没                  | 找不到结果                                                                                                                                       | 用 `Select-String '^r\d+ '` 过滤（或 `LOG` 只留 `printf`）                                                                                                                                 |
| 14  | 只测"发送函数"                       | **成本归属错**：落地被推到主循环时结论反转                                                                                                       | 同时测"发送"与"**端到端(发送+落地)**"                                                                                                                                                      |
| 15  | 依赖外设驱动代码                     | 外设无模型 ⇒ 跑不出中断                                                                                                                         | 不依赖外设；需要 handler 模式用 **NVIC 软件触发**（未验证）                                                                                                                                |
| 16  | 混用体积口径                         | `Program Size`（链接后、含死代码消除）≠ 对象级 `fromelf` 统计                                                                                   | 明确口径并写进结论                                                                                                                                                                         |
| 17  | 停掉 UV4 后**立刻读 `sim_log.txt`**  | 写日志的进程**仍持有文件句柄** ⇒ 抛"文件正由另一进程使用"（在脚本里表现为**通道假失败**；`run_all` 里"体积→周期"连着跑时必现，单跑却常常通过） | 用 `[IO.File]::Open($log, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)` 读，并**重试**几次（参考 `Tests/run_cycles.ps1`）；`Stop-Process UV4` **不保证**立刻释放 |

---

## 八、能力边界

| 能力                        | 说明                                                                                   |
| --------------------------- | -------------------------------------------------------------------------------------- |
| ✅ 指令级执行 + 周期计数    | 适合"**同口径对比**"（A/B 方案、改前改后）                                             |
| ⚠️ 绝对周期               | 模拟器是简化存储模型；真实芯片有 Flash 等待周期/总线仲裁 ⇒ 绝对值有偏差，**比值可信** |
| ❌ 外设                     | UART/TIM/GPIO 等是否有模型取决于器件与 DFP（ST 器件一般无厂商仿真模型）⇒ 不要依赖     |
| ❌ 总线竞争/DMA 抢占/低功耗 | 不建模                                                                                 |
| ❓ 未验证                   | `DWT->CYCCNT`、`signal` 函数内调用用户函数、`SAVE` 转存命令                            |

---

## 九、实战样例（本仓库 XCOS 用例）

| 项                 | 值                                                                                                                                                                              |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 工程               | `Examples/CortexM3_Test` 临时副本（器件 STM32F103ZE，`<uSim>1`，`SARMCM3.DLL`）                                                                                                 |
| 编译器             | 工程实际配置 **AC5 V5.06u7 / -O1**（`<uAC6>0`、`<Optim>1`）                                                                                                                     |
| 测点               | 替换副本的 `Code/Main/main.c`：造 N 个延时任务 + 1 个等待通知任务，用 SysTick 计时，结果写 `g_res[]`，末尾调 `SimDone()`                                                        |
| 脚本               | `sim.ini`：`LOG` → `g, SimDone` → `FUNC DumpRes(){printf…}` → `D &g_res,…`                                                                                                 |
| 得到               | 单次 API 的周期数、主循环每轮成本、`Code/ZI`、`sizeof`（程序内打印）                                                                                                            |
| 结论存档           | 本仓库"唤醒路径优化"性能专题（**内部记录，不对外引用**；含全部实测与三次更正过程）                                                                                              |
| 原始数据           | `%TEMP%\xcos_sim\sim_log.txt`、`build_before.log`、`build_after.log`                                                                                                            |
| **本仓库已脚本化** | **`Tests/run_cycles.ps1`**（测点 `Tests/bench/`、ini 模板 `Tests/bench/cycles.ini`、基线 `Tests/baseline.json` 的 `cycles` 段，±5% 对比）——即本文档 §2~§6 流程的自动化封装 |

**样例数字（AC5 -O1，21 节点，供对照）**：

| 测点                   | 现状        | 变体         |
| ---------------------- | ----------- | ------------ |
| 唤醒 API（目标在等待） | 104 周期    | 78 周期      |
| "通知→落地"端到端     | 104 周期    | 179~204 周期 |
| 每轮门控               | 13 周期     | 9 周期       |
| 时间表插入（20 节点）  | 335 周期    | 325 周期     |
| `Code` / `ZI`          | 2256 / 2388 | 2188 / 2404  |

> 该样例最有价值的一条经验：**先测端到端、再谈优化**——单看"发送函数"会得出完全相反的结论。

---

## 十、开工/收工检查清单

**开工前**
- [ ] 器件 DFP 已安装；`.uvprojx` 指向仿真器 DLL；`.uvoptx`：`uSim=1`、`sGomain=0`、`sIfile` 指向 `.ini`
- [ ] 临时工作区已**镜像相对路径**；测点程序已就位（含 `SimDone()` 空函数）
- [ ] `SysTick` 计时已初始化；**计时开销已校准**并记录
- [ ] 若要对比：两版 bench **等长**、同编译器/同优化级别

**收工前**
- [ ] 两版数据都跑完，且 `Program Size` 与周期数**同口径**
- [ ] `UV4` 残进程已清理
- [ ] 结果落档（LOG 原文 + 汇总表 + 口径说明）
- [ ] `git status` 确认**仓库源码未被改动**（临时目录才允许改）

---

## 附：文件索引

| 路径                                     | 内容                                                                                   |
| ---------------------------------------- | -------------------------------------------------------------------------------------- |
| `<Keil>\UV4\uv4.chm`                     | µVision 手册（Command Line / Debugging / Debug Scripting / 命令参考 `uv4_cm_*.html`） |
| `<Keil>\ARM\BIN\SarmCM3.dll`             | Cortex-M3 仿真核心（其他核心：各核心对应 DLL）                                         |
| `<Keil>\UV4\uVision.com`                 | 控制台版启动器（stdout 可捕获，适合批处理）                                            |
| `<工程>.uvoptx`                          | 调试设置：`uSim` / `sIfile` / `sGomain` / `TargetDllName`                              |
| `Docs/Skill/XCOS_Agent_Skill.md`         | XCOS 框架开发技能（配套）                                                              |
| 本仓库"唤醒路径优化"性能专题（内部记录） | 本文技能的实战样例与本仓库测量数据                                                     |


