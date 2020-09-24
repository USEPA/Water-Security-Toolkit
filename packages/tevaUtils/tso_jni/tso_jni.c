/*
 * Copyright � 2008 UChicago Argonne, LLC
 * NOTICE: This computer software, TEVA-SPOT, was prepared for UChicago Argonne, LLC
 * as the operator of Argonne National Laboratory under Contract No. DE-AC02-06CH11357
 * with the Department of Energy (DOE). All rights in the computer software are reserved
 * by DOE on behalf of the United States Government and the Contractor as provided in
 * the Contract.
 * NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
 *
 * This software is distributed under the BSD License.
 */
#ifdef __linux__
#include <errno.h>
#endif
#include <stdlib.h>
#include <memory.h>

#include "anl_teva_db_TSOReader.h"
#include "jniUtils.h"
#include "loggingUtils.h"
#include "tevautil.h"
#include "erd_utils.h"

// local data definitions
static DBDataPtr x_tsoData=NULL;

// private method definitions
static jboolean x_initTSOData(JNIEnv *env);
static void x_ensureDataInitialized(JNIEnv *env);
static void x_populateNodeInfoAndSources(JNIEnv *env, jobject reader, PNetInfo pNetInfo, PNodeInfo pNodeInfo, PSourceData pSource, jobjectArray jNodeInfo);


/*
 * Class:     anl_msv_dpf_erd_db_TSOReader
 * Method:    open
 * Signature: (Ljava/lang/String;Ljava/lang/String;CC)I
 */
JNIEXPORT jlong JNICALL Java_anl_msv_dpf_erd_db_TSOReader_open
  (JNIEnv *env, jobject reader, jstring jDirOrFilename, jstring jPatternOrNull, jchar fileVersion, jchar storageMethod)
{
	char *dirOrFilename;
	char *patternOrNull=NULL;
	PTSO ptso;
	char msg[1024];

	dirOrFilename=ANL_UTIL_javaStringToC(env,jDirOrFilename);
	if(jPatternOrNull != NULL)
		patternOrNull = ANL_UTIL_javaStringToC(env,jPatternOrNull);
	sprintf(msg,"Opening TSO file: %s  pattern: %s",dirOrFilename,patternOrNull);
	ANL_UTIL_LogInfo(env,"teva",msg);
	ptso = TSO_Open(dirOrFilename,patternOrNull);
	ANL_UTIL_free(dirOrFilename);
	ANL_UTIL_free(patternOrNull);
	return PTRTOJLONG(ptso);
}

/*
 * Class:     anl_msv_dpf_erd_db_TSOReader
 * Method:    positionAfterPrologue
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_anl_msv_dpf_erd_db_TSOReader_positionAfterPrologue
  (JNIEnv *env, jobject reader, jlong p_tso)
{
	TSO_PositionAfterPrologue(PTRCAST(PTSO)p_tso);
}

/*
 * Class:     anl_msv_dpf_erd_db_TSOReader
 * Method:    readPrologue
 * Signature: (I[Ljava/lang/Object;[I)V
 */
JNIEXPORT void JNICALL Java_anl_msv_dpf_erd_db_TSOReader_readPrologue
  (JNIEnv *env, jobject reader, jlong ptso, jint flags)
{
	PNetInfo pNetInfo=NULL;
	PNodeInfo pNodeInfo;
	PLinkInfo pLinkInfo;
	PSource pSources;
	jobject netInfo;
	jobjectArray nodeInfoArray;
	jobjectArray linkInfoArray;

	x_ensureDataInitialized(env);
	TSO_ReadPrologue(PTRCAST(PTSO)ptso,&pNetInfo,&pNodeInfo,&pLinkInfo,flags);
	(*env)->CallVoidMethod(env,reader,x_tsoData->readerSetCompressionLevel,TSO_getCompressionLevel(PTRCAST(PTSO)ptso));
	pSources = (PSource)calloc(MAXSOURCES,sizeof(Source));
	ANL_UTIL_setIntegerInstVar(env,reader,"fileVersion",(PTRCAST(PTSO)ptso)->fileVersion);
	ANL_UTIL_setLongInstVar(env,reader,"pNodeInfo",PTRTOJLONG(pNodeInfo));
	ANL_UTIL_setLongInstVar(env,reader,"pLinkInfo",PTRTOJLONG(pLinkInfo));
	ANL_UTIL_setLongInstVar(env,reader,"pSource",PTRTOJLONG(pSources));

	netInfo = ERD_CreateNetInfoObject(env,reader,x_tsoData,pNetInfo);
	ANL_UTIL_setObjectInstVar(env,reader,"netInfo",netInfo,"Lanl/msv/dpf/erd/db/NetInfo;");
	nodeInfoArray = ERD_CreateNodeInfoArray(env,x_tsoData,pNetInfo,pNodeInfo);
	ANL_UTIL_setObjectInstVar(env,reader,"nodeInfo",nodeInfoArray,"[Lanl/msv/dpf/erd/db/NodeInfo;");
	linkInfoArray = ERD_CreateLinkInfoArray(env,x_tsoData,pNetInfo,pLinkInfo);
	ANL_UTIL_setObjectInstVar(env,reader,"linkInfo",linkInfoArray,"[Lanl/msv/dpf/erd/db/LinkInfo;");

}

/*
 * Class:     anl_msv_dpf_erd_db_TSOReader
 * Method:    closeAndFreeMemory
 * Signature: (IIIII)V
 */
JNIEXPORT void JNICALL Java_anl_msv_dpf_erd_db_TSOReader_closeAndFreeMemory
  (JNIEnv *env, jobject reader, jlong ptso, jlong pNetInfo, jlong pNodeInfo, jlong pLinkInfo, jlong pSource)
{
	PTSO tso = PTRCAST(PTSO)ptso;
	PNodeInfo nodeInfo = PTRCAST(PNodeInfo)pNodeInfo;
	PLinkInfo linkInfo = PTRCAST(PLinkInfo)pLinkInfo;
	PNetInfo netInfo = PTRCAST(PNetInfo)pNetInfo;

	TSO_Close(&tso);
	TSO_ReleaseNetworkData(&nodeInfo,&linkInfo,PTRCAST(PNetInfo)pNetInfo);
	TSO_ReleaseNetworkInfo(&netInfo);

}


/*
 * Class:     anl_msv_dpf_erd_db_TSOReader
 * Method:    readSimulationResults
 * Signature: (IIII[[F)V
 */
JNIEXPORT jboolean JNICALL Java_anl_msv_dpf_erd_db_TSOReader_readSimulationResults
  (JNIEnv *env, jobject reader, jlong ptso, jlong pNetInfo, jlong pNodeInfo, jlong pSource, jobjectArray jNodeInfo, jobjectArray jLinkInfo)
{
  jboolean hasData=0;
  int err;
  err = TSO_ReadSimulationResults(PTRCAST(PTSO)ptso,PTRCAST(PNetInfo)pNetInfo,
		  	  	  	  	  	  	  PTRCAST(PNodeInfo)pNodeInfo);
  if(err < 0) {
    char msg[512];
    sprintf(msg,"Error retrieving sim results in readSimulationResults. TSO_ERROR=%d",err);
    ANL_UTIL_LogSevere(env,"teva",msg);
    sprintf(msg," err=%s",strerror(err));
    ANL_UTIL_LogSevere(env,"teva",msg);
    return 0;
  } else {
    hasData=err==1;
    if(hasData) {
      PTSO tso=PTRCAST(PTSO)ptso;
      ERD_populateNodeInfoAndSources(env,x_tsoData,reader,PTRCAST(PNetInfo)pNetInfo,PTRCAST(PNodeInfo)pNodeInfo,tso->index[tso->curIndex]->sourceData, jNodeInfo, jLinkInfo);
    }
    return hasData;
  }
}

/*
 * Class:     anl_msv_dpf_erd_db_TSOReader
 * Method:    getSimulationResults
 * Signature: (IIII[[F)V
 */
