#include<stdio.h>
#include<stdlib.h>
#include"buffer_mgr.h"
#include"storage_mgr.h"
#include"record_mgr.h"
#include"btree_mgr.h"
#include <math.h>


// structure to keep track of num nodes traversed in for splitting
typedef struct btStack {
	int pageNum;			// stores the page number of the node
	int index;				// stores which index of the key within the node is traversed
} btStack;


// Management structure for B-Tree
typedef struct BtreeManager {
	DataType keyType;
	int numKeys;		// number of keys that can be stored in a node
	int treeRoot;		// tree root node num
	int recentPage;		// indicates the latest node
	int numEntries; 	// number of keys total in the Btree
	int numNodes;
	// pages and type
	btStack stack[100];
} BtreeManager;

// Node structure
typedef struct Node {
	bool leaf;	// true if node is a leaf node
	// Node information
	int numKeys;	// indicates the number of keys currently in the node (0 1 or 2)
	RID rid[2];
	Value key[2];
	// nextLeafNode has different behavior for a leaf node and for a non leaf node
	// for a leaf node it will inidicate the right sibling (once at end will loop back)
	// for a non leaf node it will indicate the right child	
	int nextLeafNode;
	// nonleaf node - [node, value, node, value, node]
	// leaf node - [rid, value, rid, value, next leaf node]
	int node[3];		// this holds the node page num of the current node's children
} Node;

typedef struct BtreeScanManager {
	int curNode;		// indicates the page num of the Current Node being scanned
	int curKey;			// indicates the current key num inside the current node
} BtreeScanManager;

// init and shutdown index manager
extern RC initIndexManager(void* mgmtData) {
	return RC_OK;
}
extern RC shutdownIndexManager() {
	return RC_OK;
}


// create, destroy, open, and close an btree index
extern RC createBtree(char* idxId, DataType keyType, int n) {
	// createBtree needs to initialize the BtreeManager structure
	// after initializing it, we need to initialize the BufferPool that is going to hold our index
	// in the first page of the bufferPool identified by idxId, the BtreeManager structure is going to reside
	// once the BtreeManager structure is in the first page, we need to allocate an empty node into the tree
	// this node will be in a different page so we create the page 
	// set up buffer pool and handle 

	// initialize Btree manager
	BtreeManager* btmanager = (BtreeManager*)malloc(sizeof(BtreeManager));
	//finally set up the btree structure we defined 
	btmanager->keyType = keyType;
	btmanager->numKeys = n;			// number of keys that each node in the tree can hold is n
	btmanager->treeRoot = 1;		// tree root starts at page 1, because page 0 is the btree manager struct
	btmanager->recentPage = 0;		
	btmanager->numEntries = 0;		// currently 0 entries in tree  
	btmanager->numNodes = 0;

	// initialize BufferPool
	BM_BufferPool* bp = (BM_BufferPool*)malloc(sizeof(BM_BufferPool));
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	createPageFile(idxId); // create page file using the passed pointer
	if (initBufferPool(bp, idxId, 100, RS_FIFO, NULL) == RC_FILE_NOT_FOUND) {
		printf("initBuffer Failed.");
		// error checking 
		return RC_BTREE_CREATION_ERROR;
	};

	// need to pin the page  while we work on it
	pinPage(bp, bph, 0);		// put page 0 of buffer pool into bph


	// set empty node in page 1 of buffer pool
	// allocate page for first node
	BM_PageHandle* ph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	// set recent page to 1, the page with the first node
	btmanager->recentPage = 1;

	// pin page as we will modify it
	pinPage(bp, ph, btmanager->recentPage);		// put page 1 of buffer pool into ph

	//update btree struc
	btmanager->numNodes = 1;				// btree manager update to one node

	// initialize the first node, first get the data from the page handle then update attr
	Node* initnode = (Node*)ph->data;
	initnode->leaf = true;		 // first node is a leaf node
	initnode->nextLeafNode = 0;		
	initnode->numKeys = 0;
	/*for (int i = 0; i < n; i++) {
		initnode->key[i].v.intV = NULL;
		initnode->key[i].dt = keyType;
		initnode->rid[i].page = NULL;
		initnode->rid[i].slot = NULL;
	}*/

	// indicate that the page has been updated
	markDirty(bp, ph);
	unpinPage(bp, ph);

	// this must be done after inserting empty node so that the manager struct is updated
	// insert BtreeManager inside bph 
	memcpy(bph->data, btmanager, sizeof(BtreeManager));
	markDirty(bp, bph);
	unpinPage(bp, bph);

	//clean up
	if (shutdownBufferPool(bp) != RC_OK) {
		// debugging
		printf("shutdownBuffer Failed.");
		// error checking 
		return RC_BTREE_CREATION_ERROR;
	}

	free(bph);
	free(bp);
	free(ph);
	free(btmanager);

	// btree created
	return RC_OK;
}
extern RC openBtree(BTreeHandle** tree, char* idxId) {
	// openBtree needs to open the buffer pool by name idxId
	// then we must retrieve the BtreeManager struct from page 0 in the buffer pool
	// then we must update the tree handle passed in so that when we return success, the user can access the btree

	// error checking
	if (tree == NULL)
		return RC_NO_TREE;
	if (idxId == NULL)
		return RC_ID_NULL;

	// open buffer pool by the name idxId
	BM_BufferPool* bp = (BM_BufferPool*)malloc(sizeof(BM_BufferPool));
	if (initBufferPool(bp, idxId, 100, RS_FIFO, NULL) == RC_FILE_NOT_FOUND) {
		printf("initBuffer Failed.");
		// error checking 
		return RC_BTREE_OPEN_ERROR;
	};
	// retrieve BtreeManager struct from page 0 
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	// need to pin the page bc we opened it (might also work on it)
	pinPage(bp, bph, 0);			// bph holds page 0 of the buffer pool, which is the manager struct


	//set the tree handle up using our structure and the info passed into function
	BTreeHandle* btree = (BTreeHandle*)malloc(sizeof(BTreeHandle));
	btree->mgmtData = bp;			// tree handle mgmtData holds the 
	btree->keyType = ((BtreeManager*)bph->data)->keyType;
	btree->idxId = idxId;


	//set the btreehandle to the pointer
	*tree = btree;

	//page handling + cleanup
	unpinPage(bp, bph);
	free(bph);

	//successfully opened the tree
	return RC_OK;
}
extern RC closeBtree(BTreeHandle* tree) {
	// need to shutdown the buffer pool and clean up 
	// before closing the tree must be written back to the disk
	shutdownBufferPool((BM_BufferPool*)tree->mgmtData);
	free(tree->mgmtData);
	free(tree);
	return RC_OK;
}

