
/**
 * @file        main.c
 * @brief       主文件
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.00
 * @date        2025/11/21
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

/** 配置开关见"main.h"文件 */

/************************************************ 我是分割线 ************************************************/

// 芯片用
TIM_HandleTypeDef htim2;

// XCOS变量
#if (_Cnf_Examples != 0)
XCOS_t s_hXCOS0; // XCOS句柄
#endif

#if (_Cnf_Examples == 2)
XC_TaskCB_t s_hTCBn[1] = { 0 }; // 任务控制块
#endif

#if (_Cnf_Examples == 3)
XC_TaskCB_t s_hTCBn[10]   = { 0 }; // 任务控制块
uint32_t    s_A0TaskCount = 0;     // A0计数
/**
 * A1和A2运行计数
 * A1触发A2运行;
 */
struct {
    uint32_t A1TaskCount;      // A1任务计数
    uint32_t A1SendErrorCount; // A1发送错误计数
    uint32_t A2TaskCount;      // A2任务计数
    uint32_t A2SucceedCount;   // A2成功计数
    uint32_t A2ErrorCount;     // A2错误计数;
    uint32_t A2TimeoutCount;   // A2超时计数;
} s_A1A2Data = { 0 };

/**
 * A3和A4运行计数
 * A3触发A4运行;
 */
struct {
    uint32_t A3TaskCount;        // A3任务计数
    uint32_t A3SendErrorCount;   // A3发送错误计数
    uint32_t A3SendTimeoutCount; // A3发送超时计数
    uint32_t A4TaskCount;        // A4任务计数
    uint32_t A4SucceedCount;     // A4成功计数
    uint32_t A4ErrorCount;       // A4错误计数;
    uint32_t A4TimeoutCount;     // A4超时计数;
} s_A3A4Data = { 0 };

uint32_t s_A5TaskCount = 0; // A5计数
uint32_t s_A6TaskCount = 0; // A6计数

struct {
    uint32_t IntSendCount; // 中断发送计数
    uint32_t TaskCount;    // 任务运行计数
} s_A7Data = { 0 };

struct {
    uint32_t TaskCount;           // 任务运行计数
    uint32_t SucceedCount;        // 成功计数
    uint32_t TimeoutCount;        // 超时计数
    uint32_t IntNotifySendCount;  // 中断通知发送计数
    uint32_t IntSuspendSendCount; // 中断挂起发送计数
    uint32_t IntResumeSendCount;  // 中断挂起恢复发送计数
} s_A8Data = { 0 };

struct {
    uint32_t IntTaskCount;   // 任务计数
    uint32_t A9TaskCount;    // 任务计数
    uint32_t A9SucceedCount; // 成功计数
    uint32_t A9ErrorCount;   // 错误计数;
    uint32_t A9TimeoutCount; // 超时计数;
} s_A9Data = { 0 };

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
#if (_Cnf_Examples >= 2)
/**
 * @brief       任务0
 * @param[in]   phTCB   任务控制块
 * @details
 *  基础框架,演示延时和让出控制;
 */
void Task_A0(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB); // 协程任务块开始标志
    /** --- */
    while(1) {
        XC_DelayMs(10); // 延时
#if (_Cnf_Examples >= 3)
        s_A0TaskCount++; // 计数,延时了几次
        XC_Yield();      // 让出控制
#endif
    }
    /** --- */
    XC_Leave(); // 协程任务块结束标志
}
#endif

/************************************************ 我是分割线 ************************************************/

#if (_Cnf_Examples >= 3)
/**
 * @brief       任务1
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试通知1-发送;
 *  A1和A2是一组测试,A1唤醒A2;
 */