JNIEXPORT jboolean JNICALL Java_anl_msv_dpf_erd_db_TSOReader_getSimulationResults
  (JNIEnv *env, jobject reader, jint id, jlong ptso, jlong pNetInfo, jlong pNodeInfo, jlong pSource, jobjectArray jNodeInfo, jobjectArray jLinkInfo)
{
	PTSO tso = PTRCAST(PTSO)ptso;
	jboolean hasData;
	int err = TSO_GetSimulationResults(id,tso,PTRCAST(PNetInfo)pNetInfo,PTRCAST(PNodeInfo)pNodeInfo);
	if(err < 0) {
	  char msg[512];
	  sprintf(msg,"Error retrieving sim results for id=%d. TSO_ERROR=%d",id,err);
	  ANL_UTIL_LogSevere(env,"teva",msg);
	  sprintf(msg," err=%s",strerror(err));
	  ANL_UTIL_LogSevere(env,"teva",msg);
	} else {
	  hasData=err==1;
	  if(hasData) {
	    ERD_populateNodeInfoAndSources(env,x_tsoData,reader,PTRCAST(PNetInfo)pNetInfo,PTRCAST(PNodeInfo)pNodeInfo,tso->index[tso->curIndex]->sourceData, jNodeInfo, jLinkInfo);
	  }
	}
	return hasData;
}

/*
 * Class:     anl_msv_dpf_erd_db_TSOReader
 * Method:    getRawSimulationResults
 * Signature: (IIII[[F)V
 */
JNIEXPORT jboolean JNICALL Java_anl_msv_dpf_erd_db_TSOReader_getRawSimulationResults
  (JNIEnv *env, jobject reader, jint id, jlong ptso, jlong pNetInfo, jlong pNodeInfo, jlong junkSource, jobjectArray jNodeInfo, jbyteArray jbuf)
{
	PTSO tso = PTRCAST(PTSO)ptso;
	jboolean hasData;
	jbyte* rawData;
	jboolean isCopy;
	int bLen;
	int err;
	char msg[512];

	bLen = (*env)->GetArrayLength(env,jbuf);
	rawData = (*env)->GetPrimitiveArrayCritical(env,jbuf,&isCopy);
	err = TSO_GetRawSimulationResults(id,tso,PTRCAST(PNetInfo)pNetInfo,PTRCAST(PNodeInfo)pNodeInfo,rawData,bLen);
	(*env)->ReleasePrimitiveArrayCritical(env,jbuf,rawData,0);
	if(err < 0) {
	  sprintf(msg,"Error retrieving raw sim results for id=%d. TSO_ERROR=%d",id,err);
	  ANL_UTIL_LogSevere(env,"teva",msg);
	  if(err) {
	    sprintf(msg," err=%s",strerror(err));
	    ANL_UTIL_LogSevere(env,"teva",msg);
	  }
	} else {
	  hasData=err==1;
	  if(hasData) {
		PSourceData pSource=tso->index[id]->sourceData;
	    jobjectArray sources = ERD_createSourcesArray(env, x_tsoData, pSource->nsources, pSource->source);
	    (*env)->SetObjectField(env,reader,x_tsoData->readerSources,sources);
	  }
	}
	return hasData;
}

/*
 * Class:     anl_msv_dpf_erd_db_TSOReader
 * Method:    getIndex
 * Signature: (JJ)[Lanl/msv/dpf/erd/db/DataIndex
 */
JNIEXPORT jobjectArray JNICALL Java_anl_msv_dpf_erd_db_TSOReader_getIndex(
         JNIEnv *env, jobject reader, jlong ptso, jlong pNodeInfo)
{
	jobjectArray index;
	int i;
	PTSO tso = PTRCAST(PTSO)ptso;
	x_ensureDataInitialized(env);

	index = (*env)->NewObjectArray(env,tso->count,x_tsoData->dataIndexClass,0);

	for(i=0;i<tso->count;i++) {
		PDataIndex di = tso->index[i];
		jobjectArray sources;
		jobject dataIndex;

		sources = ERD_createSourcesArray(env, x_tsoData, di->sourceData->nsources, di->sourceData->source);
		dataIndex = (*env)->NewObject(env,x_tsoData->dataIndexClass,x_tsoData->dataIndexConstructor,
			di->id,di->fileIndex,di->fileOffset,di->dataLength,sources);
		(*env)->SetObjectArrayElement(env,index,i,dataIndex);
	}
	return index;
}

//  private methods...
static void  x_ensureDataInitialized(JNIEnv *env)
{
	if(x_tsoData == NULL)
		ERD_initJavaData(env,&x_tsoData,"TSOReader");
}


