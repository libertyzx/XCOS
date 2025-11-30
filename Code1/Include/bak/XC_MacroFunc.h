/**
 * @file        XC_MacroFunc.h
 * @brief       通用函数宏定义
 * @author      libertyzx (libertyzx@163.com)
 * @version     1.00
 * @date        2024/02/26
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     一些常用的宏定义
 * **********************************************
 *  修改日志
 *  - 2024/02/26
 *      - 版本:1.00
 *      - 由"XCBase"库中"Macro_Func.h"(V0.3)中更新的函数宏;
 *      - 删除不常用的宏,增加位处理宏;
 */
/************************************************ 我是分割线 ************************************************/
//=== 防重复定义
#ifndef _XC_MacroFunc_H_
#define _XC_MacroFunc_H_
//=== 头文件
#include "XC_Type.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/** 二进制位操作 */

/**
 * @brief       二进制0输出(低_Len位为0,剩余位为1)
 * @param[in]   _Len    [uint8_t]总共几位0(对应低位,值为:1-32)
 * @details
 *  uint8_t  Cache =  (uchar)_Bin0Out(3);   //8位数据, 输出:_B1111 11000 \n
 *  uint16_t Cache = (ushort)_Bin0Out(3);   //16位数据,输出:_B1111 1111 1111 1000 \n
 *  uint32_t Cache =  (ulong)_Bin0Out(3);   //32位数据,输出:_B1111 1111 1111 1111 1111 1111 1111 1000

 */
#define _Bin0Out(_Len)                    (((uint32_t)(~0) >> (_Len)) << (_Len))

/**
 * @brief       二进制1输出(低_Len位为1,剩余位为0)
 * @param[in]   _Len    [uint8_t]总共几位1(对应低位,值为:1-32)
 * @details
 *  uint8_t  Cache =  (uchar)_Bin1Out(1);   //8位数据, 输出:_B1 \n
 *  uint16_t Cache = (ushort)_Bin1Out(3);   //16位数据,输出:_B111 \n
 *  uint32_t Cache =  (ulong)_Bin1Out(9);   //32位数据,输出:_B0001 1111 1111
 */
#define _Bin1Out(_Len)                    (~(_Bin0Out(_Len)))

/************************************************ 我是分割线 ************************************************/

/** 对应数据清除&置位 */

/**
 * @brief       清除n位数据
 * @param[in]   _Dest       [任意位类型]需要清除位的数据(最大32位类型)
 * @param[in]   _Offset     [uint8_t]清除位的偏移(低位开始,值为:0-31)
 * @param[in]   _BitLen     [uint8_t]清零的位数(值为:1-32)
 * @details
 *  uint16_t Cache = 0xFFFF; \n
 *  _BitClr(Cache, 3, 3); \n
 *  > Cache初值(0xFFFF): 1111 1111 1111 1111 \n
 *  > Cache得到(0xFFC7): 1111 1111 1100 0111
 */
#define _BitClr(_Dest, _Offset, _BitLen)  ((_Dest) &= ~(_Bin1Out(_BitLen) << (_Offset)))

/**
 * @brief       设置n位数据
 * @param[in]   _Dest       [任意位类型]需要设置位的数据(最大32位类型)
 * @param[in]   _Offset     [uint8_t]设置位的偏移(低位开始,值为:0-31)
 * @param[in]   _BitLen     [uint8_t]设置的位数(值为:1-32)
 * @details
 *  uint16_t Cache = 0xF000; \n
 *  _BitSet(Cache, 3, 3);  \n
 *  > Cache初值(0xF000): 1111 0000 0000 0000 \n
 *  > Cache得到(0xF038): 1111 0000 0011 1000
 */
#define _BitSet(_Dest, _Offset, _BitLen)  ((_Dest) |= (_Bin1Out(_BitLen) << (_Offset)))

/**
 * @brief       写n位数据
 * @param[in]   _Dest       [任意位类型]需要写入目标数据(最大32位类型)
 * @param[in]   _Offset     [uint8_t]写入数据的偏移(低位开始,0-31)
 * @param[in]   _Src        [任意位类型]写入的数据
 * @details
 *  不会清除原来的数据,只是与上; \n
 *  uint32_t Cache = 0xF810; \n
 *  _BitWrite(Cache, 3, 3); \n
 *  > Cache初值(0xF810): 1111 1000 0001 0000 \n
 *  > Cache得到(0xF818): 1111 1000 0001 1000
 */
#define _BitWrite(_Dest, _Offset, _Src)   ((_Dest) |= ((_Src) << (_Offset)))

/**
 * @brief       读n位数据
 * @param[in]   _Dest       [任意位类型]需要读取的目标数据(最大32位类型)
 * @param[in]   _Offset     [uint8_t]读取数据的偏移(低位开始,0-31)
 * @param[in]   _Src        [任意位类型]读取的位数
 * @return      读取到的数据;
 * @details
 *  读取的数据会位移到最低位; \n
 *  uint32_t Cache = 0xFF5F; \n
 *  Cache = _BitRead(Cache, 3, 4); \n
 *  > Cache初值(0xFF5F):    1111 1111 0101 1111 \n
 *  > 读的偏移(3)和位数(4):            ^^^ ^
 *  > Cache得到(0xB): 1011
 */
