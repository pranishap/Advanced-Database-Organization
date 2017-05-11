#include <stdio.h>
#include "record_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "struct.h"
#include <string.h>

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
	BM_BufferPool *bm;
	BM_PageHandle *ph;

} RM_TableMgmtInfo;

/*
 * Initializes the storage manager
 *
 */
RC initRecordManager (void *mgmtData){
	if(mgmtData==NULL){
		initStorageManager();
	}
	return RC_OK;
}
/*
 * Shuts down the record manager
 *
 */
RC shutdownRecordManager (){
	return RC_OK;

}
/*
 * Creates file with the given name and writes the
 * schema into the first page of the file.
 *
 */
RC createTable (char *name, Schema *schema){

	//Generating the string of schema structure to write
	//it in the first page of file
	char *p;
	int i,j;
	sprintf(p,"%d",schema->numAttr);
	strcpy(p+strlen(p),";");
	sprintf(p+strlen(p),"%d",schema->keySize);
	strcpy(p+strlen(p),";");

	for(i = 0; i < schema->numAttr; i++)
	{
		strcpy(p+strlen(p),schema->attrNames[i]);
		strcpy(p+strlen(p),",");
		//checking the dataType to append datatype and length to the
		//string
		switch (schema->dataTypes[i])
		{
		case DT_INT:
			strcpy(p+strlen(p), "0");
			strcpy(p+strlen(p),",");
			sprintf(p+strlen(p),"%d",schema->typeLength[i]);
			strcpy(p+strlen(p),",");
			break;
		case DT_FLOAT:
			strcpy(p+strlen(p),"2");
			strcpy(p+strlen(p),",");
			sprintf(p+strlen(p),"%d",schema->typeLength[i]);
			strcpy(p+strlen(p),",");
			break;
		case DT_STRING:
			strcpy(p+strlen(p),"1");
			strcpy(p+strlen(p),",");
			sprintf(p+strlen(p),"%d",schema->typeLength[i]);
			strcpy(p+strlen(p),",");
			break;
		case DT_BOOL:
			strcpy(p+strlen(p),"3");
			strcpy(p+strlen(p),",");
			sprintf(p+strlen(p),"%d",schema->typeLength[i]);
			strcpy(p+strlen(p),",");
			break;
		}
		int isPrimary= 0;
		for(j = 0; j < schema->keySize; j++)
		{
			if(i == schema->keyAttrs[j] ){
				sprintf(p+strlen(p),"%d",1);
				isPrimary = 1;
				break;
			}

		}
		if(isPrimary== 0){
			sprintf(p+strlen(p),"%d",0);
		}

		strcpy(p+strlen(p),";");

	}

	SM_PageHandle ph = (SM_PageHandle) malloc(PAGE_SIZE);
	strcpy(ph,p);
	//creating the file
	int isCreated = createPageFile(name);

	if(isCreated == RC_OK){
		SM_FileHandle fHandle;
		int isOpen = openPageFile(name,&fHandle);

		if(isOpen == RC_OK){
			//writing the schema into the file
			writeBlock (0, &fHandle,ph);

			closePageFile (&fHandle);

		}
	}


	return RC_OK;
}
/*
 * Open the file with given name and reads the first page (schema)
 * and populates RM_TableData structure.
 *
 */
