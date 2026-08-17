/**
 * @file        XC_TimeInternal.h
 * @brief       内部时间处理
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     存放框架内部时间处理的相关代码
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_TimeInternal_h
#define XC_TimeInternal_h
//=== 头文件
#include "XC_Config.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**
 * 滴答计数频率转换
 * "XC_CFG_TICKS_PER_SEC"在"XC_Config.h"文件定义;
 */

/**
 * (XC_CFG_TICKS_PER_SEC <= 1000),表示时基单位是1ms到1000ms
 */
#if (XC_CFG_TICKS_PER_SEC <= 1000)

#define XC_TICK_PERIOD_MS        (1000U / XC_CFG_TICKS_PER_SEC) // 系统滴答计数时基(单位:ms)
#define XC_TICK_PERIOD_US        (XC_TICK_PERIOD_MS * 1000U)    // 系统滴答计数时基(单位:us)

/**
 *  将时间转换成Tick(1个时间是多少Tick)
 *  注意:使用"XC_Time_UsToTicks"时,时间误差会非常大,tick是ms级的,us时间处理按1个tick处理;
 */
#define XC_Time_UsToTicks(Time)  (((Time) + XC_TICK_PERIOD_MS * 1000U - 1U) / (XC_TICK_PERIOD_MS * 1000U)) /*(Time)us 转换成Tick*/
#define XC_Time_MsToTicks(Time)  (((Time) + XC_TICK_PERIOD_MS - 1U) / XC_TICK_PERIOD_MS)                   /*(Time)ms 转换成Tick*/
#define XC_Time_SecToTicks(Time) ((Time) * XC_CFG_TICKS_PER_SEC)                                           /*(Time)s  转换成Tick*/

/**将Tick转换成时间(1个tick等于多少个时间)*/
#define XC_Time_TicksToUs(Tick)  ((Tick) * XC_TICK_PERIOD_MS * 1000U) /*(Tick)转换成us*/
#define XC_Time_TicksToMs(Tick)  ((Tick) * XC_TICK_PERIOD_MS)         /*(Tick)转换成ms*/
#define XC_Time_TicksToSec(Tick) ((Tick) / XC_CFG_TICKS_PER_SEC)      /*(Tick)转换成s*/

/**
 *  (XC_CFG_TICKS_PER_SEC <= 1000000),表示时基单位是1us到999us
 */
#elif (XC_CFG_TICKS_PER_SEC <= 1000000)

#define XC_TICK_PERIOD_US        (1000000U / XC_CFG_TICKS_PER_SEC)                       // 系统滴答计数时基(单位:us)

/*将时间转换成Tick(1个时间是多少Tick)*/
#define XC_Time_UsToTicks(Time)  (((Time) + XC_TICK_PERIOD_US - 1U) / XC_TICK_PERIOD_US) /*(Time)us 转换成Tick*/
#define XC_Time_MsToTicks(Time)  ((Time) * XC_CFG_TICKS_PER_SEC)                         /*(Time)ms 转换成Tick*/
#define XC_Time_SecToTicks(Time) (XC_Time_MsToTicks(Time) * 1000U)                       /*(Time)s  转换成Tick*/

/**将Tick转换成时间(1个tick等于多少个时间)*/
#define XC_Time_TicksToUs(Tick)  ((Tick) * XC_TICK_PERIOD_US)                            /*(Tick)转换成us*/
#define XC_Time_TicksToMs(Tick)  ((Tick) / XC_CFG_TICKS_PER_SEC)                         /*(Tick)转换成ms*/
#define XC_Time_TicksToSec(Tick) (XC_Time_TicksToMs(Tick) / 1000U)                       /*(Tick)转换成s*/

#else
#error "系统每秒滴答数[XC_CFG_TICKS_PER_SEC]设置错误!"
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

//=== 文件结束
#endif
