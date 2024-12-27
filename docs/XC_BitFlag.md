# 位标志的实现
## 1.描述
用于定义处理位标志(boot类型);
使用Byte数组存储,可无限制定义标志;

## 2.文件

| 文件名 | 说明 |
| --- | --- |
| XC_BitFlag.h | 位标志的实现,使用宏和内联实现 |

## 3. 说明

| 函数名 | 函数类型 | 说明 |
| --- | --- | --- |
| XCBitFlag_New | 宏 | 新建(定义)标志变量(会初始化,并清除标志) |
| XCBitFlag_new | 宏 | 新建(定义)标志变量(不初始化,适用于结构体内定义) |
| XCBitFlag_Extern | 宏 | 外部引用标志变量 |
| XCBitFlag_Set | 宏 | 设置一个标志位 |
| XCBitFlag_Clr | 宏 | 清除一个标志位 |
| XCBitFlag_Get | 宏 | 获取一个标志位 |
| XCBitFlag_SetAll | 宏-系统函数 | 设置全部标志位 |
| XCBitFlag_ClrAll | 宏-系统函数 | 清除全部标志位 |
| XCBitFlag_Check | 宏-内联函数 | 判断是否有标志 |
