/**
 * @file        XC_Config.h
 * @brief       配置
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     用于存放"XCOS"的配置参数;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 防重复定义
#ifndef XC_Config_h
#define XC_Config_h
//=== 头文件
#include <stddef.h>
#include <stdint.h>

//=== 全局配置
#ifdef XCOS_CFG
#include "XCOS_Cfg.h"
#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 需要配置的参数

/**
 * @brief   [配置]嘀嗒计数数据类型
 * @details
 *  默认32位,用于系统滴答计数的数据
 *  一般不用改
 */
#ifndef XC_CFG_TICK_TYPE
#define XC_CFG_TICK_TYPE uint32_t
#endif

/**
 * @brief   [内部]嘀嗒计数数据类型定义
 * @details
 *  由"XC_CFG_TICK_TYPE"指定的类型定义出"XC_Tick_t";
 *  必须定义在本文件(最底层配置),供"XC_Config.h"的全局滴答计数声明及上层类型使用;
 */
typedef XC_CFG_TICK_TYPE XC_Tick_t;

/**
 * @brief   [内部]编译期断言: Tick 类型必须 >= 32 位
 * @details
 *  16 位 Tick 会在 65535 个 Tick 后回绕(1000Hz 下约 65.5s),被调度器判为"时间溢出",
 *  从而把**整张时间表一次性搬到就绪表** ⇒ 所有延时任务被提前唤醒(语义破坏),
 *  且 TicksToMs/GetMs 等换算随之错乱;
 *  本断言用"负数组长度"实现: 不满足时编译报错, 且**不产生任何代码/数据**(0 开销);
 *  位宽约束与配置说明见 Docs/XC_Config.md;
 */
typedef char XC_StaticAssert_TickWidth[(sizeof(XC_Tick_t) >= 4U) ? 1 : -1];

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   [配置]最大任务数
 * @details
 *  因为是协作式调度,任务数太多会导致每个任务运行卡顿;
 *  这个参数限制最大能注册的任务;
 *  数值范围:上限 **255**(由下方编译期断言强制, 因为框架实例的 "TaskNum" 是 uint8_t);
 *  建议取值:>= 10(任务太少时"轮转粒度"粗, 实时性反而不稳)
 *  默认:100
 *  注: 超出上限时 "XC_Task_Reg"/"XC_Task_RegExt"/"XC_Task_Add" 返回 "XC_FAIL"(运行期检查);
 */
#ifndef XC_CFG_MAX_TASKS
#define XC_CFG_MAX_TASKS (100U)
#endif

/**
 * @brief   [内部]编译期断言: "XC_CFG_MAX_TASKS" 必须在 1~255 之间
 * @details
 *  - 上限 255 是**硬约束**: 框架实例的 "TaskNum" 是 uint8_t, 若配置超过 255, 任务数会**回绕**
 *    (注册再多也涨不上去), "XC_Task_Remove" 的 "TaskNum--" 还会下溢 => 诊断/统计全部失真;
 *  - 下限 1 是"配置写错"的兜底(写 0 表示任何任务都不能注册, 通常是笔误);
 *  本断言用"负数组长度"实现: 不满足即编译报错, 且**不产生任何代码/数据**(0 开销);
 */
typedef char XC_StaticAssert_MaxTasks[((XC_CFG_MAX_TASKS >= 1U) && (XC_CFG_MAX_TASKS <= 255U)) ? 1 : -1];

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   [配置]系统每秒滴答数(滴答计数频率)
 * @details
 *  单位Hz,是系统最小的时间单位;
 *  最大值是1000000Hz;对应时间是1us;
 *  一般设置1000,对应时间为1ms;
 *  注意: 取值 <=1000Hz 时**必须是 1000 的因子**(1/2/4/5/8/10/20/25/40/50/100/125/200/250/500/1000)
 *        —— 否则 ms 换算会失真(TicksToMs/GetMs 偏小、MsToTicks 偏长), 框架有**编译期断言**拦截;
 *        取值 >1000Hz(<=1000000)时按 us 时基换算, 取值任意;
 */
#ifndef XC_CFG_TICKS_PER_SEC
#define XC_CFG_TICKS_PER_SEC (1000U)
#endif

