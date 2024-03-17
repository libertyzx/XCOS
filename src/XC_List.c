/*=========================================================|
 | 文件名:  XC_List.c
 | 描述:    链表操作
 | 版本:    V1.00
 | 日期:    2024/03/05
 | 语言:    C语言
 | 作者:    libertyzx
 | E-mail:  libertyzx@163.com
 +-----------------------------------------------|
 | 开源协议: MIT License
 +-----------------------------------------------|
 +--- 说明
 |  定义双向链表操作
 +-----------------------------------------------|
 +--- 版本说明:
 |  V1.00:-2024/03/15
 |      1.初始化
 *========================================================*/
//=== 头文件
#include "XC_List.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */

/************************************************|
 * 描述:    初始化链表
 * 函数名:  XCList_Init
 * 形参[I]: XCListRoot_t* pList     //需要初始化的链表
 * 返回:    void
 * 说明:    将链表初始化,清除数据
 * 例程:    无
 ************************************************/
void XCList_Init(XCListRoot_t* pList)
{
    //根节点指向自己
    pList->RootNode.pNext     = (XCListNode_t*)&(pList->RootNode);
    pList->RootNode.pPrevious = (XCListNode_t*)&(pList->RootNode);
    pList->RootNode.Value     = ~0;         //辅助值最大
    pList->ListNodeNum        = 0;          //节点数量为0
}

/************************************************|
 * 描述:    初始化节点
 * 函数名:  XCList_InitNode
 * 形参[I]: XCListRoot_t* pList     //需要初始化的链表
 * 返回:    void
 * 说明:    将链表初始化,清除数据
 * 例程:    无
 ************************************************/
void XCList_InitNode(XCListNode_t* pNode)
{
    pNode->pNext     = NULL;
    pNode->pPrevious = NULL;
    pNode->Value     = ~0;
    pNode->pRootList = NULL;
}

/************************************************|
 * 描述:    将节点插入开始
 * 函数名:  XCList_InsertStart
 * 形参[I]: XCListRoot_t* pList     //需插入的链表
 * 形参[I]: XCListNode_t* pNewNode  //新节点
 * 返回:    void
 * 说明:    插入RootNode前;
 * 例程:    无
 ************************************************/
void XCList_InsertStart(XCListRoot_t* pList, XCListNode_t* pNewNode)
{
    XCListNode_t *pRootNode;

    pRootNode = (XCListNode_t*)&pList->RootNode;    //根节点
    XCList_InsertNext(pRootNode, pNewNode);         //插入节点
    pNewNode->pRootList = pList;                    //保存根
    pList->ListNodeNum++;                           //节点+1
}

/************************************************|
 * 描述:    将节点插入结尾
 * 函数名:  XCList_InsertEnd
 * 形参[I]: XCListRoot_t* pList     //需插入的链表
 * 形参[I]: XCListNode_t* pNewNode  //新节点
 * 返回:    void
 * 说明:    插入RootNode前;
 * 例程:    无
 ************************************************/
void XCList_InsertEnd(XCListRoot_t* pList, XCListNode_t* pNewNode)
{
    XCListNode_t *pRootNode;

    pRootNode = (XCListNode_t*)&pList->RootNode;    //根节点
    XCList_InsertPrevious(pRootNode, pNewNode);     //插入节点
    pNewNode->pRootList = pList;                    //保存根
    pList->ListNodeNum++;                           //节点+1
}

/************************************************|
 * 描述:    升序排列的插入节点
 * 函数名:  XCList_InsertAsc
 * 形参[I]: XCListRoot_t* pList     //需插入的链表
 * 形参[I]: XCListNode_t* pNewNode  //新节点
 * 返回:    void
 * 说明:    从根节点向下(Next)查询辅助值,若值相同,新节点插入在旧节点的后面;
 * 例程:    无
 ************************************************/
void XCList_InsertAsc(XCListRoot_t* pList, XCListNode_t* pNewNode)
{
    XCListNode_t* pIterator;
    XCuint_t NewNodeValue;

    NewNodeValue = pNewNode->Value;                 //新节点较的值
    if(pList->RootNode.Value == NewNodeValue){
        pIterator = pList->RootNode.pPrevious;      //根节点的上个节点
    }
    else{
        //查找(小->大)
        pIterator = (XCListNode_t*)&pList->RootNode;
        do{
            pIterator = pIterator->pNext;
        }while(pIterator->Value <= NewNodeValue);
    }
    XCList_InsertNext(pIterator, pNewNode);     //插入节点
    pNewNode->pRootList = pList;                //保存根
    pList->ListNodeNum++;                       //节点+1
}

/************************************************|
 * 描述:    从链表中移除一个节点
 * 函数名:  XCList_Remove
 * 形参[I]: XCListNode_t* pNode     //需要删除的节点
 * 返回:    void
 * 说明:
 *  节点在插入时就已经保存自身所在的链表;
 *  调用此函数会清除节点中除辅助值(Value)外所有数据;
 *  注意:只处理链表节点部分,不影响节点挂载的其他数据;
 * 例程:    无
 ************************************************/
void XCList_Remove(XCListNode_t* pNode)
{
    XCList_LinkNode(pNode->pPrevious, pNode->pNext);    //删除节点
    if(pNode->pRootList != NULL){
        pNode->pRootList->ListNodeNum--;
    }
    //清除节点数据
    pNode->pNext     = NULL;
    pNode->pPrevious = NULL;
    pNode->pRootList = NULL;
}

/************************************************|
 * 描述:    交换链表
 * 函数名:  XCList_SwapList
 * 形参[I]: XCListRoot_t* pList1
 * 形参[I]: XCListRoot_t* pList2
 * 返回:    void
 * 说明:    更换链表的根节点和链表数量
 * 例程:    无
 ************************************************/
void XCList_SwapList(XCListRoot_t* pList1, XCListRoot_t* pList2)
{
    XCListNode_t* pNext;
    XCListNode_t* pPrevious;
    uint32_t ListNodeNum;

    pNext       = pList1->RootNode.pNext;
    pPrevious   = pList1->RootNode.pPrevious;
    ListNodeNum = pList1->ListNodeNum;

    pList1->RootNode.pNext     = pList2->RootNode.pNext;
    pList1->RootNode.pPrevious = pList2->RootNode.pPrevious;
    pList1->ListNodeNum        = pList2->ListNodeNum;

    pList2->RootNode.pNext     = pNext;
    pList2->RootNode.pPrevious = pPrevious;
    pList2->ListNodeNum        = ListNodeNum;
}


/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
