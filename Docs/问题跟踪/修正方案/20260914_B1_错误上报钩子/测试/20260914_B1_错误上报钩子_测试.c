/* B1 验证用例: 帧栈越界 -> 错误钩子上报(宏开关) + 原有"静默复位"行为不变
 * 钩子关: 编译/链接不需要实现 XC_Err_Hook
 * 钩子开: -DXC_CFG_ERR_HOOK=1, 必须实现 XC_Err_Hook
 * 2026-09-15: 按"单一强制点"规则, 参数合法性只由断言承担(不再有 `return XC_FAIL` 的重复检查)
 *             ⇒ 本用例中"参数不一致 ⇒ XC_FAIL"的期望已更新(见文件末尾注释)
 */
#include <stdio.h>
#include <string.h>

#include "XCOS.h"

static int g_fail = 0;

#define CHK(cond, msg)                          \
    do {                                        \
        if(cond) {                              \
            printf("  PASS  %s\n", (msg));      \
        } else {                                \
            printf("  FAIL  %s\n", (msg));      \
            g_fail++;                           \
        }                                       \
    } while(0)

static XCOS_t      g_os;
static XC_TaskCB_t g_tcb;
static int         g_topRuns = 0;
static int         g_subRuns = 0;

#if(XC_CFG_ERR_HOOK != 0)
static int             g_hookCalls    = 0;
static XC_ErrCode_t    g_hookCode     = XC_ERR_NONE;
static XC_TaskHandle_t g_hookTCB      = NULL;
static uint8_t         g_hookDepth    = 0U;
static uint8_t         g_hookDepthMax = 0U;

/**
 * 用户实现的上报钩子(开关打开时必须提供)
 */
void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode)
{
    g_hookCalls++;
    g_hookCode = ErrCode;
    g_hookTCB  = phTCB;
    /** 注意: 断言(`XC_ERR_ASSERT`)场景 phTCB 为 NULL(无任务上下文) ⇒ 取现场前必须判空 */
    if(phTCB != NULL) {
        g_hookDepth    = phTCB->CorDepth;    /* 现场: 越界时的深度 */
        g_hookDepthMax = phTCB->CorDepthMax; /* 现场: 栈容量(0 = 未配栈) */
    }
}
#endif

static void SubTask(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    g_subRuns++;
    printf("    [sub] 被进入(不应出现: 越界即复位)\n");
    XC_Cor_Leave();
}

static void TopTask(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    g_topRuns++;
    printf("    [top] 第 %d 次进入\n", g_topRuns);
    if(g_topRuns == 1) {
        XC_Cor_Call(SubTask); /* 未配帧栈 => 越界分支(上报 + 复位) */
        printf("    [top] Call 之后(越界时不可达)\n");
    }
    XC_Cor_Leave();
}