extern RC deleteBtree(char* idxId) {
	///deletes tree only if file exists
	if (destroyPageFile(idxId) == RC_OK)
		return RC_OK;
	else
		return RC_BTREE_DELETION_ERROR;
};


// access info about b-tree
// getNumNodes will store the keyType in result
// to retrieve getNumNodes all we need to do is access our management structure within each tree
// each tree should have the management structure in it, in the first page of the buffer pool inside attr mgmtData
extern RC getNumNodes(BTreeHandle* tree, int* result) {
	// get buffer pool from tree handle mgmtData
	BM_BufferPool* bp = (BM_BufferPool*)tree->mgmtData;

	// malloc page handle to store the management structure
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));

	pinPage(bp, bph, 0);		// reads management structure into bph

	//set the data to our defined structure that manages btree
	BtreeManager* btmanager = (BtreeManager*)bph->data;

	// since result pointer stores the count, we update it
	*result = btmanager->numNodes;

	// clean up
	unpinPage(bp, bph);
	free(bph);

	//return okay
	return RC_OK;
}

// getNumEntries will store the numEntries in result
// to retrieve getNumEntries all we need to do is access our management structure within each tree
// each tree should have the management structure in it, in the first page of the buffer pool inside attr mgmtData
extern RC getNumEntries(BTreeHandle* tree, int* result) {
	//similar implementation as num nodes
	BM_BufferPool* bp = (BM_BufferPool*)tree->mgmtData;
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));

	// need to pin the page  while we use it
	pinPage(bp, bph, 0);

	//set the data to our defined structure that manages btree
	BtreeManager* btmanager = (BtreeManager*)bph->data;

	// since result pointer stores the count, we update it
	*result = btmanager->numEntries;

	// clean up
	unpinPage(bp, bph);
	free(bph);

	//return okay
	return RC_OK;
}

