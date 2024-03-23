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
/**任务-私有宏定义*/

/************************************************|
 * 描述:    获取任务的唤醒的Tick
 * 宏名:    XCSch_GetTaskWakeTick
 * 形参[I]: XCTCB_t* _phTCB     //任务TCB(会强制转为"XCTCB_t*"类型)
 * 返回:    XCuint_t            //返回任务唤醒的Tick
 * 说明:    得到当前链表的开始地址
 * 例程:    无
 ************************************************/
#define XCSch_GetTaskWakeTick(_phTCB)                   (((XCTCB_t*)(_phTCB))->TaskWakeTick)

/************************************************|
 * 描述:    更新任务唤醒类型
 * 宏名:    XCSch_UpdataTaskWakeType
 * 形参[I]: XCTCB_t* _phTCB     //任务TCB(会强制转为"XCTCB_t*"类型)
 * 形参[I]: uint8_t _WakeType   //唤醒类型(_XC_Wake_**)
 * 返回:    void
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCSch_UpdataTaskWakeType(_phTCB, _WakeType)     { ((XCTCB_t*)(_phTCB))->WakeType = (_WakeType); }

/************************************************|
 * 描述:    更新任务状态
 * 宏名:    XCSch_UpdataTaskWakeType
 * 形参[I]: XCTCB_t* _phTCB     //任务TCB(会强制转为"XCTCB_t*"类型)
 * 形参[I]: uint8_t _TaskState  //任务状态(_XC_S_**)
 * 返回:    void
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCSch_UpdataTaskState(_phTCB, _TaskState)       { ((XCTCB_t*)(_phTCB))->TaskState = (_TaskState); }

/************************************************ 我是分割线 ************************************************/
/**框架-私有宏定义*/

/************************************************|
 * 描述:    更新框架中下个唤醒任务的Tick
 * 宏名:    XCSch_UpdataNextWakeTaskTick
 * 形参[I]: XCOS_t* _phXCOS             //框架句柄
 * 形参[I]: XCuint_t _NextWakeTaskTick  //更新的下个唤醒任务的Tick
 * 返回:    Xvoid
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCSch_UpdataNextWakeTaskTick(_phXCOS, _NextWakeTaskTick)    { (_phXCOS)->NextTaskWakeTick = (_NextWakeTaskTick); }

/************************************************|
 * 描述:    获取框架中下个唤醒任务的Tick
 * 宏名:    XCSch_GetNextWakeTaskTick
 * 形参[I]: XCOS_t* _phXCOS         //框架句柄
 * 返回:    XCuint_t                //下个唤醒任务的Tick
 * 说明:    无
 * 例程:    无
 ************************************************/
#define XCSch_GetNextWakeTaskTick(_phXCOS)                          ((_phXCOS)->NextTaskWakeTick)


/************************************************|
 * 描述:    更新上个Tick
 * 宏名:    XCSch_UpdataPreviousTick
 * 形参[I]: XCOS_t* _phXCOS     //框架句柄
 * 形参[I]: XCuint_t _Tick      //更新上个Tick
 * 返回:    void
 * 说明:
 *  更新上个Tick,用于下次调度使用
 *  只有在下面2个条件下才可更新
 *  1.在遍历所有时间到达处理后可更新;
 *  2.在1完成情况下,有新的任务需要入时间阻塞时可更新;
 * 例程:    无
 ************************************************/
#define XCSch_UpdataPreviousTick(_phXCOS, _Tick)                    { (_phXCOS)->PreviousTick = (_Tick); }

/************************************************|
 * 描述:    获取上个Tick
 * 宏名:    XCSch_GetPreviousTick
 * 形参[I]: XCOS_t* _phXCOS     //框架句柄
 * 返回:    XCuint_t
 * 说明:    得到上次保存的Tick
 * 例程:    无
 ************************************************/
#define XCSch_GetPreviousTick(_phXCOS)                              ((_phXCOS)->PreviousTick)

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
    XCList_InsertNodePrevious(phXCOS->pReadyListNodeIndex, &phXCTCB->ListNode); //插入索引节点前
    phXCTCB->ListNode.pRootList = &phXCOS->ReadyList;                           //插在就绪表
}

/************************************************|
 * 描述:    升序排列的插入节点
 * 函数名:  XCSch_ListNodeInsertAsc
 * 形参[I]: XCListRoot_t* pList     //需插入的链表
 * 形参[I]: XCTCB_t* phXCTCB        //协程控制块
 * 返回:    void
 * 说明:    从根节点向下(Next)查询辅助值,若值相同,新节点插入在旧节点的后面;
 * 例程:    无
 ************************************************/