int main(void)
{
    printf("== B1 用例: XC_CFG_ERR_HOOK = %u ==\n", (unsigned)XC_CFG_ERR_HOOK);
    memset(&g_os, 0, sizeof(g_os));
    memset(&g_tcb, 0, sizeof(g_tcb));

    XC_Sch_Init(&g_os);
    CHK(XC_Task_Reg(&g_os, &g_tcb, TopTask, NULL) == XC_OK, "注册任务(不配帧栈)");
    CHK((g_tcb.CorDepthMax == 0U) && (g_tcb.pCorStack == NULL), "确认未配帧栈(CorDepthMax=0)");

    /* 第 1 次调度: Top 内调用 XC_Cor_Call => 越界 => 上报 + 复位降级 */
    g_tcb.fTask(&g_tcb);

    CHK(g_topRuns == 1, "越界后本轮结束(未进入子协程)");
    CHK(g_subRuns == 0, "子协程未被执行");
    CHK(g_tcb.CorDepth == 0U, "复位后 CorDepth == 0");
    CHK(g_tcb.TaskState == (uint8_t)XC_TASK_READY, "复位后 TaskState == READY");
    CHK(g_tcb.ListNode.pNext != &g_tcb.ListNode, "复位后任务在就绪表(非游离)");
    CHK(XC_List_ListValid(&g_os.ReadyList) != 0, "框架就绪表非空");

#if(XC_CFG_ERR_HOOK != 0)
    CHK(g_hookCalls == 1, "钩子被调用 1 次");
    CHK(g_hookCode == XC_ERR_FRAME_OVERFLOW, "错误码 == XC_ERR_FRAME_OVERFLOW");
    CHK(g_hookTCB == &g_tcb, "钩子收到正确的 TCB");
    CHK((g_hookDepth == 0U) && (g_hookDepthMax == 0U), "钩子内现场可见: 0 >= 0 (未配栈)");
#else
    printf("    [info] 钩子关闭: 无上报调用(与改前行为一致), 链接不需 XC_Err_Hook\n");
#endif

    /* 第 2 次调度: 验证"静默复位"的原表现仍在(任务从头运行) */
    g_tcb.fTask(&g_tcb);
    CHK(g_topRuns == 2, "任务从头再次运行(复位表现保持)");

/*
 * 接口入口断言(落点验证) —— 按"单一强制点"规则(2026-09-15):
 *   参数合法性属**前置契约**(违背 = 未定义行为), 只由断言承担, **不再有** "return XC_FAIL" 的重复检查;
 *   ⇒ 断言档: 会真的把非法参数写进 TCB, 故验证后**必须复原**;
 *     关闭档: **不得**使用非法参数(无检查 ⇒ 会留下损坏的 TCB), 只用合法参数验证返回值。
 */
#if(XC_CFG_ASSERT != 0)
    /** 断言(B1 之外的新增能力): XC_DIAG_ASSERT 经同一钩子上报 XC_ERR_ASSERT */
    g_hookCalls = 0;
    g_hookCode  = XC_ERR_NONE;
    g_hookTCB   = &g_tcb;
    XC_DIAG_ASSERT(1); /* 条件为真 ⇒ 不上报 */
    CHK(g_hookCalls == 0, "断言(条件为真): 不上报");
    XC_DIAG_ASSERT(0); /* 条件为假 ⇒ 上报 */
    CHK(g_hookCalls == 1, "断言(条件为假): 钩子被调用 1 次");
    CHK(g_hookCode == XC_ERR_ASSERT, "错误码 == XC_ERR_ASSERT");
    CHK(g_hookTCB == NULL, "断言上报的 phTCB == NULL");

    /** 接口内断言落点(内核 API 入口): 违背前置契约(帧栈/容量不一致) ⇒ 断言上报, 且**不改变控制流** */
    g_hookCalls = 0;
    g_hookCode  = XC_ERR_NONE;
    g_hookTCB   = &g_tcb;
    (void)XC_Task_SetCorStack(&g_tcb, NULL, 2U); /* 非法组合: 断言上报(不再返回 FAIL) */
    CHK(g_hookCalls == 1, "接口断言: 钩子被调用 1 次");
    CHK(g_hookCode == XC_ERR_ASSERT, "接口断言: 错误码 == XC_ERR_ASSERT");
    CHK(g_tcb.CorDepthMax == 2U, "接口断言: 只上报不改控制流(参数照写)");
    /** 复原 TCB(非法参数已写坏配置, 必须撤销嵌套能力) */
    CHK(XC_Task_SetCorStack(&g_tcb, NULL, 0U) == XC_OK, "复原: (NULL, 0) 撤销嵌套能力 ⇒ XC_OK");
    CHK((g_tcb.CorDepthMax == 0U) && (g_tcb.pCorStack == NULL), "复原后 TCB 帧栈配置为空");
#else
    printf("    [info] 断言开关关闭(XC_CFG_ASSERT=0): 参数合法性不再检查(前置契约, 违背=UB) ⇒ 本段只用合法参数\n");
    CHK(XC_Task_SetCorStack(&g_tcb, NULL, 0U) == XC_OK, "合法参数(撤销嵌套能力) ⇒ XC_OK");
#endif

    printf("\n===== B1 用例完成: failures %d =====\n", g_fail);
    return (g_fail == 0) ? 0 : 1;
}
