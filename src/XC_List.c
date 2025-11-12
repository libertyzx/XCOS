/**
 * @file        XC_List.c
 * @brief       链表操作
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0
 * @date        2025/11/12
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
/**链表节点操作 */

/**
 * @brief       [内部]将新节点插入某节点之前
 * @param[in]   pListNode 要插入位置的节点
 * @param[in]   pNewNode  新节点
 * @details     只插入节点;
 */
void XCList_InsertNodeBefore(XCListNode_t* pListNode, XCListNode_t* pNewNode)
{
    pNewNode->pNext         = pListNode;
    pNewNode->pPrev         = pListNode->pPrev;
    pListNode->pPrev->pNext = pNewNode;
    pListNode->pPrev        = pNewNode;
}

/**
 * @brief       [内部]将节点移动到某个节点之前
 * @param[in]   pDestNode   目标节点
 * @param[in]   pSrcNode    需要移动的节点
 * @details
 *  将节点从原先链表中移除,并移动到目标节点之前;
 */
void XCList_MoveNodeBefore(XCListNode_t* pDestNode, XCListNode_t* pSrcNode)
{
    // 链接前后两个节点来删除节点
    pSrcNode->pNext->pPrev = pSrcNode->pPrev;
    pSrcNode->pPrev->pNext = pSrcNode->pNext;

    // 将新节点插入目标节点之前
    pSrcNode->pNext         = pDestNode;
    pSrcNode->pPrev         = pDestNode->pPrev;
    pDestNode->pPrev->pNext = pSrcNode;
    pDestNode->pPrev        = pSrcNode;
}

/************************************************ 我是分割线 ************************************************/
/**链表操作 */

/**
 * @brief       [内部]交换链表
 * @param[in]   pList1  链表1
 * @param[in]   pList2  链表2
 * @details     直接交换两个链表根节点的链接;交换后链表为空,则初始化;
 */
void XCList_SwapList(XCListNode_t* pList1, XCListNode_t* pList2)
{
    XCListNode_t* pNext;
    XCListNode_t* pPrev;
    uint8_t       List1NodeExistence;

    // 保存链表1的上下节点,给于链表2
    pNext              = pList1->pNext;
    pPrev              = pList1->pPrev;
    List1NodeExistence = XCList_ListValid(pList1);

    // 链表2有节点
    if(XCList_ListValid(pList2)) {
        // 将链表2的上下节点给链表1
        pList1->pNext = pList2->pNext;
        pList1->pPrev = pList2->pPrev;
        // 将链表1的上下节点链接到链表1
        pList1->pNext->pPrev = pList1;
        pList1->pPrev->pNext = pList1;
    }
    else {
        XCList_Init(pList1);
    }

    // 链表1有节点
    if(List1NodeExistence) {
        // 将保存的链表1给于链表2
        pList2->pNext = pNext;
        pList2->pPrev = pPrev;
        // 将链表2的上下节点链接到链表2
        pList2->pNext->pPrev = pList2;
        pList2->pPrev->pNext = pList2;
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
 *  >   函数为:XCList_SwapListToNodeBefore(&X2,&Y); \n
 *  >   运行后得到: \n
 *  >   链表X: Root,X1,Y1,Y2,Y3,X2,X3,Root; \n
 *  >   链表Y: 被清除;
 *  - 运行表(运行:XCList_SwapListToNodeBefore(&X2,&Y);)
 *  > XR,YR:为根节点; X*,Y*:为节点; P:指向上个节点; N:指向下个节点
 *  > 运行顺序对应代码操作;
 *  | 链表\运行顺序 | 1    | 2    | 3    | 4    | 5         | 最终得到     |
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
void XCList_SwapListToNodeBefore(XCListNode_t* pDestNode, XCListNode_t* pSrcList)
{
    pDestNode->pPrev->pNext = pSrcList->pNext;  //[1]节点上的下,链接到,链表下
    pSrcList->pNext->pPrev  = pDestNode->pPrev; //[2]链表下的上,链接到,节点上
    pSrcList->pPrev->pNext  = pDestNode;        //[3]链表上的下,链接到,节点
    pDestNode->pPrev        = pSrcList->pPrev;  //[4]节点的上,链接到,链表的上

    XCList_Init(pSrcList); //[5]节点移动完成后清除链表
}

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
