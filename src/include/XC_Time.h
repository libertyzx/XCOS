/*=========================================================|
 | 文件名: XC_Time.h
 | 描述  : 时间相关的数据
 | 版本  : V1.00
 | 日期  : 2024/03/19
 | 语言  : C语言
 | 作者  : libertyzx
 | E-mail: libertyzx@163.com
 +-----------------------------------------------|
 | 开源协议: MIT License
 +-----------------------------------------------|
 +--- 说明
 |  1.ms,us,s,h等单位转换;
 |  2.系统时间相关处理;
 |  2.编译时间格式化输出;
 +-----------------------------------------------|
 +--- 版本说明
 |  V1.00:-2024/03/19
 |      1.从"XCBase"中继承并重构;
 *========================================================*/
//=== 防重复定义
#ifndef _XC_Time_H_
#define _XC_Time_H_
//=== 头文件
#include "XC_Type.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 时间单位换算比例
/* 计算方式
 *  需要计算时间*对应比例
 *  如:
 *      5小时转换成秒:5*_Time_h2s
 *      5小时转换成ms:5*_Time_h2ms
 */
/* 毫秒->x */
    #define _Time_ms2us             (1000)                      /*毫秒转微秒*/
/* 秒->x */
    #define _Time_s2ms              (1000)                      /*秒转毫秒*/
    #define _Time_s2us              (1000*1000)                 /*秒转微秒*/
/* 分->x */
    #define _Time_min2s             (60)                        /*分转秒*/
    #define _Time_min2ms            (60*1000)                   /*分转毫秒*/
    #define _Time_min2us            (60*1000*1000)              /*分转微秒*/
/* 小时->x */
    #define _Time_h2min             (60)                        /*小时转分钟*/
    #define _Time_h2s               (60*60)                     /*小时转秒*/
    #define _Time_h2ms              (60*60*1000)                /*小时转毫秒*/
    #define _Time_h2us              (60*60*1000*1000)           /*小时转微秒*/
/* 天->x */
    #define _Time_day2h             (24)                        /*天转小时*/
    #define _Time_day2min           (24*60)                     /*天转分钟*/
    #define _Time_day2s             (24*60*60)                  /*天转秒*/
    #define _Time_day2ms            (24*60*60*1000)             /*天转毫秒*/
    #define _Time_day2us            (24*60*60*1000*1000)        /*天转微秒*/
/* 月(28天)->x */
    #define _Time_28m2h             (28*24)                     /*28天转小时*/
    #define _Time_28m2min           (28*24*60)                  /*28天转分*/
    #define _Time_28m2s             (28*24*60*60)               /*28天转秒*/
    #define _Time_28m2ms            (28*24*60*60*1000)          /*28天转毫秒*/
    #define _Time_28m2us            (28*24*60*60*1000*1000)     /*28天转微秒*/
/* 月(29天)->x */
    #define _Time_29m2h             (29*24)                     /*29天转小时*/
    #define _Time_29m2min           (29*24*60)                  /*29天转分*/
    #define _Time_29m2s             (29*24*60*60)               /*29天转秒*/
    #define _Time_29m2ms            (29*24*60*60*1000)          /*29天转毫秒*/
    #define _Time_29m2us            (29*24*60*60*1000*1000)     /*29天转微秒*/
/* 月(30天)->x */
    #define _Time_30m2h             (30*24)                     /*30天转小时*/
    #define _Time_30m2min           (30*24*60)                  /*30天转分*/
    #define _Time_30m2s             (30*24*60*60)               /*30天转秒*/
    #define _Time_30m2ms            (30*24*60*60*1000)          /*30天转毫秒*/
    #define _Time_30m2us            (30*24*60*60*1000*1000)     /*30天转微秒*/
/* 月(31天)->x */
    #define _Time_31m2h             (31*24)                     /*31天转小时*/
    #define _Time_31m2min           (31*24*60)                  /*31天转分*/
    #define _Time_31m2s             (31*24*60*60)               /*31天转秒*/
    #define _Time_31m2ms            (31*24*60*60*1000)          /*31天转毫秒*/
    #define _Time_31m2us            (31*24*60*60*1000*1000)     /*31天转微秒*/
