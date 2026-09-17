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

#define XC_TICK_PERIOD_MS (1000U / XC_CFG_TICKS_PER_SEC) // 系统滴答计数时基(单位:ms)
#define XC_TICK_PERIOD_US (XC_TICK_PERIOD_MS * 1000U)    // 系统滴答计数时基(单位:us)

/**
 * @brief   [内部]编译期断言: 本分支(<=1000Hz)要求 "XC_CFG_TICKS_PER_SEC" 是 1000 的因子
 * @details
 *  本分支以 ms 为时基("XC_TICK_PERIOD_MS = 1000/f" 是**整型除法**):
 *  - 非因子频率(如 800/600Hz)会把"每 tick"截断成整 ms => "TicksToMs"/"XC_Time_GetMs()" **系统性偏小**
 *    (800Hz 偏小 20%; 600Hz 偏小 40%);
 *  - 而 "MsToTicks" 是**向上取整到整 tick** => 延时偏长(1 tick 可能是数 ms: 300Hz 下请求 1ms 实际 3.33ms);
 *  支持频率: **1/2/4/5/8/10/20/25/40/50/100/125/200/250/500/1000 Hz**
 *  (>1000Hz 走 us 时基分支, 取值任意);
 *  本断言用"负数组长度"实现: 不满足即编译报错, 且**不产生代码/数据**(0 开销);
 */
typedef char XC_StaticAssert_TicksPerSecDivisor[(1000U % XC_CFG_TICKS_PER_SEC) == 0U ? 1 : -1];

/**
 *  将时间转换成Tick(1个时间是多少Tick)
 *  注意:使用"XC_Time_UsToTicks"时,时间误差会非常大,tick是ms级的,us时间处理按1个tick处理;
 * ---
 *  "XC_Time_SecToTicks"是"秒 × 频率"直乘(结果为 32 位 Tick),可用**秒数上限** ≈
 *  "XC_Tick_t的最大值 / XC_CFG_TICKS_PER_SEC"(默认 uint32_t ⇒ 1000Hz 约 429 万秒 ≈ 49.7 天);
 *  超过上限会**整型回绕**,且回绕结果**必然小于**请求值 ⇒ 延时/超时都**偏短**
 *  (例: 1000Hz 下请求 4295000 秒, 实际只得到 32704 Tick ≈ 32.7 秒);
 *  大延时请"分段"(多次延时)或改用 "XC_Cor_DelayTick"(直接给 Tick 数, 不经过换算);
 *  注: 跨 Tick 回绕的**延时本身是支持的**(由时间溢出表处理),受限的只是"秒 × 频率"这一次乘法;
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

#define XC_TICK_PERIOD_US        (1000000U / XC_CFG_TICKS_PER_SEC)                                 // 系统滴答计数时基(单位:us)

/*将时间转换成Tick(1个时间是多少Tick)
 *  - 本分支时基为 us(1 tick < 1ms): 统一以"us"为中间量, 与"<=1000Hz"分支保持一致的取整语义
 *    (时间 -> Tick **向上取整**);
 *  - SecToTicks 直接乘频率(精确), ms/us 走 us 换算;
 *  - 注意: "Time * 1000U" 在 ms 很大时会溢出(32 位下约 4.29e6 ms);
 *    SecToTicks(Time) = Time * f 的可用上限约为 "XC_TICK_MAX / f"(1000Hz ⇒ 约 49.7 天);
 */
#define XC_Time_UsToTicks(Time)  (((Time) + XC_TICK_PERIOD_US - 1U) / XC_TICK_PERIOD_US)           /*(Time)us 转换成Tick*/
#define XC_Time_MsToTicks(Time)  ((((Time) * 1000U) + XC_TICK_PERIOD_US - 1U) / XC_TICK_PERIOD_US) /*(Time)ms 转换成Tick*/
#define XC_Time_SecToTicks(Time) ((Time) * XC_CFG_TICKS_PER_SEC)                                   /*(Time)s  转换成Tick*/

/**将Tick转换成时间(1个tick等于多少个时间)
 *  - Tick -> 时间 **向下取整**: 1 tick < 1ms 时 TicksToMs 可能得 0(精度下限), 需要更细用 TicksToUs;
 */
#define XC_Time_TicksToUs(Tick)  ((Tick) * XC_TICK_PERIOD_US)                                      /*(Tick)转换成us*/
#define XC_Time_TicksToMs(Tick)  (((Tick) * XC_TICK_PERIOD_US) / 1000U)                            /*(Tick)转换成ms*/
#define XC_Time_TicksToSec(Tick) (XC_Time_TicksToMs(Tick) / 1000U)                                 /*(Tick)转换成s*/

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
