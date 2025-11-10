# 更新说明

---

## T2025/11/10
- 版本:2.00
- 将所有注释改为"doxdocgen"形式;
- 增加"clang-format"格式化配置文件".clang-format";
- 清除文件修改日志,以后都在此文件说明;
- 删除"二值信号量";
    > 任务通知可实现类似信号量的处理;
- 优化以下操作,使中断中可调用:
    - 任务唤醒;
    - 任务挂起;
    - 任务挂起恢复;
- 删除任务控制块类型中"通知数据"和"传递的参数"的32位整型共用体,只留下指针;
- 删除原"XC_Time.c"和"XC_Time.h"中所以日历相关函数,因为内核不使用;
- 将文件"XC_TimeCount.h"所有代码移动到"XC_Time.h",并删除"XC_TimeCount.h"文件;
- 增加配置(文件"XC_Cnf.h"):
    - "_XC_Cnf_IdleSupport"配置框架休眠支持;
- 删除函数(函数宏):
    | 函数                      | 文件              | 功能                                          | 删除原因                                          |
    | ---                       | ---               | ---                                           | ---                                               |
    | XC_GetParamUint           | XC_Task.h         | 协程块内获取任务注册时传递的参数(uint32_t)    | 基础为指针传递的,没有必要在做一层整型数据传递     |
    | XC_GetNotifyDataUint      | XC_Task.h         | 获取通知的数据(uint32_t)                      | 基础为指针传递的,没有必要在做一层整型数据传递     |
    | XC_GetTaskParamUint       | XC_Task.h         | 获取任务注册时传递的参数(uint32_t)            | 基础为指针传递的,没有必要在做一层整型数据传递     |
    | XC_UpdateNotifyDataUint   | XC_Task.h         | 更新通知数据(uint32_t)                        | 基础为指针传递的,没有必要在做一层整型数据传递     |
    | XC_ReadNotifyDataUint     | XC_Task.h         | 读取通知数据(uint32_t)                        | 基础为指针传递的,没有必要在做一层整型数据传递     |
    | XC_SendNotifyUint         | XC_Task.h         | 发送通知(uint32_t)                            | 基础为指针传递的,没有必要在做一层整型数据传递     |
    | XC_TaskReset              | XC_Task.h         | 复位当前任务                                  | 已有"XC_Reset",不在做兼容                         |
    | XC_Delay_min              | XC_Task.h         | 按min延时                                     | 延时最高到s,后面自行转换                          |
    | XC_Delay_h                | XC_Task.h         | 按小时延时                                    | 延时最高到s,后面自行转换                          |
    | XC_Delay_day              | XC_Task.h         | 按天延时                                      | 延时最高到s,后面自行转换                          |
    | XCTime_CompareTick_min    | XC_TimeCount.h    | 按min比较时间                                 | 时间处理最高到s,后面自行转换                      |
    | XCTime_CompareTick_h      | XC_TimeCount.h    | 按h比较时间                                   | 时间处理最高到s,后面自行转换                      |
    | XCTime_CompareTick_Day    | XC_TimeCount.h    | 按day比较时间                                 | 时间处理最高到s,后面自行转换                      |
    | XCTime_Delay_s            | XC_TimeCount.h    | 按s死循环                                     | 因为看门狗,死循环最大单位为ms                     |
    | XCTime_Delay_min          | XC_TimeCount.h    | 按min死循环                                   | 因为看门狗,死循环最大单位为ms                     |
    | XCTime_Delay_h            | XC_TimeCount.h    | 按h死循环                                     | 因为看门狗,死循环最大单位为ms                     |
    | XCTime_Delay_day          | XC_TimeCount.h    | 按day死循环                                   | 因为看门狗,死循环最大单位为ms                     |
    | XCTime_GetTick_ms         | XC_TimeCount.h    | 获取Tick+min后的Tick                          | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_GetTick_s          | XC_TimeCount.h    | 获取Tick+min后的Tick                          | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_GetTick_min        | XC_TimeCount.h    | 获取Tick+min后的Tick                          | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_GetTick_h          | XC_TimeCount.h    | 获取Tick+h后的Tick                            | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_GetTick_day        | XC_TimeCount.h    | 获取Tick+day后的Tick                          | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_CompareTickt       | XC_TimeCount.h    | 比较当前Tick判断时间计数是否到达              | 和"XCTime_GetTick_*"一起处理有安全隐患,故删除     |
    | XCTime_tDelay             | XC_TimeCount.h    | 按Tick死循环延时(扩展)                        | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_ms          | XC_TimeCount.h    | 按ms死循环延时(扩展)                          | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_s           | XC_TimeCount.h    | 按s死循环延时(扩展)                           | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_min         | XC_TimeCount.h    | 按min死循环延时(扩展)                         | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_h           | XC_TimeCount.h    | 按h死循环延时(扩展)                           | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_day         | XC_TimeCount.h    | 按day死循环延时(扩展)                         | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tUpdateTick_min    | XC_TimeCount.h    | 更新需要延时比较的时间(min)                   | 时间处理最高到s,后面自行转换                      |
    | XCTime_tUpdateTick_h      | XC_TimeCount.h    | 更新需要延时比较的时间(h)                     | 时间处理最高到s,后面自行转换                      |
    | XCTime_tUpdateTick_Day    | XC_TimeCount.h    | 更新需要延时比较的时间(Day)                   | 时间处理最高到s,后面自行转换                      |
    | XCTime_tGetTickCount      | XC_TimeCount.h    | 获取类型中的Tick计数值                        | 不常用,删除                                       |
    | XCTime_tSetTickCount      | XC_TimeCount.h    | 设置类型中的Tick计数值                        | 不常用,删除                                       |
    | XCTime_tClrTickCount      | XC_TimeCount.h    | 清除类型中的Tick计数值                        | 不常用,删除                                       |
    | XCTime_tGetWaitCount      | XC_TimeCount.h    | 获取类型中的等待计数值                        | 不常用,删除                                       |
    | XCTime_tSetWaitCount      | XC_TimeCount.h    | 设置类型中的等待计数值                        | 不常用,删除                                       |
    | XCTime_tClrWaitCount      | XC_TimeCount.h    | 清除类型中的等待计数值                        | 不常用,删除                                       |
    | _Time_\*\*2\*\*           | XC_TimeCount.h    | 从ms到year的各类时间转换的比例                | 不使用,删除                                       |
    | _Time_min2Tick            | XC_TimeCount.h    | 时间转Tick                                    | 不使用,删除                                       |
    | _Time_h2Tick              | XC_TimeCount.h    | 时间转Tick                                    | 不使用,删除                                       |
    | _Time_day2Tick            | XC_TimeCount.h    | 时间转Tick                                    | 不使用,删除                                       |
    | _Time_Tick2min            | XC_TimeCount.h    | Tick转时间                                    | 不使用,删除                                       |
    | _Time_Tick2h              | XC_TimeCount.h    | Tick转时间                                    | 不使用,删除                                       |
    | _Time_Tick2day            | XC_TimeCount.h    | Tick转时间                                    | 不使用,删除                                       |
