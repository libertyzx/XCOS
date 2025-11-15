# 更新说明

---

## T2025/11/12
- 版本:2.00
- 将所有注释改为"doxdocgen"形式;
- 增加"clang-format"格式化配置文件".clang-format";
- 优化代码和内存占用,优化运行性能;
- 清除文件修改日志,以后都在此文件说明;
- 优化以下操作,使中断中可调用:
    - 任务唤醒;
    - 任务挂起;
    - 任务挂起恢复;
- 信号量(XC_Sem)
    - 删除全部,任务通知可实现类似信号量的处理;
- 时间计数处理(XC_TimeCount)
    - 此文件中所有移动到了"XCTime"中
    - 删除或者修改函数:
    | 函数                      | 功能                                          | 操作  | 操作说明                                          |
    | ---                       | ---                                           | ---   | ---                                               |
    | XCTime_CompareTick_min    | 按min比较时间                                 | 删除  | 时间处理最高到s,后面自行转换                      |
    | XCTime_CompareTick_h      | 按h比较时间                                   | 删除  | 时间处理最高到s,后面自行转换                      |
    | XCTime_CompareTick_Day    | 按day比较时间                                 | 删除  | 时间处理最高到s,后面自行转换                      |
    | XCTime_Delay_s            | 按s死循环                                     | 删除  | 因为看门狗,死循环最大单位为ms                     |
    | XCTime_Delay_min          | 按min死循环                                   | 删除  | 因为看门狗,死循环最大单位为ms                     |
    | XCTime_Delay_h            | 按h死循环                                     | 删除  | 因为看门狗,死循环最大单位为ms                     |
    | XCTime_Delay_day          | 按day死循环                                   | 删除  | 因为看门狗,死循环最大单位为ms                     |
    | XCTime_GetTick_ms         | 获取Tick+min后的Tick                          | 删除  | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_GetTick_s          | 获取Tick+min后的Tick                          | 删除  | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_GetTick_min        | 获取Tick+min后的Tick                          | 删除  | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_GetTick_h          | 获取Tick+h后的Tick                            | 删除  | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_GetTick_day        | 获取Tick+day后的Tick                          | 删除  | 和"XCTime_CompareTickt"一起处理有安全隐患,故删除  |
    | XCTime_CompareTickt       | 比较当前Tick判断时间计数是否到达              | 删除  | 和"XCTime_GetTick_*"一起处理有安全隐患,故删除     |
    | XCTime_tDelay             | 按Tick死循环延时(扩展)                        | 删除  | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_ms          | 按ms死循环延时(扩展)                          | 删除  | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_s           | 按s死循环延时(扩展)                           | 删除  | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_min         | 按min死循环延时(扩展)                         | 删除  | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_h           | 按h死循环延时(扩展)                           | 删除  | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tDelay_day         | 按day死循环延时(扩展)                         | 删除  | 已有"XCTime_BlockDelay"和"XCTime_BlockDelay_ms"   |
    | XCTime_tUpdateTick_min    | 更新需要延时比较的时间(min)                   | 删除  | 时间处理最高到s,后面自行转换                      |
    | XCTime_tUpdateTick_h      | 更新需要延时比较的时间(h)                     | 删除  | 时间处理最高到s,后面自行转换                      |
    | XCTime_tUpdateTick_Day    | 更新需要延时比较的时间(Day)                   | 删除  | 时间处理最高到s,后面自行转换                      |
    | XCTime_tGetTickCount      | 获取类型中的Tick计数值                        | 删除  | 不常用,删除                                       |
    | XCTime_tSetTickCount      | 设置类型中的Tick计数值                        | 删除  | 不常用,删除                                       |
    | XCTime_tClrTickCount      | 清除类型中的Tick计数值                        | 删除  | 不常用,删除                                       |
    | XCTime_tGetWaitCount      | 获取类型中的等待计数值                        | 删除  | 不常用,删除                                       |
    | XCTime_tSetWaitCount      | 设置类型中的等待计数值                        | 删除  | 不常用,删除                                       |
    | XCTime_tClrWaitCount      | 清除类型中的等待计数值                        | 删除  | 不常用,删除                                       |
    | _Time_\*\*2\*\*           | 从ms到year的各类时间转换的比例                | 删除  | 不使用,删除                                       |
    | _Time_min2Tick            | 时间转Tick                                    | 删除  | 不使用,删除                                       |
    | _Time_h2Tick              | 时间转Tick                                    | 删除  | 不使用,删除                                       |
    | _Time_day2Tick            | 时间转Tick                                    | 删除  | 不使用,删除                                       |
    | _Time_Tick2min            | Tick转时间                                    | 删除  | 不使用,删除                                       |
    | _Time_Tick2h              | Tick转时间                                    | 删除  | 不使用,删除                                       |
    | _Time_Tick2day            | Tick转时间                                    | 删除  | 不使用,删除                                       |
    | XCTime_Delay              | [用户]死循环延时Tick个计数                    | 修改  | 改名为"XCTime_BlockDelay";宏改为函数              |
    | XCTime_Delay_ms           | [用户]死循环延时ms                            | 修改  | 改名为"XCTime_BlockDelay_ms";宏改为函数           |
    | XCTime_GetRemainTick      | [用户]获取剩下多少Tick                        | 修改  | 内联改普通函数                                    |
    | XCTime_GetRunTick         | [用户]获取运行了多少Tick                      | 修改  | 内联改普通函数                                    |
    | XCTime_tGetRemainTick     | [用户]获取剩下多少Tick                        | 修改  | 内联改普通函数                                    |
    | XCTime_tGetRunTick        | [用户]获取运行了多少Tick                      | 修改  | 内联改普通函数                                    |
