B+ Tree - Assignment 4
...............................................................................................................................
Manuel Lopez    - A20411680
Mohammed Chisti - A20393790

...............................................................................................................................
Building and Testing Instructions

Head to the assignment 4 directory
	(If you can see this you are at the right place)
Input "make" to compile the assignment (ignore the warnings the gcc is giving for memcpy)
	This results in two executable files: test4 and testexpr
	test4: this tests the project using test_assign4_1 file
	testexpr: this tests the project using test_expr.c file
Chose which executable
	For test4 input "./test4"
	For testexpr input "./testexpr"
After testing, cleanup by inputting "make clean"

*** fully functional ***


...............................................................................................................................
Init and Shutdown Functions

* `extern RC initIndexManager (void *mgmtData)`
	* initializes index manager
* `extern RC shutdownIndexManager ()`
	* shuts down index manager

...............................................................................................................................
Create, Destroy, Open, And Close An BTree Index Functions

* `extern RC createBtree (char *idxId, DataType keyType, int n)`
	* initialize the BtreeManager structure
	* after initializing it, we need to initialize the BufferPool that is going to hold our index
	* in the first page of the bufferPool identified by idxId, the BtreeManager structure is going to reside
	* once the BtreeManager structure is in the first page, we need to allocate an empty node into the tree
	* this node will be in a different page so we create the page 
	* set up buffer pool and handle 
* `extern RC openBtree (BTreeHandle **tree, char *idxId)`
	* openBtree needs to open the buffer pool by name idxId
	* then we must retrieve the BtreeManager struct from page 0 in the buffer pool
	* then we must update the tree handle passed in so that when we return success, the user can access the btree
* `extern RC closeBtree (BTreeHandle *tree)`
	* need to shutdown the buffer pool and clean up 
	* before closing the tree must be written back to the disk
* `extern RC deleteBtree (char *idxId)`
	* deletes tree only if file exists

...............................................................................................................................
Access Information About A BTree Functions

* `extern RC getNumNodes (BTreeHandle *tree, int *result`
	* getNumNodes will store the keyType in result
	* to retrieve getNumNodes all we need to do is access our management structure within each tree
	* each tree should have the management structure in it, in the first page of the buffer pool inside attr mgmtData
* `extern RC getNumEntries (BTreeHandle *tree, int *result)`
	* getNumEntries will store the keyType in result
	* to retrieve getNumEntries all we need to do is access our management structure within each tree
	* each tree should have the management structure in it, in the first page of the buffer pool inside attr mgmtData
* `extern RC getKeytype(BTreeHandle *tree, DataType *result)`
	* getKeytypewill store the keyType in result
	* to retrieve getKeytypeall we need to do is access our management structure within each tree
	* each tree should have the management structure in it, in the first page of the buffer pool inside attr mgmtData

...............................................................................................................................
Index Access + Helper Functions

* `extern RC findKey (BTreeHandle *tree, Value *key, RID *result)`
	* will find the RID for entry with key "key"
	* if found, the function needs to store the RID in the inputted value
* `extern RC insertKey (BTreeHandle *tree, Value *key, RID rid)`
	* will search the tree for the correct spot,
	* if there is space in the node, store the key and RID in the node
	* if there is no space in the node split the nodes and place the keys appropriately
	* if there is underflow, attempt to redistribute values from a sibling(nodes immediately left or right)
	* if a nonleaf node needs to be split and n is odd, 
	* the middle value must be inserted into the parent
* `extern RC insertLeaf(Node* node, Value* key, RID rid)`
	* two cases when inserting leaf node
	* no keys in node - just insert key
	* there are keys in node - find spot got key (need to order)
* `extern BM_PageHandle* createNode(BM_BufferPool* bp, BtreeManager* btmanager)`
	* allocate the page where new node will recide
	* then initialize the new nodes' data
* `extern RC splitNode(Node* child, BM_BufferPool* bp, BtreeManager* btmanager, int counter, Value *key, RID rid, int existPageNum)`
	* split node
	* two cases:- split doesn't cause non leaf split, and the other casuse also causes a split in above node
* `extern RC newNonLeafUpdate(Node* node, Value* key, int pageNum, Value* newVal)`
	* helper function to aid in split node's 2nd case
	* updates the non leaf node that has been split as a result of the second case from splitNode
* `extern RC updateNode(Node* node, Value* key, RID* rid, Value* newVal, RID* newRID)`
	* another helper function for splitNode()
	* after splitting the node, this function updates the key and rid of node
* `extern RC redistributeKeys(Node* node, int index, Value* key, int pageNum, Value* newVal)`
	* rearranges keys
	* updates node to the now corrected values
* `extern RC deleteKey (BTreeHandle *tree, Value *key)`
	* will search the tree for the correct value
	* if found, deletes key
	* handles redistribution and splitting 
* `extern RC openTreeScan(BTreeHandle* tree, BT_ScanHandle** handle)`
	* initializes the scan that is used to scan the entries in the tree
	* Basically go to the first (leftmost) leaf node and initialize the scan structure
* `extern RC nextEntry (BT_ScanHandle *handle, RID *result)`
	* first will retrieve first rid in the first leaf node then update the scan structure that was initialized in opentreescan
	* next time nextEntry is called again, we will get the second rid in the first leaf node
	* later, we must traverse to the next leaf node and store the next key in result
* `extern RC closeTreeScan(BT_ScanHandle* handle)`
	* since we are closing the scan handle, all we need to do is clean up aka free

...............................................................................................................................	
Debug and Test Functions

* `extern char *printTree (BTreeHandle *tree)`
	* prints the tree 