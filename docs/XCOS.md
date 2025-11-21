# 函数,宏,类型说明

## 基础宏

| 宏名      | 值    | 所在文件  | 说明                              |
| ---       | ---   | ---       | ---                               |
| \_\_XCOS__  | 200   | XCOS.h    | 用于标记XCOS框架,同时表明版本   |

## 类型

| 类型名        | 32位下占用字节    | 所在文件  | 说明                              |
| ---           | ---               | ---       | ---                               |
| XCuint_t      | 4                 | XC_Cnf.h  | 基本数据类型,时间计数用此类型定义 |
| XCOS_t        | 48                | XC_Sch.h  | 定义一个XCOS框架的句柄            |
| XCTCB_t       | 40                | XC_Task.h | 定义一个协程任务控制块            |
| XCTimeCount_t | 8                 | XC_Time.h | 时间计数数据类型                  |

## 常量

### 函数返回值

- 每个函数返回值意义不同,详细见每个函数的说明;
- 所在文件"XC_Task.h";
- 类型"XCRetuen_t";

| 名称              | 说明          |
| ---               | ---           |
| _XC_R_OK          | 成功          |
| _XC_R_Fail        | 失败或无效    |
| _XC_R_Continue    | 继续或未完成  |

### 任务运行状态

- 所在文件"XC_Task.h";
- 类型"XCState_t";
- 由函数"XC_GetTaskState"返回;

| 名称              | 说明                  |
| ---               | ---                   |
| _XC_S_Void        | 空,被移除后的状态     |
| _XC_S_Run         | 运行                  |
| _XC_S_Ready       | 就绪(注册后的状态)    |
| _XC_S_Suspend     | [阻塞]挂起            |
| _XC_S_Delay       | [阻塞]延时            |
| _XC_S_WaitNotify  | [阻塞]等待通知        |

### 任务唤醒状态

- 所在文件"XC_Task.h";
- 类型"XCWake_t";
- 由函数"XC_GetWakeType"返回;

| 名称                  | 说明              |
| ---                   | ---               |
| _XC_Wake_Non          | 无唤醒            |
| _XC_Wake_Time         | 时间到达唤醒      |
| _XC_Wake_Notify       | 通知到达唤醒      |
| _XC_Wake_TaskResume   | 任务挂起后恢复    |

### 二进制值

8bit数据可以用二进制表示("_B"开头,可以为1-8任意位)如下示例:
 - _B0000_0001
 - _B0
 - _B10100011
 - _B1111001

## 函数表

### 调度器(4个)

| 函数名                | 类型  | 功能                          | 所属文件  |
| ---                   | ---   | ---                           | ---       |
| XCSch_Init            | 函数  | [用户]调度器初始化            | XC_Sch.c  |
| XCSch_Run             | 函数  | [用户]调度器运行(阻塞)        | XC_Sch.c  |
| XCSch_GetTaskNum      | 函数  | [用户]获取任务数              | XC_Sch.c  |
| XCSch_SetIdleCallback | 函数  | [用户]设置空闲处理回调        | XC_Sch.c  |

### 任务处理(12个)

| 函数名                | 类型  | 功能                                  | 所属文件  |
| ---                   | ---   | ---                                   | ---       |
| XC_TaskReg            | 函数  | [用户]任务注册                        | XC_Task.c |
| XC_TaskRemove         | 函数  | [用户]任务移除                        | XC_Task.c |
| XC_TaskReset          | 函数  | [用户]任务复位                        | XC_Task.c |
| XC_SetTaskEntryPoint  | 函数  | [用户]设置任务入口                    | XC_Task.c |
| XC_AddTask            | 函数  | [用户]添加任务                        | XC_Task.c |
| XC_SendNotify         | 函数  | [用户]发送通知                        | XC_Task.c |
| XC_TaskSuspend        | 函数  | [用户]任务挂起                        | XC_Task.c |
| XC_TaskResume         | 函数  | [用户]任务挂起恢复                    | XC_Task.c |
| XC_GetTaskParam       | 宏    | [用户]获取任务注册时传递的参数(void*) | XC_Task.h |
| XC_GetTaskState       | 宏    | [用户]获取任务运行状态                | XC_Task.h |
| XC_UpdateNotifyData   | 宏    | [用户]更新通知数据(void*)             | XC_Task.h |
| XC_ReadNotifyData     | 宏    | [用户]读取通知数据(void*)             | XC_Task.h |

### 任务处理-协程块(16个)

| 函数名                | 类型  | 功能                                                  | 所属文件  |
| ---                   | ---   | ---                                                   | ---       |
| XC_Enter              | 宏    |  [用户][协程]进入                                     | XC_Task.h |
| XC_Leave              | 宏    |  [用户][协程]离开                                     | XC_Task.h |
| XC_Yield              | 宏    |  [用户][协程]让出控制                                 | XC_Task.h |
| XC_Reset              | 宏    |  [用户][协程]任务复位                                 | XC_Task.h |
| XC_GetParam           | 宏    |  [用户][协程]协程块内获取任务注册时传递的参数(void*)  | XC_Task.h |
| XC_Suspend            | 宏    |  [用户][协程]将自身挂起                               | XC_Task.h |
| XC_GetWakeType        | 宏    |  [用户][协程]获取唤醒类型                             | XC_Task.h |
| XC_Remove             | 宏    |  [用户][协程]移除自身                                 | XC_Task.h |
| XC_DelayTick          | 宏    |  [用户][协程]延时n个Tick                              | XC_Task.h |
| XC_Delay_us           | 宏    |  [用户][协程]延时us                                   | XC_Task.h |
| XC_Delay_ms           | 宏    |  [用户][协程]延时ms                                   | XC_Task.h |
| XC_Delay_s            | 宏    |  [用户][协程]延时s                                    | XC_Task.h |
| XC_WaitNotify         | 宏    |  [用户][协程]等待通知(单位:系统Tick)                  | XC_Task.h |
| XC_WaitNotify_ms      | 宏    |  [用户][协程]等待通知(单位:ms)                        | XC_Task.h |
| XC_GetNotifyWakeState | 宏    |  [用户][协程]获取通知唤醒状态                         | XC_Task.h |
| XC_GetNotifyData      | 宏    |  [用户][协程]获取通知的数据(void*)                    | XC_Task.h |

### 时间处理(20个)

| 函数名                    | 类型  | 功能                                      | 所属文件  |
| ---                       | ---   | ---                                       | ---       |
| XCTime_GetTickUnit        | 宏    | [用户]获取系统Tick最小时间(单位:us)       | XC_Time.h |
| XCTime_GetTick            | 宏    | [用户]获取系统Tick                        | XC_Time.h |
| XCTime_AccTick            | 宏    | [用户]累加系统Tick                        | XC_Time.c |
| XCTime_UpdateTick         | 宏    | [用户]更新系统Tick                        | XC_Time.c |
| XCTime_CompareTick        | 宏    | [用户]比较Tick是否到达设定值              | XC_Time.h |
| XCTime_CompareTick_ms     | 宏    | [用户]比较时间是否到达设定值(单位:ms)     | XC_Time.h |
| XCTime_CompareTick_s      | 宏    | [用户]比较时间是否到达设定值(单位:s)      | XC_Time.h |
| XCTime_GetRemainTick      | 函数  | [用户]获取剩下多少Tick                    | XC_Time.c |
| XCTime_GetRunTick         | 函数  | [用户]获取运行了多少Tick                  | XC_Time.c |
| XCTime_tUpdateTick        | 宏    | [用户]更新需要延时比较的Tick              | XC_Time.h |
| XCTime_tUpdateTick_ms     | 宏    | [用户]更新需要延时比较的时间(ms)          | XC_Time.h |
| XCTime_tUpdateTick_s      | 宏    | [用户]更新需要延时比较的时间(s)           | XC_Time.h |
| XCTime_tRepeatLastTick    | 宏    | [用户]重复更新上次需要延时比较的时间      | XC_Time.h |
| XCTime_tCompareTick       | 宏    | [用户]判断需要比较或延时的时间是否到达    | XC_Time.h |
| XCTime_tClrAllCount       | 宏    | [用户]清除类型中所有计数                  | XC_Time.h |
| XCTime_tGetRemainTick     | 函数  | [用户]获取剩下多少Tick                    | XC_Time.c |
| XCTime_tGetRunTick        | 函数  | [用户]获取运行了多少Tick                  | XC_Time.c |
| XCTime_tGetRunTime_ms     | 宏    | [用户]获取运行了多少ms                    | XC_Time.h |
| XCTime_BlockDelay         | 函数  | [用户]死循环延时Tick个计数                | XC_Time.c |
| XCTime_BlockDelay_ms      | 函数  | [用户]死循环延时ms                        | XC_Time.c |

