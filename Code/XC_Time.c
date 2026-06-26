/**
 * @file        XC_Time.c
 * @brief       时间处理
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0.0
 * @date        2025/10/30
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     时间计数处理
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 头文件
#include "XC_Time.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 全局变量 */

// 是中断累加模式,则定义一个全局变量
#ifdef XC_SYS_TICK_INT_INC_MODE
volatile XC_Tick_t g_SysTickCount = 0U; // 用户系统Tick
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 死循环操作 */

/**
 * @brief       [用户]死循环延时Tick个计数
 * @param[in]   DelayTick   需要延时的Tick数
 * @details     while判断死延时
 */
void XC_Time_BlockDelay(XC_Tick_t DelayTick)
{
    XC_Tick_t Tick;
    Tick = XC_Time_GetTick();
    while(!XC_Time_CheckTimeout(Tick, DelayTick)) {
        /* 等待延时到达 */
    }
}

/**
 * @brief       [用户]死循环延时ms
 * @param[in]   Delay_ms    需要延时的ms数
 * @details     while判断死延时
 */
void XC_Time_BlockDelayMs(XC_Tick_t Delay_ms)
{
    XC_Tick_t Tick;
    Tick = XC_Time_GetTick();
    while(!XC_Time_CheckTimeout(Tick, XC_Time_MsToTicks(Delay_ms))) {
        /* 等待延时到达 */
    }
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       [用户]获取剩下多少Tick
 * @param[in]   LastTick        上个记录的Tick
 * @param[in]   CompareTick     等待到达的Tick时间
 * @return      XC_Tick_t        从"LastTick"开始,离"CompareTick"还差多少个Tick,返回0表示到达或者早已到达;
 * @details     获取当前Tick离设定的值还有多少个Tick;
 */
XC_Tick_t XC_Time_GetRemain(XC_Tick_t LastTick, XC_Tick_t CompareTick)
{
    XC_Tick_t Tick;
    Tick = XC_Time_GetTick(); // 当前Tick

    if((Tick - LastTick) >= CompareTick) {
        return (0U);
    }
    return (CompareTick - (Tick - LastTick));
}

/**
 * @brief       [用户]获取运行了多少Tick
 * @param[in]   LastTick    上个记录的Tick
 * @param[in]   CompareTick 等待到达的Tick时间
 * @return      XC_Tick_t    从"LastTick"开始,到"CompareTick"个Tick已经运行了多少Tick,返回"CompareTick"表示已经运行完成或者早已运行完成;
 * @details     获取当前Tick到设定的值,已经运行了多少个Tick;
 */
XC_Tick_t XC_Time_GetElapsed(XC_Tick_t LastTick, XC_Tick_t CompareTick)
{
    XC_Tick_t Tick;
    Tick = XC_Time_GetTick(); // 当前Tick

    if((Tick - LastTick) >= CompareTick) {
        return (CompareTick);
    }
    return (Tick - LastTick);
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/**
 * @brief       [用户]获取剩下多少Tick
 * @param[in]   tTime       "XC_TimerTick_t"类型定义的时间数据;
 * @return      XC_Tick_t    返回余下Tick,返回0表示到达或者早已到达
 * @details     [扩展]从上次调用更新计算,获取当前Tick离设定的值还有多少个Tick;
 */
XC_Tick_t XC_Time_TimerGetRemain(XC_TimerTick_t tTime)
{
    XC_Tick_t Tick;
    Tick = XC_Time_GetTick(); // 当前Tick

    if((Tick - tTime.TickCount) >= tTime.WaitCount) {
        return (0U);
    }
    return (tTime.WaitCount - (Tick - tTime.TickCount));
}

/**
 * @brief       [用户]获取运行了多少Tick
 * @param[in]   tTime       "XC_TimerTick_t"类型定义的时间数据;
 * @return      XC_Tick_t    返回已经运行的Tick,若等于"tTime.WaitCount",表示时间到达或早已到达;
 * @details     [扩展]从上次调用更新计算,获取当前Tick到设定的值已经运行了多少个Tick;
 */
XC_Tick_t XC_Time_TimerGetElapsed(XC_TimerTick_t tTime)
{
    XC_Tick_t Tick;
    Tick = XC_Time_GetTick();

    if((Tick - tTime.TickCount) >= tTime.WaitCount) {
        return (tTime.WaitCount);
    }
    return (Tick - tTime.TickCount);
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
