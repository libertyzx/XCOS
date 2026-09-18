/* A22 回归测试: 协程嵌套配置的**初始化职责**(注册入口 + 分步注册)
 * ===========================================================================
 * 背景(V2.1.1 复核 #10/A22): "帧栈/容量"(`pCorStack`/`CorDepthMax`)是**用户配置**, 若不落成确定值,
 *   未清零 TCB 上的垃圾指针会被 `XC_Cor_Call`(PushFrame) 当真 ⇒ 野指针写;
 *   而"配置落点"必须与实现顺序一致:
 *     ① `XC_Task_Reg`        : 无嵌套入口 ⇒ 清空 `pCorStack`/`CorDepthMax`;
 *     ② `XC_Task_RegExt`     : **先 Reg 成功、再写帧栈** ⇒ 配置必须被保留(回归点: 顺序写反会被静默抹掉);
 *     ③ 分步注册             : `XC_Task_SetEntry`(未注册时清零) → `XC_Task_SetCorStack` → `XC_Task_Add`;
 *     ④ `CorDepth`/`CorState` 由 `BasicInit` 统一复位(Add/Remove/Reset 都走它), 不属"用户配置"。
 * 判定: 5 组断言共 16 项 —— 注册入口的保留/清零、分步注册的顺序、垃圾 TCB 的两种表现
 *       (整块未清零 ⇒ 注册被拒[文档契约]; 仅帧栈是垃圾 ⇒ 被 SetEntry 清掉后调用 `Call` 安全降级)。
 * 预期: 修复前 FAIL(RegExt 的帧栈被 Reg 清掉 / 分步注册保留垃圾), 修复后 PASS。
 * ---------------------------------------------------------------------------
 * 编译: gcc -std=gnu99 -O1 -Wall -Wextra -I Code\Inc -o a22.exe Code\Src\XC_*.c <本文件>
 */
#include <stdio.h>
#include <string.h>

#include "XCOS.h"
#include "Internal/XC_TaskInternal.h"
#include "Internal/XC_TypeInternal.h"

#if (XC_CFG_COR_NESTING == 0)
#error "本用例需要协程嵌套能力(默认 XC_CFG_COR_NESTING=1; Tests/manifest.txt 不加开关)"
#endif

static int g_fail = 0;

#define CHK(cond, msg)                     \
    do {                                   \
        if(cond) {                         \
            printf("  PASS  %s\n", (msg)); \
        } else {                           \
            printf("  FAIL  %s\n", (msg)); \
            g_fail++;                      \
        }                                  \
    } while(0)

