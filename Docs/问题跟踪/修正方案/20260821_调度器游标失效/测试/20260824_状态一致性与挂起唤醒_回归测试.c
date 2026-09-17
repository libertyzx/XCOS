/**
 * @file  regress_full.c
 * @brief 回归测试: 延时/通知/挂起恢复/嵌套同轮切父/顶层DONE, 验证修复方案无副作用
 * 分别链接原内核与修复后内核运行, 结果应一致(核心功能不破坏)。
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "XCOS.h"
#include "XC_Cor.h"
#include "Internal/XC_Core.h"


/* ===== 测试自我超时看门狗(防止程序卡死; C标准 clock(), 全平台可编译) ===== */
#include <time.h>
static clock_t s_WD_Start = 0;
static clock_t s_WD_TimeoutMs = 30000;
static const char* s_WD_Tag = "test";
static void WD_Init(clock_t timeoutMs, const char* tag)
{
    s_WD_Start = clock();
    s_WD_TimeoutMs = timeoutMs;
    s_WD_Tag = tag;
}
static void WD_Check(void)
{
    if((clock() - s_WD_Start) >= (clock_t)(s_WD_TimeoutMs * (clock_t)CLOCKS_PER_SEC / 1000)) {
        printf("[WATCHDOG] %s: timeout (>%u ms), kill!\n", s_WD_Tag, (unsigned)s_WD_TimeoutMs);
        fflush(stdout);
        exit(2);
    }
}

XCOS_t g_xc;
volatile int g_stop = 0;

/* A0: 延时调度 */
static XC_TaskCB_t s_A0;
static int s_A0c = 0;
void Task_A0(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) { s_A0c++; XC_Cor_DelayMs(2); }
    XC_Cor_Leave();
}

/* A1: 通知唤醒(A1Send 定时发通知) */
static XC_TaskCB_t s_A1, s_A1Send;
static int s_A1c = 0, s_A1Timeout = 0;
void Task_A1Send(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) { XC_Task_SendNotify(&s_A1, NULL); XC_Cor_DelayMs(2); }
    XC_Cor_Leave();
}
void Task_A1(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) {
        XC_Cor_ClrNotify();
        XC_Cor_WaitNotifyMs(0);                       /* 死等, 由 SendNotify 唤醒 */
        if(XC_Cor_IsNotifyTimeout()) { s_A1Timeout++; }
        else { s_A1c++; }
        XC_Cor_DelayMs(1);
    }
    XC_Cor_Leave();
}

/* A3/A4: 挂起/恢复 */
static XC_TaskCB_t s_A3, s_A4;
static int s_A3c = 0, s_A4c = 0;
void Task_A4(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) { s_A4c++; XC_Cor_Yield(); }
    XC_Cor_Leave();
}
void Task_A3(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) {
        s_A3c++;
        if(s_A3c == 1) { XC_Task_Suspend(&s_A4); }
        if(s_A3c == 2) { XC_Task_Resume(&s_A4); }
        XC_Cor_DelayMs(1);
    }
    XC_Cor_Leave();
}

/* A10: 嵌套 Call 同轮切父 */
static XC_TaskCB_t s_A10;
static XC_CorFrame_t s_A10Stack[2];
static int s_A10Parent = 0, s_A10Sub = 0;
void Task_Sub10(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    s_A10Sub++;
    XC_Cor_Leave();
}
void Task_A10(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) {
        s_A10Parent++;
        XC_Cor_Call(Task_Sub10);
        XC_Cor_DelayMs(2);
    }
    XC_Cor_Leave();
}

/* A5: 有限循环任务 -> 顶层 DONE 后状态应回落 READY */
static XC_TaskCB_t s_A5;
static int s_A5c = 0, s_A5Done = 0;
void Task_A5(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) {
        if(s_A5c >= 40) { s_A5Done = 1; break; }  /* 40 tick 后退出循环 -> Leave(DONE) */
        s_A5c++;
        XC_Cor_DelayMs(1);                        /* 每 tick 运行一次, 给其他任务调度机会 */
    }
    XC_Cor_Leave();
}

/* Done: 汇总检查并退出 */
static XC_TaskCB_t s_Done;
void Task_Done(XC_TaskHandle_t t)
{
    XC_Cor_Enter(t);
    while(1) {
        WD_Check();
        if(s_A5Done) {
            printf("== Regression (kernel) ==\n");
            printf("A0 delay       = %d (>0)\n", s_A0c);
            printf("A1 notify      = %d (>0), timeout=%d (=0)\n", s_A1c, s_A1Timeout);
            printf("A3/A4 suspend  = A3=%d A4=%d (>0)\n", s_A3c, s_A4c);
            printf("A10 nested     = Parent=%d Sub=%d (Sub>=Parent, >0)\n", s_A10Parent, s_A10Sub);
            printf("A5 top-DONE    = c=%d state=%d (expect READY=2)\n", s_A5c, (int)XC_Task_GetState(&s_A5));
            g_stop = 1;
            exit(0);
        }
        XC_Cor_Yield();
    }
    XC_Cor_Leave();
}

DWORD WINAPI TickThread(LPVOID p)
{
    (void)p;
    while(!g_stop) { WD_Check(); XC_Time_TickInc(); Sleep(1); }
    return 0;
}

int main(void)
{
    WD_Init(30000, "回归测试");
    XC_Sch_Init(&g_xc);
    CreateThread(NULL, 0, TickThread, NULL, 0, NULL);
    XC_Task_Reg(&g_xc, &s_A5, Task_A5, NULL);
    XC_Task_Reg(&g_xc, &s_A0, Task_A0, NULL);
    XC_Task_Reg(&g_xc, &s_A1Send, Task_A1Send, NULL);
    XC_Task_Reg(&g_xc, &s_A1, Task_A1, NULL);
    XC_Task_Reg(&g_xc, &s_A4, Task_A4, NULL);
    XC_Task_Reg(&g_xc, &s_A3, Task_A3, NULL);
    XC_Task_RegExt(&g_xc, &s_A10, Task_A10, NULL, s_A10Stack, 2U);
    XC_Task_Reg(&g_xc, &s_Done, Task_Done, NULL);
    XC_Sch_Start(&g_xc);
    return 0;
}
