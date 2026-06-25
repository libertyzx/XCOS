# 更新说明

---

## T2026/06/25
- 版本:2.1.0

- 修改文件编码,统一到utf-8

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