- 移动修改函数
    | 原文件名                          | 原所在文件        | 现文件名                          | 现所在文件    | 修改说明                              |
    | ---                               | ---               | ---                               | ---           | ---                                   |
    | XCTime_Delay                      | XC_TimeCount.h    | XCTime_BlockDelay                 | XC_Time.c     | 宏改为函数,并重命名更加符合功能说明   |
    | XCTime_Delay_ms                   | XC_TimeCount.h    | XCTime_BlockDelay_ms              | XC_Time.c     | 宏改为函数,并重命名更加符合功能说明   |
    | XCTime_GetRemainTick              | XC_TimeCount.h    | XCTime_GetRemainTick              | XC_Time.c     | 内联改普通函数,没有重命名             |
    | XCTime_GetRunTick                 | XC_TimeCount.h    | XCTime_GetRunTick                 | XC_Time.c     | 内联改普通函数,没有重命名             |
    | XCTime_tGetRemainTick             | XC_TimeCount.h    | XCTime_tGetRemainTick             | XC_Time.c     | 内联改普通函数,没有重命名             |
    | XCTime_tGetRunTick                | XC_TimeCount.h    | XCTime_tGetRunTick                | XC_Time.c     | 内联改普通函数,没有重命名             |
    | XCSch_TaskReg                     | XC_Sch.c          | XC_TaskReg                        | XC_Task.c     | 修改函数名称                          |
    | XCSch_TaskRemove                  | XC_Sch.c          | XC_TaskRemove                     | XC_Task.c     | 修改函数名称                          |
    | XCSch_SetTaskEntryPoint           | XC_Sch.c          | XC_SetTaskEntryPoint              | XC_Task.c     | 修改函数名称                          |
    | XCSch_AddTask                     | XC_Sch.c          | XC_AddTask                        | XC_Task.c     | 修改函数名称                          |
    | XCSch_ListNodeRemove              | XC_Sch.c          | XC_ListNodeRemove                 | XC_Task.c     | 修改函数名称                          |
    | XCSch_ListNodeInsertIndexPrevious | XC_Sch.c          | XC_ListNodeInsertIndexPrevious    | XC_Task.c     | 修改函数名称                          |
    | XCSch_ListNodeInsertAsc           | XC_Sch.c          | XC_ListNodeInsertAsc              | XC_Task.c     | 修改函数名称                          |
