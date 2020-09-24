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
#include <jni.h>
/* Header for class anl_epanet_model_EPANETModel */

typedef struct EPANETModelData {
	jclass epanetModelClass;

	jclass epanetExceptionClass;

	jclass enLinkNodesClass;
	jmethodID enLinkNodesConstructor;

	jfieldID errCodeField;
	jfieldID errStringField;
	jfieldID floatBufferField;
	jfieldID floatBufferLengthField;
	jfieldID intBufferField;
	jfieldID intBufferLengthField;
} EPANETModelData, *EPANETModelDataPtr;


#ifndef _Included_anl_epanet_model_EPANETModel
#define _Included_anl_epanet_model_EPANETModel
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enEpanet
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/reflect/Method;)V
 */
JNIEXPORT void JNICALL Java_enEpanet
  (JNIEnv *, jobject, jstring, jstring, jstring, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enOpen
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enOpen
  (JNIEnv *, jobject, jstring, jstring, jstring);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSaveinpfile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enSaveInpfile
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enClose
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSolveH
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enSolveH
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSaveH
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enSaveH
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enOpenH
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enOpenH
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enInitH
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_enInitH
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enRunH
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enRunH
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enNextH
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enNextH
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enCloseH
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enCloseH
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSavehydfile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enSaveHydFile
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enUsehydfile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enUseHydFile
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSolveQ
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enSolveQ
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enOpenQ
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enOpenQ
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enInitQ
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_enInitQ
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enRunQ
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enRunQ
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enNextQ
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enNextQ
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enStepQ
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_enStepQ
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enCloseQ
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enCloseQ
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enReport
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enReport
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enResetreport
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_enResetReport
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetreport
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enSetReport
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetcontrol
 * Signature: (I)Lanl/epanet/model/ENControl;
 */
JNIEXPORT jobject JNICALL Java_enGetControl
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetcount
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_enGetCount
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetoption
 * Signature: (I)F
 */
JNIEXPORT jfloat JNICALL Java_enGetOption
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGettimeparam
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_enGetTimeParam
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetflowunits
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_enGetFlowUnits
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetpatternindex
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_enGetPatternIndex
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetpatternid
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_enGetPatternID
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetpatternlen
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_enGetPatternLen
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetpatternvalue
 * Signature: (II)F
 */
JNIEXPORT jfloat JNICALL Java_enGetPatternValue
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetqualtype
 * Signature: ()Lanl/epanet/model/ENQuality;
 */
JNIEXPORT jobject JNICALL Java_enGetQualType
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGeterror
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_enGetError
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetnodeindex
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_enGetNodeIndex
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetnodeid
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_enGetNodeID
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetnodetype
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_enGetNodeType
  (JNIEnv *, jobject, jint);
JNIEXPORT void JNICALL Java_enGetNodeTypes
  (JNIEnv *env, jobject model, jintArray jtypes, jlong buf, jint bufsize);
/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetnodevalue
 * Signature: (II)F
 */
JNIEXPORT jfloat JNICALL Java_enGetNodeValue
  (JNIEnv *, jobject, jint, jint);
/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetNodeValue
 * Signature: (I[F)V
 */
JNIEXPORT void JNICALL Java_enGetNodeValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray jvals, jint startIdx, jint len, jlong buf, jint bufsize);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetlinkindex
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_enGetLinkIndex
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetlinkid
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_enGetLinkID
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetlinktype
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_enGetLinkType
  (JNIEnv *, jobject, jint);

JNIEXPORT void JNICALL Java_enGetLinkTypes
  (JNIEnv *env, jobject model, jintArray jtypes, jlong buf, jint bufsize);
/*
 * Class:     anl_epanet_model_EPANETModelInterface
 * Method:    enGetLinkType
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_enGetLinkTypes
  (JNIEnv *, jobject, jintArray, jlong, jint);
/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetlinknodes
 * Signature: (I)Lanl/epanet/model/ENLinkNodes;
 */
JNIEXPORT jobject JNICALL Java_enGetLinkNodes
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetlinkvalue
 * Signature: (II)F
 */
JNIEXPORT jfloat JNICALL Java_enGetLinkValue
  (JNIEnv *, jobject, jint, jint);
/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetlinkvalues
 * Signature: (I[F)V
 */
JNIEXPORT void JNICALL Java_enGetLinkValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray jvals, jint startIdx, jint len, jlong buf, jint bufsize);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enGetversion
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_enGetVersion
  (JNIEnv *, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetcontrol
 * Signature: (ILanl/epanet/model/ENControl;)V
 */
JNIEXPORT void JNICALL Java_enSetControl
  (JNIEnv *, jobject, jint, jobject);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetnodevalue
 * Signature: (IIF)V
 */
JNIEXPORT void JNICALL Java_enSetNodeValue
  (JNIEnv *, jobject, jint, jint, jfloat);

JNIEXPORT void JNICALL Java_enSetNodeValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray values, jlong buf, jint bufsize);

JNIEXPORT void JNICALL Java_enSetConstantNodeValues
  (JNIEnv *env, jobject model,jint paramCode,jfloat constantVal, jint num);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetlinkvalue
 * Signature: (IIF)V
 */
JNIEXPORT void JNICALL Java_enSetLinkValue
  (JNIEnv *, jobject, jint, jint, jfloat);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetpattern
 * Signature: (I[F)V
 */
JNIEXPORT void JNICALL Java_enSetPattern
  (JNIEnv *, jobject, jint, jfloatArray);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetpatternvalue
 * Signature: (IIF)V
 */
JNIEXPORT void JNICALL Java_enSetPatternValue
  (JNIEnv *, jobject, jint, jint, jfloat);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSettimeparam
 * Signature: (IJ)V
 */
JNIEXPORT void JNICALL Java_enSetTimeParam
  (JNIEnv *, jobject, jint, jlong);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetoption
 * Signature: (IF)V
 */
JNIEXPORT void JNICALL Java_enSetOption
  (JNIEnv *, jobject, jint, jfloat);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetstatusreport
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_enSetStatusReport
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETModel
 * Method:    enSetqualtype
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_enSetQualType
  (JNIEnv *, jobject, jint, jstring, jstring, jstring);

#ifdef __cplusplus
}
#endif
#endif
