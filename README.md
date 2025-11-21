# XCubicOS
## 1.介绍
一个精简的协程调度框架;

## 2.简介
- 纯C语言实现(ANSI/GNU),可裸机或嵌入RTOS等支持C语言的环境;
- 低存储,低内存占用
    - MDK(-O0),无任务: ram=56Byte, rom<900Byte;
    - MDK(-O0),1个纯延时任务: ram=96Byte, rom<1100Byte;
    - 1个任务内存固定占40Byte;
- 协作式任务调度,无优先级概念;
- 无任务堆栈概念,所有任务共用系统堆栈;
- 没有中断上下文切换,框架无需任何调度中断支撑,不需要中断开关;
- 框架目前已实现:阻塞延时,任务通知,任务挂起恢复;

## 3.框架的局限性
- 非实时系统,实时性和编写者代码风格有直接关系;
- 任务通知,任务挂起/恢复在中断中调用的话,响应会有一定延时(异步操作);
- 不做上下文切换保存,任务中局部变量数据会丢失,需要做全局变量;
- 延时,等待通知等函数必须在"任务协程块"内调用(这意味着这些函数不能像RTOS在任意任务子函数内调用);
- 建议在GNU环境下编译,以得到最高性能;

## 4.快速使用
加入工程:

1. 将"src"文件中所有".c"文件加入工程,并将"src/include"添加到工程包含(include paths)即可;
2. 使用时只需包含"XCOS.h"文件;

### 4.1. 调用说明
- 数据类型,常量,函数表见[XCOS](./docs/XCOS.md)说明;
- 详细调用见[例程](./docs/examples.md)说明;
- 基本使用方式如下:
```c
XCOS_t s_hXCOS0 = {0};      //XCOS实例句柄
XCTCB_t s_hTCB0 = {0};      //任务控制块

/**
 * @brief       任务
 * @param[in]   phTCB   任务控制块
 * @details     任务的实现;
 */
void Task(XCTCB_t* phTCB)
{
    XC_Enter(phTCB);        //进入协程块
    /** --- */
    while(1){
        XC_Delay_ms(20);    //延时20ms
    }
    /** --- */
    XC_Leave();             //离开协程块
}

/**
 * @brief       空闲处理
 * @param[in]   hXCOS       框架句柄
 * @param[in]   IdleTick    空闲的Tick
 * @details     空闲处理
 */
void Idle(XCOS_t* hXCOS, XCuint_t IdleTick)
{
    __WFI(); // 休眠
}

/**
 * @brief   主函数
 * @return  不返回
 * @details 初始化,创建任务,运行调度
 */
int main(void)
{
    /*初始化一个1ms的定时器,TimerInt为中断服务*/

    XCSch_Init(&s_hXCOS0);                          // 创建一个XCOS实例
    CSch_SetIdleCallback(&s_hXCOS0, Idle);          // 空闲处理回调
    XCSch_TaskReg(&s_hXCOS0, &s_hTCB0, Task, NULL); // 创建任务
    XCSch_Run(&s_hXCOS0);                           // 调度器运行
}

/**
 * @brief   定时器中断
 * @details 这里1ms一次中断
 */
void TimerInt(void)
{
    XC_AccSysTickCount();   //全局Tick计数
}

```

### 4.2. 配置

正常使用的情况下,框架不需要更改配置,直接调用即可;

若有一些特殊需求,配置见"[XC_Cnf.h](./docs/XC_Cnf.md)配置说明;

## 5.实现
协程实现方式见我的CSDN:
[c语言协程](https://blog.csdn.net/libertyzx/article/details/126186870)

## 6.文件说明

| 文件夹        | 文件          | 说明 |
| :---          | ---           |  --- |
| ./src         | -             | 核心源码文件夹 |
| ./src         | XC_List.c     | 内部链表实现 |
| ./src         | XC_Sch.c      | 调度处理 |
| ./src         | XC_Task.c     | 任务处理 |
| ./src         | XC_Time.c     | 时间处理 |
| ./src/include | -             | 源码".h"文件,需包含 |
| ./src/include | BinData.h     | 二进制数值宏定义 |
| ./src/include | COR_ANSI.h    | 协程底层实现("ANSI-C"是由"switch case"实现) |
| ./src/include | COR_GNU.h     | 协程底层实现("GNU-C" 是由"goto label" 实现) |
| ./src/include | XC_Cnf.h      | 用于存放"XCOS"的配置参数 |
| ./src/include | XC_List.h     | 链表实现头文件 |
| ./src/include | XC_Sch.h      | 调度处理头文件 |
| ./src/include | XC_Task.h     | 任务处理头文件 |
| ./src/include | XC_Time.h     | 时间处理头文件 |
| ./src/include | XCOS.h        | 总包含 |

## 7.更新说明
见文件["XC_UpdateInfo.md"](./docs/XC_UpdateInfo.md)
