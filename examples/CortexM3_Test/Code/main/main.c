
/**
 * @file        main.c
 * @brief       主文件
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/10/30
 * **********************************************
 * @copyright   Copyright (c) 2023 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     演示XCOS的主文件
 *  :
 *  Program Size: Code=3404 RO-data=380 RW-data=16 ZI-data=1024
 * **********************************************
 *  修改日志
 *  - 2025/10/30
 *      - 初始编写
 */
//=== 头文件
#include "main.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */



// XCOS变量
_XC_CreateSysTickCount; // 创建系统Tick
XCOS_t s_hXCOS0;        // XCOS句柄

#if (_Cnf_RunTask == 1)
XCTCB_t  s_hTCBn[10]     = { 0 }; // 任务控制块
uint32_t s_TickCount[10] = { 0 }; // 保存计数
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
#if (_Cnf_RunTask == 1)
/**
 * @brief       任务0
 * @param[in]   phTCB   任务控制块
 * @details
 *  基础框架,演示延时和让出控制,后被A6删除
 */
void Task_A0(XCTCB_t* phTCB)
{
    XC_Enter(phTCB); // 协程任务块开始标志
    /** --- */
    s_TickCount[0] = 0;
    while(1) {
        s_TickCount[0]++;
        XC_Delay_ms(15); // 延时
        XC_Yield();      // 让出控制
    }
    /** --- */
    XC_Leave(); // 协程任务块结束标志
}

/**
 * @brief       任务1
 * @param[in]   phTCB   任务控制块
 * @details
 *  通知处理演示,等待A2通知唤醒任务;
 */