void XCSch_ListNodeInsertAsc(XCListRoot_t* pList, XCTCB_t* phXCTCB)
{
    XCListNode_t* pIterator;
    XCuint_t MaxTick = ~0;          //Tick最大的值


    if(MaxTick == phXCTCB->TaskWakeTick){
        //若是唤醒值等于最大值,则设置为根节点
        pIterator = (XCListNode_t*)&pList->RootNode;
    }
    else{
        //查找(小->大)
        pIterator = pList->RootNode.pNext;    //获取根节点的下个节点地址
        while( (pIterator != (XCListNode_t*)&pList->RootNode) && (((XCTCB_t*)pIterator)->TaskWakeTick <= phXCTCB->TaskWakeTick) ){
            //指向节点不是根节点 && 指向节点的值小于新节点的值;
            pIterator = pIterator->pNext;                           //指向下个节点
        }
    }
    XCList_InsertNodePrevious(pIterator, (XCListNode_t*)phXCTCB);   //插入节点
    phXCTCB->ListNode.pRootList = pList;                            //保存根节点
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
    XCListNode_t* pIndex;
    XCTCB_t* phTCB;

    /**上个保存的Tick大于当前当前Tick,表示Tick溢出
     *  需要做以下处理:
     *  1.将"TimeList"表所有节点移动到"ReadyList"表;
     *  2.将"TimeOverflowList"表所有节点移动到"TimeList"表;
     *  3.更新下个唤醒的时间;
     */
    if(XCSch_GetPreviousTick(phXCOS) > Tick){
        //上个保存的Tick大于当前当前Tick,表示Tick已经溢出,判断表中是否有节点需要处理;
        if(XCList_ListValid(&phXCOS->TimeList)){
            //"TimeList"表中有节点,将所有节点移动到"ReadyList"表
            pIndex = XCList_GetListStartNode(&phXCOS->TimeList);    //得到时间链表初始节点
            while(!XCList_ReachEndNode(&phXCOS->TimeList, pIndex)){
                //节点没有到达结尾,将节点状态改变
                XCSch_UpdataTaskState(pIndex, _XC_S_Ready);         //任务状态:就绪
                XCSch_UpdataTaskWakeType(pIndex, _XC_Wake_Time);    //时间唤醒
                pIndex = pIndex->pNext;                             //指向下个节点
            }
            //将"TimeList"链表移动到节点"pReadyListNodeIndex"前,并清除"TimeList"链表
            XCList_SwapListToNodePrevious(phXCOS->pReadyListNodeIndex, &phXCOS->TimeList);
        }
        //判断"TimeOverflowList"表中是否有节点
        if(XCList_ListValid(&phXCOS->TimeOverflowList)){            //表中有节点
            //"TimeList"表节点处理完成后,直接和"TimeOverflowList"表换,同等于"TimeOverflowList"表所有节点移动到"TimeList"表;
            XCList_SwapList(&phXCOS->TimeList, &phXCOS->TimeOverflowList);
            //更新下个唤醒时间
            XCSch_UpdataNextWakeTaskTick(phXCOS, XCSch_GetTaskWakeTick(XCList_GetListStartNode(&phXCOS->TimeList)));
        }
        else{
            XCSch_UpdataNextWakeTaskTick(phXCOS, ~0);               //若是没有节点,将唤醒时间调整为最大
        }
        XCSch_UpdataPreviousTick(phXCOS, Tick);                     //[注]更新保存的Tick
    }

    /**时间表处理
     *  "TimeList"表有节点,且下个唤醒任务的时间到达,则调用;
     *  需要以下操作;
     *  1.遍历"TimeList"表,唤醒时间到达的任务都移动到"ReadyList"表;
     *  2.更新下个唤醒的时间;
     */
    if(XCList_ListValid(&phXCOS->TimeList)){
        //链表中是有节点的
        if(Tick >= XCSch_GetNextWakeTaskTick(phXCOS)){
            //唤醒时间到达
            pIndex = XCList_GetListStartNode(&phXCOS->TimeList);    //得到时间链表初始节点
            while( (!XCList_ReachEndNode(&phXCOS->TimeList, pIndex))  && (Tick >= XCSch_GetTaskWakeTick(pIndex)) ){
                //节点没有到达结尾 && 当前索引任务唤醒时间到达
                phTCB = (XCTCB_t*)pIndex;       //得到索引任务的TCB
                pIndex = pIndex->pNext;         //指向下个任务
                //唤醒任务移到就绪表
                XCSch_ListNodeRemove(phXCOS, phTCB);                //原链表移除节点
                XCSch_ListNodeInsertIndexPrevious(phXCOS, phTCB);   //插入就绪表
                XCSch_UpdataTaskState(phTCB, _XC_S_Ready);          //任务状态:就绪
                XCSch_UpdataTaskWakeType(phTCB, _XC_Wake_Time);     //时间唤醒
            }
            //轮询结束,判断索引是否到达链表的结尾
            if(!XCList_ReachEndNode(&phXCOS->TimeList, pIndex)){
                //节点没有到达结尾
                XCSch_UpdataNextWakeTaskTick(phXCOS, XCSch_GetTaskWakeTick(pIndex));    //更新下个唤醒时间
            }
            else{
                XCSch_UpdataNextWakeTaskTick(phXCOS, ~0);           //若是没有节点,将唤醒时间调整为最大
            }
            XCSch_UpdataPreviousTick(phXCOS, Tick);                 //[注]更新保存的Tick
        }
    }
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
    if( (phTCB->Blocked & _XC_B_WaitTime) && (phTCB->TaskWakeTick) ){
        TaskWakeTick = phTCB->TaskWakeTick + Tick;      //当前任务下次唤醒的时刻
        phTCB->TaskWakeTick = TaskWakeTick;             //保存任务下次唤醒的时刻
        XCSch_ListNodeRemove(phXCOS, phTCB);            //移除当前节点
        if(TaskWakeTick < Tick){        //下次唤醒的Tick溢出
            XCSch_ListNodeInsertAsc(&phXCOS->TimeOverflowList, phTCB);
        }
        else{                           //下次唤醒的Tick没有溢出
            XCSch_ListNodeInsertAsc(&phXCOS->TimeList, phTCB);
            //更新下次任务唤醒的时刻
            if(TaskWakeTick < phXCOS->NextTaskWakeTick){
                phXCOS->NextTaskWakeTick = TaskWakeTick;
                phXCOS->PreviousTick = Tick;            //更新保存的Tick
            }
        }
    }
    //阻塞
    else if(phTCB->Blocked & _XC_B_Blocked){
        XCSch_ListNodeRemove(phXCOS, phTCB);                        //移除当前节点
        XCList_InsertEnd(&phXCOS->BlockedList, &phTCB->ListNode);   //插入阻塞表
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
 * 返回:    void
 * 说明:    无
 * 例程:    无
 ************************************************/
void XCSch_Init(XCOS_t* phXCOS)
{
    XCList_Init(&phXCOS->ReadyList);
    XCList_Init(&phXCOS->TimeList);
    XCList_Init(&phXCOS->TimeOverflowList);
    XCList_Init(&phXCOS->BlockedList);
    phXCOS->pReadyListNodeIndex = (XCListNode_t*)&phXCOS->ReadyList.RootNode;   //就绪表的根链表
    phXCOS->NextTaskWakeTick = ~0;                  //下个任务唤醒时间为最大
    phXCOS->PreviousTick     = 0;                   //保存上个Tick值
}

/************************************************|
 * 描述:    调度器运行
 * 函数名:  XCSch_Run
 * 形参[I]: XCOS_t* phXCOS  //XCOS句柄
 * 返回:    void
 * 说明:
 *  此函数无返回,阻塞运行;
 *  注意,阻塞下要是有看门狗,记得在任务中喂狗;
 * 例程:    无
 ************************************************/
void XCSch_Run(XCOS_t* phXCOS)
{
    XCTCB_t *phTCB;
    XCuint_t Tick;

    while(1){
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
            phTCB->TaskState = _XC_S_Run;                       //任务状态:运行
            phTCB->fTask(phTCB);                                //运行任务
            Tick = XCTime_GetTick();                            //得到当前系统Tick
            XCSch_TimeSched(phXCOS, Tick);                      //时间调度处理
            if(phTCB->Blocked){
                XCSch_BlockedSched(phXCOS, phTCB, Tick);        //阻塞调度处理
            }
        }
    }
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
    phTCB->TaskWakeTick = ~0;                   //任务下个唤醒的时间
    phTCB->fTask     = fTask;                   //更新任务入口
    _COR_Init(phTCB->BP);                       //初始化断点
    phTCB->Blocked   = _XC_B_NonBlocked;        //没有阻塞
    phTCB->WakeType  = _XC_Wake_Non;            //没有唤醒
    phTCB->TaskState = _XC_S_Ready;             //任务状态:就绪
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
