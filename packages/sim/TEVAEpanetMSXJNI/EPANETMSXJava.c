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
#include <jni.h>
#include "EPANETMSXModel.h"
#include "jniUtils.h"
#include "epanet2.h"
#include "epanetmsx.h"
#include <stdlib.h>
#include <string.h>

/* MSX object functions */
static EPANETMSXModelDataPtr modelData=NULL;

static void x_ensureMSXModelDataInitialized(JNIEnv *env);
static jboolean x_initMSXModelData(JNIEnv *env);

static void x_ensureMSXModelDataInitialized(JNIEnv *env)
{
	if(modelData == NULL)
		x_initMSXModelData(env);
}

JNIEXPORT EPANETMSXModelDataPtr EPANETMSXModel_getData(JNIEnv *env)
{
	x_ensureMSXModelDataInitialized(env);
	return modelData;
}

static jboolean x_initMSXModelData(JNIEnv *env)
{
	jclass tclass;
	if(modelData != NULL)
		return TRUE;
	modelData = (EPANETMSXModelDataPtr)calloc(1,sizeof(EPANETMSXModelData));

	tclass = (*env)->FindClass(env,"anl/epanet/model/EPANETMSXException");
	if(ANL_UTIL_CheckForException(env, "unable to find class anl.epanet.model.EPANETMSXException\n"))
		return FALSE;
	modelData->epanetmsxExceptionClass = (*env)->NewGlobalRef(env,tclass);

	/* EPANETMSXModelInterface class */
	tclass = (*env)->FindClass(env,"anl/epanet/model/EPANETMSXModelInterface");
	if(ANL_UTIL_CheckForException(env, "unable to find class anl.epanet.model.EPANETMSXModelInterface\n"))
		return FALSE;
	modelData->epanetmsxModelClass = (*env)->NewGlobalRef(env,tclass);

	modelData->errCodeField = (*env)->GetFieldID(env,modelData->epanetmsxModelClass,"errCode","I");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"errCode\" in class anl.epanet.model.EPANETMSXModelInterface\n"))
		return FALSE;
	modelData->errStringField = (*env)->GetFieldID(env,modelData->epanetmsxModelClass,"errString","Ljava/lang/String;");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"errString\" in class anl.epanet.model.EPANETMSXModelInterface\n"))
		return FALSE;
	modelData->doubleBufferField = (*env)->GetFieldID(env,modelData->epanetmsxModelClass,"extDoubleBuffer","J");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"extDoubleBuffer\" in class anl.epanet.model.EPANETMSXModelInterface\n"))
		return FALSE;
	modelData->doubleBufferLengthField = (*env)->GetFieldID(env,modelData->epanetmsxModelClass,"extDoubleBufferLen","I");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"extDoubleBufferLen\" in class anl.epanet.model.EPANETMSXModelInterface\n"))
		return FALSE;

	return TRUE;
}
static void x_resetEPANETMSXError(JNIEnv *env,jobject model)
{
	(*env)->SetIntField(env,model,modelData->errCodeField,0);
	(*env)->SetObjectField(env,model,modelData->errStringField,NULL);
}
static void x_throwEPANETMSXError(JNIEnv *env,jobject model,int errcode,char *func)
{
	char msg[256];
	char emsg[256];

	MSXgeterror(errcode,emsg,256);
	sprintf(msg,"%s: %s occurred in method %s",errcode>100?"Error":"Warning",emsg,func);
	(*env)->SetIntField(env,model,modelData->errCodeField,errcode);
	(*env)->SetObjectField(env,model,modelData->errStringField,(*env)->NewStringUTF(env,msg));
	if(errcode > 100)
		(*env)->ThrowNew(env,EPANETMSXModel_getData(env)->epanetmsxExceptionClass,msg);
}

static void x_notYetImplemented(JNIEnv *env,const char *func)
{
	char buf[100];
	jeprintf(env,buf,100,"The %s() function has not yet been implemented!\n",func);
}

