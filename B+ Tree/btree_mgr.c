#include <stdio.h>
#include <string.h>
#include <math.h>
#include "btree_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "struct.h"

typedef enum NodeType {
	LEAF = 0,
	NON_LEAF = 1,
	ROOT = 2
} NodeType;

typedef struct BT_MgmtInfo
{
	int numKeys;
	int totalNumOfPages;
	int rootNode;
	BM_BufferPool *bm;
	BM_PageHandle *ph;

} BT_MgmtInfo;

typedef struct BT_ScanMgmtData {
	int pn;
	char *curNode;
	int curPos;
} BT_ScanMgmtData;

static char *getNodeKeyString (char *);
// init and shutdown index manager
RC initIndexManager (void *mgmtData){
	if(mgmtData==NULL){
		initStorageManager();
	}
	return RC_OK;
}
RC shutdownIndexManager (){
	return RC_OK;
}

// create, destroy, open, and close an btree index
RC createBtree (char *idxId, DataType keyType, int n){

	char *p = (char *)malloc(sizeof(char)*10);

	sprintf(p,"%d",keyType);
	strcpy(p+strlen(p),",");
	sprintf(p+strlen(p),"%d",n);

	SM_PageHandle ph = (SM_PageHandle) malloc(PAGE_SIZE);
	strcpy(ph,p);
	//creating the file
	int isCreated = createPageFile(idxId);

	if(isCreated == RC_OK){
		SM_FileHandle fHandle;
		int isOpen = openPageFile(idxId,&fHandle);

		if(isOpen == RC_OK){
			//writing the schema into the file
			writeBlock (0, &fHandle,ph);

			closePageFile (&fHandle);

		}
	}

free(p);
free(ph);
	return RC_OK;
}
RC openBtree (BTreeHandle **tree, char *idxId){
	BT_MgmtInfo *btMgmtInfo=(BT_MgmtInfo *)malloc(sizeof(BT_MgmtInfo));
	btMgmtInfo->bm= (BM_BufferPool *)malloc (sizeof(BM_BufferPool));
	btMgmtInfo->ph= (BM_PageHandle *)malloc (sizeof(BM_PageHandle));
	btMgmtInfo->rootNode=(int)malloc(sizeof(int));
	//intilialises the buffer pool
	initBufferPool(btMgmtInfo->bm, idxId, 3, RS_FIFO, NULL);
	//reads the first page and populates the *rel
	pinPage(btMgmtInfo->bm, btMgmtInfo->ph, 0);
	unpinPage (btMgmtInfo->bm, btMgmtInfo->ph);
	char *ph=btMgmtInfo->ph->data;
	char *p;
	*tree=(BTreeHandle *)malloc(sizeof(BTreeHandle));
	(*tree)->idxId=(char *)malloc(sizeof(char));
	(*tree)->idxId=idxId;
	BM_BookKeeping *bk;
	bk=btMgmtInfo->bm->mgmtData;
	SM_FileHandle *fH1;
	fH1=(bk)->fh;
	btMgmtInfo->totalNumOfPages=fH1->totalNumPages;
	btMgmtInfo->rootNode=-1;


	int array[2];
	int j=0;
	p = strtok(ph,",");
	while( p != NULL )
	{
		array[j]=atoi(p);
		j++;
		p= strtok(NULL,",");
	}
	(*tree)->keyType=array[0];
	btMgmtInfo->numKeys=array[1];
	(*tree)->mgmtData=btMgmtInfo;
	return RC_OK;
}
RC closeBtree (BTreeHandle *tree){

	BT_MgmtInfo *btMgmtInfo;
	btMgmtInfo=tree->mgmtData;
	int ret=shutdownBufferPool(btMgmtInfo->bm);

	BM_BookKeeping *bk;
	bk=btMgmtInfo->bm->mgmtData;
	closePageFile(bk->fh);
	free(tree->mgmtData);
	free(tree);


	return RC_OK;
}
RC deleteBtree (char *idxId){
	destroyPageFile (idxId);
	return RC_OK;

}

