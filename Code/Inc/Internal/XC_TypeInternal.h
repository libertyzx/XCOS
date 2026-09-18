/**
 * @file        XC_TypeInternal.h
 * @brief       内部类型
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.1
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     存放框架内部类型的代码
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_TypeInternal_h
#define XC_TypeInternal_h
//=== 头文件
#include "Internal/XC_List.h"
#include "XC_Config.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/** 协程底层实现("ANSI-C"和"GNU-C"区分) */
#ifdef __GNUC__
#include "Internal/XC_CorGNU.h" //运行"GNU-C"库
#else
#include "Internal/XC_CorANSI.h" //运行"ANSI-C"库
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 枚举类型 */

/**
 * @brief   [私有]通知唤醒状态
 */
typedef enum {
    XC_NOTIFY_WAKEUP = 0, // 通知唤醒
    XC_NOTIFY_WAIT,       // 等待通知
} XC_NotifyState_t;

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/** 数据类型 */

/**
 * @brief   [用户]嘀嗒计数类型(用户可见: 随 `XC_Type.h` / `XCOS.h` 一起提供)
 * @details
 *  **固定为 32 位无符号**(`uint32_t`), **不可配置** —— 它是全框架的时间基准, 三处都要求"恰好 32 位":
 *  - **接口宽度**: 阻塞/延时入口(`XC_Task_HandleBlocking`/`HandleDelay`/`HandleWaitNotify`)的
 *    "TickCount" 形参是 `uint32_t` ⇒ 更宽的类型在这里被**静默截断**(大延时变成短延时);
 *  - **回绕机制**: 溢出表(`TimeOverflowList`)靠"Tick 回绕"整表搬回时间表 ⇒
 *    更宽的类型回绕周期长到实际不出现 ⇒ 被判"溢出"的节点**永不排空**(任务永久挂死);
 *  - **读的原子性**: `XC_Time_GetTick()` 是对 `volatile` 计数的直接读 ⇒ 32 位在 Cortex-M 上
 *    是**单次原子访问**; 更宽的类型会被中断在两次读之间改写 ⇒ 读到**撕裂值**;
 *  更窄(16 位)则回绕过密(1000Hz 下约 65.5s): 整张时间表被**提前唤醒**(语义破坏), 且换算随之错乱;
 *  "要跑很多年"**不是**加宽的理由: 比较类 API 都是**无符号减法**语义(回绕安全), 跨回绕延时由溢出表支持
 *  ⇒ 只要**单次**超时 < 回绕周期即可无限期运行(**论证与回绕周期表见 `Docs/XC_Config.md`「XC_Tick_t」**);
 * @note
 *  定义位置说明: 本文件是**所有框架类型的最低层**(`XC_Type.h` 包含本文件);
 *  不能把它放到 `XC_Type.h` —— 那样本文件与 `XC_Type.h` 会互相包含(反向包含), 编译层序会冲突;
 */
typedef uint32_t XC_Tick_t;

/**
 * @brief   [内部]XCOS句柄
 * @details
 *  用于记录XCOS实例的数据,一个工程中开源有多个XCOS实例,用此句柄区分;
 *  字节数说明(32bit): 8*4+4+4+4 = 44Byte(含 1Byte 对齐 padding)
 */
struct XCOS_tag {
    /**
     * [并发] 字段级契约(完整矩阵见 `Docs/架构概览/XCOS_V2.1.0_架构概览.md`「并发模型与字段所有权」):
     *  - ReadyList / TimeList / TimeOverflowList / BlockedList : 锁串行化(主循环 + 中断直接路径; Lock 保护, 非无锁)
     *  - PrevTick / TaskNum / fIdle                            : 主上下文(仅主循环; TaskNum 为 uint8_t, 受 XC_CFG_MAX_TASKS 约束)
     *  - Lock                                                  : 串行化握手(不可重入; ISR "读到 0 才获取、配对释放")
     *  - EventPending                                          : MPSC(所有中断写 1; 主循环写 0; 常量写, 无 RMW)
     */
    // 链表
    XC_ListNode_t ReadyList;        // 就绪链表
    XC_ListNode_t TimeList;         // 延时/超时/等待的链表
    XC_ListNode_t TimeOverflowList; // 时间溢出的链表
    XC_ListNode_t BlockedList;      // 阻塞链表

    XC_Tick_t PrevTick; // 上次观察到的Tick(**每次时间调度末尾都更新**), 用于判断 Tick 回绕

