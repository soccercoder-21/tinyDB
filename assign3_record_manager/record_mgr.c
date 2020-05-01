#include<stdio.h>
#include<stdlib.h>
#include"buffer_mgr.h"
#include"storage_mgr.h"
#include "record_mgr.h"
#include <math.h>

// This is custom data structure defined for making the use of Record Manager.
typedef struct TableManager
{	
	BM_BufferPool bufferPool; // buffer pool
    BM_PageHandle mgmtPage;   // stores management page 
	int numTuples;
	int firstFree; // first page with empty slot

    // for scanning purposes
    RID id;
    Expr *cond;
    int count;
} TableManager;

TableManager *table;

extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
    // Initialize schema
	Schema *schema = (Schema *) malloc(sizeof(Schema));
	schema->numAttr = numAttr;
	schema->attrNames = attrNames;
	schema->dataTypes = dataTypes;
	schema->typeLength = typeLength;
	schema->keySize = keySize;
	schema->keyAttrs = keys;

    return schema;
}

extern RC freeSchema(Schema* schema) {
    free(schema);
    return RC_OK;
}

extern RC initRecordManager (void *mgmtData)
{
    // this should only print that the storage manager was initialized
	initStorageManager();
	return RC_OK;
}

extern RC createTable (char *name, Schema *schema)
{
    printf("debug\n");
    // create management page
    SM_FileHandle fileHandle;
    //creates Page
	if (createPageFile(name) == RC_FILE_NOT_FOUND) {
		return RC_CREATE_PAGE_ERROR;
	}
	table = (TableManager*) malloc(sizeof(TableManager));
    if (initBufferPool(&table->bufferPool, name, 100, RS_FIFO, NULL) == RC_FILE_NOT_FOUND) {
		printf("initBuffer Failed.");
		// error checking 
		return RC_TABLE_CREATION_ERROR;
	}
    char data[PAGE_SIZE];
    char *initPage = data;

    // initialize the management page
    // set num tuples
    *(int *)initPage = 0;
    initPage += sizeof(int);
    // set first free page to 1 because 0 page is management page
    *(int *)initPage = 1;
    initPage += sizeof(int);

    // set table's schema data
    *(int *)initPage = schema->numAttr;
    initPage += sizeof(int);
    
    // fill management page with the attribute types
    for(int i = 0; i < schema->numAttr; i++) {
            // set attribute name
       		strncpy(initPage, schema->attrNames[i], sizeof(int));
	       	initPage += sizeof(int);         // move pointer size of 2 bytes to represent any ascii character
            // set data type
	       	*(int*)initPage = (int)schema->dataTypes[i];
	       	initPage += sizeof(int);
            // set length
	       	*(int*)initPage = (int) schema->typeLength[i];
	       	initPage += sizeof(int);
    }
   
    //opens Page
	if(openPageFile(name, &fileHandle) == RC_FILE_NOT_FOUND){
		return RC_PAGE_NOT_FOUND;
	}
    //writes to Page 
	if (writeBlock(0, &fileHandle, data) == RC_WRITE_FAILED) {
		return RC_WRITE_FAILED;
	}
    //closes Page
	int rc_output;
	if ((rc_output = closePageFile(&fileHandle)) != RC_OK) {
		return rc_output;
	}

	return RC_OK;
}

extern RC openTable (RM_TableData *rel, char *name)
{
    rel->mgmtData=table;
    rel->name = name;

    // pin page basically opens page num ( in this case 0 ) and puts it into the second argument
    // from the buffer pool indicated.
    pinPage(&table->bufferPool, &table->mgmtPage, 0);

    SM_PageHandle page;
    page = (char *) table->mgmtPage.data;

    // set numTuples from mgmtPage
    table->numTuples = *(int*)page;
    page += sizeof(int);

    // set firstFree from mgmtPage
    table->firstFree =  *(int*)page;
    page += sizeof(int);
    int numAttributes = *(int*)page;
    page += sizeof(int);


    // initialize schema
    Schema *schema = (Schema *) malloc(sizeof(Schema));
    schema->numAttr = numAttributes;
    // allocate memory for schema attributes
    schema->attrNames = (char**) malloc(sizeof(char*) * numAttributes);
	schema->dataTypes = (DataType*) malloc(sizeof(DataType) * numAttributes);
	schema->typeLength = (int*) malloc(sizeof(int) * numAttributes);

    // allocate memory for each attribute 
    for(int i = 0; i < numAttributes; i++)
		schema->attrNames[i]= (char*) malloc(sizeof(int));

    // assign the attribute
    for(int i = 0; i < schema->numAttr; i++)
    	{
		// set name
		strncpy(schema->attrNames[i], page, sizeof(int));
		page += sizeof(int);
	   
		// set type
		schema->dataTypes[i]= *(int*) page;
		page +=  sizeof(int);

		// set length
		schema->typeLength[i]= *(int*)page;
		page += sizeof(int);
	}

    rel->schema = schema;

    unpinPage(&table->bufferPool, &table->mgmtPage);
    // write page to disk
    forcePage(&table->bufferPool, &table->mgmtPage);

	return RC_OK;
}

