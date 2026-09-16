/* B3 验证用例: 运行期不变量自检(XC_Diag_CheckInvariants)
 *  - 关(XC_CFG_DEBUG_CHECK=0): 只验证"开关关闭时工程照常编译/链接"(无自检符号依赖);
 *  - 开(XC_CFG_DEBUG_CHECK=1): 跑 6 组检查(正常 + 5 种注入), 并验证自检"只读不写";
 * 编译: gcc -std=gnu99 -O1 -Wall -Wextra [-DXC_CFG_DEBUG_CHECK=1] -I Code/Inc \
 *        Code/Src/XC_List.c Code/Src/XC_Sch.c Code/Src/XC_Task.c Code/Src/XC_Time.c \
 *        <本文件> -o b3.exe
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

#if(XC_CFG_DEBUG_CHECK != 0)

static XCOS_t      g_os;
static XC_TaskCB_t g_ready; /* 就绪任务 */
static XC_TaskCB_t g_delay; /* 延时任务(时间表 + BLOCKED) */
static XC_TaskCB_t g_susp;  /* 挂起任务(阻塞表 + SUSPEND) */

/** 任务体(本用例手工驱动, 不启动调度器) */
static void TaskBody(XC_TaskHandle_t phTCB)
{
    XC_Cor_Enter(phTCB);
    XC_Cor_Leave();
}

/** 组装一个"全部一致"的现场 */
static void Setup(void)
{
    (void)memset(&g_os, 0, sizeof(g_os));
    (void)memset(&g_ready, 0, sizeof(g_ready));
    (void)memset(&g_delay, 0, sizeof(g_delay));
    (void)memset(&g_susp, 0, sizeof(g_susp));

    XC_Sch_Init(&g_os);
    (void)XC_Task_Reg(&g_os, &g_ready, TaskBody, NULL); /* READY + 就绪表 */
    (void)XC_Task_Reg(&g_os, &g_delay, TaskBody, NULL);
    (void)XC_Task_Reg(&g_os, &g_susp, TaskBody, NULL);
    XC_Task_HandleDelay(&g_delay, 100U);  /* BLOCKED + 时间表 */
    XC_Task_HandleSuspend(&g_susp);       /* SUSPEND + 阻塞表 */
}
#endif

int main(void)
{
    printf("== B3 用例: XC_CFG_DEBUG_CHECK = %u ==\n", (unsigned)XC_CFG_DEBUG_CHECK);

#if(XC_CFG_DEBUG_CHECK == 0)
    /** 关闭档: 自检不编译, 只要能正常编译/链接/运行即通过 */
    printf("  [info] 开关关闭: 自检实现不编译(0 代码/0 RAM), 本档只验证工程照常构建\n");
    printf("\n===== B3 用例完成: failures %d =====\n", g_fail);
    return ((g_fail == 0) ? 0 : 1);
#else
    /** 第 0 组: 正常现场 => 自检必须通过, 且"只读"(调用前后状态/计数不变) */
    Setup();
    {
        uint8_t  st0   = g_ready.TaskState;
        uint8_t  num0  = g_os.TaskNum;
        uint8_t  deg0  = g_delay.CorDepth;

        CHK(XC_Diag_CheckInvariants(&g_os) == XC_CHK_OK, "正常现场: 自检 == XC_CHK_OK");
        CHK(XC_Sch_GetTaskNum(&g_os) == 3U, "正常现场: TaskNum == 3");
        CHK((g_ready.TaskState == st0) && (g_os.TaskNum == num0) && (g_delay.CorDepth == deg0),
            "自检只读: 调用前后状态/计数/深度不变");
    }

    /** 第 1 组: 就绪表内任务状态被写错 => STATE_TABLE */
    Setup();
    g_ready.TaskState = (uint8_t)XC_TASK_BLOCKED; /* 还在就绪表, 却标成 BLOCKED */
    CHK(XC_Diag_CheckInvariants(&g_os) == XC_CHK_STATE_TABLE, "注入(就绪表状态不符): STATE_TABLE");

    /** 第 2 组: 时间表内任务标成 READY => STATE_TABLE */
    Setup();
    g_delay.TaskState = (uint8_t)XC_TASK_READY; /* 还在时间表, 却标成 READY(僵尸状态) */
    CHK(XC_Diag_CheckInvariants(&g_os) == XC_CHK_STATE_TABLE, "注入(僵尸状态): STATE_TABLE");

    /** 第 3 组: TaskNum 与四表节点总数不符 => TASKNUM */
    Setup();
    g_os.TaskNum = (uint8_t)(g_os.TaskNum - 1U);
    CHK(XC_Diag_CheckInvariants(&g_os) == XC_CHK_TASKNUM, "注入(TaskNum 不符): TASKNUM");

    /** 第 4 组: 嵌套不变量被破坏(声明了容量却无帧栈) => NESTING */
    Setup();
    g_ready.CorDepthMax = 2U; /* pCorStack 仍为 NULL => 两者不匹配 */
    CHK(XC_Diag_CheckInvariants(&g_os) == XC_CHK_NESTING, "注入(有容量无帧栈): NESTING");

    /** 第 5 组: 表内任务的 phXCOS 不是本框架实例 => HANDLE */
    Setup();
    g_delay.phXCOS = NULL;
    CHK(XC_Diag_CheckInvariants(&g_os) == XC_CHK_HANDLE, "注入(任务归属错): HANDLE");

    /** 第 6 组: 表成环(节点自指) => LINK_BROKEN */
    Setup();
    g_ready.ListNode.pNext = &g_ready.ListNode; /* 环: 遍历步数超上限必须被拦截(不能死循环) */
    g_ready.ListNode.pPrev = &g_ready.ListNode;
    CHK(XC_Diag_CheckInvariants(&g_os) == XC_CHK_LINK_BROKEN, "注入(成环): LINK_BROKEN(有步数上限, 不死循环)");

    printf("\n===== B3 用例完成: failures %d =====\n", g_fail);
    return ((g_fail == 0) ? 0 : 1);
#endif
}
