/**
 * @file        XC_List.c
 * @brief       链表操作
 * @author      libertyzx (libertyzx@163.com)
 * @version     1.0
 * @date        2024/03/18
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     实现了双向链表操作;
 *  本文件中所有函数都是内部使用;不建议用户使用;
 * **********************************************
 *  修改日志
 *  - 见"XC_UpdateInfo.md"的更新说明;
 */
//=== 头文件
#include "XC_List.h"
#include "XC_Cnf.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**初始化操作 */

/**
 * @brief       [内部]初始化链表
 * @param[in]   pList   需要初始化的链表
 * @details     将链表初始化,清除数据;
 */
void XCList_Init(XCListRoot_t* pList)
{
    // 根节点指向自己
    pList->RootNode.pNext     = (XCListNode_t*)&(pList->RootNode);
    pList->RootNode.pPrevious = (XCListNode_t*)&(pList->RootNode);
}

/**
 * @brief       [内部]初始化节点
 * @param[in]   pNode   需要初始化的节点
 * @details     节点初始化,指向自己;
 */
void XCList_InitNode(XCListNode_t* pNode)
{
    pNode->pNext     = pNode;
    pNode->pPrevious = pNode;
    pNode->pRootList = NULL;
}

/************************************************ 我是分割线 ************************************************/
/**链表节点操作 */

/**
 * @brief       [内部]将新节点插入某节点之前
 * @param[in]   pListNode 要插入位置的节点
 * @param[in]   pNewNode  新节点
 * @details     只插入节点;
 */
void XCList_InsertNodePrevious(XCListNode_t* pListNode, XCListNode_t* pNewNode)
{
    pNewNode->pNext             = pListNode;
    pNewNode->pPrevious         = pListNode->pPrevious;
    pListNode->pPrevious->pNext = pNewNode;
    pListNode->pPrevious        = pNewNode;
}

/**
 * @brief       [内部]将新节点插入某节点之后
 * @param[in]   pListNode   要插入位置的节点
 * @param[in]   pNewNode    新节点
 * @details     只插入节点;
 */
void XCList_InsertNodeNext(XCListNode_t* pListNode, XCListNode_t* pNewNode)
{
    pNewNode->pNext            = pListNode->pNext;
    pNewNode->pNext->pPrevious = pNewNode;
    pNewNode->pPrevious        = pListNode;
    pListNode->pNext           = pNewNode;
}

/**
 * @brief       [内部]连接2个节点
 * @param[in]   pPreviousNode   上个节点
 * @param[in]   pNextNode       下个节点
 * @details     将两个节点连接,以删除2节点间的所有节点;
 */
void XCList_LinkNode(XCListNode_t* pPreviousNode, XCListNode_t* pNextNode)
{
    pNextNode->pPrevious = pPreviousNode;
    pPreviousNode->pNext = pNextNode;
}

/**
 * @brief       [内部]移除基础节点
 * @param[in]   pNode   需要删除的节点
 * @details     只处理链表节点部分,不影响节点挂载的其他数据;
 */
void XCList_RemoveBasicNode(XCListBasic_t* pNode)
{
    XCList_LinkNode(pNode->pPrevious, pNode->pNext); // 表中删除
    pNode->pNext     = (XCListNode_t*)pNode;
    pNode->pPrevious = (XCListNode_t*)pNode;
}

/************************************************ 我是分割线 ************************************************/
/**链表操作 */

/**
 * @brief       [内部]从链表中移除一个节点
 * @param[in]   pNode   需要删除的节点;
 * @details     只处理链表节点部分,不影响节点挂载的其他数据;
 */
void XCList_Remove(XCListNode_t* pNode)
{
    XCList_LinkNode(pNode->pPrevious, pNode->pNext); // 删除节点
    XCList_InitNode(pNode);                          // 初始化链表
}

/**
 * @brief       [内部]将节点插入链表的开始
 * @param[in]   pList       需插入的链表
 * @param[in]   pNewNode    新节点
 * @details     插入RootNode头;
 */
void XCList_InsertStart(XCListRoot_t* pList, XCListNode_t* pNewNode)
{
    XCList_InsertNodeNext((XCListNode_t*)&pList->RootNode, pNewNode); // 插入节点
    pNewNode->pRootList = pList;                                      // 保存根
}

/**
 * @brief       [内部]将节点插入链表的结尾
 * @param[in]   pList       需插入的链表
 * @param[in]   pNewNode    新节点
 * @details     插入RootNode尾;
 */
void XCList_InsertEnd(XCListRoot_t* pList, XCListNode_t* pNewNode)
{
    XCList_InsertNodePrevious((XCListNode_t*)&pList->RootNode, pNewNode); // 插入节点
    pNewNode->pRootList = pList;                                          // 保存根
}

/**
 * @brief       [内部]交换链表
 * @param[in]   pList1  链表1
 * @param[in]   pList2  链表2
 * @details     直接交换两个链表根节点的链接;交换后链表为空,则初始化;
 */