    uint8_t          TaskNum; // 任务数量
    volatile uint8_t Lock;    // 锁(1锁定;0解锁),用于中断处理
    /**
     * 异步调度事件-待处理标志(替代原"事件生产者/消费者"计数对);
     * - 并发模型: **MPSC(多生产者单消费者)** —— 所有中断都是生产者,主循环是唯一消费者;
     * - 生产侧只写常量 1(不做读-改-写) => MPSC 下任一交错结果都是 1,不会"丢更新";
     *   改造前的 EventProduced++ 是读-改-写,多中断下属丢更新竞争(仅因门控只要"非零"未暴露);
     * - 主循环"先清后处理":处理期间再置位 => 下轮再处理,不丢事件;
     */
    volatile uint8_t EventPending; // 异步调度事件-待处理(1:需要事件调度;0:无)

    /**
     * @brief       框架空闲处理回调
     * @param[in]   phXCOS      [XC_OSHandle_t]框架句柄
     * @param[in]   IdleTick    [XC_Tick_t]空闲提示值(距下一个任务唤醒还有多少 Tick)
     * @details
     *  在此回调函数中处理空闲相关事宜;
     *  当函数被调用时,必定没有任务是就绪的,框架是空闲的;
     *  ---
     *  **"IdleTick"取值语义(实现回调时务必先判 0)**:
     *  - **"0" = 不要休眠**: 此刻有"已到点 / 待处理"的工作 ⇒ **立刻返回**, 回主循环干活;
     *    产生 0 的三种情况: 时间表首节点唤醒时刻**已过点**、窗口内发生 **Tick 回绕**、
     *    计算期间**刚有任务被唤醒**;
     *    ⚠️ **不要把 0 直接交给"设置休眠时长"类接口** —— 有些平台把 0 解释成"无限/最长",
     *    会一睡不起(正确做法: 判 0 **直接 return**);
     *  - **"~0U"(0xFFFFFFFF) = 无待唤醒任务**: 时间表与溢出表都空 ⇒ 按平台**最大能力**处理
     *    (例如只等下一次中断 / 平台支持的最长休眠周期);
     *    ⚠️ 若平台在休眠期间**停走了 Tick**, 不要把它当成"睡 49.7 天"用(硬件表达不了);
     *  - **其余(1 ~ 0xFFFFFFFE) = 建议休眠上限**: 用它配置唤醒源 / 低功耗定时器即可;
     *  另外: 本提示值是"**快照**" —— 回调返回后状态可能已变(例如刚有任务被唤醒),
     *        按"上限"理解, 不要当作"必然睡满这么久"的前提;
     *  ---
     *  若是在空闲时休眠系统,则需配置系统在"IdleTick"后唤醒框架;
     *  若是系统Tick计数也停止了则需要更新Tick值:
     *  - 系统Tick是定时器中断计数运行的,可以使用以下方式更新:
     *      ```
     *      XC_Tick_t Tick;
     *      Tick = XC_Time_GetTick() + IdleTick;
     *      XC_Time_TickSet(Tick);
     *      ```
     *  - 系统Tick是一个计数器,则计数器等于"XC_Time_GetTick() + IdleTick";
     *  - 注意: XC_Time_TickSet/TickInc 仅在"中断累加模式"下存在
     *      (即定义了 "XC_SYS_TICK_INT_INC_MODE", 见 "XC_Config.h");
     *      若使用"计数器模式"(把 "XC_SYS_TICK_COUNT" 指向硬件计数器),
     *      这两个宏**不存在**, 此时应把**硬件计数器**校正到 "XC_Time_GetTick() + IdleTick";
     */
    void (*fIdle)(struct XCOS_tag*, XC_Tick_t);
};

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   [内部]XCOS核心断点类型
 * @details 用于创建上下文切换的断点标记
 */
typedef COR_BP_t XC_BP_t;

/**
 * @brief   [内部]协程状态(存 TCB.CorState,调度器据此决定是否同轮切父)
 * @details
 *  - "XC_COR_DONE":      本层完成(执行到"XC_Cor_Leave");
 *  - "XC_COR_SUSPENDED": 本层挂起(延时/通知/让出/Call 登记后跳出);
 *  枚举值存 TCB.CorState(uint8_t),与 TaskState/NotifyState 同为"枚举值存 uint8_t 字段"模式;
 */
typedef enum {
    XC_COR_DONE = 0,  // 本层完成
    XC_COR_SUSPENDED, // 本层挂起(延时/通知/让出)
} XC_CorState_t;