RC openTable (RM_TableData *rel, char *name){

	RM_TableMgmtInfo *rmMgmtInfo=(RM_TableMgmtInfo *)malloc(sizeof(RM_TableMgmtInfo));
	rmMgmtInfo->bm= (BM_BufferPool *)malloc (sizeof(BM_BufferPool));
	rmMgmtInfo->ph= (BM_PageHandle *)malloc (sizeof(BM_PageHandle));
	rmMgmtInfo->rmInfo=(RM_TableInfo *)malloc (sizeof(RM_TableInfo));
	//intilialises the buffer pool
	initBufferPool(rmMgmtInfo->bm, name, 3, RS_FIFO, NULL);
	//reads the first page and populates the *rel
	pinPage(rmMgmtInfo->bm, rmMgmtInfo->ph, 0);
	unpinPage (rmMgmtInfo->bm, rmMgmtInfo->ph);
	char *ph=rmMgmtInfo->ph->data;

	 //deserializing the schema
	int k=strlen(ph);
	char *p;
	char arr[k];
	strcpy(arr,ph);
	int t ;
	int charcount;
	charcount = 0;

	for(t=0; ph[t]; t++) {
		if(ph[t] == ';') {
			charcount ++;
		}
	}


	char **array=(char **)malloc(sizeof(char *) * charcount);
	int j=0;

	p = strtok(arr,";");
	while( p != NULL )
	{
		array[j]=(char *)malloc(sizeof(char)*strlen(p));
		strcpy(array[j],p);
		j++;
		p= strtok(NULL,";");
	}

	int numAttr=atoi(array[0]);

	int keySize=atoi(array[1]);
	char **subArray=(char **)malloc(sizeof(char *) * (numAttr*4));
	char *r;
	int a;
	int i=0;
	for(a=2;a<charcount;a++){


		int arrLen=strlen(array[a]);
		char app[arrLen];
		strcpy(app,array[a]);
		r = strtok(app,",");
		while( r != NULL )
		{
			subArray[i]=(char *)malloc(sizeof(char)*strlen(r));
			strcpy(subArray[i],r);

			i++;

			r=strtok(NULL,",");
		}
	}

	rel->schema=(Schema *)malloc(sizeof(Schema));

	rel->schema->attrNames=(char**) malloc( sizeof(char*)*numAttr);
	rel->schema->dataTypes=(DataType*) malloc( sizeof(DataType)*numAttr);
	rel->schema->typeLength=(int*) malloc( sizeof(int)*numAttr);
	rel->schema->keyAttrs=(int*) malloc( sizeof(int)*keySize);

	rel->name=name;
	rel->schema->numAttr=numAttr;

	int m=0,n=0,recordSize;
	for(i=0;i<numAttr;i++)
	{

		rel->schema->attrNames[i]=subArray[i+m+0];

		switch(atoi(subArray[i+1+m]))
		{
		case 0:
			rel->schema->dataTypes[i]=DT_INT;
			recordSize=recordSize+sizeof(int);
			break;

		case 2:
			rel->schema->dataTypes[i]=DT_FLOAT;
			recordSize=recordSize+sizeof(float);

			break;
		case 1:
			rel->schema->dataTypes[i]=DT_STRING;
			recordSize=recordSize+atoi(subArray[i+2+m]);

			break;
		case 3:
			rel->schema->dataTypes[i]=DT_BOOL;
			recordSize=recordSize+sizeof(bool);

			break;
		}
		rel->schema->typeLength[i]=atoi(subArray[i+2+m]);
		int key=atoi(subArray[i+3+m]);
		if(key==1)
		{
			rel->schema->keyAttrs[n]=i;
			n++;

		}

		m=m+3;
		if(i==(numAttr-1)){

			recordSize=recordSize+2;
		}
		else{

			recordSize=recordSize+1;
		}
	}
	rel->schema->keySize=keySize;


	BM_BookKeeping *bk;
	bk=rmMgmtInfo->bm->mgmtData;
	SM_FileHandle *fH1;
	fH1=(bk)->fh;
	rmMgmtInfo->rmInfo->totalNumOfPages=fH1->totalNumPages;
	rel->mgmtData=rmMgmtInfo;

	return RC_OK;
}

/*
 * Closes the file and shuts down the buffer pool and frees the related memory
 *
 */

RC closeTable (RM_TableData *rel){

	RM_TableMgmtInfo *rmMgmtInfo;
	rmMgmtInfo=rel->mgmtData;
	int ret=shutdownBufferPool(rmMgmtInfo->bm);
	printf("Shut down buffer %d \n",ret);
	BM_BookKeeping *bk;
	bk=rmMgmtInfo->bm->mgmtData;
	closePageFile(bk->fh);


	free(rel->mgmtData);
	free(rel->schema->dataTypes);

	free(rel->schema->attrNames);

	free(rel->schema->keyAttrs);
	free(rel->schema->typeLength);

	free(rel->schema);


	return RC_OK;
}
/*
 * Deletes the file
 *
 */
RC deleteTable (char *name){

	destroyPageFile (name);
	return RC_OK;
}
/*
 * Gets the total number of records in the table
 *
 */
