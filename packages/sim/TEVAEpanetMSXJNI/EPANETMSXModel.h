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

typedef struct EPANETMSXModelData {
	jclass epanetmsxModelClass;

	jclass epanetmsxExceptionClass;


	jfieldID errCodeField;
	jfieldID errStringField;
	jfieldID doubleBufferField;
	jfieldID doubleBufferLengthField;
} EPANETMSXModelData, *EPANETMSXModelDataPtr;


#ifndef _Included_anl_epanet_model_EPANETMSXModel
#define _Included_anl_epanet_model_EPANETMSXModel
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxOpen
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxOpen
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxUseHydFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxUseHydFile
  (JNIEnv *, jobject, jstring);

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxInit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxInit
  (JNIEnv *, jobject, jint);

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxSetSources
 * Signature: ([[F)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxSetSources
  (JNIEnv *, jobject, jobjectArray);

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxStep
 * Signature: ([J)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxStep
  (JNIEnv *, jobject, jlongArray);

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxGetInitQual
 * Signature: (II[DJI)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxGetInitQual
  (JNIEnv *, jobject, jint, jint, jdoubleArray, jlong, jint);

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxRestoreInitQual
 * Signature: (II[DJI)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxRestoreInitQual
  (JNIEnv *, jobject, jint, jint, jdoubleArray, jlong, jint);

/*
 * Class:     anl_epanet_model_EPANETMSXModelInterface
 * Method:    msxInitializeSources
 * Signature: ([Lanl/epanet/msx/MSXSource;)V
 */
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETMSXModelInterface_msxInitializeSources
  (JNIEnv *, jobject, jobjectArray);

#ifdef __cplusplus
}
#endif
#endif