/* 年->x */
    #define _Time_year2day          (365)                       /*一年转天*/
    #define _Time_year2h            (365*24)                    /*一年转小时*/
    #define _Time_year2s            (365*24*60*60)              /*一年转秒*/
    #define _Time_year2ms           (365*24*60*60*1000)         /*一年转毫秒*/
    #define _Time_year2us           (365*24*60*60*1000*1000)    /*一年转微秒*/
/* 润年->x */
    #define _Time_lyear2day         (366)                       /*闰年转天*/
    #define _Time_lyear2h           (366*24)                    /*闰年转小时*/
    #define _Time_lyear2s           (366*24*60*60)              /*闰年转秒*/
    #define _Time_lyear2ms          (366*24*60*60*1000)         /*闰年转毫秒*/
    #define _Time_lyear2us          (366*24*60*60*1000*1000)    /*闰年转微秒*/

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**滴答计数频率转换*/
#if(_XC_SysTickPerScond <= 1000)
    /**时间从1ms到1000ms*/
    #define _XC_SysTickTime_ms      (1000/_XC_SysTickPerScond)              //系统滴答计数时间(ms)
    #define _XC_SysTickTime_us      (_XC_SysTickTime_ms * 1000)             //系统滴答计数时间(us)

    /**将时间转换成Tick(1个时间是多少Tick)
     *  注意:使用"_Time_us2Tick"时,时间误差会非常大,tick是ms级的,us时间处理按1个tick处理;
     */
    #define _Time_us2Tick(_t)       ( ((_t) + _XC_SysTickTime_ms * 1000 - 1) / (_XC_SysTickTime_ms * 1000) )    /*(t)us 转换成Tick*/
    #define _Time_ms2Tick(_t)       ( ((_t) + _XC_SysTickTime_ms - 1) / _XC_SysTickTime_ms )                    /*(t)ms 转换成Tick*/
    #define _Time_s2Tick( _t)       ( (_t) * _XC_SysTickPerScond )                                              /*(t)s  转换成Tick*/
    /**将Tick转换成时间(1个tick等于多少个时间)*/
    #define _Time_Tick2us(_c)       ( (_c) * _XC_SysTickTime_ms * 1000 )    /*(c)转换成us*/
    #define _Time_Tick2ms(_c)       ( (_c) * _XC_SysTickTime_ms )           /*(c)转换成ms*/
    #define _Time_Tick2s(_c)        ( (_c) / _XC_SysTickPerScond )          /*(c)转换成s*/

#elif(_XC_SysTickPerScond <= 1000000)
    /*时间从1us到999us*/
    #define _XC_SysTickTime_us      (1000000 / _XC_SysTickPerScond)         //系统滴答计数时间(us)
    /*将时间转换成Tick(1个时间是多少Tick)*/
    #define _Time_us2Tick(_t)       ( ((_t) + _XC_SysTickTime_us - 1) / _XC_SysTickTime_us )    /*(t)us 转换成Tick*/
    #define _Time_ms2Tick(_t)       ( (_t) * _XC_SysTickPerScond )                              /*(t)ms 转换成Tick*/
    #define _Time_s2Tick( _t)       ( _Time_ms2Tick(_t) * _Time_s2ms )                          /*(t)s  转换成Tick*/
    /**将Tick转换成时间(1个tick等于多少个时间)*/
    #define _Time_Tick2us(_c)       ( (_c) * _XC_SysTickTime_us )           /*(c)转换成us*/
    #define _Time_Tick2ms(_c)       ( (_c) / _XC_SysTickPerScond )          /*(c)转换成ms*/
    #define _Time_Tick2s(_c)        ( _Time_Tick2ms(_c) / _Time_s2ms )      /*(c)转换成s*/

#else
    #error "系统每秒滴答数(_XC_SysTickPerScond)设置错误!"
#endif

/**将时间转换成Tick(1个时间是多少Tick)*/
#define _Time_min2Tick(_t)      ( _Time_s2Tick(_t) * _Time_min2s)   /*(t)min转换成Tick*/
#define _Time_h2Tick(  _t)      ( _Time_s2Tick(_t) * _Time_h2s)     /*(t)h  转换成Tick*/
#define _Time_day2Tick(_t)      ( _Time_s2Tick(_t) * _Time_day2s)   /*(t)day转换成Tick*/

