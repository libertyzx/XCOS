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
 *  - 2:一个延时任务,无空闲回调;
 *  - 3:10个测试任务,有中断处理,空闲回调为阻塞延时模拟休眠;
 */
#define _Cnf_Examples (3)

/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */
