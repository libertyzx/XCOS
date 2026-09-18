/**
 * @file        XCOS.h
 * @brief       框架包含
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.1
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     头文件合并,使用时只要包含此文件即可;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XCOS_h
#define XCOS_h

/************************************************ 我是分割线 ************************************************/

/**
 * XCOS 版本信息
 *
 * 版本规则遵循语义化版本 (Semantic Versioning 2.0.0):
 * - 主版本号 (MAJOR): 不兼容的API修改
 * - 次版本号 (MINOR): 向下兼容的功能性新增
 * - 修订号   (PATCH): 向下兼容的问题修正
 */
#define XCOS_VER_MAJOR  (2U) // 主版本号
#define XCOS_VER_MINOR  (1U) // 次版本号
#define XCOS_VER_PATCH  (1U) // 修订号

/**
 * 辅助版本信息
 * "XCOS_VER_STRING":字符串版本;
 * "XCOS_VER_NUMBER":版本数值,24位数值,格式:版本号-次版本号-修订号;
 */
#define XCOS_VER_STRING "2.1.1"                                                             // 字符串版本
#define XCOS_VER_NUMBER ((XCOS_VER_MAJOR << 16U) | (XCOS_VER_MINOR << 8U) | XCOS_VER_PATCH) // 版本数值

/**
 * @brief XCOS框架API兼容性标识
 * @details
 * - 此宏必须存在，用于标识XCOS框架的可用性
 * - 指向主版本号，便于版本兼容性检查
 * - 用户代码可通过检查此宏确认框架是否被正确包含
 */
#define XCOS_API_LEVEL  (XCOS_VER_MAJOR)

/************************************************ 我是分割线 ************************************************/
//=== 头文件

#include "XC_BitPatterns.h" //二进制值
#include "XC_Config.h"      //配置
#include "XC_Diag.h"        //诊断能力(断言/运行期自检/运行统计; 开关 XC_CFG_*; 可物理裁剪 XC_Diag.c)
#include "XC_Err.h"         //错误上报(错误码/用户钩子/上报宏; 开关 XC_CFG_ERR_HOOK; **无实现文件**)
#include "XC_Type.h"        //类型

#include "XC_Cor.h"  //协程处理
#include "XC_Sch.h"  //调度处理
#include "XC_Task.h" //任务处理
#include "XC_Time.h" //时间处理

#include "XC_Compat.h" //向后兼容宏(旧版函数/宏名); 仅供迁移期使用, 计划 V2.2.0 删除(见 Docs/XCOS.md「向后兼容」)

/************************************************ 我是分割线 ************************************************/

/**说明详见"README.md"*/

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
