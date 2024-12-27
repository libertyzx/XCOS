# 更新说明

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
- 修正"XCTime_GetRemainTick"函数,获取滴答计数变为ms的问题;
- 增加函数宏"XCTime_tGetRunTime_ms"获取运行了多少ms;

---

## T2024/08/12
#### XC_Time
- 将原"XCAPI"中的Time处理(时间戳,日历,秒转换计算处理)转移至"XC_Time"中;
- "XC_Time.h"中编译相关宏独立到"XC_TimeCompile.h";
- "XC_Time.h"中系统时间计数相关宏独立到"XC_TimeCount.h";
- 将"XC_Time"中所有兼容性代码移至"XC_TimeCompatibility.h";
- 将"XCTime_CompareTick_t"重命名"XCTime_CompareTickt",防止认为是变量类型;