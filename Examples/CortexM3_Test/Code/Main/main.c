
/**
 * @file        main.c
 * @brief       主文件
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/20
 * **********************************************
 * @copyright   Copyright (c) 2023 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     演示XCOS的主文件
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 头文件
#include "main.h"
#include <stdint.h>
#include <stdlib.h>

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/** 配置开关见"main.h"文件 */

/************************************************ 我是分割线 ************************************************/

// 芯片用
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

#if (_Cnf_Examples > 0)
// XCOS变量
XCOS_t s_hXCOS0; // XCOS句柄

#if (_Cnf_Examples > 1)
// 任务
XC_TaskCB_t s_hTCBn[12] = { 0 }; // 任务控制块(12个:A0~A11)

uint32_t s_A0TaskCount = 0; // A0计数
/**
 * A1和A2运行计数
 * 测试A1发送通知给A2;
 */
struct {
    // 提前通知
    uint32_t A1_PreNotifyCount;          // A1 提前通知计数
    uint32_t A2_PreNotifyRxTimeoutCouut; // A2 提前通知接收超时计数
    uint32_t A2_PreNotifyRxCount;        // A2 提前通知接收成功计数
    // 正常通知
    uint32_t A1_NotifyCount;             // A1 通知计数
    uint32_t A2_NotifyRxCount;           // A2 通知接收成功计数
    uint32_t A2_NotifyRxErrorCount;      // A2 通知接收错误计数(非通知唤醒)
    uint32_t A2_NotifyRxParamErrorCount; // A2 通知接收参数错误计数
} s_A1A2Data = { 0 };

/**
 * A3和A4运行计数
 * 测试A3挂起和恢复A4;
 */
struct {
    uint32_t A3_SuspendCount; // A3挂起A4计数;
    uint32_t A3_ResumeCount;  // A3恢复A4计数;
    uint32_t A4_ResumeCount;  // A4唤醒计数
} s_A3A4Data = { 0 };

uint32_t s_A5_ResetCount  = 0; // A5复位计数
uint32_t s_A6_RemoveCount = 0; // A6移除计数

/**
 * 测试中断通知唤醒
 */
struct {
    uint32_t Int_NotifyCount;         // 中断发送通知计数
    uint32_t A7_NotifyRxCount;        // 中断接收唤醒成功计数
    uint32_t A7_NotifyRxTimeoutCount; // 中断接收唤醒超时计数
} s_A7Data = { 0 };

/**
 * 测试中断通知唤醒
 */
struct {
    uint32_t Int1_NotifyCount; // 中断唤醒1
    uint32_t Int2_NotifyCount; // 中断唤醒2
    uint32_t Int3_NotifyCount; // 中断唤醒3
    uint32_t Int4_NotifyCount; // 中断唤醒4

    uint32_t A8_NotifyRxTimeoutCount;    // 中断接收唤醒超时计数
    uint32_t A8_NotifyRx1Count;          // 中断接收参数1计数
    uint32_t A8_NotifyRx2Count;          // 中断接收参数2计数
    uint32_t A8_NotifyRx3Count;          // 中断接收参数3计数
    uint32_t A8_NotifyRx4Count;          // 中断接收参数4计数
    uint32_t A8_NotifyRxParamErrorCount; // 中断接收参数错误计数
} s_A8Data = { 0 };

/**
 * 测试手动中断通知唤醒
 */
struct {
    uint32_t Int1_NotifyCount; // 中断唤醒1
    uint32_t Int2_NotifyCount; // 中断唤醒2
    uint32_t Int3_NotifyCount; // 中断唤醒3
    uint32_t Int4_NotifyCount; // 中断唤醒4

    uint32_t A9_NotifyRxParamErrorCount; // 中断接收未知计数
    uint32_t A9_NotifyRxTimeoutCount;    // 中断接收唤醒超时计数
    uint32_t A9_NotifyRx1Count;          // 中断接收参数1计数
    uint32_t A9_NotifyRx2Count;          // 中断接收参数2计数
    uint32_t A9_NotifyRx3Count;          // 中断接收参数3计数
    uint32_t A9_NotifyRx4Count;          // 中断接收参数4计数
} s_A9Data = { 0 };

#endif
#endif
/************************************************ 我是分割线 ************************************************/
/** 函数声明 */

uint32_t GetRand(void); // 获取随机数

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
#if (_Cnf_Examples > 1)
/**
 * @brief       任务0
 * @param[in]   phTCB   任务控制块
 * @details     基础框架,演示延时;
 */