// getKeyType will store the keyType in result
// to retrieve getKeyType all we need to do is access our management structure within each tree
// each tree should have the management structure in it, in the first page of the buffer pool inside attr mgmtData
extern RC getKeyType(BTreeHandle* tree, DataType* result) {
	//similar implementation as other get functions
	BM_BufferPool* bp = (BM_BufferPool*)tree->mgmtData;
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));

	// need to pin the page  while we use it
	pinPage(bp, bph, 0);

	//set the data to our defined structure that manages btree
	BtreeManager* btmanager = (BtreeManager*)bph->data;

	// since result pointer stores the count, we update it
	*result = btmanager->keyType;

	// clean up
	unpinPage(bp, bph);
	free(bph);

	//return okay
	return RC_OK;
}


// index access
extern RC findKey(BTreeHandle* tree, Value* key, RID* result) {
	// find key will find the RID for entry with key "key"
	// if found, the function needs to store the RID in the inputted value
	// if found returns RC_OK
	// else return RC_IM_KEY_NOT_FOUND

		//Retrieve BtreeManger from BufferPool page 0
	BM_BufferPool* bp = (BM_BufferPool*)tree->mgmtData;
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, bph, 0);
	BtreeManager* btmanager = (BtreeManager*)bph->data;

	// get root node
	BM_PageHandle* tempnode = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, tempnode, btmanager->treeRoot);

	// while currently pinned page isn't a leaf
	while (!(((Node*)tempnode->data)->leaf)) {
		int subtree;
		// in this loop we check for the first key, starting from the end, that is less than the insertion key 
		// so that we can enter the correct subtree
		// Example:
		//			2 keys - (subtree 0, key 0, subtree 1, key 1, subtree 2)
		// 		
		// 		on the first iteration, 
		//								subtree = numKeys = 2
		//								key[subtree - 1] = key[1]
		//					if key[1] < insertion key, then we must enter the subtree in subtree 2
		// 					if key[1] > insertion key, then the subtree we must enter is either subtree 1 or subtree 0
		//		second iteration
		//								subtree = 1
		//								key[subtree - 1] = key[0]
		//					if key[0] < insertion key, then we must enter the subtree in subtree 1
		//					if key[0] > insertion key, then we must enter the subtree in subtree 0
		for (subtree = ((Node*)tempnode->data)->numKeys; subtree >= 1; subtree--) {
			if (((Node*)tempnode->data)->key[subtree - 1].v.intV <= key->v.intV) {
				break;
			}
		}
		// Now we must enter that subtree and verify that itmin is a leaf node, if not we repeat the process
		int tempNodePage = ((Node*)tempnode->data)->node[subtree];
		unpinPage(bp, tempnode);
		pinPage(bp, tempnode, tempNodePage);
	}
	// We are now at the correct leaf node
	// we must find its position within the node
	int i = ((Node *)tempnode->data)->numKeys;
	while(i>=1 && ((Node*)tempnode->data)->key[i-1].v.intV > key->v.intV){
		i--;
	}

	if (((Node*)tempnode->data)->key[i-1].v.intV == key->v.intV) {
		// if match write to result
		*result = ((Node*)tempnode->data)->rid[i-1];
		// unpin the pages that we used and free
		unpinPage(bp, tempnode);
		unpinPage(bp, bph);
		free(tempnode);
		free(bph);
		// find key successful
		return RC_OK;
	}
	else {
		// if no match
		// unpin pages that we used and free
		unpinPage(bp, tempnode);
		unpinPage(bp, bph);
		free(tempnode);
		free(bph);
		// find key not successful
		return RC_IM_KEY_NOT_FOUND;
	}

}