int getNumTuples (RM_TableData *rel){
	RM_TableMgmtInfo *rmMgmtInfo;
	rmMgmtInfo=(rel->mgmtData);
	int pageNum=rmMgmtInfo->rmInfo->totalNumOfPages;
	//pins each page and counts number of valid records in that page
	int i;
	int numTuples=0;
	for(i=1;i<pageNum;i++){
		pinPage(rmMgmtInfo->bm ,rmMgmtInfo->ph, i);
		char *p;
		p=strtok(rmMgmtInfo->ph->data,"(");
		while( (p != NULL) )
		{
			p= strtok(NULL,"(");
			if(p != NULL){
				numTuples=numTuples+1;
			}
		}
		unpinPage(rmMgmtInfo->bm ,rmMgmtInfo->ph);
	}
	return numTuples;
}

/*
 * Inserts the record into the file
 *
 */
RC insertRecord (RM_TableData *rel, Record *record){

	RM_TableMgmtInfo *rmMgmtInfo;
	rmMgmtInfo=(rel->mgmtData);
	int pageNum=rmMgmtInfo->rmInfo->totalNumOfPages;
	record->id.page=0;
	record->id.slot=0;
	//Gets the last RID of the page,calculates RID for
	//new record and add the new record
	int i=1;
	while(record->id.page<100){
		int ret=pinPage(rmMgmtInfo->bm ,rmMgmtInfo->ph, i);
		rmMgmtInfo->rmInfo->totalNumOfPages=i+1;
		int strBuffer=strlen(rmMgmtInfo->ph->data);
		char *strp = rmMgmtInfo->ph->data;
		record->id.page=rmMgmtInfo->ph->pageNum;
		int counter = 0;
		int counterb = 0;
		int m;
		for(m=0;m <strBuffer;m++){
			if( strncmp(strp+m,"-",1) == 0){
				counter = m;
			}
			if( strncmp(strp+m,"]",1) == 0){
				counterb = m;
			}

		}
		if(strBuffer!=0){
			char *slotStrn, *p;
			int slotLen;
			slotLen= counterb - counter;
			strncpy(slotStrn,strp+counter+1,slotLen);
			p = strtok(slotStrn,"]");
			record->id.slot=(atoi(p))+1;
		}
		else{
			record->id.slot=0;
		}
		char *str=serializeRecord(record,rel->schema);
		int bufSize=strlen(str);
		if((strBuffer+bufSize)<PAGE_SIZE){
			markDirty (rmMgmtInfo->bm, rmMgmtInfo->ph);
			strcat(rmMgmtInfo->ph->data,str);
			int ret1=unpinPage(rmMgmtInfo->bm, rmMgmtInfo->ph);

			break;
		}

		i++;
		record->id.page=i;

		unpinPage(rmMgmtInfo->bm, rmMgmtInfo->ph);

	}

	return RC_OK;
}
/*
 * Delete the record with given RID from the table
 *
 */
RC deleteRecord (RM_TableData *rel, RID id){
	//removes the data of the record and replaces it with #
	RM_TableMgmtInfo *rmMgmtInfo;
	rmMgmtInfo=(rel->mgmtData);
	pinPage(rmMgmtInfo->bm ,rmMgmtInfo->ph,id.page);
	int strBuffer,len,j,y;
	strBuffer=strlen(rmMgmtInfo->ph->data);
	char *strp = rmMgmtInfo->ph->data;
	char *strp1=rmMgmtInfo->ph->data;

	char *rid=(char *)malloc(sizeof(char)*20);
	char *rid1=(char *)malloc(sizeof(char)*20);
	sprintf(rid,"%s%d-%d%s","[",id.page,id.slot,"]");
	sprintf(rid1,"%s%d-%d%s","[",id.page,id.slot+1,"]");
	char *r, *p ,*s;
	r=(char *)malloc(sizeof(char)*strBuffer);
	p=strstr(strp, rid);

	len=strlen(strp)-strlen(p);

	s=strstr(strp1,rid1);
	if(len!=0){
		strncpy(r,strp,len);
	}


	char *str=(char *)malloc(sizeof(char)*10);
	sprintf(str,"%s%d-%d%s","[",id.page,id.slot,"]#");

	if(len==0){
		strcpy(r,str);
	}

	else
	{
		strcat(r,str);
	}
	if(s!=NULL){
		strcat(r,s);
	}

	int bufSize=strlen(r);

	if((bufSize)<PAGE_SIZE){

		markDirty (rmMgmtInfo->bm, rmMgmtInfo->ph);
		strcpy(rmMgmtInfo->ph->data,r);
		int ret1=unpinPage(rmMgmtInfo->bm, rmMgmtInfo->ph);


	}

	free(rid);
	free(rid1);
	return RC_OK;


}
/*
 * Updates the given record into the table
 *
 */