// access information about a b-tree
RC getNumNodes (BTreeHandle *tree, int *result){
	BT_MgmtInfo *btMgmtInfo;
	btMgmtInfo=tree->mgmtData;
	int temp=btMgmtInfo->totalNumOfPages-1;
	*result=temp;


	return RC_OK;

}
RC getNumEntries (BTreeHandle *tree, int *result){
	int i;
	int keyCount=0;
	int count=0;
	BT_MgmtInfo *btMgmtInfo;
	btMgmtInfo=tree->mgmtData;
	for(i=1;i<btMgmtInfo->totalNumOfPages;i++){
		pinPage(btMgmtInfo->bm, btMgmtInfo->ph, i);
		char *ph=btMgmtInfo->ph->data;
		char *ph1=strdup(ph);
		int nodeType=getNodeType(ph1);
		if(nodeType==LEAF){
			count=getNumKeys(ph);
			keyCount=keyCount+count;
		}
		unpinPage (btMgmtInfo->bm, btMgmtInfo->ph);
	}
	*result=keyCount;
	return RC_OK;
}
RC getKeyType (BTreeHandle *tree, DataType *result){
	*result=(DataType)tree->keyType;
	return RC_OK;
}

// index access
RC findKey (BTreeHandle *tree, Value *key, RID *result){
	BT_MgmtInfo *btMgmtInfo;
	btMgmtInfo=tree->mgmtData;
	int page;

		if(btMgmtInfo->rootNode!=-1){
			page=btMgmtInfo->rootNode;
		}
		else{
			page=btMgmtInfo->totalNumOfPages-1;
		}
	bool keyFound=false;
	while(!keyFound){
		pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
		unpinPage(btMgmtInfo->bm ,btMgmtInfo->ph);
		if(strlen(btMgmtInfo->ph->data)==0){
			return RC_IM_KEY_NOT_FOUND;
		}

		char *ph1=btMgmtInfo->ph->data;
		char *ph3=strdup(ph1);
		char *ph2=strdup(ph1);
		int nodeType=getNodeType(ph3);
		char *keyString;
		if(nodeType!=LEAF){

			keyString=getNodeKeyString(ph2);

			int size=getNumKeys(keyString);
			size=size*2;

			char **array=(char **)malloc(sizeof(char*) * (size+1));
			getNodeKeyStringArray(array,keyString);

			int pos=0;
			pos=size;
			int i,value=0;
			for(i=0;i<size;i++){
				if((i%2)==1){
					value=atoi(array[i]);
					if(key->v.intV<value){

						pos=i-1;
						break;
					}
					else{
						pos=i+1;
					}
				}
			}
			page=atoi(array[pos]);



			free(array);
		}
		if(nodeType==LEAF){
			keyFound=true;
		}

	}


	pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
	unpinPage(btMgmtInfo->bm ,btMgmtInfo->ph);
	char *ph2=btMgmtInfo->ph->data;
	char *ph3=strdup(ph2);

	char *keyString;
	keyString=getNodeKeyString(ph3);

	int size=getNumKeys(keyString);
	size=size*2;

	char **array=(char **)malloc(sizeof(char*) * (size+1));
	getNodeKeyStringArray(array,keyString);

	int pos=-1;

	int i,value=0;
	for(i=0;i<size;i++){
		if((i%2)==1){
			value=atoi(array[i]);
			if(key->v.intV==value){

				pos=i-1;
				break;
			}

		}
	}
	if(pos==-1){
			return RC_IM_KEY_NOT_FOUND;

		}
	char *res=array[pos];

	char *t;
	t=strtok(res,".");

	result->page=atoi(t);
	t=strtok(NULL,"");

	result->slot=atoi(t);
	return RC_OK;
}
RC insertKey (BTreeHandle *tree, Value *key, RID rid){


	BT_MgmtInfo *btMgmtInfo;

	btMgmtInfo=(tree)->mgmtData;

	int pageNum=btMgmtInfo->totalNumOfPages;

	int rootNode=btMgmtInfo->rootNode;

	if(pageNum==1){
		pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, 1);
		unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
		char *str;
		Value *v=(Value *)malloc(sizeof(Value));
		v->dt=DT_INT;
		v->v.intV=LEAF;
		str=serializeValue(v);
		strcat(str,"(");
		v->v.intV=2;
		strcat(str,serializeValue(v));
		strcat(str,")[");
		v->v.intV=rid.page;
		strcat(str,serializeValue(v));
		strcat(str,".");
		v->v.intV=rid.slot;
		strcat(str,serializeValue(v));
		strcat(str,",");
		strcat(str,serializeValue(key));
		strcat(str,",");
		v->v.intV=3;
		strcat(str,serializeValue(v));
		strcat(str,"]");
		pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, 1);
		markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
		strcpy(btMgmtInfo->ph->data,str);

		unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
		btMgmtInfo->totalNumOfPages++;

	}
	else{



		if(rootNode!=-1){
			int leafTemp;


			leafTemp=findLeafNode(btMgmtInfo,key->v.intV,rootNode, rid);


		}else{
			int i;

			pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, 1);
			unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
			char *ph=btMgmtInfo->ph->data;
			char *t=getNodeKeyString(ph);
			int len1=strlen(t);
			char *k=(char *)malloc(sizeof(char)*len1);
			strcpy(k,t);
			int charcount=getNumKeys(k);
			int size=charcount*2;
			char **array=(char **)malloc(sizeof(char*) * (size+1));
			int val=key->v.intV;
			int j=0;
			getNodeKeyStringArray(array,k);
			int pos=0;
			pos=size;
			int value=0;


			for(i=0;i<size;i++){
				if((i%2)==1){
					value=atoi(array[i]);
					if(val<value){

						pos=i;
						break;
					}
					else{
						pos=size+1;
					}
				}
			}



			char *str;
			char *result=(char *)malloc(sizeof(char)*200);
			Value *v=(Value *)malloc(sizeof(Value));
			v->dt=DT_INT;
			v->v.intV=rid.page;
			strcpy(str,serializeValue(v));
			strcat(str,".");
			v->v.intV=rid.slot;
			strcat(str,serializeValue(v));
			strcat(str,",");
			strcat(str,serializeValue(key));

			for(i=0;i<(pos-1);i++){
				strcat(result,array[i]);
				strcat(result,",");
			}
			strcat(result,str);

			for(i=pos-1;i<size;i++){
				strcat(result,",");
				strcat(result,array[i]);
			}

			int node=getNodeType(ph);
			int parent=getParent(ph);
			int next=atoi(array[size]);
			char *subArray2=(char *)malloc(sizeof(char)*10);
			char *subArray3=(char *)malloc(sizeof(char)*10);
			subArray2[0]='\0';
			subArray3[0]='\0';

			int cmp=ceil((btMgmtInfo->numKeys+1)/2);
			char **subArray1=(char **)malloc(sizeof(char*) * (3*2));
			int a=0;
			char *s;
			s = strtok(result,",");
			while( s != NULL )
			{

				subArray1[a]=(char *)malloc(sizeof(char)*strlen(s));

				strcpy(subArray1[a],s);

				if((a/2)<=cmp){

					strcat(subArray2,subArray1[a]);
					strcat(subArray2,",");

				}
				else{
					strcat(subArray3,subArray1[a]);
					strcat(subArray3,",");

				}

				a++;
				s= strtok(NULL,",");
			}


			char *str1,*str2,*str3;
			str2=(char*)malloc(sizeof(char)*20);
			v->dt=DT_INT;
			v->v.intV=node;
			str1=serializeValue(v);
			strcat(str1,"(");
			v->v.intV=parent;
			strcat(str1,serializeValue(v));
			strcat(str1,")[");
			strcpy(str2,str1);
			strcat(str1,subArray2);
			v->v.intV=next;
			strcat(str1,serializeValue(v));
			strcat(str1,"]");
			pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, 1);
			markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
			strcpy(btMgmtInfo->ph->data,str1);

			unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
			if(strlen(subArray3)!=0){
				char *strParentKey,*temp;
				char *dp;
				dp=strdup(subArray3);
				temp=strpbrk(subArray3,",");

				strParentKey=strtok(temp+1,",");

				strcat(str2,dp);
				v->v.intV=next+1;
				strcat(str2,serializeValue(v));
				strcat(str2,"]");
				pinPage(btMgmtInfo->bm, btMgmtInfo->ph,next);
				markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
				strcpy(btMgmtInfo->ph->data,str2);

				unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
				btMgmtInfo->totalNumOfPages++;
				btMgmtInfo->rootNode=parent;
				v->dt=DT_INT;
				v->v.intV=ROOT;
				str3=serializeValue(v);
				strcat(str3,"(");
				strcat(str3,")[");
				v->v.intV=1;
				strcat(str3,serializeValue(v));
				strcat(str3,",");
				strcat(str3,strParentKey);
				strcat(str3,",");
				v->v.intV=next;
				strcat(str3,serializeValue(v));
				strcat(str3,"]");
				//btMgmtInfo->ph->data[0]='\0';
				pinPage(btMgmtInfo->bm, btMgmtInfo->ph,2);
				markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
				strcpy(btMgmtInfo->ph->data,str3);

				unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
				btMgmtInfo->totalNumOfPages++;
				free(subArray2);
				free(subArray3);
			}
		}

	}
	(tree)->mgmtData=btMgmtInfo;
	return RC_OK;
}
RC deleteKey (BTreeHandle *tree, Value *key){
	BT_MgmtInfo *btMgmtInfo;
	btMgmtInfo=tree->mgmtData;
	int page;
	if(btMgmtInfo->rootNode!=-1){
		page=btMgmtInfo->rootNode;
	}
	else{
		page=btMgmtInfo->totalNumOfPages-1;
	}
	bool keyFound=false;
	while(!keyFound){
		pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
		unpinPage(btMgmtInfo->bm ,btMgmtInfo->ph);
		char *ph1=btMgmtInfo->ph->data;
		char *ph3=strdup(ph1);
		char *ph2=strdup(ph1);
		int nodeType=getNodeType(ph3);
		char *keyString;
		if(nodeType!=LEAF){

			keyString=getNodeKeyString(ph2);

			int size=getNumKeys(keyString);
			size=size*2;

			char **array=(char **)malloc(sizeof(char*) * (size+1));
			getNodeKeyStringArray(array,keyString);

			int pos=0;
			pos=size;
			int i,value=0;
			for(i=0;i<size;i++){
				if((i%2)==1){
					value=atoi(array[i]);
					if(key->v.intV<value){

						pos=i-1;
						break;
					}
					else{
						pos=i+1;
					}
				}
			}
			page=atoi(array[pos]);



			free(array);
		}
		if(nodeType==LEAF){
			keyFound=true;
		}

	}


	pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
	unpinPage(btMgmtInfo->bm ,btMgmtInfo->ph);
	char *ph2=btMgmtInfo->ph->data;
	char *ph3=strdup(ph2);
	char *ph4=strdup(ph2);

	char *keyString;
	keyString=getNodeKeyString(ph3);

	int size=getNumKeys(keyString);
	size=size*2;

	char **array=(char **)malloc(sizeof(char*) * (size+1));
	getNodeKeyStringArray(array,keyString);
	int node=getNodeType(ph4);
	int parent=getParent(ph4);
	int next=atoi(array[size]);
	int pos=-1;

	int i,value=0;
	for(i=0;i<size;i++){
		if((i%2)==1){
			value=atoi(array[i]);
			if(key->v.intV==value){

				pos=i-1;
				break;
			}

		}
	}
	if(pos==-1){
		return RC_IM_KEY_NOT_FOUND;

	}
	char *result=(char *)malloc(sizeof(char)*200);
	for(i=0;i<(pos);i++){
		strcat(result,array[i]);
		strcat(result,",");
	}


	for(i=pos+2;i<size;i++){

		strcat(result,array[i]);
		strcat(result,",");
	}
	if(strlen(result)!=0){
		char *str1;
		Value *v=(Value *)malloc(sizeof(Value));
		v->dt=DT_INT;
		v->v.intV=node;
		str1=serializeValue(v);
		strcat(str1,"(");
		v->v.intV=parent;
		strcat(str1,serializeValue(v));
		strcat(str1,")[");

		strcat(str1,result);
		v->v.intV=next;

		strcat(str1,serializeValue(v));
		strcat(str1,"]");
		pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
		markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
		strcpy(btMgmtInfo->ph->data,str1);

		unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
	}
	else{
		pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
				markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
				memset(btMgmtInfo->ph->data,'\0',PAGE_SIZE);

				unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
		if(btMgmtInfo->rootNode!=-1){

		pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, parent);
		char *ph2=btMgmtInfo->ph->data;
		char *ph3=strdup(ph2);
		char *ph4=strdup(ph2);

		char *keyString;
		keyString=getNodeKeyString(ph3);

		int size=getNumKeys(keyString);
		size=size*2;

		char **array=(char **)malloc(sizeof(char*) * (size+1));
		getNodeKeyStringArray(array,keyString);
		int node=getNodeType(ph4);
		for(i=2;i<size;i++){
			strcat(result,array[i]);
			strcat(result,",");
		}
		if(strlen(result)!=0){
			strcat(result,array[size]);

			char *str1;
			Value *v=(Value *)malloc(sizeof(Value));
			v->dt=DT_INT;
			v->v.intV=node;
			str1=serializeValue(v);
			strcat(str1,"(");

			strcat(str1,")[");

			strcat(str1,result);

			strcat(str1,"]");

			markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
			strcpy(btMgmtInfo->ph->data,str1);

			unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
		}else{
			pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, parent);
			markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
			memset(btMgmtInfo->ph->data,'\0',PAGE_SIZE);
			unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
			btMgmtInfo->rootNode=-1;

		}
		}
	}
	tree->mgmtData=btMgmtInfo;

	return RC_OK;
}
RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle){
	   RC rc;
	    BT_ScanMgmtData *btsmd;
	    (*handle) = (BT_ScanHandle*) malloc(sizeof(BT_ScanHandle));
	    memset(*handle, 0, sizeof(BT_ScanHandle));

	    btsmd= (BT_ScanMgmtData*) malloc(sizeof(BT_ScanMgmtData));
	    btsmd->curNode = (char *)malloc(sizeof(char)*100);
	    memset(btsmd, 0, sizeof(BT_ScanMgmtData));

	    (*handle)->tree= tree;
	    (*handle)->mgmtData= btsmd;
	    btsmd->pn= -1;
	    btsmd->curPos= -1;

	    return(RC_OK);
}
RC nextEntry (BT_ScanHandle *handle, RID *result){
	 RC rc;

	    BT_ScanMgmtData *scanMgmt= handle->mgmtData;
	    BT_MgmtInfo *btMgmtInfo= handle->tree->mgmtData;
	    int pageNum,tmp;

	    if (!btMgmtInfo->totalNumOfPages)
	        return RC_IM_NO_MORE_ENTRIES;

	    // Go down and get 1st left most leaf.
	    if (scanMgmt->pn == -1)
	    {
	        pageNum= btMgmtInfo->rootNode;
	        tmp = pageNum;
	        pageNum = getLeftChildNode(btMgmtInfo,pageNum);
	        while (pageNum != -1)
	        {
	        	tmp = pageNum;
	           unpinPage(btMgmtInfo->bm ,btMgmtInfo->ph);
	           pageNum = getLeftChildNode(btMgmtInfo,pageNum);
	        }

	        char *ph1=btMgmtInfo->ph->data;
	        scanMgmt->pn= tmp;
	        scanMgmt->curPos= 0;

	        scanMgmt->curNode=strdup(ph1);
	    }

	    char *ph2=strdup(btMgmtInfo->ph->data);
	    char *ph3=strdup(ph2);
	    char *keyString;
	    keyString =getNodeKeyString(ph3);
	    int size=getNumKeys(keyString);
	    char **array=(char **)malloc(sizeof(char*) * ((size*2)+1));
	    getNodeKeyStringArray(array,keyString);


	    if (scanMgmt->curPos >= (size*2))
	    {
	    	pageNum = atoi(array[size*2]);
	        if (pageNum == -1 || pageNum == btMgmtInfo->totalNumOfPages )
	        {
	        	unpinPage(btMgmtInfo->bm ,btMgmtInfo->ph);
	            scanMgmt->pn= -1;
	            scanMgmt->curPos= -1;
	            scanMgmt->curNode= "";

	            return RC_IM_NO_MORE_ENTRIES;
	        }


	        unpinPage(btMgmtInfo->bm ,btMgmtInfo->ph);

	        scanMgmt->pn= pageNum;
	        scanMgmt->curPos= 0;
	        pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, pageNum);
	        char * p ;
	        p = strdup(btMgmtInfo->ph->data);
	        scanMgmt->curNode= strdup(p);
	    }
	    /**/

	    // Read the RID
	      char *str;
	      str = scanMgmt->curNode;
	    	keyString=getNodeKeyString(str);

	    	size=getNumKeys(keyString);
	        size=size*2;
	    	char **arraya=(char **)malloc(sizeof(char*) * (size+1));
	    	getNodeKeyStringArray(arraya,keyString);
	    	int pos=scanMgmt->curPos;
	    	char *value;
	    	value=arraya[pos];

	    	getRID(value,result);

	    scanMgmt->curPos = scanMgmt->curPos +2;
	    handle->mgmtData = scanMgmt;
	    return RC_OK;
}
int getLeftChildNode(BT_MgmtInfo *btMgmtInfo,int page){
	int leftChild = -1;
	pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
char *ph1=btMgmtInfo->ph->data;
		    char *ph3=strdup(ph1);
		    char *ph2=strdup(ph3);
		    int nodeType=getNodeType(ph3);
		    char *keyString;
		    if(nodeType!=LEAF){
		    	keyString=getNodeKeyString(ph2);
		    	int size=getNumKeys(keyString);
		    		size=size*2;
		    	char **array=(char **)malloc(sizeof(char*) * (size+1));
		    	getNodeKeyStringArray(array,keyString);
		    	leftChild = atoi(array[0]);
		    }
	return leftChild;
}
RC closeTreeScan (BT_ScanHandle *handle){
	 RC rc;
	    BT_ScanMgmtData *btsmd= handle->mgmtData;
	    // Close the scan and unpin active page in buffer
	    if (btsmd->pn != -1)
	    {
	        //unpinBTNode(handle->tree, btsmd->pn);

	        BT_MgmtInfo *btmd= (BT_MgmtInfo*) handle->tree->mgmtData;
	           BM_PageHandle *ph = btmd->ph;

	           ph->pageNum = btsmd->pn;
	           unpinPage(&btmd->bm, &ph);

	        btsmd->pn= -1;
	        free(btsmd->curNode);
	    }
	    free(handle->mgmtData);
	    free(handle);

	    return(RC_OK);
}
RC getRID(char *value,RID *rid){
	int j=0;
	char **array=(char **)malloc(sizeof(char*) * (2));
	char *p;
	p = strtok(value,".");
		while( p != NULL )
		{
			array[j]=(char *)malloc(sizeof(char)*strlen(p));
			strcpy(array[j],p);

			j++;
			p= strtok(NULL,".");
		}

		rid->page = atoi(array[0]);
		rid->slot = atoi(array[1]);
		return RC_OK;
}

