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
#include "EPANETModel.h"
#include "jniUtils.h"
#include "toolkit.h"
#include <stdlib.h>
#include <string.h>

/* Geometry object functions */
static EPANETModelDataPtr modelData=NULL;

static void x_ensureModelDataInitialized(JNIEnv *env);
static jboolean x_initModelData(JNIEnv *env);

static void x_ensureModelDataInitialized(JNIEnv *env)
{
	if(modelData == NULL)
		x_initModelData(env);
}

JNIEXPORT EPANETModelDataPtr EPANETModel_getData(JNIEnv *env)
{
	x_ensureModelDataInitialized(env);
	return modelData;
}

static jboolean x_initModelData(JNIEnv *env)
{
	jclass tclass;
	if(modelData != NULL)
		return TRUE;
	modelData = (EPANETModelDataPtr)calloc(1,sizeof(EPANETModelData));

	tclass = (*env)->FindClass(env,"anl/epanet/model/EPANETModelInterface");
	if(ANL_UTIL_CheckForException(env, "unable to find class anl.epanet.model.EPANETModelInterface\n"))
		return FALSE;
	modelData->epanetModelClass = (*env)->NewGlobalRef(env,tclass);

	tclass = (*env)->FindClass(env,"anl/epanet/model/EPANETException");
	if(ANL_UTIL_CheckForException(env, "unable to find class anl.epanet.model.EPANETException\n"))
		return FALSE;
	modelData->epanetExceptionClass = (*env)->NewGlobalRef(env,tclass);

	tclass = (*env)->FindClass(env,"anl/epanet/model/ENLinkNodes");
	if(ANL_UTIL_CheckForException(env, "unable to find class anl.epanet.model.ENLinkNodes\n"))
		return FALSE;
	modelData->enLinkNodesClass = (*env)->NewGlobalRef(env,tclass);

	modelData->enLinkNodesConstructor = (*env)->GetMethodID(env,modelData->enLinkNodesClass,"<init>","(II)V");
	if(ANL_UTIL_CheckForException(env, "unable to find constructor ENLinkNodes(int,int) in class anl.epanet.model.ENLinkNodes\n"))
		return FALSE;

	modelData->errCodeField = (*env)->GetFieldID(env,modelData->epanetModelClass,"errCode","I");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"errCode\" in class anl.epanet.model.EPANETModelInterface\n"))
		return FALSE;
	modelData->errStringField = (*env)->GetFieldID(env,modelData->epanetModelClass,"errString","Ljava/lang/String;");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"errString\" in class anl.epanet.model.EPANETModelInterface\n"))
		return FALSE;
	modelData->floatBufferField = (*env)->GetFieldID(env,modelData->epanetModelClass,"extFloatBuffer","J");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"extFloatBuffer\" in class anl.epanet.model.EPANETModelInterface\n"))
		return FALSE;
	modelData->floatBufferLengthField = (*env)->GetFieldID(env,modelData->epanetModelClass,"extFloatBufferLen","I");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"extFloatBufferLen\" in class anl.epanet.model.EPANETModelInterface\n"))
		return FALSE;
	modelData->intBufferField = (*env)->GetFieldID(env,modelData->epanetModelClass,"extIntBuffer","J");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"extIntBuffer\" in class anl.epanet.model.EPANETModelInterface\n"))
		return FALSE;
	modelData->intBufferLengthField = (*env)->GetFieldID(env,modelData->epanetModelClass,"extIntBufferLen","I");
	if(ANL_UTIL_CheckForException(env, "unable to find field \"extIntBufferLen\" in class anl.epanet.model.EPANETModelInterface\n"))
		return FALSE;

	return TRUE;
}
static void x_resetEPANETError(JNIEnv *env,jobject model)
{
	(*env)->SetIntField(env,model,modelData->errCodeField,0);
	(*env)->SetObjectField(env,model,modelData->errStringField,NULL);
}
static void x_throwEPANETError(JNIEnv *env,jobject model,int errcode,char *func)
{
	char msg[256];
	char emsg[256];

	ENgeterror(errcode,emsg,256);
	sprintf(msg,"%s: %s occurred in method %s",errcode>100?"Error":"Warning",emsg,func);
	(*env)->SetIntField(env,model,modelData->errCodeField,errcode);
	(*env)->SetObjectField(env,model,modelData->errStringField,(*env)->NewStringUTF(env,msg));
	if(errcode > 100)
		(*env)->ThrowNew(env,EPANETModel_getData(env)->epanetExceptionClass,msg);
}

