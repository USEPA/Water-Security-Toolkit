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
#include <limits.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>

#include "InfstrImpacts.h"
#include "ExternalAnalysis.h"
#include "AnalysisModuleUtils.h"
#include "NamedData.h"
#include "loggingUtils.h"

PII_Data x_iid;
int x_nscenarios;

/* 
 * called once after the first simulaiton has been completed (so demands are there)
 */
int IIAnalysis_ia_initialize(NamedDataRef *analysisOptionsRef, NamedDataRef *simResultsRef)
{
	// at this point, simResultsRef does not conatin any quality, but will contain
	// demands however
	x_iid = loadIIAOptions(analysisOptionsRef, simResultsRef,IntermediateAnalysis);
	x_nscenarios=0;
	if(x_iid==NULL) return 0;
	return 1;
}
/* 
 * called once after every simulation is done to perform the intermediate analysis.  The
 * data placed in the intResultsRef will be passed along to the aggregation server
 */
void IIAnalysis_ia_newSimResults(NamedDataRef *analysisOptionsRef,NamedDataRef *simResultsRef, NamedDataRef *intResultsRef)
{
//	PIIAResults iiares;
	JNIEnv *env = analysisOptionsRef->env;
	int speciesIndex;
	char *simID;
	int nSpecies=x_iid->net->numSpecies;
	int nnodes=x_iid->net->numNodes;
	int nlinks=x_iid->net->numLinks;
	int nsteps=x_iid->net->numSteps;
	int nInj;
	float **pfotd;
	int **lotd;
	float *pf,*diams;
	int i,j;

	if(getInt(analysisOptionsRef, "SpeciesIndex", &speciesIndex)==ND_FAILURE) return;
	if(getString(simResultsRef,"injectionID",&simID)==ND_FAILURE) return;

	x_nscenarios++;
//	iiares=allocateIIAResultsData(simID,x_iid);
//	memset(&iiares,0,sizeof(IIAResults));

	loadQuality(simResultsRef,x_iid->net, x_iid->nodeinfo,x_iid->speciesIndex);
	loadFlow(simResultsRef,x_iid->net);
	resetPipeFeetOverThreshold(x_iid);
	pipesContaminated(x_iid);

//	for(i=0;i<x_iid->pot->numThresh;i++) {
//		iiares->pfot[i]=x_iid->pot->feetOver[i];
//	}

	if(addVariable(intResultsRef,"SimID",  STRING_TYPE)==ND_FAILURE) return;
	if(setString(intResultsRef,"SimID", simID )==ND_FAILURE) return;
	nd_free(simID);

	if(addVariable(intResultsRef,"InjDef",  STRING_TYPE)==ND_FAILURE) return;
	if(getString(simResultsRef,"injectionDef",&simID)==ND_FAILURE) return;
	if(setString(intResultsRef,"InjDef", simID )==ND_FAILURE) return;
	nd_free(simID);

	if(addVariable(intResultsRef,"SimulationID",  STRING_TYPE)==ND_FAILURE) return;
	if(getString(simResultsRef,"injectionID",&simID)==ND_FAILURE) return;
	if(setString(intResultsRef,"SimulationID", simID )==ND_FAILURE) return;
	nd_free(simID);

	if(addVariable(intResultsRef,"numInjections",  INT_TYPE)==ND_FAILURE) return;
	if(getInt(simResultsRef,"numInjections",&nInj)==ND_FAILURE) return;
	if(setInt(intResultsRef,"numInjections", nInj )==ND_FAILURE) return;

	// doses over threshold...
	if(addVariable(intResultsRef,"NumThresholds",  INT_TYPE)==ND_FAILURE) return;
	if(setInt(intResultsRef,"NumThresholds", x_iid->pot->numThresh )==ND_FAILURE) return;

	if(addVariable(intResultsRef,"TotPipeFeetOverThreshold", FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	if(setFloatArray(intResultsRef,"TotPipeFeetOverThreshold", x_iid->pot->potd.feetOver,x_iid->pot->numThresh)==ND_FAILURE) return;

	if(addVariable(intResultsRef,"LinksOverThreshold", INT_ARRAY_TYPE )==ND_FAILURE) return;
	if(setIntArray(intResultsRef,"LinksOverThreshold", x_iid->pot->potd.linksOver,x_iid->pot->numThresh)==ND_FAILURE) return;

	if(addVariable(intResultsRef,"PipeFeetOverThresholds",     FLOAT_ARRAY_2D_TYPE)==ND_FAILURE) return;
	if(set2DFloatArray(intResultsRef,"PipeFeetOverThresholds",    x_iid->pot->pipeFeetOver,x_iid->pot->numThresh,nlinks)==ND_FAILURE) return;

	pf = (float *)calloc(x_iid->pot->numDiameters,sizeof(float));
	diams=(float *)calloc(x_iid->pot->numDiameters,sizeof(float));
	for(j=0;j<x_iid->pot->numDiameters;j++) {
		pf[j]=x_iid->pot->potdDiam[j]->potd.totalFeet;
		diams[j]=x_iid->pot->potdDiam[j]->diameter;
	}
	pfotd=(float **)calloc(x_iid->pot->numThresh,sizeof(float*));
	lotd=(int **)calloc(x_iid->pot->numThresh,sizeof(int*));
	for(i=0;i<x_iid->pot->numThresh;i++) {
		pfotd[i]=(float *)calloc(x_iid->pot->numDiameters,sizeof(float));
		lotd[i]=(int *)calloc(x_iid->pot->numDiameters,sizeof(int));
		for(j=0;j<x_iid->pot->numDiameters;j++) {
			pfotd[i][j]=x_iid->pot->potdDiam[j]->potd.feetOver[i];
			lotd[i][j]=x_iid->pot->potdDiam[j]->potd.linksOver[i];
		}
	}
	if(addVariable(intResultsRef,"TotPipeFeetOverThresholdByDiameter",     FLOAT_ARRAY_2D_TYPE)==ND_FAILURE) return;
	if(set2DFloatArray(intResultsRef,"TotPipeFeetOverThresholdByDiameter",    pfotd,x_iid->pot->numThresh,x_iid->pot->numDiameters)==ND_FAILURE) return;

	if(addVariable(intResultsRef,"LinksOverThresholdByDiameter",     INT_ARRAY_2D_TYPE)==ND_FAILURE) return;
	if(set2DIntArray(intResultsRef,"LinksOverThresholdByDiameter",    lotd,x_iid->pot->numThresh,x_iid->pot->numDiameters)==ND_FAILURE) return;

	if(addVariable(intResultsRef,"NumDiameters",     INT_TYPE)==ND_FAILURE) return;
	if(setInt(intResultsRef,"NumDiameters", x_iid->pot->numDiameters)==ND_FAILURE) return;
	if(addVariable(intResultsRef,"TotalPipeFeet",     FLOAT_TYPE)==ND_FAILURE) return;
	if(setFloat(intResultsRef,"TotalPipeFeet", x_iid->pot->potd.totalFeet)==ND_FAILURE) return;
	if(addVariable(intResultsRef,"TotalPipeFeetByDiameter",     FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	if(setFloatArray(intResultsRef,"TotalPipeFeetByDiameter", pf,x_iid->pot->numDiameters)==ND_FAILURE) return;
	if(addVariable(intResultsRef,"PipeDiameters",     FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	if(setFloatArray(intResultsRef,"PipeDiameters", diams,x_iid->pot->numDiameters)==ND_FAILURE) return;

	free(pf);
	free(diams);
	for(i=0;i<x_iid->pot->numThresh;i++) {
		free(pfotd[i]);
		free(lotd[i]);
	}
	free(pfotd);
	free(lotd);
}

/* 
 * called once after the scenario has been completed
 */
void IIAnalysis_ia_shutdown(NamedDataRef *analysisOptionsRef)
{
	freeIIAMemory(x_iid,IntermediateAnalysis);
}
