/**
 * @file        XC_CorANSI.h
 * @brief       协程库(ANSI-C实现)
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2018 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  使用宏"__LINE__"来定义上下文断点;
 *  由"switch case"实现;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
/************************************************ 我是分割线 ************************************************/
//=== 防重复定义
#ifndef XC_CorANSI_h
#define XC_CorANSI_h

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 数据类型 */

/**
 * @brief       协程断点类型
 * @details     用于断点时保存当前行的值(__LINE__值)
 */
typedef unsigned long COR_BP_t;

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 协程实现 */

/**
 * @brief       协程-断点初始化
 * @param[in]   BP  [COR_BP_t]协程断点
 * @details     初始化断点;
 */
#define COR_Init(BP) ((BP) = 0UL)

/**
 * @brief       协程-代码开头
 * @param[in]   BP  [COR_BP_t]协程断点
 * @details     协程块的开始(状态机的开始);
 */
#define COR_Start(BP) \
    switch((BP)) {    \
        case 0:

/**
 * @brief       协程-设置断点
 * @param[in]   BP  [COR_BP_t]协程断点
 * @details     将当前行号保存到断点中;
 */
#define COR_SetBP(BP)                 \
    (BP) = (unsigned long)(__LINE__); \
    case __LINE__:;

/**
 * @brief       协程-跳出
 * @param[in]   BP  [COR_BP_t]协程断点
 * @details     直接跳出协程
 */
#define COR_Break(BP) \
    goto COR_GOTO_END;

/**
 * @brief       协程-设置断点并跳出
 * @param[in]   BP  [COR_BP_t]协程断点
 * @details     将当前行号保存到断点中,然后跳出;
 */
#define COR_SetBPBreak(BP)            \
    (BP) = (unsigned long)(__LINE__); \
    goto COR_GOTO_END;                \
    case __LINE__:;

/**
 * @brief       协程-代码结束
 * @details     协程块的结束(状态机的结尾);
 */
#define COR_End() \
    default:      \
        break;    \
        }         \
    COR_GOTO_END: /*结束跳转标志*/

/**
 * @brief       协程-跳出并放置返回标签(供 XC_Cor_Call 使用,不设断点)
 * @details
 *  返回点已由 XC_Task_PushFrame 存入帧,故只需"跳出 + 标签";
 *  goto 跳到 switch 外的 COR_GOTO_END,跳过 Leave 的 CorState = DONE 赋值,保持挂起;
 */
#define COR_BreakLabel()   \
    {                      \
        goto COR_GOTO_END; \
        case __LINE__:;    \
    }

/**
 * @brief       协程-返回点表达式(供 XC_Task_PushFrame 传参)
 * @return      COR_BP_t  返回当前行的行号(__LINE__值)
 * @details
 *  与 COR_BreakLabel 同处 XC_Cor.h 的外层宏内展开,__LINE__ 为同一用户行号,
 *  保证帧内返回点与恢复标签匹配;
 */
#define COR_RetPoint() ((COR_BP_t)(__LINE__))

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
