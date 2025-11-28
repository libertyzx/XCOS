/**
 * @file        XC_TimeInternal.h
 * @brief       内部时间处理
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/11/26
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     存放框架内部时间处理的相关代码
 * **********************************************
 *  修改日志
 *  - 见"XC_UpdateInfo.md"的更新说明;
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

#define _XC_TICK_PERIOD_MS    (1000 / XC_CFG_TICKS_PER_SEC) // 系统滴答计数时基(单位:ms)
#define _XC_TICK_PERIOD_US    (_XC_TICK_PERIOD_MS * 1000)   // 系统滴答计数时基(单位:us)

/**
 *  将时间转换成Tick(1个时间是多少Tick)
 *  注意:使用"XCTime_UsToTicks"时,时间误差会非常大,tick是ms级的,us时间处理按1个tick处理;
 */
#define XCTime_UsToTicks(_t)  (((_t) + _XC_TICK_PERIOD_MS * 1000 - 1) / (_XC_TICK_PERIOD_MS * 1000)) /*(t)us 转换成Tick*/
#define XCTime_MsToTicks(_t)  (((_t) + _XC_TICK_PERIOD_MS - 1) / _XC_TICK_PERIOD_MS)                 /*(t)ms 转换成Tick*/
#define XCTime_SecToTicks(_t) ((_t) * XC_CFG_TICKS_PER_SEC)                                          /*(t)s  转换成Tick*/

/**将Tick转换成时间(1个tick等于多少个时间)*/
#define XCTime_TicksToUs(_c)  ((_c) * _XC_TICK_PERIOD_MS * 1000) /*(c)转换成us*/
#define XCTime_TicksToMs(_c)  ((_c) * _XC_TICK_PERIOD_MS)        /*(c)转换成ms*/
#define XCTime_TicksToSec(_c) ((_c) / XC_CFG_TICKS_PER_SEC)      /*(c)转换成s*/

/**
 *  (XC_CFG_TICKS_PER_SEC <= 1000000),表示时基单位是1us到999us
 */
#elif (XC_CFG_TICKS_PER_SEC <= 1000000)

#define _XC_TICK_PERIOD_US    (1000000 / XC_CFG_TICKS_PER_SEC)                       // 系统滴答计数时基(单位:us)

/*将时间转换成Tick(1个时间是多少Tick)*/
#define XCTime_UsToTicks(_t)  (((_t) + _XC_TICK_PERIOD_US - 1) / _XC_TICK_PERIOD_US) /*(t)us 转换成Tick*/
#define XCTime_MsToTicks(_t)  ((_t) * XC_CFG_TICKS_PER_SEC)                          /*(t)ms 转换成Tick*/
#define XCTime_SecToTicks(_t) (XCTime_MsToTicks(_t) * 1000)                          /*(t)s  转换成Tick*/

/**将Tick转换成时间(1个tick等于多少个时间)*/
#define XCTime_TicksToUs(_c)  ((_c) * _XC_TICK_PERIOD_US)                            /*(c)转换成us*/
#define XCTime_TicksToMs(_c)  ((_c) / XC_CFG_TICKS_PER_SEC)                          /*(c)转换成ms*/
#define XCTime_TicksToSec(_c) (XCTime_TicksToMs(_c) / 1000)                          /*(c)转换成s*/

#else
#error "系统每秒滴答数[XC_CFG_TICKS_PER_SECd]设置错误!"
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
