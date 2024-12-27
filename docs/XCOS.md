# 句柄类型

| 类型名 | 32位下占用字节 | 所属文件 | 说明 |
| --- | --- | --- | --- |
| XCOS_t | 56 | XC_Sch.h | 定义一个XCOS框架的句柄 |
| XCTCB_t | 40 | XC_Task.h | 定义一个协程任务控制块 |
| XCSemBin_t | 16 | XC_Sem.h | 定义一个二值信号量句柄 |

# 常量

- **函数返回值**

具体说明见函数头注释;

| 名 | 说明 |
| --- | --- |
| _XC_R_OK | 成功 |
| _XC_R_Fail | 失败或无效 |
| _XC_R_Continue | 继续或未完成 |

- **任务运行状态**

由函数"XC_GetTaskState"返回;

| 名 | 说明 |
| --- | --- |
| _XC_S_Void | 被移除后的状态 |
| _XC_S_Run | 运行 |
| _XC_S_Ready | 就绪(注册后的状态) |
| _XC_S_Delay | [阻塞]延时 |
| _XC_S_WaitNotify | [阻塞]等待通知 |
| _XC_S_WaitSem | [阻塞]等待信号 |
| _XC_S_Suspend | [阻塞]挂起 |


# 基础函数表

- **调度器操作**

| 函数名 | 函数类型 | 功能 | 注意 | 所属文件 | 说明 |
| --- | --- | --- | --- | --- | --- |
| XCSch_Init | 函数 | 调度器 | - | XC_Sch.c | 初始化一个调度器 |
| XCSch_Run | 函数 | 调度器 | - | XC_Sch.c | 运行调度器(阻塞) |
| XCSch_RunNonBlocked | 函数 | 调度器 | - | XC_Sch.c | 运行调度器(非阻塞) |
| XCSch_GetTaskNum | 函数 | 调度器 | - | XC_Sch.c |  返回当前任务数量 |

- **任务操作**

| 函数名 | 函数类型 | 功能 | 注意 | 所属文件 | 说明 |
| --- | --- | --- | --- | --- | --- |
| XCSch_TaskReg | 函数 | 任务操作 | **不可在中断中调用** | XC_Sch.c | 注册一个任务 |
| XCSch_TaskRemove | 函数 | 任务操作 | **不可在中断中调用** | XC_Sch.c | 移除一个任务 |
| XCSch_TaskReset | 函数 | 任务操作 | **不可在中断中调用,不可复位自身** | XC_Sch.c | 复位任务,需要复位自身则调用"XC_TaskReset" |
| XC_Suspend | 宏 | 任务挂起 | ***协程块内调用*** | XC_Task.h | 挂起自身 |
| XC_TaskSuspend | 函数 | 任务挂起 | **不可在中断中调用** | XC_Task.c | 任务挂起 |
| XC_TaskResume | 函数 | 任务挂起 | **不可在中断中调用** | XC_Task.c | 任务恢复 |

- **协程块基础操作**

| 函数名 | 函数类型 | 功能 | 注意 | 所属文件 | 说明 |
| --- | --- | --- | --- | --- | --- |
| XC_Enter | 宏 | 基础 | - | XC_Task.h | 进入协程块 |
| XC_Leave | 宏 | 基础 | - | XC_Task.h | 离开协程块 |
| XC_Reset | 宏 | 基础 | ***协程块内调用*** | XC_Task.h | 复位自身,立刻退出协程块 |
| XC_Yield | 宏 | 基础 | ***协程块内调用*** | XC_Task.h | 使当前任务让出CPU的控制权 |
| XC_GetParam  | 宏 | 基础 | ***协程块内调用*** | XC_Task.h | 协程块内获取任务注册时传递的参数(指针) |
| XC_GetParamUint  | 宏 | 基础 | ***协程块内调用*** | XC_Task.h | 协程块内获取任务注册时传递的参数(32位无符号) |
| XC_GetTaskParam | 宏 | 基础 | **任意位置调用** | XC_Task.h | 获取任务注册时传递的参数(指针) |
| XC_GetTaskParamUint | 宏 | 基础 | **任意位置调用** | XC_Task.h | 获取任务注册时传递的参数(32位无符号) |
| XC_GetTaskState | 宏 | 基础 | **任意位置调用** | XC_Task.h | 获取任务运行状态 |
| XC_GetWakeTimeout | 宏 | 基础 | ***协程块内调用*** | XC_Task.h | 获取任务唤醒超时状态(目前支持通知和二值信号量) |

