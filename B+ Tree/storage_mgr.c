#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "storage_mgr.h"

int isInitialized = 0;

/*
 *Initialize the system and sets the variable isInitialized to 1
 */
void initStorageManager (void){
	isInitialized = 1;
 }

/*
 * Function createPageFile creates a file with header and a single page.
 */
RC createPageFile(char *fileName){
if(isInitialized){
	//creates a new file
	int fileDesc = open(fileName,O_CREAT|O_RDWR,S_IRWXU);
	if(fileDesc < 0){
		return RC_FILE_CREATION_FAILED;
	}else{
		//Adding header with total Number of pages
		char header[HEADER_SIZE]="1";
		write(fileDesc,header,HEADER_SIZE);
		//Adding single page with '\0' bytes
		char firstEmptyPage[PAGE_SIZE];
		memset(firstEmptyPage,'\0',PAGE_SIZE);
		int currentPageSize = write(fileDesc,firstEmptyPage,PAGE_SIZE);
		if(currentPageSize < PAGE_SIZE){
			remove(fileName);
			return RC_FILE_CREATION_FAILED;
		}
		else{
			return RC_OK;
		}
	}
}else{
	return RC_STORAGE_MGR_NOT_INIT;
}
}

/*
 * Function openPageFile opens the pagefile with the given fileName.
 * File is opened and fHandle is populated.
 */
RC openPageFile (char *fileName, SM_FileHandle *fHandle){
if(isInitialized){
	//Opening a file
	int fileDesc = open(fileName,O_RDWR,S_IRWXU);
	if(fileDesc < 0){
		return RC_FILE_NOT_FOUND;
	}else{
		struct stat statusBuffer;
		fstat (fileDesc,&statusBuffer);
		//populating the SM_FileHandle structure with values of current file
		fHandle->curPagePos = 0;
		fHandle->fileName = fileName;
		fHandle->mgmtInfo = fileDesc;
		fHandle->totalNumPages = (statusBuffer.st_size-HEADER_SIZE)/PAGE_SIZE;
		return RC_OK;
	}
	}else{
		return RC_STORAGE_MGR_NOT_INIT;
	}
}

/*
 * Function closePageFile takes fHandle as input.
 * closes the file and clears the value of fHandle.
 */
RC closePageFile(SM_FileHandle *fHandle){
if(isInitialized){
	if((fHandle->fileName)== NULL){
		return RC_FILE_NOT_FOUND;
	}
	else{
		//To ensure that the  data has reached the disk
		fsync(fHandle->mgmtInfo);
		lseek(fHandle->mgmtInfo, 0, SEEK_SET);
		char header[HEADER_SIZE]="";
		snprintf(header, HEADER_SIZE, "%d", fHandle->totalNumPages);
		write(fHandle->mgmtInfo,header,HEADER_SIZE);
		//closing a file
		int returnClose=close(fHandle->mgmtInfo);
		if(returnClose< 0){
			return RC_FILE_CLOSE_FAILED;
		}else{
			//Setting the values in structure to null
			fHandle->fileName = NULL;
			fHandle->mgmtInfo = NULL;
			fHandle->totalNumPages = NULL;
			fHandle->curPagePos = NULL;
			return RC_OK;
		}
	}
	}else{
		return RC_STORAGE_MGR_NOT_INIT;
	}
}

/*
 * Function destroyPageFile takes fileName as input and
 * deletes that file.
 */
RC destroyPageFile (char *fileName){
if(isInitialized){
	int returnRemove;
	//Deleting the file from disk
	returnRemove = remove(fileName);
	if(returnRemove < 0){
		return RC_FILE_NOT_FOUND;
	}else{
		return RC_OK;
	}
	}else{
		return RC_STORAGE_MGR_NOT_INIT;
	}
}

/*
 * Function readBlock takes pageNum as input
 * and reads that page in the file to memPage
 */
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
if(isInitialized){
	if((fHandle->fileName) == NULL){
		return RC_FILE_NOT_FOUND;
	}else if(pageNum >= fHandle->totalNumPages || pageNum <0){
		return RC_READ_NON_EXISTING_PAGE;
	}else{
		off_t position = (pageNum)*PAGE_SIZE+HEADER_SIZE;
		//reading the pageNumth page
		int readBytes=pread(fHandle->mgmtInfo, memPage, PAGE_SIZE, position);
		if(readBytes < PAGE_SIZE){
			return RC_READ_BLOCK_FAILED;
		}
		else{
			//Updating the current position
			off_t posAfterRead = lseek(fHandle->mgmtInfo, position, SEEK_SET);
			fHandle->curPagePos = (posAfterRead-HEADER_SIZE)/PAGE_SIZE;
			return RC_OK;
		}
	}
	}else{
		return RC_STORAGE_MGR_NOT_INIT;
	}
}

/*
 * Function getBlockPos returns the current
 * page position in a file.
 */