#define _BitRead(_Dest, _Offset, _BitLen) (((_Dest) >> (_Offset)) & _Bin1Out(_BitLen))

/**
 * @brief       更新n位数据(清除并写数据)
 * @param[in]   _Dest       [任意位类型]需要写入目标数据(最大32位类型)
 * @param[in]   _Offset     [uint8_t]写入数据的偏移(低位开始,0-31)
 * @param[in]   _BitLen     [uint8_t]设置的位数(1-32)
 * @param[in]   _Src        [任意位类型]写入的数据
 * @details
 *  会清除对应的位数,并写入数据; \n
 *  注意:"_Src"值会确认有效位数(_BitLen); \n
 *  参考:
 *      uint32_t Cache = 0xFFFF;
 *      _BitUpdate(Cache, 3, 9, 0xA);
 *      > Cache初值(0xFFFF):      1111 1111 1111 1111 \n
 *      > 移(3)位写入(9)位的(0xA):     0000 0101 0    \n
 *      > 得到:(0xF057)           1111 0000 0101 0111 \n
 */
#define _BitUpdate(_Dest, _Offset, _BitLen, _Src) \
    ((_Dest) = ((_Dest) & (~(_Bin1Out(_BitLen) << (_Offset)))) | (((_Src) & _Bin1Out(_BitLen)) << (_Offset)))

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/**
 * @brief       获取数组元素个数
 * @param[in]   _a  数组名
 * @details
 *  sizeof(数组名);可以得到整个数组的占字节数; \n
 *  sizeof(数组名[0]);可以得到数组中单个元素大小; \n
 *  sizeof(数组名) / sizeof(数组名[0]) 即可得到数组元素大小; \n
 *  注意,只可用在数组,或数组指针上,若是纯指针或者形参是指针则不支持;
 */
#define _ArraySize(_a)                                        (sizeof((_a)) / sizeof((_a)[0]))

/**
 * @brief       获取两个数中最大值
 * @param[in]   _D1  [任意位类型]数值1
 * @param[in]   _D2  [任意位类型]数值2
 * @return       _D1和_D2中大的值;
 */
#define _Max(_D1, _D2)                                        (((_D1) > (_D2)) ? (_D1) : (_D2))

/**
 * @brief       获取两个数中最小值
 * @param[in]   _D1  [任意位类型]数值1
 * @param[in]   _D2  [任意位类型]数值2
 * @return       _D1和_D2中小的值;
 */
#define _Min(_D1, _D2)                                        (((_D1) < (_D2)) ? (_D1) : (_D2))

/**
 * @brief       判断一个数是不是2的n次方
 * @param[in]   _V      [任意位类型]数值
 * @return      boot    1表示是2的n次方;0则不是;
 */
#define _TwoN(_V)                                             (((_V) != 0) && (((_V) & ((_V) - 1)) == 0))

/**
 * @brief       获取绝对值
 * @param[in]   _V      [任意位类型]数值
 * @return      输出绝对值
 */
#define _ABS(_V)                                              ((_V) < 0 ? (-(_V)) : (_V))

/**
 * @brief       设置不使用的变量
 * @param[in]   _x      变量名
 * @details     不使用的变量用这个宏处理,防止编译器报警;
 */
#define _Unused(_x)                                           ((void)(_x))

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       从地址获取数据(8位)
 * @param[in]   _Addr       地址(整形变量/常量)
 * @return      获取地址中存储的数据
 */
#define _GetDataFromAddr8(_Addr)                              (*((volatile uint8_t*)(_Addr)))

/**
 * @brief       从地址获取数据(16位)
 * @param[in]   _Addr       地址(整形变量/常量)
 * @return      获取地址中存储的数据
 */
#define _GetDataFromAddr16(_Addr)                             (*((volatile uint16_t*)(_Addr)))

/**
 * @brief       从地址获取数据(32位)
 * @param[in]   _Addr       地址(整形变量/常量)
 * @return      获取地址中存储的数据
 */
#define _GetDataFromAddr32(_Addr)                             (*((volatile uint32_t*)(_Addr)))

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       16进制数据转字符
 * @param[in]   _Hex    输入整型数据:0x0 ~ 0xF 即(0-15)
 * @return      char    字符('0'-'9','A'-'F')
 * @details     若是数据大于15,则只转换低4位,只输出大写字符;
 */
#define _Hex2Char(_Hex)                                       ("0123456789ABCDEF"[(_Hex) & 0x0F])

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       由结构体成员获取结构体指针
 * @param[in]   _StructType     结构体的类型
 * @param[in]   _MemberName     结构体成员名(只是名字)
 * @param[in]   _MemberAddr     结构体成员地址(指针)
 * @return      对应结构体类型的指针;
 * @details
 *  注意:只能从1级成员得到结构体的指针;
 *  示例:
 *      typedef struct {
 *          uint32_t aa;
 *          uint32_t bb;
 *      }Test_t;
 *      Test_t Cache;
 *      Test_t *p;
 *      p = _GetStructAddr(Test_t, bb, &Cache->bb);
 *  //"p"的值等于"&Cache"的值;
 */
#define _GetStructAddr(_StructType, _MemberName, _MemberAddr) ((_StructType*)(((uint8_t*)(_MemberAddr)) - offsetof(_StructType, _MemberName)))

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
