/**
 * @file        XC_List.c
 * @brief       链表操作
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.1.0
 * @date        2026/08/14
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details
 *  实现了双向链表操作;
 *  注意:
 *  - 链表地址即为链表根节点地址;
 *  - 非通用完整的链表操作,做过内部优化,用户不要使用;
 * **********************************************
 *  修改日志
 *  - 见"CHANGELOG.md"的更新说明;
 */
//=== 头文件
#include "Internal/XC_List.h"

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**链表节点操作 */

/**
 * @brief       [内部]将新节点插入某节点之后
 * @param[in]   pListNode   要插入位置的节点
 * @param[in]   pNewNode    新节点
 * @details     只插入节点;
 */
void XC_List_InsertNodeAfter(XC_ListNode_t* pListNode, XC_ListNode_t* pNewNode)
{
    pNewNode->pNext        = pListNode->pNext;
    pNewNode->pNext->pPrev = pNewNode;
    pNewNode->pPrev        = pListNode;
    pListNode->pNext       = pNewNode;
}

/**
 * @brief       [内部]将节点移动到某个节点之后
 * @param[in]   pDestNode   目标节点
 * @param[in]   pSrcNode    需要移动的节点
 * @details
 *  将节点从原先链表中移除,并移动到目标节点之后;
 */
void XC_List_MoveNodeAfter(XC_ListNode_t* pDestNode, XC_ListNode_t* pSrcNode)
{
    // 链接前后两个节点来删除节点
    pSrcNode->pNext->pPrev = pSrcNode->pPrev;
    pSrcNode->pPrev->pNext = pSrcNode->pNext;

    // 将新节点插入某节点之后
    pSrcNode->pNext        = pDestNode->pNext;
    pSrcNode->pNext->pPrev = pSrcNode;
    pSrcNode->pPrev        = pDestNode;
    pDestNode->pNext       = pSrcNode;
}

/**
 * @brief       [内部]从链表中移除一个节点
 * @param[in]   pNode   需要删除的节点
 * @details     只处理链表节点部分,不影响节点挂载的其他数据;
 */
void XC_List_Remove(XC_ListNode_t* pNode)
{
    /* 链接前后两个节点来删除节点*/
    pNode->pNext->pPrev = pNode->pPrev;
    pNode->pPrev->pNext = pNode->pNext;
    /* 初始化删除的节点*/
    XC_List_InitNode(pNode);
}

/************************************************ 我是分割线 ************************************************/
/**链表操作 */

/**
 * @brief       [内部]将一个链表全部移动到另个链表的一个节点后
 * @param[in]   pDestNode   需要移入链表的节点(链表将移动到此节点后)
 * @param[in]   pSrcList    需要移动的链表
 * @details
 *  - 转移完节点后的链表会被清除;
 *  - 注意:不要移动自己;
 *  - 移动流程如下:
 *  >   链表X:Root,X1,X2,X3,Root
 *  >   链表Y:Root,Y1,Y2,Y3,Root
 *  >   在链表X节点X2的下面插入链表Y;
 *  >   函数为:XC_List_MoveListToNodeAfter(&X2,&Y);
 *  >   运行后得到:
 *  >   链表X: Root,X1,X2,Y1,Y2,Y3,X3,Root;
 *  >   链表Y: 被清除;
 *  - 运行表(运行:XC_List_MoveListToNodeAfter(&X2,&Y);)
 *  > XR,YR:为根节点; X*,Y*:为节点; P:指向上个节点; N:指向下个节点
 *  > 运行顺序对应代码操作;
 *  | 链表\运行顺序 | 1    | 2    | 3    | 4    | 5         | 最终得到     |
 *  | ---           | ---  | ---  | ---  | ---  | ---       | ---          |
 *  | XR,P=X3,N=X1  |      |      |      |      |           | XR,P=X3,N=X1 |
 *  | X1,P=XR,N=X2  |      |      |      |      |           | X1,P=XR,N=X2 |
 *  | X2,P=X1,N=X3  |      |      | N=Y1 |      |           | X2,P=X1,N=Y1 |
 *  | X3,P=X2,N=XR  |      | P=Y3 |      |      |           | X3,P=Y3,N=XR |
 *  | YR,P=Y3,N=Y1  |      |      |      |      | P=YR,N=YR | YR,P=YR,N=YR |
 *  | Y1,P=YR,N=Y2  |      |      |      | P=X2 |           | Y1,P=X2,N=Y2 |
 *  | Y2,P=Y1,N=Y3  |      |      |      |      |           | Y2,P=Y1,N=Y3 |
 *  | Y3,P=Y2,N=YR  | N=X3 |      |      |      |           | Y3,P=Y2,N=X3 |
 *  最终2个链表的值:
 *  | 链表X        | 链表Y        |
 *  | ---          | ---          |
 *  | XR,P=X3,N=X1 | YR,P=YR,N=YR |
 *  | X1,P=XR,N=X2 |              |
 *  | X2,P=X1,N=Y1 |              |
 *  | Y1,P=X2,N=Y2 |              |
 *  | Y2,P=Y1,N=Y3 |              |
 *  | Y3,P=Y2,N=X3 |              |
 *  | X3,P=Y3,N=XR |              |
 */
void XC_List_MoveListToNodeAfter(XC_ListNode_t* pDestNode, XC_ListNode_t* pSrcList)
{
    pSrcList->pPrev->pNext  = pDestNode->pNext; //[1]链表上的下,链接到,节点下;[YR->P(Y3)->N = X2->N(X3)]
    pDestNode->pNext->pPrev = pSrcList->pPrev;  //[2]节点下的上,链接到,链表上;[X2->N(X3)->P = YR->P(Y3)]
    pDestNode->pNext        = pSrcList->pNext;  //[3]节点下,链接到,链表下;[X2->N = YR->N(Y1)]
    pSrcList->pNext->pPrev  = pDestNode;        //[4]链表下的上,链接到,节点;[YR->N(Y1)->P = X2]

    XC_List_Init(pSrcList); //[5]节点移动完成后清除链表
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
