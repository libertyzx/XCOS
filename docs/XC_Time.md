# 时间处理
## 1.描述
实现各类简单的时间处理

## 2.文件

| 文件名 | 说明 |
| --- | --- |
| XC_Time.c | 时间戳,日历,秒转换计算处理 |
| XC_Time.h | 时间处理头文件 |
| XC_TimeCompatibility.h | 时间处理,兼容性代码 |
| XC_TimeCompile.h | 时间处理,编译相关代码 |
| XC_TimeCount.h | 时间处理,计数处理相关代码 |

## 3.时间戳,日历,秒转换计算处理

>对应文件:
> - XC_Time.c
> - XC_Time.h

- 类型

| 类型名 | 32位下占用字节 | 说明 |
| --- | --- | --- |
| XCTime_Calendar_t | 8 | 结构体-日历变量 |
| XCTime_ParamTime_t | - | 枚举-时间参数 |

- 函数说明:

| 函数名 | 类型 | 说明 |
| --- | --- | --- |
| XCTime_IsLeapYear | 函数 | 闰年判断 |
| XCTime_GetLeapYearNum | 函数 | 计算两个年之间的闰年数量 |
| XCTime_SetYear_CalendarToSec | 函数 | 设置初始年,日历转秒 |
| XCTime_SetYear_SecToCalendar | 函数 | 设置初始年,秒转日历 |
| XCTime_TwoDateComputeDiff | 函数 | 计算2个日期的差值,输出秒 |
| XCTime_ThisYear | 函数 | | 输入日期,返回今年日期的x值(x=天,时,分,秒) |
| XCTime_CalculateTimeDiff | 函数 | 计算2个日期的差值-指定输出 |
| XCTime_CalendarToUnix | 函数 | 由日历计算Unix时间戳 |
| XCTime_UnixToCalendar | 函数 | 由Unix时间戳计算日历 |


## 4.编译时间相关代码

>对应文件:
> - XC_TimeCompile.h

处理编译相关的宏定义及时间,可以用作版本操作;

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

## 5.时间及计数处理相关代码

>对应文件:
> - XC_TimeCount.h

时间处理分2类,可以混合或者按例程建议使用;


### 5.1. 基础时间处理

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
> 2. "XCuint_t"表示是在配置文件里配置的类型,默认uint32_t


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

### 5.2. 扩展时间处理

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
> 2. "XCuint_t"表示在配置文件里配置,默认uint32_t

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


### 5.3. 时间和Tick的转换

时间转tick

| 宏 | 说明 |
| --- | --- |
| _Time_us2Tick(_t)  | 输入时间"_t", 单位us, 输出tick |
| _Time_ms2Tick(_t)  | 输入时间"_t", 单位ms, 输出tick |
| _Time_s2Tick(_t)   | 输入时间"_t", 单位s,  输出tick |
| _Time_min2Tick(_t) | 输入时间"_t", 单位min,输出tick |
| _Time_h2Tick(_t)   | 输入时间"_t", 单位h,  输出tick |
| _Time_day2Tick(_t) | 输入时间"_t", 单位day,输出tick |

tick转时间

| 宏 | 说明 |
| --- | --- |
| _Time_Tick2us(_c)  | 输入tick值"_c", 输出时间, 单位us  |
| _Time_Tick2ms(_c)  | 输入tick值"_c", 输出时间, 单位ms  |
| _Time_Tick2s(_c)   | 输入tick值"_c", 输出时间, 单位s   |
| _Time_Tick2min(_c) | 输入tick值"_c", 输出时间, 单位min |
| _Time_Tick2h(_c)   | 输入tick值"_c", 输出时间, 单位h   |
| _Time_Tick2day(_c) | 输入tick值"_c", 输出时间, 单位day |

---
