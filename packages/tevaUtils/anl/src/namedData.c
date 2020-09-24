/*
 * Copyright © 2008 UChicago Argonne, LLC
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
#include "NamedData.h"
#include "NamedDataJNI.h"
#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include "jniUtils.h"
#include "loggingUtils.h"

#include "ExternalAnalysis.h"


static NamedDataPtr x_NamedData;
static NamedDataPtr x_NamedDataServer;
static jobject x_DataTypes[NUM_TYPES];

static NamedDataPtr x_initNamedData(JNIEnv *env, int isServer);
static void x_initDataTypes(JNIEnv *env);

static void x_ensureNamedDataInitialized(JNIEnv *env)
{
	if(x_NamedData == NULL || x_NamedDataServer == NULL) {
		x_NamedData = x_initNamedData(env,0);
		x_NamedDataServer = x_initNamedData(env,1);
		x_initDataTypes(env);
	}
}

static void x_initDataTypes(JNIEnv *env)
{
	jclass dataClass = (*env)->FindClass(env,"anl/msv/dpf/util/named_data/Data");
//	x_DataTypes = calloc(NUM_TYPES,sizeof(jobject));
	if(ANL_UTIL_CheckForException(env, "unable to find class anl.msv.dpf.util.named_data.Data\n"))
		return;

	x_DataTypes[INT_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"INT_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[INT_ARRAY_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"INT_ARRAY_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[INT_ARRAY_2D_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"INT_ARRAY_2D_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));

	x_DataTypes[FLOAT_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"FLOAT_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[FLOAT_ARRAY_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"FLOAT_ARRAY_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[FLOAT_ARRAY_2D_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"FLOAT_ARRAY_2D_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	
	x_DataTypes[DOUBLE_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"DOUBLE_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[DOUBLE_ARRAY_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"DOUBLE_ARRAY_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[DOUBLE_ARRAY_2D_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"DOUBLE_ARRAY_2D_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	
	x_DataTypes[BYTE_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"BYTE_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[BYTE_ARRAY_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"BYTE_ARRAY_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[BYTE_ARRAY_2D_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"BYTE_ARRAY_2D_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	
	x_DataTypes[STRING_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"STRING_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
	x_DataTypes[STRING_ARRAY_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"STRING_ARRAY_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));

	x_DataTypes[NAMED_DATA_TYPE] = (*env)->NewGlobalRef(env,ANL_UTIL_getObjectStaticVar(env,dataClass,"NAMED_DATA_TYPE","Lanl/msv/dpf/util/named_data/DataType;"));
}
static NamedDataPtr x_initNamedData(JNIEnv *env, int isServer)
{
	jclass tclass;
	NamedDataPtr p = (NamedDataPtr)calloc(1,sizeof(NamedData));

	if(isServer) {
		tclass = (*env)->FindClass(env,"anl/msv/dpf/util/named_data/NamedDataServer");
		if(ANL_UTIL_CheckForException(env, "unable to find class anl.msv.dpf.util.named_data.NamedDataServer\n"))
			return NULL;
	} else {
		tclass = (*env)->FindClass(env,"anl/msv/dpf/util/named_data/NamedData");
		if(ANL_UTIL_CheckForException(env, "unable to find class anl.msv.dpf.util.named_data.NamedData\n"))
			return NULL;
	}
	p->namedDataClass = (*env)->NewGlobalRef(env,tclass);

	tclass = (*env)->FindClass(env,"java/lang/String");
	p->stringClass = (*env)->NewGlobalRef(env,tclass);

	// classes
	tclass = (*env)->FindClass(env,"[I");
	if(ANL_UTIL_CheckForException(env, "unable to find int[] class\n"))
		return NULL;
	p->intArrayClass = (*env)->NewGlobalRef(env,tclass);

	tclass = (*env)->FindClass(env,"[F");
	if(ANL_UTIL_CheckForException(env, "unable to find float[] class\n"))
		return NULL;
	p->floatArrayClass = (*env)->NewGlobalRef(env,tclass);

	tclass = (*env)->FindClass(env,"[D");
	if(ANL_UTIL_CheckForException(env, "unable to find double[] class\n"))
		return NULL;
	p->doubleArrayClass = (*env)->NewGlobalRef(env,tclass);
	// miscellaneous methods
	p->ndAddVariableMethod = (*env)->GetMethodID(env,p->namedDataClass,"addVariable","(Ljava/lang/String;Lanl/msv/dpf/util/named_data/DataType;)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void addVariable method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndHasDataMethod = (*env)->GetMethodID(env,p->namedDataClass,"hasData","(Ljava/lang/String;)Z");
	if(ANL_UTIL_CheckForException(env, "unable to find boolean hasData method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

//	p->ndGetDataMethod = (*env)->GetMethodID(env,p->namedDataClass,"getData","(Ljava/lang/String;Lanl/msv/dpf/util/named_data/DataType;)Ljava/lang/Object;");
//	if(ANL_UTIL_CheckForException(env, "unable to find Object getData method in class anl.msv.dpf.util.named_data.NamedData\n"))
//		return NULL;

	// get methods
	p->ndGetIntMethod = (*env)->GetMethodID(env,p->namedDataClass,"getInt","(Ljava/lang/String;)I");
	if(ANL_UTIL_CheckForException(env, "unable to find int getInt(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetIntArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"getIntArray","(Ljava/lang/String;)[I");
	if(ANL_UTIL_CheckForException(env, "unable to find int[] getIntArray(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetIntArrayAtIndexMethod = (*env)->GetMethodID(env,p->namedDataClass,"getIntArray","(Ljava/lang/String;I)[I");
	if(ANL_UTIL_CheckForException(env, "unable to find int[] getIntArray(String,int) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndGetByteMethod = (*env)->GetMethodID(env,p->namedDataClass,"getByte","(Ljava/lang/String;)B");
	if(ANL_UTIL_CheckForException(env, "unable to find byte getByte(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetByteArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"getByteArray","(Ljava/lang/String;)[B");
	if(ANL_UTIL_CheckForException(env, "unable to find byte[] getByteArray(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetByteArrayAtIndexMethod = (*env)->GetMethodID(env,p->namedDataClass,"getByteArray","(Ljava/lang/String;I)[B");
	if(ANL_UTIL_CheckForException(env, "unable to find byte[] getByteArray(String,int) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndGetFloatMethod = (*env)->GetMethodID(env,p->namedDataClass,"getFloat","(Ljava/lang/String;)F");
	if(ANL_UTIL_CheckForException(env, "unable to find float getFloat(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetFloatArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"getFloatArray","(Ljava/lang/String;)[F");
	if(ANL_UTIL_CheckForException(env, "unable to find float[] getFloatArray(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetFloatArrayAtIndexMethod = (*env)->GetMethodID(env,p->namedDataClass,"getFloatArray","(Ljava/lang/String;I)[F");
	if(ANL_UTIL_CheckForException(env, "unable to find float[] getFloatArray(String,int) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndGetDoubleMethod = (*env)->GetMethodID(env,p->namedDataClass,"getDouble","(Ljava/lang/String;)D");
	if(ANL_UTIL_CheckForException(env, "unable to find double getDouble(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetDoubleArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"getDoubleArray","(Ljava/lang/String;)[D");
	if(ANL_UTIL_CheckForException(env, "unable to find double[] getDoubleArray(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetDoubleArrayAtIndexMethod = (*env)->GetMethodID(env,p->namedDataClass,"getDoubleArray","(Ljava/lang/String;I)[D");
	if(ANL_UTIL_CheckForException(env, "unable to find double[] getDoubleArray(String,int) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndGetStringMethod = (*env)->GetMethodID(env,p->namedDataClass,"getString","(Ljava/lang/String;)Ljava/lang/String;");
	if(ANL_UTIL_CheckForException(env, "unable to find String getString(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndGetStringArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"getStringArray","(Ljava/lang/String;)[Ljava/lang/String;");
	if(ANL_UTIL_CheckForException(env, "unable to find String[] getStringArray(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndGetNamedDataMethod = (*env)->GetMethodID(env,p->namedDataClass,"getNamedData","(Ljava/lang/String;)Lanl/msv/dpf/util/named_data/NamedData;");
	if(ANL_UTIL_CheckForException(env, "unable to find NamedData getNamedData(String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	// set methods
	p->ndSetIntMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;I)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,int) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetIntArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[I)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,int[]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetIntArray2DMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[[I)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,int[]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetIntArrayAtIndexMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[[I)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,int[][]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndSetByteMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;B)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,byte) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetByteArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[B)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,byte[]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetByteArrayAtIndexMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[[B)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,byte[][]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndSetFloatMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;F)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,float) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetFloatArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[F)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,float[]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetFloatArray2DMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[[F)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,float[][]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetFloatArrayAtIndexMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[[F)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,float[][]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndSetDoubleMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;D)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,double) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetDoubleArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[D)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,double[]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetDoubleArray2DMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[[D)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,double[][]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetDoubleArrayAtIndexMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[[D)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,double[][]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndSetStringMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;Ljava/lang/String;)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,String) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	p->ndSetStringArrayMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;[Ljava/lang/String;)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,double[]) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;

	p->ndSetNamedDataMethod = (*env)->GetMethodID(env,p->namedDataClass,"setData","(Ljava/lang/String;Lanl/msv/dpf/util/named_data/NamedData;)V");
	if(ANL_UTIL_CheckForException(env, "unable to find void setData(String,NamedData) method in class anl.msv.dpf.util.named_data.NamedData\n"))
		return NULL;
	
	return p;
}

static NamedDataPtr x_getNamedDataPtr(JNIEnv *env, jobject obj)
{
	x_ensureNamedDataInitialized(env);
	if((*env)->IsInstanceOf(env,obj,x_NamedData->namedDataClass)) {
		return x_NamedData;
	} else if ((*env)->IsInstanceOf(env,obj,x_NamedDataServer->namedDataClass)) {
		return x_NamedDataServer;
	} else {
		// print something here.
		return NULL;
	}
}

int hasData(NamedDataRef *dataObject, char *name, int dataType, int *bval) {
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp;

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	x_ensureNamedDataInitialized(env);

	ndp = x_getNamedDataPtr(env,obj);

	if(ndp != NULL) {
		jboolean bv;
		bv = (*env)->CallBooleanMethod(env,obj,ndp->ndHasDataMethod,jname);
		*bval=bv;

		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int addVariable(NamedDataRef *dataObject, char *name, int type)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	jobject jtype;
	NamedDataPtr ndp;

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	x_ensureNamedDataInitialized(env);

	jtype = x_DataTypes[type];

	ndp = x_getNamedDataPtr(env,obj);
	
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndAddVariableMethod,jname,jtype);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		// print something here.
		return ND_FAILURE;
	}
}

void nd_free(void *ptr) {
	free(ptr);
}

void nd_freeStringArray(char **ptr, int len) {
	int i;
	for(i=0;i<len;i++) {
		free(ptr[i]);
	}
	free(ptr);
}
int setInt(NamedDataRef *dataObject, char *name, int val)
{
	jobject obj = dataObject->obj;
	JNIEnv *env = dataObject->env;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetIntMethod,jname,val);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		// print something here.
		return ND_FAILURE;
	}
}

int setIntArray(NamedDataRef *dataObject, char *name, int *val, int length)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname;
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);
	jintArray ja;

	jname = (*env)->NewStringUTF(env,name);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	ja = (*env)->NewIntArray(env,length);
	(*env)->SetIntArrayRegion(env,ja,0,length,val);
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetIntArrayMethod,jname,ja);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int set2DIntArray(NamedDataRef *dataObject, char *name, int **val, int length1, int length2)
{
	int i;
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname;
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);
	jobjectArray res;

	res = (*env)->NewObjectArray(env,length1,ndp->intArrayClass,NULL);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	jname = (*env)->NewStringUTF(env,name);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	for(i=0;i<length1;i++) {
		jintArray ja = (*env)->NewIntArray(env,length2);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		(*env)->SetIntArrayRegion(env,ja,0,length2,val[i]);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		(*env)->SetObjectArrayElement(env,res,i,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	}
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetIntArray2DMethod,jname,res);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

//int setIntArrayAtIndex(void *dataObject, char *name, int idx, int *val, int length);

int setByte(NamedDataRef *dataObject, char *name, char val)
{
	jobject obj = dataObject->obj;
	JNIEnv *env = dataObject->env;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetByteMethod,jname,val);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		// print something here.
		return ND_FAILURE;
	}
}
int setByteArray(NamedDataRef *dataObject, char *name, char *val, int length)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname;
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);
	jfloatArray ja;

	jname = (*env)->NewStringUTF(env,name);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	ja = (*env)->NewByteArray(env,length);
	(*env)->SetByteArrayRegion(env,ja,0,length,val);
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetByteArrayMethod,jname,ja);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

//int setByteArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, char *val, int length){}

int setFloat(NamedDataRef *dataObject, char *name, float val)
{
	jobject obj = dataObject->obj;
	JNIEnv *env = dataObject->env;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetFloatMethod,jname,val);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		// print something here.
		return ND_FAILURE;
	}
}
int setFloatArray(NamedDataRef *dataObject, char *name, float *val, int length)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname;
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);
	jfloatArray ja;

	jname = (*env)->NewStringUTF(env,name);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	ja = (*env)->NewFloatArray(env,length);
	(*env)->SetFloatArrayRegion(env,ja,0,length,val);
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetFloatArrayMethod,jname,ja);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int set2DFloatArray(NamedDataRef *dataObject, char *name, float **val, int length1, int length2)
{
	int i;
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname;
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);
	jobjectArray res;

	res = (*env)->NewObjectArray(env,length1,ndp->floatArrayClass,NULL);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	jname = (*env)->NewStringUTF(env,name);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	for(i=0;i<length1;i++) {
		jfloatArray ja = (*env)->NewFloatArray(env,length2);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		(*env)->SetFloatArrayRegion(env,ja,0,length2,val[i]);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		(*env)->SetObjectArrayElement(env,res,i,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	}
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetFloatArray2DMethod,jname,res);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

//int setFloatArrayAtIndex(void *dataObject, char *name, int idx, float *val, int length){}

int setDouble(NamedDataRef *dataObject, char *name, double val)
{
	jobject obj = dataObject->obj;
	JNIEnv *env = dataObject->env;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetDoubleMethod,jname,val);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		// print something here.
		return ND_FAILURE;
	}
}

int setDoubleArray(NamedDataRef *dataObject, char *name, double *val, int length)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname;
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);
	jdoubleArray ja;

	jname = (*env)->NewStringUTF(env,name);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	ja = (*env)->NewDoubleArray(env,length);
	(*env)->SetDoubleArrayRegion(env,ja,0,length,val);
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetDoubleArrayMethod,jname,ja);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int set2DDoubleArray(NamedDataRef *dataObject, char *name, double **val, int length1, int length2)
{
	int i;
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname;
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);
	jobjectArray res;

	res = (*env)->NewObjectArray(env,length1,ndp->doubleArrayClass,NULL);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	jname = (*env)->NewStringUTF(env,name);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	for(i=0;i<length1;i++) {
		jdoubleArray ja = (*env)->NewDoubleArray(env,length2);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		(*env)->SetDoubleArrayRegion(env,ja,0,length2,val[i]);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		(*env)->SetObjectArrayElement(env,res,i,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	}
	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetDoubleArray2DMethod,jname,res);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

//int setDoubleArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, double *val, int length){}

int setString(NamedDataRef *dataObject, char *name, char *val)
{
	jobject obj = dataObject->obj;
	JNIEnv *env = dataObject->env;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	
	if(ndp != NULL) {
		jstring jval = (*env)->NewStringUTF(env,val);
		(*env)->CallVoidMethod(env,obj,ndp->ndSetStringMethod,jname,jval);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		// print something here.
		return ND_FAILURE;
	}
}

int setStringArray(NamedDataRef *dataObject, char *name, char **val, int length)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname;
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);
	jobjectArray ja;
	int i;

	jname = (*env)->NewStringUTF(env,name);
	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	ja = (*env)->NewObjectArray(env,length,ndp->stringClass,NULL);
	for(i=0;i<length;i++) {
		if(val[i] != NULL) {
			jstring s;
			s = (*env)->NewStringUTF(env,val[i]);
			if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
			(*env)->SetObjectArrayElement(env,ja,i,s);
			if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		}
	}

	if(ndp != NULL) {
		(*env)->CallVoidMethod(env,obj,ndp->ndSetStringArrayMethod,jname,ja);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

int setNamedData(NamedDataRef *dataObject, char *name, NamedDataRef *val)
{
	jobject obj = dataObject->obj;
	JNIEnv *env = dataObject->env;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
	
	if(ndp != NULL) {
		jobject ndpObj = val->obj;
		(*env)->CallVoidMethod(env,obj,ndp->ndSetNamedDataMethod,jname,ndpObj);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		// print something here.
		return ND_FAILURE;
	}
}



int getInt(NamedDataRef *dataObject, char *name, int *val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if(ndp != NULL) {
		*val = (*env)->CallIntMethod(env,obj,ndp->ndGetIntMethod,jname);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

int getIntArray(NamedDataRef *dataObject, char *name, int **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		int *elems,*rv;
		jintArray ja;
		int len;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetIntArrayMethod,jname);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (int *)calloc(len,sizeof(int));
		elems = (*env)->GetIntArrayElements(env,ja,NULL);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		memcpy(rv,elems,len*sizeof(int));
		(*env)->ReleaseIntArrayElements(env,ja,elems,JNI_ABORT);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

int getIntArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, int **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		int *elems,*rv;
		jintArray ja;
		int len;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetIntArrayAtIndexMethod,jname,idx);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (int *)calloc(len,sizeof(int));
		elems = (*env)->GetIntArrayElements(env,ja,NULL);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		memcpy(rv,elems,len*sizeof(int));
		(*env)->ReleaseIntArrayElements(env,ja,elems,JNI_ABORT);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}


int getByte(NamedDataRef *dataObject, char *name, char *val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if(ndp != NULL) {
		*val = (*env)->CallByteMethod(env,obj,ndp->ndGetByteMethod,jname);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

int getByteArray(NamedDataRef *dataObject, char *name, char **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		jbyte *elems,*rv;
		jbyteArray ja;
		int len;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetByteArrayMethod,jname);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (jbyte *)calloc(len,sizeof(jbyte));
		elems = (*env)->GetByteArrayElements(env,ja,NULL);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		memcpy(rv,elems,len*sizeof(jbyte));
		(*env)->ReleaseByteArrayElements(env,ja,elems,JNI_ABORT);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

int getByteArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, char **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		jbyte *elems,*rv;
		jbyteArray ja;
		int len;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetByteArrayAtIndexMethod,jname, idx);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (jbyte *)calloc(len,sizeof(jbyte));
		elems = (*env)->GetByteArrayElements(env,ja,NULL);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		memcpy(rv,elems,len*sizeof(jbyte));
		(*env)->ReleaseByteArrayElements(env,ja,elems,JNI_ABORT);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}


int getFloat(NamedDataRef *dataObject, char *name, float *val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		*val = (*env)->CallFloatMethod(env,obj,ndp->ndGetFloatMethod,jname);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int getFloatArray(NamedDataRef *dataObject, char *name, float **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		float *elems,*rv;
		jfloatArray ja;
		int len;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetFloatArrayMethod,jname);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (float *)calloc(len,sizeof(float));
		elems = (*env)->GetFloatArrayElements(env,ja,NULL);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		memcpy(rv,elems,len*sizeof(float));
		(*env)->ReleaseFloatArrayElements(env,ja,elems,JNI_ABORT);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int getFloatArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, float **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		float *elems,*rv;
		jfloatArray ja;
		int len;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetFloatArrayAtIndexMethod,jname,idx);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (float *)calloc(len,sizeof(float));
		elems = (*env)->GetFloatArrayElements(env,ja,NULL);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		memcpy(rv,elems,len*sizeof(float));
		(*env)->ReleaseFloatArrayElements(env,ja,elems,JNI_ABORT);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

int getDouble(NamedDataRef *dataObject, char *name, double *val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		*val = (*env)->CallDoubleMethod(env,obj,ndp->ndGetDoubleMethod,jname);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

int getDoubleArray(NamedDataRef *dataObject, char *name, double **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		double *elems,*rv;
		jdoubleArray ja;
		int len;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetDoubleArrayMethod,jname);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (double *)calloc(len,sizeof(double));
		elems = (*env)->GetDoubleArrayElements(env,ja,NULL);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		memcpy(rv,elems,len*sizeof(double));
		(*env)->ReleaseDoubleArrayElements(env,ja,elems,JNI_ABORT);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int getDoubleArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, double **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		double *elems,*rv;
		jdoubleArray ja;
		int len;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetDoubleArrayAtIndexMethod,jname,idx);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (double *)calloc(len,sizeof(double));
		elems = (*env)->GetDoubleArrayElements(env,ja,NULL);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		memcpy(rv,elems,len*sizeof(double));
		(*env)->ReleaseDoubleArrayElements(env,ja,elems,JNI_ABORT);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}

int getString(NamedDataRef *dataObject, char *name, char **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		jstring sval ;
		const char *str;
		
		sval = (*env)->CallObjectMethod(env,obj,ndp->ndGetStringMethod,jname);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		str = (*env)->GetStringUTFChars(env, sval, 0);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		*val=calloc(strlen(str)+1,sizeof(char));
		strcpy(*val,str);
		(*env)->ReleaseStringUTFChars(env, sval, str);
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int getStringArray(NamedDataRef *dataObject, char *name, char ***val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		char **rv;
		jobjectArray ja;
		int len,i;
		ja = (*env)->CallObjectMethod(env,obj,ndp->ndGetStringArrayMethod,jname);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		len = (*env)->GetArrayLength(env,ja);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		rv = (char **)calloc(len,sizeof(char *));
		for(i=0;i<len;i++) {
			jstring s = (*env)->GetObjectArrayElement(env,ja,i);
			if(s != NULL) {
				const char *str = (*env)->GetStringUTFChars(env, s, 0);
				char *cs=calloc(strlen(str)+1,sizeof(char));
				strcpy(cs,str);
				(*env)->ReleaseStringUTFChars(env, s, str);
				rv[i] = cs;
			}
		}
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

		*val = rv;
		return ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
int getNamedData(NamedDataRef *dataObject, char *name, NamedDataRef **val)
{
	JNIEnv *env = dataObject->env;
	jobject obj = dataObject->obj;
	jstring jname = (*env)->NewStringUTF(env,name);
	NamedDataPtr ndp = x_getNamedDataPtr(env,obj);

	if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;

	if(ndp != NULL) {
		jobject oval;
		
		oval = (*env)->CallObjectMethod(env,obj,ndp->ndGetNamedDataMethod,jname);
		if((*env)->ExceptionOccurred(env)!= NULL) return ND_FAILURE;
		*val = (NamedDataRef *)calloc(1,sizeof(NamedDataRef));
		(*val)->obj = oval;
		(*val)->env = env;
		return ((*env)->ExceptionOccurred(env)!= NULL) ? ND_FAILURE : ND_SUCCESS;
	} else {
		return ND_FAILURE;
	}
}