extern RC insertKey(BTreeHandle* tree, Value* key, RID rid) {
	// insertKey will search the tree for the correct spot,
	// if there is space in the node, store the key and RID in the node
	// if there is no space in the node split the nodes and place the keys appropriately
	// if there is underflow, attempt to redistribute values from a sibling(nodes immediately left or right)
	// if a nonleaf node needs to be split and n is odd, 
	// the middle value must be inserted into the parent

		//Retrieve BtreeManger from BufferPool page 0
	BM_BufferPool* bp = (BM_BufferPool*)tree->mgmtData;
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, bph, 0);
	BtreeManager* btmanager = (BtreeManager*)bph->data;

	// get root node
	BM_PageHandle* tempnode = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, tempnode, btmanager->treeRoot);

	// split node count
	int counter = 0;
	// while currently pinned page isn't a leaf
	while (!(((Node*)tempnode->data)->leaf)) {
		int subtree;
		// in this loop we check for the first key, starting from the end, that is less than the insertion key 
		// so that we can enter the correct subtree
		// Example:
		//			2 keys - (subtree 0, key 0, subtree 1, key 1, subtree 2)
		// 		
		// 		on the first iteration, 
		//								subtree = numKeys = 2
		//								key[subtree - 1] = key[1]
		//					if key[1] < insertion key, then we must enter the subtree in subtree 2
		// 					if key[1] > insertion key, then the subtree we must enter is either subtree 1 or subtree 0
		//		second iteration
		//								subtree = 1
		//								key[subtree - 1] = key[0]
		//					if key[0] < insertion key, then we must enter the subtree in subtree 1
		//					if key[0] > insertion key, then we must enter the subtree in subtree 0
		for (subtree = ((Node*)tempnode->data)->numKeys; subtree >= 1; subtree--) {
			if (((Node*)tempnode->data)->key[subtree - 1].v.intV <= key->v.intV) {
				break;
			}
		}
		// store information in case we need to split
		btmanager->stack[counter].pageNum = tempnode->pageNum;
		btmanager->stack[counter].index = subtree;
		counter++;
		// Now we must enter that subtree and verify that it is a leaf node, if not we repeat the process
		int tempNodePage = ((Node*)tempnode->data)->node[subtree];
		unpinPage(bp, tempnode);
		pinPage(bp, tempnode, tempNodePage);
	}
	// We are now at the correct leaf node, we must decide whether to insert or to split
	// to decide whether to insert or to split depends on the nodes current number of keys
	// if the number of keys in the node is not less than the max num then we must split
	if (((Node*)tempnode->data)->numKeys < btmanager->numKeys) {
		insertLeaf(((Node*)tempnode->data), key, rid);
	}
	else {
		// split node
		splitNode(((Node*)tempnode->data), bp, btmanager, counter, key, rid, 0);
	}

	// After insertion we must update the BtreeManager struct
	btmanager->numEntries++;

	/*BM_PageHandle* test = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,test,2);
	Node* testnode = (Node*)test->data;
	unpinPage(bp,test);
	free(test);*/

	// write updated BtreeManager to disk
	markDirty(bp, bph);
	unpinPage(bp, bph);

	// write updated Node to disk
	markDirty(bp, tempnode);
	unpinPage(bp, tempnode);

	free(bph);
	free(tempnode);

	// successfully inserted node
	return RC_OK;
}

extern RC insertLeaf(Node* node, Value* key, RID rid) {
	// 2 cases
	// We must check the number of keys currently in the node
	// Case 1:
	// There are no keys in the node
	//		This means that we must insert the key and the rid at slot 0 in node
	// Case 2:
	// There are keys in the node
	//		This means that we must find the spot for the key
	// 		to find the correct spot we must order the keys
	//		to order the keys we do a substitution if the right key is less than the left key

	int numKeys = node->numKeys;

	// Case 1
	if (numKeys == 0) {
		// all data types for every node will be INT so no need to update
		node->key[0].v.intV = key->v.intV;
		node->key[0].dt = key->dt;
		node->rid[0].page = rid.page;
		node->rid[0].slot = rid.slot;
		node->numKeys++;
	}
	// Case 2
	else {
		// we initialize i to be the number of keys
		int i;

		// in this loop we check for the first key, starting from the end, that is less than the insertion key
		// this will determine the position of the insertion onto the leaf node
		// i can never be equal to 2, in that case we should have entered split node
		// Example:
		//			1 key - (rid 0, key 0, empty rid 1, empty key 1, next leaf node)
		// 		
		// 		on the first iteration, 
		//								i = num keys, since there is only 1, i = 1
		//								key[i - 1] = key[0]
		//					if key[0] < insertion key, then we must place the key and rid in slot 1
		// 					if key[0] > insertion key, then we must place the key and rid in slot 0
		//											this means that we must also move what is in slot 0 to slot 1, what's in slot 1 to slot 2 etc
		//
		for (i = numKeys; i >= 1; i--) {
			if ((node->key[i - 1].v.intV == key->v.intV) && (node->rid[i - 1].page == rid.page) && (node->rid[i - 1].slot == rid.slot)) {
				return RC_IM_KEY_ALREADY_EXISTS;
			}
			if (node->key[i - 1].v.intV < key->v.intV) {
				break;
			}
			node->key[i].v.intV = node->key[i - 1].v.intV;
			node->key[i].dt = node->key[i - 1].dt;
			node->rid[i].page = node->rid[i - 1].page;
			node->rid[i].slot = node->rid[i - 1].slot;
		}
		// now that the position is determined we insert
		node->key[i].v.intV = key->v.intV;
		node->key[i].dt = key->dt;
		node->rid[i].page = rid.page;
		node->rid[i].slot = rid.slot;
		node->numKeys++;
	}
	// insertion to leaf successful
	return RC_OK;
}

