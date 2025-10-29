/**
 * @file        XC_Time.c
 * @brief       时间处理
 * @author      libertyzx (libertyzx@163.com)
 * @version     0.1
 * @date        2025/10/17
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     时间计数处理
 * **********************************************
 *  修改日志
 *  - 2025/10/17
 *      - 初始编写
 */
//=== 头文件
#include "XC_Time.h"

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
void XCTime_BlockDelay(XCuint_t DelayTick)
{
    XCuint_t Tick;
    Tick = XCTime_GetTick();
    while(!XCTime_CompareTick(Tick, DelayTick));
}

/**
 * @brief       [用户]死循环延时ms
 * @param[in]   Delay_ms    需要延时的ms数
 * @details     while判断死延时
 */
void XCTime_BlockDelay_ms(XCuint_t Delay_ms)
{
    XCuint_t Tick;
    Tick = XCTime_GetTick();
    while(!XCTime_CompareTick(Tick, _Time_ms2Tick(Delay_ms)));
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       [用户]获取剩下多少Tick
 * @param[in]   LastTick        上个记录的Tick
 * @param[in]   CompareTick     等待到达的Tick时间
 * @return      XCuint_t        从"LastTick"开始,离"CompareTick"还差多少个Tick,返回0表示到达或者早已到达;
 * @details     获取当前Tick离设定的值还有多少个Tick;
 */
XCuint_t XCTime_GetRemainTick(XCuint_t LastTick, XCuint_t CompareTick)
{
    XCuint_t Tick;
    Tick = XCTime_GetTick(); // 当前Tick

    if((Tick - LastTick) >= CompareTick) {
        return (0);
    }
    return (CompareTick - (Tick - LastTick));
}

/**
 * @brief       [用户]获取运行了多少Tick
 * @param[in]   LastTick    上个记录的Tick
 * @param[in]   CompareTick 等待到达的Tick时间
 * @return      XCuint_t    从"LastTick"开始,到"CompareTick"个Tick已经运行了多少Tick,返回"CompareTick"表示已经运行完成或者早已运行完成;
 * @details     获取当前Tick到设定的值,已经运行了多少个Tick;
 */
XCuint_t XCTime_GetRunTick(XCuint_t LastTick, XCuint_t CompareTick)
{
    XCuint_t Tick;
    Tick = XCTime_GetTick(); // 当前Tick

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
 * @param[in]   tTime       "XCTimeCount_t"类型定义的时间数据;
 * @return      XCuint_t    返回余下Tick,返回0表示到达或者早已到达
 * @details     [扩展]从上次调用更新计算,获取当前Tick离设定的值还有多少个Tick;
 */
XCuint_t XCTime_tGetRemainTick(XCTimeCount_t tTime)
{
    XCuint_t Tick;
    Tick = XCTime_GetTick(); // 当前Tick

    if((Tick - tTime.TickCount) >= tTime.WaitCount) {
        return (0);
    }
    return (tTime.WaitCount - (Tick - tTime.TickCount));
}

/**
 * @brief       [用户]获取运行了多少Tick
 * @param[in]   tTime       "XCTimeCount_t"类型定义的时间数据;
 * @return      XCuint_t    返回已经运行的Tick,若等于"tTime.WaitCount",表示时间到达或早已到达;
 * @details     [扩展]从上次调用更新计算,获取当前Tick到设定的值已经运行了多少个Tick;
 */
XCuint_t XCTime_tGetRunTick(XCTimeCount_t tTime)
{
    XCuint_t Tick;
    Tick = _XC_SysTickCount;

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
