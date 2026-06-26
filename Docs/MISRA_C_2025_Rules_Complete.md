# MISRA C:2025 完整规则清单

> **来源**: Perforce QAC for C 2026.2 Rule Enforcement Summary + LDRA MISRA C:2025 分类统计  
> **统计**: 201条 Rules + 22条 Directives = 223条 Guidelines（MISRA C:2023 为221条，2025新增4条Rules，净增2条）  
> **规则分类**: Mandatory(强制) 22 / Required(必要) 139 / Advisory(建议) 39 / Disapplied(废止) 1 (Rule 15.5)  
> **可静态执行**: 201条中 Decidable 规则可由工具自动判定，Undecidable 规则需人工审查；Perforce QAC 对全部可判定规则 **100% 覆盖**  
> **不可静态执行**: 22条 Directives（Assisted: 17条 提供辅助检查 / Unassisted: 5条 需完全人工）

---

## 目录

- [§1 环境 (Rules 1.1, 1.3-1.5)](#1-环境-rules-11-13-15)
- [§2 未使用代码 (Rules 2.1-2.8)](#2-未使用代码-rules-21-28)
- [§3 注释 (Rules 3.1-3.2)](#3-注释-rules-31-32)
- [§4 字符常量 (Rules 4.1-4.2)](#4-字符常量-rules-41-42)
- [§5 标识符 (Rules 5.1-5.10)](#5-标识符-rules-51-510)
- [§6 位域 (Rules 6.1-6.3)](#6-位域-rules-61-63)
- [§7 字面量 (Rules 7.1-7.6)](#7-字面量-rules-71-76)
- [§8 声明与定义 (Rules 8.1-8.19)](#8-声明与定义-rules-81-819)
- [§9 初始化 (Rules 9.1-9.7)](#9-初始化-rules-91-97)
- [§10 基本类型 (Rules 10.1-10.8)](#10-基本类型-rules-101-108)
- [§11 指针类型转换 (Rules 11.1-11.6, 11.8-11.11)](#11-指针类型转换-rules-111-116-118-1111)
- [§12 表达式 (Rules 12.1-12.6)](#12-表达式-rules-121-126)
- [§13 副作用 (Rules 13.1-13.6)](#13-副作用-rules-131-136)
- [§14 循环 (Rules 14.1-14.4)](#14-循环-rules-141-144)
- [§15 goto与选择 (Rules 15.1-15.7)](#15-goto与选择-rules-151-157)
- [§16 switch (Rules 16.1-16.7)](#16-switch-rules-161-167)
- [§17 函数 (Rules 17.1-17.5, 17.7-17.13)](#17-函数-rules-171-175-177-1713)
- [§18 指针与数组 (Rules 18.1-18.10)](#18-指针与数组-rules-181-1810)
- [§19 联合体 (Rules 19.1-19.3)](#19-联合体-rules-191-193)
- [§20 预处理 (Rules 20.1-20.15)](#20-预处理-rules-201-2015)
- [§21 标准库 (Rules 21.3-21.26)](#21-标准库-rules-213-2126)  <!-- 21.1, 21.2 永久删除/重新编号 -->
- [§22 资源管理 (Rules 22.1-22.20)](#22-资源管理-rules-221-2220)
- [§23 泛型选择 (Rules 23.1-23.8)](#23-泛型选择-rules-231-238)
- [Directives 指令清单](#directives-指令清单)

---

## §1 环境 (Rules 1.1, 1.3-1.5)  <!-- 1.2 永久删除 -->

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-1.1 | Required | Decidable | 程序不得违反标准C语法和约束，且不得超过实现的翻译限制 |
| Rule-1.3 | Required | Undecidable | 不得发生未定义或严重未指定行为 |
| Rule-1.4 | Required | Decidable | 不得使用新兴语言特性 |
| Rule-1.5 | Required | Undecidable | 不得使用废弃语言特性 |

## §2 未使用代码 (Rules 2.1-2.8)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-2.1 | Required | Undecidable | 项目不得包含不可达代码 |
| Rule-2.2 | Required | Undecidable | 项目不得包含死代码 |
| Rule-2.3 | Advisory | Decidable | 项目不应包含未使用的类型声明 |
| Rule-2.4 | Advisory | Decidable | 项目不应包含未使用的标签声明 |
| Rule-2.5 | Advisory | Decidable | 项目不应包含未使用的宏定义 |
| Rule-2.6 | Advisory | Decidable | 函数不应包含未使用的标签声明 |
| Rule-2.7 | Advisory | Decidable | 函数不应包含未使用的参数 |
| Rule-2.8 | Advisory | Decidable | 项目不应包含未使用的对象定义 |

## §3 注释 (Rules 3.1-3.2)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-3.1 | Required | Decidable | `/*` 和 `//` 字符序列不得出现在注释内 |
| Rule-3.2 | Required | Decidable | `//` 注释中不得使用行拼接 |

## §4 字符常量 (Rules 4.1-4.2)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-4.1 | Required | Decidable | 八进制和十六进制转义序列必须终止 |
| Rule-4.2 | Advisory | Decidable | 不应使用三字符组(trigraphs) |

## §5 标识符 (Rules 5.1-5.10)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-5.1 | Required | Decidable | 外部标识符必须互异 |
| Rule-5.2 | Required | Decidable | 同一作用域和命名空间中声明的标识符必须互异 |
| Rule-5.3 | Required | Decidable | 内部作用域声明的标识符不得隐藏外部作用域的标识符 |
| Rule-5.4 | Required | Decidable | 宏标识符必须互异 |
| Rule-5.5 | Required | Decidable | 标识符必须与宏名互异 |
| Rule-5.6 | Required | Decidable | typedef名必须是唯一标识符 |
| Rule-5.7 | Required | Decidable | 标签(tag)名必须是唯一标识符 |
| Rule-5.8 | Required | Decidable | 具有外部链接的对象或函数标识符必须唯一 |
| Rule-5.9 | Advisory | Decidable | 具有内部链接的标识符应唯一 |
| Rule-5.10 | Required | Decidable | 不得声明保留标识符或保留宏名 |

## §6 位域 (Rules 6.1-6.3)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-6.1 | Required | Decidable | 位域必须以适当类型声明 |
| Rule-6.2 | Required | Decidable | 单比特命名位域不得为有符号类型 |
| Rule-6.3 | Required | Decidable | 位域不得声明为联合体成员 |

## §7 字面量 (Rules 7.1-7.6)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-7.1 | Required | Decidable | 不得使用八进制常量 |
| Rule-7.2 | Required | Decidable | 所有表示为无符号类型的整数常量必须加 `u` 或 `U` 后缀 |
| Rule-7.3 | Required | Decidable | 字面量后缀中不得使用小写字符 `l` |
| Rule-7.4 | Required | Decidable | 字符串字面量不得赋给非 `pointer to const-qualified char` 类型的对象 |
| Rule-7.5 | **Mandatory** | Decidable | 整数常量宏的参数必须有适当形式 |
| Rule-7.6 | Required | Decidable | 不得使用最小宽度整数常量宏的小整数变体 |

## §8 声明与定义 (Rules 8.1-8.19)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-8.1 | Required | Decidable | 类型必须显式指定 |
| Rule-8.2 | Required | Decidable | 函数类型必须为带命名参数的原型形式 |
| Rule-8.3 | Required | Decidable | 对象或函数的所有声明必须使用相同的名称和类型限定符 |
| Rule-8.4 | Required | Decidable | 定义具有外部链接的对象或函数时，兼容声明必须可见 |
| Rule-8.5 | Required | Decidable | 外部对象或函数必须在一个且仅一个文件中声明一次 |
| Rule-8.6 | Required | Decidable | 具有外部链接的标识符必须恰好有一个外部定义 |
| Rule-8.7 | Advisory | Decidable | 仅在单个翻译单元中引用的函数和对象不应定义为外部链接 |
| Rule-8.8 | Required | Decidable | 具有内部链接的对象和函数的所有声明必须使用 `static` 存储类说明符 |
| Rule-8.9 | Advisory | Decidable | 若对象标识符仅在单个函数中出现，应在块作用域中声明 |
| Rule-8.10 | Required | Decidable | 内联函数必须以 `static` 存储类声明 |
| Rule-8.11 | Advisory | Decidable | 声明具有外部链接的数组时，应显式指定其大小 |
| Rule-8.12 | Required | Decidable | 枚举列表中隐式指定的枚举常量值必须唯一 |
| Rule-8.13 | Advisory | Undecidable | 只要可能，指针应指向 const 限定类型 |
| Rule-8.14 | Required | Decidable | 不得使用 `restrict` 类型限定符 |
| Rule-8.15 | Required | Decidable | 具有显式对齐规范的对象的所有声明必须指定相同的对齐方式 |
| Rule-8.16 | Advisory | Decidable | 对象声明中不应出现零对齐规范 |
| Rule-8.17 | Advisory | Decidable | 对象声明中最多应出现一个显式对齐说明符 |
| **Rule-8.18🆕** | **Required** | **Decidable** | **头文件中不得出现 tentative definition** |
| **Rule-8.19🆕** | **Advisory** | **Decidable** | **源文件中不应出现外部声明(`extern`)** |

## §9 初始化 (Rules 9.1-9.7)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-9.1 | **Mandatory** | Undecidable | 具有自动存储期的对象的值在设置前不得读取 |
| Rule-9.2 | Required | Decidable | 聚合体或联合体的初始化器必须用花括号括起 |
| Rule-9.3 | Required | Decidable | 数组不得部分初始化 |
| Rule-9.4 | Required | Decidable | 对象的元素不得多次初始化 |
| Rule-9.5 | Required | Decidable | 使用指定初始化器初始化数组对象时，必须显式指定数组大小 |
| Rule-9.6 | Required | Decidable | 使用链式指示符的初始化器中不得包含无指示符的初始化器 |
| Rule-9.7 | **Mandatory** | Undecidable | 原子对象在访问前必须适当初始化 |

## §10 基本类型 (Rules 10.1-10.8)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-10.1 | Required | Decidable | 操作数不得为不适当的基本类型 |
| Rule-10.2 | Required | Decidable | 基本字符类型的表达式不得不适当地用于加减运算 |
| Rule-10.3 | Required | Decidable | 表达式的值不得赋给具有更窄基本类型或不同基本类型类别的对象 |
| Rule-10.4 | Required | Decidable | 执行通常算术转换的运算符的两个操作数必须具有相同的基本类型类别 |
| Rule-10.5 | Advisory | Decidable | 表达式的值不应转换为不适当的基本类型 |
| Rule-10.6 | Required | Decidable | 复合表达式的值不得赋给具有更宽基本类型的对象 |
| Rule-10.7 | Required | Decidable | 若复合表达式用作执行通常算术转换的运算符的一个操作数，则另一操作数不得具有更宽的基本类型 |
| Rule-10.8 | Required | Decidable | 复合表达式的值不得转换为不同的基本类型类别或更宽的基本类型 |

## §11 指针类型转换 (Rules 11.1-11.6, 11.8-11.11)  <!-- 11.7 永久删除 -->

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-11.1 | Required | Decidable | 不得在函数指针与任何其他类型之间执行转换 |
| Rule-11.2 | Required | Decidable | 不得在不完整类型指针与任何其他类型之间执行转换 |
| Rule-11.3 | Required | Decidable | 不得在指向对象类型的指针与指向不同对象类型的指针之间执行转换 |
| Rule-11.4 | Required | Decidable | 不应在指向对象的指针与算术类型之间执行转换 |
| Rule-11.5 | Advisory | Decidable | 不应将 `void` 指针转换为对象指针 |
| Rule-11.6 | Required | Decidable | 不得在 `void` 指针与算术类型之间执行转换 |
| Rule-11.8 | Required | Decidable | 转换不得移除指针指向类型上的 `const`、`volatile` 或 `_Atomic` 限定 |
| Rule-11.9 | Required | Decidable | 宏 `NULL` 必须是整数空指针常量的唯一允许形式 |
| Rule-11.10 | Required | Decidable | `_Atomic` 限定符不得应用于不完整类型 `void` |
| **Rule-11.11🆕** | **Required** | **Decidable** | **指针不得隐式与 NULL 比较** |

## §12 表达式 (Rules 12.1-12.6)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-12.1 | Advisory | Decidable | 表达式中的运算符优先级应明确 |
| Rule-12.2 | Required | Undecidable | 移位运算符的右操作数必须介于0到左操作数基本类型位宽减1之间 |
| Rule-12.3 | Advisory | Decidable | 不应使用逗号运算符 |
| Rule-12.4 | Advisory | Decidable | 常量表达式的求值不应导致无符号整数回绕 |
| Rule-12.5 | **Mandatory** | Decidable | `sizeof` 运算符不得将声明为"array of type"的函数参数作为操作数 |
| Rule-12.6 | Required | Decidable | 不得直接访问原子对象的结构体和联合体成员 |

## §13 副作用 (Rules 13.1-13.6)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-13.1 | Required | Undecidable | 初始化器列表不得包含持久副作用 |
| Rule-13.2 | Required | Undecidable | 表达式的值及其持久副作用必须在所有允许的求值顺序下相同，且不得依赖线程交错 |
| Rule-13.3 | Advisory | Decidable | 包含 `++` 或 `--` 运算符的完整表达式不应有其他潜在副作用 |
| Rule-13.4 | Advisory | Decidable | 不应使用赋值运算符的结果 |
| Rule-13.5 | Required | Undecidable | 逻辑 `&&` 或 `||` 运算符的右操作数不得包含持久副作用 |
| Rule-13.6 | Required | Decidable | `sizeof` 运算符的操作数不得包含任何具有潜在副作用的表达式 |

## §14 循环 (Rules 14.1-14.4)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-14.1 | Required | Undecidable | 循环计数器不得具有基本浮点类型 |
| Rule-14.2 | Required | Undecidable | `for` 循环必须结构良好 |
| Rule-14.3 | Required | Undecidable | 控制表达式不得是不变的 |
| Rule-14.4 | Required | Decidable | `if` 语句和迭代语句的控制表达式必须具有基本布尔类型 |

## §15 goto与选择 (Rules 15.1-15.7)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-15.1 | Advisory | Decidable | 不应使用 `goto` 语句 |
| Rule-15.2 | Required | Decidable | `goto` 语句必须跳转到同一函数中稍后声明的标签 |
| Rule-15.3 | Required | Decidable | `goto` 语句引用的任何标签必须在同一块或包围 `goto` 语句的任何块中声明 |
| Rule-15.4 | Advisory | Decidable | 终止任何迭代语句的 `break` 或 `goto` 语句应不超过一个 |
| **Rule-15.5** | ~~Advisory~~ **Disapplied** | **Decidable** | **函数应在末尾有单一出口点** ⚠️ **已废止(Disapplied)** |
| Rule-15.6 | Required | Decidable | 迭代语句或选择语句的主体必须是复合语句 |
| Rule-15.7 | Required | Decidable | 所有 `if...else if` 构造必须以 `else` 语句终止 |

## §16 switch (Rules 16.1-16.7)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-16.1 | Required | Decidable | 所有 `switch` 语句必须结构良好 |
| Rule-16.2 | Required | Decidable | `switch` 标签只能用在最内层复合语句为 `switch` 语句体的情况下 |
| Rule-16.3 | Required | Decidable | 每个 `switch` 子句必须适当终止（允许 `return`/`abort()`/`exit()`） |
| Rule-16.4 | Required | Decidable | 每个 `switch` 语句必须有 `default` 标签 |
| Rule-16.5 | Required | Decidable | `default` 标签必须作为 `switch` 语句的第一个或最后一个标签出现 |
| Rule-16.6 | Required | Decidable | 每个 `switch` 语句必须至少有两个 `switch` 子句 |
| Rule-16.7 | Required | Decidable | `switch` 表达式不得具有基本布尔类型 |

## §17 函数 (Rules 17.1-17.5, 17.7-17.13)  <!-- 17.6 永久删除 -->

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-17.1 | Required | Decidable | 不得使用标准头文件 `<stdarg.h>` |
| Rule-17.2 | Required | Undecidable | 函数不得直接或间接调用自身（无递归） |
| Rule-17.3 | **Mandatory** | Decidable | 函数不得隐式声明 |
| Rule-17.4 | **Mandatory** | Decidable | 具有非 `void` 返回类型的函数的所有退出路径必须带有表达式的显式 `return` 语句 |
| Rule-17.5 | Required | Undecidable | 对应于声明为数组类型的参数的函数实参必须具有适当数量的元素 |
| Rule-17.7 | Required | Decidable | 具有非 `void` 返回类型的函数返回的值必须被使用 |
| Rule-17.8 | Advisory | Undecidable | 函数参数不应被修改 |
| Rule-17.9 | **Mandatory** | Undecidable | 以 `_Noreturn` 函数说明符声明的函数不得返回其调用者 |
| Rule-17.10 | Required | Decidable | 以 `_Noreturn` 函数说明符声明的函数必须具有 `void` 返回类型 |
| Rule-17.11 | Advisory | Undecidable | 从不返回的函数应以 `_Noreturn` 函数说明符声明 |
| Rule-17.12 | Advisory | Decidable | 函数标识符仅应与前置 `&` 或带括号的参数列表一起使用 |
| Rule-17.13 | Required | Decidable | 函数类型不得有类型限定 |

## §18 指针与数组 (Rules 18.1-18.10)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-18.1 | Required | Undecidable | 指针算术运算产生的指针必须寻址与该指针操作数相同的数组的元素 |
| Rule-18.2 | Required | Undecidable | 指针间的减法仅适用于寻址同一数组元素的指针 |
| Rule-18.3 | Required | Undecidable | 关系运算符 `>`、`>=`、`<` 和 `<=` 不得应用于指针类型表达式，除非它们指向同一对象 |
| Rule-18.4 | Advisory | Decidable | `+`、`-`、`+=` 和 `-=` 运算符不应应用于指针类型表达式 |
| Rule-18.5 | Advisory | Decidable | 声明中的指针嵌套不应超过两层 |
| Rule-18.6 | Required | Undecidable | 具有自动或线程局部存储期的对象的地址不得复制到在第一个对象失效后仍然存在的另一对象 |
| Rule-18.7 | Required | Decidable | 不得声明柔性数组成员 |
| Rule-18.8 | Required | Decidable | 不得使用变长数组(VLA) |
| Rule-18.9 | Required | Decidable | 具有临时生命周期的对象不得进行数组到指针的转换 |
| Rule-18.10 | **Mandatory** | Decidable | 不得使用指向可变修改数组类型的指针 |

## §19 联合体 (Rules 19.1-19.3)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-19.1 | **Mandatory** | Undecidable | 对象不得赋值或复制到重叠对象 |
| Rule-19.2 | Advisory | Decidable | 不应使用 `union` 关键字 |
| **Rule-19.3🆕** | **Required** | **Undecidable** | **联合体成员在设置前不得读取** |

## §20 预处理 (Rules 20.1-20.15)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-20.1 | Advisory | Decidable | `#include` 指令前只应有预处理指令或注释 |
| Rule-20.2 | Required | Decidable | 头文件名中不得出现 `'`、`"`、`\` 字符及 `/*` 或 `//` 字符序列 |
| Rule-20.3 | Required | Decidable | `#include` 指令后面必须跟 `<filename>` 或 `"filename"` 序列 |
| Rule-20.4 | Required | Decidable | 不得定义与关键字同名的宏 |
| Rule-20.5 | Advisory | Decidable | 不应使用 `#undef` |
| Rule-20.6 | Required | Decidable | 看似预处理指令的标记不得出现在宏实参中 |
| Rule-20.7 | Required | Decidable | 宏参数展开产生的表达式必须适当定界 |
| Rule-20.8 | Required | Decidable | `#if` 或 `#elif` 预处理指令的控制表达式必须求值为0或1 |
| Rule-20.9 | Required | Decidable | `#if` 或 `#elif` 预处理指令控制表达式中使用的所有标识符必须在求值前 `#define` |
| Rule-20.10 | Advisory | Decidable | 不应使用 `#` 和 `##` 预处理运算符 |
| Rule-20.11 | Required | Decidable | 紧跟 `#` 运算符的宏参数不得紧接着后面有 `##` 运算符 |
| Rule-20.12 | Required | Decidable | 用作 `#` 或 `##` 运算符操作数且本身会进行进一步宏替换的宏参数，仅应用作这些运算符的操作数 |
| Rule-20.13 | Required | Decidable | 第一个标记为 `#` 的行必须是有效的预处理指令 |
| Rule-20.14 | Required | Decidable | 所有 `#else`、`#elif` 和 `#endif` 预处理指令必须与它们所关联的 `#if`、`#ifdef` 或 `#ifndef` 指令位于同一文件中 |
| Rule-20.15 | Required | Decidable | 不得在保留标识符或保留宏名上使用 `#define` 和 `#undef` |

## §21 标准库 (Rules 21.3-21.26)  <!-- 21.1, 21.2 永久删除/重新编号 -->

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-21.3 | Required | Decidable | 不得使用 `<stdlib.h>` 的内存分配和释放函数 |
| Rule-21.4 | Required | Decidable | 不得使用标准头文件 `<setjmp.h>` |
| Rule-21.5 | Required | Decidable | 不得使用标准头文件 `<signal.h>` |
| Rule-21.6 | Required | Decidable | 不得使用标准库输入/输出函数 |
| Rule-21.7 | Required | Decidable | 不得使用 `<stdlib.h>` 的函数 `atof`、`atoi`、`atol`、`atoll` |
| Rule-21.8 | Required | Decidable | 不得使用 `<stdlib.h>` 的函数 `abort`、`exit`、`getenv`、`system` |
| Rule-21.9 | Required | Decidable | 不得使用 `<stdlib.h>` 的函数 `bsearch` 和 `qsort` |
| Rule-21.10 | Required | Decidable | 不得使用标准库时间和日期函数 |
| Rule-21.11 | Advisory | Decidable | 不应使用标准头文件 `<tgmath.h>` |
| Rule-21.12 | Required | Decidable | 不得使用标准头文件 `<fenv.h>` |
| Rule-21.13 | **Mandatory** | Undecidable | 传递给 `<ctype.h>` 函数的任何值必须可表示为 `unsigned char` 或为 `EOF` 值 |
| Rule-21.14 | Required | Undecidable | 不得使用标准库函数 `memcmp` 比较空终止字符串 |
| Rule-21.15 | Required | Decidable | 传递给标准库函数 `memcpy`、`memmove` 和 `memcmp` 的指针实参必须是指向限定或非限定版本的兼容类型的指针 |
| Rule-21.16 | Required | Decidable | 传递给标准库函数 `memcmp` 的指针实参必须指向指针类型、基本有符号类型、基本无符号类型、基本布尔类型或基本枚举类型 |
| Rule-21.17 | **Mandatory** | Undecidable | 使用 `<string.h>` 的字符串处理函数不得导致超出其指针参数引用的对象边界的访问 |
| Rule-21.18 | **Mandatory** | Undecidable | 传递给 `<string.h>` 任何函数的 `size_t` 实参必须具有适当的值 |
| Rule-21.19 | **Mandatory** | Undecidable | 标准库函数 `localeconv`、`getenv`、`setlocale` 或 `strerror` 返回的指针只能视为指向 const 限定类型的指针使用 |
| Rule-21.20 | **Mandatory** | Undecidable | 标准库函数 `asctime`、`ctime`、`gmtime`、`localtime`、`localeconv`、`getenv`、`setlocale` 或 `strerror` 返回的指针在后续调用同一函数后不得使用 |
| Rule-21.21 | Required | Decidable | 不得使用 `<stdlib.h>` 的 `system` |
| Rule-21.22 | **Mandatory** | Decidable | 传递给 `<tgmath.h>` 中声明的任何类型泛型宏的所有操作数实参必须具有适当的基本类型 |
| Rule-21.23 | Required | Decidable | 传递给 `<tgmath.h>` 中声明的任何多参数类型泛型宏的所有操作数实参必须具有相同的标准类型 |
| Rule-21.24 | Required | Decidable | 不得使用 `<stdlib.h>` 的随机数生成器函数 |
| Rule-21.25 | Required | Decidable | 所有内存同步操作必须以顺序一致顺序执行 |
| Rule-21.26 | Required | Undecidable | 标准库函数 `mtx_timedlock()` 仅应在适当互斥类型的互斥对象上调用 |

## §22 资源管理 (Rules 22.1-22.20)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-22.1 | Required | Undecidable | 通过标准库函数动态获取的所有资源必须显式释放 |
| Rule-22.2 | **Mandatory** | Undecidable | 内存块仅在其由标准库函数分配时才能被释放 |
| Rule-22.3 | Required | Undecidable | 同一文件不得同时在不同流上以读写访问方式打开 |
| Rule-22.4 | **Mandatory** | Undecidable | 不得尝试写入已以只读方式打开的流 |
| Rule-22.5 | **Mandatory** | Undecidable | 不得解引用指向 `FILE` 对象的指针 |
| Rule-22.6 | **Mandatory** | Undecidable | 在关联流关闭后，不得使用指向 `FILE` 的指针的值 |
| Rule-22.7 | Required | Undecidable | 宏 `EOF` 只能与任何能返回 `EOF` 的标准库函数的未修改返回值进行比较 |
| Rule-22.8 | Required | Undecidable | 在调用设置 `errno` 的函数之前，必须将 `errno` 的值置零 |
| Rule-22.9 | Required | Undecidable | 调用设置 `errno` 的函数后，必须检查 `errno` 的值是否为零 |
| Rule-22.10 | Required | Undecidable | 仅当最后调用的函数是设置 `errno` 的函数时，才应测试 `errno` 的值 |
| Rule-22.11 | Required | Undecidable | 先前已连接或分离的线程不得随后再连接或分离 |
| Rule-22.12 | **Mandatory** | Undecidable | 线程对象、线程同步对象和线程特定存储指针只能通过适当的标准库函数访问 |
| Rule-22.13 | Required | Decidable | 线程对象、线程同步对象和线程特定存储指针必须具有适当的存储期 |
| Rule-22.14 | **Mandatory** | Undecidable | 线程同步对象在访问前必须初始化 |
| Rule-22.15 | Required | Undecidable | 线程同步对象和线程特定存储指针在所有访问它们的线程终止前不得销毁 |
| Rule-22.16 | Required | Undecidable | 线程锁定的所有互斥对象必须由同一线程显式解锁 |
| Rule-22.17 | Required | Undecidable | 任何线程不得对尚未锁定的互斥对象调用解锁或 `cnd_wait()`/`cnd_timedwait()` |
| Rule-22.18 | Required | Undecidable | 非递归互斥锁不得递归锁定 |
| Rule-22.19 | Required | Undecidable | 条件变量最多与一个互斥对象关联 |
| Rule-22.20 | **Mandatory** | Undecidable | 线程特定存储指针在访问前必须创建 |

## §23 泛型选择 (Rules 23.1-23.8)

| ID | 分类 | 可分析性 | 描述 |
|----|------|----------|------|
| Rule-23.1 | Advisory | Decidable | 泛型选择仅应从宏展开 |
| Rule-23.2 | Required | Decidable | 非从宏展开的泛型选择不得在控制表达式中包含潜在副作用 |
| Rule-23.3 | Advisory | Decidable | 泛型选择应包含至少一个非默认关联 |
| Rule-23.4 | Required | Decidable | 泛型关联必须列出适当的类型 |
| Rule-23.5 | Advisory | Decidable | 泛型选择不应依赖隐式指针类型转换 |
| Rule-23.6 | Required | Decidable | 泛型选择的控制表达式的基本类型必须与其标准类型匹配 |
| Rule-23.7 | Advisory | Decidable | 从宏展开的泛型选择应仅对其参数求值一次 |
| Rule-23.8 | Required | Decidable | 默认关联必须作为泛型选择的第一个或最后一个关联出现 |

---

## Directives 指令清单

> 指令是"无法提供执行检查所需明确描述"的指南（MISRA C:2025 §6.1），理论上不可静态执行，但工具可提供辅助检查。

| ID | 描述 | 辅助类型 |
|----|------|----------|
| Dir-1.1 | 任何程序输出依赖的实现定义行为必须记录和理解 | Assisted |
| Dir-1.2 | 应最小化语言扩展的使用 | Assisted |
| Dir-2.1 | 所有源文件必须无编译错误编译 | ❌ Unassisted |
| Dir-3.1 | 所有代码必须可追溯到文档化需求 | ❌ Unassisted |
| Dir-4.1 | 运行时故障必须最小化 | Assisted |
| Dir-4.2 | 所有汇编语言的使用应记录 | Assisted |
| Dir-4.3 | 汇编语言必须封装和隔离 | Assisted |
| Dir-4.4 | 不应"注释掉"代码段 | Assisted |
| Dir-4.5 | 同一命名空间中具有重叠可见性的标识符应在类型上无歧义 | Assisted |
| Dir-4.6 | 应使用表示大小和符号的 typedef 替代基本整数类型 | Assisted |
| Dir-4.7 | 若函数返回错误信息，则必须测试该错误信息 | Assisted |
| Dir-4.8 | 若翻译单元中从未解引用指向结构体或联合体的指针，则应隐藏该对象的实现 | Assisted |
| Dir-4.9 | 在可互换的情况下，应优先使用函数而非函数式宏 | Assisted |
| Dir-4.10 | 必须采取预防措施防止头文件内容被多次包含 | Assisted |
| Dir-4.11 | 必须检查传递给库函数的值有效性 | ❌ Unassisted |
| Dir-4.12 | 不得使用动态内存分配 | Assisted |
| Dir-4.13 | 设计用于对资源执行操作的函数应按适当顺序调用 | Assisted |
| Dir-4.14 | 必须检查从外部接收的值有效性 | Assisted |
| Dir-4.15 | 浮点表达式的求值不得导致未检测到的无穷大和NaN生成 | ❌ Unassisted |
| Dir-5.1 | 线程间不得有数据竞争 | Assisted |
| Dir-5.2 | 线程间不得有死锁 | Assisted |
| Dir-5.3 | 不得有动态线程创建 | ❌ Unassisted |

---

> **版权声明**: 本文档基于 Perforce QAC for C 2026.2 Rule Enforcement Summary + LDRA MISRA C:2025 分类统计整理。MISRA、MISRA C、MISRA C++ 是 The MISRA Consortium Limited 的注册商标。完整标准文档可从 [misra.org.uk](https://misra.org.uk/product/misra-c2025/) 购买，售价 £15/授权。
> 
> **注**: Perforce 博客称"225条活跃Guidelines"，与 LDRA（其技术专家 Andrew Banks 担任 MISRA C Working Group 主席）所述 223 条相差 2 条。本文档以 LDRA 的 223 条（201 Rules + 22 Directives）为准。Perforce 工具中"新增 5 条"的第 5 条推测为 1 条新增 Directive（文档 22 条 Directive 列表中尚未标出 🆕），并非 Perforce 内部编造。