extern BM_PageHandle* createNode(BM_BufferPool* bp, BtreeManager* btmanager) {
	// allocate the page the new node will reside
	BM_PageHandle* ph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	// update recent page
	btmanager->recentPage++;
	// update num nodes
	btmanager->numNodes++;
	// retrieve the page to update the node
	pinPage(bp,ph,btmanager->recentPage);
	Node* newnode = (Node *)ph->data;
	// initialize new nodes data
	newnode->leaf = true;
	newnode->numKeys = 0;
	newnode->nextLeafNode = 0;		
	return ph;
}


extern RC splitNode(Node* child, BM_BufferPool* bp, BtreeManager* btmanager, int counter, Value *key, RID rid, int existPageNum) {				
	// splitNode has 2 cases
	// Case 1:
	// 			the Leaf split does not cause a non-leaf split
	// Case 2:
	//			the Leaf split causes the non leaf to also split above it

	RID* newRID = (RID *)malloc(sizeof(RID));
	Value* newVal = (Value *)malloc(sizeof(Value));
	Value *tempVal = (Value *)malloc(sizeof(Value));
	// Case 1:
	if(counter == 0){
		// allocate nodes to split
		BM_PageHandle* ph = createNode(bp,btmanager);
		BM_PageHandle* ph2 = createNode(bp,btmanager);
		Node* parent = (Node *)ph->data;
		Node* sibling = (Node *)ph2->data;
		parent->leaf = false;
		parent->node[0] = btmanager->treeRoot;
		parent->node[1] = ph2->pageNum;
		sibling->numKeys = 1;
		parent->numKeys = 1;
		// first iteration with child being the root leaf
		if(child->leaf){
			updateNode(child,key,&rid,newVal,newRID);
			// insert record into sibling leaf
			sibling->key[0].v.intV = newVal->v.intV;
			sibling->key[0].dt = newVal->dt;
			sibling->rid[0].page = newRID->page;
			sibling->rid[0].slot = newRID->slot;
			// insert key into parent
			parent->key[0].v.intV = newVal->v.intV;
			parent->key[0].dt = newVal->dt;
			// assign the next leaf node to be the sibling
			child->nextLeafNode = ph2->pageNum;				// sibling pagenum
		}
		else{
			sibling->key[0].v.intV = key->v.intV;
			sibling->key[0].dt = key->dt;
			// non leaf node
			sibling->leaf = false;
			sibling->node[0] = existPageNum;
			sibling->node[1] = child->node[child->numKeys];
			parent->key[0].v.intV = child->key[btmanager->numKeys-1].v.intV;
			parent->key[0].dt = child->key[btmanager->numKeys-1].dt;
			child->key[btmanager->numKeys - 1].v.intV = 0;
			child->node[btmanager->numKeys] = 0;
			child->numKeys--;
		}
		// assign the new root
		btmanager->treeRoot = ph->pageNum; 		// parent pagenum
		// write back to disk and free
		markDirty(bp,ph);
		unpinPage(bp,ph);
		markDirty(bp,ph2);
		unpinPage(bp,ph2);
		free(newRID);
		free(newVal);
		free(ph);
		free(ph2);
	}
	// Case 2
	else {
		BM_PageHandle* ph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
		BM_PageHandle* ph2 = createNode(bp,btmanager);
		pinPage(bp,ph,btmanager->stack[counter - 1].pageNum); 	//pin parent to ph
		Node* parent = (Node *)ph->data;
		Node* sibling = (Node *)ph2->data;
		sibling->numKeys = 1;
		if(child->leaf){
			updateNode(child,key,&rid,newVal,newRID);
			sibling->key[0].v.intV = newVal->v.intV;
			sibling->key[0].dt = newVal->dt;
			sibling->rid[0].page = newRID->page;
			sibling->rid[0].slot = newRID->slot;
			child->nextLeafNode = ph2->pageNum;			// next leaf node is sibling pagenum
			if(btmanager->stack[counter-1].index + 1 <= btmanager->numKeys){
				// assign the new siblings next leaf node to parents child
				sibling->nextLeafNode = parent->node[btmanager->stack[counter-1].index + 1];
			}
		}
		else{
			sibling->key[0].v.intV = newVal->v.intV;
			sibling->key[0].dt = newVal->dt;
			sibling->leaf = false;
			sibling->node[0] = child->node[btmanager->numKeys];
		}
		if(parent->numKeys < btmanager->numKeys && child->leaf){
			// split node is a leaf node
			existPageNum = redistributeKeys(parent,btmanager->stack[counter-1].index, &(sibling->key[0]), btmanager->recentPage, newVal);
		}
		else if(parent->numKeys < btmanager->numKeys) {
			// split node is not a leaf node
			redistributeKeys(parent,btmanager->stack[counter-1].index, &(child->key[btmanager->numKeys - 1]), btmanager->recentPage, newVal);
			child->numKeys--;
		}
		else {
			existPageNum = newNonLeafUpdate(parent, newVal, btmanager->recentPage, tempVal);
			counter--;
			splitNode(parent,bp,btmanager,counter, tempVal ,rid,existPageNum);
		}
		markDirty(bp,ph2);
		unpinPage(bp,ph2);
		free(newRID);
		free(ph2);
		free(newVal);
		free(tempVal);
		// write back to disk and free
		markDirty(bp,ph);
		unpinPage(bp,ph);
		free(ph);
	}
	return RC_OK;
}