static double *x_getNewDoubleBuffer(JNIEnv *env, jobject model, double *buf, int size) {
	x_ensureMSXModelDataInitialized(env);
	if(buf != NULL)
		free(buf);
	buf=(double*)calloc(size,sizeof(double));
	(*env)->SetIntField(env,model,modelData->doubleBufferLengthField,size);
	(*env)->SetLongField(env,model,modelData->doubleBufferField,PTRTOJLONG(buf));
	return buf;
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxOpen
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxOpen
  (JNIEnv *env, jobject model, jstring inputFile)
{
	int errcode;
	char *iFile;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);
	iFile = ANL_UTIL_javaStringToC(env,inputFile);

	errcode = MSXopen(iFile);
	ANL_UTIL_free(iFile);
	if(errcode)
		x_throwEPANETMSXError(env,model,errcode,"msxOpen");
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxUseHydFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxUseHydFile
  (JNIEnv *env, jobject model, jstring hydFileName)
{
	int errcode;
	char *hFile;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);
	hFile = ANL_UTIL_javaStringToC(env,hydFileName);

	errcode = MSXusehydfile(hFile);
	ANL_UTIL_free(hFile);
	if(errcode)
		x_throwEPANETMSXError(env,model,errcode,"msxUseHydFile");
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxInit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxInit
  (JNIEnv *env, jobject model, jint saveFlag)
{
	int errcode;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);
	errcode = MSXinit(saveFlag);
	if(errcode)
		x_throwEPANETMSXError(env,model,errcode,"msxInit");
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxSetSources
 * Signature: ([[F)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxSetSources
  (JNIEnv *env, jobject model, jobjectArray jSourceStrength)
{
	int errcode,numSpecies;
	int i;
	jfloatArray jSources;

	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);

	numSpecies = (*env)->GetArrayLength(env, jSourceStrength);

	for(i=0;i<numSpecies;i++) {
		jSources = (*env)->GetObjectArrayElement(env,jSourceStrength,i);
		if(jSources != NULL) {
			float *sources;
			int j;
			int nnodes = (*env)->GetArrayLength(env, jSources);
			sources=(*env)->GetPrimitiveArrayCritical(env,jSources,NULL);
			for(j=0;j<nnodes;j++) {
				if(errcode = MSXsetsource(j+1,i+1,(sources[j]==0?MSX_NOSOURCE:MSX_MASS),sources[j],0)) {
					x_throwEPANETMSXError(env,model,errcode,"msxSetSources");
				}
			}
			(*env)->ReleasePrimitiveArrayCritical(env,jSources,sources,JNI_ABORT);
		}
	}
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxStep
 * Signature: ([J)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxStep
  (JNIEnv *env, jobject model, jlongArray jtimes)
{
	jlong *times;
	long entime,tstep;
	int errcode;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);

	// times[0]: entime;
	// times[1]: tstep;
	errcode = MSXstep(&entime,&tstep);
	times=(*env)->GetPrimitiveArrayCritical(env,jtimes,NULL);
	times[0]=entime;
	times[1]=tstep;
	(*env)->ReleasePrimitiveArrayCritical(env,jtimes,times,0);
	if(errcode) {
		x_throwEPANETMSXError(env,model,errcode,"msxStep");
	}
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxGetInitQual
 * Signature: (II[DJI)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxGetInitQual
  (JNIEnv *env, jobject model, jint msxElemType, jint speciesIdx, jdoubleArray jiq, jlong buf, jint bufSize)
{
	int iqLen,n,errcode;
	double *iq;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);
	iqLen=(*env)->GetArrayLength(env,jiq);
	iq=(*env)->GetPrimitiveArrayCritical(env,jiq,NULL);
	for(n=0;n<iqLen;n++) {
		if(errcode = MSXgetinitqual((int)msxElemType,n+1,speciesIdx,&iq[n])) {
			x_throwEPANETMSXError(env,model,errcode,"msxGetInitQual");
		}
	}
	(*env)->ReleasePrimitiveArrayCritical(env,jiq,iq,0);
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxRestoreInitQual
 * Signature: (II[DJI)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxRestoreInitQual
  (JNIEnv *env, jobject model, jint msxElemType, jint speciesIdx, jdoubleArray jiq, jlong buf, jint bufSize)
{
	int errcode,num,i;
	double *iq;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);

	num = (*env)->GetArrayLength(env, jiq);
	iq=(*env)->GetPrimitiveArrayCritical(env,jiq,NULL);
	for(i=0;i<num;i++) {
		if(errcode = MSXsetinitqual(msxElemType,i+1,speciesIdx,iq[i])) {
			x_throwEPANETMSXError(env,model,errcode,"msxRestoreInitQual");
		}
	}
	(*env)->ReleasePrimitiveArrayCritical(env,jiq,iq,JNI_ABORT);
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxGetInitialSources
 * Signature: (II[I[D[I)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxGetInitialSources
  (JNIEnv *env, jobject model, jint speciesIdx, jintArray jtypes, jdoubleArray jlevels, jintArray jpats)
{
	int errcode,num,i;
	double *levels;
	int *types,*pats;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);

	num = (*env)->GetArrayLength(env, jtypes);
	types=(*env)->GetPrimitiveArrayCritical(env,jtypes,NULL);
	levels=(*env)->GetPrimitiveArrayCritical(env,jlevels,NULL);
	pats=(*env)->GetPrimitiveArrayCritical(env,jpats,NULL);
	for(i=0;i<num;i++) {
		if(errcode = MSXgetsource(i+1, speciesIdx, &types[i], &levels[i], &pats[i])) {
			x_throwEPANETMSXError(env,model,errcode,"msxGetInitialSources");
		}
	}
	(*env)->ReleasePrimitiveArrayCritical(env,jtypes,types,0);
	(*env)->ReleasePrimitiveArrayCritical(env,jlevels,levels,0);
	(*env)->ReleasePrimitiveArrayCritical(env,jpats,pats,0);
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxSetInitialSources
 * Signature: (II[I[D[I)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxSetInitialSources
  (JNIEnv *env, jobject model, jint speciesIdx, jintArray jtypes, jdoubleArray jlevels, jintArray jpats)
{
	int errcode,num,i;
	double *levels;
	int *types,*pats;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);

	num = (*env)->GetArrayLength(env, jtypes);
	types=(*env)->GetPrimitiveArrayCritical(env,jtypes,NULL);
	levels=(*env)->GetPrimitiveArrayCritical(env,jlevels,NULL);
	pats=(*env)->GetPrimitiveArrayCritical(env,jpats,NULL);
	for(i=0;i<num;i++) {
		if(errcode = MSXsetsource(i+1, speciesIdx, types[i], levels[i], pats[i])) {
			x_throwEPANETMSXError(env,model,errcode,"msxSetInitialSources");
		}
	}
	(*env)->ReleasePrimitiveArrayCritical(env,jtypes,types,JNI_ABORT);
	(*env)->ReleasePrimitiveArrayCritical(env,jlevels,levels,JNI_ABORT);
	(*env)->ReleasePrimitiveArrayCritical(env,jpats,pats,JNI_ABORT);
}
/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxClose
  (JNIEnv *env, jobject model)
{
	int errcode;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);
	errcode = MSXclose();
	if(errcode)
		x_throwEPANETMSXError(env,model,errcode,"msxClose");
}

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxGetQuality
 * Signature: (II[DJI)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxGetQuality
  (JNIEnv *env, jobject model, jint msxElemType, jint speciesIdx, jfloatArray jq)
{
	int qLen,n,errcode;
	float *q;
	x_ensureMSXModelDataInitialized(env);
	x_resetEPANETMSXError(env,model);
	qLen=(*env)->GetArrayLength(env,jq);
	q=(*env)->GetPrimitiveArrayCritical(env,jq,NULL);
	for(n=0;n<qLen;n++) {
		double qv;
		if(errcode = MSXgetqual((int)msxElemType,n+1,speciesIdx,&qv)) {
			x_throwEPANETMSXError(env,model,errcode,"msxGetInitQual");
		}
		q[n]=(float)qv;
	}
	(*env)->ReleasePrimitiveArrayCritical(env,jq,q,0);
}