- 增加函数
    | 函数名                        | 所在文件  | 功能                                                      |
    | ---                           | ---       | ---                                                       |
    | XCSch_TaskSched               | XC_Sch.c  | [私有]中断处理时任务唤醒,任务挂起,任务挂起恢复的异步操作  |
    | XCSch_SetIdleCallback         | XC_Sch.c  | [用户]设置休眠处理回调                                    |
    | XCSch_UpdateTickAfterWakeup   | XC_Sch.c  | [用户]休眠唤醒后更新tick                                  |
- 修改函数
    | 函数名                | 所在文件  | 功能                      | 修改说明          |
    | ---                   | ---       | ---                       | ---               |
    | XC_SendNotify         | XC_Sch.c  | [用户]发送通知            | 增加中断支持      |
    | XC_TaskSuspend        | XC_Sch.c  | [用户]任务挂起            | 增加中断支持      |
    | XC_TaskResume         | XC_Sch.c  | [用户]任务挂起恢复        | 增加中断支持      |
    | XCSch_Init            | XC_Sch.c  | [用户]调度器初始化        | 增加中断,休眠支持 |
    | XCSch_Run             | XC_Sch.c  | [用户]调度器运行(阻塞)    | 增加中断,休眠支持 |
    | XCSch_RunNonBlocked   | XC_Sch.c  | [用户]调度器运行(非阻塞)  | 增加中断,休眠支持 |
- 删除文件
    | 文件名                    | 功能              | 删除原因                                      |
    | ---                       | ---               | ---                                           |
    | CortexMx.h                | CortexMx核的功能  | 内核中不使用                                  |
    | XC_CPU.h                  | 控制器兼容宏      | 框架是纯C语言(ANSI-C或GNU-C)实现,和控制器无关 |
    | XC_Type.h                 | 类型兼容          | 内核中不使用                                  |
    | XC_MacroFunc.h            | 一些通用的宏定义  | 内核中不使用                                  |
    | XC_BitFlag.h              | 通用位定义处理    | 内核中不使用                                  |
    | XC_TimeCompatibility.h    | 时间处理兼容函数  | 保证内核纯净,不在做兼容                       |
    | XC_TimeCompile.h          | 版本数据实现      | 内核中不使用                                  |
    | XCBase.h                  | 兼容"XCBase"      | 保证内核纯净,不在做兼容                       |
    | XC_Sem.c                  | 信号量实现        | 用通知替代                                    |
    | XC_Sem.h                  | 信号量实现        | 用通知替代                                    |

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

---

## T2024/03/18
- 初始编写;