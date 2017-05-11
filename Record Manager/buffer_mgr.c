#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "test_helper.h"
#include "struct.h"
#include <time.h>
#include <sys/time.h>





//BM_MemBufferPool : allocates memory for each page frame.
typedef char* BM_MemBufferPool;

// Buffer Manager Interface Pool Handling

/*
 * initBufferPool creates a new buffer pool with numPages page frames
 * initialize the BM_BufferPool,BM_PageFrame and BM_BookKeeping information
 */
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		const int numPages, ReplacementStrategy strategy,
		void *stratData){

	if(bm==NULL || pageFileName==NULL ||numPages ==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}
	bm->pageFile = pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;

	BM_PageFrame *buffer;
	buffer=malloc((numPages+1)*sizeof(BM_PageFrame));
	int i;

	for(i=0;i<numPages;i++){
		BM_MemBufferPool *memBufferPool;
		memBufferPool= (BM_MemBufferPool) malloc(PAGE_SIZE);
		(buffer+i)->pageFrameNum=i;
		(buffer+i)->pageFileNum=-1;
		(buffer+i)->fixCount=0;
		(buffer+i)->dirtyBit=false;

		//pointer to the page Frame
		(buffer+i)->pageFrameHandle=memBufferPool;

		// setting Time stamp for FIFO and LRU
		gettimeofday((&(buffer+i)->timer),NULL);

		// setting counter for LFU
		(buffer+i)->counterLFU=0;

		// setting reference bit for CLOCK
		(buffer+i)->referenceBit=false;
	}

	SM_FileHandle *fHandle;
	fHandle= malloc(sizeof(SM_FileHandle));
	openPageFile (pageFileName, fHandle);

	// initialize BM_BookKeeping and assign to mgmtData
	BM_BookKeeping *bk;
	bk=malloc(sizeof(BM_BookKeeping));

	bk->readCount=0;
	bk->writeCount=0;
	bk->fh=fHandle;
	bk->buffer=buffer;
	bk->ptrClock=0;

	bm->mgmtData=bk;

	return RC_OK;
}

/*
 * shutdownBufferPool destroys a buffer pool if there is no pinned page
 * It free up all resources associated with buffer pool.
 */
RC shutdownBufferPool(BM_BufferPool *const bm){

	if(bm==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}


	forceFlushPool(bm);

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	SM_FileHandle *fH;
	fH=(bk)->fh;

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	int i;
	for(i=0;i<bm->numPages;i++){
		if ((buffer+i)->fixCount!=0){

			//throw error if the buffer has pinned pages
			return RC_BUFFER_HAS_PINNED_PAGE;
		}

	}

	for(i=0;i<bm->numPages;i++){
		free((buffer+i)->pageFrameHandle);
	}

	free(buffer);
	free(fH);
	free(bk);

	return RC_OK;
}

/*
 * forceFlushPool causes all dirty pages (with fix count 0)
 * from the buffer pool to be written to disk.
 * Once written, it sets the dirtyBit to false
 */
RC forceFlushPool(BM_BufferPool *const bm){
	if(bm==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	SM_FileHandle *fH1;
	fH1=(bk)->fh;

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	int i;
	for(i=0;i<bm->numPages;i++){
		if(((buffer+i)->fixCount==0)&&((buffer+i)->dirtyBit==true)){
			writeBlock ((buffer+i)->pageFileNum, fH1, (buffer+i)->pageFrameHandle);

			(buffer+i)->dirtyBit=false;
			bk->writeCount=bk->writeCount+1;

		}
	}
	return RC_OK;
}

// Buffer Manager Interface Access Pages

/*
 * markDirty marks the given page as dirty.
 */
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){


	if(bm==NULL||page ==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}


	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	int i;
	for(i=0;i<bm->numPages;i++){

		if((page->pageNum==(buffer+i)->pageFileNum)&&(buffer+i)->pageFrameHandle==page->data){

			(buffer+i)->dirtyBit=true;
			break;
		}
	}

	return RC_OK;

}

