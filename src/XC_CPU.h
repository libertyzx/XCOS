/*=========================================================|
 | 文件名: XC_CPU.h
 | 描述  : 由不同CPU,编译器判断不同处理
 | 版本  : V1.00
 | 日期  : 2023/12/23
 | 语言  : C语言
 | 作者  : libertyzx
 | E-mail: libertyzx@163.com
 +-----------------------------------------------|
 | 开源协议: MIT License
 +-----------------------------------------------|
 +--- 说明
 |  适配编译器:
 |      Keil-MDK
 |      Keil-C51
 |      IAR-ARM
 |      IAR-C51
 |      IAR-STM8
 |      gcc
 |  适配核心:
 |      8051
 |      ARM-CortexM*
 |      STM8
 +-----------------------------------------------|
 +--- 8051特定类型关键字说明
 |  code     代码存储区（64KB）, 由MOVC @DPTR访问
 |  xdata    (extend)片外数据存储区（64KB）, 由MOVX @ DPTR访问。
 |  bdata    (bit)可位寻址片内数据存储区, 允许位与字节混合访问（0X20-0X2F,16字节）
 |  data     直接寻址片内数据存储区, 访问速度快（0X00-0X7F,128字节）
 |  idata    (indirect)间接寻址片内数据存储区, 访问片内全部RAM空间（8052,256字节）
 |  pdata    (page)分页寻址外部数据存储区（256字节）由MOVX @R0访问
 +-----------------------------------------------|
 +--- 版本说明:
 |  V1.00:-2023/12/23
 |      1.由"XCBase"库中"Macro.h"(V0.32)独立分离出与编译器和CPU相关的数据;
 |      2.修正位数错误;
 *========================================================*/
//=== 防重复定义
#ifndef _XC_CPU_H_
#define _XC_CPU_H_
//=== 头文件
#include <stdint.h>
#include <stddef.h>
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/**对应不同编译器适配关键字*/

//=== KEIL-MDK使用这种 =====================================|
#if( defined(__CC_ARM) || defined(__ARMCC_VERSION) )
    /*ARM-CortexMx-32bit环境*/
    //CPU位数
    #define _CPUbit             (32)
    //声明汇编函数
    #ifndef __ASM
        #define __ASM           __asm
    #endif
    //内联
    #ifndef __INLINE
        #define __INLINE        __inline
    #endif
    //私有内联
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE static __INLINE
    #endif
    //空操作
    #ifndef _NOP
        #define _NOP()          __nop()
    #endif
    //弱定义标志
    #ifndef __WEAK
        #define __WEAK          __attribute__((weak))
    #endif
    //变量指定地址
    #ifndef __SetAddr
        #define __SetAddr(_Addr)__attribute__((at(_Addr)))
    #endif
    //代码存储区
    #define __ROM               const
    //特定包含
    #include "CortexMx.h"
//=== IAR-ARM使用这种 ======================================|
#elif defined(__ICCARM__)
    /*ARM-CortexMx-32bit环境*/
    //CPU位数
    #define _CPUbit             (32)
    //声明汇编函数
    #ifndef __ASM
        #define __ASM           __asm
    #endif
    //内联
    #ifndef __INLINE
        #define __INLINE        inline
    #endif
    //私有内联
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE static __INLINE
    #endif
    //空操作
    #ifndef _NOP
        #define _NOP()          __nop()
    #endif
    //弱定义标志
    #ifndef __WEAK
        #define __WEAK          __weak
    #endif
    //代码存储区
    #define __ROM               const
    //特定包含
    #include "CortexMx.h"
//=== KEIL-C51使用这种 =====================================|
#elif (defined(__C51__)) || (defined(__CX51__))
    /*8051-8bit环境*/
    //CPU位数
    #define _CPUbit             (8)
    //空操作
    #ifndef _NOP
        extern void _nop_(void);                            //_nop_()是外部函数
        #define _NOP()          _nop_()
    #endif
    //可位寻址片内数据存储区
    #define bit8                unsigned char  bdata        //8bit (可位寻址)
    #define bit16               unsigned short bdata        //16bit(可位寻址)
    #define bit32               unsigned long  bdata        //32bit(可位寻址)
    //代码存储区
    #define __ROM               code
//=== IAR-C51使用这种 ======================================|
#elif defined(__ICC8051__)
    /*8051-8bit环境*/
    //CPU位数
    #define _CPUbit             (8)
    //空操作
    #ifndef _NOP
        #include <intrinsics.h>                             //__no_operation()
        #define _NOP()          __no_operation()
    #endif
    //特定变量
    #ifndef code
        #define code            __code const                //代码存储区
    #endif
    #ifndef data
        #define data            __data                      //直接寻址片内数据存储区
    #endif
    #ifndef xdata
        #define xdata           __xdata                     //片外数据存储区
    #endif
    #ifndef bdata
        #define bdata           __bdata                     //片外数据存储区
    #endif
    #ifndef idata
        #define idata           __idata                     //间接寻址片内数据存储区
    #endif
    #ifndef pdata
        #define pdata           __pdata                     //分页寻址外部数据存储区
    #endif
    #ifndef bit
        #define bit             __no_init __bit _Bool       //位定义
    #endif
    //可位寻址片内数据存储区
    #define bit8                unsigned char  bdata        //8bit (可位寻址)
    #define bit16               unsigned short bdata        //16bit(可位寻址)
    #define bit32               unsigned long  bdata        //32bit(可位寻址)
//=== IAR-STM8使用这种 =====================================|
#elif defined(__ICCSTM8__)
    /*STM8-8bit环境*/
    //CPU位数
    #define _CPUbit             (8)
    //空操作
    #ifndef _NOP
        #define _NOP()           __no_operation()
    #endif
    /*弱定义标志
     *  若是在IAR中使用弱定义,则需要勾选"Multi-file Compilation";
     *  否则会造成调用强定义时覆盖被调用的函数的变量;
     */
    #ifndef __WEAK
        #define __WEAK          __weak
    #endif
    //代码存储区
    #define __ROM               const
//=== GCC ==================================================|
#elif defined(__GNUC__)
    //CPU位数
    #define _CPUbit             (sizeof(int)*8)
    //内联
    #ifndef __INLINE
        #define __INLINE        __inline
    #endif
    //私有内联
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE static __INLINE
    #endif
    //代码存储区
    #define __ROM               const
//=== x86 ==================================================|
#elif defined(_M_IX86)
    //CPU位数
    #define _CPUbit             (32)
    //内联
    #ifndef __INLINE
        #define __INLINE        __inline
    #endif
    //私有内联
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE static __INLINE
    #endif
    //弱定义标志
    /*window下没有弱函数关键字*/
    //代码存储区
    #define __ROM               const
//=== x64 ==================================================|
#elif defined(_M_X64)
    //CPU位数
    #define _CPUbit             (64)
    //内联
    #ifndef __INLINE
        #define __INLINE        __inline
    #endif
    //私有内联
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE static __INLINE
    #endif
    //弱定义标志
    /*window下没有弱函数关键字*/
    //代码存储区
    #define __ROM               const
//=== 默认环境 =============================================|
#else
    //CPU位数
    #define _CPUbit             (32)
    //内联
    #ifndef __INLINE
        #define __INLINE        __inline
    #endif
    //私有内联
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE static __INLINE
    #endif
    //代码存储区
    #define __ROM               const
//===
#endif
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

//空操作
#ifndef NOP
    #define NOP()           _NOP()      //空操作
#endif
#ifndef Nop
    #define Nop()           _NOP()      //空操作
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
