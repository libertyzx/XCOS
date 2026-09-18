/**
 * @file        XC_Time.h
 * @brief       时间计数的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.1
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     时间计数实现的相关代码
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_Time_h
#define XC_Time_h
//=== 头文件
#include "Internal/XC_TimeInternal.h"
#include "XC_Type.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 获取基础时间 */

/**
 * @brief       [用户]获取系统Tick最小时间(单位:us)
 * @return      uint32_t    返回当前系统嘀嗒计数(Tick)最小时间(单位:us);
 * @details     即是一次计数过去了多少时间;
 */
#define XC_Time_GetTickUnit() (XC_TICK_PERIOD_US)

/**
 * @brief       [用户]获取系统Tick
 * @return      XC_Tick_t    返回当前系统嘀嗒计数(Tick)值;
 * @details     获取的是一个全局变量
 */
#define XC_Time_GetTick()     (XC_SYS_TICK_COUNT)

/**
 * @brief       [用户]获取系统运行的时间(ms)
 * @return      XC_Tick_t    返回当前系统运行的ms值;
 * @details
 *  获取系统当前运行时间;
 *  注意是32位值,在ms计数的情况下最长记录时间是约是49天;
 */
#define XC_Time_GetMs()       (XC_Time_TicksToMs(XC_Time_GetTick()))

/************************************************ 我是分割线 ************************************************/

/**
 *  系统嘀嗒计数处理;
 *  在"XC_SYS_TICK_COUNT"配置为默认状态下有效;
 */

#ifdef XC_SYS_TICK_INT_INC_MODE

/**
 * @brief   [用户]系统滴答计数(全局累加值)
 * @details
 *  定义在 `Code/Src/XC_Time.c`, 与下面的 `XC_Time_TickInc`/`XC_Time_TickSet` **同受本 `#ifdef` 约束**:
 *  - **方式1**(中断累加, 默认): 该变量存在, 由 `XC_Time_TickInc()` 在定时器中断里递增;
 *  - **方式2**(计数器模式): `XC_SYS_TICK_COUNT` 指向用户自己的计数器(函数返回值/寄存器), 该变量**不存在**
 *    ⇒ 声明也必须收在本条件内, 否则会留下一个"指向不存在变量"的悬空声明(用户提问点);
 *  @note 声明从 `XC_Config.h` 移到本文件: 配置头只放配置, 不再承担类型/变量声明
 */
extern volatile XC_Tick_t g_SysTickCount; // 外部声明全局滴答时间计数

/**
 * @brief   [用户]递增系统Tick
 * @details 在中断中调用,XC_CFG_TICKS_PER_SEC"频率计数;
 */
#define XC_Time_TickInc() \
    do {                  \
        g_SysTickCount++; \
    } while(0)

/**
 * @brief   [用户]设置系统Tick
 * @param[in]   _Tick   设置的Tick
 * @details
 *  特殊情况下使用,如休眠唤醒后重新设置系统滴答时间计数;
 */
#define XC_Time_TickSet(_Tick)    \
    do {                          \
        g_SysTickCount = (_Tick); \
    } while(0)

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**
 * 基础时间比较
 *
 *  使用方式(例程):
 *  - 允许出现"Tick"溢出,溢出后时间计算仍然是对的;
 *  - 这里时间计数限制是:延时等待时间+每次查询Tick是否到达时间,不能超过设置的类型值大小;
 *  ```c
 *  XC_Tick_t Lc;
 *  XC_Tick_t Cache32;
 *  Lc  = XC_Time_GetTick();      //得到当前系统Tick
 *  while(1){
 *      if(XC_Time_CheckTimeout(Lc, 1000)){
 *          //等待>=1000个Tick后会运行这里;
 *      }
 *      Cache32 = XC_Time_GetRemain(Lc,1000);    //从Lc开始,离1000个Tick还差多少个Tick
 *      Cache32 = XC_Time_GetElapsed(Lc,1000);   //从Lc开始,到1000个Tick已经运行了多少Tick
 *      if(XC_Time_CheckTimeoutMs(Lc, 700)){
 *          //等待>=700ms后会运行这里;
 *      }
 *  }
 *  ```
 * ---
 *  注:在时间处理上不要用以下方式处理,在Tick溢出后会出错;
 *  ```c
 *  XC_Tick_t Lct;
 *  Lct = XC_Time_GetTick() + 100;           //获取系统Tick+100;
 *  while(1){
 *      if(XC_Time_GetTick() >= (Lct)){      //得到Tick+100后运行;
 *          //等待超过100个Tick后运行;
 *      }
 *  }
 *  ```
 */

