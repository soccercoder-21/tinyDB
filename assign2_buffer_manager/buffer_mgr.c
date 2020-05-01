#include<stdio.h>
#include<stdlib.h>
#include"buffer_mgr.h"
#include"storage_mgr.h"
#include <math.h>

SM_FileHandle* FILE_HANDLE; // holds file's information
int WRITE_IO;
int READ_IO;


void initPageFrameQueue(Queue *queue){
    //create new pageFrame
    PageFrame *newPage = (PageFrame*)malloc(sizeof(PageFrame));
    WRITE_IO=0;
    READ_IO=0;
    newPage->data= NULL;
    newPage->pageNum= NO_PAGE;
    newPage->fixCount= 0;
    newPage->dirty=0;
    newPage->numAcc =0;
    // make circular queue
    if(queue->total==0){
        newPage->next=newPage;
        queue->head=newPage;
    }
    else{
        queue->tail->next=newPage;
        newPage->next=queue->head;
    }
    queue->tail=newPage;
    queue->total++;
    
}

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    // create new buffer pool with numPages page frames

    int i;
    FILE_HANDLE=(SM_FileHandle*)malloc(sizeof(SM_FileHandle));
    Queue *queue= (Queue*)malloc(sizeof(Queue)*numPages);
    queue->head=NULL;
    queue->tail=NULL;
    queue->curPos=NULL;
    queue->total=0;
    if(openPageFile((char*)pageFileName,FILE_HANDLE)== RC_OK){
        //initialise Page frame
        for(i=0;i< numPages; i++)
            initPageFrameQueue(queue);
        //initialis buffer pool
        queue->curPos=queue->head;
        bm->pageFile= (char*)pageFileName;
        bm->numPages=numPages;
        bm->strategy= strategy;
        bm->mgmtData= queue;
        
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}

RC shutdownBufferPool( BM_BufferPool *const bm){
    // destroys buffer pool
	// frees all resources associated freeing memory allocated for page frames
	// if any pages are dirty they must be forced before destroying pool
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;
    PageFrame *current;
    int i;
    // printf("inside shutdown\n");
	forceFlushPool(bm); // write all dirty pages to disk
	// printf("after force flush pool\n");
	//closePageFile(FILE_HANDLE);
    for(int i = 0; i < bm->numPages; i++) {
        if(temp->fixCount!=0){
            return -1;
        }
        temp=temp->next;
    }
    bm->mgmtData=NULL;
    for(i=0;i<bm->numPages-1;i++){
        current=queue->head->next;
        free(queue->head);
        queue->head=current;
    }
    free(bm->mgmtData);
    closePageFile(FILE_HANDLE);
    free(FILE_HANDLE);
    return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm){
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;

    for(int i = 0; i < bm->numPages; i++) {
        if(temp->fixCount==0 && temp->dirty==1){
            // printf("inside if\n");
			// printf("%s\n", temp->data);
			// printf("%i\n",temp->pageNum);
			// printf("%p\n",FILE_HANDLE);
			// openPageFile(bm->pageFile, FILE_HANDLE);
            writeBlock(temp->pageNum,FILE_HANDLE,temp->data);
			// closePageFile(FILE_HANDLE);
            WRITE_IO++;
            temp->dirty=0; // when force flushing need to clear all dirty flags
            
        }
        temp=temp->next;
    }
    
    return RC_OK;
}



RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page){
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;

    for(int i = 0; i < bm->numPages; i++) {
        if(temp->pageNum==page->pageNum){
            temp->dirty=1;
            return RC_OK;
            
        }
        temp=temp->next;
    }
    return RC_FILE_NOT_FOUND;
}


RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page){
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;

    for(int i = 0; i < bm->numPages; i++) {
        if(temp->pageNum==page->pageNum){
            temp->fixCount--;
            break;
            
        }
        temp=temp->next;
    }
    return RC_OK;
}

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page){
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;
    SM_FileHandle fh;
	openPageFile(bm->pageFile, &fh);

    for(int i = 0; i < bm->numPages; i++) {
        if(temp->pageNum==page->pageNum){
            writeBlock(page->pageNum,FILE_HANDLE,page->data);
            WRITE_IO++;
            temp->dirty=0;
            temp->fixCount=0;
        }
        temp=temp->next;
    }
    
    return RC_OK;
}



PageNumber *getFrameContents (BM_BufferPool *const bm)
{
    int *frameContents = (int*)malloc(sizeof(int) * bm->numPages);
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;

    for(int i = 0; i < bm->numPages; i++) {
        // printf("i: %i\n", i);
        // printf("pagenum: %i\n", temp->pageNum);
        // printf("numpages: %i\n", bm->numPages);
        frameContents[i]=temp->pageNum;

        temp=temp->next;
    }

    return frameContents;
     
    
}

