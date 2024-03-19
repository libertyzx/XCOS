# XCubicOS
## 1.介绍
一个精简的协程系统调度框架;

## 2.简介
- 纯C语言实现,可裸机或嵌入RTOS中调用;
- 低存储,内存占用(ram<100Byte,rom<1000Byte);
- 协作式任务调度,无优先级概念;
- 无任务堆栈概念,所有任务共用系统堆栈;
- 没有上下文切换,框架无需任何中断支撑,没有临界段和中断开关;
- 框架目前已实现:阻塞延时,任务通知,任务挂起恢复;

## 3.实现
协程实现方式见我的CSDN:
[c语言协程](https://blog.csdn.net/libertyzx/article/details/126186870)

## 4.使用说明
加入工程
> 将"src"文件中所有".c"文件加入工程,并将"src/include"添加到工程包含(include paths)即可;
> 使用时只需包含"XCOS.h"文件;

### 4.1. XCOS的配置
- 配置数据在"XC_Cnf.h"文件中定义;

XCOS的配置参数如下:
| 类型名 | 配置类型 | 说明 |
| --- | --- | --- |
| _XC_SysTickPerScond | 宏 | 系统每秒滴答数(滴答计数频率) |
| _XC_SysTickCount | 宏 | 系统滴答计数 |
| XCuint_t | 宏 | XCOS基础数据类型,默认32位(uint32_t) |

**"_XC_SysTickPerScond"系统每秒滴答数(滴答计数频率)说明**
- 单位Hz,是系统最小的时间单位;
- 默认值是"1000"也就是1ms;
- 最大值是1000000Hz;对应时间是1us;

**"_XC_SysTickCount"系统滴答计数说明**
- 滴答计数是一个累加值,累加时间必须和"_XC_SysTickPerScond"时间一致;
- 此值会在操作中被读取(只读),作为时间计算的依据;

**系统滴答计数可用2种方式实现:**

***方式1:中断累加形式(默认);***
- 用一个变量在定时器中累加,每一个滴答时间则+1;
- 宏"_XC_SysTickCount"指向此变量即可;
> 用户需要处理:
> 1. 在".c"文件中定义"volatile uint32_t"类型的全局变量"g_SysTickCount";
> 2. 创建定时器,按"_XC_SysTickPerScond"计数;
> 3. 在定时器中累加系统滴答计数:"g_SysTickCount++";

***方式2:计数器形式;***
- 因为协程并不需要中断来切换上下文,所以为了使效率最高可以用一个计数器来做系统滴答计数;
- 宏"_XC_SysTickCount"作为一个计数器的函数的返回值(或其本身寄存器值);
> 用户需要处理:
> 1. 创建定时器,按"_XC_SysTickPerScond"计数;
> 2. 将计数值作为"_XC_SysTickCount"的指向;


### 4.2. 调用说明

基本句柄类型
| 类型名 | 32位下占用字节 | 说明 |
| --- | --- | --- |
| XCOS_t | 80 | 定义一个XCOS框架的句柄 |
| XCTCB_t | 36 | 定义一个协程任务控制块 |

调度器函数
| 函数名 | 函数类型 | 说明 |
| --- | --- | --- |
| XCSch_Init | 函数 | 初始化一个调度器 |
| XCSch_TaskReg | 函数 | 注册一个任务 |
| XCSch_TaskRemove | 函数 | 移除一个任务 |
| XCSch_Run | 函数 | 运行调度器 |

协程任务处理函数
| 函数名 | 函数类型 | 功能 | 说明 |
| --- | --- | --- | --- |
| XC_Enter | 宏 | 基础 | 进入协程块 |
| XC_Leave | 宏 | 基础 | 离开协程块 |
| XC_Yield | 宏 | 基础 | **协程块内调用**;让出当前任务的控制权 |
| XC_DelayTick | 宏 | 时间调度 | **协程块内调用**;延时n个Tick |
| XC_Delay_** | 宏 | 时间调度 | **协程块内调用**;延时n个时间(**=[ms,s,min,h,day]) |
| XC_WaitNotify | 宏 | 任务通知 | **协程块内调用**;等待通知到来 |
| XC_GetNotifyTimeout | 宏 | 任务通知 | **协程块内调用**;获取通知超时 |
| XC_GetNotifyData | 宏 | 任务通知 | **协程块内调用**;获取通知的数据 |
| XC_GetTaskState | 宏 | 基础 | ***任意位置调用***;获取任务状态 |
| XC_SendNotify | 函数 | 任务通知 | ***不可在中断中调用***;发送通知 |
| XC_TaskSuspend | 函数 | 任务挂起 | ***不可在中断中调用***;任务挂起 |
| XC_TaskResume | 函数 | 任务挂起 | ***不可在中断中调用***;任务恢复 |


## 5.文件说明

| 文件 | 说明 |
| --- | --- |
| COR_ANSI.h | 协程底层实现("ANSI-C"是由"switch case"实现) |
| COR_GNU.h | 协程底层实现("GNU-C" 是由"goto label" 实现) |
| CortexMx.h | 对应CortexMx使用的宏定义 |
| BinData.h | 二进制数值宏定义 |
| [XC_CPU.h](./docs/XC_Type.md) | 不同平台的数据类型,关键字统一 |
| [XC_Type.h](./docs/XC_Type.md) | 通用数据类型定义 |
| [XC_MacroFunc.h](./docs/XC_MacroFunc.md) | 通用函数宏实现 |
| XC_Cnf.h | 用于存放"XCOS"的配置参数 |
| [XC_Time.h](./docs/XC_Time.md) | 时间处理,及编译时间输出 |
| XC_List.c | 链表实现 |
| XC_List.h | 链表实现 |
| XC_Sch.c | 调度处理 |
| XC_Sch.h | 调度处理 |
| XC_Task.c | 任务处理 |
| XC_Task.h | 任务处理 |
| XCOS.h | 总包含 |