static void x_notYetImplemented(JNIEnv *env,const char *func)
{
	char buf[100];
	jeprintf(env,buf,100,"The %s() function has not yet been implemented!\n",func);
}

static float *x_getNewFloatBuffer(JNIEnv *env, jobject model, float*buf, int size) {
	x_ensureModelDataInitialized(env);
	if(buf != NULL)
		free(buf);
	buf=(float*)calloc(size,sizeof(float));
	(*env)->SetIntField(env,model,modelData->floatBufferLengthField,size);
	(*env)->SetLongField(env,model,modelData->floatBufferField,PTRTOJLONG(buf));
	return buf;
}
static int *x_getNewIntBuffer(JNIEnv *env, jobject model, int*buf, int size) {
	x_ensureModelDataInitialized(env);
	if(buf != NULL)
		free(buf);
	buf=(int*)calloc(size,sizeof(int));
	(*env)->SetIntField(env,model,modelData->intBufferLengthField,size);
	(*env)->SetLongField(env,model,modelData->intBufferField,PTRTOJLONG(buf));
	return buf;
}
/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enEpanet
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/reflect/Method;)V
 */
JNIEXPORT void JNICALL Java_enEpanet
  (JNIEnv *env, jobject model, jstring inputFile, jstring reportFile, jstring outputFile, jobject progressMethod) 
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enEpanet");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enOpen
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enOpen
  (JNIEnv *env, jobject model, jstring inputFile, jstring reportFile, jstring outputFile)
{
	int errcode;
	char *iFile,*repFile,*outFile;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	iFile = ANL_UTIL_javaStringToC(env,inputFile);
	if(reportFile == NULL)
		repFile="";
	else
		repFile = ANL_UTIL_javaStringToC(env,reportFile);
	if(outputFile == NULL)
		outFile="";
	else
		outFile = ANL_UTIL_javaStringToC(env,outputFile);

	errcode = ENopen(iFile,repFile,outFile);
	ANL_UTIL_free(iFile);
	if(reportFile != NULL) ANL_UTIL_free(repFile);
	if(outputFile != NULL) ANL_UTIL_free(outFile);
	if(errcode)
		x_throwEPANETError(env,model,errcode,"enOpen");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSaveInpfile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enSaveInpfile
  (JNIEnv *env, jobject model, jstring inputFileName)
{
	int errcode;
	char *fn;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	fn = ANL_UTIL_javaStringToC(env,inputFileName);

	errcode = ENsaveinpfile(fn);
	if(errcode)
		x_throwEPANETError(env,model,errcode,"enSaveInpfile");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enClose
  (JNIEnv *env, jobject model)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENclose())
		x_throwEPANETError(env,model,errcode,"enClose");
		
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSolveH
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enSolveH
  (JNIEnv *env, jobject model)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENsolveH())
		x_throwEPANETError(env,model,errcode,"enSolveH");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSaveH
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enSaveH
  (JNIEnv *env, jobject model)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enSaveH");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enOpenH
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enOpenH
  (JNIEnv *env, jobject model)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENopenH())
		x_throwEPANETError(env,model,errcode,"enOpenH");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enInitH
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_enInitH
  (JNIEnv *env, jobject model, jint flag)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENinitH(flag))
		x_throwEPANETError(env,model,errcode,"enInitH");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enRunH
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enRunH
  (JNIEnv *env, jobject model)
{
	int errcode;
	long t=0;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENrunH(&t))
		x_throwEPANETError(env,model,errcode,"enRunH");
	return t;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enNextH
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enNextH
  (JNIEnv *env, jobject model)
{
	int errcode;
	long tstep=0;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENnextH(&tstep))
		x_throwEPANETError(env,model,errcode,"enNextH");
	return tstep;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enCloseH
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enCloseH
  (JNIEnv *env, jobject model)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENcloseH())
		x_throwEPANETError(env,model,errcode,"enCloseH");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSaveHydFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enSaveHydFile
  (JNIEnv *env, jobject model, jstring hydFileName)
{
	char *hydFile;
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	hydFile=ANL_UTIL_javaStringToC(env,hydFileName);
	errcode = ENsavehydfile(hydFile);
	ANL_UTIL_free(hydFile);
	if(errcode)
		x_throwEPANETError(env,model,errcode,"enSaveHydFile");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enUseHydFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enUseHydFile
  (JNIEnv *env, jobject model, jstring hydFileName)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enUseHydFile");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSolveQ
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enSolveQ
  (JNIEnv *env, jobject model)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enSolveQ");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enOpenQ
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enOpenQ
  (JNIEnv *env, jobject model)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENopenQ())
		x_throwEPANETError(env,model,errcode,"enOpenQ");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enInitQ
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_enInitQ
  (JNIEnv *env, jobject model, jint saveFlag)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENinitQ(saveFlag))
		x_throwEPANETError(env,model,errcode,"enInitQ");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enRunQ
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enRunQ
  (JNIEnv *env, jobject model)
{
	int errcode;
	long t=0;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENrunQ(&t))
		x_throwEPANETError(env,model,errcode,"enRunQ");
	return t;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enNextQ
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enNextQ
  (JNIEnv *env, jobject model)
{
	int errcode;
	long tstep=0;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENnextQ(&tstep))
		x_throwEPANETError(env,model,errcode,"enNextQ");
	return tstep;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enStepQ
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enStepQ
  (JNIEnv *env, jobject model)
{
	int errcode;
	long tstep=0;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENstepQ(&tstep))
		x_throwEPANETError(env,model,errcode,"enStepQ");
	return tstep;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enCloseQ
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enCloseQ
  (JNIEnv *env, jobject model)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENcloseQ())
		x_throwEPANETError(env,model,errcode,"enCloseQ");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enReport
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enReport
  (JNIEnv *env, jobject model)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENreport())
		x_throwEPANETError(env,model,errcode,"enReport");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enResetReport
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enResetReport
  (JNIEnv *env, jobject model)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enResetReport");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetReport
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enSetReport
  (JNIEnv *env, jobject model, jstring command)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enSetReport");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetControl
 * Signature: (I)Lanl/epanet/model/ENControl;
 */
