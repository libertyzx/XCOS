/*=========================================================|
 | 文件名:  XC_Time.c
 | 描述:    时间操作
 | 版本:    V1.00
 | 日期:    2024/08/12
 | 语言:    C语言
 | 作者:    libertyzx
 | E-mail:  libertyzx@163.com
 +-----------------------------------------------|
 | 开源协议: MIT License
 +-----------------------------------------------|
 +--- 说明
 |  1.时间戳,日历,秒转换计算处理
 +-----------------------------------------------|
 +--- 版本说明:
 |  V1.00:-2024/08/12
 |      1.初始化
 *========================================================*/
//=== 头文件
#include "XC_Time.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/* 计算方式
 *  需要计算时间*对应比例
 *  如:
 *      5小时转换成秒:5*_Time_h2s
 *      5小时转换成ms:5*_Time_h2ms
 */
/* 毫秒->x */
// #define _Time_ms2us     (1000)                             /*毫秒转微秒*/
//                                                            /* 秒->x */
// #define _Time_s2ms      (1000)                             /*秒转毫秒*/
// #define _Time_s2us      (1000 * 1000)                      /*秒转微秒*/
//                                                            /* 分->x */
// #define _Time_min2s     (60)                               /*分转秒*/
// #define _Time_min2ms    (60 * 1000)                        /*分转毫秒*/
// #define _Time_min2us    (60 * 1000 * 1000)                 /*分转微秒*/
//                                                            /* 小时->x */
// #define _Time_h2min     (60)                               /*小时转分钟*/
// #define _Time_h2s       (60 * 60)                          /*小时转秒*/
// #define _Time_h2ms      (60 * 60 * 1000)                   /*小时转毫秒*/
// #define _Time_h2us      (60 * 60 * 1000 * 1000)            /*小时转微秒*/
//                                                            /* 天->x */
// #define _Time_day2h     (24)                               /*天转小时*/
// #define _Time_day2min   (24 * 60)                          /*天转分钟*/
// #define _Time_day2s     (24 * 60 * 60)                     /*天转秒*/
// #define _Time_day2ms    (24 * 60 * 60 * 1000)              /*天转毫秒*/
// #define _Time_day2us    (24 * 60 * 60 * 1000 * 1000)       /*天转微秒*/
//                                                            /* 月(28天)->x */
// #define _Time_28m2h     (28 * 24)                          /*28天转小时*/
// #define _Time_28m2min   (28 * 24 * 60)                     /*28天转分*/
// #define _Time_28m2s     (28 * 24 * 60 * 60)                /*28天转秒*/
// #define _Time_28m2ms    (28 * 24 * 60 * 60 * 1000)         /*28天转毫秒*/
// #define _Time_28m2us    (28 * 24 * 60 * 60 * 1000 * 1000)  /*28天转微秒*/
//                                                            /* 月(29天)->x */
// #define _Time_29m2h     (29 * 24)                          /*29天转小时*/
// #define _Time_29m2min   (29 * 24 * 60)                     /*29天转分*/
// #define _Time_29m2s     (29 * 24 * 60 * 60)                /*29天转秒*/
// #define _Time_29m2ms    (29 * 24 * 60 * 60 * 1000)         /*29天转毫秒*/
// #define _Time_29m2us    (29 * 24 * 60 * 60 * 1000 * 1000)  /*29天转微秒*/
//                                                            /* 月(30天)->x */
// #define _Time_30m2h     (30 * 24)                          /*30天转小时*/
// #define _Time_30m2min   (30 * 24 * 60)                     /*30天转分*/
// #define _Time_30m2s     (30 * 24 * 60 * 60)                /*30天转秒*/
// #define _Time_30m2ms    (30 * 24 * 60 * 60 * 1000)         /*30天转毫秒*/
// #define _Time_30m2us    (30 * 24 * 60 * 60 * 1000 * 1000)  /*30天转微秒*/
//                                                            /* 月(31天)->x */
// #define _Time_31m2h     (31 * 24)                          /*31天转小时*/
// #define _Time_31m2min   (31 * 24 * 60)                     /*31天转分*/
// #define _Time_31m2s     (31 * 24 * 60 * 60)                /*31天转秒*/
// #define _Time_31m2ms    (31 * 24 * 60 * 60 * 1000)         /*31天转毫秒*/
// #define _Time_31m2us    (31 * 24 * 60 * 60 * 1000 * 1000)  /*31天转微秒*/
//                                                            /* 年->x */
// #define _Time_year2day  (365)                              /*一年转天*/
// #define _Time_year2h    (365 * 24)                         /*一年转小时*/
// #define _Time_year2s    (365 * 24 * 60 * 60)               /*一年转秒*/
// #define _Time_year2ms   (365 * 24 * 60 * 60 * 1000)        /*一年转毫秒*/
// #define _Time_year2us   (365 * 24 * 60 * 60 * 1000 * 1000) /*一年转微秒*/
//                                                            /* 润年->x */
// #define _Time_lyear2day (366)                              /*闰年转天*/
// #define _Time_lyear2h   (366 * 24)                         /*闰年转小时*/
// #define _Time_lyear2s   (366 * 24 * 60 * 60)               /*闰年转秒*/
// #define _Time_lyear2ms  (366 * 24 * 60 * 60 * 1000)        /*闰年转毫秒*/
// #define _Time_lyear2us  (366 * 24 * 60 * 60 * 1000 * 1000) /*闰年转微秒*/
//=== 私有宏定义
    //Unix初始年
    #define _InitYearUnix                   (1970)      //Unix计数初始时间(时间是1月1日0:0:0)
    //最大时间计算(32位按秒算)
    #define _SecCount_32bit_MaxYearUnix     (_InitYearUnix + (0xFFFFFFFF / _Time_lyear2s)) //按照秒计数,允许计数到的最大年数(按闰年算)

//=== 私有变量
    //-只读
    const uint8_t r_DaysPerMonth[2][12] = {             //|每月天数
        {31,28,31,30,31,30,31,31,30,31,30,31},          //| 0:非闰年每月天数
        {31,29,31,30,31,30,31,31,30,31,30,31} };        //| 1:闰年每月天数

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/************************************************|
 * 描述:    闰年判断
 * 函数名:  XCTime_IsLeapYear
 * 参数[I]: uint16_t Year   //输入年
 * 返回:    uint8_t
 *  +=返回
 *  | 0: 不是闰年
 *  | 1: 是闰年
 * 说明:
 *  闰年判断依据:
 *  1.年份是4的倍数且不是100倍数(非整百年且被4整除);
 *  2.年份是400的倍数(整百年被400整除);
 ************************************************/
uint8_t XCTime_IsLeapYear(uint16_t Year)
{
    return(!! ((((Year&0x03)==0) && ((Year%100)!=0)) || ((Year%400)==0)) );
}

/************************************************|
 * 描述:    计算两个年之间的闰年数量
 * 函数名:  XCTime_GetLeapYearNum
 * 参数[I]: uint16_t FrontYear      //输入年(>0)
 * 参数[I]: uint16_t BackYear       //输入年(>=FrontYear)
 * 返回:    uint16_t                //闰年数量
 * 说明:
 *  闰年数量包含FrontYear年,不包含BackYear年;
 ************************************************/
uint16_t XCTime_GetLeapYearNum(uint16_t FrontYear, uint16_t BackYear)
{
    FrontYear--;                                                //包含输入小的年份
    BackYear--;                                                 //不包含输入大的年份
    FrontYear = FrontYear/4 - FrontYear/100 + FrontYear/400;    //从0年开始的闰年
    BackYear  = BackYear/4  - BackYear/100  + BackYear/400;     //从0年开始的闰年
    return(BackYear-FrontYear);                                 //相减得到两个年份间的闰年数
}

/************************************************ 我是分割线 ************************************************/

/************************************************|
 * 描述:    设置初始年,日历转秒
 * 函数名:  XCTime_SetYear_CalendarToSec
 * 参数[I]: uint16_t InitYear               //初始年
 * 参数[I]: XCTime_Calendar_t *pCalendar    //日历指针(必须大于初始年)
 * 返回:    uint32_t                        //返回秒
 * 说明:
 *  指定一个年的1月1日0点0分0秒为初始,与日历数据计算得到中间的秒;
 *  无时区概念;
 ************************************************/
