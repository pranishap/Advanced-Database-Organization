Storage Manager: Implement a storage manager that allows read/writing of blocks to/from a file on disk
**************************************************************************************************************************
storage_mgr.c
**************************************************************************************************************************

Manipulating page files
------------------------

Function: initStorageManager 
Parameters: Void
Returns: void
purpose:1) Initialize the system and sets the variable isInitialized to 1.
	2) All the functions will be executed only if isInitialized is set to 1.
	3) If it is not set, then all functions will return RC_STORAGE_MGR_NOT_INIT.

Function: createPageFile 
Parameters: fileName
Returns:1) If file is created successfully, then return RC_OK.
	2) If file creation fails, then return RC_FILE_CREATION_FAILED.
purpose:1) Creates a new page file with a given file name. 
	2) Creates header of 20 bytes which stores the information of total number of pages. 
	3) Creates single page of fixed size (4096 bytes) and fills it with '\0' bytes.


Function: openPageFile 
Parameters: fileName, fHandle 
Returns:1) If file is openned successfully, then return RC_OK.
	2) If it fails, then return RC_FILE_NOT_FOUND.
purpose:1) Opens a page file with the given file name.
	2) Sets the fHandle values with the current file details.
	3) Stores the file descriptor value in the mgmtInfo of the fHandle .

Function: closePageFile 
Parameters:fHandle
Returns:1) If file closes successfully, then return RC_OK.
	2) If it fails, then return RC_FILE_CLOSE_FAILED.
purpose:1) Ensures that the data has reached the disk.
	2) Closes the file pointed by the mgmtInfo of the fHandle.
	3) Clears the fHandle values.

Function: destroyPageFile 
Parameters: fileName
Returns:1) If file exists and file is deleted then return RC_OK.
	2) If file doesn't exists, then return RC_FILE_NOT_FOUND.
purpose:1) Deletes the file with the given fileName.

 Reading blocks from disc
-------------------------

Function:readBlock  
Parameters: pageNum, fHandle, memPage
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If the pageNum is less than zero or greater than equal to total number of pages, 
	   then return RC_READ_NON_EXISTING_PAGE.
	3) If the block was read succesfully, then RC_OK.
	4) If couldn't read the block, then RC_READ_BLOCK_FAILED.
purpose:1) Reads the pageNumth block to memPage.
	2) Updates the current page position.

Function: getBlockPos 
Parameters: fHandle
Returns:1) If the fileName of fHandle is empty, then return RC_FILE_NOT_FOUND.
	2) If couldn't locate the current position, 
	   then return RC_CURRENT_POSITION_NOT_FOUND.
	3) If current position is found successfully, then returns its block position. 
purpose:1) Gets the current block position.

Function: readFirstBlock 
Parameters: fHandle, memPage
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If the pageNum is less than zero or greater than equal to total number of pages, 
	   then return RC_READ_NON_EXISTING_PAGE.
	3) If the first block was read succesfully, then RC_OK.
	4) If the first block read fails, then RC_READ_BLOCK_FAILED.
purpose:1) Reads the first block of the file(after the header).
	2) Calls the readBlock function with zero as pageNum value.

Function: readPreviousBlock
Parameters: fHandle,memPage 
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If the pageNum is less than zero or greater than equal to total number of pages, 
	   then return RC_READ_NON_EXISTING_PAGE.
	3) If the previous block from current position was read succesfully, then RC_OK.
	4) If previous block read fails, then RC_READ_BLOCK_FAILED.
purpose:1) Reads the previous block from current position.
	2) Calls the readblock function with (currentposition-1) as pageNum value.

Function:  readCurrentBlock 
Parameters: fHandle, memPage
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If the pageNum is less than zero or greater than equal to total number of pages, 
	   then return RC_READ_NON_EXISTING_PAGE.
	3) If the current block  was read succesfully, then RC_OK.
	4) If the current block read fails, then RC_READ_BLOCK_FAILED.
purpose:1) Reads the current block.
	2) Calls the readblock function with currentposition as pageNum value.

Function: readNextBlock 
Parameters: fHandle, memPage
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If the pageNum is less than zero or greater than equal to total number of pages, 
	   then return RC_READ_NON_EXISTING_PAGE.
	3) If the next block from current position was read succesfully, then RC_OK.
	4) If the next block read fails, then RC_READ_BLOCK_FAILED.
purpose:1) Reads the next page block from the current position.
	2) Calls the readblock function with (currentposition+1) as pageNum value.

Function: readLastBlock 
Parameters:fHandle, memPage
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If the pageNum is less than zero or greater than equal to total number of pages, 
	   then return RC_READ_NON_EXISTING_PAGE.
	3) If the last block was read succesfully, then RC_OK.
	4) If the last block read fails, then RC_READ_BLOCK_FAILED.
purpose:1) Reads the last page block by moving file pointer to last page.
	2) Calls the readblock function with (totalnumberofpages-1) as pageNum value.
	   
Writing blocks to a page file
------------------------------

Function: writeBlock 
Parameters: pageNum,fHandle,memPage
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If pageNum less than 0, then return RC_INVALID_PAGE.
	3) If write was successful,return RC_OK.
	4) If it fails, return RC_WRITE_BLOCK_FAILED.
purpose:1) If the pageNum is greater than or equal to total number of pages then call function ensureCapacity.
	2) Sets the file pointer to the pageNum th page.
	3) Writes the memPage into the file.
	4) Updates the values of fHandle. 

Function: writeCurrentBlock 
Parameters: fHandle,memPage
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If write was successful,return RC_OK.
	3) If it fails, return RC_WRITE_BLOCK_FAILED.
purpose:1) Gets the current position of the file.
	2) Calls writeBlock function with current position as pageNum.

Function: appendEmptyBlock 
Parameters: fHandle
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If append was successful,then return RC_OK.
 	3) If it fails,then return RC_WRITE_FAILED.
purpose:1) Sets the pointer to the end of the file
	2) Populates the emptyPage with 4096 bytes of '\0' using memset.
	3) Writes the emptyPage into the file
	4) Updates the fHandle with current values.

Function: ensureCapacity
Parameters: numberOfPages, fHandle
Returns:1) If the file is not open or doesn't exist,then return RC_FILE_NOT_FOUND.
	2) If successful,then return RC_OK.
purpose:1) Checks if numberOfPages is greater than or equal to total number of pages in the file.
	2) If yes then find the difference in the number of the pages and call
	   appendEmptyBlock iteratively for that difference.

***************************************************************************************************************************
dberror.h
***************************************************************************************************************************
Following errors have been added:

#define RC_FILE_CREATION_FAILED 5
#define RC_FILE_CLOSE_FAILED 6
#define RC_CURRENT_POSITION_NOT_FOUND 7
#define RC_READ_BLOCK_FAILED 8
#define RC_WRITE_BLOCK_FAILED 9
#define RC_STORAGE_MGR_NOT_INIT 10
#define RC_INVALID_PAGE 11


***************************************************************************************************************************
test_assign1_1.c
***************************************************************************************************************************

In testSinglePageContent function, free the memory pointed by pageHandle.

***************************************************************************************************************************