RC updateRecord (RM_TableData *rel, Record *record){
	//finds the page where record is present
	// splits the string of records of that page into 3 string
	//first string is the string before the record to be updated
	//second string the record
	//third string is the string after the record
	RM_TableMgmtInfo *rmMgmtInfo;
	rmMgmtInfo=(rel->mgmtData);
	pinPage(rmMgmtInfo->bm ,rmMgmtInfo->ph, record->id.page);
	int strBuffer,len,j,y;
	strBuffer=strlen(rmMgmtInfo->ph->data);
	char *strp = rmMgmtInfo->ph->data;
	char *strp1=rmMgmtInfo->ph->data;
	char *rid=(char *)malloc(sizeof(char)*20);
	char *rid1=(char *)malloc(sizeof(char)*20);
	sprintf(rid,"%s%d-%d%s","[",record->id.page,record->id.slot,"]");
	sprintf(rid1,"%s%d-%d%s","[",record->id.page,record->id.slot+1,"]");
	char *r, *p ,*s;
	r=(char *)malloc(sizeof(char)*strBuffer);
	p=strstr(strp, rid);

	if((strncmp(p+strlen(rid),"#",1))==0){


		return RC_RM_DELETED_RECORD;
	}


	len=strlen(strp)-strlen(p);

	s=strstr(strp1,rid1);
	if(len!=0){
		strncpy(r,strp,len);
	}
	//after update the record it is serialised and appended to the first record and then
	//third string is appended to the it

	char *str=serializeRecord(record,rel->schema);
	if(len==0){
		strcpy(r,str);
	}
	else
	{
		strcat(r,str);
	}
	if(s!=NULL){
		strcat(r,s);
	}

	int bufSize=strlen(r);

	if((bufSize)<PAGE_SIZE){

		markDirty (rmMgmtInfo->bm, rmMgmtInfo->ph);
		strcpy(rmMgmtInfo->ph->data,r);
		int ret1=unpinPage(rmMgmtInfo->bm, rmMgmtInfo->ph);


	}

	free(rid);
	free(rid1);
	return RC_OK;
}
/*
 * Gets the record based on the RID and populate it in record structure
 *
 */
RC getRecord (RM_TableData *rel, RID id, Record *record){

	RM_TableMgmtInfo *rmMgmtInfo;
	rmMgmtInfo=(rel->mgmtData);

	if(id.page>=rmMgmtInfo->rmInfo->totalNumOfPages){
		return RC_RM_PAGE_NOT_FOUND;
	}
	pinPage(rmMgmtInfo->bm ,rmMgmtInfo->ph, id.page);
	int strBuffer,len,j,y;
	strBuffer=strlen(rmMgmtInfo->ph->data);
	char *strp = rmMgmtInfo->ph->data;
	unpinPage(rmMgmtInfo->bm, rmMgmtInfo->ph);
	char *rid=malloc(sizeof(char)*20);
	sprintf(rid,"%s%d-%d%s","[",id.page,id.slot,"]");
	char *r, *p ,*q,*t;
	//checks each record if the RID matches
	p=strstr(strp, rid);

	if(p==NULL){
		return RC_RM_RECORD_ID_NOT_FOUND;
	}
	//if after RID  # is present instead of data it means its deleted record
	if((strncmp(p+strlen(rid),"#",1))==0){


		return RC_RM_DELETED_RECORD;
	}

	r=strstr(p, ")");
	len=strlen(p)-strlen(r)+1;

	char *s=(char *)malloc(sizeof(char)*len);
	memcpy(s,p,len);
	q = strpbrk(s,"(");

	t=strtok(q+1,")");
	int len1=strlen(t);
	char *k=(char *)malloc(sizeof(char)*len1);
	strcpy(k,t);
	char **array=(char **)malloc(sizeof(char *) *((rel->schema->numAttr)*2));
	j=0;
	y=0;
	int x,num;
	array[j]=strtok(k,",:");
	num=rel->schema->numAttr;

	while(array[j]!=NULL)
	{

		j++;
		array[j]= strtok(NULL,",:");

	}
	record->id.page=id.page;
	record->id.slot=id.slot;
	Value *value;
	j=0;
	for(x=1;x<num*2;x=x+2){
		y=j;


		switch(rel->schema->dataTypes[y]){
		case DT_INT:


			MAKE_VALUE(value,DT_INT,atoi(array[x]));
			break;

		case DT_FLOAT:

			MAKE_VALUE(value,DT_FLOAT,atof(array[x]));

			break;
		case DT_STRING:

			MAKE_STRING_VALUE(value,array[x]);
			break;
		case DT_BOOL:
			value->v.boolV=(bool)malloc(sizeof(bool));
			value->dt=DT_BOOL;
			value->v.boolV=(array[x] == "TRUE") ? TRUE : FALSE;

			break;
		}

		setAttr (record, rel->schema, y,value);
		j++;
		free(value);
	}

	return RC_OK;
}