// debug and test functions
char *printTree (BTreeHandle *tree){
	char *result;
	result = malloc(sizeof(char)*600);
	BT_MgmtInfo *btMgmtInfo =(BT_MgmtInfo *) tree->mgmtData;
    int totalNumPages = btMgmtInfo->totalNumOfPages;
    int i;
    for(i=1;i<totalNumPages;i++){
    	pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, i);

    	char *ph1=btMgmtInfo->ph->data;
    	char *ph3=strdup(ph1);
    	char *keyString;
    	char *s;
    	sprintf(s,"%d",i);

    	keyString=getNodeKeyString(ph3);


    	strcat(result,"(");
    	strcat(result,s);
    	strcat(result,")[");
    	strcat(result,keyString);
    	strcat(result,"]");
    	strcat(result,"\n");


    	unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);

    }
    return result;
}

char *getNodeKeyString (char *ph){
	char *q,*t;
	q = strpbrk(ph,"[");
	t=strtok(q+1,"]");

	return t;
}

int getParent (char *ph){
	char *q,*t;
	q = strpbrk(ph,"(");
	t=strtok(q+1,")");

	return atoi(t);
}
int getNodeType(char *p){
	char *q;
	q=strtok(p,",");
	return atoi(q);
}
int getNumKeys(char *k){
	int charcount,i;
	charcount = 0;

	for(i=0; k[i]; i++) {
		if(k[i] == ',') {
			charcount ++;
		}
	}
	return charcount/2;
}


void getNodeKeyStringArray (char **array,char *k){

	int j=0;
		char *p;


		int h=strlen(k);
			char arr[h];
			strcpy(arr,k);

		p = strtok(arr,",");
		while( p != NULL )
		{

			array[j]=(char *)malloc(sizeof(char)*strlen(p));

			strcpy(array[j],p);

			j++;
			p= strtok(NULL,",");
		}

}

int findLeafNode(BT_MgmtInfo *btMgmtInfo,int key,int page,RID rid){
	bool keyFound=false;
	while(!keyFound){
		pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
		unpinPage(btMgmtInfo->bm ,btMgmtInfo->ph);

		char *ph1=btMgmtInfo->ph->data;
		char *ph3=strdup(ph1);
		char *ph2=strdup(ph1);
		int nodeType=getNodeType(ph3);
		char *keyString;
		if(nodeType!=LEAF){

			keyString=getNodeKeyString(ph2);

			int size=getNumKeys(keyString);
			size=size*2;

			char **array=(char **)malloc(sizeof(char*) * (size+1));
			getNodeKeyStringArray(array,keyString);

			int pos=0;
			pos=size;
			int i,value=0;
			for(i=0;i<size;i++){
				if((i%2)==1){
					value=atoi(array[i]);
					if(key<value){

						pos=i-1;
						break;
					}
					else{
						pos=i+1;
					}
				}
			}
			page=atoi(array[pos]);



			//free(array);
		}else if(nodeType==LEAF){
			keyFound=true;
		}

	}

	pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
	unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
	char *ph=btMgmtInfo->ph->data;
	char *t;
	t=getNodeKeyString(ph);
	int len1=strlen(t);
	char *k=(char *)malloc(sizeof(char)*len1);
	strcpy(k,t);
	int charcount=getNumKeys(k);
	int size=charcount*2;
	char **array=(char **)malloc(sizeof(char*) * (size+1));

	int j=0;
	getNodeKeyStringArray(array,k);
	int pos=0;
	pos=size;
	int value=0;

	int i;
	for(i=0;i<size;i++){
		if((i%2)==1){
			value=atoi(array[i]);
			if(key<value){

				pos=i;
				break;
			}
			else{
				pos=size+1;
			}
		}
	}

	char *str=(char *)malloc(sizeof(char)*200);
	char *result=(char *)malloc(sizeof(char)*200);
	Value *v=(Value *)malloc(sizeof(Value));
	v->dt=DT_INT;
	v->v.intV=rid.page;
	strcpy(str,serializeValue(v));
	strcat(str,".");
	v->v.intV=rid.slot;
	strcat(str,serializeValue(v));
	strcat(str,",");
	v->v.intV=key;
	strcat(str,serializeValue(v));

	for(i=0;i<(pos-1);i++){
		strcat(result,array[i]);
		strcat(result,",");
	}
	strcat(result,str);

	for(i=pos-1;i<size;i++){
		strcat(result,",");
		strcat(result,array[i]);
	}

	int node=getNodeType(ph);
	int parent=getParent(ph);
	int next=atoi(array[size]);
	char *subArray2=(char *)malloc(sizeof(char)*10);
	char *subArray3=(char *)malloc(sizeof(char)*10);
	subArray2[0]='\0';
	subArray3[0]='\0';
	int cmp=ceil((btMgmtInfo->numKeys+1)/2);
	char **subArray1=(char **)malloc(sizeof(char*) * (3*2));
	int a=0;
	char *s;
	s = strtok(result,",");
	while( s != NULL )
	{

		subArray1[a]=(char *)malloc(sizeof(char)*strlen(s));

		strcpy(subArray1[a],s);

		if((a/2)<=cmp){

			strcat(subArray2,subArray1[a]);
			strcat(subArray2,",");

		}
		else{
			strcat(subArray3,subArray1[a]);
			strcat(subArray3,",");

		}

		a++;
		s= strtok(NULL,",");
	}

	char *str1,*str2,*str3;
	str2=(char*)malloc(sizeof(char)*20);
	v->dt=DT_INT;
	v->v.intV=node;
	str1=serializeValue(v);
	strcat(str1,"(");
	v->v.intV=parent;
	strcat(str1,serializeValue(v));
	strcat(str1,")[");
	strcpy(str2,str1);
	strcat(str1,subArray2);
	v->v.intV=next;


	strcat(str1,serializeValue(v));
	strcat(str1,"]");

	pinPage(btMgmtInfo->bm ,btMgmtInfo->ph, page);
	markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
	strcpy(btMgmtInfo->ph->data,str1);

	unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
	if(strlen(subArray3)!=0){
		char *strParentKey,*temp;
		char *dp;
		dp=strdup(subArray3);
		temp=strpbrk(subArray3,",");

		strParentKey=strtok(temp+1,",");

		strcat(str2,dp);
		v->v.intV=next+1;
		strcat(str2,serializeValue(v));
		strcat(str2,"]");
		//btMgmtInfo->ph->data[0]='\0';
		pinPage(btMgmtInfo->bm, btMgmtInfo->ph,next);
		markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
		strcpy(btMgmtInfo->ph->data,str2);

		unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);
		btMgmtInfo->totalNumOfPages++;
		pinPage(btMgmtInfo->bm, btMgmtInfo->ph,parent);

		if(btMgmtInfo->ph->data !=NULL){
			char *ph=btMgmtInfo->ph->data;
			char *ph3=strdup(ph);
			char *ph2=strdup(ph);
			int node=getNodeType(ph2);
			int parent=getParent(ph3);
			//int next=v->v.intV;
			char *t;
			t=getNodeKeyString(ph);

			int len1=strlen(t);
			char *k=(char *)malloc(sizeof(char)*len1);
			strcpy(k,t);
			int charcount=getNumKeys(k);
			int size=charcount*2;
			int pos=0;
			if(charcount<2){
				//pos=size;
				strcat(t,",");
				strcat(t,strParentKey);
			}

			v->dt=DT_INT;
			v->v.intV=node;
			str3=serializeValue(v);
			strcat(str3,"(");
			//strcat(str3,parent);
			strcat(str3,")[");
			strcat(str3,t);
			strcat(str3,",");
			v->v.intV=next;
			strcat(str3,serializeValue(v));
			strcat(str3,"]");
			markDirty (btMgmtInfo->bm, btMgmtInfo->ph);
			strcpy(btMgmtInfo->ph->data,str3);

			unpinPage(btMgmtInfo->bm, btMgmtInfo->ph);

		}
	}
	return page;
}