extern RC newNonLeafUpdate(Node* node, Value* key, int pageNum, Value* newVal){
	newVal->v.intV = node->key[node->numKeys-1].v.intV;
	int result;
	if (node->key[0].v.intV > key->v.intV){
		node->key[node->numKeys-1].v.intV = node->key[0].v.intV;
		node->key[0].v.intV = key->v.intV;
		result = node->node[1];		// get the middle subtree index of the parent
		node->node[1] = pageNum;
	}
	else{
		node->key[node->numKeys-1].v.intV = key->v.intV;
		result = node->node[1];		// get the middle subtree index of the parent
		node->node[1] = pageNum;
	}
	return result;
}

extern RC updateNode(Node* node, Value* key, RID* rid, Value* newVal, RID* newRID){
	// store key and rid in newval and newrid
	memcpy(newVal,key,sizeof(Value));
	memcpy(newRID,rid, sizeof(RID));			// memcpy(dst,src,size)
	int i;
	// move to correct spot
	for(i = 0; i < node->numKeys; i++){
		if(node->key[i].v.intV > key->v.intV){
			break;
		}
	}
	if(i != node->numKeys){
		int j = node->numKeys - 1;
		newVal->v.intV = node->key[j].v.intV;
		newRID->page = node->rid[j].page;
		newRID->slot = node->rid[j].slot;
		while(j > i){
			// begin swapping to update all keys ahead of correct spot
			node->key[j].v.intV = node->key[j-1].v.intV;
			node->rid[j].page = node->rid[j-1].page;
			node->rid[j].slot = node->rid[j-1].slot;
			j--;
		}
		node->key[i].v.intV = key->v.intV;
		node->rid[i].page = rid->page;
		node->rid[i].slot = rid->slot;
	}
	// successfully updated Node
	return RC_OK;
}
extern RC redistributeKeys(Node* node, int index, Value* key, int pageNum, Value* newVal) {		
	int i = node->numKeys;
	int returnPageNum;
	memcpy(newVal,&(node->key[i-1]), sizeof(Value));
	returnPageNum = node->node[i];
	if(index < node->numKeys){
		// rearrange and place
		while(i != index){
			// rearrange
			node->key[i].v.intV = node->key[i-1].v.intV;
			node->node[i+1] = node->node[i];
			i--;
		}
		// update the node to redistributed values
		node->key[index].v.intV = key->v.intV;
		node->key[index].dt = key->dt;
		node->node[index + 1] = pageNum;
		node->numKeys++;
	}
	else{
		// just place
		node->key[index].v.intV = key->v.intV;
		node->key[index].dt = key->dt;
		node->node[index+1] = pageNum;
		node->numKeys++;
	}
	return returnPageNum;
}


