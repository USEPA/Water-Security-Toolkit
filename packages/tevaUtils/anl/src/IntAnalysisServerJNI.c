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
#include "jniUtils.h"
#include "IntAnalysisServer.h"
#include "ExternalAnalysis.h"
/*
 * Class:     anl_teva_analysis_module_ExternalIntermediateAnalysisServerImpl
 * Method:    init
 * Signature: (Lanl/teva/analysis/module/NamedDataServer;)V
 */
JNIEXPORT jboolean JNICALL Java_anl_teva_analysis_module_ExternalIntermediateAnalysisServerImpl_init
  (JNIEnv *env, jobject obj, jlong functions, jobject jAnalysisOptions, jobject jSimResults)
{
	FunctionsPtr fp = PTRCAST(FunctionsPtr)functions;

	NamedDataRef analysisOptionsRef;
	NamedDataRef simResultsRef;

	analysisOptionsRef.env = env;
	analysisOptionsRef.obj = jAnalysisOptions;

	simResultsRef.env = env;
	simResultsRef.obj = jSimResults;

	return fp->ia_initialize(&analysisOptionsRef,&simResultsRef)==1;
}

/*
 * Class:     anl_teva_analysis_module_ExternalIntermediateAnalysisServerImpl
 * Method:    shutdown
 * Signature: (Lanl/teva/analysis/module/NamedDataServer;)V
 */
JNIEXPORT void JNICALL Java_anl_teva_analysis_module_ExternalIntermediateAnalysisServerImpl_shutdown
  (JNIEnv *env, jobject obj, jlong functions, jobject jAnalysisOptions)
{
	FunctionsPtr fp = PTRCAST(FunctionsPtr)functions;

	NamedDataRef analysisOptionsRef;
	analysisOptionsRef.env = env;
	analysisOptionsRef.obj = jAnalysisOptions;

	fp->ia_shutdown(&analysisOptionsRef);
}

/*
 * Class:     anl_teva_analysis_module_ExternalIntermediateAnalysisServerImpl
 * Method:    newSimulationResults
 * Signature: (Lanl/teva/analysis/module/NamedDataServer;Lanl/teva/analysis/module/NamedDataServer;Lanl/teva/analysis/module/NamedData;)V
 */
JNIEXPORT void JNICALL Java_anl_teva_analysis_module_ExternalIntermediateAnalysisServerImpl_newSimulationResults
  (JNIEnv *env, jobject obj, jlong functions, jobject jAnalysisOptions, jobject jSimResults, jobject jIntermediateResults)
{
	FunctionsPtr fp = PTRCAST(FunctionsPtr)functions;

	NamedDataRef analysisOptionsRef;
	NamedDataRef simResultsRef;
	NamedDataRef intResultsRef;

	analysisOptionsRef.env = env;
	analysisOptionsRef.obj = jAnalysisOptions;

	simResultsRef.env = env;
	simResultsRef.obj = jSimResults;

	intResultsRef.env = env;
	intResultsRef.obj = jIntermediateResults;

	fp->ia_newSimResults(&analysisOptionsRef,&simResultsRef, &intResultsRef);
}


#ifdef __cplusplus
}
#endif
