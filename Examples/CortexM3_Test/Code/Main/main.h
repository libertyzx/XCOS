/**
 ******************************************************************************
 * @file    GPIO/GPIO_IOToggle/Inc/main.h
 * @author  MCD Application Team
 * @brief   Header for main.c module
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "XCOS.h"
#include "stm32f1xx_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/**
 *  配置开关
 *  "_Cnf_Examples"不同值功能不同
 *  - 0:关闭所有框架代码;
 *  - 1:只开启框架,无空闲回调,无任务;
 *  - 2:12个测试任务(A0~A11),有中断处理,空闲回调为阻塞延时模拟休眠;
 *    其中 A10/A11 为 V2.1.0 协程嵌套与分步注册测试;
 *  可在编译选项(-D_Cnf_Examples=N)覆盖此默认值;
 */
#ifndef _Cnf_Examples
#define _Cnf_Examples (2)
#endif

/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */
