// Manuel Lopez A20411680
// Mohammed Chisti A20393790


#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//Done here to avoid having to keep writing File *f1 in each function
FILE *f1;

void initStorageManager(void)
{
	printf("Database Storage Manager Initialized!\n");
}

RC createPageFile(char* fileName)
{
	// FILE* f1;
	f1 = fopen(fileName, "w+"); // "w+" creates empty file

	// checks if f1 is null, if so then returns error
	if (f1 == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	else{
		// calloc here allocates page_size number of elements where each element is a size of char. Finally they are initialized to 0
		char* fp = (char*)calloc(PAGE_SIZE, sizeof(char));
		size_t verifySize = fwrite(fp, sizeof(char), PAGE_SIZE, f1);
		if (verifySize != PAGE_SIZE)
		{
			return RC_FILE_SIZE_ERROR;
		}
		fclose(f1);
		return RC_OK;
	}

}


RC openPageFile(char* fileName, SM_FileHandle* fHandle)
{
	// FILE* f1;
	f1 = fopen(fileName, "r+"); //read only empty file

	if (f1 == NULL) {
		printf("Requested file doesn't exists!\n");
		return RC_FILE_NOT_FOUND;
	}

	else {
		//fHandle->fileName = fileName;
		//moved down to organize code - Manuel
		fseek(f1,0,SEEK_END);
		int sizeOfFile = ftell(f1);

		// fseek(f1, 0, SEEK_SET);		
		// int currentPosOfPage = ftell(f1)/PAGE_SIZE;

// initialize file handle structure attributes
		fHandle->fileName = fileName;
		fHandle->totalNumPages = sizeOfFile / PAGE_SIZE;
		fHandle->curPagePos = 0;
		fHandle->mgmtInfo = f1;
		return RC_OK;
	}

}


RC closePageFile(SM_FileHandle* fHandle)
{
	// FILE* f1;
	f1 = (fHandle->mgmtInfo);

	if (f1 == NULL){
		return RC_FILE_HANDLE_NOT_INIT;
	}
	if (fHandle->fileName == NULL) {
		return RC_FILE_NOT_FOUND;
	}
	if (fclose(f1) == 0) {
		return RC_OK;
	}
	else {
		return RC_FILE_NOT_FOUND;
	}
}

RC destroyPageFile(char* fileName)
{
	// int delPF = remove(fileName);

	// if (delPF == 0) {
	// 	return RC_OK;
	// }
	// else {
	// 	printf("file not destroyed")
	// 	return RC_FILE_NOT_DELETED;
	// }
	 if (fileName == NULL) {
		 return RC_FILE_NOT_FOUND;
	 }
	 remove(fileName);
	 return RC_OK;

}

RC readBlock(int pageNum, SM_FileHandle* fHandle, SM_PageHandle memPage)
{
	// FILE* f1;
	f1 = fopen(fHandle->fileName, "r"); //read only empty file

	if (f1 == NULL) {
		printf("Requested file doesn't exists!\n");
		return RC_FILE_NOT_FOUND;
	}
	if (fHandle == NULL) {
		printf("File Handler not initialized!\n");
		return RC_FILE_HANDLE_NOT_INIT;
	}
	if (pageNum< 0 || pageNum > fHandle->totalNumPages) {
		printf("Page doesn't exist!\n");
		return RC_READ_NON_EXISTING_PAGE;
	}
	if(fseek(f1,(pageNum*PAGE_SIZE), SEEK_SET) == 0 ){ // fseek sets cursor position of file stream, returns 0 if successful
		fread(memPage, sizeof(char), PAGE_SIZE, f1);	
	}
	else {
		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos = ftell(f1); //sets new current page position
	fclose(f1);
	return RC_OK;
}


int getBlockPos(SM_FileHandle* fHandle)
{
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}
	return fHandle->curPagePos;
}

RC readFirstBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
{
	return readBlock(0, fHandle, memPage);
}

RC readPreviousBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
{
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE; // current position divided by page size = page number
	return readBlock(currentPageNumber - 1, fHandle, memPage);
}

RC readCurrentBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
{
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE; // current position divided by page size = page number
	return readBlock(currentPageNumber, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
{
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE; // current position divided by page size = page number
	return readBlock(currentPageNumber + 1, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
{
	return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

RC writeBlock(int pageNum, SM_FileHandle* fHandle, SM_PageHandle memPage)
{
// first check that the block exists
	// FILE* f1;
	f1 = fopen(fHandle->fileName, "r+"); //read and write mode file

	if (f1 == NULL) {
		printf("Requested file doesn't exists!\n");
		return RC_WRITE_FAILED;
	}
	if (fHandle == NULL) {
		printf("File Handler not initialized!\n");
		return RC_FILE_HANDLE_NOT_INIT;
	}
	if (pageNum< 0 || pageNum > fHandle->totalNumPages) {
		printf("Page doesn't exist!\n");
		return RC_WRITE_FAILED;
	}
	//find file position and write
	if(fseek(f1,(pageNum*PAGE_SIZE), SEEK_SET) == 0 ){ // fseek sets cursor position of file stream, returns 0 if successful
		fwrite(memPage, sizeof(char), PAGE_SIZE, f1);// write the file
	}
	else {
		return RC_WRITE_FAILED;
	}
	//set attributes
	fHandle->curPagePos = pageNum; //sets new current page position
	fseek(f1,0,SEEK_END);
	fHandle->totalNumPages = ftell(f1) / PAGE_SIZE;
	fclose(f1);
	return RC_OK;

}

RC writeCurrentBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
{
	return writeBlock(fHandle->curPagePos,fHandle,memPage);
}

//might need to double check this
RC appendEmptyBlock(SM_FileHandle* fHandle)
{
	//open file
	// f1 = fopen(fHandle->fileName, "a") dont need open done so in ensure capacity
	//create empty block
	char *emptyblock = (char *) calloc(PAGE_SIZE, sizeof(char));
	//move file to beginning
	if(0 != fseek(f1, 0, SEEK_END))
		return RC_WRITE_FAILED;
	
	fwrite(emptyblock, 1, PAGE_SIZE, f1);		//append empty block
	fHandle->totalNumPages++;
	return RC_OK;
}

RC ensureCapacity(int numberOfPages, SM_FileHandle* fHandle)
{
	f1 = fopen(fHandle->fileName, "a");

	if (f1 == NULL)
		return RC_FILE_NOT_FOUND;

	while (numberOfPages>fHandle->totalNumPages)
		appendEmptyBlock(fHandle);
	
	fclose(f1);

	return RC_OK;
}

