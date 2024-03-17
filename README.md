# XCubicOS
## 1.介绍
一个精简的协程系统调度框架;

## 2.说明
- 纯C语言实现;
- 全".h"文件形式,使用函数宏及内联实现所有功能,调用只需要包含"XCOS.h"即可;

## 3.实现
协程实现方式见我的CSDN:
[c语言协程](https://blog.csdn.net/libertyzx/article/details/126186870)

## 4.函数说明

所有函数/函数宏以XC开头,后面紧随功能类型,下划线后是功能说明

### 4.0. 文件说明

| 文件 | 说明 |
| --- | --- |
| BinData.h | 二进制数值宏定义 |
| COR_ANSI.h | 协程底层实现("ANSI-C"是由"switch case"实现) |
| COR_GNU.h | 协程底层实现("GNU-C" 是由"goto label" 实现) |
| CortexMx.h | 对应CortexMx使用的宏定义 |
| XC_Cnf.h | 用于存放"XCOS"的配置参数 |
| XC_Type.h | XCOS中使用的所有通用数据类型 |
| XC_MacroFunc.h | 通用函数宏定义 |
| XC_CPU.h | 编译器及内核适配 |
| XC_Time.h | 所有时间处理,及编译时间输出 |
| XC_Core.h | 协程核心 |
| XC_List.c | 链表处理 |
| XC_List.h | 链表处理 |
| XC_Sch.c | 调度处理 |
| XC_Sch.h | 调度处理 |

---
---
---

### 4.1. 配置说明
- 配置数据在"XC_Cnf.h"文件中定义;

XCOS的配置只有2个:
| 类型名 | 配置类型 | 说明 |
| --- | --- | --- |
| _XC_SysTickPerScond | 宏 | 系统每秒滴答数(滴答计数频率) |
| _XC_SysTickCount | 宏 | 系统滴答计数 |
| XCuint_t | 宏 | XCOS基础数据类型,默认32位 |

#### "_XC_SysTickPerScond"系统每秒滴答数(滴答计数频率)说明
- 单位Hz,是系统最小的时间单位;
- 默认值是"1000"也就是1ms;
- 最大值是1000000Hz;对应时间是1us;

#### "_XC_SysTickCount"系统滴答计数说明
- 滴答计数是一个累加值,累加时间必须和"_XC_SysTickPerScond"时间一致;
- 此值会在操作中被读取(只读),作为时间计算的依据;

系统滴答计数可用2种方式使用:

##### 方式1:中断累加形式(默认);
- 用一个变量在定时器中累加,每一个滴答时间则+1;
- 宏"_XC_SysTickCount"指向此变量即可;
> 用户需要处理:
> 1. 在".c"文件中定义"volatile uint32_t"类型的全局变量"g_SysTickCount";
> 2. 创建定时器,按"_XC_SysTickPerScond"计数;
> 3. 在定时器中累加系统滴答计数:"g_SysTickCount++";

##### 方式2:计数器形式;
- 因为协程并不需要中断来切换上下文,所以为了使效率最高可以用一个计数器来做系统滴答计数;
- 宏"_XC_SysTickCount"作为一个计数器的函数的返回值(或其本身寄存器值);
> 用户需要处理:
> 1. 创建定时器,按"_XC_SysTickPerScond"计数;
> 2. 将计数值作为"_XC_SysTickCount"的指向;

---
---
---

### 4.1. XCOS调用函数说明

基础调度函数(XC_Sch.c)

| 函数名 | 函数类型 | 说明 |
| --- | --- | --- |
| XCSch_Init | 内联 | 初始化一个调度器 |
| XCSch_TaskReg | 内联 | 注册一个任务 |
| XCSch_TaskRemove | 宏 | 移除一个任务 |
| XCSch_Run | 内联 | 运行调度器 |

协程任务处理函数(XC_Core.c)

| 函数名 | 函数类型 | 说明 |
| --- | --- | --- |
| XC_Enter | 宏 | 进入协程块 |
| XC_Leave | 宏 | 离开协程块 |
| XC_Yield | 宏 | **协程块内调用**;让出当前任务的控制权 |
| XC_DelayTick | 宏 | **协程块内调用**;延时n个Tick |
| XC_Delay_** | 宏 | **协程块内调用**;延时n个时间(**=[ms,s,min,h,day]) |


---
---
---

### 4.1. 基础类型说明
- 基础类型在"XC_Type.h"文件中定义;
- 建议使用"stdint.h"库中定义基础数据类型;

#### **附加类型**

| 类型名 | 类型的类型 | 说明 |
| --- | --- | --- |
| __RW | 宏 | 定义变量可读写(volatile) |
| __R | 宏 | 定义变量只读(volatile const) |
| __W | 宏 | 定义变量只写(volatile) |
| __REG | 宏 | 定义变量寄存器变量(register) |
| __ROM | 宏 | 定义变量在ROM中(const),在"XC_CPU.h"文件定义 |
| __INLINE | 宏 | 定义内联函数,在"XC_CPU.h"文件定义 |
| __STATIC_INLINE | 宏 | 定义私有内联函数,在"XC_CPU.h"文件定义 |
| __WEAK | 宏 | 定义弱函数,在"XC_CPU.h"文件定义 |

#### **基础类型**
注意:
> - char        固定8字节;
> - short       固定2字节;
> - long        类型在8/16/32位机下是4字节,64位机下是8字节;
> - int         类型在8/16位机下是2字节,在32/64位机下是4字节;
> - long long   类型固定8字节;
> - 指针类型固定是cpu位数的长度;

| 类型名 | 类型的类型 | 位数 | 是否有符号 | 说明 |
| --- | --- | --- | --- | --- |
| schar | 重定义 | 8 | 有 | 强制有符号8位数 |
| sshort | 重定义 | 16 | 有 | \ |
| sint | 重定义 | # | 有 | \ |
| slong | 重定义 | # | 有 | \ |
| sllong | 重定义 | 64 | 有 | 32位CPU才支持 |
| llong | 重定义 | 64 | 有 | 32位CPU才支持 |
| uchar | 重定义 | 8 | 无 | \ |
| ushort | 重定义 | 16 | 无 | \ |
| uint | 重定义 | # | 无 | \ |
| ulong | 重定义 | # | 无 | \ |
| ullong | 重定义 | 64 | 无 | 32位CPU才支持 |

>操作支持变量位数说明:
> 1. "*"表示支持所有位数(因为是宏操作);
> 2. "#"表示有cpu自动适应;
> 3. "8","16","32","64"表示各种位数;

#### **扩展类型**

| 类型名 | 类型的类型 | 说明 |
| --- | --- | --- |
| TypeDevSW | boot | 用于设备开关/使能的类型 |
| TypeVar32 | union | 共用体32位整合变量,包含有无符号变量,字节/字符类型 |

#### **扩展数据**

| 类型名 | 说明 |
| --- | --- |
| _H | 高电平("TypeDevSW"类型) |
| _L | 低电平("TypeDevSW"类型) |
| _EN | 使能("TypeDevSW"类型) |
| _DI | 除能("TypeDevSW"类型) |
| NULL | 空指针 |
| Null | 空指针 |
| null | 空指针 |

### 4.1. 控制函数

| 函数名 | 类型 | 文件 | 说明 |
| --- | --- | --- | --- |
| XC_Enter | 宏 | XC_Core.h | 在任务中调用,协程块进入 |
| XC_Leave | 宏 | XC_Core.h | 在任务中调用,协程块离开 |
| XC_Yield | 宏 | XC_Core.h | 在协程块中调用,让出当前控制 |
| XCSch_Init | 内联函数 | XC_Sch.h | 初始化一个调度器 |
| XCSch_TaskReg | 内联函数 | XC_Sch.h | 注册一个协程任务 |
| XCSch_TaskRemove | 内联函数 | XC_Sch.h | 移除一个协程任务 |
| XCSch_Run | 内联函数 | XC_Sch.h | 调度器运行,开始调度 |

### 4.2. 时间处理
时间处理在"XC_Time.h"文件中实现;

时间处理分2类,可以混合或者按例程建议使用;

---
---
#### **4.2.1. 基础时间处理**

这里的函数/宏处理基本需要一个临时变量存放当前系统的Tick;然后在设定需要比较/判断的Tick数来处理;
本质是和系统Tick计算一个时间差,用来来比较;

| 函数名 | 类型 | 位数 | 说明 |
| --- | --- | --- | --- |
| XCTime_GetTickUnit | 宏 | * | 获取系统Tick最小时间(单位:us) |
| XCTime_GetTick | 宏 | * | 获取系统Tick |
| XCTime_GetTick_** | 宏 | * | 获取系统Tick加上n(**=[ms,s,min,h,day])后的Tick |
| XCTime_CompareTick | 宏 | * | 比较Tick是否到达设定值 |
| XCTime_CompareTick_** | 宏 | * | 比较时间是否到达设定值(**=[ms,s,min,h,day]) |
| XCTime_CompareTick_t | 宏 | * | 比较当前Tick |
| XCTime_Delay | 宏 | * | 死循环延时Tick个计数 |
| XCTime_Delay_** | 宏 | * | 死循环延时(**=[ms,s,min,h,day]) |
| XCTime_GetRemainTick | 内联 | XCuint_t | 获取当前Tick离设定的值还有多少个Tick |
| XCTime_GetRunTick | 内联 | XCuint_t | 获取当前Tick到设定的值,已经运行了多少个Tick |

>操作支持变量位数说明:
> 1. "*"表示支持所有位数(因为是宏操作);
> 2. "#"表示有cpu自动适应;
> 3. "8","16","32","64"表示各种位数;

---

**例程1:**

说明:
1. 按例程调用,允许出现"Tick"溢出,溢出后时间计算仍然是对的;
2. 这里时间计数限制是:延时等待时间+每次查询Tick是否到达时间,不能超过设置的类型值大小;

```c
XCuint_t Lc;
XCuint_t Delay;
XCuint_t Cache32;
Lc  = XCTime_GetTick();      //得到当前系统Tick
while(1){
    if(XCTime_CompareTick(Lc, 1000)){
        //等待>=1000个Tick后会运行这里;
    }

    Cache32 = XCTime_GetRemainTick(Lc,1000);     //从Lc开始,离1000个Tick还差多少个Tick
    Cache32 = XCTime_GetRunTick(Lc,1000);        //从Lc开始,到1000个Tick已经运行了多少Tick

    if(XCTime_CompareTick_ms(Lc, 700){
        //等待>=700ms后会运行这里;
    }

    /** 死循环延时的时候,临时保存变量不能是"Lc",这里会直接覆盖临时变量位当前系统Tick;
     *  所有这里使用独立变量"Delay"来保存延时数据;
     *  当然,若是"Lc"已经使用完成,也可直接使用"Lc";
     */
    XCTime_Delay_ms(Delay, 10);        //死循环等待10ms
}
```

---

**例程2**

说明:
1. 按例程调用需要注意计数溢出,只要在溢出前后设定时间必然会出现时间错误;
2. 按例程调用方式适用于能定时清除Tick的应用;

```c
XCuint_t Lct;
Lct = XCTime_GetTick_ms(100);           //获取系统Tick+100ms;
while(1){
    if(XCTime_CompareTick_t(Lct)){      //得到Tick的100ms后运行;
        //等待超过100ms后运行;
    }
}
```

---

**调用建议**

> "XCTime_GetTick"可以和以下宏配合使用:
> - XCTime_CompareTick
> - XCTime_CompareTick_**
> - XCTime_GetRemainTick
> - XCTime_GetRunTick
>
> "XCTime_CompareTick_t"可以和以下宏配合使用:
> - XCTime_GetTick_**
>
> 以下函数/宏可以独立使用:
> - XCTime_Delay
> - XCTime_GetRemainTick
> - XCTime_GetRunTick

---
---

#### **4.2.2. 扩展时间处理**

不同于时间处理1,时间处理2中将需要保存Tick的临时变量和需要比较的Tick数统一成了一个结构体类型,这样更方便操作,同时也保证了Tick溢出安全;


| 数据类型 | 数据位数 | 说明 |
| --- | --- | --- |
| XCTimeCount_t | XCuint_t | 用于定义Tick时间处理的类型 |

| 函数名 | 类型 | 位数 |说明 |
| --- | --- | --- | --- |
| XCTime_tUpdateTick | 宏 | * | 更新需要比较或者延时的Tick数 |
| XCTime_tUpdateTick_** | 宏 | * | 更新需要比较或者延时的时间(**=[ms,s,min,h,day]) |
| XCTime_tRepeatLastTick | 宏 | * | 重复更新需要比较或者延时的时间 |
| XCTime_tCompareTick | 宏 | * | 判断需要比较或者延时的时间是否到达 |
| XCTime_tDelay | 宏 | * | 死循环延时n个Tick计数 |
| XCTime_tDelay_** | 宏 | * | 死循环延时n个时间(**=[ms,s,min,h,day]) |
| XCTime_tGetTickCount | 宏 | * | 获取类型中的Tick计数值 |
| XCTime_tSetTickCount | 宏 | * | 设置类型中的Tick计数值 |
| XCTime_tClrTickCount | 宏 | * | 清除类型中的Tick计数值 |
| XCTime_tGetWaitCount | 宏 | * | 获取类型中的等待计数值 |
| XCTime_tSetWaitCount | 宏 | * | 设置类型中的等待计数值 |
| XCTime_tClrWaitCount | 宏 | * | 清除类型中的等待计数值 |
| XCTime_tClrAllCount | 宏 | * | 清除类型中所有数据 |
| XCTime_tGetRemainTick | 内联 | XCuint_t | 从调用更新计算,获取当前Tick离设定的值还有多少个Tick |
| XCTime_tGetRunTick | 内联 | XCuint_t | 从调用更新计算,获取当前Tick到设定的值已经运行了多少个Tick |

>操作支持变量位数说明:
> 1. "*"表示支持所有位数(因为是宏操作);
> 2. "#"表示有cpu自动适应;
> 3. "8","16","32","64"表示各种位数;

---

**例程**

说明:
1. "XCTimeCount_t"类型由2个变量组成,简化"时间处理1"的操作;
2. 允许出现"Tick"溢出,溢出后时间计算仍然是对的;
3. 这里时间计数限制是:延时等待时间+每次查询Tick是否到达时间,不能超过设置的类型值大小;

```c
XCTimeCount_t tCount;
XCTime_tUpdateTick_ms(tCount, 100);     //更新比较的Tick值为100ms
while(1){
    if(XCTime_tCompareTick(tCount)){
        //等待超过100ms后运行;
    }
}
XCTime_tDelay_ms(tCount, 150);          //死循环延时150ms
```

---
---

#### **4.2.3. 编译相关数据**

处理编译相关的宏定义及时间,可以用作版本操作;

**通用宏定义**

| 宏 | 说明 |
| --- | --- |
| _LINE | 当前文件当前"_LINE"宏所在的行号(是数字) |
| _FILE | 当前文件的文件名(是字符串首地址) |

**编译时间相关**

| 宏 | 输出类型 | 说明 |
| --- | --- | --- |
| _Compile_GetYear | 数值 | 输出编译日期的年(4位十进制数值) |
| _Compile_GetMonth | 数值 | 输出编译日期的月(1-12月,十进制数值) |
| _Compile_GetDate | 数值 | 输出编译日期的日期天数(1-31天,十进制数值) |
| _Compile_GetHour | 数值 | 输出编译时间的小时(0-23小时,十进制数值) |
| _Compile_GetMin | 数值 | 输出编译时间的分钟(0-59分,十进制数值) |
| _Compile_GetSec | 数值 | 输出编译时间的秒(0-59秒,十进制数值) |
| _Compile_GetDateTimeStr | 字符串 | 获取一个编译日期时间的总字符串,含'\0'(输出格式是(年月是时分秒):20220803230638) |
| _Compile_GetDateDec | 数值 | 获取十进制表示的日期(8位数字,20230818表示2023年8月18号) |
| _Compile_GetTimeDec | 数值 | 获取十进制表示的时间(6位数字,100345表示10点03分45秒) |

---
---
---

## x.通用函数宏
- 通用函数宏在"XC_MacroFunc.h"文件中定义;

函数宏列表(若是不清楚使用方式可以看对应函数宏的说明)

| 宏 | 类型 | 最大数据位数 | 说明 |
| --- | --- | --- | --- |
| _Bin0Out | 宏 | 32 | 二进制0输出(低_Len位为0,剩余位为1) |
| _Bin1Out | 宏 | 32 | 二进制1输出(低_Len位为1,剩余位为0) |
| _BitClr | 宏 | 32 | 清除n位数据 |
| _BitSet | 宏 | 32 | 设置n位数据 |
| _BitWrite | 宏 | 32 | 写n位数据 |
| _BitRead | 宏 | 32 | 读n位数据(数据会位移到最低位) |
| _BitUpdate | 宏 | 32 | 更新n位数据(清除并写数据) |
| _KHz2Hz | 宏 | \ | 频率转换,KHz转Hz |
| _MHz2Hz | 宏 | \ | 频率转换,MHz转Hz |
| _GHz2Hz | 宏 | \ | 频率转换,GHz转Hz |
| _ArraySize | 宏 | \ | 获取数组元素个数 |
| _Max  | 宏 | * | 获取2个数中最大值 |
| _Min  | 宏 | * | 获取2个数中最小值 |
| _TwoN | 宏 | * |  判断一个数是不是2的n次方 |
| _ABS | 宏 | * | 获取绝对值 |
| _GetDataFromAddr** | 宏 | 8,16,32 | 从地址获取数据(**==8,16,32) |
| _Hex2Char | 宏 | \ | 16进制数据转字符 |
| _GetStructAddr | 宏 | \ | 由结构体成员获取结构体指针 |

>操作支持变量位数说明:
> 1. "*"表示支持所有位数(因为是宏操作);
> 2. "#"表示有cpu自动适应;
> 3. "8","16","32","64"表示各种位数;
> 4. "\\"表示没有位数的概念;