/*
 * unpinPage unpins the given page
 * Reduces the fix count by 1
 */
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){

	if(bm==NULL||page==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}


	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	int i;
	for(i=0;i<bm->numPages;i++){
		if((page->pageNum==(buffer+i)->pageFileNum)&&((buffer+i)->fixCount>0)){
			(buffer+i)->fixCount=(buffer+i)->fixCount-1;
			break;
		}
	}

	return RC_OK;
}

/*
 * forcePage  writes the current content of the given page back to the
 * page file on disk if the page is dirty and its fix count is zero
 */
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){
	if(bm==NULL || page==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	SM_FileHandle *fH;
	fH=(bk)->fh;

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	int i;
	for(i=0;i<bm->numPages;i++){
		if(((buffer+i)->fixCount==0)&& (page->pageNum==(buffer+i)->pageFileNum)&&((buffer+i)->pageFrameHandle==page->data)&&((buffer+i)->dirtyBit=true)){
			writeBlock (page->pageNum, fH, page->data);
			bk->writeCount=bk->writeCount+1;
			(buffer+i)->dirtyBit=false;
			break;
		}

	}
	return RC_OK;
}
/*
 * pinPage pins the page with page number pageNum.
 * It sets the pageNum field and the data field of the BM_PageHandle.
 */
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
		const PageNumber pageNum){

	if(bm==NULL||page==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}

	int i,rt;

	//check whether page already exists or load into empty pageFrame(if available)
	rt=getFrame(bm,page,pageNum);

	if(rt==RC_PAGE_ALREADY_EXISTS || rt==RC_LOADED_INTO_EMPTY_PAGEFRAME){
		return	RC_OK;
	}

	//If pageFrame is full, replace a page using given strategy
	if(rt==RC_PERFORM_PAGE_REPLACEMENT)
	{

		BM_BookKeeping *bk;
		bk=(bm->mgmtData);
		SM_FileHandle *fH;
		fH=(bk)->fh;
		BM_PageFrame *buffer;
		buffer=bk->buffer;

		//get the position of the frame that has to be replaced.
		int frameOffset;
		switch (bm->strategy)
		{
		case RS_FIFO:
			frameOffset=getFIFO_LRUFrame(bm,pageNum);
			break;
		case RS_LRU:
			frameOffset=getFIFO_LRUFrame(bm,pageNum);
			break;
		case RS_CLOCK:
			frameOffset=getCLOCKFrame(bm,pageNum);
			break;
		case RS_LFU:
			frameOffset=getLFUFrame(bm,pageNum);
			break;
		default:

			break;
		}


		if(frameOffset!=-1)
		{
			//if the page is dirty,write back to disk before replacing it
			if((buffer+frameOffset)->dirtyBit==true){


				writeBlock ((buffer+frameOffset)->pageFileNum, fH, (buffer+frameOffset)->pageFrameHandle);
				bk->writeCount=bk->writeCount+1;

				(buffer+frameOffset)->dirtyBit=false;

			}

			//Read the pinned page
			int isRead;
			isRead=readBlock (pageNum, fH, (buffer+frameOffset)->pageFrameHandle);

			/*if the pinned page doesn't exists in the page File,
			then store it in the buffer till it gets written back to disk */
			if(isRead==RC_READ_NON_EXISTING_PAGE){
				memset((buffer+frameOffset)->pageFrameHandle,'\0',PAGE_SIZE);

			}
			if(isRead!=RC_READ_NON_EXISTING_PAGE){
				bk->readCount=bk->readCount+1;
			}

			//set the BM_PageFrame
			(buffer+frameOffset)->pageFileNum=pageNum;
			(buffer+frameOffset)->fixCount=1;
			(buffer+frameOffset)->counterLFU=1;
			(buffer+frameOffset)->referenceBit=true;



			gettimeofday((&(buffer+frameOffset)->timer),NULL);


			//set the BM_PageHandle
			page->pageNum=pageNum;
			page->data=(buffer+frameOffset)->pageFrameHandle;

			return RC_OK;


		}
		else{

			/*throw error if all the page Frame has pinned pages
			 * with fix count greater than zero
			 */

			return RC_NO_FREE_PAGE_FRAME;
		}
	}


}