/**
 * @brief   [配置]错误上报钩子开关
 * @details
 *  取值: 0(默认,关闭) / 1(开启);
 *  ---
 *  开启后, 框架在检测到"**用户可修复的错误**"时调用用户实现的钩子
 *  `XC_Err_Hook(phTCB, ErrCode)`(声明与错误码见 `XC_Err.h`), 便于现场记录日志、
 *  点亮指示灯或打断点定位; **不会**改变框架原有的安全降级流程;
 *  关闭(默认)时所有上报点编译为**空操作** ⇒ 0 代码 / 0 RAM / 0 开销, 且无需实现钩子;
 *  ---
 *  当前上报点:
 *      - 帧栈越界(`XC_ERR_FRAME_OVERFLOW`): 未配帧栈就调用 `XC_Cor_Call`, 或嵌套层数超过 `CorDepthMax`
 *        —— 原行为是"静默复位任务"(任务从头运行, 现场无任何痕迹), 打开本开关即可捕获;
 *  ---
 *  用法:
 *      1. 编译期定义 `XC_CFG_ERR_HOOK=1`(或在包含 `XCOS.h` 前 `#define XC_CFG_ERR_HOOK (1U)`);
 *      2. 实现 `void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)`;
 *  详见 `Docs/XC_Config.md`「XC_CFG_ERR_HOOK」与 `Docs/Skill/XCOS_Agent_Skill.md` 相关陷阱;
 */
#ifndef XC_CFG_ERR_HOOK
#define XC_CFG_ERR_HOOK (0U)
#endif

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   [配置]运行期不变量自检开关(调试用)
 * @details
 *  取值: 0(默认,关闭) / 1(开启);
 *  ---
 *  开启后框架额外提供 `XC_Diag_CheckInvariants(phXCOS)`(声明见 `XC_Diag.h`):
 *  它把"四张任务表 + 每个任务的状态标签"逐条盘点一遍, 检查内核依赖的**不变量**是否仍成立
 *  (状态↔所在表匹配、同一任务不重复挂表、`TaskNum` 与节点总数一致、协程帧栈与深度自洽、链表双向指针完好),
 *  返回 `XC_CHK_OK` 或**第一个失败项编号**;
 *  ---
 *  定位: 这是**诊断/调试**手段, **不参与内核运行**(不改变任何行为, 也**不会**自动调用);
 *  关闭(默认)时整个自检实现**不编译** ⇒ 发布固件 **0 代码 / 0 RAM / 0 耗时**;
 *  开启(调试档)的体积: 增量**全部落在诊断模块**(`Code/Src/XC_Diag.c`) ⇒ 内核侧 `XC_Sch.o`/`XC_Task.o` **不变**;
 *  镜像级同理(示例工程 `Examples/CortexM3_Test` + 启动时调用一次); RO/RW/ZI 均不变 ⇒ **RAM 0**(不使用静态/堆内存, 仅少量栈);
 *  **具体数值见 `Tests/baseline.md`**(机器口径 `Tests/baseline.json`; armcc AC5 `-O1 --cpu=Cortex-M3`);
 *  ---
 *  推荐用法(选一种或并用):
 *      1. 开发/回归期: 在**每个 API 调用之后**调一次 ⇒ 在"出错的那一步"立刻报出不一致(定位最强);
 *      2. 运行期守护: 在空闲回调(`XC_Sch_SetIdleCallback`)里每隔若干轮抽查一次;
 *      3. 排障期: 怀疑"任务卡死/莫名重启"时, 在可疑点手工插入一次调用;
 *  自检内部会**短暂持锁**(防中断直接路径改表), 因此不要在"已持锁"的上下文里调用;
 *  检查项与用法详见 `Docs/XCOS.md`「运行期自检」节;
 */
#ifndef XC_CFG_DEBUG_CHECK
#define XC_CFG_DEBUG_CHECK (0U)
#endif

/**
 * @brief   [配置]每任务运行统计开关(可选功能)
 * @details
 *  取值: 0(默认,关闭) / 1(开启);
 *  ---
 *  开启后, 调度器会在**每次"调度槽"**前后取时刻, 为每个任务累计:
 *      - `RunCnt`     : 该任务被运行的次数(饱和计数, 不回绕);
 *      - `MaxRunTick` : 该任务**最长一次**运行耗时(单位 Tick);
 *  查询/清零 API: `XC_Diag_GetRunCnt` / `XC_Diag_GetMaxRunTick` / `XC_Diag_ClrRunStats`(声明见 `XC_Diag.h`);
 *  ---
 *  计数口径(一次"调度槽"): 从任务被摘出就绪表开始, 到它让出(延时/等待/挂起/让出)为止,
 *  含"同轮切父"的父层运行时间; 任务**等待期间不计时**;
 *  ---
 *  体积/内存(实测, armcc AC5 `-O1 --cpu=Cortex-M3`):
 *      - 关闭(默认): **0 代码 / 0 RAM**(统计字段与统计代码都不编译);
 *      - 开启: TCB **44 → 52 Byte(+8 Byte/任务)**(100 任务 ⇒ +800 Byte RAM); 代码增量见 `Docs/XC_Config.md` 实测表;
 *  ---
 *  精度与边界(务必了解):
 *      - 默认 1ms 时基 ⇒ **短于 1 tick 的任务记到 0**(量化误差 ±1 tick);
 *        需要更细精度: 用"计数器模式"(`XC_SYS_TICK_COUNT` 指向硬件计数器)或高频档(`>1000Hz` 走 us 时基);
 *      - 测量包含运行期间的**中断时间** ⇒ 它是"调度槽耗时", 不等价于"纯任务指令数"
 *        (要看指令周期请用 `Docs/Skill/MDK_Sim_Agent_Skill.md` 的仿真测量, 二者互补);
 *      - 生命周期: **注册/移除时清零**(`XC_Task_Add`/`XC_Task_Remove`), **`XC_Task_Reset` 保留**(排障场景不丢证据);
 *        ⚠️ 例外: 任务在**本调度槽内移除自身**时不再累计(否则会覆盖"移除时清零", 残留 RunCnt=1);
 *  ---
 *  用途: 协作式内核无优先级 ⇒ "某任务执行多久"直接决定其它任务的额外延迟;
 *  本开关用于回答"**哪个任务是耗时大户**、某任务最长跑了多久"(配合 `MaxRunTick` 与运行次数取平均);
 *  详见 `Docs/XC_Config.md`「XC_CFG_TASK_STATS」与 `Docs/XCOS.md`「运行统计」;
 */
