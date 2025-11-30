/*=========================================================|
 | 文件名: XC_TimeCompatibility.h
 | 描述  : 时间处理的兼容性代码
 | 版本  : V1.00
 | 日期  : 2024/08/12
 | 语言  : C语言
 | 作者  : libertyzx
 | E-mail: libertyzx@163.com
 +-----------------------------------------------|
 | 开源协议: MIT License
 +-----------------------------------------------|
 +--- 说明
 |  1.所有时间处理的兼容性处理代码;
 +-----------------------------------------------|
 +--- 版本说明
 |  V1.00:-2024/08/12
 |      1.从"XC_Time.h"中独立出;
 *========================================================*/
//=== 防重复定义
#ifndef _XC_TimeCompatibility_H_
#define _XC_TimeCompatibility_H_

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/**基础时间处理
 * 文件:"XC_TimeCount.h"
 * 兼容基础库"XCBase.h"
 */

#define _Time_GetTickUnit     XCTime_GetTickUnit // 获取系统Tick最小时间(单位:us)
#define _Time_GetTick         XCTime_GetTick     // 获取系统Tick

/*获取系统Tick加上指定时间后的Tick*/
#define _Time_GetTick_ms      XCTime_GetTick_ms
#define _Time_GetTick_s       XCTime_GetTick_s
#define _Time_GetTick_min     XCTime_GetTick_min
#define _Time_GetTick_h       XCTime_GetTick_h
#define _Time_GetTick_day     XCTime_GetTick_day

#define _Time_CompareTick     XCTime_CompareTick // 比较Tick是否到达设定值

/*比较时间是否到达设定值*/
#define _Time_CompareTick_ms  XCTime_CompareTick_ms
#define _Time_CompareTick_s   XCTime_CompareTick_s
#define _Time_CompareTick_min XCTime_CompareTick_min
#define _Time_CompareTick_h   XCTime_CompareTick_h
#define _Time_CompareTick_Day XCTime_CompareTick_Day

#define _Time_CompareTick_t   XCTime_CompareTickt // 比较当前Tick
#define _Time_Delay           XCTime_Delay        // 死循环延时Tick个计数

/*死循环延时*/
#define _Time_Delay_ms        XCTime_Delay_ms
#define _Time_Delay_s         XCTime_Delay_s
#define _Time_Delay_min       XCTime_Delay_min
#define _Time_Delay_h         XCTime_Delay_h
#define _Time_Delay_day       XCTime_Delay_day

#define _Time_GetRemainTick   XCTime_GetRemainTick //[内联]获取剩下多少Tick
#define _Time_GetRunTick      XCTime_GetRunTick    //[内联]获取运行了多少Tick

/************************************************ 我是分割线 ************************************************/

/**扩展时间处理
 * 兼容基础库"XCBase.h"
 */

#define _Time_tUpdateTick     XCTime_tUpdateTick // 更新需要延时比较的Tick

/*更新需要延时比较的时间*/
#define _Time_tUpdateTick_ms  XCTime_tUpdateTick_ms
#define _Time_tUpdateTick_s   XCTime_tUpdateTick_s
#define _Time_tUpdateTick_min XCTime_tUpdateTick_min
#define _Time_tUpdateTick_h   XCTime_tUpdateTick_h
#define _Time_tUpdateTick_Day XCTime_tUpdateTick_Day

#define _Time_tRepeatLastTick XCTime_tRepeatLastTick // 重复更新上次需要延时比较的时间
#define _Time_tCompareTick    XCTime_tCompareTick    // 判断需要比较或延时的时间是否到达
#define _Time_tDelay          XCTime_tDelay          // 死循环延时n个Tick计数

/*死循环延时n个时间*/
#define _Time_tDelay_ms       XCTime_tDelay_ms
#define _Time_tDelay_s        XCTime_tDelay_s
#define _Time_tDelay_min      XCTime_tDelay_min
#define _Time_tDelay_h        XCTime_tDelay_h
#define _Time_tDelay_day      XCTime_tDelay_day

#define _Time_tGetTickCount   XCTime_tGetTickCount // 获取类型中的Tick计数值
#define _Time_tSetTickCount   XCTime_tSetTickCount // 设置类型中的Tick计数值
#define _Time_tClrTickCount   XCTime_tClrTickCount // 清除类型中的Tick计数值

#define _Time_tGetWaitCount   XCTime_tGetWaitCount  // 获取类型中的等待计数值
#define _Time_tSetWaitCount   XCTime_tSetWaitCount  // 设置类型中的等待计数值
#define _Time_tClrWaitCount   XCTime_tClrWaitCount  // 清除类型中的等待计数值
#define _Time_tClrAllCount    XCTime_tClrAllCount   // 清除类型中所有计数
#define _Time_tGetRemainTick  XCTime_tGetRemainTick //[内联]获取剩下多少Tick
#define _Time_tGetRunTick     XCTime_tGetRunTick    //[内联]获取运行了多少Tick

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