/*
 * check whether page already exists or
 * loads requested page into empty pageFrame(if available)
 */
int getFrame(BM_BufferPool *const bm, BM_PageHandle *const page,
		const PageNumber pageNum)
{

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;
	int i;


	SM_FileHandle *fH;
	fH=(bk)->fh;

	if(fH->fileName==bm->pageFile){
		//check whether page already exists
		for(i=0;i<bm->numPages;i++){
			if((buffer+i)->pageFileNum==pageNum)
			{
				(buffer+i)->fixCount=(buffer+i)->fixCount+1;

				//If exists, then set BM_PageHandle
				page->pageNum=pageNum;
				page->data=(buffer+i)->pageFrameHandle;

				//set the parameters according to the given replacement strategy
				switch (bm->strategy)
				{
				case RS_LRU:
					gettimeofday((&(buffer+i)->timer),NULL);
					break;
				case RS_CLOCK:
					(buffer+i)->referenceBit=true;
					break;
				case RS_LFU:
					(buffer+i)->counterLFU++;
					break;

				default:

					break;
				}

				return RC_PAGE_ALREADY_EXISTS;


			}
		}

		//loads requested page into empty pageFrame(if available)
		for(i=0;i<bm->numPages;i++){
			if(((buffer+i)->pageFileNum==-1)){

				int isRead;
				isRead=readBlock (pageNum, fH,(buffer+i)->pageFrameHandle);
				if(isRead==RC_READ_NON_EXISTING_PAGE){
					memset((buffer+i)->pageFrameHandle,'\0',PAGE_SIZE);
				}
				bk->readCount=bk->readCount+1;
				(buffer+i)->pageFileNum=pageNum;
				gettimeofday((&(buffer+i)->timer),NULL);
				(buffer+i)->fixCount=(buffer+i)->fixCount+1;
				(buffer+i)->counterLFU=(buffer+i)->counterLFU+1;
				(buffer+i)->referenceBit=true;
				page->pageNum=pageNum;
				page->data=(buffer+i)->pageFrameHandle;
				return RC_LOADED_INTO_EMPTY_PAGEFRAME;

			}




		}
	}

	/*If page doesn't exist in buffer and there
	is no empty pageFrame then perform page Replacement*/
	return RC_PERFORM_PAGE_REPLACEMENT;
}

/*
 * getFIFO_LRUFrame calculates the minimum time stamp among pageFrames with
 * fix count=zero and returns its position
 */
int getFIFO_LRUFrame(BM_BufferPool *const bm,const PageNumber pageNum){

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	struct timeval min;
	min.tv_sec=NULL;
	min.tv_usec=NULL;

	int frameOffset =-1 ;
	int i;


	for(i=0;i<bm->numPages;i++){
		if((buffer+i)->fixCount==0){
			if( min.tv_sec== NULL){
				min.tv_sec = (buffer+i)->timer.tv_sec;
				min.tv_usec = (buffer+i)->timer.tv_usec;

			}
			if((min.tv_sec>(buffer+i)->timer.tv_sec)||(min.tv_sec==(buffer+i)->timer.tv_sec && min.tv_usec>=(buffer+i)->timer.tv_usec))
			{

				frameOffset = i;
				min.tv_sec=(buffer+i)->timer.tv_sec;
				min.tv_usec=(buffer+i)->timer.tv_usec;
			}
		}

	}

	return frameOffset;
}

/*
 * getLFUFrame finds the least accessed page in the pageFrames with
 * fix count=zero and returns its position
 */