- **延时操作**

| 函数名 | 函数类型 | 功能 | 注意 | 所属文件 | 说明 |
| --- | --- | --- | --- | --- | --- |
| XC_DelayTick | 宏 | 时间调度 | ***协程块内调用*** | XC_Task.h | 延时n个Tick |
| XC_Delay_ms | 宏 | 时间调度 | ***协程块内调用*** | XC_Task.h | 延时n毫秒时间 |
| XC_Delay_s | 宏 | 时间调度 | ***协程块内调用*** | XC_Task.h | 延时n秒时间|
| XC_Delay_min | 宏 | 时间调度 | ***协程块内调用*** | XC_Task.h | 延时n分钟时间 |
| XC_Delay_h | 宏 | 时间调度 | ***协程块内调用*** | XC_Task.h | 延时n小时时间 |
| XC_Delay_day | 宏 | 时间调度 | ***协程块内调用*** | XC_Task.h | 延时n天时间 |

- **任务通知操作**

| 函数名 | 函数类型 | 功能 | 注意 | 所属文件 | 说明 |
| --- | --- | --- | --- | --- | --- |
| XC_SendNotify | 函数 | 任务通知 | **不可在中断中调用** | XC_Task.c | 发送通知,数据是指针 |
| XC_SendNotifyUint  | 宏 | 任务通知 | **不可在中断中调用** | XC_Task.h | 发送通知,数据是32位无符号 |
| XC_WaitNotify | 宏 | 任务通知 | ***协程块内调用*** | XC_Task.h | 等待通知到来(超时单位:Tisk)  |
| XC_WaitNotify_ms | 宏 | 任务通知 | ***协程块内调用*** | XC_Task.h | 等待通知到来(超时单位:ms) |
| XC_GetNotifyData | 宏 | 任务通知 | ***协程块内调用*** | XC_Task.h | 获取通知的数据(指针) |
| XC_GetNotifyDataUint | 宏 | 任务通知 | ***协程块内调用*** | XC_Task.h | 获取通知的数据(32位无符号) |
| XC_UpdateNotifyData | 宏 | 任务通知 | **任意位置调用** | XC_Task.h | 更新通知数据(指针) |
| XC_UpdateNotifyDataUint | 宏 | 任务通知 | **任意位置调用** | XC_Task.h | 更新通知数据(32位无符号) |
| XC_ReadNotifyData | 宏 | 任务通知 | **任意位置调用** | XC_Task.h | 读取通知数据(指针) |
| XC_ReadNotifyDataUint | 宏 | 任务通知 | **任意位置调用** | XC_Task.h | 读取通知数据(32位无符号) |

- **二值信号量操作**

> ***二值信号量可在中断中调用***

| 函数名 | 函数类型 | 功能 | 注意 | 所属文件 | 说明 |
| --- | --- | --- | --- | --- | --- |
| XCSem_BinSemInit | 函数 | 二值信号量 | - | XC_Sem.c | 初始化一个信号量 |
| XCSem_BinSemTake | 宏 | 二值信号量 | ***协程块内调用*** | XC_Sem.h | 获取信号(消费者)(超时单位:Tisk) |
| XCSem_BinSemTake_ms | 宏 | 二值信号量 | ***协程块内调用*** | XC_Sem.h | 获取信号(消费者)(超时单位:ms) |
| XCSem_BinSemGive | 函数 | 二值信号量 | **任意位置调用(可中断调用)** | XC_Sem.c | 释放信号(生产者) |
| XCSem_BinSemForceClrSem | 函数 | 二值信号量 | - | XC_Sem.c | 强制清除信号(获取信号,消费者) |
| XCSem_BinSemForceSetSem | 函数 | 二值信号量 | - | XC_Sem.c | 强制设置信号(释放信号,生产者) |