JNIEXPORT jobject JNICALL Java_enGetControl
  (JNIEnv *env, jobject model, jint controlIndex)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enGetControl");
	return NULL;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetCount
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_enGetCount
  (JNIEnv *env, jobject model, jint countCode)
{
	int errcode;
	int count;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetcount(countCode,&count))
		x_throwEPANETError(env,model,errcode,"enGetCount");
	return count;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetOption
 * Signature: (I)F
 */
JNIEXPORT jfloat JNICALL Java_enGetOption
  (JNIEnv *env, jobject model, jint optionCode)
{
	int errcode;
	float value;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetoption(optionCode,&value)) {
		x_throwEPANETError(env,model,errcode,"enGetOption");
	}
	return value;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetTimeParam
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_enGetTimeParam
  (JNIEnv *env, jobject model, jint paramCode)
{
	int errcode;
	long value;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgettimeparam(paramCode,&value)) {
		x_throwEPANETError(env,model,errcode,"enGettimeparam");
	}
	return value;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetFlowUnits
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_enGetFlowUnits
  (JNIEnv *env, jobject model)
{
	int errcode;
	int value;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetflowunits(&value)) {
		x_throwEPANETError(env,model,errcode,"enGetFlowUnits");
	}
	return value;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetPatternIndex
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_enGetPatternIndex
  (JNIEnv *env, jobject model, jstring patternID)
{
	int errcode,index;
	char *id;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	id = ANL_UTIL_javaStringToC(env,patternID);
	errcode = ENgetpatternindex(id,&index);
	ANL_UTIL_free(id);
	if(errcode)
		x_throwEPANETError(env,model,errcode,"enGetPatternIndex");
	return index;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetPatternID
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_enGetPatternID
  (JNIEnv *env, jobject model, jint patternIndex)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enGetPatternID");
	return NULL;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetPatternLen
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_enGetPatternLen
  (JNIEnv *env, jobject model, jint patternIndex)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enGetPatternLen");
	return 0;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetPatternValue
 * Signature: (II)F
 */
JNIEXPORT jfloat JNICALL Java_enGetPatternValue
  (JNIEnv *env, jobject model, jint patternIndex, jint period)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enGetPatternValue");
	return 0;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetQualType
 * Signature: ()Lanl/epanet/model/ENQuality;
 */
JNIEXPORT jobject JNICALL Java_enGetQualType
  (JNIEnv *env, jobject model)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enGetQualType");
	return NULL;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetError
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_enGetError
  (JNIEnv *env, jobject model, jint errcode)
{
	int errorcode;
	char msg[256];
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errorcode = ENgeterror(errcode,msg,256))
		x_throwEPANETError(env,model,errorcode,"enGetError");
	return (*env)->NewStringUTF(env,msg);
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetNodeIndex
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_enGetNodeIndex
  (JNIEnv *env, jobject model, jstring nodeID)
{
	int errcode,index;
	char *id;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	id = ANL_UTIL_javaStringToC(env,nodeID);
	errcode = ENgetnodeindex(id,&index);
	ANL_UTIL_free(id);
	if(errcode)
		x_throwEPANETError(env,model,errcode,"enGetNodeIndex");
	return index;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetNodeID
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_enGetNodeID
  (JNIEnv *env, jobject model, jint nodeIndex)
{
	int errcode;
	char id[128];

	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetnodeid(nodeIndex,id))
		x_throwEPANETError(env,model,errcode,"enGetNodeID");

	return (*env)->NewStringUTF(env,id);
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetNodeType
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_enGetNodeType
  (JNIEnv *env, jobject model, jint nodeIndex)
{
	int errcode,nodetype;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetnodetype(nodeIndex,&nodetype)) {
		x_throwEPANETError(env,model,errcode,"enGetNodeType");
	}
	return nodetype;
}
/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetNodeTypes
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL Java_enGetNodeTypes
  (JNIEnv *env, jobject model, jintArray jtypes, jlong buf, jint bufsize)
{
	int errcode;
	int num, i;
	int *types;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	num = (*env)->GetArrayLength(env, jtypes);
	types = PTRCAST(int*)buf;
	if(bufsize < num) {
		types=x_getNewIntBuffer(env,model,PTRCAST(int*)buf,num);
	}
	for(i=0;i<num;i++) {
		if(errcode = ENgetnodetype(i+1,&types[i])) {
			x_throwEPANETError(env,model,errcode,"enGetNodeTypes");
		}
	}
	(*env)->SetIntArrayRegion(env,jtypes,0,num,types);
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetNodeValue
 * Signature: (II)F
 */
JNIEXPORT jfloat JNICALL Java_enGetNodeValue
  (JNIEnv *env, jobject model, jint nodeIndex, jint paramCode)
{
	int errcode;
	float value;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetnodevalue(nodeIndex,paramCode,&value)) {
		x_throwEPANETError(env,model,errcode,"enGetNodeValue");
	}
	return value;
}
/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetNodeValue
 * Signature: (I[F)V
 */
JNIEXPORT void JNICALL Java_enGetNodeValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray jvals, jint startIdx, jint len, jlong buf, jint bufsize)
{
	int errcode;
	int i;
	float *vals;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	vals=(*env)->GetPrimitiveArrayCritical(env,jvals,NULL);
	for(i=0;i<len;i++) {
		if(errcode = ENgetnodevalue(i+startIdx+1,paramCode,&vals[i])) {
			x_throwEPANETError(env,model,errcode,"enGetNodeValues");
		}
	}
	(*env)->ReleasePrimitiveArrayCritical(env,jvals,vals,0);
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetLinkIndex
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_enGetLinkIndex
  (JNIEnv *env, jobject model, jstring linkID)
{
	int errcode,index;
	char *id;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	id = ANL_UTIL_javaStringToC(env,linkID);
	errcode = ENgetlinkindex(id,&index);
	ANL_UTIL_free(id);
	if(errcode)
		x_throwEPANETError(env,model,errcode,"enGetLinkIndex");
	return index;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetLinkID
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_enGetLinkID
  (JNIEnv *env, jobject model, jint linkIndex)
{
	int errcode;
	char id[128];
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetlinkid(linkIndex,id))
		x_throwEPANETError(env,model,errcode,"enGetLinkID");
	return (*env)->NewStringUTF(env,id);
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetLinkType
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_enGetLinkType
  (JNIEnv *env, jobject model, jint linkIndex)
{
	int errcode,linktype;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetlinktype(linkIndex,&linktype)) {
	x_throwEPANETError(env,model,errcode,"enGetLinkType");
	}
	return linktype;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetLinkType
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_enGetLinkTypes
  (JNIEnv *env, jobject model, jintArray jtypes, jlong buf, jint bufsize)
{
	int errcode;
	int num, i;
	int *types;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	num = (*env)->GetArrayLength(env, jtypes);
	types = PTRCAST(int*)buf;
	if(bufsize < num) {
		types=x_getNewIntBuffer(env,model,PTRCAST(int*)buf,num);
	}
	for(i=0;i<num;i++) {
		if(errcode = ENgetlinktype(i+1,&types[i])) {
			x_throwEPANETError(env,model,errcode,"enGetLinkTypes");
		}
	}
	(*env)->SetIntArrayRegion(env,jtypes,0,num,types);
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetLinkNodes
 * Signature: (I)Lanl/epanet/model/ENLinkNodes;
 */
JNIEXPORT jobject JNICALL Java_enGetLinkNodes
  (JNIEnv *env, jobject model, jint linkIndex)
{
	int errcode;
	int from,to;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetlinknodes(linkIndex,&from,&to))
		x_throwEPANETError(env,model,errcode,"enGetLinkNodes");
	return (*env)->NewObject(env,modelData->enLinkNodesClass,modelData->enLinkNodesConstructor,from,to);

}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetLinkValue
 * Signature: (II)F
 */
JNIEXPORT jfloat JNICALL Java_enGetLinkValue
  (JNIEnv *env, jobject model, jint linkIndex, jint paramCode)
{
	int errcode;
	float value;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetlinkvalue(linkIndex,paramCode,&value)) {
		x_throwEPANETError(env,model,errcode,"enGetLinkValue");
	}
	return value;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetNodeValue
 * Signature: (I[F)V
 */
JNIEXPORT void JNICALL Java_enGetLinkValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray jvals, jint startIdx, jint len, jlong buf, jint bufsize)
{
	int errcode;
	int i;
	float *vals;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	vals=(*env)->GetPrimitiveArrayCritical(env,jvals,NULL);
	for(i=0;i<len;i++) {
		if(errcode = ENgetlinkvalue(i+startIdx+1,paramCode,&vals[i])) {
			x_throwEPANETError(env,model,errcode,"enGetLinkValues");
		}
	}
	(*env)->ReleasePrimitiveArrayCritical(env,jvals,vals,0);
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetVersion
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_enGetVersion
  (JNIEnv *env, jobject model)
{
	int errcode,version;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENgetversion(&version)) {
		x_throwEPANETError(env,model,errcode,"enGetVersion");
	}
	return version;
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetControl
 * Signature: (ILanl/epanet/model/ENControl;)V
 */
JNIEXPORT void JNICALL Java_enSetControl
  (JNIEnv *env, jobject model, jint controlIndex, jobject enControl)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enSetControl");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetNodeValue
 * Signature: (IIF)V
 */
JNIEXPORT void JNICALL Java_enSetNodeValue
  (JNIEnv *env, jobject model, jint nodeIndex, jint paramCode, jfloat value)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENsetnodevalue(nodeIndex,paramCode,value)) {
		x_throwEPANETError(env,model,errcode,"enSetNodeValue");
	}
}

JNIEXPORT void JNICALL Java_enSetNodeValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray values, jlong buf, jint bufsize)
{
	int errcode,num,i;
	float *vals;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	num = (*env)->GetArrayLength(env, values);
	vals = PTRCAST(float*)buf;
	if(bufsize < num) {
		vals=x_getNewFloatBuffer(env,model,PTRCAST(float*)buf,num);
	}
	(*env)->GetFloatArrayRegion(env,values,0,num,vals);
	for(i=0;i<num;i++) {
		if(errcode = ENsetnodevalue(i+1,paramCode,vals[i])) {
			x_throwEPANETError(env,model,errcode,"enSetNodeValues");
		}
	}
}
JNIEXPORT void JNICALL Java_enSetConstantNodeValues
  (JNIEnv *env, jobject model,jint paramCode,jfloat constantVal, jint num)
{
	int errcode,i;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	for(i=0;i<num;i++) {
		if(errcode = ENsetnodevalue(i+1,paramCode,constantVal)) {
			x_throwEPANETError(env,model,errcode,"enSetNodeValues");
		}
	}
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetLinkValue
 * Signature: (IIF)V
 */
JNIEXPORT void JNICALL Java_enSetLinkValue
  (JNIEnv *env, jobject model, jint linkIndex, jint paramCode, jfloat value)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENsetlinkvalue(linkIndex,paramCode,value)) {
		x_throwEPANETError(env,model,errcode,"enSetLinkValue");
	}
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetPattern
 * Signature: (I[F)V
 */
JNIEXPORT void JNICALL Java_enSetPattern
  (JNIEnv *env, jobject model, jint patternIndex, jfloatArray factors)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enSetPattern");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetPatternValue
 * Signature: (IIF)V
 */
JNIEXPORT void JNICALL Java_enSetPatternValue
  (JNIEnv *env, jobject model, jint patternIndex, jint period, jfloat value)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENsetpatternvalue(patternIndex,period,value)) {
		x_throwEPANETError(env,model,errcode,"enSetPatternValue");
	}
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetTimeParam
 * Signature: (IJ)V
 */
JNIEXPORT void JNICALL Java_enSetTimeParam
  (JNIEnv *env, jobject model, jint paramCode, jlong value)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENsettimeparam(paramCode,value)) {
		x_throwEPANETError(env,model,errcode,"enSetTimeParam");
	}
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetOption
 * Signature: (IF)V
 */
JNIEXPORT void JNICALL Java_enSetOption
  (JNIEnv *env, jobject model, jint optionCode, jfloat value)
{
	int errcode;
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	if(errcode = ENsetoption(optionCode,value)) {
		x_throwEPANETError(env,model,errcode,"enSetOption");
	}
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetStatusReport
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_enSetStatusReport
  (JNIEnv *env, jobject model, jint statusLevel)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enSetStatusReport");
}

/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enSetQualType
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enSetQualType
  (JNIEnv *env, jobject model, jint qualCode, jstring chemName, jstring chemUnits, jstring traceNode)
{
	x_ensureModelDataInitialized(env);
	x_resetEPANETError(env,model);
	x_notYetImplemented(env,"enSetQualType");
}