extern RC createRecord(Record** record, Schema* schema) {
    Record* newrec = (Record *) malloc(sizeof(Record)); // allocate memory for the record
    newrec->data = (char *) malloc(getRecordSize(schema)); // allocate memory to store the data but initialize to 0
    char *tupData = newrec->data;
    // set up the tombstone marker ; indicate empty
    *tupData = '-';
    tupData++;
    *tupData = '\0';

    //set the page and the slot to -1 as they aren't valid
    newrec->id.page = -1;
    newrec->id.slot = -1;
    // set the record
    *record = newrec;
    return RC_OK;
}

RC calcOffset (Schema *schema, int attrNum, int *offset)
{
    // offset starts at 1 because of the tombstone
	*offset = 1;
    // go through all the attributes in schema before attrNum
	for(int i = 0; i < attrNum; i++)
	{
        // based on the attribute number increment offset by size of attribute type
		switch (schema->dataTypes[i])
		{
			case DT_STRING:
				*offset +=  schema->typeLength[i];
				break;
			case DT_INT:
				*offset += sizeof(int);
				break;
			case DT_FLOAT:
				*offset += sizeof(float);
				break;
			case DT_BOOL:
				*offset += sizeof(bool);
				break;
		}
	}
	return RC_OK;
}

extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
	// we need to calculate the offset based on the attribute Number to be able to go to that attribute, then we write the value to that

	int offset = 0;
	calcOffset(schema, attrNum, &offset);

	Value *attribute = (Value*) malloc(sizeof(Value));

    // calculate location of the attribute
	char *attr = record->data;
	attr = attr + offset;

	// verify that the first attribute is 1
	if(attrNum == 1){
		schema->dataTypes[attrNum] = 1;
	}
	else{
		schema->dataTypes[attrNum] = schema->dataTypes[attrNum];
	}


	switch(schema->dataTypes[attrNum])
	{
		case DT_STRING:
		{
     			// get string
			int length = schema->typeLength[attrNum];
			attribute->v.stringV = (char *) malloc(length + 1);
			strncpy(attribute->v.stringV, attr, length);
			attribute->v.stringV[length] = '\0';
			attribute->dt = DT_STRING;
      			break;
		}

		case DT_INT:
		{
			// get int
			int value = 0;
			memcpy(&value, attr, sizeof(int));
			attribute->v.intV = value;
			attribute->dt = DT_INT;
      			break;
		}
    
		case DT_FLOAT:
		{
			// get float
	  		float value;
	  		memcpy(&value, attr, sizeof(float));
	  		attribute->v.floatV = value;
			attribute->dt = DT_FLOAT;
			break;
		}

		case DT_BOOL:
		{
			// get bool
			bool value;
			memcpy(&value,attr, sizeof(bool));
			attribute->v.boolV = value;
			attribute->dt = DT_BOOL;
      			break;
		}

		default:
			printf("Datatype not defined. \n");
			return RC_UNKNOWN_DATATYPE;
	}

	*value = attribute;
	return RC_OK;
}

// This function sets the attribute value in the record in the specified schema
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
	// we need to calculate the offset based on the attribute Number to be able to go to that attribute, then we write the value to that
	int offset = 0;

	calcOffset(schema, attrNum, &offset);
    // calculate location of the attribute
	char *attr = record->data;	
	attr = attr + offset;
		
	// based on the type of attribute update
	switch(schema->dataTypes[attrNum])
	{
		case DT_STRING:
		{
            // if string use strncpy
			int length = schema->typeLength[attrNum];
			strncpy(attr, value->v.stringV, length);
			attr = attr + schema->typeLength[attrNum];
		  	break;
		}

		case DT_INT:
		{
            // if int just place
			*(int *) attr = value->v.intV;	  
			attr = attr + sizeof(int);
		  	break;
		}
		
		case DT_FLOAT:
		{
			// if float just place
			*(float *) attr = value->v.floatV;
			attr = attr + sizeof(float);
			break;
		}
		
		case DT_BOOL:
		{
            // if bool just place
			*(bool *) attr = value->v.boolV;
			attr = attr + sizeof(bool);
			break;
		}

		default:
			printf("Datatype not defined. \n");
			return RC_UNKNOWN_DATATYPE;
	}			
	return RC_OK;
}