/**
 * @brief   [内部]协程函数类型(顶层任务与子协程统一)
 * @details
 *  与现有 TCB.fTask 类型等价(void 返回):参数经"struct XC_TaskCB_tag*"引用,
 *  即用户侧"XC_TaskHandle_t"(XC_TaskCB_t* = struct XC_TaskCB_tag*),任务函数签名
 *  保持 void 不变(非破坏性 API);
 */
typedef void (*XC_CorFn_t)(struct XC_TaskCB_tag*);

/**
 * @brief   [内部]协程帧(用户按需定义帧栈数组; 字段由框架内部维护)
 * @details
 *  8 字节/层(32位):父层函数入口 + 父层返回点;
 *  栈底帧恒为顶层任务(只在第一次"XC_Cor_Call"时写入);
 *  类型名 `XC_CorFrame_t` 在公共头 "XC_Type.h" 中声明(用户只负责定义数组, 不访问字段);
 */
struct XC_CorFrame_tag {
    XC_CorFn_t pfn; // 父层函数入口(弹帧恢复 fTask 用;栈底帧恒为顶层任务)
    XC_BP_t    BP;  // 父层返回点(ANSI:行号 / GNU:标签地址)
};

/**
 * @brief   [内部]协程任务控制块(Task Control Block)
 * @details
 *  用于记录任务控制相关的数据,每个任务都需要一个独立的TCB;
 *  类型占字节数(32bit): **44Byte**(嵌套开启, 默认) / **36Byte**(`XC_CFG_COR_NESTING=0`, 每任务 −8B);
 *  构成: 基础字段 36B + 嵌套字段 7B + 1B 对齐 padding, 整体 4B 对齐;
 *  通知字段由 2Byte 计数对改为 1Byte 标志后(偏移 34 起),偏移 35 转为对齐 padding;
 *  **打开 `XC_CFG_TASK_STATS` 时**再追加 `RunCnt`(4B) + `MaxRunTick`(4B) ⇒ 44 → **52Byte**(+8B/任务);
 *  ---
 *  基础数据(在"XC_Task_BasicInit"中被初始化)
 *      - "TaskWakeupTick"  任务下个唤醒的时间
 *      - "BP"              协程断点
 *      - "pNotifyData"     通知数据
 *      - "NotifyState"     通知状态
 *      - "NotifyPending"   通知-待处理标志
 *      - "CorDepth"        协程深度
 *  非基础数据
 *      - "ListNode"    链表节点
 *      - "phXCOS"      任务所属的框架句柄
 *      - "fTask"       当前层入口(顶层或子层,由 XC_Cor_Call 切换;无嵌套时恒为顶层)
 *      - "pParam"      传递的参数
 *      - "TaskState"   任务状态
 *      - "pCorStack"   用户按需提供的帧栈(NULL=无嵌套)
 *      - "CorDepthMax" 栈容量(越界检查)
 *      - "CorState"    本轮结果
 */
struct XC_TaskCB_tag {
    /**
     * [并发] 字段级契约(2026-09-14; 完整矩阵见 `Docs/架构概览/XCOS_V2.1.0_架构概览.md`「并发模型与字段所有权」):
     *  - ListNode / phXCOS                                : 锁串行化(链表) / 主上下文(句柄)
     *  - fTask / BP / pParam / pCorStack / CorDepth*      : 主上下文(注册期/协程内; 中断不得触碰)
     *  - TaskWakeupTick                                   : 主上下文(HandleBlocking / TimeSched)
     *  - TaskState                                        : 锁串行化(主循环 + SendNotify 直接路径; SUSPEND 受"挂起优先级最高"保护)
     *  - NotifyState                                      : 协议字段(生产侧 SendNotify / 消费侧 WaitNotify·EventSched·Resume)
     *  - NotifyPending                                    : SPSC(生产者只写 1 / 消费者只写 0; 常量写, 无 RMW)
     *  - pNotifyData                                      : SPSC(volatile; "先写数据 → 再置 NotifyPending"的发布顺序由编译器保证)
     *  - CorState / CorDepth / CorDepthMax                : 主上下文(协程内)
     *  - RunCnt / MaxRunTick(仅 TASK_STATS 档)            : 主上下文(仅调度器写; 用户/调试只读, 单字段读原子)
     */
    XC_ListNode_t    ListNode;       // 链表节点
    struct XCOS_tag* phXCOS;         // 任务所属的框架句柄
    XC_CorFn_t       fTask;          // 当前层入口(顶层或子层,由 XC_Cor_Call 切换;无嵌套时恒为顶层)
    XC_BP_t          BP;             // 当前执行层断点(唯一,顶层/子层共用;不占栈)
    XC_Tick_t        TaskWakeupTick; // 任务下个唤醒的时间(0则一直阻塞)

    void* pParam; // 传递的参数
    /**
     * 通知数据 [并发] SPSC: 写者 = 生产者(`XC_Task_SendNotify` / `XC_Task_UpdateNotifyData`), 读者 = 目标任务;
     * - **为什么是 volatile**: 生产侧是"**先写数据、再置标志**(`NotifyPending`)"的发布/订阅配对 ——
     *   若数据不是 volatile, 编译器可以把"数据写"重排到"标志写"之后(两次访问之间没有序列点约束)
     *   ⇒ 消费侧见到标志后可能读到**旧数据**; 加 volatile 后编译器不得跨 volatile 访问重排;
     * - 只保证"**编译器**不重排 + 每次访问都真的读写内存"; **硬件**层面的重排(多核 / 弱序总线 +
     *   中断共享)仍需用户自行加屏障(见 `Docs/架构概览`「并发模型与字段所有权」);
     * - `XC_Task_UpdateNotifyData` 只改数据、**不动标志** ⇒ 已挂起的通知不会因此变"新"(
     *   消费侧仍按 SPSC 语义可能读到旧值), 需要"新数据重新通知"请用 `XC_Task_SendNotify`;
     */
    void* volatile pNotifyData;   // 通知数据 [并发] SPSC(见上方说明)
    volatile uint8_t TaskState;   // 任务状态("XC_TaskState_t"类型数据) [并发] 锁串行化: 主循环 + SendNotify 直接路径
    volatile uint8_t NotifyState; // 通知状态("XC_NotifyState_t"类型数据) [并发] 协议字段: 生产侧 SendNotify / 消费侧 WaitNotify·EventSched·Resume

    /**
     * [并发] SPSC(常量写) —— 框架的通知使用"待处理标志"实现(替代原"生产者/消费者"计数对);
     * - 并发模型: **SPSC(单生产者单消费者)** —— 生产者(某一个中断或任务)只写常量 1,消费者(目标任务自身)只写常量 0;
     * - 两侧都不做"读-改-写" => 无需原子操作/关中断; 即使被多个中断通知(违反单生产者约定)也只是"写同值 1"幂等;
     * - 单字节 volatile:写入即整体生效,不存在"计数回绕/判满"边界(原计数实现曾因此丢唤醒);
     * - 纪律:严禁 "|="、"++"、位域 等写法(会退化成读-改-写);
     */
    volatile uint8_t NotifyPending; // 通知-待处理(1:有未消费通知;0:无)

    /* 协程嵌套支持: 由 `XC_CFG_COR_NESTING` 控制(关闭时本段整体不编译 ⇒ TCB 44 → 36 Byte) */
#if (XC_CFG_COR_NESTING != 0)
    struct XC_CorFrame_tag* pCorStack;   // 用户按需提供的帧栈(NULL=无嵌套)
    uint8_t                 CorDepth;    // 当前子层深度(0=在顶层)
    uint8_t                 CorDepthMax; // 栈容量(越界检查)
    uint8_t                 CorState;    // 本轮结果(值域:XC_CorState_t;填补对齐padding)
#endif

#if (XC_CFG_TASK_STATS != 0)
    /**
     * [并发] 主上下文(仅调度器主循环写) —— 运行统计(仅 `XC_CFG_TASK_STATS` 打开时存在);
     * - 写者: `XC_Diag_RunStatsEnd`(调度器, 每次调度槽结束一次);
     * - 读者: 用户查询 API / 调试器(32 位对齐的单字段读在 Cortex-M 上是原子的; 但两个字段之间不保证成对一致);
     * - 语义: `RunCnt` 为**饱和计数**(到 `0xFFFFFFFF` 后保持, 不回绕); `MaxRunTick` 只增不减;
     * - 清零: 注册(`XC_Task_Add`)/移除(`XC_Task_Remove`)时清零; `XC_Task_Reset` **保留**;
     */
    volatile uint32_t RunCnt;     // 运行次数(饱和计数)
    XC_Tick_t         MaxRunTick; // 最长一次运行耗时(Tick)
#endif
};

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