/**将Tick转换成时间*/
#define _Time_Tick2min(_c)      ( _Time_Tick2s(_c) / _Time_min2s )  /*(c)转换成min*/
#define _Time_Tick2h(  _c)      ( _Time_Tick2s(_c) / _Time_h2s )    /*(c)转换成h*/
#define _Time_Tick2day(_c)      ( _Time_Tick2s(_c) / _Time_day2s )  /*(c)转换成day*/

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/**基础时间处理
 * 例程说明见"README.md"文件;
 */

/************************************************|
 * 描述:    获取系统Tick最小时间(单位:us)
 * 宏名:    XCTime_GetTickUnit
 * 形参[N]: 无
 * 返回:    Tick的最小时间(单位:us)
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCTime_GetTickUnit()    (_XC_SysTickTime_us)

/************************************************|
 * 描述:    获取系统Tick
 * 宏名:    XCTime_GetTick
 * 形参[N]: 无
 * 返回:    Tick的计数
 * 说明:    返回当前系统Tick计数
 * 例程:
 *  设Tick单位是1ms,当前Tick值是2365;
 *      Cache = _Time_GetTick();        //Cache = 2365
 ************************************************/
#define XCTime_GetTick()        (_XC_SysTickCount)

/************************************************|
 * 描述:    获取系统Tick加上指定时间后的Tick
 * 宏名:    XCTime_GetTick_**
 * 形参[I]: _t      //增加的时间
 * 返回:    Tick的计数
 * 说明:    返回当前_t个时间后的系统Tick计数
 * 例程:
 *  设Tick单位是1ms,当前Tick值是2365;
 *      Cache = _Time_GetTick_ms(20);   //Cache = 2365+20/1ms = 2385;
 *      Cache = _Time_GetTick_s(20);    //Cache = 2365+(20*1000)/1ms = 22365;
 ************************************************/
#define XCTime_GetTick_ms( _t)  ( _Time_GetTick() + _Time_ms2Tick(_t) )     /*获取Tick+t(ms)*/
#define XCTime_GetTick_s(  _t)  ( _Time_GetTick() + _Time_s2Tick(_t) )      /*获取Tick+t(s)*/
#define XCTime_GetTick_min(_t)  ( _Time_GetTick() + _Time_min2Tick(_t) )    /*获取Tick+t(min)*/
#define XCTime_GetTick_h(  _t)  ( _Time_GetTick() + _Time_h2Tick(_t) )      /*获取Tick+t(h)*/
#define XCTime_GetTick_day(_t)  ( _Time_GetTick() + _Time_day2Tick(_t) )    /*获取Tick+t(day)*/

//===

/************************************************|
 * 描述:    比较Tick是否到达设定值
 * 宏名:    XCTime_CompareTick
 * 形参[I]: _Lc     //上个记录的Tick
 * 形参[I]: _c      //需要比较的Tick个数
 * 返回:    boor    //1:时间到达;0:时间没有到达;
 * 说明:    判断是否运行了"_c"个Tick,用于判断时间是否到达;
 ************************************************/
#define XCTime_CompareTick(_Lc, _c)         ( (_XC_SysTickCount-(_Lc)) >= (_c) )

/************************************************|
 * 描述:    比较时间是否到达设定值
 * 宏名:    XCTime_CompareTick_**
 * 形参[I]: _Lc     //上个记录的Tick
 * 形参[I]: _t      //需要比较的时间值
 * 返回:    boor    //1:时间到达;0:时间没有到达;
 * 说明:    判断是否运行了"_t"个时间,用于判断时间是否到达;
 * 例程:    无
 ************************************************/
