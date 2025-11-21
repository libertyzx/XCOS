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
此处Flash和RAM大小只做参考;

示例代码中可以修改在例程的"main.h"文件中的"_Cnf_Examples"宏的值来切换配置:
- 0:关闭所有框架代码;
- 1:只开启框架,无空闲回调,无任务;
- 2:一个延时任务,无空闲回调;
- 3:10个测试任务,有中断处理,空闲回调为"__WFI()"休眠;

### 配置0
在无任何框架函数及变量的情况下,MDK代码内存占用如下:
> Program Size: Code=4504 RO-data=380 RW-data=16 ZI-data=1096

### 配置1
添加以下框架代码:
- 1个"XCOS_t"类型框架句柄;
- 嘀嗒中断中运行"XC_AccSysTickCount()";
- 框架初始化"XCSch_Init",框架运行"XCSch_Run"

得到代码内存占用为:
> Program Size: Code=5352 RO-data=380 RW-data=20 ZI-data=1148

得到:
- Flash占用: (5352-4504) + (20-16) = 852Byte
- RAM占用: (1148-1096) + (20-16) = 56Byte

### 配置2
只开启一个纯延时任务
> Program Size: Code=5544 RO-data=380 RW-data=20 ZI-data=1188

得到:
- Flash占用: (5544-4504) + (20-16) = 1044Byte
- RAM占用: (1188-1096) + (20-16) = 96Byte

### 配置3
代码如下:
- 10个任务
- 每个任务独立计数;
- 10个任务涵盖框架基本控制函数(任务块控制,延时,通知,挂起,挂起恢复,中断处理,空闲处理)
