/*=========================================================|
 | 文件名:  XC_Sch.c
 | 描述:    调度器实现
 | 版本:    V1.00
 | 日期:    2024/03/18
 | 语言:    C语言
 | 作者:    libertyzx
 | E-mail:  libertyzx@163.com
 +-----------------------------------------------|
 | 开源协议: MIT License
 +-----------------------------------------------|
 +--- 说明
 |  用于调度任务;
 +-----------------------------------------------|
 +--- 版本说明:
 |  V1.00:-2024/03/18
 |      1.初始化
 *========================================================*/
//=== 头文件
#include "XC_Sch.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 以下私有函数

/************************************************ 我是分割线 ************************************************/
/**任务链表节点处理*/

/************************************************|
 * 描述:    链表节点移除
 * 函数名:  XCSch_ListNodeRemove
 * 形参[I]: XCOS_t* phXCOS      //XCOS句柄
 * 形参[I]: XCTCB_t* phXCTCB    //协程控制块
 * 返回:    void
 * 说明:
 *  处理"XCOS"索引,并移除节点;
 *  适用"XCOS"下所以链表的节点;
 * 例程:    无
 ************************************************/
void XCSch_ListNodeRemove(XCOS_t* phXCOS, XCTCB_t* phXCTCB)
{
    //若移除的是索引指向的节点,则将索引指向上个节点
    if(phXCOS->pReadyListNodeIndex == &phXCTCB->ListNode){
        phXCOS->pReadyListNodeIndex = phXCTCB->ListNode.pPrevious;
    }
    XCList_Remove(&phXCTCB->ListNode);      //移除节点
}

/************************************************|
 * 描述:    将节点插入索引前
 * 函数名:  XCSch_ListNodeInsertIndexPrevious
 * 形参[I]: XCOS_t* phXCOS      //XCOS句柄
 * 形参[I]: XCTCB_t* phXCTCB    //协程控制块
 * 返回:    void
 * 说明:    注意,索引指向的是就绪表,所以节点是插入就绪表的;
 * 例程:    无
 ************************************************/
void XCSch_ListNodeInsertIndexPrevious(XCOS_t* phXCOS, XCTCB_t* phXCTCB)
{
    XCListNode_t* pIndex;
    XCListNode_t* pNewNode;

    pNewNode = (XCListNode_t*)phXCTCB;

    pIndex = phXCOS->pReadyListNodeIndex;       //当前索引节点
    XCList_InsertPrevious(pIndex, pNewNode);    //插入索引节点前
    pNewNode->pRootList = &phXCOS->ReadyList;   //插在就绪表
    phXCOS->ReadyList.ListNodeNum++;            //就绪表节点+1
}

/************************************************ 我是分割线 ************************************************/
//调度处理

/************************************************|
 * 描述:    [私有]时间调度
 * 函数名:  XCSch_TimeSched
 * 形参[I]: XCOS_t* phXCOS      //XCOS句柄
 * 形参[I]: XCuint_t Tick       //当前的Tick
 * 返回:    void
 * 说明:
 *  判断"TimeList"或"TimeOverflowList"表中时间是否已经到达;
 *  时间到达则转入"ReadyList";
 * 例程:    无
 ************************************************/
