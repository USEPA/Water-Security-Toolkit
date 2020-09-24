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

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <float.h>

extern "C" {
#include "loggingUtils.h"
#include "NamedData.h"
#include "ExternalAnalysis.h"
#include "HelloAnalysis.h"


static PMem x_mem;
static int x_scenarioCount;
static int* x_totalNonzero;
static double* x_aveNonzero;
static char** x_nodeID;


static JNIEnv *x_env;

static int x_nSims;


/* 
 * called once after the first simulaiton has been completed (so demands are there)
 */
void HelloAnalysis_aggr_initialize(NamedDataRef *analysisOptionsRef, 
				   NamedDataRef *simResultsRef)
{
	// at this point, simResultsRef does not conatin any quality, but will contain
	// demands however
	int nnodes;
	int nsteps;
	x_nSims = 0;
	x_mem = loadOptions(analysisOptionsRef, simResultsRef,Aggregation);
	// x_mem->net,x_mem->dr,x_mem->node

	nnodes = x_mem->net->nnodes;
	nsteps = x_mem->net->nsteps;
	x_totalNonzero = (int*) calloc(nnodes,sizeof(int));
	x_nodeID       = (char**) calloc(nnodes,sizeof(char*));
	for (int i=0; i<nnodes; i++) {
		x_nodeID[i] = x_mem->node[i].id;
		x_totalNonzero[i] = 0;
	}
	x_scenarioCount=0;
}

/* 
 * called once after every simulation with the results of the intermediate analysis
 */
void HelloAnalysis_aggr_newResults(NamedDataRef *analysisOptionsRef,NamedDataRef *intResultsRef)
{
	int i;
	int nnodes = x_mem->net->nnodes;
	int nsteps = x_mem->net->nsteps;
	int nActualSteps;
	char *simID;
	double rank;
	JNIEnv *env = analysisOptionsRef->env;
	x_env=env;

	x_nSims++;
	if (0) ANL_UTIL_LogSevere(x_env,"anl.teva", "test error!");

	char msg[256];
	sprintf(msg,"Aggregation: newResults: \n");
	if (0) ANL_UTIL_LogInfo(env,"teva.analysis.server",msg);
		
	int *numNonzero;
	if(getIntArray(intResultsRef,"NumNonzeroConcens",
			       &numNonzero)==ND_FAILURE)
			return;
	for (int i=0; i<nnodes; i++) {
		x_totalNonzero[i] += numNonzero[i];
	}
	free(numNonzero);
}


/* 
 * called usually only once after the scenario is done to obtain the completed analysis results
 */
void HelloAnalysis_aggr_getResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef)
{
	int i;
	int nnodes = x_mem->net->nnodes;
	int nsteps = x_mem->net->nsteps;
	if(addVariable(resultsRef,"nodeID",
		       STRING_ARRAY_TYPE)==ND_FAILURE) 
		return;
	if(addVariable(resultsRef,"averageNonzero",
		       DOUBLE_ARRAY_TYPE)==ND_FAILURE) 
		return;

	x_aveNonzero = (double*) calloc(nnodes,sizeof(double));
	for (int i=0; i<nnodes; i++) {
		x_aveNonzero[i] = x_totalNonzero[i]/(double)x_nSims;
		printf("tnz[%d]: %d, nsims: %d\n",i,x_totalNonzero[i], x_nSims);
	}
	if(setDoubleArray(resultsRef,"averageNonzero", 
		       x_aveNonzero,nnodes)==ND_FAILURE) 
		return;
	if(setStringArray(resultsRef,"nodeID", 
		       x_nodeID,nnodes)==ND_FAILURE) 
		return;
}

/* 
 * called usually only once after the scenario is done to obtain the completed analysis results
 */
void HelloAnalysis_aggr_writeResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
	FILE *fp;
	char fname[256];

	int nnodes = x_mem->net->nnodes;
	int nsteps = x_mem->net->nsteps;
	double *averageNonzero; // why retrieve from resultsRef when we have it?
	if(getDoubleArray(resultsRef,"averageNonzero", 
		       &averageNonzero)==ND_FAILURE) {
	}

	sprintf(fn,"%s/Hello.txt",dir);
	if ((fp = fopen(fn,"w"))==NULL)
	     printf("HelloAnalysis_aggr_writeResults: failed to open file\n");	
	
	fprintf(fp,"%d\n",nnodes);
	fprintf(fp,"%d\n",nsteps);
	for (int i=0; i<nnodes; i++) {
		fprintf(fp, "%f\n", averageNonzero[i]);
	}
	fprintf(fp,"\n");

	fclose(fp);
}

/* 
 * called once after the scenario has been completed
 */
void HelloAnalysis_aggr_shutdown(NamedDataRef *analysisOptionsRef)
{
	int nnodes = x_mem->net->nnodes;
	int nsteps = x_mem->net->nsteps;
	char msg[256];

	JNIEnv *env = analysisOptionsRef->env;

	sprintf(msg,"There were %d simulations", x_nSims);
	ANL_UTIL_LogInfo(env,"teva.analysis.server",msg);

	// should be last in case any others need data from x_mem
	freeMemory(x_mem,Aggregation);
	free(x_aveNonzero);
}
};