void Task_A0(XC_TaskHandle_t phTCB)
{
    uint32_t Delay;

    XC_Cor_Enter(phTCB); // 协程任务块开始标志
    /** --- */
    while(1) {
        Delay = GetRand();     // 得到随机数
        XC_Cor_DelayMs(Delay); // 延时
        s_A0TaskCount++;       // 计数,延时了几次
    }
    /** --- */
    XC_Cor_Leave(); // 协程任务块结束标志
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       任务1
 * @param[in]   phTCB   任务控制块
 * @details     测试A1发送通知给A2-发送;
 */
void Task_A1(XC_TaskHandle_t phTCB)
{
    uint32_t Delay;

    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        /**这里立刻发送通知,演示通知比等待通知先到 */
        XC_Task_SendNotify(&s_hTCBn[2], (void*)1); // 唤醒任务2,传递参数1
        s_A1A2Data.A1_PreNotifyCount++;            // 提前通知计数
        Delay = GetRand();                         // 得到随机数
        XC_Cor_DelayMs(Delay);                     // 延时(大于A2提前通知处理:延时+超时)
        /**这里正常通知 */
        XC_Task_SendNotify(&s_hTCBn[2], (void*)2); // 唤醒任务2,传递参数2
        s_A1A2Data.A1_NotifyCount++;               // A1 通知计数
        XC_Cor_Yield();                            // 让出控制
    }
    /** --- */
    XC_Cor_Leave();
}

/**
 * @brief       任务2
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试A1发送通知给A2-等待通知;
 *  测试结果:
 *  - 提前通知
 *      A1_PreNotifyCount == A2_PreNotifyRxCount    预先通知成功
 *      A2_PreNotifyRxTimeoutCouut > 0              说明预先通知有错误,有bug
 *  - 正常通知
 *      A1_NotifyCount == A2_NotifyRxCount          正常通知成功
 *      A2_NotifyRxParamErrorCount > 0              收到参数错误,有bug
 *      A2_NotifyRxErrorCount > 0                   被非通知唤醒,有bug
 */
void Task_A2(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        /** 提前通知演示 */
        XC_Cor_DelayMs(7);                           // 延时
        XC_Cor_WaitNotifyMs(10);                     // 等待通知
        if(XC_Cor_IsNotifyTimeout()) {               // 超时
            s_A1A2Data.A2_PreNotifyRxTimeoutCouut++; // A2 接收提前通知超时
        }
        else {                                // 接收成功处理
            s_A1A2Data.A2_PreNotifyRxCount++; // A2 提前通知接收成功计数
        }
        /** 正常通知演示 */
        XC_Cor_WaitNotifyMs(0);                 // 等待通知,阻塞等
        if(XC_Cor_IsNotifyTimeout()) {          // 非通知唤醒
            s_A1A2Data.A2_NotifyRxErrorCount++; // A2 通知接收错误计数(非通知唤醒)
        }
        else {
            if((uint32_t)XC_Cor_GetNotifyData() == 2) { // A1 发送的是2
                s_A1A2Data.A2_NotifyRxCount++;          // A2 通知接收成功计数
            }
            else {
                s_A1A2Data.A2_NotifyRxParamErrorCount++; // A2 通知接收参数错误计数(参数错误)
            }
        }
    }
    /** --- */
    XC_Cor_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       任务3
 * @param[in]   phTCB   任务控制块
 * @details     测试A3挂起和恢复A4-触发;
 */
void Task_A3(XC_TaskHandle_t phTCB)
{
    uint32_t Delay;

    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        Delay = GetRand();            // 得到随机数
        XC_Cor_DelayMs(Delay);        // 延时
        XC_Task_Suspend(&s_hTCBn[4]); // 挂起任务4
        s_A3A4Data.A3_SuspendCount++; // A3挂起A4计数;

        Delay = GetRand();           // 得到随机数
        XC_Cor_DelayMs(Delay);       // 延时
        XC_Task_Resume(&s_hTCBn[4]); // 恢复任务4
        s_A3A4Data.A3_ResumeCount++; // A3恢复A4计数;
    }
    /** --- */
    XC_Cor_Leave();
}

/**
 * @brief       任务4
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试A3挂起和恢复A4-处理;
 *  测试结果:
 *  "A3_SuspendCount","A3_ResumeCount","A4_ResumeCount" 应相同,若不同则有bug;
 */
