Buffer Manager: Implement a buffer manager that manages a buffer of blocks in memory including reading/flushing to disk and 
block replacement (flushing blocks to disk to make space for reading new blocks from disk)
------------------------------------------------------------------------------------------------------------------------------------------
Implemented following page replacement strategies:

	1) FIFO
	2) LRU
	3) LFU
	4) CLOCK
*****************************************************************************************************************************************
Buffer_mgr.c
*****************************************************************************************************************************************

Structures created in buffer_mgr.c
------------------------------------

BM_PageFrame stores the information about a pageFrame.
 
	pageFrameHandle points to the memory of page frame
	timeval timer has the time stamp for FIFO and LRU
	referenceBit is for CLOCK strategy
	counterLFU tracks the number of times a page is pinned/accessed

typedef struct BM_PageFrame{
	int pageFrameNum;
	int pageFileNum;
	int fixCount;
	bool dirtyBit;
	char* pageFrameHandle;
	struct timeval timer;
	bool referenceBit;
	int counterLFU;
}BM_PageFrame;


BM_BookKeeping stores information associated  with a buffer pool, maps file on disk to the memory/buffer.
It will be assigned to the mgmtData of BM_BufferPool
 
typedef struct BM_BookKeeping{
	SM_FileHandle *fh;
	BM_PageFrame *buffer;
	int readCount;
	int writeCount;
	int ptrClock;

}BM_BookKeeping;

BM_MemBufferPool : allocates memory for each page frame.
typedef char* BM_MemBufferPool;

-----------------------------------------
Buffer Manager Interface Pool Handling
-----------------------------------------

Function: initBufferPool
Parameters: BufferPool, pageFileName, numPages of Page Frame, Replacement Stratergy [stratData for LRU-k if needed]
Returns: 1) if buffer pool is initialized successfully,it will return RC_OK
	 2) if it is not initliazed return RC_BUFFER_NOT_INIT
purpose: 1) Initialize the system and loads the buffer manager.
	 2) Creates and allocate space for the Empty page frame in buffer manager upto size numPages.
	 3) open the page file and initialize the BM_PageFrame for each page frame.


Function: shutdownBufferPool
Parameters: BM_BufferPool
Returns: 1) If BufferPool is not initliazed return RC_BUFFER_NOT_INIT
	 2) Return RC_OK If Buffer pool is shut down succesfully
	 3) Return RC_BUFFER_HAS_PINNED_PAGE if buffer pool has pinned pages
purpose: 1) if there is no pinned page it free up all resources associated with buffer pool.


Function: forceFlushpool
Parameters: BM_BufferPool 
Returns: 1) If BufferPool is not initialzed return error RC_BUFFER_NOT_INIT
	 2) if one or more pages are written succesfully to disk then return RC_OK.
	 3) If write fails return error RC_WRITE_FAILED.
purpose: 1) Loops through whole buffer pool and writes all the pages to the disk which are dirty and with zero fixcount
	 2) if write is successful it increases the writecount for the buffer pool 
	 3) sets the dirty bit to false after writing each page back to disk

-----------------------------------------
Buffer Manager Interface Access Pages
-----------------------------------------

Function: markDirty
Parameters: BM_BufferPool, BM_PageHandle 
Returns: 1) If  BufferPool is not initialized, then return RC_BUFFER_NOT_INIT
	 2) If page is marked dirty, then return RC_OK.
purpose: 1) marks the given page as dirty and sets the dirty bit as true.


Function: unpinPage 
Parameters: BM_BufferPool, BM_PageHandle 
Returns: 1) If  BufferPool is not initialized then return RC_BUFFER_NOT_INIT
	 2) If page is unpinned successfully, then return RC_OK.
purpose: 1) If fix count for given page is greather than 0 , decrease the fix count by 1 (unpins the page).
	

Function: forcePage
Parameters: BM_BufferPool , BM_PageHandle 
Returns: 1) if BufferPool is not initialzed return error RC_BUFFER_NOT_INIT
	 2) If page is written succesfully to the disk return RC_OK
	 3) If  the write fails return error RC_WRITE_FAILED.
purpose:1) if page is dirty and fixcount is 0 then write back to disk
	2) if given page doesn't exists on pageFile but exists in the buffer,the writeblock ensures pageFiles capacity. 
	2) if write is successful it increases the writecount for the buffer pool 
	3) sets the dirty bit to false after writing page back to disk
	

Function: pinPage 
Parameters: BM_BufferPool, BM_PageHandle, PageNumber
Returns: 1) if BufferPool is not initialzed return error RC_BUFFER_NOT_INIT
	 2) If a frame is available in memory and page is pinned successfully, then return RC_OK.
	 3) If frame is not available then return RC_NO_FREE_PAGE_FRAME