- 时间处理(XC_Time)
    - 删除所有日历相关的代码;
    - 将"XC_TimeCount"所有代码移动到此文件中;
    - 删除或者修改函数说明件"时间计数处理(XC_TimeCount)";
- 链表操作(XC_List)
    - 删除"XCListNode_t"链表类型中所属链表(pRootList)字段;
    - 修改"XCListNode_t"链表类型中指向上个节点字段名称("pPrevious"->"pPrev")字段;
    - 删除"XCListBasic_t"类型;
    - 删除"XCListRoot_t"类型用"XCListNode_t"类型替代,根节点只是标记的普通节点,不在做特殊化;
        > 注意,此改动会更改整个工程相关函数,说明内不做独立函数说明;
    - 删除或者修改函数(不含,"XCListRoot_t"类型改"XCListNode_t"类型修改的函数或者函数宏):
    | 函数                          | 功能                                              | 操作  | 操作说明                                          |
    | ---                           | ---                                               | ---   | ---                                               |
    | XCList_RemoveBasicNode        | [内部]移除基础节点                                | 删除  | 基础类型已经删除,函数无效,由"XCList_Remove"替代   |
    | XCList_InsertNodeNext         | [内部]将新节点插入某节点之后                      | 删除  | 框架不使用                                        |
    | XCList_InsertStart            | [内部]将节点插入链表的开始                        | 删除  | 框架不使用                                        |
    | XCList_InsertEnd              | [内部]将节点插入链表的结尾                        | 删除  | 框架不使用                                        |
    | XCList_LinkNode               | [内部]连接2个节点                                 | 删除  | 框架不使用                                        |
    | XCList_InitNode               | [内部]初始化节点                                  | 修改  | 改为宏                                            |
    | XCList_Init                   | [内部]初始化链表                                  | 修改  | 改为宏                                            |
    | XCList_InsertNodeBefore       | [内部]将新节点插入某节点之前                      | 修改  | 修改名称,原"XCList_InsertNodePrevious"            |
    | XCList_Remove                 | [内部]从链表中移除一个节点                        | 修改  | 改为宏                                            |
    | XCList_MoveListToNodeBefore   | [内部]将一个链表全部移动到另个链表的一个节点前    | 修改  | 修改名称,原"XCList_MoveListToNodeBefore"          |
- 调度处理(XC_Sch)
    - "XCOS_t"类型字段"pReadyListNodeIndex"改名为"pReadyNode";
    - "XCOS_t"类型字段"PreviousTick"改名为"PrevTick";
    - 删除或者修改函数:
    | 函数                              | 功能                                  | 操作  | 操作说明                                          |
    | ---                               | ---                                   | ---   | ---                                               |
    | XCSch_TaskReg                     | [用户]任务注册                        | 删除  | 移动为"XC_Task"的"XC_TaskReg"                     |
    | XCSch_TaskRemove                  | [用户]任务移除                        | 删除  | 移动为"XC_Task"的"XC_TaskRemove"                  |
    | XCSch_SetTaskEntryPoint           | [用户]设置任务入口                    | 删除  | 移动为"XC_Task"的"XC_SetTaskEntryPoint"           |
    | XCSch_AddTask                     | [用户]添加任务                        | 删除  | 移动为"XC_Task"的"XC_AddTask"                     |
    | XCSch_ListNodeRemove              | [内部]链表节点移除                    | 删除  | 移动为"XC_Task"的"XC_ListNodeRemove"              |
    | XCSch_ListNodeInsertIndexPrevious | [内部]将节点插入索引前                | 删除  | 移动为"XC_Task"的"XC_ListNodeInsertIndexPrevious" |
    | XCSch_ListNodeInsertAsc           | [内部]升序排列的插入节点              | 删除  | 移动为"XC_Task"的"XC_ListNodeInsertAsc"           |
    | XCSch_Init                        | [用户]调度器初始化                    | 修改  | 增加中断,休眠支持                                 |
    | XCSch_Run                         | [用户]调度器运行(阻塞)                | 修改  | 增加中断,休眠支持                                 |
    | XCSch_RunNonBlocked               | [用户]调度器运行(非阻塞)              | 修改  | 增加中断,休眠支持                                 |
    | XCSch_TaskSched                   | [私有]任务唤醒,挂起,恢复的异步操作    | 新增  | 增加中断支持                                      |
    | XCSch_SetIdleCallback             | [用户]设置空闲处理回调                | 新增  | 增加空闲处理                                      |
    | XCSch_UpdateTickAfterWakeup       | [用户]休眠唤醒后更新tick              | 新增  | 增加空闲处理                                      |