int *getNumAccessed (BM_BufferPool *const bm)
{
    int *numAccContents = (int*)malloc(sizeof(int) * bm->numPages);
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;

    for(int i = 0; i < bm->numPages; i++) {
        // printf("i: %i\n", i);
        // printf("pagenum: %i\n", temp->pageNum);
        // printf("numpages: %i\n", bm->numPages);
        numAccContents[i]=temp->numAcc;

        temp=temp->next;
    }

    return numAccContents;
     
    
}


bool *getDirtyFlags (BM_BufferPool *const bm)
{
    bool *dirtyFlags = (bool*)malloc(sizeof(bool) * bm->numPages);
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;

    for(int i = 0; i < bm->numPages; i++) {
        dirtyFlags[i]=temp->dirty;

        temp=temp->next;
    }
    return dirtyFlags;
}

int *getFixCounts (BM_BufferPool *const bm)
{
    int *fixCounts = (int*)malloc(sizeof(int) * bm->numPages);
    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *temp= (PageFrame*)queue->head;
    for(int i = 0; i < bm->numPages; i++) {
        fixCounts[i]=temp->fixCount;

        temp=temp->next;
    }
    return fixCounts;
    
    
}

int getNumReadIO (BM_BufferPool *const bm)
{
    return READ_IO;
}


int getNumWriteIO (BM_BufferPool *const bm)
{   
    return WRITE_IO;
}

void FIFO(BM_BufferPool *const bm, BM_PageHandle *const page, const  PageNumber pageNum){

    // pin page using FIFO
	// if pageNum in buffer pool, just increase fix count of pageNum and increment READ_IO
    // if pageNum is not in buffer pool, add to head of queue increase fix count and increment READ_IO
	// if buffer pool is filled, evict a page from queue, unpin that page, add new page to head of queue and increment READ_IO
    // update page to pageFrame pointed to by pageNum


    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *pageFrame= (PageFrame*)queue->head;
    
    if(pageFrame->pageNum==NO_PAGE){ // if empty add to head pageframe of queue
        SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
        pageFrame->pageNum=pageNum;
        pageFrame->fixCount=1;
        page->data=(SM_PageHandle)calloc(sizeof(char),4096);
        // printf("pageNUM: %i\n", pageNum);
        ensureCapacity(pageNum,&fh);
        readBlock(pageNum,FILE_HANDLE,page->data);
        READ_IO++;
        pageFrame->data=page->data;
        
        page->pageNum= pageNum;
        
        return;
    }
    else{   // if queue is not empty
        for(int i = 0; i < bm->numPages; i++) { // iterate through queue to see if page is already there
            if(pageFrame->pageNum==pageNum){
                pageFrame->fixCount++;
                page->pageNum= pageNum;
                page->data=pageFrame->data;
                return;
            }
            pageFrame=pageFrame->next;
        }
        // if page is not in the queue
        for(int i = 0; i < bm->numPages; i++) { // find next empty page and pin to it
            if(pageFrame->pageNum==NO_PAGE){
                SM_FileHandle fh;
		        openPageFile(bm->pageFile, &fh);
                pageFrame->pageNum=pageNum;
                pageFrame->fixCount++;
                page->pageNum= pageNum;
                page->data=(SM_PageHandle)calloc(sizeof(char),4096);
                // printf("1pageNUM: %i\n", pageNum);

                readBlock(pageNum,FILE_HANDLE,page->data);
                READ_IO++;
                pageFrame->data=page->data;
                queue->curPos=pageFrame;
                return;
                
            }
            pageFrame=pageFrame->next;
        }
    }
    // if the buffer pool is full, evict a page and write to that page
    pageFrame=queue->curPos->next;
    for(int i = 0; i < bm->numPages; i++) { // iterate through queue finding a page with fix count of 0
        if(pageFrame->fixCount!=0){
            queue->curPos=pageFrame;
        }
        else if(pageFrame->fixCount==0)
            break;
        
        pageFrame=pageFrame->next;
        }
    // if fix count is 0, overwrite it 
    page->data=pageFrame->data; 
    page->pageNum=pageFrame->pageNum;
    if(pageFrame->dirty==1) // if written to, write to disk
        forcePage(bm,page);
    pageFrame->fixCount=0;
    SM_FileHandle fh;
	openPageFile(bm->pageFile, &fh);
    pageFrame->pageNum=pageNum;
    pageFrame->fixCount++;
    page->pageNum= pageNum;
    // printf("11pageNUM: %i\n", pageNum);

    readBlock(pageNum,FILE_HANDLE,page->data);  // put desired data in newly pinned page
    READ_IO++;
    pageFrame->data=page->data;
    queue->curPos=pageFrame;
    return;
    
    
}