/*
 * Starts scan set the starting page and slot number and
 *  saves the condition in RM_ScanHandle
 *
 */
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){
	scan->rel = rel;
	RM_ScanMgmt *scanMgmt;
	scanMgmt= (RM_ScanMgmt *) malloc(sizeof(RM_ScanMgmt));
	scanMgmt->curPage = 1;
	scanMgmt->curSlot = 0;
	scanMgmt->condition = cond;
	scan->mgmtData = scanMgmt;
	return RC_OK;
}
/*
 * Gets each record in the table and checks if it satisfies the condition,
 * if yes then populates the record function
 *
 */
RC next (RM_ScanHandle *scan, Record *record){
	RM_ScanMgmt *scanMgmt;
	Value *value;
	scanMgmt = scan->mgmtData;
	RC status;
	record->id.slot = scanMgmt->curSlot;
	record->id.page = scanMgmt->curPage;

	int returVal = getRecord(scan->rel, record->id, record);
	//if the record is deleted
	if(returVal == RC_RM_DELETED_RECORD){
		(scanMgmt->curSlot)++;
		scan->mgmtData = scanMgmt;
		return next(scan, record);
	}
	//if record is not found ,move to next page
	if(returVal == RC_RM_RECORD_ID_NOT_FOUND){
		scanMgmt->curSlot = 0;
		(scanMgmt->curPage)++;
		scan->mgmtData = scanMgmt;
		return next(scan, record);
	}
	//if page is not found,it means no more tuples to scan
	if(returVal==RC_RM_PAGE_NOT_FOUND){
		return RC_RM_NO_MORE_TUPLES;
	}

	evalExpr(record, scan->rel->schema, scanMgmt->condition,&value);
	(scanMgmt->curSlot)++;

	scan->mgmtData = scanMgmt;


	if(value->v.boolV != 1){
		return next(scan, record);
	}

	return RC_OK;

}
/*
 * Frees memory allocated for RM_ScanHandle
 *
 */
RC closeScan (RM_ScanHandle *scan){
	free(scan);
	return RC_OK;
}

/*
 * Calculates the record size using the data types of each attribute in schema
 *
 */
int getRecordSize (Schema *schema){
	int numAttr=schema->numAttr;
	int i;
	int sizeData=0;
	for(i=0;i<numAttr;i++){
		switch (schema->dataTypes[i])
		{
		case DT_INT:
			sizeData=sizeData+sizeof(int);
			break;
		case DT_FLOAT:
			sizeData=sizeData+sizeof(float);
			break;
		case DT_STRING:
			sizeData=sizeData+schema->typeLength[i];
			break;
		case DT_BOOL:
			sizeData=sizeData+sizeof(bool);
			break;
		}
	}
	return sizeData;
}
/*
 * Creates the schema structure and populates it with given value
 *
 */
Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){
	Schema *schema=(Schema *)malloc(sizeof(Schema));
	schema->attrNames=(char**) malloc( sizeof(char*)*numAttr);
	schema->dataTypes=(DataType*) malloc( sizeof(DataType)*numAttr);
	schema->typeLength=(int*) malloc( sizeof(int)*numAttr);
	schema->keyAttrs=(int*) malloc( sizeof(int)*keySize);
	int i;

	for(i = 0; i < numAttr; i++){

		schema->numAttr=numAttr;
		schema->attrNames[i]= attrNames[i];

		schema->dataTypes[i] = dataTypes[i];

		schema->typeLength[i]= typeLength[i];
		schema->keySize=keySize;


	}
	for(i = 0; i < schema->keySize; i++)
	{
		schema->keyAttrs[i] = keys[i];

	}

	return schema;
}
/*
 * Frees the memory allocated for the schema structure.
 *
 */

