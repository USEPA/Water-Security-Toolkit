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
#include <string.h>		// for memset
#include "jniUtils.h"
#include "AggregationServer.h"
#include "ExternalAnalysis.h"

/*
 * Class:     anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl
 * Method:    getResults
 * Signature: (Lanl/teva/analysis/module/AnalysisResults;)V
 */
JNIEXPORT void JNICALL Java_anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl_getResults
  (JNIEnv *env, jobject obj, jlong functions, jobject jAnalysisOptions, jobject jResults)
{
	FunctionsPtr fp = PTRCAST(FunctionsPtr)functions;
	
	NamedDataRef analysisOptionsRef;
	NamedDataRef resultsRef;

	analysisOptionsRef.env = env;
	analysisOptionsRef.obj = jAnalysisOptions;

	resultsRef.env = env;
	resultsRef.obj = jResults;

	fp->aggr_getResults(&analysisOptionsRef,&resultsRef);
}

/*
 * Class:     anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl
 * Method:    initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl_init
  (JNIEnv *env, jobject obj, jlong functions, jobject jAnalysisOptions, jobject jSimResults)
{
	FunctionsPtr fp = PTRCAST(FunctionsPtr)functions;

	NamedDataRef analysisOptionsRef;
	NamedDataRef simResultsRef;

	analysisOptionsRef.env = env;
	analysisOptionsRef.obj = jAnalysisOptions;

	simResultsRef.env = env;
	simResultsRef.obj = jSimResults;

	fp->aggr_initialize(&analysisOptionsRef,&simResultsRef);
}

/*
 * Class:     anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl
 * Method:    shutdown
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl_shutdown
  (JNIEnv *env, jobject obj, jlong functions, jobject jAnalysisOptions)
{
	FunctionsPtr fp = PTRCAST(FunctionsPtr)functions;

	NamedDataRef analysisOptionsRef;
	analysisOptionsRef.env = env;
	analysisOptionsRef.obj = jAnalysisOptions;

	fp->aggr_shutdown(&analysisOptionsRef);
}

/*
 * Class:     anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl
 * Method:    newResults
 * Signature: (Lanl/teva/analysis/module/NamedData;)V
 */
JNIEXPORT void JNICALL Java_anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl_newResults
  (JNIEnv *env, jobject obj, jlong functions, jobject jAnalysisOptions, jobject jIntResults)
{
	FunctionsPtr fp = PTRCAST(FunctionsPtr)functions;

	NamedDataRef analysisOptionsRef;
	NamedDataRef intResultsRef;

	analysisOptionsRef.env = env;
	analysisOptionsRef.obj = jAnalysisOptions;

	intResultsRef.env = env;
	intResultsRef.obj = jIntResults;

	fp->aggr_newResults(&analysisOptionsRef,&intResultsRef);
}

/*
 * Class:     anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl
 * Method:    newResults
 * Signature: (Lanl/teva/analysis/module/NamedData;)V
 */
JNIEXPORT jstring JNICALL Java_anl_teva_analysis_module_ExternalAnalysisAggregationServerImpl_writeResults
  (JNIEnv *env, jobject obj, jlong functions, jobject jAnalysisOptions, jobject jResults, jstring jDir)
{
	FunctionsPtr fp = (FunctionsPtr)functions;
	char fn[16384],*dir;

	NamedDataRef analysisOptionsRef;
	NamedDataRef resultsRef;

	memset(&fn,0,16384);
	analysisOptionsRef.env = env;
	analysisOptionsRef.obj = jAnalysisOptions;

	resultsRef.env = env;
	resultsRef.obj = jResults;

	dir = ANL_UTIL_toString(env,jDir);
	if(fp->aggr_writeResults!=NULL)
		fp->aggr_writeResults(&analysisOptionsRef,&resultsRef,dir,fn);

	ANL_UTIL_free(dir);

	return (*env)->NewStringUTF(env,fn);
}

//
//#ifdef __cplusplus
//}
//#endif