## 函数运行等级表

- 总共52个函数(函数宏);
- 这个表说明了所有函数被中断(或RTOS上下文切换)的等级
    - [4]协程层:   最低等级; 只能在函数协程("XC_Enter"和"XC_Leave"之间)被调用;
    - [3]用户:     正常用户"大循环"中调用;
    - [2]中断:     能在任意位置打断"[4]协程层"和"[3]用户"的函数(中断或RTOS上下文切换);
    - [1]任意:     任何位置可调用;
    - [0]嘀嗒计数: 最高等级; 除了休眠,嘀嗒计数不能停;
    - 备注:
        - [3]用户,也可是一个中断或者一个rtos的任务;
        - 小数字层能在大数字层中调用;

| [4]协程层             | [3]用户                       | [2]中断           | [1]任意                   | [0]嘀嗒计数           | 函数功能                                              |
| ---                   | ---                           | ---               | ---                       | ---                   | ---                                                   |
| XC_Enter              |                               |                   |                           |                       | [用户][协程]进入                                      |
| XC_Leave              |                               |                   |                           |                       | [用户][协程]离开                                      |
| XC_Yield              |                               |                   |                           |                       | [用户][协程]让出控制                                  |
| XC_Reset              |                               |                   |                           |                       | [用户][协程]任务复位                                  |
| XC_GetParam           |                               |                   |                           |                       | [用户][协程]协程块内获取任务注册时传递的参数(void*)   |
| XC_Suspend            |                               |                   |                           |                       | [用户][协程]将自身挂起                                |
| XC_GetWakeType        |                               |                   |                           |                       | [用户][协程]获取唤醒类型                              |
| XC_Remove             |                               |                   |                           |                       | [用户][协程]移除自身                                  |
| XC_DelayTick          |                               |                   |                           |                       | [用户][协程]延时n个Tick                               |
| XC_Delay_us           |                               |                   |                           |                       | [用户][协程]延时us                                    |
| XC_Delay_ms           |                               |                   |                           |                       | [用户][协程]延时ms                                    |
| XC_Delay_s            |                               |                   |                           |                       | [用户][协程]延时s                                     |
| XC_WaitNotify         |                               |                   |                           |                       | [用户][协程]等待通知(单位:系统Tick)                   |
| XC_WaitNotify_ms      |                               |                   |                           |                       | [用户][协程]等待通知(单位:ms)                         |
| XC_GetNotifyWakeState |                               |                   |                           |                       | [用户][协程]获取唤醒超时                              |
| XC_GetNotifyData      |                               |                   |                           |                       | [用户][协程]获取通知的数据(void*)                     |
|                       | XCSch_Init                    |                   |                           |                       | [用户]调度器初始化                                    |
|                       | XCSch_Run                     |                   |                           |                       | [用户]调度器运行(阻塞)                                |
|                       |                               |                   | XCSch_GetTaskNum          |                       | [用户]获取任务数                                      |
|                       | XCSch_SetIdleCallback         |                   |                           |                       | [用户]设置空闲处理回调                                |
|                       | XC_TaskReg                    |                   |                           |                       | [用户]任务注册                                        |
|                       | XC_TaskRemove                 |                   |                           |                       | [用户]任务移除                                        |
|                       | XC_TaskReset                  |                   |                           |                       | [用户]任务复位                                        |
|                       | XC_SetTaskEntryPoint          |                   |                           |                       | [用户]设置任务入口                                    |
|                       | XC_AddTask                    |                   |                           |                       | [用户]添加任务                                        |
|                       |                               | XC_SendNotify     |                           |                       | [用户]发送通知                                        |
|                       |                               | XC_TaskSuspend    |                           |                       | [用户]任务挂起                                        |
|                       |                               | XC_TaskResume     |                           |                       | [用户]任务挂起恢复                                    |
|                       | XC_GetTaskParam               |                   |                           |                       | [用户]获取任务注册时传递的参数(void*)                 |
|                       |                               |                   | XC_GetTaskState           |                       | [用户]获取任务运行状态                                |
|                       | XC_UpdateNotifyData           |                   |                           |                       | [用户]更新通知数据(void*)                             |
|                       | XC_ReadNotifyData             |                   |                           |                       | [用户]读取通知数据(void*)                             |
|                       |                               |                   |                           | XCTime_AccTick        | [用户]累加系统Tick                                    |
|                       |                               |                   | XCTime_GetTickUnit        |                       | [用户]获取系统Tick最小时间(单位:us)                   |
|                       |                               |                   | XCTime_GetTick            |                       | [用户]获取系统Tick                                    |
|                       |                               |                   | XCTime_UpdateTick         |                       | [用户]更新系统Tick                                    |
|                       |                               |                   | XCTime_CompareTick        |                       | [用户]比较Tick是否到达设定值                          |
|                       |                               |                   | XCTime_CompareTick_ms     |                       | [用户]比较时间是否到达设定值(单位:ms)                 |
|                       |                               |                   | XCTime_CompareTick_s      |                       | [用户]比较时间是否到达设定值(单位:s)                  |
|                       |                               |                   | XCTime_GetRemainTick      |                       | [用户]获取剩下多少Tick                                |
|                       |                               |                   | XCTime_GetRunTick         |                       | [用户]获取运行了多少Tick                              |
|                       |                               |                   | XCTime_tUpdateTick        |                       | [用户]更新需要延时比较的Tick                          |
|                       |                               |                   | XCTime_tUpdateTick_ms     |                       | [用户]更新需要延时比较的时间(ms)                      |
|                       |                               |                   | XCTime_tUpdateTick_s      |                       | [用户]更新需要延时比较的时间(s)                       |
|                       |                               |                   | XCTime_tRepeatLastTick    |                       | [用户]重复更新上次需要延时比较的时间                  |
|                       |                               |                   | XCTime_tCompareTick       |                       | [用户]判断需要比较或延时的时间是否到达                |
|                       |                               |                   | XCTime_tClrAllCount       |                       | [用户]清除类型中所有计数                              |
|                       |                               |                   | XCTime_tGetRemainTick     |                       | [用户]获取剩下多少Tick                                |
|                       |                               |                   | XCTime_tGetRunTick        |                       | [用户]获取运行了多少Tick                              |
|                       |                               |                   | XCTime_tGetRunTime_ms     |                       | [用户]获取运行了多少ms                                |
|                       | XCTime_BlockDelay             |                   |                           |                       | [用户]死循环延时Tick个计数                            |
|                       | XCTime_BlockDelay_ms          |                   |                           |                       | [用户]死循环延时ms                                    |