void Task_A1(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    while(1) {
        XC_DelayMs(15);                           // 延时
        XCTask_NotifySend(&s_hTCBn[2], (void*)2); // 唤醒任务2,传递参数2,正确数据
        s_A1A2Data.A1TaskCount++;                 // 任务计数

        XC_DelayMs(15);                           // 延时
        XCTask_NotifySend(&s_hTCBn[2], (void*)3); // 唤醒任务2,传递参数3,错误数据
        s_A1A2Data.A1TaskCount++;                 // 任务计数
        s_A1A2Data.A1SendErrorCount++;            // 发送错误数据数据
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务2
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试通知1-等待通知;
 *  A1唤醒A2,A2在每次结束后参数值为:
 *  A1TaskCount == A2TaskCount  //任务计数
 *  A2SucceedCount == A2ErrorCount == A1SendErrorCount //成功&错误&发送错误计数
 *  A2TimeoutCount == 0         //超时计数
 */
void Task_A2(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    while(1) {
        XC_WaitNotifyMs(20);                        // 等待通知,超时20ms
        if(XC_GetNotifyWakeState() == 0) {          // 未超时,通知到达
            if((uint32_t)XC_GetNotifyData() == 2) { // 获取传递的参数
                s_A1A2Data.A2SucceedCount++;        // 正确计数
            }
            else {                         // 传递参数错误
                s_A1A2Data.A2ErrorCount++; // 错误计数
            }
        }
        else {                           // 超时
            s_A1A2Data.A2TimeoutCount++; // 超时计数
        }
        s_A1A2Data.A2TaskCount++; // 总计数
    }
    /** --- */
    XC_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       任务3
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试通知2-发送;
 *  A3和A4是一组测试,A3唤醒A4;
 */
void Task_A3(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    while(1) {
        XC_DelayMs(50);                           // 延时
        XCTask_NotifySend(&s_hTCBn[4], (void*)4); // 唤醒任务4,传递参数4,正确数据
        s_A3A4Data.A3TaskCount++;                 // 任务计数

        XC_DelayMs(50);                           // 延时
        XCTask_NotifySend(&s_hTCBn[4], (void*)5); // 唤醒任务4,传递参数5,错误数据
        s_A3A4Data.A3TaskCount++;                 // 任务计数
        s_A3A4Data.A3SendErrorCount++;            // 发送错误数据计数

        XC_DelayMs(65);                           // 延时
        XCTask_NotifySend(&s_hTCBn[4], (void*)2); // 唤醒任务4,传递参数6,正确数据
        s_A3A4Data.A3TaskCount++;                 // 任务计数
        s_A3A4Data.A3SendTimeoutCount++;          // 发送超时数据计数
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务4
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试通知2-等待通知;
 *  A3唤醒A4,A4在每次结束后参数值为:
 *  A3TaskCount == A4TaskCount              //总计数相同
 *  A3SendErrorCount == A4ErrorCount        //错误计数相同
 *  A3SendTimeoutCount == A4TimeoutCount    //超时计数相同
 *  A4TaskCount == (A4SucceedCount + A4ErrorCount + A4TimeoutCount) //成功计数
 */
void Task_A4(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    while(1) {
        XC_WaitNotifyMs(60);                        // 等待通知,超时60ms
        if(XC_GetNotifyWakeState() == 0) {          // 未超时,通知到达
            if((uint32_t)XC_GetNotifyData() == 4) { // 获取传递的参数
                s_A3A4Data.A4SucceedCount++;        // 正确计数
            }
            else {                         // 传递参数错误
                s_A3A4Data.A4ErrorCount++; // 错误计数
            }
        }
        else {                           // 超时
            s_A3A4Data.A4TimeoutCount++; // 超时计数
            XC_DelayMs(10);
        }
        s_A3A4Data.A4TaskCount++; // 总计数
    }
    /** --- */
    XC_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       任务5
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示复位;
 *  复位后任务重新运行,计数会+1;
 *  "s_A5TaskCount"计数会按10s一次累加;
 */
void Task_A5(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_A5TaskCount++; // 复位后这里会++
    while(1) {
        XC_DelayMs(10); // 延时
        XC_Reset();     // 每10s复位一次
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务6
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示移除任务;
 *  计数到达指定值(100*10ms=1s),移除自己;
 *  s_A6TaskCount == 10,不会改变;
 */
void Task_A6(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    s_A6TaskCount = 0;
    while(1) {
        XC_DelayMs(100); // 延时
        s_A6TaskCount++;
        if(s_A6TaskCount >= 10) {
            XC_Remove(); // 移除自己
        }
    }
    /** --- */
    XC_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       任务7
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示挂起-手动中断;
 *  运行后立刻挂起;
 *  挂起恢复操作在手动外部中断中(外部中断0);
 *  操作完成后值数据应该为:
 *  IntSendCount == TaskCount
 */
void Task_A7(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    while(1) {
        XC_Suspend();         // 挂起自身
        s_A7Data.TaskCount++; // 挂起恢复后计数+1
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务8
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示通知-手动中断
 *  运行后立刻阻塞等待通知;
 *  发送通知操作在手动外部中断中(外部中断1);
 *  操作完成后值数据应该为:
 *  IntSendCount == TaskCount
 */
void Task_A8(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    while(1) {
        XC_WaitNotifyMs(0);                         // 阻塞等待通知
        if(XC_GetNotifyWakeState() == 0) {          // 是被通知唤醒
            if((uint32_t)XC_GetNotifyData() == 8) { // 唤醒通知参数是8
                s_A8Data.SucceedCount++;            // 成功计数
            }
        }
        else {
            s_A8Data.TimeoutCount++;
        }
        s_A8Data.TaskCount++; // 总计数
    }
    /** --- */
    XC_Leave();
}

/**
 * @brief       任务9
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示通知-中断定时器唤醒
 */
void Task_A9(XC_TaskHandle_t phTCB)
{
    XC_Enter(phTCB);
    /** --- */
    while(1) {
        XC_WaitNotifyMs(33);                        // 等待通知,超时33ms
        if(XC_GetNotifyWakeState() == 0) {          // 未超时,通知到达
            if((uint32_t)XC_GetNotifyData() == 9) { // 获取传递的参数
                s_A9Data.A9SucceedCount++;          // 正确计数
            }
            else {                       // 传递参数错误
                s_A9Data.A9ErrorCount++; // 错误计数
            }
        }
        else {                         // 超时
            s_A9Data.A9TimeoutCount++; // 超时计数
        }
        s_A9Data.A9TaskCount++; // 总计数
    }
    /** --- */
    XC_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       空闲处理
 * @param[in]   phXCOS      框架句柄
 * @param[in]   IdleTick    空闲的Tick
 * @details     空闲处理
 */
void Idle(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)
{
    // XCTime_BlockDelay(IdleTick); // 阻塞循环,模拟休眠
    __WFI(); // 休眠
}

/************************************************ 我是分割线 ************************************************/
#endif

#if (_Cnf_Examples > 0)
/**
 * @brief   框架
 * @details 框架处理
 */
void XCOS(void)
{
    XCSch_Init(&s_hXCOS0); // 初始化XCOS

#if (_Cnf_Examples >= 3)
    XCTime_TickSet(0xFFFFF000 - 1);         // 为了测试溢出增加
    XCSch_SetIdleCallback(&s_hXCOS0, Idle); // 空闲处理回调
#endif
    // 初始化任务
#if (_Cnf_Examples >= 2)
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[0], Task_A0, NULL);
#endif
#if (_Cnf_Examples >= 3)
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[1], Task_A1, NULL);
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[2], Task_A2, NULL);
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[3], Task_A3, NULL);
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[4], Task_A4, NULL);
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[5], Task_A5, NULL);
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[6], Task_A6, NULL);
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[7], Task_A7, NULL);
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[8], Task_A8, NULL);
    XCTask_Reg(&s_hXCOS0, &s_hTCBn[9], Task_A9, NULL);
#endif
    XCSch_Start(&s_hXCOS0); // 调度器启动
}
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/**
 * @brief       外部中断处理
 * @param[in]   GPIO_Pin
 * @details
 *  注意需要软中断 EXTI->SWIER*
 *  - 0: 挂起恢复A7
 *  - 1: 通知唤醒A8
 *  - 2: 挂起A8
 *  - 3: 挂起恢复A8
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

    switch(GPIO_Pin) {
        case GPIO_PIN_0:
#if (_Cnf_Examples >= 3)
            XCTask_Resume(&s_hTCBn[7]); // 恢复A7
            s_A7Data.IntSendCount++;
#endif
            break;
        case GPIO_PIN_1:
#if (_Cnf_Examples >= 3)
            XCTask_NotifySend(&s_hTCBn[8], (void*)8); // 唤醒任务8,传递参数8
            s_A8Data.IntNotifySendCount++;
#endif
            break;
        case GPIO_PIN_2:
#if (_Cnf_Examples >= 3)
            XCTask_Suspend(&s_hTCBn[8]); // 挂起A8
            s_A8Data.IntSuspendSendCount++;
#endif
            break;
        case GPIO_PIN_3:
#if (_Cnf_Examples >= 3)
            XCTask_Resume(&s_hTCBn[8]); // 恢复A8
            s_A8Data.IntResumeSendCount++;
#endif
            break;
        default:
            break;
    }
}

/**
 * @brief       定时器中断服务处理
 * @param[in]   htim
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    uint32_t NextIntTime = 500; // 下次中断时间(ms)

    if(htim->Instance == TIM2) {
#if (_Cnf_Examples >= 3)
        XCTask_NotifySend(&s_hTCBn[9], (void*)9); // 唤醒任务8,传递参数8
        s_A9Data.IntTaskCount++;
        NextIntTime = 32;
#endif
        // 更新
        __HAL_TIM_SET_AUTORELOAD(&htim2, (NextIntTime * 10) - 1);
    }
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/**
 * @brief   系统时钟配置
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

/**
 * @brief   外部中断配置
 */
static void IRQ_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /**
     * 外部中断配置
     *  在MDK软件仿真中使用,可以设置"EXTI->SWIER"寄存器0-n位触发中断;
     */

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

    GPIO_InitStruct.Pin  = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // 下降沿触发
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // 上拉
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);

    GPIO_InitStruct.Pin  = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // 下降沿触发
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // 上拉
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

/**
 * @brief   时钟配置
 */
static void Timer_Config(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();                       // 启用TIM2时钟
    htim2.Instance           = TIM2;                   // 设置定时器实例
    htim2.Init.Period        = 32 * 10 - 1;            // 设置自动重载寄存器的值，周期为1000-1，因为计数是从0开始的
    htim2.Init.Prescaler     = 6400 - 1;               // 设置预分频器的值，根据系统时钟设置
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; // 时钟分割因子
    htim2.Init.CounterMode   = TIM_COUNTERMODE_UP;     // 向上计数模式
    HAL_TIM_Base_Init(&htim2);                         // 初始化定时器
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim2); // 启动定时器并允许中断
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   外部中断0处理函数
 */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

/**
 * @brief   外部中断1处理函数

 */
void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

/**
 * @brief   外部中断2处理函数
 */
void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

/**
 * @brief   外部中断3处理函数
 */
void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

/**
 * @brief   TIM2中断服务
 */
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2); // 调用HAL库的中断处理函数
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
    Timer_Config();
    IRQ_Config();
#if (_Cnf_Examples > 0)
    XCOS(); // XCOS框架
#endif
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