#ifndef XC_CFG_TASK_STATS
#define XC_CFG_TASK_STATS (0U)
#endif

/**
 * @brief   [配置]参数/状态断言开关(诊断用)
 * @details
 *  取值: 0(默认,关闭) / 1(开启);
 *  ---
 *  开启后可用 `XC_DIAG_ASSERT(cond)`:
 *      - 条件为假 ⇒ 经"错误上报钩子"(错误码/钩子声明见 `XC_Err.h`)上报 `XC_ERR_ASSERT`(在钩子内打断点即可由调用栈定位);
 *      - **只上报, 不改变控制流**(不 return/不止损) —— 内核原有 `return XC_FAIL` 等检查一律保留;
 *  关闭(默认)时宏展开为 `((void)0)` ⇒ **0 代码 / 0 RAM**, 且条件**不求值**;
 *  ---
 *  ⚠️ **依赖 `XC_CFG_ERR_HOOK`**: 本开关为 1 且钩子开关为 0 时**编译报错**(断言失败将不可见);
 *  单独打开本开关无用 ⇒ 调试档建议 `-DXC_CFG_ASSERT=1 -DXC_CFG_ERR_HOOK=1`;
 *  ---
 *  用法示例(建议放在 API 入口/状态前提处):
 *      XC_DIAG_ASSERT(phTCB != NULL);
 *      XC_DIAG_ASSERT((phTCB->pCorStack == NULL) == (phTCB->CorDepthMax == 0U));
 *  详见 `Docs/XC_Config.md`「XC_CFG_ASSERT」与 `Docs/XCOS.md`「断言」;
 */
#ifndef XC_CFG_ASSERT
#define XC_CFG_ASSERT (0U)
#endif

/************************************************ 我是分割线 ************************************************/

/**
 * @brief   [配置]系统滴答计数实现
 * @details
 *  滴答计数是一个累加值,累加时间必须和"XC_CFG_TICKS_PER_SEC"时间一致;
 *  此值会在操作中被读取,作为时间计算的依据;
 *  系统滴答计数可用2种方式使用:
 *--- 方式1:中断累加形式(默认);
 *  用一个变量在定时器中累加,每一个滴答时间则+1;
 *  在此模式下,系统已经定义了一个全局变量"g_SysTickCount"(位于"XC_Time.c"文件),
 *  并同时在本文件中做了"外部声明全局滴答时间计数";
 *  用户只需要做以下处理即可:
 *      1.创建定时器,按"XC_CFG_TICKS_PER_SEC"计数;
 *      2.在定时器中累加系统滴答计数,即调用"XC_Time_TickInc()"函数;
 *--- 方式2:计数器形式;
 *  因为协程并不需要中断来切换上下文,所以为了使效率最高可以用一个计数器来做系统滴答计数;
 *  宏"XC_SYS_TICK_COUNT"作为一个计数器的函数的返回值(或其本身寄存器值);
 *  用户需要处理:
 *      1.创建定时器,按"XC_CFG_TICKS_PER_SEC"计数;
 *      2.将计数值作为"XC_SYS_TICK_COUNT"的指向;
 *  注意:
 *      计数器必须是32位计数器;
 *      宏"XC_SYS_TICK_COUNT"会频繁只读调用,注意寄存器读取效率;
 */
#ifndef XC_SYS_TICK_COUNT

#define XC_SYS_TICK_INT_INC_MODE          // [默认]以中断递增的模式形式
extern volatile XC_Tick_t g_SysTickCount; // 外部声明全局滴答时间计数
#define XC_SYS_TICK_COUNT g_SysTickCount  // 调用滴答时间计数

#endif

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
