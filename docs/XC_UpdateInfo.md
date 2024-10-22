# 更新说明

## T2024/08/28
### XC_Sch
- 修改"XCSch_TaskReset"函数,任务复位不复位传递参数"Param";
### XC_Time
- 修正"XCTime_GetRemainTick"函数,获取滴答计数变为ms的问题;
- 增加函数宏"XCTime_tGetRunTime_ms"获取运行了多少ms;

## T2024/08/12
### XC_Time
- 将原"XCAPI"中的Time处理(时间戳,日历,秒转换计算处理)转移至"XC_Time"中;
- "XC_Time.h"中编译相关宏独立到"XC_TimeCompile.h";
- "XC_Time.h"中系统时间计数相关宏独立到"XC_TimeCount.h";
- 将"XC_Time"中所有兼容性代码移至"XC_TimeCompatibility.h";
- 将"XCTime_CompareTick_t"重命名"XCTime_CompareTickt",防止认为是变量类型;