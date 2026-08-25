/**
 * @file        XC_Cor.h
 * @brief       协程块实现
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  协程块（Coroutine Block）是 XCOS 的核心执行模型；
 *  所有协程控制宏必须在 `XC_Cor_Enter` / `XC_Cor_Leave` 块中使用；
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_Cor_h
#define XC_Cor_h
//=== 头文件
#include "XC_Task.h"
#include "XC_Time.h" // XC_Time_UsToTicks/MsToTicks/SecToTicks:协程延时及等待通知宏使用
#include "XC_Type.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 协程块:开始/结束 */

/**
 * @brief       [用户][协程]进入
 * @param[in]   phTCB  [XC_TaskHandle_t]任务控制块
 * @details
 *  **协程块的开始;**
 *  必须搭配"XC_Cor_Leave"使用;
 *  进入即置"CorState = XC_COR_SUSPENDED"(本轮默认挂起),由"XC_Cor_Leave"置
 *  "DONE"区分"本层完成"(调度器据此决定是否同轮切父);
 */
#define XC_Cor_Enter(phTCB)                            \
    {                                                  \
        /*全局变量转局部变量可加快运行速度*/           \
        XC_TaskHandle_t phXCCorTCB = (phTCB);          \
        XC_BP_t*        pXCCorPB   = &phXCCorTCB->BP;  \
        phXCCorTCB->CorState       = XC_COR_SUSPENDED; \
        /*启动协程*/                                   \
        COR_Start(*pXCCorPB)

/**
 * @brief       [用户][协程]离开
 * @details
 *  **协程块的结束;**
 *  必须搭配"XC_Cor_Enter"使用;
 *  "DONE"赋值必须在"COR_End()"之前——挂起/Call 经底层 goto 跳出时跳过它,
 *  保持"SUSPENDED";正常走完本层才执行"DONE";
 *  ---
 *  **实现说明(消除 armcc #111-D)**:若 DONE 赋值直接写在循环后(while(1) 恒真),
 *  armcc(AC5)会将其判为"不可达语句"并告警(#111-D)——因为普通语句在无限循环后不可达,
 *  而"标签后的语句"不告警;故此处用 `if(0){goto}` 引用一个标签,使 DONE 赋值成为
 *  "标签后的语句",消除告警;
 *  - 运行时:`if(0)` 恒假、`goto` 永不执行,`do-while(0)` 单次执行,行为与原版逐字节等价;
 *  - 编译期:常量条件 `if(0)` 在 -O0~-O3 均被消除,实测各优化级别 Flash 大小/性能与原版完全一致(+0);
 *  - 任务代码无需任何改动,`while(1) { ... } XC_Cor_Leave();` 写法即可免告警;
 */
#define XC_Cor_Leave()                                 \
    do {                                               \
        if(0) {                                        \
            goto COR_DONE_L;                           \
        } /* 引用下方标签,免 armcc #177-D */           \
    COR_DONE_L:; /* 标签后的语句不触发 armcc #111-D */ \
        phXCCorTCB->CorState = XC_COR_DONE;            \
    } while(0);                                        \
    COR_End(); /*结束*/                                \
    }

/**
 * @brief       [用户][协程]嵌套调用子协程(登记,不做真实 C 嵌套调用)
 * @param[in]   fn  [XC_CorFn_t]子协程函数(签名同任务函数:void (XC_TaskHandle_t))
 * @details
 *  **必须在协程块中使用;**
 *  登记子协程:入栈保存"父层入口+返回点",入口切换为子函数后跳出,由调度器下一轮
 *  进入子层;子层完成后调度器弹帧恢复父层,同轮切父;
 *  需要嵌套的任务必须用"XC_Task_RegExt"/"XC_Task_SetCorStack"配置帧栈,
 *  未配置栈时调用将触发任务复位保护;
 *  **注意**:Call 是必挂起点,调用前后局部变量一律不保存,需跨调用保留的数据
 *  放 TCB/静态区;
 */
#define XC_Cor_Call(fn)                                      \
    {                                                        \
        XC_Task_PushFrame(phXCCorTCB, (fn), COR_RetPoint()); \
        COR_BreakLabel();                                    \
    }

/************************************************ 我是分割线 ************************************************/
/** 协程块:协程控制,必须在协程块中调用 */

/**
 * @brief       [用户][协程]协程块内获取任务注册时传递的参数(void*)
 * @return      void*   返回空指针类型数据;
 * @details
 *  **必须在协程块中使用;**
 *  用来获取参数;
 *  > 注意: 因为协程内上下文切换局部变量是不保存的,
 *  > 所以若是要使用传递的参数需要再"XC_Cor_Enter"前将参数赋值给变量;
 */
#define XC_Cor_GetParam() (phXCCorTCB->pParam)

/**
 * @brief       [用户][协程]让出控制
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,跳出协程,下次调度后从此处继续处理;
 */
