#ifndef DBERROR_H
#define DBERROR_H

#include "stdio.h"

/* module wide constants */
#define PAGE_SIZE 4096
#define HEADER_SIZE 20
/* return code definitions */
typedef int RC;

#define RC_OK 0
#define RC_FILE_NOT_FOUND 1
#define RC_FILE_HANDLE_NOT_INIT 2
#define RC_WRITE_FAILED 3
#define RC_READ_NON_EXISTING_PAGE 4
#define RC_FILE_CREATION_FAILED 5
#define RC_FILE_CLOSE_FAILED 6
#define RC_CURRENT_POSITION_NOT_FOUND 7
#define RC_READ_BLOCK_FAILED 8
#define RC_WRITE_BLOCK_FAILED 9
#define RC_STORAGE_MGR_NOT_INIT 10
#define RC_INVALID_PAGE 11

#define RC_BUFFER_HAS_PINNED_PAGE 12
#define RC_NO_FREE_PAGE_FRAME 13
#define RC_BUFFER_NOT_INIT 14
#define RC_PERFORM_PAGE_REPLACEMENT 15
#define RC_LOADED_INTO_EMPTY_PAGEFRAME 16
#define RC_PAGE_ALREADY_EXISTS 17

#define RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE 200
#define RC_RM_EXPR_RESULT_IS_NOT_BOOLEAN 201
#define RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN 202
#define RC_RM_NO_MORE_TUPLES 203
#define RC_RM_NO_PRINT_FOR_DATATYPE 204
#define RC_RM_UNKOWN_DATATYPE 205

#define RC_RM_DELETED_RECORD 206
#define RC_RM_RECORD_ID_NOT_FOUND 207
#define RC_RM_PAGE_NOT_FOUND 208

#define RC_IM_KEY_NOT_FOUND 300
#define RC_IM_KEY_ALREADY_EXISTS 301
#define RC_IM_N_TO_LAGE 302
#define RC_IM_NO_MORE_ENTRIES 303

/* holder for error messages */
extern char *RC_message;

/* print a message to standard out describing the error */
extern void printError (RC error);
extern char *errorMessage (RC error);

#define THROW(rc,message) \
  do {			  \
    RC_message=message;	  \
    return rc;		  \
  } while (0)		  \

// check the return code and exit if it is an error
#define CHECK(code)							\
  do {									\
    int rc_internal = (code);						\
    if (rc_internal != RC_OK)						\
      {									\
	char *message = errorMessage(rc_internal);			\
	printf("[%s-L%i-%s] ERROR: Operation returned error: %s\n",__FILE__, __LINE__, __TIME__, message); \
	free(message);							\
	exit(1);							\
      }									\
  } while(0);


#endif