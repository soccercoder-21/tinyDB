Database Record Manager - Assignment 3
...............................................................................................................................
Manuel Lopez    - A20411680
Mohammed Chisti - A20393790

...............................................................................................................................
Building and Testing Instructions

Head to the assignment 3 directory
	(If you can see this you are at the right place)
Input "make" to compile the assignment (ignore the warnings the gcc is giving for memcpy)
	This results in two executable files: test3 and testexpr
	test3: this tests the project using test_assign3_1 file
	testexpr: this tests the project using test_expr.c file
Chose which executable
	For test3 input "./test3"
	For testexpr input "./testexpr"
After testing, cleanup by inputting "make clean"

*** fully functional ***

...............................................................................................................................
Table and Record Manager Functions

initRecordManager;
	initializes record manager
shutdownRecordManager;
	shuts down record manager and deallocates table manger structure
createTable;
	creates table in memory 
	initializes buffer pool via init and we utilize the FIFO replacement policy
	creates page file, opens that page file and writes to it and later closes the page file
openTable;
	table opened from memory
	pins page
	initializes schema, allocates memory and sets the copies the pages information there
closeTable;
	grabs the mgmtData and assigns to table struc
	later shutsdown the buffer pool for table struc
getNumTuples;
	gets number of tuples in table

...............................................................................................................................
Record Functions

insertRecord;
	insert a new record at page and slot
	pins page -> inserts record
	page is marked dirty and written to disk
deleteRecord;
	deletes record
	pins page -> record updated
	uses tombstone
	marks page as dirty and written to disk
updateRecord;
	updates record
	pins page -> update
	marks page as dirty and written to disk
getRecord;
	retrieves record from page and slot

...............................................................................................................................
Scan Functions

startScan;
	scans tuples based on data from RM_ScanHandle
next;
	gets next tuple that satisfies cond.
closeScan;
	closes the scan

...............................................................................................................................
Schema Functions

getRecordSize;
	using schema gets record size 
createSchema;
	creates new Schema
freeSchema;
	deletes schema 
	frees the allocated resources

...............................................................................................................................
Attribute Functions

createRecord;
	creates empty record
	sets tombstone marker to indicate empty
freeRecord;
	pins page with record
	deletes record -> updates tombstone 
	marks dirty and writes to disk
getAttr;
	gets attribute value for a record
setAttr;
	sets the attribute value based on int, string etc.