extern RC deleteKey(BTreeHandle* tree, Value* key) {
	// deleteKey will search the tree for the correct value
	// if found, delete.
	// after deletion, if there is overflow attempt to redistribute
	// if redistribution fails merge the node with a sibling
	// if key is not found, return fail condition

		//Retrieve BtreeManger from BufferPool page 0
	BM_BufferPool* bp = (BM_BufferPool*)tree->mgmtData;
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, bph, 0);

	// set our btreemanager to the data from the page
	BtreeManager* btmanager = (BtreeManager*)bph->data;

	// get root node
	BM_PageHandle* tempnode = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, tempnode, btmanager->treeRoot);

	int t, tPage;
	while (!((Node*)tempnode->data)->leaf) {
		t = ((Node*)tempnode->data)->numKeys;
		while (((Node*)tempnode->data)->key[t - 1].v.intV > key->v.intV && t >= 1) {
			t--;
		}
		tPage = ((Node*)tempnode->data)->node[t];
		unpinPage(bp, tempnode);
		pinPage(bp, tempnode, tPage);
	}

	t = 0;
	while (((((Node*)tempnode->data))->numKeys > t) && (((Node*)tempnode->data)->key[t].v.intV != key->v.intV))
	{
		t++;
	}

	if (t < ((Node*)tempnode->data)->numKeys) {
		while (t < (((Node*)tempnode->data)->numKeys - 1)) {
			((Node*)tempnode->data)->key[t].v.intV = ((Node*)tempnode->data)->key[t + 1].v.intV;
			((Node*)tempnode->data)->rid[t].page = ((Node*)tempnode->data)->rid[t + 1].page;
			((Node*)tempnode->data)->rid[t].slot = ((Node*)tempnode->data)->rid[t + 1].slot;
			t++;
		}
		((Node*)tempnode->data)->numKeys--;

	}
	else {
		// write updated BtreeManager to disk
		markDirty(bp, bph);
		unpinPage(bp, bph);

		// write updated Node to disk
		markDirty(bp, tempnode);
		unpinPage(bp, tempnode);

		free(bph);
		free(tempnode);

		return RC_BTREE_KEY_ERROR;
	}

	if (((Node*)tempnode->data)->numKeys < 1) {
		// Here, we would need to use the redistribute keys function
		// fix me
	}


	// After insertion we must update the BtreeManager struct
	btmanager->numEntries--;

	// write updated BtreeManager to disk
	markDirty(bp, bph);
	unpinPage(bp, bph);

	// write updated Node to disk
	markDirty(bp, tempnode);
	unpinPage(bp, tempnode);

	free(bph);
	free(tempnode);

	return RC_OK;

}