purpose: 1) If page is existing in page frame then point page handle pointer to that pageframe. 
	 2) If page does not exist in page frame and there is empty frame available then load the page into that frame
	 3) if empty  frame is not available then use replacement stratergy and replace the existing page frame with new pageNum
	 4) while replacement if exisiting page is dirty it will be written back to the disk  
 	 5) if user request a page that does not exist in disk file then temperory memory is assigned for that page and that page is written back into disk while replacement or when buffer memory is flushed
	 6) if fix count is not 0 for any page frame no replacement will be done, Requested page will not be loaded in memory and error message will be thrown.


Function: getFrame  
Parameters: BM_BufferPool, BM_PageHandle, PageNumber
Returns: 1) if page requested by user is existing in page frame it moves the page handle pointer to that page frame  and returns RC_PAGE_ALREADY_EXISTS
	 2) if page requested by user is loaded in any available empty page frame it returns RC_LOADED_INTO_EMPTY_PAGEFRAME
	 3) If any empty frame is not found for page to be loaded it returns RC_PERFORM_PAGE_REPLACEMENT
Purpose :1) To check if page requested by user is existing in page Frame or any frame is available for page to be loaded
	 2) If page does not exist in page frame it loops through buffer pool and checks if empty frame is available.
	 3) If there is empty frame available then read the page into that frame and set the page frame structure values  


Function: getFIFO_LRUFrame
Parameters: BM_BufferPool 
Returns: 1) If successful Returns the page Frame that was pinned first (FIFO Stratergy)
	 2) If no page frame can be replaced it Returns -1
Purpose: 1) Finds the frameoffset which was loaded first by its load time
	 2) Loop through buffer pool and sets the min value to frame that needs to be replaced using the FIFO Strategy


Function: getClockFrame
Parameters: BM_BufferPool
Returns: 1) Returns the position of the page to be replaced using CLOCK replacement strategy
purpose: 1) sets the reference bit of the page in the pageFrame to false till it finds one with the reference bit as false.
	 2) to avoid infinite looping, if clock finishes two cycles then break the loop and inform no page Frame is available with fix count=zero


Function: getLFUFrame
Parameters: BM_BufferPool
Returns: 1) Returns the position of the page to be replaced using LFU replacement strategy
purpose: 1) finds the least accessed/used page in the the buffer with fix count=zero.
	 2) It calculates the minimum value of BM_PageFrame counterLFU associated with the page in page frame

-----------------------------------------
Statistics Interface
-----------------------------------------

Function: getFrameContents
Parameters: BM_BufferPool 
Returns: 1) If BufferPool is not initalized, it returns RC_BUFFER_NOT_INIT
	 2) If excuted succesully, Returns array of pages which are stored in bufferpool
Purpose: 1) Gives an array of page number if page exist in a frame else displays no_page


Function: getDirtyFlags
Parameters: BM_BufferPool
Returns: 1) If BufferPool is not initalized, it returns RC_BUFFER_NOT_INIT
	 2) If excuted succesully, Returns page frame that are marked dirty
Purpose: 1) Gives an array of dirty bits associated with each page frame of a buffer pool.


Function: getFixCounts
Parameters: BM_BufferPool
Returns: 1) If BufferPool is not initalized, it returns RC_BUFFER_NOT_INIT
	 2) If excuted succesully, Returns fixcount of each page frames 
purpose: 1) Gives an array of fix count of all the pages loaded in buffer pool


Function: getNumReadIO
Parameters: BM_BufferPool
Returns: 1) If BufferPool is not initalized, it returns RC_BUFFER_NOT_INIT
	 2) Else, return the total number of read IO's for the given Buffer pool
purpose: 1) Gives the total number of read IO's performed since a buffer pool has been initialized.


Function: getNumWriteIO
Parameters:BM_BufferPool 
Returns: 1) If BufferPool is not initalized, it returns RC_BUFFER_NOT_INIT
	 2) Else, return the total number of write IO's for the given Buffer pool
purpose: 1) Gives the total number of write IO's performed since a buffer pool has been initialized.

***************************************************************************************************************************
buffer_mgr_stat.c
***************************************************************************************************************************

Free the memory allocated for the following varaiable in the calling functions a)printPoolContent and b) sprintPoolContent 
to avoid memory leak.

  PageNumber *frameContent;
  bool *dirty;
  int *fixCount;

***************************************************************************************************************************
dberror.h
***************************************************************************************************************************
Following errors/return codes have been added:

#define RC_BUFFER_HAS_PINNED_PAGE 12
#define RC_NO_FREE_PAGE_FRAME 13
#define RC_BUFFER_NOT_INIT 14
#define RC_PERFORM_PAGE_REPLACEMENT 15
#define RC_LOADED_INTO_EMPTY_PAGEFRAME 16
#define RC_PAGE_ALREADY_EXISTS 17



***************************************************************************************************************************
test_assign2_1.c
***************************************************************************************************************************

Test cases for LFU and CLOCK page replacement strategies have been added
--------------------------------------------------------------------------------------------------------------------------------