#define XCTime_CompareTick_ms( _Lc, _t)     ( _Time_CompareTick((_Lc), _Time_ms2Tick(_t)) )     /*比较:毫秒*/
#define XCTime_CompareTick_s(  _Lc, _t)     ( _Time_CompareTick((_Lc), _Time_s2Tick(_t)) )      /*比较:秒*/
#define XCTime_CompareTick_min(_Lc, _t)     ( _Time_CompareTick((_Lc), _Time_min2Tick(_t)) )    /*比较:分钟*/
#define XCTime_CompareTick_h(  _Lc, _t)     ( _Time_CompareTick((_Lc), _Time_h2Tick(_t)) )      /*比较:小时*/
#define XCTime_CompareTick_Day(_Lc, _t)     ( _Time_CompareTick((_Lc), _Time_day2Tick(_t)) )    /*比较:天*/

/************************************************|
 * 描述:    比较当前Tick
 * 宏名:    XCTime_CompareTick_t
 * 形参[I]: _Lc     //上个记录的Tick
 * 返回:    boor    //1:时间到达;0:时间没有到达;
 * 说明:    系统Tick和"_Lc"直接比较,判断是否到达"_Lc";
 * 例程:    无
 ************************************************/
#define XCTime_CompareTick_t(_Lc)           ( _XC_SysTickCount >= (_Lc) )

//===

/************************************************|
 * 描述:    死循环延时Tick个计数
 * 宏名:    XCTime_Delay
 * 形参[I]: _Lc     //上个记录的Tick(必须是一个变量,会保存延时开始时的Tick)
 * 形参[I]: _Dc     //延时的Tick个数
 * 返回:    无
 * 说明:    while判断延时
 * 例程:    无
 ************************************************/
#define XCTime_Delay(_Lc, _Dc)              { (_Lc)=_Time_GetTick(); while(!_Time_CompareTick((_Lc),(_Dc))); }

/************************************************|
 * 描述:    死循环延时
 * 宏名:    XCTime_Delay_**
 * 形参[I]: _Lc     //上个记录的Tick(必须是一个变量,会保存延时开始时的Tick)
 * 形参[I]: _t      //延时的时间
 * 返回:    无
 * 说明:    while判断延时
 * 例程:    无
 ************************************************/
#define XCTime_Delay_ms( _Lc, _t)       _Time_Delay((_Lc), _Time_ms2Tick(_t))       /*延时:毫秒*/
#define XCTime_Delay_s(  _Lc, _t)       _Time_Delay((_Lc), _Time_s2Tick(_t))        /*延时:秒*/
#define XCTime_Delay_min(_Lc, _t)       _Time_Delay((_Lc), _Time_min2Tick(_t))      /*延时:分钟*/
#define XCTime_Delay_h(  _Lc, _t)       _Time_Delay((_Lc), _Time_h2Tick(_t))        /*延时:小时*/
#define XCTime_Delay_day(_Lc, _t)       _Time_Delay((_Lc), _Time_day2Tick(_t))      /*延时:天*/

//===

/************************************************|
 * 描述:    [内联]获取剩下多少Tick
 * 函数名:  XCTime_GetRemainTick
 * 形参[I]: XCuint_t _Lc    //上个记录的Tick
 * 形参[I]: XCuint_t _c     //需要比较的Tick个数(等待的Tick)
 * 返回:    XCuint_t        //从"_Lc"开始,离"_c"还差多少个Tick,返回0表示到达或者早已到达;
 * 说明:    获取当前Tick离设定的值还有多少个Tick;
 *  为了中断安全,这里使用内联函数替代函数宏;
 * 例程:    无
 ************************************************/
__STATIC_INLINE XCuint_t XCTime_GetRemainTick(XCuint_t _Lc, XCuint_t _c)
{
    XCuint_t SysTickCount;
    SysTickCount = _XC_SysTickCount;

    if( (SysTickCount-_Lc) >= _c ){
        return(0);
    }
    return(_c-(SysTickCount-_Lc));
}

/************************************************|
 * 描述:    [内联]获取运行了多少Tick
 * 函数名:  XCTime_GetRunTick
 * 形参[I]: XCuint_t _Lc        //上个记录的Tick
 * 形参[I]: XCuint_t _c         //需要比较的Tick个数(等待的Tick)
 * 返回:    XCuint_t            //从"_Lc"开始,到"_c"个Tick已经运行了多少Tick,返回"_c"表示已经运行完成或者早已运行完成;
 * 说明:  获取当前Tick到设定的值,已经运行了多少个Tick;
 *  为了中断安全,这里使用内联函数替代函数宏;
 * 例程:  无
 ************************************************/
