#ifndef STRUCT_H_
#define STRUCT_H_

#include <time.h>
#include <sys/time.h>

/*
 * BM_PageFrame stores the information about a pageFrame.
 */
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


/*
 * BM_BookKeeping stores information associated  with
 * a buffer pool, maps file on disk to the memory/buffer.
 * Assigned to the mgmtData of BM_BufferPool
 */


typedef struct BM_BookKeeping{
	SM_FileHandle *fh;
	BM_PageFrame *buffer;
	int readCount;
	int writeCount;
	int ptrClock;

}BM_BookKeeping;



#endif /* STRUCT_H_ */