#define XC_Cor_Yield()                                            \
    {                                                             \
        phXCCorTCB->TaskState = XC_TASK_READY; /*任务状态:就绪*/  \
        COR_SetBPBreak(*pXCCorPB)              /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]将自身挂起
 * @details
 *  **必须在协程块中使用;**
 *  将自身挂起;
 */
#define XC_Cor_Suspend()                                      \
    {                                                         \
        XC_Task_HandleSuspend(phXCCorTCB); /*任务挂起*/       \
        COR_SetBPBreak(*pXCCorPB);         /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]任务复位
 * @details
 *  **必须在协程块中使用;**
 *  复位当前在运行的任务;运行此宏后将立刻退出协程块
 */
#define XC_Cor_Reset()                   \
    {                                    \
        XC_Task_HandleReset(phXCCorTCB); \
        COR_Break(*pXCCorPB);            \
    }

/**
 * @brief   [用户][协程]移除自身
 * @details
 *  **必须在协程块中使用;**
 *  将自身移除;
 *  注意,唤醒源不会被清除,直到下次唤醒;
 */
#define XC_Cor_Remove()                                \
    {                                                  \
        XC_Task_HandleRemove(phXCCorTCB); /*移除任务*/ \
        COR_Break(*pXCCorPB);             /*直接跳出*/ \
    }

/************************************************ 我是分割线 ************************************************/
/** 协程块:延时处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]延时n个Tick
 * @param[in]   Tick  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时Tick个基础时钟;
 */
#define XC_Cor_DelayTick(Tick)                        \
    {                                                 \
        XC_Task_HandleDelay(phXCCorTCB, Tick);        \
        COR_SetBPBreak(*pXCCorPB); /*设置断点并跳出*/ \
    }

/**
 * @brief       [用户][协程]延时us
 * @param[in]   Us  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时Us个时间;
 *  > 注意:us延时必须要Tick计数在us时基上才能准确的调用;
 */
#define XC_Cor_DelayUs(Us)   XC_Cor_DelayTick(XC_Time_UsToTicks(Us))

/**
 * @brief       [用户][协程]延时ms
 * @param[in]   Ms  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时Ms个时间;
 */
#define XC_Cor_DelayMs(Ms)   XC_Cor_DelayTick(XC_Time_MsToTicks(Ms))

/**
 * @brief       [用户][协程]延时s
 * @param[in]   Sec  [XC_Tick_t]延时的值
 * @details
 *  **必须在协程块中使用;**
 *  让出CPU的使用权,延时Sec个时间;
 */
#define XC_Cor_DelaySec(Sec) XC_Cor_DelayTick(XC_Time_SecToTicks(Sec))

/************************************************ 我是分割线 ************************************************/
/** 协程块:任务通知处理,必须在协程块中调用 */

/**
 * @brief       [用户][协程]等待通知(单位:系统Tick)
 * @param[in]   TickTimeout    [XC_Tick_t]超时时间,单位:系统Tick;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;**
 *  任务等待通知;
 *  唤醒后用"XC_Cor_IsNotifyTimeout"判断是否超时;
 */
#define XC_Cor_WaitNotify(TickTimeout)                     \
    {                                                      \
        XC_Task_HandleWaitNotify(phXCCorTCB, TickTimeout); \
        COR_SetBPBreak(*pXCCorPB); /*设置断点并跳出*/      \
    }

/**
 * @brief       [用户][协程]等待通知(单位:ms)
 * @param[in]   MsTimeout  [XC_Tick_t]超时时间,单位:ms;注意:若参数为0则死等;
 * @details
 *  **必须在协程块中使用;**
 *  任务等待通知;
 *  唤醒后用"XC_Cor_IsNotifyTimeout"判断是否超时;
 */
#define XC_Cor_WaitNotifyMs(MsTimeout) XC_Cor_WaitNotify(XC_Time_MsToTicks(MsTimeout))

/**
 * @brief       [用户]清除通知
 * @param[in]   phTCB       [XC_TaskHandle_t]任务控制块
 * @details
 *  **必须在协程块中使用;**
 *  用于清除通知;
 *  可在"等待通知"前调用,防止通知提前到达;
 */
#define XC_Cor_ClrNotify()                                       \
    do {                                                         \
        phXCCorTCB->NotifyConsumed = phXCCorTCB->NotifyProduced; \
    } while(0)

/**
 * @brief       [用户][协程]检查通知唤醒是否超时
 * @return      bool
 * @retval      0 : 没有超时
 * @retval      1 : 超时
 * @details
 *  **必须在协程块中使用;**
 *  用于判断任务通知阻塞唤醒后是否超时;
 *  > 通知函数:XC_Cor_WaitNotify;
 */
#define XC_Cor_IsNotifyTimeout() (phXCCorTCB->NotifyState != XC_NOTIFY_WAKEUP)

/**
 * @brief       [用户][协程]获取通知的数据(void*)
 * @return      void*   返回通知数据
 * @details
 *  **必须在协程块中使用;**
 *  被通知唤醒后获取通知传递的数据;
 */
#define XC_Cor_GetNotifyData()   (phXCCorTCB->pNotifyData)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

//=== 文件结束
#endif
