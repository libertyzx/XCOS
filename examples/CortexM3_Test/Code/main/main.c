/*=========================================================|
 | 文件名:  main.h
 | 描述:    主文件
 | 版本:    V1.00
 | 日期:    2024/12/06
 | 语言:    C语言
 | 作者:    libertyzx
 | E-mail:  libertyzx@163.com
 +-----------------------------------------------|
 | 开源协议: MIT License
 +-----------------------------------------------|
 +--- 说明
 |  演示XCOS的主文件
 +-----------------------------------------------|
 +--- 版本说明:
 |  V0.01:-2024/12/05
 |      初始;
 *========================================================*/
//=== 头文件
#include "main.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

//XCOS变量
_XC_CreateSysTickCount();       //创建系统Tick
XCOS_t     s_hXCOS1 = {0};      //XCOS句柄
XCTCB_t    s_hTCBn[10] = {0};   //任务控制块

XCSemBin_t s_hSem[3] = {0};     //二值信号量


//其他变量
uint32_t s_TickCount[10] = {0}; //保存计数

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A0
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    基础任务样例
 ************************************************/
void Task_A0(XCTCB_t* phTCB)
{
    __IO uint32_t Param;

    /**代码块0*/

    /**任务块入口
     * 每个任务中必须有;
     * 任务块中代码按顺序执行,使用"XCOS"函数,可以做到类上下文切换;
     * "XC_Enter"前代码块(代码块0),每次任务运行都会执行;
     */
    XC_Enter(phTCB);
//===
    /**获取传递的参数
     * 使用下面函数可以获取任务注册时传递的参数;
     *  XC_GetParam();                  //按(void*)传递
     *  XC_GetParamUint();              //按(uint32_t)传递
     * 若是要获指定任务传递的参数则使用下面函数;
     *  XC_GetTaskParam(任务TCB);       //按(void*)传递
     *  XC_GetTaskParamUint(任务TCB);   //按(uint32_t)传递
     */
    Param = XC_GetParamUint();  //获取传递的参数(这里传递参数是666)

    /**代码块1
     *  可以做初始化等一些单次调用的代码;
     */

    s_TickCount[0] = 0;         //计数清零
    while(1){
        /**代码块2
         *  循环处理的代码;
         */

        /**协程代码块内部调用函数
         * 注:让出控制权,跳出,延时等离开任务的调用,都会跳转到"XC_Leave";
         *  XC_Yield();         //让出控制权;
         *  XC_TaskReset();     //复位本任务,复位后会直接跳出;
         *  XC_DelayTick(100);  //按Tick延时,延时100个Tick;
         *  XC_Delay_ms(10);    //按毫秒延时;
         *  XC_Delay_s(100);    //按秒延时;
         *  XC_Delay_min(60);   //按分钟延时;
         *  XC_Delay_h(1);      //按小时延时;
         *  XC_Delay_day(1);    //按天延时;
         */
        XC_Delay_ms(10);        //延时10ms

        s_TickCount[0]++;       //每10ms累加一次
        /**代码块3*/

        if(s_TickCount[0] > 6000){
            //时间>60000,60s后复位任务
            XC_Reset();         //复位任务
        }
    }
//===
    /**任务块出口
     * 每个任务中必须有;
     * "XC_Leave"后的代码块(代码块4),每次调用完任务都会被调用;
     */
    XC_Leave();

    /**代码块4*/
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A1
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    任务通知示例
 ************************************************/
void Task_A1(XCTCB_t* phTCB)
{
    __IO uint32_t NotifyData;

    XC_Enter(phTCB);
//===
    s_TickCount[1] = 0;
    while(1){
        XC_WaitNotify_ms(1000);             //等待通知(超时1000ms)
        if(XC_GetWakeTimeout() == 1){
            /*超时处理*/
        }
        else{
            /*非超时处理*/
            NotifyData = XC_GetNotifyDataUint();    //获取通知的数据
        }
        s_TickCount[1]++;
        XC_Yield();                         //让出控制权
    }
//===
    XC_Leave();
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A2
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    二值信号示例
 ************************************************/
void Task_A2(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
//===
    s_TickCount[2] = 0;
    while(1){
        XCSem_BinSemTake_ms(&s_hSem[0], 1000);  //获取信号(超时1000ms)
        if(XC_GetWakeTimeout() == 1){
            /*超时处理*/
        }
        else{
            /*非超时处理*/
        }
        s_TickCount[2]++;
        XC_Yield();                         //让出控制权
    }
//===
    XC_Leave();
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A3
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    通知任务A1启动,发送信号启动A2,并挂起A0
 ************************************************/
void Task_A3(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
//===
    s_TickCount[3] = 0;
    while(1){
        XC_Delay_ms(20);    //延时20ms
        XC_SendNotifyUint(&s_hTCBn[1], 777);    //发送通知,通知数据是777
        XC_Delay_ms(70);    //延时70ms
        XCSem_BinSemGive(&s_hSem[0]);            //释放信号
        s_TickCount[3]++;
    }
//===
    XC_Leave();
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A4
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    中断测试信号量
 ************************************************/
void Task_A4(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
//===
    s_TickCount[4] = 0;
    while(1){
        XCSem_BinSemTake(&s_hSem[1], 0);    //获取信号(阻塞等待)
        XC_Delay_ms(10);
        s_TickCount[4]++;
    }
//===
    XC_Leave();
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A5
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    中断测试信号量
 ************************************************/
void Task_A5(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
//===
    s_TickCount[5] = 0;
    while(1){
        XCSem_BinSemTake(&s_hSem[2], 89);   //获取信号
        XC_Delay_ms(11);
        s_TickCount[5]++;
    }
//===
    XC_Leave();
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A6
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    挂起任务
 ************************************************/
void Task_A6(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
//===
    s_TickCount[6] = 0;
    while(1){
        XC_Suspend();       //挂起自身
        s_TickCount[6]++;
    }
//===
    XC_Leave();
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A7
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    挂起恢复
 ************************************************/
void Task_A7(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
//===
    s_TickCount[7] = 0;
    while(1){
        XC_Delay_ms(13);
        XC_TaskResume(&s_hTCBn[6]);     //挂起恢复A6
        s_TickCount[7]++;
    }
//===
    XC_Leave();
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A8
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    无
 ************************************************/
void Task_A8(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
//===
    s_TickCount[8] = 0;
    while(1){
        XC_Delay_ms(14);
        s_TickCount[8]++;
    }
//===
    XC_Leave();
}

/************************************************|
 * 描述:    任务
 * 函数名:  Task_A9
 * 形参[I]: XCTCB_t* phTCB
 * 返回:    void
 * 说明:    无
 ************************************************/
void Task_A9(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);
//===
    s_TickCount[9] = 0;
    while(1){
        XC_Delay_ms(15);
        s_TickCount[9]++;
    }
//===
    XC_Leave();
}

/************************************************ 我是分割线 ************************************************/

/************************************************|
 * 描述:    外部中断处理函数
 * 函数名:  EXTI0_IRQHandler
 * 形参[N]: void
 * 返回:    void
 * 说明:    无
 ************************************************/
void EXTI0_IRQHandler(void) {
    //调用HAL库提供的外部中断处理函数
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    XCSem_BinSemGive(&s_hSem[1]);     //发送信号
}

/************************************************|
 * 描述:    外部中断处理函数
 * 函数名:  EXTI1_IRQHandler
 * 形参[N]: void
 * 返回:    void
 * 说明:    无
 ************************************************/
void EXTI1_IRQHandler(void) {
    //调用HAL库提供的外部中断处理函数
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
    XCSem_BinSemGive(&s_hSem[2]);     //发送信号
}

/************************************************|
 * 描述:    系统时钟配置
 * 函数名:  SystemClock_Config
 * 形参[N]: void
 * 返回:    void
 * 说明:    无
 ************************************************/
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef clkinitstruct = {0};
    RCC_OscInitTypeDef oscinitstruct = {0};

    /* Configure PLL ------------------------------------------------------*/
    /* PLL configuration: PLLCLK = (HSI / 2) * PLLMUL = (8 / 2) * 16 = 64 MHz */
    /* PREDIV1 configuration: PREDIV1CLK = PLLCLK / HSEPredivValue = 64 / 1 = 64 MHz */
    /* Enable HSI and activate PLL with HSi_DIV2 as source */
    oscinitstruct.OscillatorType  = RCC_OSCILLATORTYPE_HSI;
    oscinitstruct.HSEState        = RCC_HSE_OFF;
    oscinitstruct.LSEState        = RCC_LSE_OFF;
    oscinitstruct.HSIState        = RCC_HSI_ON;
    oscinitstruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscinitstruct.HSEPredivValue    = RCC_HSE_PREDIV_DIV1;
    oscinitstruct.PLL.PLLState    = RCC_PLL_ON;
    oscinitstruct.PLL.PLLSource   = RCC_PLLSOURCE_HSI_DIV2;
    oscinitstruct.PLL.PLLMUL      = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&oscinitstruct)!= HAL_OK)
    {
      /* Initialization Error */
      while(1);
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
      clocks dividers */
    clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
    clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2)!= HAL_OK)
    {
      /* Initialization Error */
      while(1);
    }
}

/************************************************ 我是分割线 ************************************************/

/************************************************|
 * 描述:    主函数
 * 函数名:  main
 * 形参[N]: void
 * 返回:    void
 * 说明:    无
 ************************************************/
int main(void)
{
    uint32_t i;

    HAL_Init();
    SystemClock_Config();
    /* 外部中断配置
     *  在MDK软件仿真中使用,可以设置"EXTI->SWIER"寄存器0-1位触发中断;
     */
    {
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;    //下降沿触发
        GPIO_InitStruct.Pull = GPIO_PULLUP;             //上拉
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);

        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;    //下降沿触发
        GPIO_InitStruct.Pull = GPIO_PULLUP;             //上拉
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    }


    XCSch_Init(&s_hXCOS1);                              //初始化XCOS
    for(i=0; i<3; i++){
        XCSem_BinSemInit(&s_hSem[i]);
    }

    //初始化10个任务
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[0], Task_A0, (void*)666);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[1], Task_A1, NULL);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[2], Task_A2, NULL);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[3], Task_A3, NULL);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[4], Task_A4, NULL);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[5], Task_A5, NULL);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[6], Task_A6, NULL);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[7], Task_A7, NULL);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[8], Task_A8, NULL);
    XCSch_TaskReg(&s_hXCOS1, &s_hTCBn[9], Task_A9, NULL);

    XCSch_Run(&s_hXCOS1);                               //调度器运行

}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */


