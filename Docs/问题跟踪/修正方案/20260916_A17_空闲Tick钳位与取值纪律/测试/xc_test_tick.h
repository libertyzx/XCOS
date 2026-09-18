/**
 * @file    xc_test_tick.h
 * @brief   [测试]脚本化 Tick 源声明(A17 复现用例)
 * @details
 *  本用例把系统 Tick 源切到"**方式2: 直接计数器模式**"(见 `Docs/XC_Config.md`「XC_SYS_TICK_COUNT」),
 *  从而可以在**框架内部两次读 Tick 之间**(= 调度主循环的一次迭代内)注入 Tick 跳变,
 *  等价于"TimeSched 读 Tick 之后、空闲分支读 Tick 之前, 时间已经前进了若干 tick"。
 *
 *  编译开关(加入 `Tests/manifest.txt` 时照抄):
 *      -include "Docs\问题跟踪\修正方案\20260916_A17_空闲Tick钳位与取值纪律\测试\xc_test_tick.h" -DXC_SYS_TICK_COUNT=XC_TestTickRead()
 *
 *  @note 仅测试使用; 真实工程按 `XC_Config.md` 的方式1/方式2 配置 Tick 源。
 */
//=== 防重复定义
#ifndef xc_test_tick_h
#define xc_test_tick_h
//=== 头文件
#include "XC_Type.h" // XC_Tick_t(类型层; 已不再由 XC_Config.h 提供)

/* 由用例实现(非静态: 框架各编译单元都要链接到它) */
XC_Tick_t XC_TestTickRead(void);

//=== 文件结束
#endif
