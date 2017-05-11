B+-Tree Index: Implement a disk-based B+-tree index structure
---------------------------------------------------------------------------------------------------------------------------
*****************************************************************************************************************************************
Btree_mgr.c
*****************************************************************************************************************************************

Structures created in btree_mgr.c
------------------------------------

BT_MgmtInfo stores the information about a tree.
 
 
typedef enum NodeType {
	LEAF = 0,
	NON_LEAF = 1,
	ROOT = 2
} NodeType;

typedef struct BT_MgmtInfo
{
	int numKeys;
	int totalNumOfPages;
	int rootNode;
	BM_BufferPool *bm;
	BM_PageHandle *ph;

} BT_MgmtInfo;

***************************************************************************************************************************
Implementation:
***************************************************************************************************************************
-->Create Btree creates pagefile and delete Btree destroys that pagefile.
-->We are storing each node in each page of a pagefile as a string.
-->The first page of the file holds datatype of the keys and the n value(maximum number of keys per node);
-->Each page will have the following information:
	1) node type: leaf/root/non leaf
	2) parent page of the node.
	3.a) for leaf node: RID,Key and at the end next page
	3.b) for non leaf or root: previous child's page , key and next child's page.
-->Deletion, findKey and scan takes place from Root to  leaf(based on the key passed).
-->In insert: root node will be created once n+1 keys are inserted(node is split).
-->Default root node page is :2 
-->We are using buffer manager/ buffer pool to pin and unpin the required page and then copy the data to the page.

***************************************************************************************************************************