/** 任务体: 协程块(只为占位, 不实际运行) */
static void TaskNested(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) {
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

/** ① RegExt: 注册成功且帧栈配置**未被** Reg 清掉 */
static void CaseRegExt(void)
{
    static XCOS_t        s_os;
    static XC_TaskCB_t   s_tcb;
    static XC_CorFrame_t s_stk[3];

    memset(&s_os, 0, sizeof(s_os));
    memset(&s_tcb, 0, sizeof(s_tcb));
    XC_Sch_Init(&s_os);

    printf("-- ① RegExt(先 Reg 再写帧栈)\n");
    CHK(XC_Task_RegExt(&s_os, &s_tcb, TaskNested, NULL, s_stk, 3U) == XC_OK, "RegExt 注册成功");
    CHK(s_tcb.pCorStack == s_stk, "帧栈**未被** XC_Task_Reg 清掉(回归点)");
    CHK(s_tcb.CorDepthMax == 3U, "栈容量保留(3)");
    CHK(s_tcb.CorDepth == 0U, "深度为 0");
    CHK(s_tcb.CorState == (uint8_t)XC_COR_SUSPENDED, "本轮结果复位为\"未完成\"");
}

/** ② Reg: 无嵌套入口 ⇒ 帧栈/容量清成 NULL/0 */
static void CaseReg(void)
{
    static XCOS_t      s_os;
    static XC_TaskCB_t s_tcb;

    memset(&s_os, 0, sizeof(s_os));
    memset(&s_tcb, 0, sizeof(s_tcb));
    s_tcb.pCorStack   = (XC_CorFrame_t*)0x1234U; /* 脏配置 */
    s_tcb.CorDepthMax = 0x37U;
    XC_Sch_Init(&s_os);

    printf("-- ② Reg(无嵌套入口)\n");
    CHK(XC_Task_Reg(&s_os, &s_tcb, TaskNested, NULL) == XC_OK, "Reg 注册成功");
    CHK(s_tcb.pCorStack == NULL, "脏帧栈被清成 NULL");
    CHK(s_tcb.CorDepthMax == 0U, "脏容量被清成 0");
}

/** ③ 分步注册: SetEntry(未注册时清零) → SetCorStack → Add, 配置被保留 */
static void CaseStepwise(void)
{
    static XCOS_t        s_os;
    static XC_TaskCB_t   s_tcb;
    static XC_CorFrame_t s_stk[2];

    memset(&s_os, 0, sizeof(s_os));
    memset(&s_tcb, 0, sizeof(s_tcb));
    s_tcb.pCorStack   = (XC_CorFrame_t*)0x5678U; /* 脏配置 */
    s_tcb.CorDepthMax = 0x40U;
    XC_Sch_Init(&s_os);

    printf("-- ③ 分步注册(SetEntry → SetCorStack → Add)\n");
    XC_Task_SetEntry(&s_tcb, TaskNested, NULL);
    CHK((s_tcb.pCorStack == NULL) && (s_tcb.CorDepthMax == 0U), "SetEntry(未注册)把脏配置清成 NULL/0");
    CHK(XC_Task_SetCorStack(&s_tcb, s_stk, 2U) == XC_OK, "SetCorStack 配置(栈/容量=2)");
    CHK(XC_Task_Add(&s_os, &s_tcb) == XC_OK, "Add 添加成功");
    CHK((s_tcb.pCorStack == s_stk) && (s_tcb.CorDepthMax == 2U), "Add 后配置仍保留");
    CHK(s_tcb.CorDepth == 0U, "Add 复位深度为 0");
}

/** ④ 垃圾 TCB: 整块未清零 ⇒ 注册被拒(文档契约); 仅帧栈是垃圾 ⇒ 被清掉后调用 Call 安全降级 */
static void CaseGarbage(void)
{
    static XCOS_t        s_os;
    static XC_TaskCB_t   s_junk;
    static XC_TaskCB_t   s_tcb;
    static XC_CorFrame_t s_stk[1];

    memset(&s_os, 0, sizeof(s_os));
    XC_Sch_Init(&s_os);

    printf("-- ④ 垃圾 TCB 的两种表现\n");
    memset(&s_junk, 0xA5, sizeof(s_junk)); /* 整块未清零(phXCOS 也是垃圾) */
    CHK(XC_Task_Reg(&s_os, &s_junk, TaskNested, NULL) == XC_FAIL, "整块未清零 ⇒ Reg 被拒(XC_FAIL, 文档契约)");
    CHK(XC_Task_Add(&s_os, &s_junk) == XC_FAIL, "整块未清零 ⇒ Add 被拒(同上)");

    memset(&s_tcb, 0, sizeof(s_tcb));
    s_tcb.pCorStack   = (XC_CorFrame_t*)0xDEADBEEFU; /* 只有帧栈是垃圾 */
    s_tcb.CorDepthMax = 0x7FU;
    XC_Task_SetEntry(&s_tcb, TaskNested, NULL);
    CHK(XC_Task_Add(&s_os, &s_tcb) == XC_OK, "已清零 TCB + 脏帧栈 ⇒ 可注册(SetEntry 已清)");
    XC_Task_PushFrame(&s_tcb, TaskNested, (XC_BP_t)0); /* 未配帧栈调用 Call ⇒ 必须安全降级 */
    CHK(s_tcb.CorDepth == 0U, "PushFrame 走安全降级(未被计入嵌套)");
    CHK(s_tcb.TaskState == (uint8_t)XC_TASK_READY, "任务被复位为就绪(而非写野指针)");
    (void)s_stk;
}

/** ⑤ 已注册任务换入口: 嵌套能力保留(不动帧栈) */
static void CaseChangeEntry(void)
{
    static XCOS_t        s_os;
    static XC_TaskCB_t   s_tcb;
    static XC_CorFrame_t s_stk[3];

    memset(&s_os, 0, sizeof(s_os));
    memset(&s_tcb, 0, sizeof(s_tcb));
    XC_Sch_Init(&s_os);
    (void)XC_Task_RegExt(&s_os, &s_tcb, TaskNested, NULL, s_stk, 3U);

    printf("-- ⑤ 已注册任务换入口\n");
    XC_Task_SetEntry(&s_tcb, TaskNested, NULL);
    CHK((s_tcb.pCorStack == s_stk) && (s_tcb.CorDepthMax == 3U), "换入口不动帧栈(嵌套能力保留)");
}

int main(void)
{
    printf("=== A22: 嵌套配置的初始化职责 ===\n");
    CaseRegExt();
    CaseReg();
    CaseStepwise();
    CaseGarbage();
    CaseChangeEntry();

    printf("\n===== A22 用例完成: failures %d =====\n", g_fail);
    return ((g_fail == 0) ? 0 : 1);
}
