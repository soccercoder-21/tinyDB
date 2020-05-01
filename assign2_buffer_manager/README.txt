# CS 525

Assignment 2 Function descriptions

void initPageFrameQueue (BM_BufferPool *const bm)
    creates queue data structure and initializes it

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
    initializes bufferpool

RC shutdownBufferPool(BM_BufferPool *const bm) 
    shuts down buffer pool; forces writes on all dirty pages before completing; frees allocated memory

RC forceFlushPool(BM_BufferPool *const bm)
    writes all dirty pages to disk

RC pinPage(BM_BufferPool * const bm, BM_PageHandle * const page, const PageNumber pageNum)
    marks page as pinned; uses bm->strategy to decide which page to evict given bufferpool is filled

RC unpinPage(BM_BufferPool * const bm, BM_PageHandle * const page)
    marks page as unpinned; forces write if dirty

RC markDirty(BM_BufferPool * const bm, BM_PageHandle * const page)
	Function marks the requested page as 'dirty'

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
	Function writes the requested page back to the page file which is on disk

PageNumber *getFrameContents (BM_BufferPool *const bm)
	Function returns the pointer to the frameContents of PageNumbers which is of size PageNumber.

int *getNumAccessed (BM_BufferPool *const bm)
	Function returns the pointer to the frameContents of numAccessed which is of size int.

bool *getDirtyFlags (BM_BufferPool *const bm)
	Function returns an array of booleams which is of size PageNumber.
	Empty page frames are not 'dirty'

int *getFixCounts (BM_BufferPool *const bm)
	Function returns an array of integers which is of size PageNumber.

int getNumReadIO (BM_BufferPool *const bm)
	Function returns the number of pages which have been read from disk since the initalization of Buffer Pool

int getNumWriteIO (BM_BufferPool *const bm)
	Function returns the number of pages which have been written to page file since the initalization of Buffer Pool

void FIFO(BM_BufferPool *const bm, BM_PageHandle *const page, const  PageNumber pageNum)
    Runs pin page using FIFO strategy

void LRUBufferPool *const bm, BM_PageHandle *const page, const  PageNumber pageNum)
    Runs pin page using LRU strategy