int getLFUFrame(BM_BufferPool *const bm,const PageNumber pageNum){

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	int min=-1;
	int frameOffset =-1 ;
	int i;

	for(i=0;i<bm->numPages;i++){
		if((buffer+i)->fixCount==0){
			if( min == -1){
				min = (buffer+i)->counterLFU;
			}
			if(min>=(buffer+i)->counterLFU )
			{

				frameOffset = i;
				min=(buffer+i)->counterLFU;
			}
		}

	}

	return frameOffset;
}

/*
 * getCLOCKFrame sets the reference bit of the pageFrame to false
 * till it finds one with the reference bit as false and returns its position
 */
int getCLOCKFrame(BM_BufferPool *const bm,const PageNumber pageNum){

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	int frameOffset =-1 ;
	int count=0;
	int i=bk->ptrClock;

	for(i=bk->ptrClock;i<=bm->numPages;i++)
	{
		if(i==bm->numPages){
			i=0;
			count=count+1;
		}

		/*
		 * to avoid infinite looping, if clock finishes two cycles then
		 * break the loop and inform no page Frame is available
		 */

		if(count==2){
			break;
		}
		if((buffer+i)->fixCount==0){
			if((buffer+i)->referenceBit == true){
				(buffer+i)->referenceBit = false;
			}
			else if((buffer+i)->referenceBit == false){
				frameOffset=i;
				break;
			}



		}
		bk->ptrClock=i+1;
	}



	return frameOffset;
}


// Statistics Interface

/*
 * The getFrameContents function returns an array of PageNumbers
 * with respect to each pageFrame in the Buffer.
 */
PageNumber *getFrameContents (BM_BufferPool *const bm){

	if(bm==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	PageNumber *pageNumArray;
	pageNumArray=(PageNumber *) malloc(bm->numPages*sizeof(int));

	int i;
	for(i=0;i<bm->numPages;i++){
		if((buffer+i)->pageFileNum != -1){

			pageNumArray[i]= (buffer+i)->pageFileNum;
		}else{
			pageNumArray[i]= NO_PAGE;
		}

	}

	return pageNumArray;
}

/*
 * The getDirtyFlags function returns an array of dirty Bits
 * with respect to each page in the pageFrame of the Buffer
 */
bool *getDirtyFlags (BM_BufferPool *const bm){
	if(bm==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	bool *dirtyBitArray;
	dirtyBitArray=(PageNumber *) malloc(bm->numPages*sizeof(bool));

	int i;
	for(i=0;i<bm->numPages;i++){

		dirtyBitArray[i]= (buffer+i)->dirtyBit;
	}
	return dirtyBitArray;

}

/*
 * The getFixCounts function returns an array of fix counts
 * with respect to each page in the pageFrame of the buffer
 */
int *getFixCounts (BM_BufferPool *const bm){
	if(bm==NULL )
	{
		return RC_BUFFER_NOT_INIT;
	}

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	BM_PageFrame *buffer;
	buffer=bk->buffer;

	int *fixCountArray;
	fixCountArray=(PageNumber *) malloc(bm->numPages*sizeof(int));

	int i;
	for(i=0;i<bm->numPages;i++){
		if((buffer+i)->pageFileNum != -1){
			fixCountArray[i]= (buffer+i)->fixCount;
		}else{
			fixCountArray[i]= 0;
		}
	}
	return fixCountArray;
}

/*
 * The getNumReadIO function returns the total number of "reads"
 * performed since a buffer pool has been initialized.
 */
int getNumReadIO (BM_BufferPool *const bm){

	if(bm==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);
	return bk->readCount;
}

/*
 * The getNumWriteIO function returns the total number of "writes"
 * performed since a buffer pool has been initialized.
 */
int getNumWriteIO (BM_BufferPool *const bm){
	if(bm==NULL)
	{
		return RC_BUFFER_NOT_INIT;
	}

	BM_BookKeeping *bk;
	bk=(bm->mgmtData);

	return bk->writeCount;
}
