// Manuel Lopez A20411680
// Mohammed Chisti A20393790


#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



void initStorageManager(void)
{
	printf("Database Storage Manager Initialized!\n");
}

RC createPageFile(char* fileName)
{
	FILE* f1;
	f1 = fopen(fileName, "w+"); // "w+" creates empty file

	// checks if f1 is null, if so then returns error
	if (f1 == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	else {
		for (int i = 0; i < PAGE_SIZE; i++) {
			fwrite("\0", sizeof(char), 1, f1);

		}
		// calloc here allocates page_size number of elements where each element is a size of char. Finally they are initialized to 0
		//char* fp = (char*)calloc(PAGE_SIZE, sizeof(char));
		/*
		size_t verifySize = fwrite(fp, sizeof(char), PAGE_SIZE, f1);
		if (verifySize != PAGE_SIZE)
		{
			return RC_FILE_SIZE_ERROR;
		}
		*/
		fclose(f1);
		return RC_OK;
	}

}


RC openPageFile(char* fileName, SM_FileHandle* fHandle)
{
	FILE* f1;
	f1 = fopen(fileName, "r+"); //read only empty file

	if (f1 == NULL) {
		printf("Requested file doesn't exists!\n");
		return RC_FILE_NOT_FOUND;
	}

	else {
		//fHandle->fileName = fileName;
		//moved down to organize code - Manuel
		fseek(f1, 0, SEEK_END);
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
	FILE* f1;
	f1 = (fHandle->mgmtInfo);

	if (f1 == NULL) {
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
	//FILE* f1;
	//f1 = fopen(fHandle->fileName, "r"); //read only empty file

	if (fHandle->mgmtInfo == NULL) {
		printf("Requested file doesn't exists!\n");
		return RC_FILE_NOT_FOUND;
	}
	if (fHandle == NULL) {
		printf("File Handler not initialized!\n");
		return RC_FILE_HANDLE_NOT_INIT;
	}
	if (pageNum< 0 || pageNum > fHandle->totalNumPages) {
		printf("Page doesn't exist! read \n");
		return RC_READ_NON_EXISTING_PAGE;
	}
	if (fseek(fHandle->mgmtInfo, (pageNum * PAGE_SIZE), SEEK_SET) == 0) { // fseek sets cursor position of file stream, returns 0 if successful
		fread((void*)memPage, PAGE_SIZE, 1, fHandle->mgmtInfo);
	}
	else {
		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos = pageNum+1; //sets new current page position
	//fclose(f1);
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
	//f1 = fopen(fHandle->fileName, "r+"); //read and write mode file
		//check if pageNum is greater than totalNumPages
	if (pageNum > fHandle->totalNumPages) {
		appendEmptyBlock(fHandle);//return error if pageNum is greater
	}
	if (fHandle->mgmtInfo == NULL) {
		printf("Requested file doesn't exists!\n");
		return RC_WRITE_FAILED;
	}
	if (fHandle == NULL) {
		printf("File Handler not initialized!\n");
		return RC_FILE_HANDLE_NOT_INIT;
	}
	if (pageNum< 0 || pageNum > fHandle->totalNumPages) {
		printf("Page doesn't exist! write \n");
		return RC_WRITE_FAILED;
	}
	//find file position and write
	if (fseek(fHandle->mgmtInfo, (pageNum * PAGE_SIZE), SEEK_SET) == 0) { // fseek sets cursor position of file stream, returns 0 if successful
		fwrite((void*)memPage, PAGE_SIZE, 1, fHandle->mgmtInfo);// write the file
	}
	else {
		return RC_WRITE_FAILED;
	}
	//set attributes
	fHandle->curPagePos = pageNum+1; //sets new current page position
	fseek(fHandle->mgmtInfo, 0, SEEK_END);
	fHandle->totalNumPages = ftell(fHandle->mgmtInfo) / PAGE_SIZE;
	//fclose(f1);
	return RC_OK;

}

RC writeCurrentBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
{
	return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

//might need to double check this
RC appendEmptyBlock(SM_FileHandle* fHandle)
{
	//open file
	// f1 = fopen(fHandle->fileName, "a") dont need open done so in ensure capacity
	//create empty block
	char* emptyblock = (char*)calloc(PAGE_SIZE, sizeof(char));
	//move file to beginning
	if (0 != fseek(fHandle->mgmtInfo, 0, SEEK_END))
		return RC_WRITE_FAILED;

	for (int i = 0; i < PAGE_SIZE; i++) {
		fwrite(emptyblock, sizeof(char), 1, fHandle->mgmtInfo);
		fseek(fHandle->mgmtInfo, 0L, 2);
	}		//append empty block
	fHandle->totalNumPages =ftell(fHandle->mgmtInfo)/PAGE_SIZE;
	fHandle->curPagePos = fHandle->totalNumPages - 1;
	free(emptyblock);
	return RC_OK;
}

RC ensureCapacity(int numberOfPages, SM_FileHandle* fHandle)
{
	//f1 = fopen(fHandle->fileName, "a");

	if (fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_FOUND;

	while (numberOfPages > fHandle->totalNumPages)
		appendEmptyBlock(fHandle);

	//fclose(f1);

	return RC_OK;
}