- 任务操作(XC_Task)
    - 删除任务控制块类型中"通知数据"和"传递的参数"的32位整型共用体,只留下指针;
    - 删除或者修改函数:
    | 函数                              | 功能                                          | 操作  | 操作说明                                                          |
    | ---                               | ---                                           | ---   | ---                                                               |
    | XC_GetParamUint                   | 协程块内获取任务注册时传递的参数(uint32_t)    | 删除  | 基础为指针传递的,没有必要在做一层整型数据传递                     |
    | XC_GetNotifyDataUint              | 获取通知的数据(uint32_t)                      | 删除  | 基础为指针传递的,没有必要在做一层整型数据传递                     |
    | XC_GetTaskParamUint               | 获取任务注册时传递的参数(uint32_t)            | 删除  | 基础为指针传递的,没有必要在做一层整型数据传递                     |
    | XC_UpdateNotifyDataUint           | 更新通知数据(uint32_t)                        | 删除  | 基础为指针传递的,没有必要在做一层整型数据传递                     |
    | XC_ReadNotifyDataUint             | 读取通知数据(uint32_t)                        | 删除  | 基础为指针传递的,没有必要在做一层整型数据传递                     |
    | XC_SendNotifyUint                 | 发送通知(uint32_t)                            | 删除  | 基础为指针传递的,没有必要在做一层整型数据传递                     |
    | XC_TaskReset                      | 复位当前任务                                  | 删除  | 已有"XC_Reset",不在做兼容                                         |
    | XC_Delay_min                      | 按min延时                                     | 删除  | 延时最高到s,后面自行转换                                          |
    | XC_Delay_h                        | 按小时延时                                    | 删除  | 延时最高到s,后面自行转换                                          |
    | XC_Delay_day                      | 按天延时                                      | 删除  | 延时最高到s,后面自行转换                                          |
    | XC_MoveTaskToReadyList            | [内部]将任务移动到就绪表                      | 新增  | 优化代码                                                          |
    | XC_MoveTaskToBlockedList          | [内部]将任务移动到阻塞表                      | 新增  | 优化代码                                                          |
    | XC_InsertTaskToReadyList          | [内部]将任务插入到就绪表                      | 新增  | 原"XC_Sch"的"XCSch_ListNodeInsertIndexPrevious"移入,改名,改宏     |
    | XC_InsertTaskToTimeList           | [内部]将任务插入到时间表                      | 新增  | 原"XC_Sch"的"XCSch_ListNodeInsertAsc"移入,并改名                  |
    | XC_RemoveTaskNode                 | [内部]移除任务节点                            | 新增  | 原"XC_Sch"的"XCSch_ListNodeRemove"移入                            |
    | XC_TaskReg                        | [用户]任务注册                                | 新增  | 原"XC_Sch"的"XCSch_TaskReg"移入                                   |
    | XC_TaskRemove                     | [用户]任务移除                                | 新增  | 原"XC_Sch"的"XCSch_TaskRemove"移入,并优化                         |
    | XC_SetTaskEntryPoint              | [用户]设置任务入口                            | 新增  | 原"XC_Sch"的"XCSch_SetTaskEntryPoint"移入                         |
    | XC_AddTask                        | [用户]添加任务                                | 新增  | 原"XC_Sch"的"XCSch_AddTask"移入,并改名                            |
    | XC_SendNotify                     | [用户]发送通知                                | 修改  | 增加中断支持                                                      |
    | XC_TaskSuspend                    | [用户]任务挂起                                | 修改  | 增加中断支持                                                      |
    | XC_TaskResume                     | [用户]任务挂起恢复                            | 修改  | 增加中断支持                                                      |
    | XC_GetNotifyWakeState             | [用户][协程]获取通知唤醒状态                  | 修改  | 改名,改通知专用;原函数为"XC_GetWakeTimeout"                       |
    | XC_GetWakeType                    | [用户][协程]获取唤醒类型                      | 新增  | 完善功能                                                          |
- 删除文件统计
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