static void XCSch_TimeSched(XCOS_t* phXCOS, XCuint_t Tick)
{
    XCListNode_t* pTimeListRootNode;
    XCListNode_t* pIndex;

    pTimeListRootNode = (XCListNode_t*)&phXCOS->TimeList.RootNode;          //时间表的根节点

    //上个保存的Tick大于当前当前Tick,表示Tick溢出
    if(phXCOS->PreviousTick > Tick){
        //溢出则转移所有无溢出时间表节点
        pIndex = pTimeListRootNode->pNext;                                  //得到时间节点初始索引
        while(pIndex != pTimeListRootNode){
            XCSch_ListNodeInsertIndexPrevious(phXCOS, (XCTCB_t*)pIndex);    //插入就绪表
            ((XCTCB_t*)pIndex)->State    = _XC_S_Ready;                     //就绪态
            ((XCTCB_t*)pIndex)->WakeType = _XC_Wake_Time;                   //时间唤醒
            pIndex = pIndex->pNext;
        }
        XCList_SwapList(&phXCOS->TimeList, &phXCOS->TimeOverflowList);      //交换2个时间表
        XCList_Init(&phXCOS->TimeOverflowList);                             //清除溢出表
        //更新下个唤醒时间表
        phXCOS->NextTaskWakeTick = pTimeListRootNode->pNext->Value;         //下个唤醒时间
        phXCOS->PreviousTick     = Tick;                                    //更新保存的Tick
    }

    //Tick大于唤醒时间,需要唤醒任务
    if(Tick >= phXCOS->NextTaskWakeTick){
        pIndex = pTimeListRootNode->pNext;                                  //得到时间节点初始索引
        //所有需要唤醒(<=Tick)的任务入就绪表
        while( (pIndex != pTimeListRootNode) && (pIndex->Value <= Tick) ){
            XCSch_ListNodeRemove(phXCOS, (XCTCB_t*)pIndex);
            XCSch_ListNodeInsertIndexPrevious(phXCOS, (XCTCB_t*)pIndex);    //插入就绪表
            ((XCTCB_t*)pIndex)->State    = _XC_S_Ready;                     //就绪态
            ((XCTCB_t*)pIndex)->WakeType = _XC_Wake_Time;                   //时间唤醒
            pIndex = pIndex->pNext;
        }
        //更新下个唤醒时间表
        phXCOS->NextTaskWakeTick = pIndex->Value;                           //下个唤醒时间
        phXCOS->PreviousTick     = Tick;                                    //更新保存的Tick
    }

    /**"PreviousTick"参数说明
     *  只有在下面2个条件下才可更新
     *  1.在遍历所有时间到达并处理后可更新;
     *  2.在1完成情况下,有新的任务需要入时间阻塞时可更新;
     */
}

/************************************************|
 * 描述:    [私有]阻塞调度
 * 函数名:  XCSch_BlockedSched
 * 形参[I]: XCOS_t* phXCOS      //XCOS句柄
 * 形参[I]: XCTCB_t *phTCB      //TCB句柄
 * 形参[I]: XCuint_t Tick       //当前的Tick
 * 返回:    void
 * 说明:
 *  处理"TCB"阻塞信号;
 *  阻塞信号分"等待时间"和"等待信号"可以同时或单独出现;
 *  当有"等待时间"时只会处理"等待时间",不会在处理"等待信号";
 *  注意:调用"XCSch_BlockedSched"前必须先调用"XCSch_TimeSched"处理时间;
 * 例程:    无
 ************************************************/
