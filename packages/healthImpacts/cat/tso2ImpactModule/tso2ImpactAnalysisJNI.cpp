/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top SPOT directory.
 *  _________________________________________________________________________
 */

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>

#include "tso2ImpactAnalysis.h"
extern "C" {
#include "ExternalAnalysis.h"
#include "jniUtils.h"


/*
 * Class:     Java_anl_teva_analysis_modules_tso2ImpactA
 * Method:    getMethods
 * Signature: (V)I
 */
JNIEXPORT jlong JNICALL Java_anl_teva_analysis_modules_tso2ImpactModule_getMethodPointers
  (JNIEnv *env, jobject obj)
{
	printf("getMethodPointers\n");
	fflush(stdout);
	FunctionsPtr f = (FunctionsPtr)calloc(1,sizeof(Functions));
	f->aggr_initialize   = tso2ImpactAnalysis_aggr_initialize;
	f->aggr_newResults   = tso2ImpactAnalysis_aggr_newResults;
	f->aggr_getResults   = tso2ImpactAnalysis_aggr_getResults;
	f->aggr_writeResults = tso2ImpactAnalysis_aggr_writeResults;
	f->aggr_shutdown     = tso2ImpactAnalysis_aggr_shutdown;

	f->ia_initialize    = tso2ImpactAnalysis_ia_initialize;
	f->ia_newSimResults = tso2ImpactAnalysis_ia_newSimResults;
	f->ia_shutdown      = tso2ImpactAnalysis_ia_shutdown;

	return PTRTOJLONG(f);
}
/*
 * Class:     Java_anl_teva_analysis_modules_tso2ImpactA
 * Method:    cleanup
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_anl_teva_analysis_modules_tso2ImpactModule_cleanup
  (JNIEnv *env, jobject obj, jlong functions)
{
	free(PTRCAST(void *)functions);
}

};
