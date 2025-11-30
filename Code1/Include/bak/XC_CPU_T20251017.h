/**
 * @file        XC_CPU.h
 * @brief       由不同CPU,编译器判断不同处理
 * @author      libertyzx (libertyzx@163.com)
 * @version     1.00
 * @date        2023/12/23
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 * 注意在不同编译器下实现 \n
 *  汇编,内联,空操作,弱定义等的定义统一;
 * ---
 *  适配编译器:
 *      - Keil-MDK
 *      - Keil-C51
 *      - IAR-ARM
 *      - IAR-C51
 *      - IAR-STM8
 *      - gcc
 *  适配核心:
 *      - 8051
 *      - ARM-CortexM*
 *      - STM8
 *  8051特定类型关键字说明:
 *      - code  : 代码存储区（64KB）, 由MOVC @DPTR访问
 *      - xdata : (extend)片外数据存储区（64KB）, 由MOVX @ DPTR访问。
 *      - bdata : (bit)可位寻址片内数据存储区, 允许位与字节混合访问（0X20-0X2F,16字节）
 *      - data  : 直接寻址片内数据存储区, 访问速度快（0X00-0X7F,128字节）
 *      - idata : (indirect)间接寻址片内数据存储区, 访问片内全部RAM空间（8052,256字节）
 *      - pdata : (page)分页寻址外部数据存储区（256字节）由MOVX @R0访问
 * **********************************************
 *  修改日志
 *  - 2023/12/23
 *      - 版本: 1.00
 *      - 由"XCBase"库中"Macro.h"(V0.32)独立分离出与编译器和CPU相关的数据;
 *      - 修正位数错误;
 */
/************************************************ 我是分割线 ************************************************/
//=== 防重复定义
#ifndef _XC_CPU_H_
#define _XC_CPU_H_

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/** 对应不同编译器适配关键字 */

#if (defined(__CC_ARM) || defined(__ARMCC_VERSION))
/**
 *  KEIL-MDK 编译环境
 *  ARM-CortexMx-32bit 架构
 */
#define _CPUbit (32) // CPU位数
// 声明汇编函数
#ifndef __ASM
#define __ASM __asm
#endif
// 内联
#ifndef __INLINE
#define __INLINE __inline
#endif
// 私有内联
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static __INLINE
#endif
// 空操作
#ifndef _NOP
#define _NOP() __nop()
#endif
// 弱定义标志
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif
// 代码存储区
#ifndef __ROM
#define __ROM const
#endif

/** 其他特有 */

// 变量指定地址
#ifndef __SetAddr
#define __SetAddr(_Addr) __attribute__((at(_Addr)))
#endif

/************************************************ 我是分割线 ************************************************/

#elif defined(__ICCARM__)
/**
 *  IAR-ARM 编译环境
 *  ARM-CortexMx-32bit 架构
 */
#define _CPUbit (32) // CPU位数
// 声明汇编函数
#ifndef __ASM
#define __ASM __asm
#endif
// 内联
#ifndef __INLINE
#define __INLINE inline
#endif
// 私有内联
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static __INLINE
#endif
// 空操作
#ifndef _NOP
#define _NOP() __nop()
#endif
// 弱定义标志
#ifndef __WEAK
#define __WEAK __weak
#endif
// 代码存储区
#ifndef __ROM
#define __ROM const
#endif

/************************************************ 我是分割线 ************************************************/

#elif (defined(__C51__)) || (defined(__CX51__))
/**
 *  KEIL-C51 编译环境
 *  8051-8bit 架构
 */
#define _CPUbit (8) // CPU位数
// 声明汇编函数(未实现)
// 内联(未实现)
// 私有内联(未实现)
// 空操作
#ifndef _NOP
extern void _nop_(void); //_nop_()是外部函数
#define _NOP() _nop_()
#endif
// 弱定义(未实现)
// 代码存储区
#ifndef __ROM
#define __ROM code
#endif

/** 其他特有 */

// 可位寻址片内数据存储区
#define bit8  unsigned char bdata  // 8bit (可位寻址)
#define bit16 unsigned short bdata // 16bit(可位寻址)
#define bit32 unsigned long bdata  // 32bit(可位寻址)

