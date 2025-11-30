/*=========================================================|
 | 文件名: XC_Time.h
 | 描述  : 时间相关的数据
 | 版本  : V1.02
 | 日期  : 2024/08/12
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
 |  V1.01:-2024/03/27
 |      1.兼容基础库"XCBase";
 |  V1.02:-2024/08/12
 |      1."XC_Time.h"中编译相关宏独立到"XC_TimeCompile.h";
 |      2."XC_Time.h"中系统时间计数相关宏独立到"XC_TimeCount.h";
 |      3.所有兼容处理独立到"XC_TimeCompatibility.h"文件;
 *========================================================*/
//=== 防重复定义
#ifndef _XC_Time_H_
#define _XC_Time_H_
//=== 头文件
#include "XC_Type.h"

/************************************************ 我是分割线 ************************************************/

#include "XC_TimeCount.h"       //时间计数处理
#include "XC_TimeCompile.h"     //编译时间处理

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 数据类型

/*日历变量类型*/
typedef struct{
    ushort Year;                //年[1~65535]
    uchar  Month;               //月[1~12]
    uchar  Date;                //日(日期)[1~31]
    uchar  Hour;                //时[0~23]
    uchar  Minute;              //分[0~59]
    uchar  Second;              //秒[0~59]
}XCTime_Calendar_t;

/*时间参数*/
typedef enum{
    _Time_P_Second = (0),       //秒
    _Time_P_Minute = (1),       //分钟
    _Time_P_Hour   = (2),       //小时
    _Time_P_Day    = (3),       //天数
}XCTime_ParamTime_t;

//=== 宏定义

/*固定时间(Unix时间,时区:加*|减*[A*|S*])*/
#define _Time_UnixA8_20170101_000000    (1483200000)    /*2017/01/01-00:00:00+8*/
#define _Time_UnixA8_20180101_000000    (1514736000)    /*2018/01/01-00:00:00+8*/
#define _Time_UnixA8_20190101_000000    (1546272000)    /*2019/01/01-00:00:00+8*/
#define _Time_UnixA8_20190301_000000    (1551369600)    /*2019/03/01-00:00:00+8*/

/*32位情况下最大时间(Unix时间,时区:加*|减*[A*|S*])*/
#define _Time_UnixA8_MaxValue           (0xFFFFFFFF-(8*_Time_h2s))  /*+8区*/

//=== 函数声明

/*基础计算*/

uint8_t XCTime_IsLeapYear(uint16_t Year);                               //闰年判断
uint16_t XCTime_GetLeapYearNum(uint16_t FrontYear, uint16_t BackYear);  //计算两个年之间的闰年数量

/*日历转换*/

uint32_t XCTime_SetYear_CalendarToSec(uint16_t InitYear, XCTime_Calendar_t *pCalendar);             //设置初始年,日历转秒
void XCTime_SetYear_SecToCalendar(uint16_t InitYear, uint32_t Sec, XCTime_Calendar_t *pCalendar);   //设置初始年,秒转日历

/*时间计算*/

uint32_t XCTime_TwoDateComputeDiff(XCTime_Calendar_t *Calendar1, XCTime_Calendar_t *Calendar2);     //计算2个日期的差值
uint32_t XCTime_ThisYear(XCTime_Calendar_t *pt, XCTime_ParamTime_t ParamTime);                      //输入日期,返回今年日期的x值(x=天,时,分,秒)
uint32_t XCTime_CalculateTimeDiff(XCTime_Calendar_t Calendar[2], XCTime_ParamTime_t ParamTime);     //计算2个日期的差值

/*Unix时间计算*/

uint32_t XCTime_CalendarToUnix(XCTime_Calendar_t *pCalendar, int8_t Timezone);                      //由日历计算Unix时间戳
void XCTime_UnixToCalendar(uint32_t UnixSec, int8_t Timezone, XCTime_Calendar_t *pCalendar);        //由Unix时间戳计算日历

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

#include "XC_TimeCompatibility.h"   //兼容处理

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