uint32_t XCTime_SetYear_CalendarToSec(uint16_t InitYear, XCTime_Calendar_t *pCalendar)
{
    uint32_t Sec;
    uint8_t  LeapYearMark;          //闰年标志
    uint8_t  Month;
    uint16_t Day;
//===
    //年处理(年转秒)
    Sec  = (pCalendar->Year - InitYear) * _Time_year2s;                     //所有年先按不是闰年算,得到秒;
    Sec += XCTime_GetLeapYearNum(InitYear, pCalendar->Year) * _Time_day2s;  //加上闰年的天数的秒;
    LeapYearMark = XCTime_IsLeapYear(pCalendar->Year);                      //得到当前年闰年标志;
    //月和天处理(月转天转秒)
    Month = pCalendar->Month-1;                     //不包含当月
    Day   = pCalendar->Date -1;                     //不包含当天
    while(Month--){
        Day += r_DaysPerMonth[LeapYearMark][Month]; //加上,月的天数
    }
    Sec += Day * _Time_day2s;                       //天转秒
    //时间
    Sec += pCalendar->Hour*_Time_h2s + pCalendar->Minute*_Time_min2s + pCalendar->Second;   //加上时间得到最后的秒
    //返回秒
    return(Sec);
}

/************************************************|
 * 描述:    设置初始年,秒转日历
 * 函数名:  XCTime_SetYear_SecToCalendar
 * 参数[I]: uint16_t InitYear           //初始年
 * 参数[I]: uint32_t Sec                //经过的秒
 * 参数[O]: XCTime_Calendar_t *Calendar //返回日历指针
 * 返回:    void
 * 说明:
 *  指定一个年的1月1日0点0分0秒为初始,与一个秒计算得到日历;
 *  无时区概念;
 ************************************************/
void XCTime_SetYear_SecToCalendar(uint16_t InitYear, uint32_t Sec, XCTime_Calendar_t *pCalendar)
{
    uint16_t Year;
    uint16_t LeapYear;
    uint16_t Day;
    uint8_t  LeapYearMark;                          //闰年标志
    uint8_t  Month;

    //时分秒
    pCalendar->Second = Sec%_Time_min2s;                    //得到秒
    Sec             /= _Time_min2s;                         //除去秒

    pCalendar->Minute = Sec%_Time_h2min;                    //得到分
    Sec             /= _Time_h2min;                         //除去分

    pCalendar->Hour   = Sec%_Time_day2h;                    //得到小时
    Sec             /= _Time_day2h;                         //除去小时-得到总天数
    //年
    Year = Sec/_Time_year2day+InitYear;                     //全部年按非闰年计算,加上初始年
    Day  = Sec%_Time_year2day;                              //除去年得到剩余天数
    do{
        LeapYear = XCTime_GetLeapYearNum(InitYear, Year);   //得到闰年数
        if(Day >= LeapYear){
            break;                                          //剩余天数足够扣除闰年多出的天数数-跳出
        }
        Year--;                                             //闰年数不够,减1年
        Day += _Time_year2day;                              //天数+1年
    }while(1);
    pCalendar->Year = Year;                                 //得到年;
    Day -= LeapYear;                                        //天数减闰年数;
    //月日
    LeapYearMark = XCTime_IsLeapYear(Year);                 //得到当前年闰年标志;
    for(Month=0; Day>=r_DaysPerMonth[LeapYearMark][Month]; Month++){
        Day-=r_DaysPerMonth[LeapYearMark][Month];
    }
    pCalendar->Month = Month+1;                             //得到月(月从1开始)
    pCalendar->Date  = Day+1;                               //得到日期(日期从1开始)
}

/************************************************ 我是分割线 ************************************************/
/*=== 时间计算*/

/************************************************|
 * 描述:    计算2个日期的差值
 * 函数名:  XCTime_TwoDateComputeDiff
 * 参数[I]: XCTime_Calendar_t *Calendar1    //日期1
 * 参数[I]: XCTime_Calendar_t *Calendar2    //日期2
 * 返回:    uint32_t                        //2个日期的差(秒)
 * 说明:    计算2个日期的差;
 ************************************************/
uint32_t XCTime_TwoDateComputeDiff(XCTime_Calendar_t *Calendar1, XCTime_Calendar_t *Calendar2)
{
    uint16_t InitYear;
    uint32_t  Sec1;
    uint32_t  Sec2;

    InitYear = Calendar1->Year > Calendar2->Year ? Calendar2->Year : Calendar1->Year;
    Sec1 = XCTime_SetYear_CalendarToSec(InitYear, Calendar1);
    Sec2 = XCTime_SetYear_CalendarToSec(InitYear, Calendar2);
    return(Sec1>Sec2 ? Sec1-Sec2 : Sec2-Sec1);
}