int lru = 100;

void LRU(BM_BufferPool *const bm, BM_PageHandle *const page, const  PageNumber pageNum){
    // pin page using LRU
	// if page in buffer pool, increase fix count of page, move to the head and increment READ_IO
	// ^^^^^^^^^^^^^^^^^^^this is the difference between FIFO and LRU, in LRU you must also move the accessed page to the head so that the queue priorities change
	// if page is not in buffer pool, add to queue at the head, increase the fix count and increment READ_IO
	// if buffer pool is filled, evict tail, unpin the page at tail, add new page to head of queue and increment READ_IO
		

    Queue *queue=(Queue*)bm->mgmtData;
    PageFrame *pageFrame= (PageFrame*)queue->head;
    PageFrame *temp = (PageFrame*)queue->head;
    PageFrame *new;
    PageNumber *frameContent;
    
    
    if(pageFrame->pageNum==NO_PAGE){ // if empty add to first empty pageframe of queue
        SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
        pageFrame->pageNum=pageNum;
        pageFrame->fixCount=1;
        page->data=(SM_PageHandle)calloc(sizeof(char),4096);
        // printf("pageNUM: %i\n", pageNum);
        ensureCapacity(pageNum,&fh);
        readBlock(pageNum,FILE_HANDLE,page->data);
        READ_IO++;
        pageFrame->data=page->data;  
        pageFrame->numAcc=0;
        page->pageNum= pageNum;
        // frameContent = getFrameContents(bm); 
        // for (int i = 0; i < bm->numPages; i++)
        //             printf("init queue: %i\n", frameContent[i]);
        return;
    }
    else{   // if queue is not empty
        for(int i = 0; i < bm->numPages; i++) { // iterate through queue to see if page is already there
            if(pageFrame->pageNum==pageNum){       
                // if page is there, must indicate it is most recently accessed
                // with singly linked list save the current node
                // make the current node have the value of next and eliminate the next
                // once the node is removed make temp saved node into the head
                // nodes closest to the head are most recently used
                // evict tail with fix count of 0 if the queue is full
                pageFrame->fixCount++;
                page->pageNum= pageNum;
                page->data=pageFrame->data;
                pageFrame->numAcc = lru;
                lru--;
                             
                return;
            }
            pageFrame=pageFrame->next;
        }
        // if page is not in the queue
        for(int i = 0; i < bm->numPages; i++) { // find next empty page and pin to it
            if(pageFrame->pageNum==NO_PAGE){
                SM_FileHandle fh;
		        openPageFile(bm->pageFile, &fh);
                pageFrame->pageNum=pageNum;
                pageFrame->fixCount++;
                page->pageNum= pageNum;
                page->data=(SM_PageHandle)calloc(sizeof(char),4096);
                pageFrame->numAcc=0;
                // printf("1pageNUM: %i\n", pageNum);

                readBlock(pageNum,FILE_HANDLE,page->data);
                READ_IO++;
                pageFrame->data=page->data;
          
                return;
                
            }
            pageFrame=pageFrame->next;
        }
    }


    // if the buffer pool is full, evict a page and write to that page
    pageFrame=queue->head;
    for(int i = 0; i < bm->numPages; i++) { // iterate through queue finding a page with fix count of 0
        if(pageFrame->fixCount==0 && pageFrame->numAcc == 100)
            break;
        
        pageFrame=pageFrame->next;
        }
    //printf("evicted page: %i\n", pageFrame->pageNum);
    // write data to page to evict 
    page->data=pageFrame->data; 
    page->pageNum=pageFrame->pageNum;
    if(pageFrame->dirty==1) // if written to, write to disk
        forcePage(bm,page);
    temp = queue->head;
    for(int i = 0; i < bm->numPages; i++){
        if (temp->numAcc != 100){
            temp->numAcc++;
        }
        temp = temp->next;
    }
    // assign new pageframe
    pageFrame->fixCount=0;
    SM_FileHandle fh;
	openPageFile(bm->pageFile, &fh);
    pageFrame->pageNum=pageNum;
    pageFrame->fixCount++;
    pageFrame->numAcc = 0;
    page->pageNum= pageNum;
    // printf("11pageNUM: %i\n", pageNum);
    readBlock(pageNum,FILE_HANDLE,page->data);  // put desired data in newly pinned page
    READ_IO++;
    pageFrame->data=page->data;
    frameContent = getFrameContents(bm); 
    return;
    
    
}

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const  PageNumber pageNum){
    if(bm->strategy==RS_FIFO)
        FIFO( bm, page, pageNum);
    if(bm->strategy==RS_LRU)
        LRU(bm, page,pageNum);
    
    return RC_OK;
}