# 20260914 B1 错误上报钩子（诊断）

> **条目**：[`Docs/问题跟踪/问题清单/20260914_诊断能力与工程化.md`](../../问题清单/20260914_诊断能力与工程化.md)（✅ 已修复）
> **状态**：已实施并归档（2026-09-14）

## 做了什么

帧栈越界（未配帧栈就 `XC_Cor_Call` / 嵌套超过 `CorDepthMax`）原本是**静默复位**（任务从头运行、现场无痕迹）；本方案增加**编译期可开关的错误上报钩子**，把现场报给用户：

| 文件 | 内容 |
| --- | --- |
| `Code/Inc/XC_Diag.h`（新增） | `XC_ErrCode_t`（`XC_ERR_NONE` / `XC_ERR_FRAME_OVERFLOW`）+ 钩子 `XC_Err_Hook` + 上报宏 `XC_Err_Report` |
| `Code/Inc/XC_Config.h` | 开关 `XC_CFG_ERR_HOOK`（默认 `0`） |
| `Code/Inc/XCOS.h` | 包含 `XC_Diag.h` |
| `Code/Src/XC_Task.c` | `XC_Task_PushFrame` 越界分支：**先上报、后降级**（钩子内可见 `CorDepth`/`CorDepthMax`/`fTask`） |

开关关闭时 `XC_Err_Report` 为 `((void)0)`：**0 代码 / 0 RAM**，且无需实现钩子。

## 怎么跑这个用例

```powershell
# 宿主编译器（任选 gcc/clang；本仓库验证用 CLion 自带 mingw）
gcc -std=gnu99 -O1 -Wall -Wextra -I Code/Inc `
    Code/Src/XC_List.c Code/Src/XC_Sch.c Code/Src/XC_Task.c Code/Src/XC_Time.c `
    "Docs/问题跟踪/修正方案/20260914_B1_错误上报钩子/测试/20260914_B1_错误上报钩子_测试.c" `
    -o b1_off.exe        # 开关关闭（不需要实现 XC_Err_Hook）

gcc -std=gnu99 -O1 -Wall -Wextra -DXC_CFG_ERR_HOOK=1 -I Code/Inc `
    Code/Src/XC_List.c Code/Src/XC_Sch.c Code/Src/XC_Task.c Code/Src/XC_Time.c `
    "Docs/问题跟踪/修正方案/20260914_B1_错误上报钩子/测试/20260914_B1_错误上报钩子_测试.c" `
    -o b1_on.exe         # 开关开启（用例内置 XC_Err_Hook 实现）
```

期望：两种配置都输出 `===== B1 用例完成: failures 0 =====`；开启档额外校验「钩子被调用 1 次 / 错误码 `XC_ERR_FRAME_OVERFLOW` / TCB 正确 / 钩子内现场 `0 >= 0`」。

> **2026-09-15 更新（"单一强制点"规则连带）**：本用例末尾原有 2 处期望"参数不一致 ⇒ `XC_Task_SetCorStack` 返回 `XC_FAIL`"，
> 而该参数校验已按新规则**删除**（参数合法性改由断言承担，见 [`Docs/XCOS.md`](../../../XCOS.md)「断言」边界规则 R1~R7）⇒ 期望同步改为：
> **断言档** = 断言上报 `XC_ERR_ASSERT` 且**参数照写**（随后立即用合法参数复原），**关闭档** = 只用合法参数验证 `XC_OK`。
> 注意：`manifest.txt` 里的 3 条 B1 用例（关档 / 开档 / 断言）判定均为 `failures 0`。

## 实测结果（2026-09-14）

| 项 | 结果 |
| --- | --- |
| 宿主（开关关/开） | 7 项 PASS / 11 项 PASS，两套 **failures = 0** |
| 行为 | 原有"静默复位"表现**保持**（复位后任务从头再次运行） |
| 体积·对象级（`XC_Task.o`，AC5 `-O1`） | 改前 **884** → 关 **884（一致）** → 开 **896（+12 B）** |
| 体积·镜像级（`Examples/CortexM3_Test` 全量构建） | 改前 **Code 8684** → 关 **8684（一致）**；`i.XC_Task_PushFrame` 段 **66 B 亦同**；0 Error / 0 Warning |