static void XCSch_BlockedSched(XCOS_t* phXCOS, XCTCB_t *phTCB, XCuint_t Tick)
{
    XCuint_t TaskWakeTick;

    //时间类阻塞(时间不为0才可调用)
    if( (phTCB->Blocked & _XC_B_WaitTime) && (phTCB->ListNode.Value) ){
        TaskWakeTick = phTCB->ListNode.Value + Tick;    //当前任务下次唤醒的时刻
        phTCB->ListNode.Value = TaskWakeTick;           //保存任务下次唤醒的时刻
        XCSch_ListNodeRemove(phXCOS, phTCB);            //移除当前节点
        if(TaskWakeTick < Tick){        //下次唤醒的Tick溢出
            XCList_InsertAsc(&phXCOS->TimeOverflowList, &phTCB->ListNode);
        }
        else{                           //下次唤醒的Tick没有溢出
            XCList_InsertAsc(&phXCOS->TimeList, &phTCB->ListNode);
            //更新下次任务唤醒的时刻
            if(TaskWakeTick < phXCOS->NextTaskWakeTick){
                phXCOS->NextTaskWakeTick = TaskWakeTick;
                phXCOS->PreviousTick = Tick;            //更新保存的Tick
            }
        }
        phTCB->State = _XC_S_Blocking;                  //阻塞态
    }
    //阻塞
    else if(phTCB->Blocked & _XC_B_Blocked){
        XCSch_ListNodeRemove(phXCOS, phTCB);                        //移除当前节点
        XCList_InsertEnd(&phXCOS->BlockedList, &phTCB->ListNode);   //插入阻塞表
        phTCB->State = _XC_S_Blocking;                              //阻塞态
    }
    //清除阻塞标志
    phTCB->Blocked = _XC_B_NonBlocked;
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 以下用户函数

/************************************************|
 * 描述:    调度器初始化
 * 函数名:  XCSch_Init
 * 形参[I]: XCOS_t* phXCOS          //XCOS句柄
 * 形参[I]: uint8_t SchBlocked      //阻塞选项
 *  +=参数
 *  |   1:  "XCSch_Run"阻塞运行;
 *  |   0:  "XCSch_Run"非阻塞运行;
 * 返回:    void
 * 说明:    注意:在非阻塞调度下"XCSch_Run"需要被循环调用;
 * 例程:    无
 ************************************************/
void XCSch_Init(XCOS_t* phXCOS, uint8_t SchBlocked)
{
    XCList_Init(&phXCOS->ReadyList);
    XCList_Init(&phXCOS->TimeList);
    XCList_Init(&phXCOS->TimeOverflowList);
    XCList_Init(&phXCOS->BlockedList);
    phXCOS->pReadyListNodeIndex = (XCListNode_t*)&phXCOS->ReadyList.RootNode;   //就绪表的根链表
    phXCOS->NextTaskWakeTick = ~0;                  //下个任务唤醒时间为最大
    phXCOS->PreviousTick     = 0;                   //保存上个Tick值
    phXCOS->SchBlocked       = !!(SchBlocked);      //运行阻塞情况
}

/************************************************|
 * 描述:    调度器运行
 * 函数名:  XCSch_Run
 * 形参[I]: XCOS_t* phXCOS  //XCOS句柄
 * 返回:    void
 * 说明:
 *  若是"XCSch_Init"中"SchBlocked"为1,调度器将阻塞;
 *  注意,阻塞下要是有看门狗,记得在任务中喂狗;
 * 例程:    无
 ************************************************/
void XCSch_Run(XCOS_t* phXCOS)
{
    XCTCB_t *phTCB;
    XCuint_t Tick;

    do{
        phXCOS->pReadyListNodeIndex = phXCOS->pReadyListNodeIndex->pNext;   //向下更新就绪表索引
        //判断是否是根节点,根节点不可运行任务
        if( ((void*)phXCOS->pReadyListNodeIndex) == ((void*)&phXCOS->ReadyList.RootNode) ){
            //是根节点
            Tick = XCTime_GetTick();                            //得到当前系统Tick
            XCSch_TimeSched(phXCOS, Tick);                      //时间调度处理
        }
        else{
            //不是根节点处理任务
            phTCB = (XCTCB_t*)(phXCOS->pReadyListNodeIndex);    //得到任务TCB
            phTCB->State = _XC_S_Run;                           //运行态
            phTCB->fTask(phTCB);                                //运行任务
            phTCB->State = _XC_S_Ready;                         //就绪态
            Tick = XCTime_GetTick();                            //得到当前系统Tick
            XCSch_TimeSched(phXCOS, Tick);                      //时间调度处理
            if(phTCB->Blocked){
                XCSch_BlockedSched(phXCOS, phTCB, Tick);        //阻塞调度处理
            }
        }
    }while(phXCOS->SchBlocked);                                 //任务调度是否阻塞处理
}

/************************************************ 我是分割线 ************************************************/

/************************************************|
 * 描述:    任务注册
 * 函数名:  XCSch_TaskReg
 * 形参[I]: XCOS_t* phXCOS          //XCOS句柄
 * 形参[I]: XCTCB_t* phTCB          //协程任务控制块
 * 形参[I]: void(*fTask)(XCPCB_t*)  //任务的函数指针
 * 返回:    void
 * 说明:    挂载到就绪表;
 * 例程:    无
 ************************************************/
void XCSch_TaskReg(XCOS_t* phXCOS, XCTCB_t* phTCB, void(*fTask)(XCTCB_t*))
{
    XCList_InitNode(&phTCB->ListNode);          //初始化链表
    phTCB->phXCOS = phXCOS;                     //保存任务的所属框架句柄
    _COR_Init(phTCB->BP);                       //初始化断点
    phTCB->fTask    = fTask;                    //更新任务入口
    phTCB->Blocked  = _XC_B_NonBlocked;         //没有阻塞
    phTCB->WakeType = _XC_Wake_Non;             //没有唤醒
    phTCB->State    = _XC_S_Ready;              //注册的任务进入就绪态
    XCSch_ListNodeInsertIndexPrevious(phXCOS, phTCB);
}

/************************************************|
 * 描述:    任务移除
 * 函数名:  XCSch_TaskRemove
 * 形参[I]: XCTCB_t* phTCB      //协程控制块
 * 返回:    void
 * 说明:    注销任务,只是将任务从表中移除,需要时可以重新注册;
 * 例程:    无
 ************************************************/
void XCSch_TaskRemove(XCTCB_t* phTCB)
{
    if(phTCB->phXCOS != NULL){
        XCSch_ListNodeRemove(phTCB->phXCOS, phTCB);
    }
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
