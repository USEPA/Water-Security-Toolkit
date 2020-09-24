#include "erd_utils.h"
#include "jniUtils.h"

static jclass x_findClass(JNIEnv *env, char *className, char *dottedClassName);
static jfieldID x_findField(JNIEnv *env,jclass cls, char *fieldName, char *sig, char *className);
static jmethodID x_findMethod(JNIEnv *env,jclass cls, char *methodName, char *sig, char *className);
static jobject x_CreateLinkInfoObject(JNIEnv *env,DBDataPtr dbData, PLinkInfo pLinkInfo, PNetInfo pNetInfo, int linkidx, int nsteps);
static jobject x_CreateNodeInfoObject(JNIEnv *env,DBDataPtr dbData, PNodeInfo pNodeInfo, PNetInfo pNetInfo, int nodeidx, int nspecies, int nsteps);

JNIEXPORT jboolean ERD_initJavaData(JNIEnv *env, DBDataPtr *dbData, char *readerClassName)
{
	char *args;
	char className[256],dottedClassName[256];
	DBDataPtr db = *dbData = (DBDataPtr)calloc(1,sizeof(DBData));

	// Reader
	sprintf(className,"anl/msv/dpf/erd/db/%s",readerClassName);
	sprintf(dottedClassName,"anl.msv.dpf.erd.db.%s",readerClassName);
	if((db->readerClass = x_findClass(env,className,dottedClassName))==NULL)
		return FALSE;
	if((db->readerSources = x_findField(env,db->readerClass,"sources","[Lanl/msv/dpf/erd/db/Source;",dottedClassName))==NULL)
		return FALSE;
	if((db->readerCreateSpeciesMethod = x_findMethod(env,db->readerClass,"createSpecieObject","(ILjava/lang/String;Ljava/lang/String;)Lanl/epanet/domain/Specie;",dottedClassName))==0)
		return FALSE;
	if((db->readerSetCompressionLevel = x_findMethod(env,db->readerClass,"setCompressionLevel","(I)V",dottedClassName))==0)
		return FALSE;

	// Specie
	if((db->speciesClass = x_findClass(env,"anl/epanet/domain/Specie","anl.epanet.domain.Specie"))==NULL)
		return FALSE;

	// DataIndex
	if((db->dataIndexClass = x_findClass(env,"anl/msv/dpf/erd/db/DataIndex","anl.msv.dpf.erd.db.DataIndex"))==NULL)
		return FALSE;
	args = "(IIJI[Lanl/msv/dpf/erd/db/Source;)V";
	db->dataIndexConstructor = (*env)->GetMethodID(env,db->dataIndexClass,"<init>",args);
	if(ANL_UTIL_CheckForException(env, "unable to find constructor for DataIndex\n"))
		return FALSE;

	// NetInfo
	if((db->netInfoClass = x_findClass(env,"anl/msv/dpf/erd/db/NetInfo","anl.msv.dpf.erd.db.NetInfo"))==NULL)
		return FALSE;
	args = "(IIIIIIII[Lanl/epanet/domain/Specie;FIIIIF)V";
	db->netInfoConstructor = (*env)->GetMethodID(env,db->netInfoClass,"<init>",args);
	if(ANL_UTIL_CheckForException(env, "unable to find constructor for NetInfo\n"))
		return FALSE;

	// NodeInfo
	if((db->nodeInfoClass = x_findClass(env,"anl/msv/dpf/erd/db/NodeInfo","anl.msv.dpf.erd.db.NodeInfo"))==NULL)
		return FALSE;
	args = "(IFF[F[F[[F[ILjava/lang/String;)V";
	db->nodeInfoConstructor = (*env)->GetMethodID(env,db->nodeInfoClass,"<init>",args);
	if(ANL_UTIL_CheckForException(env, "unable to find constructor for NodeInfo\n"))
		return FALSE;
	if((db->nodeInfoQ = x_findField(env,db->nodeInfoClass,"q","[F","anl.msv.dpf.erd.db.NodeInfo"))==NULL)
		return FALSE;
	if((db->nodeInfoC = x_findField(env,db->nodeInfoClass,"c","[[F","anl.msv.dpf.erd.db.NodeInfo"))==NULL)
		return FALSE;
	if((db->nodeInfoNZ = x_findField(env,db->nodeInfoClass,"nz","[I","anl.msv.dpf.erd.db.NodeInfo"))==NULL)
		return FALSE;

	// LinkInfo
	if((db->linkInfoClass = x_findClass(env,"anl/msv/dpf/erd/db/LinkInfo","anl.msv.dpf.erd.db.LinkInfo"))==NULL)
		return FALSE;
	args = "(IIFF[F[FLjava/lang/String;I[F[F[[F[I)V";
	db->linkInfoConstructor = (*env)->GetMethodID(env,db->linkInfoClass,"<init>",args);
	if(ANL_UTIL_CheckForException(env, "unable to find constructor for LinkInfo\n"))
		return FALSE;
	if((db->linkInfoC = x_findField(env,db->linkInfoClass,"c","[[F","anl.msv.dpf.erd.db.LinkInfo"))==NULL)
		return FALSE;
	if((db->linkInfoNZ = x_findField(env,db->linkInfoClass,"nz","[I","anl.msv.dpf.erd.db.LinkInfo"))==NULL)
		return FALSE;
	if((db->linkInfoVelocity = x_findField(env,db->linkInfoClass,"velocity","[F","anl.msv.dpf.erd.db.LinkInfo"))==NULL)
		return FALSE;
	if((db->linkInfoFlow = x_findField(env,db->linkInfoClass,"flow","[F","anl.msv.dpf.erd.db.LinkInfo"))==NULL)
		return FALSE;
	if((db->linkInfoVX = x_findField(env,db->linkInfoClass,"vx","[F","anl.msv.dpf.erd.db.LinkInfo"))==NULL)
		return FALSE;
	if((db->linkInfoVY = x_findField(env,db->linkInfoClass,"vy","[F","anl.msv.dpf.erd.db.LinkInfo"))==NULL)
		return FALSE;


	// Source
	if((db->sourceClass = x_findClass(env,"anl/msv/dpf/erd/db/Source","anl.msv.dpf.erd.db.Source"))==NULL)
		return FALSE;
	args = "(Ljava/lang/String;IIIIIF)V";
	db->sourceConstructor = (*env)->GetMethodID(env,db->sourceClass,"<init>",args);
	if(ANL_UTIL_CheckForException(env, "unable to find constructor for Source\n"))
		return FALSE;

	if((db->stringClass = x_findClass(env,"java/lang/String","java.lang.String"))==NULL)
		return FALSE;
	if((db->floatArrayClass = x_findClass(env,"[F","float[]"))==NULL)
		return FALSE;

	return TRUE;

}