void Task_A4(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        XC_Cor_DelayMs(0);           // 延时(阻塞)
        s_A3A4Data.A4_ResumeCount++; // A4唤醒计数
    }
    /** --- */
    XC_Cor_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       任务5
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示复位;
 *  随机时间复位,复位后任务重新运行,计数会+1;
 */
void Task_A5(XC_TaskHandle_t phTCB)
{
    uint32_t Delay;

    XC_Cor_Enter(phTCB);
    /** --- */
    s_A5_ResetCount++; // A5复位计数
    while(1) {
        Delay = GetRand();     // 得到随机数
        XC_Cor_DelayMs(Delay); // 延时
        XC_Cor_Reset();        // 每10s复位一次
    }
    /** --- */
    XC_Cor_Leave();
}

/**
 * @brief       任务6
 * @param[in]   phTCB   任务控制块
 * @details
 *  演示移除任务;
 *  随机时间移除自己,计数应固定为10;
 */
void Task_A6(XC_TaskHandle_t phTCB)
{
    uint32_t Delay;

    XC_Cor_Enter(phTCB);
    /** --- */
    s_A6_RemoveCount = 0;
    while(1) {
        Delay = GetRand();     // 得到随机数
        XC_Cor_DelayMs(Delay); // 延时
        s_A6_RemoveCount++;
        if(s_A6_RemoveCount >= 10) {
            XC_Cor_Remove(); // 移除自己
        }
    }
    /** --- */
    XC_Cor_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       任务7
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试中断通知唤醒
 *  测试结果:中断发送时间和超时等待都是随机的,这里计数也为随机;
 */
void Task_A7(XC_TaskHandle_t phTCB)
{
    uint32_t Delay;

    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        Delay = GetRand() / 10;                 // 得到随机数
        XC_Cor_WaitNotifyMs(Delay);             // 等待通知,随机超时3-30
        if(XC_Cor_IsNotifyTimeout()) {          // 超时
            s_A7Data.A7_NotifyRxTimeoutCount++; // 中断接收唤醒超时计数
        }
        else {                           // 接收成功处理
            s_A7Data.A7_NotifyRxCount++; // 中断接收唤醒成功计数
        }
    }
    /** --- */
    XC_Cor_Leave();
}

/**
 * @brief       任务8
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试中断通知唤醒
 *  结果:
 *  - Int1_NotifyCount == A8_NotifyRx1Count 4个计数应相同,不同则有bug;
 *  - A8_NotifyRxTimeoutCount               应为0,否则有bug;
 *  - A8_NotifyRxParamErrorCount            应为0,否则有bug;
 */
void Task_A8(XC_TaskHandle_t phTCB)
{
    uint32_t Delay;

    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        Delay = GetRand();                      // 得到随机数
        XC_Cor_WaitNotifyMs(1 + Delay);         // 定时器中断时间是3-30,这里超时大于30,必定能收到通知
        if(XC_Cor_IsNotifyTimeout()) {          // 超时
            s_A8Data.A8_NotifyRxTimeoutCount++; // 中断接收唤醒超时计数
        }
        else { // 成功
            switch((uint32_t)XC_Cor_GetNotifyData()) {
                case 1:
                    s_A8Data.A8_NotifyRx1Count++; // 中断接收参数1计数
                    break;
                case 2:
                    s_A8Data.A8_NotifyRx2Count++; // 中断接收参数2计数
                    break;
                case 3:
                    s_A8Data.A8_NotifyRx3Count++; // 中断接收参数3计数
                    break;
                case 4:
                    s_A8Data.A8_NotifyRx4Count++; // 中断接收参数4计数
                    break;
                default:
                    s_A8Data.A8_NotifyRxParamErrorCount++; // 中断接收未知计数
                    break;
            }
        }
    }
    /** --- */
    XC_Cor_Leave();
}

/**
 * @brief       任务9
 * @param[in]   phTCB   任务控制块
 * @details
 *  测试中断通知唤醒
 *  这里需要手动唤醒;
 *  若无手动,"A9_NotifyRxTimeoutCount"一直累加;
 */