void XCList_SwapList(XCListRoot_t* pList1, XCListRoot_t* pList2)
{
    XCListNode_t* pNext;
    XCListNode_t* pPrevious;
    uint8_t       List1NodeExistence;

    // 保存链表1的上下节点,给于链表2
    pNext              = pList1->RootNode.pNext;
    pPrevious          = pList1->RootNode.pPrevious;
    List1NodeExistence = XCList_ListValid(pList1);

    // 链表2有节点
    if(XCList_ListValid(pList2)) {
        // 将链表2的上下节点给链表1
        pList1->RootNode.pNext     = pList2->RootNode.pNext;
        pList1->RootNode.pPrevious = pList2->RootNode.pPrevious;
        // 将链表1的上下节点链接到链表1
        pList1->RootNode.pNext->pPrevious = (XCListNode_t*)&pList1->RootNode;
        pList1->RootNode.pPrevious->pNext = (XCListNode_t*)&pList1->RootNode;
    }
    else {
        XCList_Init(pList1);
    }

    // 链表1有节点
    if(List1NodeExistence) {
        // 将保存的链表1给于链表2
        pList2->RootNode.pNext     = pNext;
        pList2->RootNode.pPrevious = pPrevious;
        // 将链表2的上下节点链接到链表2
        pList2->RootNode.pNext->pPrevious = (XCListNode_t*)&pList2->RootNode;
        pList2->RootNode.pPrevious->pNext = (XCListNode_t*)&pList2->RootNode;
    }
    else {
        XCList_Init(pList2);
    }
}

/**
 * @brief       [内部]将一个链表全部移动到另个链表的一个节点前
 * @param[in]   pDestNode   需要移入链表的节点(链表将移动到此节点前)
 * @param[in]   pSrcList    需要移动的链表
 * @details
 *  - 转移完节点后的链表会被清除;
 *  - 注意:不要移动自己;
 *  - 移动流程如下:
 *  >   链表X:Root,X1,X2,X3,Root \n
 *  >   链表Y:Root,Y1,Y2,Y3,Root \n
 *  >   在链表1节点X2的上面插入链表2; \n
 *  >   函数为:XCList_SwapListToNodePrevious(&X2,&Y); \n
 *  >   运行后得到: \n
 *  >   链表X: Root,X1,Y1,Y2,Y3,X2,X3,Root; \n
 *  >   链表Y: 被清除;
 *  - 运行表(运行:XCList_SwapListToNodePrevious(&X2,&Y);)
 *  > XR,YR:为根节点; X*,Y*:为节点; P:指向上个节点; N:指向下个节点
 *  | 链表/运行顺序 | 1    | 2    | 3    | 4    | 5         | 最终得到     |
 *  | ---           | ---  | ---  | ---  | ---  | ---       | ---          |
 *  | XR,P=X3,N=X1  |      |      |      |      |           | XR,P=X3,N=X1 |
 *  | X1,P=XR,N=X2  | N=Y1 |      |      |      |           | X1,P=XR,N=Y1 |
 *  | X2,P=X1,N=X3  |      |      |      | P=Y3 |           | X2,P=Y3,N=X3 |
 *  | X3,P=X2,N=XR  |      |      |      |      |           | X3,P=X2,N=XR |
 *  | YR,P=Y3,N=Y1  |      |      |      |      | P=YR,N=YR | YR,P=YR,N=YR |
 *  | Y1,P=YR,N=Y2  |      | P=X1 |      |      |           | Y1,P=X1,N=Y2 |
 *  | Y2,P=Y1,N=Y3  |      |      |      |      |           | Y2,P=Y1,N=Y3 |
 *  | Y3,P=X2,N=YR  |      |      | N=X2 |      |           | Y3,P=X2,N=X2 |
 *  最终2个链表的值:
 *  | 链表X        | 链表Y        |
 *  | ---          | ---          |
 *  | XR,P=X3,N=X1 | YR,P=YR,N=YR |
 *  | X1,P=XR,N=Y1 |              |
 *  | Y1,P=X1,N=Y2 |              |
 *  | Y2,P=Y1,N=Y3 |              |
 *  | Y3,P=X2,N=X2 |              |
 *  | X2,P=Y3,N=X3 |              |
 *  | X3,P=X2,N=XR |              |
 *
 */
void XCList_SwapListToNodePrevious(XCListNode_t* pDestNode, XCListRoot_t* pSrcList)
{
    pDestNode->pPrevious->pNext         = pSrcList->RootNode.pNext;     //[1]节点上的下,链接到,链表下
    pSrcList->RootNode.pNext->pPrevious = pDestNode->pPrevious;         //[2]链表下的上,链接到,节点上
    pSrcList->RootNode.pPrevious->pNext = pDestNode;                    //[3]链表上的下,链接到,节点
    pDestNode->pPrevious                = pSrcList->RootNode.pPrevious; //[4]节点的上,链接到,链表的上

    XCList_Init(pSrcList); //[5]节点移动完成后清除链表
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