JNIEXPORT jobject ERD_CreateNetInfoObject(JNIEnv *env, jobject reader, DBDataPtr dbData, PNetInfo pNetInfo) 
{
	jobject rv;
	jobjectArray species=NULL;
	int i;

	species = (*env)->NewObjectArray(env,pNetInfo->numSpecies,dbData->speciesClass,0);

	for(i=0;i<pNetInfo->numSpecies;i++) {
		PSpeciesData sp = pNetInfo->species[i];
		jobject jID = (*env)->NewStringUTF(env,sp->id);
		jobject jType=(*env)->NewStringUTF(env,sp->type==bulk?"BULK":"WALL");
		jobject jspecies = (*env)->CallObjectMethod(env,reader,dbData->readerCreateSpeciesMethod,sp->index,jID,jType);
		(*env)->SetObjectArrayElement(env,species,i,jspecies);
	}
	rv = (*env)->NewObject(env,dbData->netInfoClass,dbData->netInfoConstructor,
		(jint)pNetInfo,pNetInfo->numNodes, pNetInfo->numLinks, pNetInfo->numTanks,
		pNetInfo->numJunctions,
		pNetInfo->numSteps, pNetInfo->qualCode, pNetInfo->numSpecies,
		species, pNetInfo->stepSize, pNetInfo->simDuration,
			       pNetInfo->reportStart, pNetInfo->reportStep, pNetInfo->simStart, pNetInfo->fltMax);
	return rv;
}

JNIEXPORT jobject ERD_createSourcesArray(JNIEnv *env, DBDataPtr dbData, int nsources, PSource pSource)
{
	jobjectArray sources;
	int s;
	sources = (*env)->NewObjectArray(env,nsources,dbData->sourceClass,0);
	for(s=0;s<nsources;s++) {
		PSource srcp = &pSource[s];
		jstring nodeID = (*env)->NewStringUTF(env,srcp->sourceNodeID);
		jobject source = (*env)->NewObject(env,dbData->sourceClass,dbData->sourceConstructor,
			nodeID, srcp->sourceNodeIndex, srcp->speciesIndex, srcp->sourceType,
			srcp->sourceStart, srcp->sourceStop, srcp->sourceStrength);
		(*env)->SetObjectArrayElement(env,sources,s,source);
	}
	return sources;
}

JNIEXPORT jobjectArray ERD_CreateNodeInfoArray(JNIEnv *env,DBDataPtr dbData, PNetInfo pNetInfo,PNodeInfo pNodeInfo)
{
	jobjectArray rv;
	int i,nspecies,nsteps,nnodes;
	nspecies = pNetInfo->numSpecies;
	nnodes = pNetInfo->numNodes;
	nsteps = pNetInfo->numSteps;
	rv = (*env)->NewObjectArray(env,pNetInfo->numNodes,dbData->nodeInfoClass,NULL);
	for(i=0;i<nnodes;i++) {
		jobject nodeInfo=NULL;
		nodeInfo = x_CreateNodeInfoObject(env,dbData,&pNodeInfo[i],pNetInfo, i, nspecies,nsteps);
		(*env)->SetObjectArrayElement(env,rv,i,nodeInfo);
	}
	return rv;
}

JNIEXPORT jobjectArray ERD_CreateLinkInfoArray(JNIEnv *env, DBDataPtr dbData, PNetInfo pNetInfo,PLinkInfo pLinkInfo)
{
	jobjectArray rv;
	int i,nsteps,nlinks;

	nlinks = pNetInfo->numLinks;
	nsteps = pNetInfo->numSteps;
	rv = (*env)->NewObjectArray(env,pNetInfo->numLinks,dbData->linkInfoClass,NULL);
	for(i=0;i<nlinks;i++) {
		jobject linkInfo = x_CreateLinkInfoObject(env,dbData, &pLinkInfo[i], pNetInfo, i,nsteps);
		(*env)->SetObjectArrayElement(env,rv,i,linkInfo);
	}
	return rv;
}
JNIEXPORT void ERD_populateNodeInfoAndSources(JNIEnv *env, DBDataPtr dbData, jobject reader,
										 PNetInfo pNetInfo, PNodeInfo pNodeInfo,
										 PSourceData pSource, jobjectArray jNodeInfo, jobjectArray jLinkInfo)
{
	int s,n,nsteps;
	jobjectArray sources;
	
	sources = ERD_createSourcesArray(env, dbData, pSource->nsources, pSource->source);
	(*env)->SetObjectField(env,reader,dbData->readerSources,sources);

	if(pNetInfo->qualResults->nodeC==NULL)
		return;
	nsteps = pNetInfo->numSteps;
	for(n=0;n<pNetInfo->numNodes;n++) {
		float **concData;  // node, time
		jobject nodeInfo;
		jobjectArray cc;

		nodeInfo = (*env)->GetObjectArrayElement(env,jNodeInfo,n);
		cc = (*env)->GetObjectField(env,nodeInfo,dbData->nodeInfoC);
		for(s=0;s<pNetInfo->numSpecies;s++) {
			jfloatArray c;
			float *fElems;
			PSpeciesData sp=pNetInfo->species[s];
			if(sp->index != -1) {
				if(sp->type==bulk) {
					concData=pNetInfo->qualResults->nodeC[s];
					c = (*env)->GetObjectArrayElement(env,cc,s);
					fElems = (*env)->GetFloatArrayElements(env,c,NULL);
					memcpy(fElems,concData[n],sizeof(float)*nsteps);
					(*env)->ReleaseFloatArrayElements(env,c,fElems,0);
				}
			}
		}
	}
	for(n=0;n<pNetInfo->numLinks;n++) {
		float **concData;  // node, time
		jobject linkInfo;
		jobjectArray cc;

		linkInfo = (*env)->GetObjectArrayElement(env,jLinkInfo,n);
		cc = (*env)->GetObjectField(env,linkInfo,dbData->linkInfoC);
		for(s=0;s<pNetInfo->numSpecies;s++) {
			jfloatArray c;
			float *fElems;
			PSpeciesData sp=pNetInfo->species[s];
			if(sp->index != -1) {
				if(sp->type==wall) {
					concData=pNetInfo->qualResults->linkC[s];
					c = (*env)->GetObjectArrayElement(env,cc,s);
					fElems = (*env)->GetFloatArrayElements(env,c,NULL);
					memcpy(fElems,concData[n],sizeof(float)*nsteps);
					(*env)->ReleaseFloatArrayElements(env,c,fElems,0);
				}
			}
		}
	}
}


static jobject x_CreateNodeInfoObject(JNIEnv *env,DBDataPtr dbData, PNodeInfo pNodeInfo, PNetInfo pNetInfo, int nodeidx, int nspecies, int nsteps)
{
	jfloatArray q=NULL;
	jfloatArray dp=NULL;
	jintArray nz=NULL;
	jobjectArray c=NULL;
	jstring id=NULL;
	int i;
	float *qelems=NULL;
	float *dpelems=NULL;

	// this will be null if the READ_DEMANDS flag wasn't set.
	if(pNetInfo->hydResults->demand != NULL) {
		float **dem=pNetInfo->hydResults->demand;
		q = (*env)->NewFloatArray(env,nsteps);
		qelems = (*env)->GetFloatArrayElements(env,q,NULL);
		for(i=0;i<nsteps;i++) {
			qelems[i]=dem[i][nodeidx];
		}
		(*env)->ReleaseFloatArrayElements(env,q,qelems,0);
	}
	dp = (*env)->NewFloatArray(env,pNodeInfo->demandProfileLength);
	dpelems = (*env)->GetFloatArrayElements(env,dp,NULL);
	memcpy(dpelems,pNodeInfo->demandProfile,sizeof(float)*pNodeInfo->demandProfileLength);
	(*env)->ReleaseFloatArrayElements(env,dp,dpelems,0);

	// this will be null if the READ_QUALITY flag wasn't set.
	if(pNetInfo->qualResults->nodeC != NULL) {
		c = (*env)->NewObjectArray(env,nspecies,dbData->floatArrayClass,NULL);
		for(i=0;i<nspecies;i++) {
			PSpeciesData sp = pNetInfo->species[i];
			if(sp->index != -1 && sp->type==bulk) {
				jfloatArray cd = (*env)->NewFloatArray(env,nsteps);
				(*env)->SetObjectArrayElement(env,c,i,cd);
			}
		}
	}
	nz = (*env)->NewIntArray(env,nspecies);
	id = (*env)->NewStringUTF(env,pNodeInfo->id);
	return (*env)->NewObject(env,dbData->nodeInfoClass,dbData->nodeInfoConstructor,
		pNodeInfo->type,pNodeInfo->x,pNodeInfo->y,dp,q,c,nz,id);
}

static jobject x_CreateLinkInfoObject(JNIEnv *env,DBDataPtr dbData, PLinkInfo pLinkInfo, PNetInfo pNetInfo, int linkidx, int nsteps)
{
	jfloatArray v=NULL,
			f=NULL,vx,vy;
	float *fElems=NULL;
	jstring id;
	int i;
	jfloatArray c=NULL;
	jintArray nz=NULL;

	// this will be null if the READ_LINKVEL flag wasn't set.
	if(pNetInfo->hydResults->velocity != NULL) {
		float **vel=pNetInfo->hydResults->velocity;
		v = (*env)->NewFloatArray(env,nsteps);
		fElems = (*env)->GetFloatArrayElements(env,v,NULL);
		for(i=0;i<nsteps;i++) {
			fElems[i]=vel[i][linkidx];
		}
		(*env)->ReleaseFloatArrayElements(env,v,fElems,0);
	}

	// this will be null if the READ_LINKFLOW flag wasn't set.
	if(pNetInfo->hydResults->flow != NULL) {
		float **flow=pNetInfo->hydResults->flow;
		f = (*env)->NewFloatArray(env,nsteps);
		fElems = (*env)->GetFloatArrayElements(env,f,NULL);
		for(i=0;i<nsteps;i++) {
			fElems[i]=flow[i][linkidx];
		}
		(*env)->ReleaseFloatArrayElements(env,f,fElems,0);
	}
	vx = (*env)->NewFloatArray(env,pLinkInfo->nv);
	fElems = (*env)->GetFloatArrayElements(env,vx,NULL);
	memcpy(fElems,pLinkInfo->vx,sizeof(float)*pLinkInfo->nv);
	(*env)->ReleaseFloatArrayElements(env,vx,fElems,0);

	vy = (*env)->NewFloatArray(env,pLinkInfo->nv);
	fElems = (*env)->GetFloatArrayElements(env,vy,NULL);
	memcpy(fElems,pLinkInfo->vy,sizeof(float)*pLinkInfo->nv);
	(*env)->ReleaseFloatArrayElements(env,vy,fElems,0);

	if(pNetInfo->qualResults->linkC != NULL) {
		c = (*env)->NewObjectArray(env,pNetInfo->numSpecies,dbData->floatArrayClass,NULL);
		for(i=0;i<pNetInfo->numSpecies;i++) {
			PSpeciesData sp = pNetInfo->species[i];
			if(sp->index != -1 && sp->type==wall) {
				jfloatArray cd = (*env)->NewFloatArray(env,nsteps);
				(*env)->SetObjectArrayElement(env,c,i,cd);
			}
		}
	}
	nz = (*env)->NewIntArray(env,pNetInfo->numSpecies);
	id = (*env)->NewStringUTF(env,pLinkInfo->id);
	
	return (*env)->NewObject(env,dbData->linkInfoClass,dbData->linkInfoConstructor,
		pLinkInfo->from, pLinkInfo->to, pLinkInfo->length, pLinkInfo->diameter, v, f, id, pLinkInfo->nv, vx, vy, c, nz);
}

static jclass x_findClass(JNIEnv *env, char *className, char *dottedClassName)
{
	jclass tclass;
	tclass = (*env)->FindClass(env,className);
	if(ANL_UTIL_CheckForException(env, "unable to find class %s\n",dottedClassName))
		return NULL;
	return (*env)->NewGlobalRef(env,tclass);
}
static jfieldID x_findField(JNIEnv *env,jclass cls, char *fieldName, char *sig, char *className)
{
	jfieldID fldID = (*env)->GetFieldID(env,cls,fieldName,sig);
	if(ANL_UTIL_CheckForException(env, "unable to find field %s with signature %s in class %s\n",fieldName,sig,className))
		return 0;
	return fldID;
}
static jmethodID x_findMethod(JNIEnv *env,jclass cls, char *methodName, char *sig, char *className)
{
	jmethodID methodID = (*env)->GetMethodID(env,cls,methodName,sig);
	if(ANL_UTIL_CheckForException(env, "unable to find method %s with signature %s in class %s\n",methodName,sig,className))
		return 0;
	return methodID;
}

