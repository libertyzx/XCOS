/*=========================================================|
 | 文件名:  XC_Sem.c
 | 描述:    信号量实现
 | 版本:    V1.00
 | 日期:    2024/03/25
 | 语言:    C语言
 | 作者:    libertyzx
 | E-mail:  libertyzx@163.com
 +-----------------------------------------------|
 | 开源协议: MIT License
 +-----------------------------------------------|
 +--- 说明
 |  1.二值信号量说明:
 |      #.二值信号量在调用"XCSem_BinSemTake_"后才有效;
 |      #.二值信号量只能对应唤醒"XCSem_BinSemTake"调用的任务;
 |      #.二值信号量发送信号"XCSem_BinSemGive"函数是中断安全的可以在任意位置调用;
 +-----------------------------------------------|
 +--- 版本说明:
 |  V1.00:-2024/03/25
 |      1.初始化
 *========================================================*/
//=== 头文件
#include "XC_Sem.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**内部函数*/

/************************************************|
 * 描述:    移除二值信号量
 * 函数名:  XCSem_BinSemRemove
 * 形参[I]: XCSemBin_t* phSem       //信号量句柄
 * 形参[I]: uint32_t RetainedSem    //保留信号(1保留;0不保留)
 * 返回:    void
 * 说明:    注意,这里会清除锁清除信号数;
 * 例程:    无
 ************************************************/
void XCSem_BinSemRemove(XCSemBin_t* phSem, uint32_t RetainedSem)
{
    XCList_LinkNode(phSem->ListNode.pPrevious, phSem->ListNode.pNext); // 表中删除
    // 清除信号
    {
        phSem->phTCB = NULL; // 不挂载TCB
        // 链表指向自己
        phSem->ListNode.pNext     = (XCListNode_t*)&phSem->ListNode;
        phSem->ListNode.pPrevious = (XCListNode_t*)&phSem->ListNode;
        phSem->Lock               = _XC_Lock_Unlock; // 没有锁
        // 是否保留信号
        if(!RetainedSem) {
            phSem->SemNum = 0; // 清除信号数
        }
    }
}

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

//=== 以下用户函数

/************************************************|
 * 描述:    二值信号量初始化
 * 函数名:  XCSem_BinSemInit
 * 形参[I]: XCSemBin_t* phSem   //信号量句柄
 * 返回:    void
 * 说明:    无
 * 例程:    无
 ************************************************/
void XCSem_BinSemInit(XCSemBin_t* phSem)
{
    phSem->phTCB = NULL; // 不挂载TCB
    // 链表指向自己
    phSem->ListNode.pNext     = (XCListNode_t*)&phSem->ListNode;
    phSem->ListNode.pPrevious = (XCListNode_t*)&phSem->ListNode;
    phSem->Lock               = _XC_Lock_Unlock; // 没有锁
    phSem->SemNum             = 0;               // 清除信号数
}

/************************************************|
 * 描述:    二值信号量强制清除信号(获取信号,消费者)
 * 函数名:  XCSem_BinSemForceClrSem
 * 形参[I]: XCSemBin_t* phSem   //信号量句柄
 * 返回:    void
 * 说明:    只清除信号,其他不动;
 * 例程:    无
 ************************************************/
void XCSem_BinSemForceClrSem(XCSemBin_t* phSem)
{
    phSem->SemNum = 0;
}

/************************************************|
 * 描述:    二值信号量强制设置信号(释放信号,生产者)
 * 函数名:  XCSem_BinSemForceSetSem
 * 形参[I]: XCSemBin_t* phSem   //信号量句柄
 * 返回:    void
 * 说明:    只设置信号,其他不动;
 * 例程:    无
 ************************************************/
void XCSem_BinSemForceSetSem(XCSemBin_t* phSem)
{
    phSem->SemNum = 1;
}

/************************************************|
 * 描述:    [内部]获取信号(消费者)
 * 函数名:  XCSem_BinSemTake_
 * 参数[I]: XCSemBin_t* phSem       //信号
 * 参数[I]: XCTCB_t* phTCB          //挂载的任务
 * 返回:    int32_t
 *  +=返回值
 *  | _XC_R_Continue    //没有信号,进入阻塞
 *  | _XC_R_OK          //获取成功
 * 说明:
 *  消费信号;
 *  在"XCSem_BinSemTake"中调用,在协程任务块中使用;
 * 例程:    无
 ************************************************/
int32_t XCSem_BinSemTake_(XCSemBin_t* phSem, XCTCB_t* phTCB)
{
    if(phSem->SemNum == 0) { // 空则等待信号
        // 插入信号表
        phTCB->TaskState = _XC_S_WaitSem;                  // 任务状态:等待信号
        phTCB->WakeType  = _XC_Wake_Non;                   // 清除唤醒类型
        phTCB->Blocked   = _XC_B_Blocked | _XC_B_WaitTime; // 阻塞状态

        // 没有在链表中则插入链表
        if(phSem->ListNode.pNext == (XCListNode_t*)&phSem->ListNode) {
            XCList_InsertNodePrevious((XCListNode_t*)&phTCB->phXCOS->SemList.RootNode, (XCListNode_t*)&phSem->ListNode);
        }

        phSem->phTCB = phTCB;             // 绑定TCB
        phSem->Lock  = _XC_Lock_WaitLock; // 等待锁定

        /**在设置信号完成前出现信号量
         *  释放信号需要满足"Lock","phTCB","TaskState"的要求;
         *  但是在处理这些参数的时候会可能存在被中断去情况;
         *  所以这里需要再次判断"SemNum"值是否有效,有效则重新唤醒;
         *  重新唤醒后任务依然会被让出控制权,但是不会被阻塞,
         *  依然就绪态,等待下次运行;
         */
        if(phSem->SemNum != 0) {
            XCSem_BinSemGive(phSem); // 重新唤醒,重新调度处理
        }

        return (_XC_R_Continue);
    }

    // 有信号
    phSem->SemNum = 0; // 清除信号
    return (_XC_R_OK);
}

