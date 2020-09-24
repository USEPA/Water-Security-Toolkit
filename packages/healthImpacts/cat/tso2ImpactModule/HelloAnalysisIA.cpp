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

#include <limits.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" {
#include "ExternalAnalysis.h"
#include "NamedData.h"

#include "HelloAnalysis.h"

static Ptso2ImpactMem x_mem;
static int x_nscenarios;
static int x_Hello;

/* 
 * called once after the first simulaiton has been completed 
 * (so demands are there)
 */
int HelloAnalysis_ia_initialize(NamedDataRef *analysisOptionsRef, 
				NamedDataRef *simResultsRef)
{
	// at this point, simResultsRef does not conatin any quality, 
	// but will contain demands 
	x_mem = loadOptions(analysisOptionsRef, simResultsRef,
			    IntermediateAnalysis);
	x_nscenarios=0;
	x_Hello = x_mem->hello;

	return NON_REMOTE;	// pass results back by value
}

/* 
 * called once after every simulation is done to perform the 
 * intermediate analysis.  The data placed in the intResultsRef will 
 * be passed along to the aggregation server
 */
void HelloAnalysis_ia_newSimResults(NamedDataRef *analysisOptionsRef,
				    NamedDataRef *simResultsRef, 
				    NamedDataRef *intResultsRef)
{
	int i,j, nsteps, totsteps, nnodes;
	float maxConc=-1;

	char *simID;
	float **conc;
	JNIEnv *env = analysisOptionsRef->env;

	x_nscenarios++;
	nsteps=x_mem->net->nsteps;
	nnodes=x_mem->net->nnodes;

	int *numNonzero= (int*)calloc(nnodes,sizeof(int));
	for(i=0;i<nnodes;i++) {
		numNonzero[i] = 0;
	}
	for(j=0;j<nnodes;j++) {
		x_mem->node[j].nz[0] = 0;
	}
	for(i=0;i<nsteps;i++) {
		float *tres;
		if(getFloatArrayAtIndex(simResultsRef,"quality",
					i,&tres)==ND_FAILURE) {
			return;
		}
		for(j=0;j<nnodes;j++) {
			float tc = tres[j];
			if(tc > 0) {
				x_mem->node[j].nz[0]++;
				numNonzero[j]++;
			}
			x_mem->node[j].c[0][i] = tc;
		}
		nd_free(tres);
	}

	if(getString(simResultsRef,"injectionID",&simID)==ND_FAILURE) return;

	if(addVariable(intResultsRef,"NumNonzeroConcens",
		       INT_ARRAY_TYPE)==ND_FAILURE) 
		return;
	
	if (setIntArray(intResultsRef,"NumNonzeroConcens",
			numNonzero,nnodes)==ND_FAILURE)
		return;
	nd_free(simID);
}

/* 
 * called once after the scenario has been completed
 */
void HelloAnalysis_ia_shutdown(NamedDataRef *analysisOptionsRef)
{
	freeMemory(x_mem,IntermediateAnalysis);
}
};
