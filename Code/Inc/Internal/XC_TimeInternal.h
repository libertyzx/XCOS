/**
 * @file        XC_TimeInternal.h
 * @brief       内部时间处理
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2025/11/26
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
#ifndef _XC_TimeInternal_h_
#define _XC_TimeInternal_h_
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

#define _XC_TICK_PERIOD_MS     (1000U / XC_CFG_TICKS_PER_SEC) // 系统滴答计数时基(单位:ms)
#define _XC_TICK_PERIOD_US     (_XC_TICK_PERIOD_MS * 1000U)   // 系统滴答计数时基(单位:us)

/**
 *  将时间转换成Tick(1个时间是多少Tick)
 *  注意:使用"XCTime_UsToTicks"时,时间误差会非常大,tick是ms级的,us时间处理按1个tick处理;
 */
#define XC_Time_UsToTicks(_t)  (((_t) + _XC_TICK_PERIOD_MS * 1000U - 1U) / (_XC_TICK_PERIOD_MS * 1000U)) /*(t)us 转换成Tick*/
#define XC_Time_MsToTicks(_t)  (((_t) + _XC_TICK_PERIOD_MS - 1U) / _XC_TICK_PERIOD_MS)                   /*(t)ms 转换成Tick*/
#define XC_Time_SecToTicks(_t) ((_t) * XC_CFG_TICKS_PER_SEC)                                             /*(t)s  转换成Tick*/

/**将Tick转换成时间(1个tick等于多少个时间)*/
#define XC_Time_TicksToUs(_c)  ((_c) * _XC_TICK_PERIOD_MS * 1000U) /*(c)转换成us*/
#define XC_Time_TicksToMs(_c)  ((_c) * _XC_TICK_PERIOD_MS)         /*(c)转换成ms*/
#define XC_Time_TicksToSec(_c) ((_c) / XC_CFG_TICKS_PER_SEC)       /*(c)转换成s*/

/**
 *  (XC_CFG_TICKS_PER_SEC <= 1000000),表示时基单位是1us到999us
 */
#elif (XC_CFG_TICKS_PER_SEC <= 1000000)

#define _XC_TICK_PERIOD_US     (1000000U / XC_CFG_TICKS_PER_SEC)                       // 系统滴答计数时基(单位:us)

/*将时间转换成Tick(1个时间是多少Tick)*/
#define XC_Time_UsToTicks(_t)  (((_t) + _XC_TICK_PERIOD_US - 1U) / _XC_TICK_PERIOD_US) /*(t)us 转换成Tick*/
#define XC_Time_MsToTicks(_t)  ((_t) * XC_CFG_TICKS_PER_SEC)                           /*(t)ms 转换成Tick*/
#define XC_Time_SecToTicks(_t) (XC_Time_MsToTicks(_t) * 1000U)                         /*(t)s  转换成Tick*/

/**将Tick转换成时间(1个tick等于多少个时间)*/
#define XC_Time_TicksToUs(_c)  ((_c) * _XC_TICK_PERIOD_US)                             /*(c)转换成us*/
#define XC_Time_TicksToMs(_c)  ((_c) / XC_CFG_TICKS_PER_SEC)                           /*(c)转换成ms*/
#define XC_Time_TicksToSec(_c) (XC_Time_TicksToMs(_c) / 1000U)                         /*(c)转换成s*/

#else
#error "系统每秒滴答数[XC_CFG_TICKS_PER_SECd]设置错误!"
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 向后兼容:保留旧版宏名(已弃用,建议迁移到 XC_Time_ 前缀)
#define XCTime_UsToTicks  XC_Time_UsToTicks
#define XCTime_MsToTicks  XC_Time_MsToTicks
#define XCTime_SecToTicks XC_Time_SecToTicks
#define XCTime_TicksToUs  XC_Time_TicksToUs
#define XCTime_TicksToMs  XC_Time_TicksToMs
#define XCTime_TicksToSec XC_Time_TicksToSec

//=== 文件结束
#endif