/************************************************|
 * 描述:    释放信号(生产者)
 * 函数名:  XCSem_BinSemGive
 * 参数[I]: XCSemBin_t* phSem       //信号句柄
 * 返回:    int32_t
 *  +=返回值
 *  | _XC_R_OK          //释放成功
 *  | _XC_R_Continue    //任务是唤醒的不需要要释放
 *  | _XC_R_Fail        //任务被挂起或没有信号需求
 * 说明:    生产信号
 * 例程:    无
 ************************************************/
int32_t XCSem_BinSemGive(XCSemBin_t* phSem)
{
    /**生产一个信号
     * "SemNum"累加是中断不安全的,但是二值信号量只要有值就可以,
     * 所以在累加时被中断并不影响实际处理;
     * 但是"SemNum"在清零时刻前被释放信号中断,则会影响SemNum实际值(值变为0),
     * 所以在逻辑中处理,"SemNum"清零只在"XCSem_BinSemGive"释放信号和
     * "XCSch_SemSched"信号调度中出现,且出现的地方都在锁中需要任务调度唤醒,
     * 在这个时候"SemNum"值并不会被使用,所以出现值错误也不影响运行;
     */
    if(phSem->SemNum < _XC_Cnf_TaskMaxNum) {
        phSem->SemNum++;
    }

    /**无效判断
     *  以下状态时,信号无效,返回"_XC_R_Fail"
     *  1."phTCB"为NULL:
     *      信号没有被挂载到任务上,被释放使只是有这个信号,但是不处理任务;
     *  2."TaskState"不为_XC_S_WaitSem
     *      在等待信号的状态下,任务被挂起,或者信号超时到达,则会出现非"_XC_S_WaitSem"的状态
     */
    if((phSem->phTCB == NULL) || (phSem->phTCB->TaskState != _XC_S_WaitSem)) {
        return (_XC_R_Fail);
    }

    /**锁定处理
     *  这里锁处理,保证在释放信号时中断或多次中断嵌套中只有一次操作是有效的;
     */
    if(phSem->Lock != _XC_Lock_WaitLock) {
        return (_XC_R_Continue); // 不是等待锁定状态,则不处理
    }
    phSem->Lock++; // 加锁(核心部分)
    if(phSem->Lock != _XC_Lock_Lock) {
        return (_XC_R_Continue); // 被中断,其他地方唤醒
    }
    // 已经锁定,可以操作
    {
        phSem->SemNum = 0; // 清除信号值
        /**判断链表是否在操作
         *  "XCSch_GetListOperationState"用来判断,在释放信号唤醒时,
         *  调度器是否在运行链表相关的操作;
         *  若没有链表操作,则直接操作链表,将任务移动至就绪表;
         *  若是在链表操作,则需要再信号调度中去唤醒任务,以此来保证中断安全;
         */
        if(XCSch_GetListOperationState(phSem->phTCB->phXCOS)) { // 链表操作中
            /**传递唤醒
             *  这里操作是中断不安全的;
             *  在"信号调度"(XCSch_SemSched)中"唤醒计数处理"中说明解决方式;
             */
            if(phSem->phTCB->phXCOS->SemWakeCount < _XC_Cnf_TaskMaxNum) {
                phSem->phTCB->phXCOS->SemWakeCount++;
            }
            // 等待解锁
            phSem->Lock = _XC_Lock_WaitUnlock;
            /*解锁在信号调度(XCSch_SemSched)中处理*/
        }
        else {                                              // 链表没有操作
            XCSch_ListOperationStart(phSem->phTCB->phXCOS); // 链表操作开始
            // 是否未阻塞
            if(phSem->phTCB->Blocked) {
                /**若是任务阻塞有效,说明任务还没运行到阻塞处理时被中断;
                 */
                phSem->phTCB->Blocked = _XC_B_NonBlocked; // 清除阻塞
            }
            else {
                XCList_Remove(&phSem->phTCB->ListNode);       // 移除节点
                XC_ListNodeInsertIndexPrevious(phSem->phTCB); // 插入就续表
            }
            phSem->phTCB->WakeType  = _XC_Wake_Sem;       // 被信号唤醒
            phSem->phTCB->TaskState = _XC_S_Ready;        // 任务状态:就绪
            XCSch_ListOperationEnd(phSem->phTCB->phXCOS); // 链表操作结束
            XCSem_BinSemRemove(phSem, 0);                 // 移除自己
        }
    }

    return (_XC_R_OK);
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
