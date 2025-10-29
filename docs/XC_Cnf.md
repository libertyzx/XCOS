# 配置说明

- 框架裁剪参数如下

| 配置参数名称          | 配置类型  | 说明              |
| ---                   | ---       | ---               |
| _XC_Cnf_IntSupport    | 宏        | 配置框架中断支持  |
| _XC_Cnf_SleepSupport  | 宏        | 配置框架休眠支持  |

- 框架配置参数如下

| 配置参数名称          | 配置类型  | 说明                                                      |
| ---                   | ---       | ---                                                       |
| XCuint_t              | 宏        | 用户可使用的框架基本数量类型,默认32bit,不用修改           |
| _XC_Cnf_TaskMaxNum    | 宏        | 定义最大允许任务数,默认100,不建议修改                     |
| _XC_SysTickPerScond   | 宏        | 系统每秒滴答数(滴答计数频率即1s除以此值,默认1000即1ms)    |
| _XC_SysTickCount      | 宏        | 系统滴答计数,每个单位计数后加1                            |


- **"_XC_SysTickPerScond"系统每秒滴答数(滴答计数频率)说明**
    - 单位Hz,是系统最小的时间单位;
    - 默认值是"1000"也就是1ms;
    - 最大值是1000000Hz;对应时间是1us;

- **"_XC_SysTickCount"系统滴答计数说明**
    - 滴答计数是一个累加值,累加时间必须和"_XC_SysTickPerScond"时间一致;
    - 此值会在操作中被读取(只读),作为时间计算的依据;
    - "_XC_CreateSysTickCount"用于创建一个全局计数变量;
    - "XC_AccSysTickCount()"用户计数中断中调用,计数+1;
    - 系统滴答计数可用2种方式实现:
        - 方式1:中断累加形式(默认);
            - 用一个变量在定时器中累加,每一个滴答时间则+1;
            - 宏"_XC_SysTickCount"指向此变量即可;
            > 用户需要处理:
            > 1. 在工程.c文件中调用"_XC_CreateSysTickCount",以定义一个全局变量(本质是定义一个"volatile uint32_t"类型的全局变量"g_SysTickCount");
            > 2. 创建一个计数定时器,定时器按"_XC_SysTickPerScond"周期计数;
            > 3. 在定时器服务中调用"XC_AccSysTickCount()"(本质是"g_SysTickCount"累加);
        - 方式2:计数器形式;
            - 因为协程并不需要中断来切换上下文,所以为了使效率最高可以用一个计数器来做系统滴答计数;
            - 宏"_XC_SysTickCount"作为一个计数器的函数的返回值(或其本身寄存器值);
            > 用户需要处理:
            > 1. 创建定时器,按"_XC_SysTickPerScond"计数;
            > 2. 将"_XC_SysTickCount"宏指向定时器的计数值;
            > - *注1*: 这里定时器尽量使用32位变量;
            > - *注2*: 宏"_XC_SysTickCount"会频繁只读调用,注意寄存器读取效率;