RC freeSchema (Schema *schema){
	free(schema->attrNames);
	free(schema->dataTypes);
	free(schema->keyAttrs);
	free(schema->typeLength);
	free(schema);
	return RC_OK;
}

/*
 * Creates record and allocates the memory depending on the attributes in the schema
 *
 */
RC createRecord (Record **record, Schema *schema){

	int sizeData=getRecordSize(schema);
	RID *rid=(RID *)malloc(sizeof(RID));
	*record=(Record *)malloc(sizeof(Record));
	(*record)->data=(char *)malloc(sizeof(char)*sizeData);
	(*record)->id=*rid;
	(*record)->id.page=-1;
	(*record)->id.slot=-1;
	return RC_OK;
}

/*
 * Frees the memory allocated for the record
 *
 */
RC freeRecord (Record *record){
	free(record->data);
	free(record);
	return RC_OK;
}

/*
 * Gets the data from record and populates it into the value structure.
 *
 */
RC getAttr (Record *record, Schema *schema, int attrNum, Value **value){
	int i;
	int offset=0;
	char *attrData;
	*value=(Value **)malloc(sizeof(Value*));
	(*value)->dt=(DataType)malloc(sizeof(DataType));
	for(i=0;i<schema->numAttr;i++){
		if(i==attrNum){
			break;
		}
		switch(schema->dataTypes[i]){
		case DT_INT:
			offset=offset+sizeof(int);
			break;
		case DT_STRING:
			offset=offset+schema->typeLength[i];
			break;
		case DT_FLOAT:
			offset=offset+sizeof(float);
			break;
		case DT_BOOL:
			offset=offset+sizeof(bool);
			break;

		}

	}
	attrData =record->data+offset;
	switch(schema->dataTypes[attrNum])
	{
	case DT_INT:
	{
		(*value)->v.intV=(int)malloc(sizeof(int));
		int val = 0;
		memcpy(&val,attrData, sizeof(int));
		(*value)->dt=DT_INT;
		(*value)->v.intV=val;

	}
	break;
	case DT_STRING:
	{
		(*value)->v.stringV=(char *)malloc(sizeof(char)*schema->typeLength[attrNum]);
		char *buf;
		int len = schema->typeLength[attrNum];
		buf = (char *) malloc(sizeof(char)*len);
		memcpy(buf, attrData, len);
		(*value)->dt=DT_STRING;
		strcpy((*value)->v.stringV,buf);

		free(buf);

	}

	break;
	case DT_FLOAT:
	{
		(*value)->v.floatV=*(float *)malloc(sizeof(float));
		float val;
		memcpy(&val,attrData, sizeof(float));
		(*value)->dt=DT_FLOAT;
		(*value)->v.floatV=val;
	}

	break;
	case DT_BOOL:
	{
		(*value)->v.boolV=(bool)malloc(sizeof(bool));
		bool val;
		memcpy(&val,attrData, sizeof(bool));
		(*value)->dt=DT_BOOL;
		(*value)->v.boolV=val;
	}
	break;
	}
	return RC_OK;
}

/*
 * Gets the values in value structure and add it to the data of record.
 *
 */
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value){

	int i;
	int offset=0;

	char *attrData;
	attrData=record->data;

	for(i=0;i<schema->numAttr;i++){
		if(i==attrNum){
			break;
		}
		switch(schema->dataTypes[i]){
		case DT_INT:
			offset=offset+sizeof(int);
			break;
		case DT_STRING:
			offset=offset+schema->typeLength[i];
			break;
		case DT_FLOAT:
			offset=offset+sizeof(float);
			break;
		case DT_BOOL:
			offset=offset+sizeof(bool);
			break;

		}

	}

	switch(schema->dataTypes[attrNum])
	{
	case DT_INT:
		memcpy(attrData+offset,&value->v.intV,sizeof(int));
		break;
	case DT_STRING:
		memcpy(attrData+offset,value->v.stringV,schema->typeLength[i]);
		break;
	case DT_FLOAT:
		memcpy(attrData+offset,&value->v.floatV, sizeof(float));
		break;
	case DT_BOOL:
		memcpy(attrData+offset,&(value)->v.boolV, sizeof(bool));
		break;

	}

	record->data=attrData;
	return RC_OK;
}