int getBlockPos (SM_FileHandle *fHandle){
if(isInitialized){
	if((fHandle->fileName) == NULL){
		return RC_FILE_NOT_FOUND;
	}else{
		//getting the current position
		off_t currentPosBytes = lseek(fHandle->mgmtInfo,0, SEEK_CUR);
		if(currentPosBytes == (off_t) -1){
			return RC_CURRENT_POSITION_NOT_FOUND;
		}else{
			int currentBlockPos = (currentPosBytes-HEADER_SIZE)/PAGE_SIZE;
			return currentBlockPos;
		}
	}
	}else{
		return RC_STORAGE_MGR_NOT_INIT;
	}
}

/*
 * Function readFirstBlock gets the first page of the file
 * and writes it into memPage.
 */
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	//reading the first page
	return readBlock(0, fHandle, memPage);
}

/*
 * Function readPreviousBlock gets the previous page of the file
 * relative to the current page position and writes it into memPage.
 */
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	//reading the previous page
	return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

/*
 * Function readCurrentBlock gets the current page of the file
 * and writes it into memPage.
 */
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	//reading the current page
	return readBlock(fHandle->curPagePos, fHandle, memPage);
}

/*
 * Function readNextBlock gets the next page of the file relative
 * to the current page position and writes it into memPage.
 */
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	//reading the next page
	return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

/*
 * Function readLastBlock gets the last page of the file
 * and writes it into memPage.
 */
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	//reading the last page
	return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}


/*
 * Function writeBlock writes the content of memPage to the
 * pageNum th Block .
 */
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
if((isInitialized) && (pageNum >= 0) && (fHandle->fileName != NULL)){
	if(pageNum >= fHandle->totalNumPages){
				ensureCapacity(pageNum,fHandle);
			}
			off_t position = (pageNum)*PAGE_SIZE+HEADER_SIZE;
			//writing into the file at page number pageNum
			int writeBytes = pwrite(fHandle->mgmtInfo,memPage,PAGE_SIZE, position);

			if(writeBytes < PAGE_SIZE){
				return RC_WRITE_BLOCK_FAILED;
			}
			else{
				struct stat statusBuffer;
				//Updating the current position
				off_t posAfterWrite = lseek(fHandle->mgmtInfo, position, SEEK_SET);
				fstat(fHandle->mgmtInfo, &statusBuffer);
				fHandle->curPagePos = (posAfterWrite-HEADER_SIZE)/PAGE_SIZE;
				fHandle->totalNumPages = (statusBuffer.st_size-HEADER_SIZE)/PAGE_SIZE;
				return RC_OK;
			}

}else{
	if(!isInitialized){
		return RC_STORAGE_MGR_NOT_INIT;
	}else if(fHandle->fileName == NULL){
		return RC_FILE_NOT_FOUND;
	}else if(pageNum < 0){
		return RC_INVALID_PAGE;
	}
	}
}

/*
 * Function writeCurrentBlock writes the content
 * of memPage to the current page of the file.
 */
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	int pageNum = getBlockPos(fHandle);
	//writing into current block
	return writeBlock(pageNum,fHandle,memPage);
}

/*
 * Function appendEmptyBlock appends a single empty block to
 * the end of the file.
 */
RC appendEmptyBlock (SM_FileHandle *fHandle){
if(isInitialized){
	if((fHandle->fileName) == NULL){
		return RC_FILE_NOT_FOUND;
	}else{
		//moving to the end of the file
		lseek(fHandle->mgmtInfo,0,SEEK_END);
		char emptyPage[PAGE_SIZE];
		memset(emptyPage,'\0', PAGE_SIZE);
		//writing empty pages
		int ret = write(fHandle->mgmtInfo,emptyPage,PAGE_SIZE);
		if(ret < PAGE_SIZE){
			return RC_WRITE_FAILED;
		}else{
			struct stat statusBuffer;
			fstat(fHandle->mgmtInfo, &statusBuffer);
			fHandle->totalNumPages = (statusBuffer.st_size-HEADER_SIZE)/PAGE_SIZE;
			fHandle->curPagePos = fHandle->curPagePos + 1;
			return RC_OK;
		}
	}
	}else{
		return RC_STORAGE_MGR_NOT_INIT;
	}


}

/*
 * Function ensureCapacity ensures that file has  numberOfPages.
 */
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){
if(isInitialized){
	if((fHandle->fileName) == NULL){
		return RC_FILE_NOT_FOUND;
	}else{
		if(numberOfPages >= fHandle->totalNumPages){
			int appendPageCount = numberOfPages - fHandle->totalNumPages;
			int i;
			for(i = 0; i <=appendPageCount; i++){
				appendEmptyBlock(fHandle);
			}
		}

		return RC_OK;
	}
	}else{
		return RC_STORAGE_MGR_NOT_INIT;
	}
}