/************************************************ 我是分割线 ************************************************/

#elif defined(__ICC8051__)
/**
 *  IAR-C51 编译环境
 *  8051-8bit 架构
 */
#define _CPUbit (8) // CPU位数
// 声明汇编函数(未实现)
// 内联(未实现)
// 私有内联(未实现)
// 空操作
#ifndef _NOP
#include <intrinsics.h> //__no_operation()
#define _NOP() __no_operation()
#endif
// 弱定义(未实现)

/** 其他特有 */

// 特定变量
#ifndef code
#define code __code const // 代码存储区
#endif
#ifndef data
#define data __data // 直接寻址片内数据存储区
#endif
#ifndef xdata
#define xdata __xdata // 片外数据存储区
#endif
#ifndef bdata
#define bdata __bdata // 片外数据存储区
#endif
#ifndef idata
#define idata __idata // 间接寻址片内数据存储区
#endif
#ifndef pdata
#define pdata __pdata // 分页寻址外部数据存储区
#endif
#ifndef bit
#define bit __no_init __bit _Bool // 位定义
#endif
// 可位寻址片内数据存储区
#define bit8  unsigned char bdata  // 8bit (可位寻址)
#define bit16 unsigned short bdata // 16bit(可位寻址)
#define bit32 unsigned long bdata  // 32bit(可位寻址)

/************************************************ 我是分割线 ************************************************/

#elif defined(__ICCSTM8__)
/**
 *  IAR-STM8 编译环境
 *  STM8-8bit 架构
 */
#define _CPUbit (8) // CPU位数
// 声明汇编函数(未实现)
// 内联(未实现)
// 私有内联(未实现)
// 空操作
#ifndef _NOP
#define _NOP() __no_operation()
#endif
/**
 *  弱定义标志
 *  若是在IAR中使用弱定义,则需要勾选"Multi-file Compilation";
 *  否则会造成调用强定义时覆盖被调用的函数的变量;
 */
#ifndef __WEAK
#define __WEAK __weak
#endif
// 代码存储区
#ifndef __ROM
#define __ROM const
#endif

/************************************************ 我是分割线 ************************************************/

#elif defined(__GNUC__)
/**
 *  GCC 编译环境
 */
#define _CPUbit (sizeof(int) * 8) // CPU位数
// 声明汇编函数(未实现)
// 内联
#ifndef __INLINE
#define __INLINE __inline
#endif
// 私有内联
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static __INLINE
#endif
// 弱定义标志(未实现)
// 代码存储区
#ifndef __ROM
#define __ROM const
#endif

/************************************************ 我是分割线 ************************************************/

#elif defined(_M_IX86)
/**
 *  WIN-X86 编译环境
 */
#define _CPUbit (32) // CPU位数
// 声明汇编函数(未实现)
// 内联
#ifndef __INLINE
#define __INLINE __inline
#endif
// 私有内联
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static __INLINE
#endif
// 弱定义标志
/*window下没有弱函数关键字*/
// 代码存储区
#ifndef __ROM
#define __ROM const
#endif

/************************************************ 我是分割线 ************************************************/

#elif defined(_M_X64)
/**
 *  WIN-X64 编译环境
 */
#define _CPUbit (64) // CPU位数
// 声明汇编函数(未实现)
// 内联
#ifndef __INLINE
#define __INLINE __inline
#endif
// 私有内联
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static __INLINE
#endif
// 弱定义标志
/*window下没有弱函数关键字*/
// 代码存储区
#ifndef __ROM
#define __ROM const
#endif

/************************************************ 我是分割线 ************************************************/

#else
/**
 *  默认环境
 */
#define _CPUbit (32) // CPU位数
// 声明汇编函数(未实现)
// 内联
#ifndef __INLINE
#define __INLINE __inline
#endif
// 私有内联
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static __INLINE
#endif
// 弱定义标志(未实现)
// 代码存储区
#ifndef __ROM
#define __ROM const
#endif

//===
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

// 空操作
#ifndef NOP
#define NOP() _NOP() // 空操作
#endif
#ifndef Nop
#define Nop() _NOP() // 空操作
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