void Task_A1(XCTCB_t* phTCB)
{
    uint32_t Cache;

    XC_Enter(phTCB);
    /** --- */
    s_TickCount[1] = 0;
    while(1) {
        XC_WaitNotify_ms(555);                    // 等待通知,超时555ms
        if(XC_GetNotifyWakeState() == 0) {        // 未超时,通知到达
            Cache = (uint32_t)XC_GetNotifyData(); // 获取传递的参数
            if(Cache == 2) {                      // 传递参数是2时
                s_TickCount[1]++;                 // 计数+1
            }
        }
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务2
 * @param[in]   phTCB   任务控制块
 * @details
 *  通知处理演示,唤醒A1
 */
void Task_A2(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_TickCount[2] = 0;
    while(1) {
        XC_Delay_ms(15);                      // 延时
        XC_SendNotify(&s_hTCBn[1], (void*)7); // 唤醒任务1,传递参数7
        XC_Delay_ms(15);                      // 延时
        XC_SendNotify(&s_hTCBn[1], (void*)2); // 唤醒任务1,传递参数2
        s_TickCount[2]++;                     // 计数+1
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务3
 * @param[in]   phTCB   任务控制块
 * @details
 *  挂起恢复演示,挂起自身,等待被A4恢复
 */
void Task_A3(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_TickCount[3] = 0;
    while(1) {
        /**
         * 挂起自身;
         * 可在其他任务调用"XC_TaskSuspend"挂起指定任务;
         */
        XC_Suspend();
        s_TickCount[3]++;
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务4
 * @param[in]   phTCB   任务控制块
 * @details
 *  挂起恢复演示,恢复任务3
 */
void Task_A4(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_TickCount[4] = 0;
    while(1) {
        XC_Delay_ms(19);            // 延时
        XC_TaskResume(&s_hTCBn[3]); // 挂起恢复任务3
        s_TickCount[4]++;
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务5
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示复位
 */
void Task_A5(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_TickCount[5] = 0;
    while(1) {
        XC_Delay_ms(10); // 延时
        s_TickCount[5]++;
        /**
         *  计数到达10则复位任务,复位后任务重头运行,计数会被清零;
         *  在其他任务中调用"XC_TaskReset"效果一样;
         */
        if(s_TickCount[5] == 10) {
            XC_Reset();
        }
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务6
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示移除任务;计数到达指定值,移除A0
 */
void Task_A6(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_TickCount[6] = 0;
    while(1) {
        XC_Delay_ms(10);                // 延时
        if(s_TickCount[6] == 100) {     // 计数到达100
            XC_TaskRemove(&s_hTCBn[0]); // 移除任务0
        }
        s_TickCount[6]++;
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务7
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示中断通知唤醒任务,需要手动中断;
 */
void Task_A7(XCTCB_t* phTCB)
{
    uint32_t Cache;

    XC_Enter(phTCB);
    /** --- */
    s_TickCount[7] = 0;
    while(1) {
        XC_WaitNotify_ms(1001);                   // 等待通知
        if(XC_GetNotifyWakeState() == 0) {        // 未超时,通知到达
            Cache = (uint32_t)XC_GetNotifyData(); // 获取传递的参数
            if(Cache == 7) {                      // 传递参数是7时
                s_TickCount[7]++;                 // 计数+1
            }
        }
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务8
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示中断挂起恢复任务,需要手动中断;
 */
void Task_A8(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_TickCount[8] = 0;
    while(1) {
        XC_Suspend(); // 挂起自身;
        s_TickCount[8]++;
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务9
 * @param[in]   phTCB   任务控制块
 * @details
 *  纯延时
 */
void Task_A9(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_TickCount[9] = 0;
    while(1) {
        XC_Delay_ms(10); // 延时
        s_TickCount[9]++;
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief
 * @param[in]   hXCOS       框架句柄
 * @param[in]   IdleTick    空闲的Tick
 * @details     空闲处理
 */
void Idle(XCOS_t* hXCOS, XCuint_t IdleTick)
{
    XCTime_BlockDelay(IdleTick); // 阻塞循环,模拟休眠
}

#endif

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       外部中断0处理函数
 * @details     注意需要软中断 EXTI->SWIER0
 */
void EXTI0_IRQHandler(void)
{
    // 调用HAL库提供的外部中断处理函数
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
#if (_Cnf_RunTask == 1)
    XC_SendNotify(&s_hTCBn[7], (void*)7); // 通知唤醒任务7
#endif
}

/**
 * @brief       外部中断1处理函数
 * @details     注意需要软中断 EXTI->SWIER1
 */
void EXTI1_IRQHandler(void)
{
    // 调用HAL库提供的外部中断处理函数
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
#if (_Cnf_RunTask == 1)
    XC_TaskResume(&s_hTCBn[8]); // 挂起恢复任务8
#endif
}

/**
 * @brief       系统时钟配置
 */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef clkinitstruct = { 0 };
    RCC_OscInitTypeDef oscinitstruct = { 0 };

    /* Configure PLL ------------------------------------------------------*/
    /* PLL configuration: PLLCLK = (HSI / 2) * PLLMUL = (8 / 2) * 16 = 64 MHz */
    /* PREDIV1 configuration: PREDIV1CLK = PLLCLK / HSEPredivValue = 64 / 1 = 64 MHz */
    /* Enable HSI and activate PLL with HSi_DIV2 as source */
    oscinitstruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    oscinitstruct.HSEState            = RCC_HSE_OFF;
    oscinitstruct.LSEState            = RCC_LSE_OFF;
    oscinitstruct.HSIState            = RCC_HSI_ON;
    oscinitstruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscinitstruct.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    oscinitstruct.PLL.PLLState        = RCC_PLL_ON;
    oscinitstruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
    oscinitstruct.PLL.PLLMUL          = RCC_PLL_MUL16;
    if(HAL_RCC_OscConfig(&oscinitstruct) != HAL_OK) {
        /* Initialization Error */
        while(1);
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
      clocks dividers */
    clkinitstruct.ClockType      = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clkinitstruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clkinitstruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
    clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;
    if(HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2) != HAL_OK) {
        /* Initialization Error */
        while(1);
    }
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   主函数
 * @return  int 无
 * @details 主函数
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    /**
     * 外部中断配置
     *  在MDK软件仿真中使用,可以设置"EXTI->SWIER"寄存器0-1位触发中断;
     */
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin  = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // 下降沿触发
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // 上拉
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    GPIO_InitStruct.Pin  = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // 下降沿触发
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // 上拉
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    /** XCOS框架 */
    XCSch_Init(&s_hXCOS0); // 初始化XCOS
#if (_Cnf_RunTask == 1)
    _XC_SysTickCount = 0xFFFFFF00-1;
    XCSch_SetIdleCallback(&s_hXCOS0, Idle); // 空闲处理回调
    // 初始化任务
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[0], Task_A0, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[1], Task_A1, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[2], Task_A2, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[3], Task_A3, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[4], Task_A4, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[5], Task_A5, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[6], Task_A6, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[7], Task_A7, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[8], Task_A8, NULL);
    XC_TaskReg(&s_hXCOS0, &s_hTCBn[9], Task_A9, NULL);
#endif
    XCSch_Run(&s_hXCOS0); // 调度器阻塞运行
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
