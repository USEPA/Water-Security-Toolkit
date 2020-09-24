#ifdef __linux__
#include <errno.h>
#endif
#include <stdlib.h>
#include <memory.h>

#include "anl_msv_dpf_erd_db_ERDReader.h"
#include "jniUtils.h"
#include "loggingUtils.h"
#include "erd.h"
#include "erd_utils.h"

// local data definitions
static DBDataPtr x_erdData=NULL;

static void  x_ensureDataInitialized(JNIEnv *env);
/*
 * Class:     anl_msv_dpf_erd_db_ERDReader
 * Method:    open
 * Signature: (Ljava/lang/String;I)J
 */
JNIEXPORT jlong JNICALL Java_anl_msv_dpf_erd_db_ERDReader_open
  (JNIEnv *env , jobject reader, jstring jFilename, jint flags)
{
	PERD p_erd;
	char msg[1024];
	char *erdFile;
	PSource pSources;
	jobject netInfo;
	jobjectArray nodeInfoArray;
	jobjectArray linkInfoArray;

	x_ensureDataInitialized(env);

	erdFile=ANL_UTIL_javaStringToC(env,jFilename);
	sprintf(msg,"Opening ERD file: %s",erdFile);
	ANL_UTIL_LogInfo(env,"teva",msg);
	ERD_open(&p_erd,erdFile,flags);
	ANL_UTIL_free(erdFile);

	(*env)->CallVoidMethod(env,reader,x_erdData->readerSetCompressionLevel,ERD_getCompressionLevel(p_erd));
	pSources = (PSource)calloc(MAXSOURCES,sizeof(Source));
	ANL_UTIL_setIntegerInstVar(env,reader,"fileVersion",ERD_getVersion(p_erd));
	ANL_UTIL_setLongInstVar(env,reader,"pNodeInfo",PTRTOJLONG(p_erd->nodes));
	ANL_UTIL_setLongInstVar(env,reader,"pLinkInfo",PTRTOJLONG(p_erd->links));
	ANL_UTIL_setLongInstVar(env,reader,"pSource",PTRTOJLONG(pSources));

	netInfo = ERD_CreateNetInfoObject(env,reader,x_erdData,p_erd->network);
	ANL_UTIL_setObjectInstVar(env,reader,"netInfo",netInfo,"Lanl/msv/dpf/erd/db/NetInfo;");
	nodeInfoArray = ERD_CreateNodeInfoArray(env,x_erdData,p_erd->network,p_erd->nodes);
	ANL_UTIL_setObjectInstVar(env,reader,"nodeInfo",nodeInfoArray,"[Lanl/msv/dpf/erd/db/NodeInfo;");
	linkInfoArray = ERD_CreateLinkInfoArray(env,x_erdData,p_erd->network,p_erd->links);
	ANL_UTIL_setObjectInstVar(env,reader,"linkInfo",linkInfoArray,"[Lanl/msv/dpf/erd/db/LinkInfo;");

	return PTRTOJLONG(p_erd);
}


/*
 * Class:     anl_msv_dpf_erd_db_ERDReader
 * Method:    getIndex
 * Signature: (JJ)[Lanl/msv/dpf/erd/db/ERDDataIndex;
 */
JNIEXPORT jobjectArray JNICALL Java_anl_msv_dpf_erd_db_ERDReader_getIndex
  (JNIEnv *env, jobject reader, jlong p_erd, jlong pNodeInfo)
{

	jobjectArray index;
	int i;
	PERD erd = PTRCAST(PERD)p_erd;
	x_ensureDataInitialized(env);

	index = (*env)->NewObjectArray(env,erd->qualSimCount,x_erdData->dataIndexClass,0);

	for(i=0;i<erd->qualSimCount;i++) {
		PQualitySim qsi=erd->qualSim[i];
		PTEVAIndexData tid=(PTEVAIndexData)qsi->appData;
		jobjectArray sources;
		jobject dataIndex;

		sources = ERD_createSourcesArray(env, x_erdData, tid->nsources, tid->source);
		dataIndex = (*env)->NewObject(env,x_erdData->dataIndexClass,x_erdData->dataIndexConstructor,
			i,qsi->fileIdx,qsi->offset,qsi->length,sources);
		(*env)->SetObjectArrayElement(env,index,i,dataIndex);
	}
	return index;
}

/*
 * Class:     anl_msv_dpf_erd_db_ERDReader
 * Method:    closeAndFreeMemory
 * Signature: (JJJJJ)V
 */
JNIEXPORT void JNICALL Java_anl_msv_dpf_erd_db_ERDReader_closeAndFreeMemory
  (JNIEnv * env, jobject reader, jlong p_erd, jlong pNetInfo, jlong pNodeInfo, jlong pLinkInfo, jlong pSource)
{

	PERD erd = PTRCAST(PERD)p_erd;

	ERD_close(&erd);
	erd=NULL;
	ANL_UTIL_setLongInstVar(env,reader,"p_erd",PTRTOJLONG(erd));

}

/*
 * Class:     anl_msv_dpf_erd_db_ERDReader
 * Method:    getSimulationResults
 * Signature: (IJJJJ[Lanl/msv/dpf/erd/db/NodeInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_anl_msv_dpf_erd_db_ERDReader_getSimulationResults
  (JNIEnv *env, jobject reader, jint id, jlong p_erd, jlong pNetInfo, jlong pNodeInfo, jlong pSource, jobjectArray jNodeInfoArray, jobjectArray jLinkInfoArray)
{

	PERD erd = PTRCAST(PERD)p_erd;
	jboolean hasData;
	int err;
	
	x_ensureDataInitialized(env);
	err = ERD_getResults(erd->qualSim[id],erd);
	if(err < 0) {
	  char msg[512];
	  sprintf(msg,"Error retrieving sim results for id=%d. ERD_ERROR=%d",id,err);
	  ANL_UTIL_LogSevere(env,"teva",msg);
	  sprintf(msg," err=%s",strerror(err));
	  ANL_UTIL_LogSevere(env,"teva",msg);
	} else {
	  hasData=err==0;
	  if(hasData) {
		  ERD_populateNodeInfoAndSources(env,x_erdData,reader,PTRCAST(PNetInfo)pNetInfo,PTRCAST(PNodeInfo)pNodeInfo,(PTEVAIndexData)erd->qualSim[id]->appData, jNodeInfoArray, jLinkInfoArray);
	  }
	}
	return hasData;
}

/*
 * Class:     anl_msv_dpf_erd_db_ERDReader
 * Method:    getRawSimulationResults
 * Signature: (IJJJJ[Lanl/msv/dpf/erd/db/NodeInfo;[B)V
 */
JNIEXPORT jboolean JNICALL Java_anl_msv_dpf_erd_db_ERDReader_getRawSimulationResults
  (JNIEnv *env, jobject reader, jint id, jlong p_erd, jlong pNetInfo, jlong pSource, jobjectArray jNodeInfoArray, jbyteArray jbuf)
{

	PERD erd = PTRCAST(PERD)p_erd;
	jboolean hasData;
	jbyte* rawData;
	jboolean isCopy;
	int bLen;
	int err;
	char msg[512];

	bLen = (*env)->GetArrayLength(env,jbuf);
	rawData = (*env)->GetPrimitiveArrayCritical(env,jbuf,&isCopy);
	//PQualitySim qualSim, PERD database, PNodeInfo nodes, PSource sources, char *buffer, int length)

	err = ERD_getRawSimulationResults(erd->qualSim[id],erd,PTRCAST(PSource)pSource,rawData,bLen);
	(*env)->ReleasePrimitiveArrayCritical(env,jbuf,rawData,0);
	if(err < 0) {
	  sprintf(msg,"Error retrieving raw sim results for id=%d. ERD_ERROR=%d",id,err);
	  ANL_UTIL_LogSevere(env,"teva",msg);
	  if(err) {
	    sprintf(msg," err=%s",strerror(err));
	    ANL_UTIL_LogSevere(env,"teva",msg);
	  }
	} else {
	  hasData=err==0;
	  if(hasData) {
		  PSourceData pSource=((PTEVAIndexData)erd->qualSim[id]->appData);
		  jobjectArray sources = ERD_createSourcesArray(env, x_erdData, pSource->nsources, pSource->source);
		  (*env)->SetObjectField(env,reader,x_erdData->readerSources,sources);
	  }
	}
	return hasData;
}

//  private methods...
static void  x_ensureDataInitialized(JNIEnv *env)
{
	if(x_erdData == NULL)
		ERD_initJavaData(env,&x_erdData,"ERDReader");
}


