Record Manager: Implement a simple record manager that allows navigation through records, and 
inserting and deleting records in blocks of a table.
 
------------------------------------------------------------------------------------------------------------------------------------------
Implemented following additional functionalities:

	1) Tombstone
		
*****************************************************************************************************************************************
record_mgr.c
*****************************************************************************************************************************************

Structures created in record_mgr.c
------------------------------------

RM_ScanMgmt stores the information about a Scan 
typedef struct RM_ScanMgmt
{
	int curSlot;
	int curPage;
	Expr *condition;

} RM_ScanMgmt;


typedef struct RM_TableInfo
{
	int totalNumOfPages;
	
} RM_TableInfo;

typedef struct RM_TableMgmtInfo
{
	RM_TableInfo *rmInfo;
	//RM_FreeSpaceMgmt *frSpace;
	BM_BufferPool *bm;
	BM_PageHandle *ph;

} RM_TableMgmtInfo;



-----------------------------------------
Record Manager Interface Handling
-----------------------------------------

Function: initRecordManager
Parameters: *mgmtData
Returns: 1) return RC_OK
purpose: 1) Initialize the system and loads the record manager.
	 
Function: shutdownRecordManager
Parameters: 
Returns: 1) Return RC_OK 

-----------------------------------------
Record Manager  Table Functions
-----------------------------------------

Function: createTable
Parameters:*name,*schema
Returns: 1) On success,return RC_OK	 
purpose: 1) Create a page file and stores the first page of file with table Information
	 2) Loads the table with the schema Information like Primary key,No of attributes 

Function: openTable 
Parameters:*rel,*name
Returns: 1) If Table Opened Successully and structure is initialized return RC_OK 
purpose: 1) Open the Page file and initialize the buffer pool
	 2) Gets the Table information from the first page of pageFile and stores it in structure RM_TableMgmtInfo,rmMgmtInfo.  
	
Function: closeTable ()
Parameters: *rel 
Returns: 1)  If table and page file are closed successfully, then return RC_OK.
purpose: 1) Close the table and free the schema related to the table
	 2) Close the page file 	

Function: deleteTable ()
Parameters: *name 
Returns: 1) If the table is deleted sucessfully returns RC_OK 
purpose: 1) Destroys the pageFile and table associated with it 

Function: getNumOfTuples ()
Parameters:  *rel
Returns: 1) If data exist in pageFile return total no of record count  
purpose: 1) For Each Page inside pageFile pin the page into memory 
	 2) Checks each record in the page data.
	 3) If data is present inside record (Not a tombstone #) it increments tuple count.
	 4) Else Check the NextRecord inside the page.
	
Function: createSchema
Parameters: numAttr,  **attrNames,  *dataTypes,  *typeLength, keySize,  *keys
Returns: 1) If schema is created Succesully return RC_OK;
purpose: 1) Allocate Memory for the schema
	 2) Initialize the schema with attributes of schema passed
	

Function: freeSchema
Parameters: Schema 
Returns: 1) If the schema is Freed Successfully returns RC_OK;
purpose: 1) Free the Memory related to the schema of table
	
-----------------------------------------
Record Manager  Record Functions
-----------------------------------------

Function: insertRecord 
Parameters: *rel,*record 
Returns: 1) if the record is inserted successully returns RC_OK
purpose: 1) Pin the page into the buffer memory 
	2) Checks in each page  if record can be added in page(page has enough space to fit the record size)
	   (record_size +Existing pageDataSize<page_size)
	3) If record can be fit into page adds the record at end of page.
	3) If record cannot be added pin the next page and checks again
	
Function: deleteRecord 
Parameters: *rel,id
Returns: 1) If successful ,returns RC_OK.
Purpose :1) Checks all the pages in the file to find a record with given id.
	 2) When found it replaces the data of that record with # and shifts all the records after it to occupy the space.

Function: updateRecord 
Parameters: *rel,*record
Returns: 1) If successful ,returns RC_OK.
	 2) If the record is deleted,returns RC_RM_DELETED_RECORD.
Purpose: 1) Gets the page which contains this record.
	 2) mark it dirty and update the record 
	 3) merge it with rest of the record in the page.

Function: getRecord 
Parameters:*rel,id,*record
Returns: 1) If successful ,returns RC_OK.
	 2) If the record is deleted,returns RC_RM_DELETED_RECORD.
	 3) If the page number is greater than the total number of pages,returns RC_RM_PAGE_NOT_FOUND.
	 4) If the  record with id is not present in the file, returns RC_RM_RECORD_ID_NOT_FOUND. 
purpose: 1) Checks all the pages of the file to find the record with given id.
	 2) Populates the record parameter with values of the record found in the file.

Function: getRecordSize 
Parameters: *schema
Returns: 1) Returns the size of each record.
purpose: 1) Depending on the type of each attribute,calculates the total size of single record.

Function: createRecord 
Parameters: **record,*schema
Returns: 1) If successful ,returns RC_OK.
purpose: 1) Allocate memory for each member of the record structure.
	 2) For id ,size of RID structure is allocated.
	 3) For data, record size is allocated.


Function: freeRecord
Parameters: *record
Returns: 1) If successful ,returns RC_OK.
purpose: 1) It frees the memory allocated to record variable.

Function: getAttribute()
Parameters:*record,*schema,attrNum,**value
Returns: 1) If successful ,returns RC_OK.
purpose: 1) Reads the data from record structure.
	 2) Checks schema of the table to get attrNum th attribute.
	 3) Reads value of attrNum th attribute from the data of the record.
	 4) Populates the value structure with the value from the record.
	
Function: setAttribute()
Parameters: *record,*schema,attrNum,*value
Returns: 1) If successful ,returns RC_OK.
purpose: 1) Finds the offset for the attrNum th attribute.
	 2) Copies the value to the record data at the calculated offset.


-----------------------------------------
Record Manager  Scan Access Functions
-----------------------------------------

Function: startScan
Parameters:*rel,*scan,*cond
Returns: 1)If successful ,returns RC_OK.
Purpose: 1) Intialise *scanMgmt
	 2) Populate the *scanMgmt with cond, curSlot and curPage.
	 3) Copy *scanMgmt to the mgmtData of *scan.
	 4) Copy *rel to the scan->rel.

Function: next 
Parameters: *scan,*record
Returns: 1)If successful ,returns RC_OK.
	 2) If no more tuple in the file,return RC_RM_NO_MORE_TUPLES.
Purpose: 1) Gets the curPage and curSlot from the mgmtData of scan.
	 2) Calls the getRecord function by passing curpage and curSlot.
	 3) If the return value of getRecord-
		- RC_RM_DELETED_RECORD then increment curSlot and call next.
		- RC_RM_RECORD_ID_NOT_FOUND then increment curpage and set curSlot to 0 and call next.
		- RC_RM_PAGE_NOT_FOUND then return RC_RM_NO_MORE_TUPLES.
	 4) After we get record,check if it satisfies the condition given.
	 5) If yes, then return that record else call next with curSlot incremented.


Function: closeScan
Parameters: *scan
Returns: 1)If successful ,returns RC_OK. 
purpose: 1) Frees the memory allocated to the scan structure.

***************************************************************************************************************************
dberror.h
***************************************************************************************************************************
Following errors/return codes have been added:
RC_RM_DELETED_RECORD 206 To Check if Record is deleted
RC_RM_RECORD_ID_NOT_FOUND 207 To Check if Record exists in page file
RC_RM_PAGE_NOT_FOUND 208 To check if the Record to be fetched has right slot number.
----------------------------------------------------------------------------------------------------------------------------