extern RC insertRecord (RM_TableData *rel, Record *record)  
{
    // we are inserting record into the table
    // retrieve tablemanager from table data and assign locally
    table = rel->mgmtData;

    // set record id
    RID *recordID = &record->id;
    // get the record size
    int recordSize = getRecordSize(rel->schema);

    // set record id page to first free page where it will be inserted
    recordID->page = table->firstFree;

    // define temporary page you are using to insert record
    BM_PageHandle *recordPage = malloc(sizeof(BM_PageHandle));

    // pin page; recordPage now holds the data of first page with free slot
    pinPage(&table->bufferPool, recordPage, recordID->page);

    // slot is -1 if search fails to find a free slot in recordPage
    recordID->slot = -1;
    
    // search page for free slot
    for(int i = 0; i < (PAGE_SIZE/recordSize); i++) {
        if(recordPage->data[i*recordSize] != '#'){
            // if there is a slot that does not start with a marker for used indicate it as a free slot
            recordID->slot = i;
            break;
        }
    }
    while(recordID->slot == -1){
        // if search fails loop to find first free page
        unpinPage(&table->bufferPool,recordPage);
        recordID->page++;
        pinPage(&table->bufferPool, recordPage, recordID->page);
        // search page for free slot
        for(int i = 0; i < (PAGE_SIZE/recordSize); i++) {
            if(recordPage->data[i*recordSize] != '#'){
                // if there is a slot that does not start with a marker for used indicate it as a free slot
                recordID->slot = i;
            }
        }
    }
    // calculate slot position
    // this also is the address within recordPage->data that we need to update with record 
    char *slotPos = recordPage->data;
    slotPos += (recordID->slot*recordSize);

    // add indication that record is in use
    *slotPos = '#';
    // move address one byte for marker
    *slotPos++;
    // add record data  ; fix sizes due to tombstone to only copy data
    memcpy(slotPos, record->data + 1, recordSize - 1);

    // write back to disk
    markDirty(&table->bufferPool,recordPage);
    unpinPage(&table->bufferPool,recordPage);
    //update mgmt page
    table->numTuples++; 
    pinPage(&table->bufferPool, &table->mgmtPage, 0);
    
   

	return RC_OK;
}

extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{
    // retrieve tablemanager from table data and assign locally
    table = rel->mgmtData;
    // define temporary page you are using to insert record
    BM_PageHandle *recordPage = malloc(sizeof(BM_PageHandle));

    pinPage(&table->bufferPool, recordPage, id.page);

    int recSize = getRecordSize(rel->schema); // get the size of the record
    char *recordSlot = recordPage->data;
    recordSlot = recordSlot + (id.slot * recSize); // move to correct record

    if(*recordSlot != '#'){
        // if the record is not marked as in use return error
        return RC_TUPLE_NOT_FOUND;
    }
    record->id = id;
    // remove tombstone and move record->data one ahead of marker
    char *data = record->data;
    // copy data
    memcpy(++data,recordSlot + 1, recSize - 1);

    // pinnedData += (id.slot * recSize); - we could use memcpy rather than the for loop

    unpinPage(&table->bufferPool, recordPage);

    return RC_OK;
}

extern int getRecordSize (Schema *schema)
{
	//initialize size
    int size = 0;
	
	// go through all the attributes in schema
	for(int i = 0; i < schema->numAttr; i++)
	{
		if(schema->dataTypes[i] == DT_STRING){
            size += schema->typeLength[i];
        }
        else if(schema->dataTypes[i] == DT_INT){
            size += sizeof(int);
        }
        else if(schema->dataTypes[i] == DT_FLOAT){
            size += sizeof(float);
        }
        else if(schema->dataTypes[i] == DT_BOOL){
            size += sizeof(bool);
        }
	}
    // increment size by one because of tombstone
    size++; 
	return size;
}

extern RC updateRecord (RM_TableData *rel, Record *record)
{
    table = rel->mgmtData; 
    
    RID id = record->id;

    // make temp page to update tuple
    BM_PageHandle *recordPage = malloc(sizeof(BM_PageHandle));
    pinPage(&table->bufferPool, recordPage, id.page);
    int recSize = getRecordSize(rel->schema);
    // this moves tuple to address of record to be deleted
    char *tuple;
    tuple = recordPage->data;
    tuple = tuple + (id.slot * recSize);
    // mark tuple as in use
    *tuple = '#';
    // prepare tuple to copy data (move one up for tombstone removal)
    tuple++;
    memcpy(tuple, record->data + 1, recSize - 1);
    // write to disk
    markDirty(&table->bufferPool,recordPage);
    unpinPage(&table->bufferPool,recordPage);
    return RC_OK;	
}

