/**
 * @file        XC_List.h
 * @brief       链表操作的声明
 * @author      libertyzx (libertyzx@163.com)
 * @version     2.0
 * @date        2025/11/12
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     链表操作的函数声明及类型定义;
 *  本文件中所有类型都是内部使用;用户不要使用;
 * **********************************************
 *  修改日志
 *  - 见"XC_UpdateInfo.md"的更新说明;
 */
//=== 防重复定义
#ifndef _XC_List_H_
#define _XC_List_H_

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**数据类型 */

/**
 * @brief   [内部]双向链表节点类型
 * @details
 *  普通节点和根节点都使用此类型; \n
 *  根节点只是标记的普通节点; \n
 *  32位下占8字节;
 */
typedef struct XCListNode_t {
    struct XCListNode_t* pNext; // 指向下个节点
    struct XCListNode_t* pPrev; // 指向上个节点
} XCListNode_t;

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**初始化操作 */

/**
 * @brief       [内部]初始化节点
 * @param[in]   pNode   需要初始化的节点
 * @details     初始化节点时使用,指向自己,优化为宏;
 */
#define XCList_InitNode(_pNode)     \
    {                               \
        (_pNode)->pNext = (_pNode); \
        (_pNode)->pPrev = (_pNode); \
    }

/**
 * @brief       [内部]初始化链表
 * @param[in]   pList   需要初始化的链表
 * @details     初始化链表时使用
 */
#define XCList_Init(_pList) XCList_InitNode(_pList)

/************************************************ 我是分割线 ************************************************/
/**链表节点操作 */

/**
 * @brief       [内部]将新节点插入某节点之前
 * @param[in]   pListNode   要插入位置的节点
 * @param[in]   pNewNode    新节点
 * @details     只插入节点;
 */
void XCList_InsertNodeBefore(XCListNode_t* pListNode, XCListNode_t* pNewNode);

/**
 * @brief       [内部]将节点移动到某个节点之前
 * @param[in]   pDestNode   目标节点
 * @param[in]   pSrcNode    需要移动的节点
 * @details
 *  将节点从原先链表中移除,并移动到目标节点之前;
 */
void XCList_MoveNodeBefore(XCListNode_t* pDestNode, XCListNode_t* pSrcNode);

/**
 * @brief       [内部]从链表中移除一个节点
 * @param[in]   pNode   需要删除的节点
 * @details     只处理链表节点部分,不影响节点挂载的其他数据;
 */
#define XCList_Remove(_pNode)                     \
    {                                             \
        /* 链接前后两个节点来删除节点*/           \
        (_pNode)->pNext->pPrev = (_pNode)->pPrev; \
        (_pNode)->pPrev->pNext = (_pNode)->pNext; \
        /* 初始化删除的节点*/                     \
        XCList_InitNode((_pNode));                \
    }

/************************************************ 我是分割线 ************************************************/
/**基础判断 */

/**
 * @brief       [内部]链表是否有效
 * @param[in]   _pList [XCListNode_t*]链表指针
 * @return      boot
 * @retval      0 : 没有节点
 * @retval      1 : 有节点
 * @details     判断一个链表的是否有效(是否有节点)
 */
#define XCList_ListValid(_pList)            ((_pList) != ((_pList)->pNext))

/**
 * @brief       [内部]节点是否到达结尾节点
 * @param[in]   _pList  [XCListNode_t*]链表指针
 * @param[in]   _pNode  [XCListNode_t*]节点指针
 * @return      boot
 * @retval      0 : 没有到达结尾节点
 * @retval      1 : 到达结尾节点(节点和链表根地址相同)
 * @details     用于遍历链表时,判断遍历的节点是否到达根节点(既是否结束遍历)
 */
#define XCList_ReachEndNode(_pList, _pNode) ((_pList) == (_pNode))

/**
 * @brief       [内部]获取链表的开始节点
 * @param[in]   _pList  [XCListNode_t*]链表地址
 * @return      XCListNode_t*   返回节点地址
 * @details     得到当前链表的开始地址
 */
#define XCList_GetListStartNode(_pList)     ((_pList)->pNext)

/************************************************ 我是分割线 ************************************************/
/**链表操作 */

/**
 * @brief       [内部]交换链表
 * @param[in]   pList1  链表1
 * @param[in]   pList2  链表2
 * @details     直接交换两个链表根节点的链接;交换后链表为空,则初始化;
 */
void XCList_SwapList(XCListNode_t* pList1, XCListNode_t* pList2);

/**
 * @brief       [内部]将一个链表全部移动到另个链表的一个节点前
 * @param[in]   pDestNode   需要移入链表的节点(链表将移动到此节点前)
 * @param[in]   pSrcList    需要移动的链表
 * @details  转移完节点后的链表会被清除;
 *  - 注意:不要移动自己;
 */
void XCList_SwapListToNodeBefore(XCListNode_t* pDestNode, XCListNode_t* pSrcList);

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
