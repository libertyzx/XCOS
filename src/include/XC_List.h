/**
 * @file        XC_List.h
 * @brief       链表操作的声明
 * @author      libertyzx (libertyzx@163.com)
 * @version     1.0
 * @date        2024/03/18
 * **********************************************
 * @copyright   Copyright (c) 2024 libertyzx. All rights reserved.
 * @license     This project is released under the MIT License.
 * **********************************************
 * @details     链表操作的函数声明及类型定义;
 *  本文件中所有类型都是内部使用;不建议用户使用;
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
 * @brief       [内部]双向链表节点类型
 * @details     32位下占12字节;
 */
typedef struct _XCListNode_t {
    struct _XCListNode_t* pNext;     // 指向下个节点
    struct _XCListNode_t* pPrevious; // 指向上个节点
    struct _XCListRoot_t* pRootList; // 指向节点所属的地址
} XCListNode_t;

/**
 * @brief       [内部]基础双向链表节点类型
 * @details     只有链表节点和辅助值;32位下占8字节;
 */
typedef struct {
    XCListNode_t* pNext;     // 指向下个节点
    XCListNode_t* pPrevious; // 指向上个节点
} XCListBasic_t;

/**
 * @brief       [内部]双向链表根节点类型
 * @details     32位下占8字节;
 */
typedef struct _XCListRoot_t {
    XCListBasic_t RootNode; // 根节点,最后/最初的节点
} XCListRoot_t;

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**函数宏 */

/**
 * @brief       [内部]链表是否有效
 * @param[in]   _pList [XCListRoot_t*]链表指针
 * @return      boot
 * @retval      0 : 没有节点
 * @retval      1 : 有节点
 * @details     判断一个链表的是否有效(是否有节点)
 */
#define XCList_ListValid(_pList)            (((XCListNode_t*)&((_pList)->RootNode)) != ((_pList)->RootNode.pNext))

/**
 * @brief       [内部]节点是否到达结尾节点
 * @param[in]   _pList  [XCListRoot_t*]链表指针
 * @param[in]   _pNode  [XCListNode_t*]节点指针
 * @return      boot
 * @retval      0 : 没有到达结尾节点
 * @retval      1 : 到达结尾节点(节点和链表根地址相同)
 * @details     用于遍历链表时,判断遍历的节点是否到达根节点(既是否结束遍历)
 */
#define XCList_ReachEndNode(_pList, _pNode) (((XCListNode_t*)&((_pList)->RootNode)) == (_pNode))

/**
 * @brief       [内部]获取链表的开始节点
 * @param[in]   _pList          [XCListRoot_t*]链表地址(会强制转为链表指针类型)
 * @return      XCListNode_t*   返回节点地址
 * @details     得到当前链表的开始地址
 */
#define XCList_GetListStartNode(_pList)     (((XCListRoot_t*)(_pList))->RootNode.pNext)

/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
/**函数声明 */

/************************************************ 我是分割线 ************************************************/
/**初始化操作 */

/**
 * @brief       [内部]初始化链表
 * @param[in]   pList   需要初始化的链表
 * @details     初始化链表时使用
 */
void XCList_Init(XCListRoot_t* pList);

/**
 * @brief       [内部]初始化节点
 * @param[in]   pNode   需要初始化的节点
 * @details     初始化节点时使用
 */
void XCList_InitNode(XCListNode_t* pNode);

/************************************************ 我是分割线 ************************************************/
/**链表节点操作 */

/**
 * @brief       [内部]将新节点插入某节点之前
 * @param[in]   pListNode   要插入位置的节点
 * @param[in]   pNewNode    新节点
 * @details     只插入节点;
 */
void XCList_InsertNodePrevious(XCListNode_t* pListNode, XCListNode_t* pNewNode);

/**
 * @brief       [内部]将新节点插入某节点之后
 * @param[in]   pListNode   要插入位置的节点
 * @param[in]   pNewNode    新节点
 * @details     只插入节点;
 */
void XCList_InsertNodeNext(XCListNode_t* pListNode, XCListNode_t* pNewNode);

/**
 * @brief       [内部]连接2个节点
 * @param[in]   pPreviousNode   上个节点
 * @param[in]   pNextNode       下个节点
 * @details     调用此函数后会删除参数中两个节点间的所有节点
 */
void XCList_LinkNode(XCListNode_t* pPreviousNode, XCListNode_t* pNextNode);

/**
 * @brief       [内部]移除基础节点
 * @param[in]   pNode   需要删除的节点
 * @details     只处理链表节点部分,不影响节点挂载的其他数据;
 */
void XCList_RemoveBasicNode(XCListBasic_t* pNode); // 移除一个基础节点

/************************************************ 我是分割线 ************************************************/
/**链表操作 */

/**
 * @brief       [内部]从链表中移除一个节点
 * @param[in]   pNode   需要删除的节点
 * @details     只处理链表节点部分,不影响节点挂载的其他数据;
 */
void XCList_Remove(XCListNode_t* pNode); // 从链表中移除一个节点

/**
 * @brief       [内部]将节点插入链表的开始
 * @param[in]   pList       需插入的链表
 * @param[in]   pNewNode    新节点
 * @details     插入RootNode头;
 */
void XCList_InsertStart(XCListRoot_t* pList, XCListNode_t* pNewNode); // 节点插入链表开始

/**
 * @brief       [内部]将节点插入链表的结尾
 * @param[in]   pList       需插入的链表
 * @param[in]   pNewNode    新节点
 * @details     插入RootNode尾;
 */
void XCList_InsertEnd(XCListRoot_t* pList, XCListNode_t* pNewNode); // 节点插入链表结尾

/**
 * @brief       [内部]交换链表
 * @param[in]   pList1  链表1
 * @param[in]   pList2  链表2
 * @details     直接交换两个链表根节点的链接;交换后链表为空,则初始化;
 */
void XCList_SwapList(XCListRoot_t* pList1, XCListRoot_t* pList2); // 交换2个链表

/**
 * @brief       [内部]将一个链表全部移动到另个链表的一个节点前
 * @param[in]   pDestNode   需要移入链表的节点(链表将移动到此节点前)
 * @param[in]   pSrcList    需要移动的链表
 * @details  转移完节点后的链表会被清除;
 *  - 注意:不要移动自己;
 */
void XCList_SwapListToNodePrevious(XCListNode_t* pDestNode, XCListRoot_t* pSrcList); // 将一个链表全部移动到一个节点前

/************************************************ 我是分割线 ************************************************/
/*
 ************************************************************************************************************|
 ************************************************ 我是分割线 ************************************************|
 ************************************************************************************************************|
 */
//=== 文件结束
#endif