void Task_A9(XC_TaskHandle_t phTCB)
{
    uint32_t Delay;

    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        Delay = GetRand();                      // 得到随机数
        XC_Cor_WaitNotifyMs(300 + Delay);       // 等待通知
        if(XC_Cor_IsNotifyTimeout()) {          // 超时
            s_A9Data.A9_NotifyRxTimeoutCount++; // 中断接收唤醒超时计数
        }
        else { // 成功
            switch((uint32_t)XC_Cor_GetNotifyData()) {
                case 1:
                    s_A9Data.A9_NotifyRx1Count++; // 中断接收参数1计数
                    break;
                case 2:
                    s_A9Data.A9_NotifyRx2Count++; // 中断接收参数2计数
                    break;
                case 3:
                    s_A9Data.A9_NotifyRx3Count++; // 中断接收参数3计数
                    break;
                case 4:
                    s_A9Data.A9_NotifyRx4Count++; // 中断接收参数4计数
                    break;
                default:
                    s_A9Data.A9_NotifyRxParamErrorCount++; // 中断接收未知计数
                    break;
            }
        }
    }
    /** --- */
    XC_Cor_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * 协程嵌套测试(V2.1.0)
 * 顶层任务 Task_A10 用"XC_Task_RegExt"注册(带帧栈),循环调用"XC_Cor_Call(Sub_A10)";
 * 子协程 Sub_A10 延时/让出挂起,完成后"XC_Cor_Leave"置DONE,调度器"同轮切父"弹帧返回父层;
 * 预期结果:
 * - CallCount(父层Call次数) == SubRunCount(子层运行次数) == ParentAfterCount(父层返回计数)
 *   (正常运行下三者递增一致;若不一致则有bug)
 */
XC_CorFrame_t s_hA10Stack[3] = { 0 }; // A10帧栈(容量3,顶层不占栈)

struct {
    uint32_t CallCount;        // 父层 Call 次数
    uint32_t SubRunCount;      // 子协程进入次数
    uint32_t ParentAfterCount; // 父层 Call 返回后执行次数(同轮切父验证)
} s_A10Data = { 0 };

/**
 * @brief       子协程A10
 * @param[in]   phTCB   任务控制块
 * @details     子层:延时挂起,让出,完成后返回父层;
 */
void Sub_A10(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    /** --- */
    s_A10Data.SubRunCount++; // 子层进入计数(每次 Call 进入执行一次)
    XC_Cor_DelayMs(5);       // 子层延时(挂起)
    XC_Cor_Yield();          // 子层让出
    XC_Cor_Leave();          // 子层完成 -> 调度器同轮切父弹帧返回父层
    /** --- */
}

/**
 * @brief       任务10(协程嵌套-顶层)
 * @param[in]   phTCB   任务控制块
 * @details
 *  用"XC_Task_RegExt"注册(带帧栈);
 *  循环:延时后 Call 子协程,子层完成后"同轮切父"回到此处;
 */
void Task_A10(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        XC_Cor_DelayMs(10);           // 顶层延时
        XC_Cor_Call(Sub_A10);         // 登记子协程(下轮进入子层)
        s_A10Data.ParentAfterCount++; // 子层完成同轮切父后执行
        s_A10Data.CallCount++;        // Call 次数
    }
    /** --- */
    XC_Cor_Leave();
}

/************************************************ 我是分割线 ************************************************/

/**
 * 分步注册 + 帧栈设置测试(V2.1.0)
 * 用"XC_Task_SetEntry" + "XC_Task_SetCorStack" + "XC_Task_Add" 分步注册任务;
 * 验证 SetCorStack 配置的帧栈可用(子协程可正常调用/返回);
 */
XC_CorFrame_t s_hA11Stack[3] = { 0 }; // A11帧栈(容量3,顶层不占栈)

struct {
    uint32_t RunCount;     // 任务运行计数
    uint32_t SubCallCount; // 子协程调用计数
    uint32_t SubRunCount;  // 子协程运行计数
} s_A11Data = { 0 };

/**
 * @brief       子协程A11
 * @param[in]   phTCB   任务控制块
 */
void Sub_A11(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    /** --- */
    s_A11Data.SubRunCount++; // 子层运行计数(每次 Call 进入执行一次)
    XC_Cor_DelayMs(6);       // 子层延时
    XC_Cor_Leave();          // 子层完成,同轮切父返回
    /** --- */
}

/**
 * @brief       任务11(分步注册+帧栈设置)
 * @param[in]   phTCB   任务控制块
 * @details
 *  用"XC_Task_SetEntry" + "XC_Task_SetCorStack" + "XC_Task_Add" 注册;
 *  循环 Call 子协程;
 */
void Task_A11(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    /** --- */
    while(1) {
        s_A11Data.RunCount++;     // 任务运行计数
        XC_Cor_DelayMs(8);        // 延时
        XC_Cor_Call(Sub_A11);     // 调用子协程
        s_A11Data.SubCallCount++; // 子协程返回计数
    }
    /** --- */
    XC_Cor_Leave();
}