/**
 * @brief       [用户]比较Tick是否到达设定值
 * @param[in]   _Lc     上个记录的Tick
 * @param[in]   _c      需要比较的Tick个数
 * @return      bool    1:时间到达;0:时间没有到达;
 * @details     判断是否运行了"_c"个Tick,用于判断时间是否到达;
 */
#define XC_Time_CheckTimeout(_Lc, _c)    ((XC_Time_GetTick() - (_Lc)) >= (_c))

/**
 * @brief       [用户]比较时间是否到达设定值(单位:ms)
 * @param[in]   _Lc     上个记录的Tick
 * @param[in]   _t      需要比较时间
 * @return      bool    1:时间到达;0:时间没有到达;
 * @details     判断是否运行了"_t"个时间,用于判断时间是否到达;
 */
#define XC_Time_CheckTimeoutMs(_Lc, _t)  (XC_Time_CheckTimeout((_Lc), XC_Time_MsToTicks(_t)))

/**
 * @brief       [用户]比较时间是否到达设定值(单位:s)
 * @param[in]   _Lc     上个记录的Tick
 * @param[in]   _t      需要比较时间(单位:s)
 * @return      bool    1:时间到达;0:时间没有到达;
 * @details
 *  判断是否运行了"_t"个时间,用于判断时间是否到达;
 * @note
 *  秒数上限 ≈ "XC_Tick_t最大值 / XC_CFG_TICKS_PER_SEC"(默认 uint32_t ⇒ 1000Hz 约 49.7 天);
 *  超限会整型回绕,且回绕结果**必然偏小** ⇒ 判为"到达"的时刻**比预期早**(与延时类接口一致);
 *  需要更长/更精确的超时请改用 "XC_Time_CheckTimeout"(直接给 Tick 数);
 */
#define XC_Time_CheckTimeoutSec(_Lc, _t) (XC_Time_CheckTimeout((_Lc), XC_Time_SecToTicks(_t)))

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       [用户]获取剩下多少Tick
 * @param[in]   LastTick        上个记录的Tick
 * @param[in]   CompareTick     等待到达的Tick时间
 * @return      XC_Tick_t        从"LastTick"开始,离"CompareTick"还差多少个Tick,返回0表示到达或者早已到达;
 * @details     获取当前Tick离设定的值还有多少个Tick;
 */
XC_Tick_t XC_Time_GetRemain(XC_Tick_t LastTick, XC_Tick_t CompareTick);

/**
 * @brief       [用户]获取运行了多少Tick
 * @param[in]   LastTick    上个记录的Tick
 * @param[in]   CompareTick 等待到达的Tick时间
 * @return      XC_Tick_t    从"LastTick"开始,到"CompareTick"个Tick已经运行了多少Tick,返回"CompareTick"表示已经运行完成或者早已运行完成;
 * @details     获取当前Tick到设定的值,已经运行了多少个Tick;
 */
XC_Tick_t XC_Time_GetElapsed(XC_Tick_t LastTick, XC_Tick_t CompareTick);

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**
 *  扩展时间处理
 *  相比基础时间处理,扩展时间处理增加了数据类型,简化处理;
 *  使用方式(例程):
 *  - "XC_TimerTick_t"类型由2个变量组成,简化"时间处理1"的操作;
 *  - 允许出现"Tick"溢出,溢出后时间计算仍然是对的;
 *  - 这里时间计数限制是:延时等待时间+每次查询Tick是否到达时间,不能超过设置的类型值大小;
 *  ```c
 *  XC_TimerTick_t tCount;
 *  XC_Time_TimerSetMs(tCount, 100);     //更新比较的Tick值为100ms
 *  while(1){
 *      if(XC_Time_TimerCheck(tCount)){
 *          //等待超过100ms后运行;
 *      }
 *  }
 *  ```
 */

/**
 * @brief       [用户]更新需要延时比较的Tick
 * @param[in]   _tC     "XC_TimerTick_t"类型定义的数据;
 * @param[in]   _c      更新比较的Tick计数
 * @details     [扩展]更新需要比较或者延时的Tick数
 */
#define XC_Time_TimerSet(_tC, _c)    ((_tC).TickCount = XC_Time_GetTick(), (_tC).WaitCount = (_c))

/**
 * @brief       [用户]更新需要延时比较的时间(ms)
 * @param[in]   _tC     "XC_TimerTick_t"类型定义的数据;
 * @param[in]   _t      更新比较的时间
 * @details     [扩展]更新需要比较或者延时的时间
 */
#define XC_Time_TimerSetMs(_tC, _t)  XC_Time_TimerSet((_tC), XC_Time_MsToTicks(_t)) /*更新:_t毫秒*/

/**
 * @brief       [用户]更新需要延时比较的时间(s)
 * @param[in]   _tC     "XC_TimerTick_t"类型定义的数据;
 * @param[in]   _t      更新比较的时间
 * @details     [扩展]更新需要比较或者延时的时间
 */
#define XC_Time_TimerSetSec(_tC, _t) XC_Time_TimerSet((_tC), XC_Time_SecToTicks(_t)) /*更新:_t秒*/

/**
 * @brief       [用户]重复更新上次需要延时比较的时间
 * @param[in]   _tC     "XC_TimerTick_t"类型定义的数据;
 * @details     [扩展]重复更新上次需要比较或者延时的时间;
 */
#define XC_Time_TimerRepeat(_tC)     ((_tC).TickCount = XC_Time_GetTick())

/**
 * @brief       [用户]判断需要比较或延时的时间是否到达
 * @param[in]   _tC     "XC_TimerTick_t"类型定义的数据;
 * @return      bool    1时间达到;0时间没有到达;
 * @details     [扩展]判断需要比较或延时的时间是否到达
 */
#define XC_Time_TimerCheck(_tC)      ((XC_Time_GetTick() - ((_tC).TickCount)) >= ((_tC).WaitCount))

/**
 * @brief       [用户]清除类型中所有计数
 * @param[in]   _tC     "XC_TimerTick_t"类型定义的数据;
 * @details     [扩展]"XC_TimerTick_t"类型数据全清零;
 */
#define XC_Time_TimerClr(_tC)        ((_tC).TickCount = 0, (_tC).WaitCount = 0)

/************************************************ 我是分割线 ************************************************/
/**
 * @brief       [用户]获取剩下多少Tick
 * @param[in]   tTime       "XC_TimerTick_t"类型定义的时间数据;
 * @return      XC_Tick_t    返回余下Tick,返回0表示到达或者早已到达
 * @details     [扩展]从上次调用更新计算,获取当前Tick离设定的值还有多少个Tick;
 */
XC_Tick_t XC_Time_TimerGetRemain(XC_TimerTick_t tTime);

/**
 * @brief       [用户]获取运行了多少Tick
 * @param[in]   tTime       "XC_TimerTick_t"类型定义的时间数据;
 * @return      XC_Tick_t    返回已经运行的Tick,若等于"tTime.WaitCount",表示时间到达或早已到达;
 * @details     从上次调用更新计算,获取当前Tick到设定的值已经运行了多少个Tick;
 */
XC_Tick_t XC_Time_TimerGetElapsed(XC_TimerTick_t tTime);

/**
 * @brief       [用户]获取运行了多少ms
 * @param[in]   tTime       "XC_TimerTick_t"类型定义的时间数据;
 * @return      XC_Tick_t    返回已经运行的ms,若等于"tTime.WaitCount",表示时间到达或早已到达;
 * @details     [扩展]从上次调用更新计算,获取当前Tick到设定的值已经运行了多少ms;
 */
#define XC_Time_TimerGetElapsedMs(_tC) XC_Time_TicksToMs(XC_Time_TimerGetElapsed(_tC)) // 获取运行了多少ms

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 死循环延时 */

/**
 * @brief       [用户]死循环延时Tick个计数
 * @param[in]   DelayTick   需要延时的Tick数
 * @details     while判断死延时
 */
void XC_Time_BlockDelay(XC_Tick_t DelayTick);

/**
 * @brief       [用户]死循环延时ms
 * @param[in]   Delay_ms    需要延时的ms数
 * @details     while判断死延时
 */
void XC_Time_BlockDelayMs(XC_Tick_t Delay_ms);

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

//=== 文件结束
#endif
