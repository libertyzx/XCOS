/*=========================================================|
 | 文件名:  XC_List.c
 | 描述:    链表操作
 | 版本:    V1.00
 | 日期:    2024/03/18
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
 |  V1.00:-2024/03/18
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
    pNode->pRootList = NULL;
}

/************************************************ 我是分割线 ************************************************/
/**链表节点操作*/

/************************************************|
 * 描述:    将新节点插入某节点之前
 * 函数名:  XCList_InsertNodePrevious
 * 形参[I]: XCListNode_t* pListNode     //要插入位置的节点
 * 形参[I]: XCListNode_t* pNewNode      //新节点
 * 返回:    void
 * 说明:    只插入节点;
 * 例程:    无
 ************************************************/
void XCList_InsertNodePrevious(XCListNode_t* pListNode, XCListNode_t* pNewNode)
{
    pNewNode->pNext             = pListNode;
    pNewNode->pPrevious         = pListNode->pPrevious;
    pListNode->pPrevious->pNext = pNewNode;
    pListNode->pPrevious        = pNewNode;
}

/************************************************|
 * 描述:    将新节点插入某节点之后
 * 函数名:  XCList_InsertNodeNext
 * 形参[I]: XCListNode_t* pListNode     //要插入位置的节点
 * 形参[I]: XCListNode_t* pNewNode      //新节点
 * 返回:    void
 * 说明:    只插入节点;
 * 例程:    无
 ************************************************/
void XCList_InsertNodeNext(XCListNode_t* pListNode, XCListNode_t* pNewNode)
{
    pNewNode->pNext            = pListNode->pNext;
    pNewNode->pNext->pPrevious = pNewNode;
    pNewNode->pPrevious        = pListNode;
    pListNode->pNext           = pNewNode;
}

/************************************************|
 * 描述:    连接2个节点
 * 函数名:  XCList_LinkNode
 * 形参[I]: XCListNode_t* pPreviousNode  //上个节点
 * 形参[I]: XCListNode_t* pNextNode      //下个节点
 * 返回:    void
 * 说明:    这将删除2个节点间所有节点;
 * 例程:    无
 ************************************************/
void XCList_LinkNode(XCListNode_t* pPreviousNode, XCListNode_t* pNextNode)
{
    pNextNode->pPrevious = pPreviousNode;
    pPreviousNode->pNext = pNextNode;
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
    XCList_InitNode(pNode);                             //初始化链表
}

/************************************************ 我是分割线 ************************************************/
/**链表操作*/

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
    XCList_InsertNodeNext((XCListNode_t*)&pList->RootNode, pNewNode);   //插入节点
    pNewNode->pRootList = pList;    //保存根
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
    XCList_InsertNodePrevious((XCListNode_t*)&pList->RootNode, pNewNode);   //插入节点
    pNewNode->pRootList = pList;    //保存根
}

/************************************************|
 * 描述:    交换链表
 * 函数名:  XCList_SwapList
 * 形参[I]: XCListRoot_t* pList1
 * 形参[I]: XCListRoot_t* pList2
 * 返回:    void
 * 说明:    无
 * 例程:    无
 ************************************************/
void XCList_SwapList(XCListRoot_t* pList1, XCListRoot_t* pList2)
{
    XCListNode_t* pNext;
    XCListNode_t* pPrevious;
    uint8_t List1NodeExistence;

    //保存链表1的上下节点,给于链表2
    pNext     = pList1->RootNode.pNext;
    pPrevious = pList1->RootNode.pPrevious;
    List1NodeExistence = XCList_NodeExistence(pList1);

    //链表2有节点
    if(XCList_NodeExistence(pList2)){
        //将链表2的上下节点给链表1
        pList1->RootNode.pNext     = pList2->RootNode.pNext;
        pList1->RootNode.pPrevious = pList2->RootNode.pPrevious;
        //将链表1的上下节点链接到链表1
        pList1->RootNode.pNext->pPrevious = (XCListNode_t*)&pList1->RootNode;
        pList1->RootNode.pPrevious->pNext = (XCListNode_t*)&pList1->RootNode;
    }
    else{
        XCList_Init(pList1);
    }

    //链表1有节点
    if(List1NodeExistence){
        //将保存的链表1给于链表2
        pList2->RootNode.pNext     = pNext;
        pList2->RootNode.pPrevious = pPrevious;
        //将链表2的上下节点链接到链表2
        pList2->RootNode.pNext->pPrevious = (XCListNode_t*)&pList2->RootNode;
        pList2->RootNode.pPrevious->pNext = (XCListNode_t*)&pList2->RootNode;
    }
    else{
        XCList_Init(pList2);
    }
}

/************************************************|
 * 描述:    将一个链表全部移动到一个节点前
 * 函数名:  XCList_SwapListToNodePrevious
 * 形参[I]: XCListNode_t* pDestNode //需要移动到的节点
 * 形参[I]: XCListRoot_t* pSrcList  //需要移动的链表
 * 返回:    void
 * 说明:    无
 *  转移后链表会被清除;
 *  移动流程如下:
 *  链表1:X1,X2,X3
 *  链表2:Root,Y1,Y2,Y3,Root
 *  在链表1节点X2的上面插入链表2;
 *  函数为:XCList_SwapListToNodePrevious(&X1,&Root);
 *  运行后得到
 *  X1,Y1,Y2,Y3,X2,X3;
 * 例程:    无
 ************************************************/
void XCList_SwapListToNodePrevious(XCListNode_t* pDestNode, XCListRoot_t* pSrcList)
{
    //节点上链接
    pDestNode->pPrevious->pNext = pSrcList->RootNode.pNext;     //节点上的下,链接到,链表下
    pSrcList->RootNode.pNext->pPrevious = pDestNode->pPrevious; //链表下的上,链接到,节点上

    //节点链接
    pSrcList->RootNode.pPrevious->pNext = pDestNode;            //链表上的下,链接到,节点
    pDestNode->pPrevious = pSrcList->RootNode.pPrevious;        //节点的上,链接到,链表的上

    XCList_Init(pSrcList);                                      //节点移动完成后清除链表
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
