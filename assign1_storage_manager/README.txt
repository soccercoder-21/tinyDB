# CS 525

Assignment 1 Function descriptions

# manipulating page files 
extern void initStorageManager (void);

    initializes Storage manager

extern RC createPageFile (char *fileName);

    creates page file using calloc, verifies size.

extern RC openPageFile (char *fileName, SM_FileHandle *fHandle);

    opens page file, verifies file exists, updates fHandle attributes to file opened

extern RC closePageFile (SM_FileHandle *fHandle);

    closes page file using fclose

extern RC destroyPageFile (char *fileName);

    destroys page file using remove

# reading blocks from disc 
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);

    Reads block; verifies that file and page exists. Moves cursor to page in file then reads and changes the current page position

extern int getBlockPos (SM_FileHandle *fHandle);

    returns block position

extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

    reads block at pageNum = 0

extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

    reads block at pageNum = current pageNum - 1

extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

    reads block at pageNum = current pageNum

extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

    reads block at pageNum = current pageNum + 1

extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

    reads block at pageNum = total pageNum - 1

# writing blocks to a page file 
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);

    writes block at pageNum, verifies file and page exists, moves cursor to page then writes, if successful attributes change

extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

    write block at pageNum = current pageNum

extern RC appendEmptyBlock (SM_FileHandle *fHandle);

    creates empty block using calloc then moves cursor to beginning then appends and updates total page num

extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle);

    verifies total num pages is at least as big as number of pages.