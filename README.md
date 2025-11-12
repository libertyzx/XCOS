# XCubicOS
## 1.介绍
一个精简的协程系统调度框架;

## 2.简介
- 纯C语言实现,可裸机或嵌入RTOS等支持C语言的环境;
- 低存储,低内存占用
    - -O0,无任务: ram=4Byte+52Byte, rom<1024Byte;
    - 1个任务内存固定占40Byte;
- 协作式任务调度,无优先级概念;
- 无任务堆栈概念,所有任务共用系统堆栈;
- 没有中断上下文切换,框架无需任何中断支撑,没有临界段和中断开关;
- 框架目前已实现:阻塞延时,任务通知,任务挂起恢复;
    - 任务通知,任务挂起/恢复可在中断中使用;
- 框架的局限性:
    - 非实时系统,实时性和编写者代码风格有直接关系;
    - 任务通知,任务挂起/恢复在中断中调用的话,响应会有一定延时;
    - 不做上下文切换保存,任务中局部变量数据会丢失,需要做全局变量;

## 3.快速使用
加入工程
> - 将"src"文件中所有".c"文件加入工程,并将"src/include"添加到工程包含(include paths)即可;
> - 使用时只需包含"XCOS.h"文件;

### 3.1. 调用说明
- 句柄类型,常量,函数表见[XCOS](./docs/XCOS.md)说明;
- 详细调用见[例子](./docs/examples.md)说明;
- 基本使用方式如下:
```c
_XC_CreateSysTickCount;     //创建系统Tick
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
//===
    while(1){
        XC_Delay_ms(20);    //延时20ms
    }
//===
    XC_Leave();             //离开协程块
}

/**
 * @brief   主函数
 * @return  不返回
 * @details 初始化,创建任务,运行调度
 */
int main(void)
{
    /*初始化一个1ms的定时器,TimerInt为中断服务*/

    XCSch_Init(&s_hXCOS0);  //创建一个XCOS实例
    XCSch_TaskReg(&s_hXCOS0, &s_hTCB0, Task, NULL); //创建任务
    XCSch_Run(&s_hXCOS0);   //调度器运行
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

### 3.2. 配置

默认情况下直接调用即可,若是要极限裁剪或者一些其他配置见"[配置说明](./docs/XC_Cnf.md)";
- 所有配置参数在"XC_Cnf.h"文件中定义;

## 4.实现
协程实现方式见我的CSDN:
[c语言协程](https://blog.csdn.net/libertyzx/article/details/126186870)

## 5.文件说明

| 文件夹        | 文件          | 说明 |
| :---          | ---           |  --- |
| ./src         | -             | 核心源码文件夹 |
| ./src         | XC_List.c     | 内部使用,链表实现 |
| ./src         | XC_Sch.c      | 调度处理 |
| ./src         | XC_Task.c     | 任务处理 |
| ./src         | XC_Time.c     | 时间计数处理 |
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

## 6.更新说明
见文件["XC_UpdateInfo.md"](./docs/XC_UpdateInfo.md)