extern RC openTreeScan(BTreeHandle* tree, BT_ScanHandle** handle) {
	// openTreeScan initializes the scan that is used to scan the entries in the tree
	// Basically go to the first (leftmost) leaf node and initialize the scan structure

		//Retrieve BtreeManger from BufferPool page 0
	BM_BufferPool* bp = (BM_BufferPool*)tree->mgmtData;
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, bph, 0);
	BtreeManager* btmanager = (BtreeManager*)bph->data;

	// get root node
	BM_PageHandle* tempnode = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, tempnode, btmanager->treeRoot);

	/*
	BM_PageHandle* test1 = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,test1,1);
	Node* testnode1 = (Node*)test1->data;
	BM_PageHandle* test2 = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,test2,2);
	Node* testnode2 = (Node*)test2->data;
	BM_PageHandle* test3 = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,test3,3);
	Node* testnode3 = (Node*)test3->data;
	BM_PageHandle* test4 = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,test4,4);
	Node* testnode4 = (Node*)test4->data;
	BM_PageHandle* test5 = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,test5,5);
	Node* testnode5 = (Node*)test5->data;
	BM_PageHandle* test6 = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,test6,6);
	Node* testnode6 = (Node*)test6->data;
	BM_PageHandle* test7 = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,test7,7);
	Node* testnode7 = (Node*)test7->data;
	unpinPage(bp, test1);
	unpinPage(bp, test2);
	unpinPage(bp, test3);
	unpinPage(bp, test4);
	unpinPage(bp, test5);
	unpinPage(bp, test6);
	unpinPage(bp, test7);
	free(test1);
	free(test2);
	free(test3);
	free(test4);
	free(test5);
	free(test6);
	free(test7);
*/
	// if the root node is a leaf, skip finding the correct leaf
	if (!(((Node*)tempnode->data)->leaf)) {
		// while currently pinned page isn't a leaf
		while (!(((Node*)tempnode->data)->leaf)) {
			// the leftmost subtree will always be on node[0]
			int tempNodePage = ((Node*)tempnode->data)->node[0];
			unpinPage(bp, tempnode);
			pinPage(bp, tempnode, tempNodePage);
		}
	}
	// allocate space for the scan manager
	BtreeScanManager* scanManager = (BtreeScanManager*)malloc(sizeof(BtreeScanManager));
	scanManager->curKey = 0;						// start scan at the 0th key
	scanManager->curNode = tempnode->pageNum;		// start scan at the page where the leftmost node is

	// allocate space for the ScanHandle
	BT_ScanHandle* result = (BT_ScanHandle*)malloc(sizeof(BT_ScanHandle));
	result->tree = tree;
	result->mgmtData = scanManager;

	// assign result to user input scanHandle
	*handle = result;

	//unpin pages that were used and free
	unpinPage(bp, tempnode);
	unpinPage(bp, bph);
	free(tempnode);
	free(bph);

	return RC_OK;
}
extern RC nextEntry(BT_ScanHandle* handle, RID* result) {
	// nextEntry when first called will retrieve the first rid in the first leaf node,
	// then it will update the scan structure that was initialized in opentreescan
	// so that when we call nextEntry again, we will retrieve the second rid in the first leaf node
	// If when we call nextEntry, the scan structure indicates that we are on the second key
	// we must traverse to the next leaf node and store the next key in result,
	// the only way this fails is if there are no nodes in the tree

		// Retrieve the scanManager structure
	BtreeScanManager* scanManager = (BtreeScanManager*)handle->mgmtData;

	// if we are on page 0, this means we are at btreemanager page and therefore no more entries exist
	if (scanManager->curNode == 0) {
		return RC_IM_NO_MORE_ENTRIES;
	}


	BM_BufferPool* bp = (BM_BufferPool *)((BTreeHandle *)handle->tree)->mgmtData;
	BM_PageHandle* ph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp,ph,scanManager->curNode);		// pins current node onto ph

	Node* node = (Node *)ph->data;
	int curEntry = scanManager->curKey;

	*result = ((Node*)node)->rid[curEntry];
	curEntry++;
	// Two cases
	// Case 1
	// 			[rid, key, rid, key, next leaf node]
	// 			After assigning first rid, there are still rids in the leaf node 
	// 			this means that we need to only increment curKey
	// 			and we must stay in the same node
	if(curEntry <= node->numKeys){
		// There are still rids in the leaf node
		scanManager->curKey = curEntry;
		unpinPage(bp, ph);
		free(ph);
		return RC_OK;
	}
	// Case 2
	// If there are no more rids in the node
	// we must reinitialize the current key to 0, and we must traverse to the next node
	scanManager->curKey = 0;
	scanManager->curNode = node->nextLeafNode;
	unpinPage(bp,ph);
	free(ph);
	return nextEntry(handle,result);

}

extern RC closeTreeScan(BT_ScanHandle* handle) {
	//since we are closing the scan handle, all we need to do is clean up aka free
	handle->mgmtData = NULL;
	//free(handle->tree);
	free(handle);
	//return ok if successfully closed
	return RC_OK;
}

// debug and test functions
extern char* printTree(BTreeHandle* tree) {
	// print tree prints a list of nodes
	// each node is a list of keys and pointers
	// example
	// NON LEAF NODE- 
	// (pointer to left child), (key), (pointer to middle child), (key), (pointer to right child)
	// LEAF NODE-
	// (r1), (key1), (r2), (key2), (pointer to next leaf)
	// r1 - record 1

	//Retrieve BtreeManger from BufferPool page 0
	BM_BufferPool* bp = (BM_BufferPool*)tree->mgmtData;
	BM_PageHandle* bph = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, bph, 0);
	BtreeManager* btmanager = (BtreeManager*)bph->data;

	// get root node
	BM_PageHandle* tempnode = (BM_PageHandle*)malloc(sizeof(BM_PageHandle));
	pinPage(bp, tempnode, btmanager->treeRoot);

	// check if the root is empty
	if (tempnode == NULL) {
		return "The BTree is empty!";
	}
	printf("The BTree:\n");
	
	Node *node = (Node *)tempnode->data;

	
	

}
