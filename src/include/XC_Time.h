/**
 * @file        XC_Time.h
 * @brief       时间计数的实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/10/29
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     时间计数实现的相关代码
 * **********************************************
 *  修改日志
 *  - 2024/03/19
 *      - 版本:1.00
 *      - 从"XCBase"中继承并重构;
 *  - 2024/03/27
 *      - 版本:1.01
 *      - 兼容基础库"XCBase";
 *  - 2024/08/12
 *      - 版本:1.02
 *      - "XC_Time.h"中编译相关宏独立到"XC_TimeCompile.h";
 *      - "XC_Time.h"中系统时间计数相关宏独立到"XC_TimeCount.h";
 *      - 所有兼容处理独立到"XC_TimeCompatibility.h"文件;
 *  - 2025/10/29
 *      - 删除全部代码,再将"XC_TimeCount.h"内代码移回;
 *      - 见"XC_UpdateInfo.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_Time_H_
#define _XC_Time_H_
//=== 头文件
#include "XC_Cnf.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**
 * 滴答计数频率转换
 * "_XC_SysTickPerScond"在"XC_Cnf.h"文件定义;
 */

/**
 *  (_XC_SysTickPerScond <= 1000),表示时基单位是1ms到1000ms
 */
#if (_XC_SysTickPerScond <= 1000)

#define _XC_SysTickTime_ms (1000 / _XC_SysTickPerScond) // 系统滴答计数时基(单位:ms)
#define _XC_SysTickTime_us (_XC_SysTickTime_ms * 1000)  // 系统滴答计数时基(单位:us)
/**
 *  将时间转换成Tick(1个时间是多少Tick)
 *  注意:使用"_Time_us2Tick"时,时间误差会非常大,tick是ms级的,us时间处理按1个tick处理;
 */

#define _Time_us2Tick(_t)  (((_t) + _XC_SysTickTime_ms * 1000 - 1) / (_XC_SysTickTime_ms * 1000)) /*(t)us 转换成Tick*/
#define _Time_ms2Tick(_t)  (((_t) + _XC_SysTickTime_ms - 1) / _XC_SysTickTime_ms)                 /*(t)ms 转换成Tick*/
#define _Time_s2Tick(_t)   ((_t) * _XC_SysTickPerScond)                                           /*(t)s  转换成Tick*/
/**将Tick转换成时间(1个tick等于多少个时间)*/
#define _Time_Tick2us(_c)  ((_c) * _XC_SysTickTime_ms * 1000) /*(c)转换成us*/
#define _Time_Tick2ms(_c)  ((_c) * _XC_SysTickTime_ms)        /*(c)转换成ms*/
#define _Time_Tick2s(_c)   ((_c) / _XC_SysTickPerScond)       /*(c)转换成s*/

/**
 *  (_XC_SysTickPerScond <= 1000000),表示时基单位是1us到999us
 */
#elif (_XC_SysTickPerScond <= 1000000)

#define _XC_SysTickTime_us (1000000 / _XC_SysTickPerScond) // 系统滴答计数时基(单位:us)
/*将时间转换成Tick(1个时间是多少Tick)*/

#define _Time_us2Tick(_t)  (((_t) + _XC_SysTickTime_us - 1) / _XC_SysTickTime_us) /*(t)us 转换成Tick*/
#define _Time_ms2Tick(_t)  ((_t) * _XC_SysTickPerScond)                           /*(t)ms 转换成Tick*/
#define _Time_s2Tick(_t)   (_Time_ms2Tick(_t) * 1000)                             /*(t)s  转换成Tick*/
/**将Tick转换成时间(1个tick等于多少个时间)*/

#define _Time_Tick2us(_c)  ((_c) * _XC_SysTickTime_us)  /*(c)转换成us*/
#define _Time_Tick2ms(_c)  ((_c) / _XC_SysTickPerScond) /*(c)转换成ms*/
#define _Time_Tick2s(_c)   (_Time_Tick2ms(_c) / 1000)   /*(c)转换成s*/

#else
#error "系统每秒滴答数(_XC_SysTickPerScond)设置错误!"
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 获取基础时间 */

/**
 * @brief       [用户]获取系统Tick最小时间(单位:us)
 * @return      int32_t     返回当前系统嘀嗒计数(Tick)最小时间(单位:us);
 * @details     即是一次计数过去了多少时间;
 */
#define XCTime_GetTickUnit()           (_XC_SysTickTime_us)

/**
 * @brief       [用户]获取系统Tick
 * @return      XCuint_t    返回当前系统嘀嗒计数(Tick)值;
 * @details     获取的是一个全局变量
 */
#define XCTime_GetTick()               (_XC_SysTickCount)

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
 *  XCuint_t Lc;
 *  XCuint_t Cache32;
 *  Lc  = XCTime_GetTick();      //得到当前系统Tick
 *  while(1){
 *      if(XCTime_CompareTick(Lc, 1000)){
 *          //等待>=1000个Tick后会运行这里;
 *      }
 *      Cache32 = XCTime_GetRemainTick(Lc,1000);     //从Lc开始,离1000个Tick还差多少个Tick
 *      Cache32 = XCTime_GetRunTick(Lc,1000);        //从Lc开始,到1000个Tick已经运行了多少Tick
 *      if(XCTime_CompareTick_ms(Lc, 700){
 *          //等待>=700ms后会运行这里;
 *      }
 *  }
 *  ```
 * ---
 *  注:在时间处理上不要用以下方式处理,在Tick溢出后会出错;
 *  ```c
 *  XCuint_t Lct;
 *  Lct = XCTime_GetTick() + 100;           //获取系统Tick+100;
 *  while(1){
 *      if(_XC_SysTickCount >= (Lct)){      //得到Tick+100后运行;
 *          //等待超过100个Tick后运行;
 *      }
 *  }
 *  ```
 */

/**
 * @brief       [用户]比较Tick是否到达设定值
 * @param[in]   _Lc     上个记录的Tick
 * @param[in]   _c      需要比较的Tick个数
 * @return      boor    1:时间到达;0:时间没有到达;
 * @details     判断是否运行了"_c"个Tick,用于判断时间是否到达;
 */
#define XCTime_CompareTick(_Lc, _c)    ((_XC_SysTickCount - (_Lc)) >= (_c))

/**
 * @brief       [用户]比较时间是否到达设定值(单位:ms)
 * @param[in]   _Lc     上个记录的Tick
 * @param[in]   _t      需要比较时间
 * @return      boor    1:时间到达;0:时间没有到达;
 * @details     判断是否运行了"_t"个时间,用于判断时间是否到达;
 */
#define XCTime_CompareTick_ms(_Lc, _t) (XCTime_CompareTick((_Lc), _Time_ms2Tick(_t)))

/**
 * @brief       [用户]比较时间是否到达设定值(单位:s)
 * @param[in]   _Lc     上个记录的Tick
 * @param[in]   _t      需要比较时间
 * @return      boor    1:时间到达;0:时间没有到达;
 * @details     判断是否运行了"_t"个时间,用于判断时间是否到达;
 */
#define XCTime_CompareTick_s(_Lc, _t)  (XCTime_CompareTick((_Lc), _Time_s2Tick(_t)))

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       [用户]获取剩下多少Tick
 * @param[in]   LastTick        上个记录的Tick
 * @param[in]   CompareTick     等待到达的Tick时间
 * @return      XCuint_t        从"LastTick"开始,离"CompareTick"还差多少个Tick,返回0表示到达或者早已到达;
 * @details     获取当前Tick离设定的值还有多少个Tick;
 */
XCuint_t XCTime_GetRemainTick(XCuint_t LastTick, XCuint_t CompareTick);

/**
 * @brief       [用户]获取运行了多少Tick
 * @param[in]   LastTick    上个记录的Tick
 * @param[in]   CompareTick 等待到达的Tick时间
 * @return      XCuint_t    从"LastTick"开始,到"CompareTick"个Tick已经运行了多少Tick,返回"CompareTick"表示已经运行完成或者早已运行完成;
 * @details     获取当前Tick到设定的值,已经运行了多少个Tick;
 */
XCuint_t XCTime_GetRunTick(XCuint_t LastTick, XCuint_t CompareTick);

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**
 *  扩展时间处理
 *  相比基础时间处理,扩展时间处理增加了数据类型,简化处理;
 *  使用方式(例程):
 *  - "XCTimeCount_t"类型由2个变量组成,简化"时间处理1"的操作;
 *  - 允许出现"Tick"溢出,溢出后时间计算仍然是对的;
 *  - 这里时间计数限制是:延时等待时间+每次查询Tick是否到达时间,不能超过设置的类型值大小;
 *  ```c
 *  XCTimeCount_t tCount;
 *  XCTime_tUpdateTick_ms(tCount, 100);     //更新比较的Tick值为100ms
 *  while(1){
 *      if(XCTime_tCompareTick(tCount)){
 *          //等待超过100ms后运行;
 *      }
 *  }
 *  ```
 */

/** 时间计数数据类型 */
typedef struct {
    XCuint_t TickCount;
    XCuint_t WaitCount;
} XCTimeCount_t;

//===

/**
 * @brief       [用户]更新需要延时比较的Tick
 * @param[in]   _tC     "XCTimeCount_t"类型定义的数据;
 * @param[in]   _c      更新比较的Tick计数
 * @details     [扩展]更新需要比较或者延时的Tick数
 */
#define XCTime_tUpdateTick(_tC, _c)    ((_tC).TickCount = _XC_SysTickCount, (_tC).WaitCount = (_c))

/**
 * @brief       [用户]更新需要延时比较的时间(ms)
 * @param[in]   _tC     "XCTimeCount_t"类型定义的数据;
 * @param[in]   _t      更新比较的时间
 * @details     [扩展]更新需要比较或者延时的时间
 */
#define XCTime_tUpdateTick_ms(_tC, _t) XCTime_tUpdateTick((_tC), _Time_ms2Tick(_t)) /*更新:_t毫秒*/

/**
 * @brief       [用户]更新需要延时比较的时间(s)
 * @param[in]   _tC     "XCTimeCount_t"类型定义的数据;
 * @param[in]   _t      更新比较的时间
 * @details     [扩展]更新需要比较或者延时的时间
 */
#define XCTime_tUpdateTick_s(_tC, _t)  XCTime_tUpdateTick((_tC), _Time_s2Tick(_t)) /*更新:_t秒*/

/**
 * @brief       [用户]重复更新上次需要延时比较的时间
 * @param[in]   _tC     "XCTimeCount_t"类型定义的数据;
 * @details     [扩展]重复更新上次需要比较或者延时的时间;
 */
#define XCTime_tRepeatLastTick(_tC)    ((_tC).TickCount = _XC_SysTickCount)

/**
 * @brief       [用户]判断需要比较或延时的时间是否到达
 * @param[in]   _tC     "XCTimeCount_t"类型定义的数据;
 * @return      boot    1时间达到;0时间没有到达;
 * @details     [扩展]判断需要比较或延时的时间是否到达
 */
#define XCTime_tCompareTick(_tC)       ((_XC_SysTickCount - ((_tC).TickCount)) >= ((_tC).WaitCount))

/**
 * @brief       [用户]清除类型中所有计数
 * @param[in]   _tC     "XCTimeCount_t"类型定义的数据;
 * @details     [扩展]"XCTimeCount_t"类型数据全清零;
 */
#define XCTime_tClrAllCount(_tC)       ((_tC).TickCount = 0, (_tC).WaitCount = 0)

/************************************************ 我是分割线 ************************************************/
/**
 * @brief       [用户]获取剩下多少Tick
 * @param[in]   tTime       "XCTimeCount_t"类型定义的时间数据;
 * @return      XCuint_t    返回余下Tick,返回0表示到达或者早已到达
 * @details     [扩展]从上次调用更新计算,获取当前Tick离设定的值还有多少个Tick;
 */
XCuint_t XCTime_tGetRemainTick(XCTimeCount_t tTime);

/**
 * @brief       [用户]获取运行了多少Tick
 * @param[in]   tTime       "XCTimeCount_t"类型定义的时间数据;
 * @return      XCuint_t    返回已经运行的Tick,若等于"tTime.WaitCount",表示时间到达或早已到达;
 * @details     从上次调用更新计算,获取当前Tick到设定的值已经运行了多少个Tick;
 */
XCuint_t XCTime_tGetRunTick(XCTimeCount_t tTime);

/**
 * @brief       [用户]获取运行了多少ms
 * @param[in]   tTime       "XCTimeCount_t"类型定义的时间数据;
 * @return      XCuint_t    返回已经运行的ms,若等于"tTime.WaitCount",表示时间到达或早已到达;
 * @details     [扩展]从上次调用更新计算,获取当前Tick到设定的值已经运行了多少个Tick;
 */
#define XCTime_tGetRunTime_ms(_tC) _Time_Tick2ms(XCTime_tGetRunTick(_tC)) // 获取运行了多少ms

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
void XCTime_BlockDelay(XCuint_t DelayTick);

/**
 * @brief       [用户]死循环延时ms
 * @param[in]   Delay_ms    需要延时的ms数
 * @details     while判断死延时
 */
void XCTime_BlockDelay_ms(XCuint_t Delay_ms);

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