// might need to come back to this
extern RC deleteRecord(RM_TableData* rel, RID id)
{
    table = rel->mgmtData; 
    table->firstFree = id.page;
    
    // make temp page to update tuple
    BM_PageHandle *recordPage = malloc(sizeof(BM_PageHandle));
    pinPage(&table->bufferPool, recordPage, id.page);
    int recSize = getRecordSize(rel->schema);
    // this moves tuple to address of record to be deleted
    char *tuple = recordPage->data;
    tuple = tuple + (id.slot * recSize);
    // mark tuple as deleted
    *tuple = '-';
    // write to disk
    markDirty(&table->bufferPool,recordPage);
    unpinPage(&table->bufferPool,recordPage);

    // we do not decrease numTuples because the tuple still exists with
    // but marked as deleted
    return RC_OK;
}

extern int getNumTuples(RM_TableData* rel) {
    return ((TableManager *) rel->mgmtData)->numTuples;
}

extern RC closeTable (RM_TableData *rel)
{
    table = rel->mgmtData;
    shutdownBufferPool(&table->bufferPool);
    return RC_OK;
}

extern RC deleteTable (char *name)
{
	destroyPageFile(name);
	return RC_OK;
}

extern RC shutdownRecordManager ()
{
    table = NULL;
    free(table);

	return RC_OK;
}

extern RC freeRecord (Record *record)
{
    free(record);
	return RC_OK;
}

extern RC startScan(RM_TableData* rel, RM_ScanHandle* scan, Expr* cond)
{
    //printf("debug");
    //error checking for condition
    if (cond == NULL) {
        RC_SCAN_CONDITION_ERROR;
    }
    openTable(rel, "scan_t");
    TableManager *tblMgr;
    TableManager *scanMgr = (TableManager *) malloc(sizeof(TableManager));
    //initializes the scan data structure passed
    scan->mgmtData= scanMgr;
    scanMgr->cond = cond;
    // records start on page 1
    scanMgr->id.page = 1; 
    // first slot is 0
    scanMgr->id.slot = 0;
    scanMgr->count = 0;
    // weird error was showin

    //printf("debug");
    tblMgr= rel->mgmtData;
    tblMgr->numTuples = 15;
    scan->rel = rel;

    return RC_OK;
}

// might need to come back to this
extern RC next(RM_ScanHandle* scan, Record* record) {

    // retrieved data initialized in start
    TableManager *scanMgr = scan->mgmtData;
    TableManager *tblMgr = scan->rel->mgmtData;
    // error checking to verify for condition
    if(scanMgr->cond == NULL)
        return RC_SCAN_CONDITION_ERROR;
    Schema *schema = scan->rel->schema; 
    int tupPerPage = PAGE_SIZE / getRecordSize(schema);
    // set the total from table 
    int total = table->numTuples;

    //set up the scan manager
    int curScanCount = scanMgr->count;
    int curPage = scanMgr->id.page;
    int curSlot = scanMgr->id.slot;

    Value *value = (Value *) malloc(sizeof(Value));
    scanMgr->count++;

    while(curScanCount<total){
        scanMgr->id.page=curPage;
        scanMgr->id.slot=curSlot;

        getRecord(scan->rel,scanMgr->id,record);
        curScanCount++;
        if (scanMgr->cond != NULL){
            evalExpr(record, (scan->rel)->schema, scanMgr->cond, &value);
            // check the value of boolV
            if(value->v.boolV == 1){
                record->id.page=curPage;
                record->id.slot=curSlot;
                if(curSlot ==(tupPerPage-1)){
                    curPage++;
                    curSlot = 0;
                }else{
                    curSlot++;
                }
                // set the scan manager to the currnt page and slot
                scanMgr->id.page= curPage;
                scanMgr->id.slot= curSlot;
                return RC_OK;
            }
        }else{
            value->v.boolV == TRUE; // when no condition return all records
        }
        if(curSlot ==(tupPerPage-1)){
            curPage++;
            curSlot = 0;
        }else{
            //have to increase the curent slot b/c not equal to tuples per page
            curSlot++;
        }
    }
    value->v.boolV == TRUE;
    // re-initialize the scanMgr
    scanMgr->id.page=1; // records starts from page 1
    scanMgr->id.slot=0; // slot starts from 0
    scanMgr->count = 0;
    return RC_RM_NO_MORE_TUPLES;

}

extern RC closeScan(RM_ScanHandle* scan)
{
    TableManager *scanMgr = scan->mgmtData;
    // reinitialize values
    scanMgr->id.page = 1;
    scanMgr->id.slot = 0;
    // we also need to set the count to 0 after closing scan
    scanMgr->count = 0;
    return RC_OK;
}

