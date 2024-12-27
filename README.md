# XCubicOS
## 1.介绍
一个精简的协程系统调度框架;

## 2.简介
- 纯C语言实现,可裸机或嵌入RTOS等支持C语言的环境;
- 低存储,内存占用
  - ram=56Byte, rom<900Byte(-O0,无任务);
  - 1个任务内存固定占40Byte;
- 协作式任务调度,无优先级概念;
- 无任务堆栈概念,所有任务共用系统堆栈;
- 没有上下文切换,框架无需任何中断支撑,没有临界段和中断开关;
- 框架目前已实现:阻塞延时,任务通知,任务挂起恢复,二值信号量;
  > - 二值信号量可在中断中调用唤醒任务;
  > - 任务通知.任务挂起/恢复不可在中断中调用;

## 3.实现
协程实现方式见我的CSDN:
[c语言协程](https://blog.csdn.net/libertyzx/article/details/126186870)

## 4.使用说明
加入工程
> - 将"src"文件中所有".c"文件加入工程,并将"src/include"添加到工程包含(include paths)即可;
> - 使用时只需包含"XCOS.h"文件;

### 4.1. XCOS的配置

> - 配置数据在"XC_Cnf.h"文件中定义;

XCOS的配置参数如下:

| 类型名 | 配置类型 | 说明 |
| --- | --- | --- |
| XCuint_t | 宏 | XCOS基础数据类型,默认32位(uint32_t) |
| _XC_Cnf_TaskMaxNum | 宏 | 定义最大允许任务数(默认100)不建议修改 |
| _XC_SysTickPerScond | 宏 | 系统每秒滴答数(滴答计数频率,默认1ms) |
| _XC_SysTickCount | 宏 | 系统滴答计数 |

**"_XC_SysTickPerScond"系统每秒滴答数(滴答计数频率)说明**
- 单位Hz,是系统最小的时间单位;
- 默认值是"1000"也就是1ms;
- 最大值是1000000Hz;对应时间是1us;

**"_XC_SysTickCount"系统滴答计数说明**
- 滴答计数是一个累加值,累加时间必须和"_XC_SysTickPerScond"时间一致;
- 此值会在操作中被读取(只读),作为时间计算的依据;
- "_XC_CreateSysTickCount"用于创建一个全局计数变量;
- "XC_AccSysTickCount()"用户计数中断中调用,计数+1;

**系统滴答计数可用2种方式实现:**

***方式1:中断累加形式(默认);***
- 用一个变量在定时器中累加,每一个滴答时间则+1;
- 宏"_XC_SysTickCount"指向此变量即可;
> 用户需要处理:
> 1. 在工程.c文件中调用"_XC_CreateSysTickCount()",以定义一个全局变量(本质是定义一个"volatile uint32_t"类型的全局变量"g_SysTickCount");
> 2. 创建一个计数定时器,定时器按"_XC_SysTickPerScond"周期计数;
> 3. 在定时器服务中调用"XC_AccSysTickCount()"(本质是"g_SysTickCount"累加);

***方式2:计数器形式;***
- 因为协程并不需要中断来切换上下文,所以为了使效率最高可以用一个计数器来做系统滴答计数;
- 宏"_XC_SysTickCount"作为一个计数器的函数的返回值(或其本身寄存器值);
> 用户需要处理:
> 1. 创建定时器,按"_XC_SysTickPerScond"计数;
> 2. 将"_XC_SysTickCount"宏指向定时器的计数值;
>
> - *注1*: 这里定时器尽量使用32位变量;
> - *注2*: 宏"_XC_SysTickCount"会频繁只读调用,注意寄存器读取效率;

### 4.2. 调用说明

- 句柄类型,常量,函数表见[XCOS](./docs/XCOS.md)说明;
- 详细见[例子](./docs/examples.md)说明;

## 5.文件说明

| 文件夹 | 文件 | 说明 |
| :--- | --- |  --- |
| ./src | - | 核心源码文件夹 |
| ./src | XC_List.c | 链表实现 |
| ./src | [XC_Sch.c](./docs/XCOS.md) | 调度处理 |
| ./src | [XC_Task.c](./docs/XCOS.md) | 任务处理 |
| ./src | [XC_Sem.c](./docs/XCOS.md) | 信号量实现 |
| ./src | [XC_Time.c](./docs/XC_Type.md) | 时间戳,日历,秒转换计算处理 |
| ./src/include | - | 源码".h"文件,需包含 |
| ./src/include | BinData.h | 二进制数值宏定义 |
| ./src/include | COR_ANSI.h | 协程底层实现("ANSI-C"是由"switch case"实现) |
| ./src/include | COR_GNU.h | 协程底层实现("GNU-C" 是由"goto label" 实现) |
| ./src/include | CortexMx.h | 对应CortexMx使用的宏定义 |
| ./src/include | [XC_Type.h](./docs/XC_Type.md) | 通用数据类型定义 |
| ./src/include | [XC_Time.h](./docs/XC_Time.md) | 时间处理-头文件 |
| ./src/include | [XC_TimeCompatibility.h](./docs/XC_Type.md) | 时间处理,兼容性代码 |
| ./src/include | [XC_TimeCompile.h](./docs/XC_Type.md) | 时间处理,编译相关代码 |
| ./src/include | [XC_TimeCount.h](./docs/XC_Type.md) | 时间处理,计数处理相关代码 |
| ./src/include | [XC_MacroFunc.h](./docs/XC_MacroFunc.md) | 通用函数宏实现 |
| ./src/include | [XC_BitFlag.h](./docs/XC_BitFlag.md) | 位标志的实现 |
| ./src/include | XC_Cnf.h | 用于存放"XCOS"的配置参数 |
| ./src/include | [XC_CPU.h](./docs/XC_Type.md) | 不同平台的数据类型,关键字统一 |
| ./src/include | XC_List.h | 链表实现 |
| ./src/include | [XC_Sch.h](./docs/XCOS.md) | 调度处理 |
| ./src/include | [XC_Sem.h](./docs/XCOS.md) | 信号量实现 |
| ./src/include | [XC_Task.h](./docs/XCOS.md) | 任务处理 |
| ./src/include | XCBase.h | 兼容"XCBase"库 |
| ./src/include | XCOS.h | 总包含 |

## 6.更新说明
见文件["XC_UpdateInfo.md"](./docs/XC_UpdateInfo.md)