/************************************************ 我是分割线 ************************************************/

#if ((XC_CFG_DEBUG_CHECK != 0) || (XC_CFG_TASK_STATS != 0))
/**
 * @brief       诊断演示(可选能力: 由"XC_Config.h"的诊断开关控制)
 * @details
 *  本段演示两类**可选诊断能力**, 默认全关 ⇒ **本段不编译(0 代码 / 0 开销)**:
 *  - `XC_CFG_DEBUG_CHECK`: 运行期不变量自检 `XC_Diag_CheckInvariants`(只读检查, 返回"首个失败项");
 *  - `XC_CFG_TASK_STATS` : 每任务运行统计 `XC_Diag_GetRunCnt` / `XC_Diag_GetMaxRunTick`(调度槽耗时);
 *  调用时机: **空闲回调**里每若干轮抽查一次(见 "Docs/XC_Config.md" 的推荐用法);
 *  观察方式: 调试器查看下面的观测变量(实际项目可改为点灯/写日志/上报);
 *  详细说明: "Docs/XCOS.md"「运行期自检」「运行统计」与 "Docs/XC_Config.md" 对应开关节;
 */
volatile XC_ChkResult_t s_DiagChkLast    = XC_CHK_OK; // [观测] 最近一次自检结果(XC_CHK_OK = 全部一致)
volatile uint32_t       s_DiagChkTimes   = 0;         // [观测] 自检触发次数
volatile uint32_t       s_DiagRunCnt     = 0;         // [观测] A0 运行次数(统计档)
volatile XC_Tick_t      s_DiagMaxRunTick = 0;         // [观测] A0 最长一次调度槽耗时(Tick)

/**
 * @brief   诊断演示处理
 * @details 自检(开档时, 每 256 次空闲抽查一次) + 刷新运行统计快照(开档时); 两开关全关时本函数不存在
 */
static void DiagDemo(void)
{
#if(XC_CFG_DEBUG_CHECK != 0)
    /* 运行期自检: 盘点四张任务表与状态标签; 失败返回首个失败项编号(1 链表 / 2 状态↔表 / 3 重复挂表 / 4 计数 / 5 嵌套 / 6 归属) */
    if((s_DiagChkTimes & 0xFFU) == 0U) {
        s_DiagChkLast = XC_Diag_CheckInvariants(&s_hXCOS0); // 抽查(全量盘点 O(n²), 故降频)
    }
    s_DiagChkTimes++;
#endif
#if(XC_CFG_TASK_STATS != 0)
    /* 运行统计: A0(基础延时任务) 的运行次数与最长一次耗时(单位 Tick; 默认 1ms/tick ⇒ 短于 1ms 记 0) */
    s_DiagRunCnt     = XC_Diag_GetRunCnt(&s_hTCBn[0]);
    s_DiagMaxRunTick = XC_Diag_GetMaxRunTick(&s_hTCBn[0]);
#endif
}
#endif

/************************************************ 我是分割线 ************************************************/

/**
 * @brief       空闲处理
 * @param[in]   phXCOS      框架句柄
 * @param[in]   IdleTick    空闲的Tick
 * @details
 *  空闲处理;
 *  另: 演示"运行期守护"用法 —— 在空闲回调里抽查诊断(见 "DiagDemo", 诊断开关全关时不编译);
 */
void Idle(XC_OSHandle_t phXCOS, XC_Tick_t IdleTick)
{
#if ((XC_CFG_DEBUG_CHECK != 0) || (XC_CFG_TASK_STATS != 0))
    DiagDemo(); // 诊断演示(全关时连本调用都不编译 ⇒ 0 开销)
#endif
    __WFI(); // 休眠
}

#endif
/************************************************ 我是分割线 ************************************************/

#if (_Cnf_Examples > 0)
/**
 * @brief   框架
 * @details 框架处理
 */
void XCOS(void)
{
    XC_Sch_Init(&s_hXCOS0); // 初始化XCOS
#if (_Cnf_Examples > 1)
    XC_Sch_SetIdleCallback(&s_hXCOS0, Idle); // 空闲处理回调
    // 初始化任务
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[0], Task_A0, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[1], Task_A1, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[2], Task_A2, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[3], Task_A3, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[4], Task_A4, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[5], Task_A5, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[6], Task_A6, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[7], Task_A7, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[8], Task_A8, NULL);
    XC_Task_Reg(&s_hXCOS0, &s_hTCBn[9], Task_A9, NULL);
    /* V2.1.0: 协程嵌套任务 - RegExt 注册(带帧栈) */
    XC_Task_RegExt(&s_hXCOS0, &s_hTCBn[10], Task_A10, NULL, s_hA10Stack, 3U);
    /* V2.1.0: 分步注册 + SetCorStack(先设入口/帧栈,再添加) */
    XC_Task_SetEntry(&s_hTCBn[11], Task_A11, NULL);
    XC_Task_SetCorStack(&s_hTCBn[11], s_hA11Stack, 3U);
    XC_Task_Add(&s_hXCOS0, &s_hTCBn[11]);