__STATIC_INLINE XCuint_t XCTime_GetRunTick(XCuint_t _Lc, XCuint_t _c)
{
    XCuint_t SysTickCount;
    SysTickCount = _XC_SysTickCount;

    if( (SysTickCount-_Lc) >= _c ){
        return(_c);
    }
    return(SysTickCount-_Lc);
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**扩展时间处理
 * 例程说明见"README.md"文件;
 * 相比基础时间处理,扩展时间处理增加了数据类型,简化处理;
 */

/**数据类型*/
typedef struct{
    XCuint_t TickCount;
    XCuint_t WaitCount;
}XCTimeCount_t;

//===

/************************************************|
 * 描述:    更新需要延时比较的Tick
 * 宏名:    XCTime_tUpdateTick
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 形参[I]: _c      //延时比较的Tick个数
 * 返回:    无
 * 说明:    更新需要比较或者延时的Tick数;
 * 例程:    无
 ************************************************/
#define XCTime_tUpdateTick(_tC, _c)         ( (_tC).TickCount=_XC_SysTickCount, (_tC).WaitCount=(_c) )

/************************************************|
 * 描述:    更新需要延时比较的时间
 * 宏名:    XCTime_tUpdateTick_**
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 形参[I]: _t      //延时比较的时间
 * 返回:    无
 * 说明:    更新需要比较或者延时的时间;
 * 例程:    无
 ************************************************/
#define XCTime_tUpdateTick_ms( _tC, _t)     XCTime_tUpdateTick((_tC), _Time_ms2Tick(_t))    /*更新:_t毫秒*/
#define XCTime_tUpdateTick_s(  _tC, _t)     XCTime_tUpdateTick((_tC), _Time_s2Tick(_t))     /*更新:_t秒*/
#define XCTime_tUpdateTick_min(_tC, _t)     XCTime_tUpdateTick((_tC), _Time_min2Tick(_t))   /*更新:_t分钟*/
#define XCTime_tUpdateTick_h(  _tC, _t)     XCTime_tUpdateTick((_tC), _Time_h2Tick(_t))     /*更新:_t小时*/
#define XCTime_tUpdateTick_Day(_tC, _t)     XCTime_tUpdateTick((_tC), _Time_day2Tick(_t))   /*更新:_t天*/

/************************************************|
 * 描述:    重复更新上次需要延时比较的时间
 * 宏名:    XCTime_tRepeatLastTick
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 返回:    无
 * 说明:    重复更新需要比较或者延时的时间;
 * 例程:    无
 ************************************************/
#define XCTime_tRepeatLastTick(_tC)         ( (_tC).TickCount=_XC_SysTickCount )

//===

/************************************************|
 * 描述:    判断需要比较或延时的时间是否到达
 * 宏名:    XCTime_tCompareTick
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 返回:    bool    //1时间达到;0时间没有到达;
 * 说明:    判断需要比较或者延时的时间是否到达;
 * 例程:    无
 ************************************************/
#define XCTime_tCompareTick(_tC)            ( (_XC_SysTickCount-((_tC).TickCount)) >= ((_tC).WaitCount) )

//===

/************************************************|
 * 描述:    死循环延时n个Tick计数
 * 宏名:    XCTime_tDelay
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 形参[I]: _n      //延时的Tick数
 * 返回:    无
 * 说明:    死循环延时
 * 例程:    无
 ************************************************/
#define XCTime_tDelay(_tC, _n)              { XCTime_tUpdateTick((_tC), (_n)); while(!XCTime_tCompareTick(_tC)); }

/************************************************|
 * 描述:    死循环延时n个时间
 * 宏名:    XCTime_tDelay_**
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 形参[I]: _n      //延时的时间
 * 返回:    无
 * 说明:    死循环延时
 * 例程:    无
 ************************************************/
#define XCTime_tDelay_ms( _tC, _n)          XCTime_tDelay((tD), _Time_ms2Tick(_n))      /*延时:毫秒*/
#define XCTime_tDelay_s(  _tC, _n)          XCTime_tDelay((tD), _Time_s2Tick(_n))       /*延时:秒*/
#define XCTime_tDelay_min(_tC, _n)          XCTime_tDelay((tD), _Time_min2Tick(_n))     /*延时:分钟*/
#define XCTime_tDelay_h(  _tC, _n)          XCTime_tDelay((tD), _Time_h2Tick(_n))       /*延时:小时*/
#define XCTime_tDelay_day(_tC, _n)          XCTime_tDelay((tD), _Time_day2Tick(_n))     /*延时:天*/

//===

/************************************************|
 * 描述:    获取类型中的Tick计数值
 * 宏名:    XCTime_tGetTickCount
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 返回:    返回类型中的Tick计数值
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCTime_tGetTickCount(_tC)           ((_tC).TickCount)

/************************************************|
 * 描述:    设置类型中的Tick计数值
 * 宏名:    XCTime_tSetTickCount
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 形参[I]: _C      //需要设置的Tick计数值
 * 返回:    无
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCTime_tSetTickCount(_tC, _C)       ((_tC).TickCount=(_C))

/************************************************|
 * 描述:    清除类型中的Tick计数值
 * 宏名:    XCTime_tClrTickCount
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 返回:    无
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCTime_tClrTickCount(_tC)           ((_tC).TickCount=0)

//===

/************************************************|
 * 描述:    获取类型中的等待计数值
 * 宏名:    XCTime_tGetWaitCount
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 返回:    返回Tick等待计数值
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCTime_tGetWaitCount(_tC)           ((_tC).WaitCount)

/************************************************|
 * 描述:    设置类型中的等待计数值
 * 宏名:    XCTime_tSetWaitCount
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 形参[I]: _C      //需要设置的Tick等待计数值
 * 返回:    无
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCTime_tSetWaitCount(_tC, _C)       ((_tC).WaitCount=(_C))

/************************************************|
 * 描述:    清除类型中的等待计数值
 * 宏名:    XCTime_tClrWaitCount
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 返回:    无
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCTime_tClrWaitCount(_tC)           ((_tC).WaitCount=0)

/************************************************|
 * 描述:    清除类型中所有计数
 * 宏名:    XCTime_tClrAllCount
 * 形参[T]: _tC     //"XCTimeCount_t"类型定义的数据;
 * 返回:    无
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCTime_tClrAllCount(_tC)            ((_tC).TickCount=0, (_tC).WaitCount=0)

//===

/************************************************|
 * 描述:    [内联]获取剩下多少Tick
 * 函数名:  XCTime_tGetRemainTick
 * 形参[I]: XCTimeCount_t _tC       //"XCTimeCount_t"类型定义的数据;
 * 返回:    XCuint_t                //返回余下Tick,返回0表示到达或者早已到达
 * 说明:    从调用更新计算,获取当前Tick离设定的值还有多少个Tick;
 *  为了中断安全,这里使用内联函数替代函数宏;
 * 例程:    无
 ************************************************/
__STATIC_INLINE XCuint_t XCTime_tGetRemainTick(XCTimeCount_t _tC)
{
    XCuint_t SysTickCount;
    SysTickCount = _XC_SysTickCount;

    if( (SysTickCount-_tC.TickCount) >= _tC.WaitCount ){
        return(0);
    }
    return(_tC.WaitCount-(SysTickCount-_tC.TickCount));
}

/************************************************|
 * 描述:    [内联]获取运行了多少Tick
 * 函数名:  XCTime_tGetRunTick
 * 形参[I]: XCTimeCount_t _tC       //"XCTimeCount_t"类型定义的数据;
 * 返回:    XCuint_t                //返回已经运行的Tick,若等于"_tC.WaitCount",表示时间到达或早已到达;
 * 说明:    从调用更新计算,获取当前Tick到设定的值已经运行了多少个Tick;
 *  为了中断安全,这里使用内联函数替代函数宏;
 * 例程:  无
 ************************************************/
__STATIC_INLINE XCuint_t XCTime_tGetRunTick(XCTimeCount_t _tC)
{
    XCuint_t SysTickCount;
    SysTickCount = _XC_SysTickCount;

    if( (SysTickCount-_tC.TickCount) >= _tC.WaitCount ){
        return(_tC.WaitCount);
    }
    return(SysTickCount-_tC.TickCount);
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

//=== 编译时间相关
/*** 说明
 * 宏"__DATE__"格式是:"MMM DD YYYY" 表示当前编译电脑的日期
 * 如:
 *  "Jan  8 2022"  //2022年8月8日
 *  "Jan 18 2022"  //2022年8月18日
 * 月份值是:
 *  1月: Jan (January)
 *  2月: Feb (February)
 *  3月: Mar (March)
 *  4月: Apr (April)
 *  5月: May (May)
 *  6月: Jun (June)
 *  7月: Jul (July)
 *  8月: Aug (August)
 *  9月: Sep (September)
 *  10月:Oct (October)
 *  11月:Nov (November)
 *  12月:Dec (December)
 * 宏"__TIME__"是时间,格式:"HH:MM:SS" 表示当前编译电脑的时间
 * 如:
 *  23:33:23    //23点33分23秒
 *  03:07:06    //3点7分6秒
 * 注意:
 *  年数字是4位10进制数据,需要16位宽及以上类型保存,
 *  其他(月日时分秒)都是2位10进制数据,需要8位宽及以上类型保存;
 ***/
    //获取年数字
    #define _Compile_GetYear    /*输出年*/  \
        ( ((__DATE__[7]-'0')*1000) + ((__DATE__[8]-'0')*100) + ((__DATE__[9]-'0')*10) + (__DATE__[10]-'0') )
    //获取月数字
    #define _Compile_GetMonth   /*输出月份*/    (   \
        __DATE__[2]=='c' ? 12 : \
        __DATE__[2]=='v' ? 11 : \
        __DATE__[2]=='t' ? 10 : \
        __DATE__[2]=='p' ? 9 : \
        __DATE__[2]=='g' ? 8 : \
        __DATE__[2]=='l' ? 7 : \
        __DATE__[2]=='n' ? (__DATE__[1]=='u' ? 6 : 1) : \
        __DATE__[2]=='y' ? 5 : \
        __DATE__[2]=='r' ? (__DATE__[1]=='p' ? 4 : 3) : 2)
    //获取日期数字
    #define _Compile_GetDate    /*输出日期*/    \
        ( (__DATE__[4]==' ' ? 0 : (__DATE__[4]-'0')) * 10 + (__DATE__[5]-'0') )
    //获取时|分|秒
    #define _Compile_GetHour    /*输出小时*/    ( ((__TIME__[0]-'0')*10) + (__TIME__[1]-'0') )
    #define _Compile_GetMin     /*输出分钟*/    ( ((__TIME__[3]-'0')*10) + (__TIME__[4]-'0') )
    #define _Compile_GetSec     /*输出秒*/      ( ((__TIME__[6]-'0')*10) + (__TIME__[7]-'0') )

//=== 获取一个编译日期时间的总字符串(编译时间格式(年月是时分秒):20220803230638)
    #define _Compile_GetDateTimeStr /*获取日期时间字符串(14个数字字符串+1个结束符,总共15个字节)*/   {   \
        /*年*/  __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10],    \
        /*月*/  (_Compile_GetMonth/10)+'0', (_Compile_GetMonth%10)+'0', \
        /*日*/  (__DATE__[4]==' ' ? '0' : '1'), __DATE__[5],    \
        /*时*/  __TIME__[0], __TIME__[1],   \
        /*分*/  __TIME__[3], __TIME__[4],   \
        /*秒*/  __TIME__[6], __TIME__[7],   \
        /*结束*/    '\0' }

//=== 获取十进制表示的日期(8位数字,20230818表示2023年8月18号)
    #define _Compile_GetDateDec /*获取十进制表示的日期*/    ((_Compile_GetYear*100+_Compile_GetMonth)*100+_Compile_GetDate)

//=== 获取十进制表示的时间(6位数字,100345表示10点03分45秒)
    #define _Compile_GetTimeDec /*获取十进制表示的时间*/    ((_Compile_GetHour*100+_Compile_GetMin)*100+_Compile_GetSec)


/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
