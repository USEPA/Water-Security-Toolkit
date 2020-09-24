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

#include <stdlib.h>
#include <jni.h>
#include "jniUtils.h"
#include "ExternalAnalysis.h"
#include "HealthImpacts.h"

/*
 * Class:     Java_anl_teva_analysis_modules_HIA
 * Method:    getMethods
 * Signature: (V)I
 */
JNIEXPORT jlong JNICALL Java_anl_teva_analysis_modules_HIAModule_getMethodPointers
  (JNIEnv *env, jobject obj)
{
	// DevPartner identifies this as a memory leak because the
	// variable f is going out of scope without being initialized.
	// this is only because the JVM is not instrumented and it does not
	// know it is being assigned upon the return from this method.
	// This is not a memory leak because the pointer value is held by the
	// HIAnalysis class and is the memory is freed up via a call to cleanup().

	FunctionsPtr f = (FunctionsPtr)calloc(1,sizeof(Functions));
	f->aggr_initialize   = HIAnalysis_aggr_initialize;
	f->aggr_newResults   = HIAnalysis_aggr_newResults;
	f->aggr_getResults   = HIAnalysis_aggr_getResults;
	f->aggr_writeResults = HIAnalysis_aggr_writeResults;
	f->aggr_shutdown     = HIAnalysis_aggr_shutdown;

	f->ia_initialize    = HIAnalysis_ia_initialize;
	f->ia_newSimResults = HIAnalysis_ia_newSimResults;
	f->ia_shutdown      = HIAnalysis_ia_shutdown;

	return PTRTOJLONG(f);
}
/*
 * Class:     Java_anl_teva_analysis_modules_HIA
 * Method:    cleanup
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_anl_teva_analysis_modules_HIAModule_cleanup
  (JNIEnv *env, jobject obj, jlong functions)
{
	free(PTRCAST(void *)functions);
}