/************************************************|
 * 描述:    输入日期,返回今年日期的x值(x=天,时,分,秒)
 * 函数名:  XCTime_ThisYear
 * 参数[I]: XCTime_Calendar_t *pt   //输入日期
 * 参数[I]: XCTime_ParamTime_t ParamTime
 *  +=需要返回的值时间类型
 *  | _Time_P_Second        //秒
 *  | _Time_P_Minute        //分钟
 *  | _Time_P_Hour          //小时
 *  | _Time_P_Day           //日
 * 返回:    uint32_t        //返回值(由"ParamTime"参数决定是啥)
 ** 说明:   从当前年1月1日0点计算
 ************************************************/
uint32_t XCTime_ThisYear(XCTime_Calendar_t *pt, XCTime_ParamTime_t ParamTime)
{
    uint16_t InitYear;
    uint32_t  Sec;

    InitYear = pt->Year;
    Sec = XCTime_SetYear_CalendarToSec(InitYear, pt);       //当前秒

    switch(ParamTime){
        case _Time_P_Second:    return(Sec);                //输出秒
        case _Time_P_Minute:    return(Sec/_Time_min2s);    //输出分
        case _Time_P_Hour:      return(Sec/_Time_h2s);      //输出小时
        case _Time_P_Day:       return(Sec/_Time_day2s);    //输出天
    }
    return(0);
}

/************************************************|
 * 描述:    计算2个日期的差值
 * 函数名:  XCTime_CalculateTimeDiff
 * 参数[I]: XCTime_Calendar_t  Calendar[2]  //0初始日期;1结束日期
 * 参数[I]: XCTime_ParamTime_t ParamTime
 *  +=需要返回的值时间类型
 *  | _Time_P_Second        //秒
 *  | _Time_P_Minute        //分钟
 *  | _Time_P_Hour          //小时
 *  | _Time_P_Day           //日
 * 返回:    uint32_t        //2个日期的差(由"ParamTime"参数决定是啥)
 * 说明:    计算2个日期的差;
 ************************************************/
uint32_t XCTime_CalculateTimeDiff(XCTime_Calendar_t Calendar[2], XCTime_ParamTime_t ParamTime)
{
    uint16_t InitYear;
    uint32_t  Sec[2];
//===
    InitYear = Calendar[0].Year;
    Sec[0]   = XCTime_SetYear_CalendarToSec(InitYear, &Calendar[0]);
    Sec[1]   = XCTime_SetYear_CalendarToSec(InitYear, &Calendar[1]);
//=== 计算
    if(Sec[0] >= Sec[1])
        return(0);
    switch(ParamTime){
        case _Time_P_Second:    return(Sec[1]-Sec[0]);                  //输出秒
        case _Time_P_Minute:    return((Sec[1]-Sec[0])/_Time_min2s);    //输出分
        case _Time_P_Hour:      return((Sec[1]-Sec[0])/_Time_h2s);      //输出小时
        case _Time_P_Day:       return((Sec[1]-Sec[0])/_Time_day2s);    //输出天
    }
    return(0);
}

/************************************************ 我是分割线 ************************************************/
/*=== Unix*/

/************************************************|
 * 描述:    由日历计算Unix时间戳
 * 函数名:  XCTime_CalendarToUnix
 * 参数[I]: XCTime_Calendar_t *pCalendar    //日历指针
 * 参数[I]: int8_t Timezone                 //时区(+-12)
 * 返回:    uint32_t                        //返回unix时间戳
 *** 说明:
 ***    Unix时间戳是1970年1月1日0点0分0秒开始计算秒;
 ************************************************/
uint32_t XCTime_CalendarToUnix(XCTime_Calendar_t *pCalendar, int8_t Timezone)
{
    return( XCTime_SetYear_CalendarToSec(_InitYearUnix, pCalendar) - (Timezone*_Time_h2s) );    //得到秒,并校准时区
}

/************************************************|
 * 描述:    由Unix时间戳计算日历
 * 函数名:  XCTime_UnixToCalendar
 * 参数[I]: uint32_t UnixSec                //Unix时间戳
 * 参数[I]: int8_t Timezone                 //时区(+-12)
 * 参数[O]: XCTime_Calendar_t *pCalendar    //得到的日历指针
 * 返回:    void
 *** 说明:
 ***    Unix时间戳是1970年1月1日0点0分0秒开始计算秒;
 ************************************************/
void XCTime_UnixToCalendar(uint32_t UnixSec, int8_t Timezone, XCTime_Calendar_t *pCalendar)
{
    XCTime_SetYear_SecToCalendar(_InitYearUnix, UnixSec+(Timezone*_Time_h2s), pCalendar);
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