#endif
    XC_Sch_Start(&s_hXCOS0); // 调度器启动
}
#endif

/**
 * @brief       获取随机数
 * @return      uint32_t 一个30-300的随机数
 * @details     说明
 */
uint32_t GetRand(void)
{
    return (30 + rand() % 271); // 随机:30-300
}

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
 *  唤醒任务9;
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#if (_Cnf_Examples > 1)
    switch(GPIO_Pin) {
        case GPIO_PIN_0:
            XC_Task_SendNotify(&s_hTCBn[9], (void*)1); // 唤醒任务9
            s_A9Data.Int1_NotifyCount++;               // 中断唤醒1
            break;
        case GPIO_PIN_1:
            XC_Task_SendNotify(&s_hTCBn[9], (void*)2); // 唤醒任务9
            s_A9Data.Int2_NotifyCount++;               // 中断唤醒2
            break;
        case GPIO_PIN_2:
            XC_Task_SendNotify(&s_hTCBn[9], (void*)3); // 唤醒任务9
            s_A9Data.Int3_NotifyCount++;               // 中断唤醒3
            break;
        case GPIO_PIN_3:
            XC_Task_SendNotify(&s_hTCBn[9], (void*)4); // 唤醒任务9
            s_A9Data.Int4_NotifyCount++;               // 中断唤醒4
            break;
        default:
            break;
    }
#endif
}

/**
 * @brief       定时器中断服务处理
 * @param[in]   htim
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    uint32_t        Time; // 下次中断时间(ms)
    static uint32_t Param = 1;

    if(htim->Instance == TIM2) {
#if (_Cnf_Examples > 1)
        XC_Task_SendNotify(&s_hTCBn[7], NULL); // 唤醒任务8,传递参数8
        s_A7Data.Int_NotifyCount++;            // 计数
#endif
        Time = GetRand();
        __HAL_TIM_SET_AUTORELOAD(&htim2, (Time)-1); // 更新
    }

    if(htim->Instance == TIM3) {
#if (_Cnf_Examples > 1)
        switch(Param) {
            case 1:
                s_A8Data.Int1_NotifyCount++; // 中断唤醒1
                break;
            case 2:
                s_A8Data.Int2_NotifyCount++; // 中断唤醒2
                break;
            case 3:
                s_A8Data.Int3_NotifyCount++; // 中断唤醒3
                break;
            case 4:
                s_A8Data.Int4_NotifyCount++; // 中断唤醒4
                break;
        }
        XC_Task_SendNotify(&s_hTCBn[8], (void*)Param); // 唤醒任务8,传递参数8
#endif
        if(++Param > 4) {
            Param = 1;
        }
        Time = GetRand();                           // 30-300 = 3-30ms
        __HAL_TIM_SET_AUTORELOAD(&htim3, (Time)-1); // 更新
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

#if (_Cnf_Examples > 0)
    XC_Time_TickSet(0xFFFFF000 - 1); // 为了测试溢出增加(Tick 初始值接近溢出点,验证时间溢出处理)
#endif
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
    // TIM2
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
    // TIM3
    __HAL_RCC_TIM3_CLK_ENABLE();                       // 启用TIM3时钟
    htim3.Instance           = TIM3;                   // 设置定时器实例
    htim3.Init.Period        = 32 * 10 - 1;            // 设置自动重载寄存器的值，周期为1000-1，因为计数是从0开始的
    htim3.Init.Prescaler     = 6400 - 1;               // 设置预分频器的值，根据系统时钟设置
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; // 时钟分割因子
    htim3.Init.CounterMode   = TIM_COUNTERMODE_UP;     // 向上计数模式
    HAL_TIM_Base_Init(&htim3);                         // 初始化定时器
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim3); // 启动定时器并允许中断
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
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

/**
 * @brief   TIM3中断服务
 */
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3); // 调用HAL库的中断处理函数
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
    srand(4718259);
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
