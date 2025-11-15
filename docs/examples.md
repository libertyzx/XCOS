# 说明

- 例程[主代码位置](../examples/CortexM3_Test/Code/main/main.c)为:"/examples/CortexM3_Test/Code/main/main.c"

例程测试环境如下:

| 项 | 说明 |
|---| --- |
| IDE | DMK |
| 版本 | V5.39.0.0 |
| 工具链 | ARM Compiler V5.06 |
| 软件模拟芯片 | STM32F103ZE |

MDK配置如下图: \n
![](./图/MDK-芯片配置.jpg)

![](./图/MDK-C配置.jpg)

![](./图/MDK-调试配置.jpg)

## 代码和内存占用说明(-O0)

在无任何框架函数及变量的情况下,MDK代码内存占用如下:
- Program Size: Code=3404 RO-data=380 RW-data=16 ZI-data=1024

添加以下框架代码:
- 系统Tick,主文件添加全局变量"_XC_CreateSysTickCount",嘀嗒定时器运行"XC_AccSysTickCount()";
- 1个"XCOS_t"类型框架句柄;
- 框架初始化"XCSch_Init",框架运行"XCSch_Run"

得到代码内存占用为:
- Program Size: Code=4420 RO-data=380 RW-data=20 ZI-data=1076

所以得到:
- Flash占用:1020Byte
- RAM占用: 56Byte (4Byte系统Tick; 52Byte框架句柄)

### 演示代码
创建代码如下:
- 10个任务
- 每个任务带4字节计数;
- 10个任务涵盖框架基本控制函数(任务块控制,延时,通知,挂起,挂起恢复,中断处理,空闲处理)

代码占用为:
- Program Size: Code=6104 RO-data=380 RW-data=20 ZI-data=1516

得到:
- 代码占用: 2704Byte
- 内存占用: 496Byte (4Byte系统Tick + 52Byte框架句柄 + 40Byte任务控制块x10 + 10x4Byte = 496Byte)



-O0
Program Size: Code=6104 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6100 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6108 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6100 RO-data=380 RW-data=20 ZI-data=1516

//无任务
Program Size: Code=4420 RO-data=380 RW-data=20 ZI-data=1076
Program Size: Code=4316 RO-data=380 RW-data=20 ZI-data=1076 //4316-3404 = 912+4=916


//修改总调度运行
Program Size: Code=6148 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6076 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6072 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6068 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6044 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6044 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=5976 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=5976 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=5984 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=5984 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=5988 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=5980 RO-data=380 RW-data=20 ZI-data=1516 //5980-3404=2576
Program Size: Code=6020 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6028 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6024 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6008 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=6004 RO-data=380 RW-data=20 ZI-data=1516
Program Size: Code=5976 RO-data=380 RW-data=20 ZI-data=1516

-O2
Program Size: Code=4752 RO-data=372 RW-data=20 ZI-data=1516
Program Size: Code=4744 RO-data=372 RW-data=20 ZI-data=1516
Program Size: Code=4624 RO-data=372 RW-data=20 ZI-data=1516

-O3
Program Size: Code=4736 RO-data=372 RW-data=20 ZI-data=1516