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
#include "HealthImpacts.h"
#include "ExternalAnalysis.h"
#include "NamedData.h"
#include "runif_if.h"
#include "loggingUtils.h"

static PMem x_mem;
static int x_nscenarios;
static int x_numConcToSave;
static int x_numDosageToSave;
static XA x_na;
//static XA x_interimXA;
//#define WRITE_SIDF


void writeFileData(char *nodeID,FILE *fp,float *data, float pop,int totsteps);
int sortFloatDesc(const void *a, const void *b);



/* 
 * called once after the first simulaiton has been completed (so demands are there)
 */
int HIAnalysis_ia_initialize(NamedDataRef *analysisOptionsRef, NamedDataRef *simResultsRef)
{
	// at this point, simResultsRef does not conatin any quality, but will contain
	// demands however
	JNIEnv *env = analysisOptionsRef->env;
	char msg[80];
	time_t ctime;
	time( &ctime );
	set_seed(ctime);
	sprintf(msg,"Seed: %ld\n",ctime);
	ANL_UTIL_LogInfo(env,"teva.analysis.server",msg);
	x_mem = loadHIAOptions(analysisOptionsRef, simResultsRef,IntermediateAnalysis);
	if(x_mem==NULL) return 0;
	x_nscenarios=0;
	Population(x_mem->net,x_mem->popData,x_mem->node);

    initializeIngestionData(x_mem->ingestionData,x_mem->net,x_mem->node,x_mem->dr,x_mem->popData);
	dumpHIAInputParameters(x_mem);
	if(getInt(analysisOptionsRef, "NumWorstConcentrationToKeep", &x_numConcToSave)==ND_FAILURE) return 0;
	if(getInt(analysisOptionsRef, "NumWorstDosageToKeep", &x_numDosageToSave)==ND_FAILURE) return 0;
	initXA(&x_na,x_mem->net->info->numNodes);
	return x_numConcToSave>0 || x_numDosageToSave>0;
}
/* 
 * called once after every simulation is done to perform the intermediate analysis.  The
 * data placed in the intResultsRef will be passed along to the aggregation server
 */
void HIAnalysis_ia_newSimResults(NamedDataRef *analysisOptionsRef,NamedDataRef *simResultsRef, NamedDataRef *intResultsRef)
{
	int i,j,
		dtmin,
		dtime,
		nsteps,
		totsteps,
		nnodes,
		simIDLen,
		mLen;
	float maxConc=0,maxDosage=0,totInfected=0;
	float *dos;
	int nInj;
	char *msg=calloc(256,sizeof(char));

	HIST hist;
	char *simID;
	float **conc=NULL,**dosage=NULL,*avgDose=NULL;
	IngestionDataPtr ing=NULL;
	JNIEnv *env = analysisOptionsRef->env;
#ifdef WRITE_SIDF
	FILE *fp[6];
#endif /* WRITE_SIDF */
	char *dt[6] = {"Dosage","Response","S","I","D","F"};
	float **c;
	int speciesIndex;
	int nSpecies=x_mem->net->info->numSpecies;

	if(getInt(analysisOptionsRef, "SpeciesIndex", &speciesIndex)==ND_FAILURE) return;

	c=x_mem->net->info->qualResults->nodeC[speciesIndex];
	x_nscenarios++;
	memset(&hist,0,sizeof(HIST));
	nsteps=x_mem->net->info->numSteps;
	nnodes=x_mem->net->info->numNodes;

	if(addVariable(intResultsRef,"NodeConcentration",   FLOAT_ARRAY_2D_TYPE)==ND_FAILURE) return;
	if(addVariable(intResultsRef,"RankValueConcentration", DOUBLE_TYPE)==ND_FAILURE) return;
	if(addVariable(intResultsRef,"NodeDosage",   FLOAT_ARRAY_2D_TYPE)==ND_FAILURE) return;
	if(addVariable(intResultsRef,"RankValueDosage", DOUBLE_TYPE)==ND_FAILURE) return;
	totsteps = x_mem->dr!=NULL?x_mem->dr->maxsteps:x_mem->net->info->numSteps;

	if(x_numConcToSave>0) {
		conc = (float**)calloc(nsteps,sizeof(float*));
		for(i=0;i<nsteps;i++) {
			conc[i] = (float*)calloc(nnodes,sizeof(float));
		}
	}
	if(x_numDosageToSave>0) {
		avgDose=(float*)calloc(nsteps,sizeof(float));
		dosage = (float**)calloc(nsteps,sizeof(float*));
		for(i=0;i<nsteps;i++) {
			dosage[i] = (float*)calloc(nnodes,sizeof(float));
		}
	}
	for(j=0;j<nnodes;j++) {
		for(i=0;i<nSpecies;i++) {
			x_mem->node[j].info->nz[i] = 0;
		}
	}
	for(i=0;i<nsteps;i++) {
		float *tres;
		/** IMPORTANT!!! Need to free these later via call to nd_free */
		if(getFloatArrayAtIndex(simResultsRef,"quality",i,&tres)==ND_FAILURE) {
			return;
		}
		for(j=0;j<nnodes;j++) {
			float tc = tres[j];
		
			if(tc > 0) x_mem->node[j].info->nz[speciesIndex] = 1;
			c[j][i] = tc;
			if(c[j][i] > maxConc)
				maxConc = c[j][i];
//			totalConcentration+=tres[j];
		}
		if(x_numConcToSave>0) {
			memcpy(conc[i],tres,nnodes*sizeof(float));
		}
		nd_free(tres);
	}

	/* Sampling network detection time */
	dtmin=INT_MAX;
    for(i=0;i<x_mem->net->info->numNodes;i++) {
        if((x_mem->node[i].sensor.type==existing || x_mem->node[i].sensor.type==selected) && x_mem->node[i].info->nz[0]) {
			if( (dtime=Detect(x_mem->net,&x_mem->node[i],i,speciesIndex))<dtmin ) dtmin=dtime;
        }
    }

#ifdef WRITE_SIDF
	if(getString(simResultsRef,"injectionID",&simID)==ND_FAILURE) return;
	for(i=0;i<6;i++) {
		char fn[256];
		sprintf(fn,"%s_%s.txt",simID,dt[i]);
		fp[i] = fopen(fn,"w");
	}
	nd_free(simID);
#endif /* WRITE_SIDF */

	resetXA(x_mem->xa,totsteps);
	resetXA(&x_na,nnodes);
    resetDoseOverThreshold(x_mem);
    resetDoseBinData(x_mem);
	/* Time-dependent cumulative expected dose and disease response */
	ing=x_mem->ingestionData;
	
	for(i=0;i<nnodes;i++) {
	  float pop = x_mem->node[i].pop;
#ifdef WRITE_SIDF
	  resetU(x_mem->u,totsteps);
#endif
	  if(x_mem->node[i].info->nz[0] && pop > 0) {
	    Dose(ing->ingestionData[i],ing,&x_mem->node[i], x_mem->net,dtmin,x_mem->u,x_mem->dot,i,x_mem->doseBins,speciesIndex,avgDose);
		if(x_numDosageToSave>0) {
			int t;
			for(t=0;t<nsteps;t++) {
				dosage[t][i]=avgDose[t];
			}
		}
		if(x_mem->dr != NULL) {
		    Response(ing->ingestionData[i],ing,x_mem->dr,&x_mem->node[i],x_mem->u);

		    DiseaseRM(x_mem->net,x_mem->dr,x_mem->u);
		}
	    x_na.dos[i] = x_mem->u->dos[totsteps-1];
	    x_na.res[i] = x_mem->u->res[totsteps-1];
	    x_na.s[i] = x_mem->u->s[totsteps-1];
	    x_na.i[i] = x_mem->u->i[totsteps-1];
	    x_na.d[i] = x_mem->u->d[totsteps-1];
	    x_na.f[i] = x_mem->u->f[totsteps-1];
	    dos = x_mem->u->dos;
	    for(j=0;j<totsteps;j++) {
	      if(dos[j] > maxDosage)
	        maxDosage = dos[j];
	    }
	    totInfected+=x_mem->node[i].pop*x_mem->u->i[totsteps-1];
	  }
#ifdef WRITE_SIDF
	  writeFileData(x_mem->node[i].info->id,fp[0],x_mem->u->dos,x_mem->node[i].pop,totsteps);
	  writeFileData(x_mem->node[i].info->id,fp[1],x_mem->u->res,x_mem->node[i].pop,totsteps);
	  writeFileData(x_mem->node[i].info->id,fp[2],x_mem->u->s,x_mem->node[i].pop,totsteps);
	  writeFileData(x_mem->node[i].info->id,fp[3],x_mem->u->i,x_mem->node[i].pop,totsteps);
	  writeFileData(x_mem->node[i].info->id,fp[4],x_mem->u->d,x_mem->node[i].pop,totsteps);
	  writeFileData(x_mem->node[i].info->id,fp[5],x_mem->u->f,x_mem->node[i].pop,totsteps);
#endif /* WRITE_SIDF */
	  Accumulate(i,x_mem->node[i].info->nz[0],x_mem->net,x_mem->dr,x_mem->node[i].pop,x_mem->u,x_mem->xa,x_mem->ta,&hist);
	  // x_mem->u holds the s,i,d, f, dosage, and response values for this receptor for all time

	}
#ifdef WRITE_SIDF
	for(i=0;i<6;i++) {
		fclose(fp[i]);
	}
#endif /* WRITE_SIDF */

	if(addVariable(intResultsRef,"Dose_t",     FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(intResultsRef,"Response_t", FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"S_t",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"I_t",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"D_t",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"F_t",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	}
	if(setFloatArray(intResultsRef,"Dose_t",    x_mem->xa->dos,totsteps)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(setFloatArray(intResultsRef,"Response_t",x_mem->xa->res,totsteps)==ND_FAILURE) return;
		if(setFloatArray(intResultsRef,"S_t",       x_mem->xa->s,  totsteps)==ND_FAILURE) return;
		if(setFloatArray(intResultsRef,"I_t",       x_mem->xa->i,  totsteps)==ND_FAILURE) return;
		if(setFloatArray(intResultsRef,"D_t",       x_mem->xa->d,  totsteps)==ND_FAILURE) return;
		if(setFloatArray(intResultsRef,"F_t",       x_mem->xa->f,  totsteps)==ND_FAILURE) return;
	}
	if(addVariable(intResultsRef,"Dose_n",     FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(intResultsRef,"Response_n", FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"S_n",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"I_n",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"D_n",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"F_n",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	}
	if(setFloatArray(intResultsRef,"Dose_n",    x_na.dos,nnodes)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(setFloatArray(intResultsRef,"Response_n",x_na.res,nnodes)==ND_FAILURE) return;
		if(setFloatArray(intResultsRef,"S_n",       x_na.s,  nnodes)==ND_FAILURE) return;
		if(setFloatArray(intResultsRef,"I_n",       x_na.i,  nnodes)==ND_FAILURE) return;
		if(setFloatArray(intResultsRef,"D_n",       x_na.d,  nnodes)==ND_FAILURE) return;
		if(setFloatArray(intResultsRef,"F_n",       x_na.f,  nnodes)==ND_FAILURE) return;
	}
	if(addVariable(intResultsRef,"Hist_testh", FLOAT_TYPE)==ND_FAILURE) return;
	if(addVariable(intResultsRef,"Hist_Dos",   FLOAT_TYPE)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(intResultsRef,"Hist_F",     FLOAT_TYPE)==ND_FAILURE) return;
		if(addVariable(intResultsRef,"Hist_Resp",  FLOAT_TYPE)==ND_FAILURE) return;
	}
	if(setFloat(intResultsRef,"Hist_testh", hist.testh)==ND_FAILURE) return;
	if(setFloat(intResultsRef,"Hist_Dos",   hist.dosh )==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(setFloat(intResultsRef,"Hist_F",     hist.fh   )==ND_FAILURE) return;
		if(setFloat(intResultsRef,"Hist_Resp",  hist.resh )==ND_FAILURE) return;
	}
	if(addVariable(intResultsRef,"Hist_SimID",  STRING_TYPE)==ND_FAILURE) return;
	if(getString(simResultsRef,"injectionID",&simID)==ND_FAILURE) return;
	if(setString(intResultsRef,"Hist_SimID", simID )==ND_FAILURE) return;
	nd_free(simID);

	if(addVariable(intResultsRef,"Hist_InjDef",  STRING_TYPE)==ND_FAILURE) return;
	if(getString(simResultsRef,"injectionDef",&simID)==ND_FAILURE) return;
	if(setString(intResultsRef,"Hist_InjDef", simID )==ND_FAILURE) return;
	nd_free(simID);

	if(addVariable(intResultsRef,"Hist_InjNodeType",  STRING_TYPE)==ND_FAILURE) return;
	if(getString(simResultsRef,"injectionNodeType",&simID)==ND_FAILURE) return;
	if(setString(intResultsRef,"Hist_InjNodeType", simID )==ND_FAILURE) return;
	nd_free(simID);

	if(setDouble(intResultsRef, "RankValueConcentration", hist.fh)==ND_FAILURE) return;
	if(x_numConcToSave>0) {
		if(set2DFloatArray(intResultsRef, "NodeConcentration", conc, nsteps, nnodes)==ND_FAILURE) return;
		for(i=0;i<nsteps;i++) {
			free(conc[i]);
		}
		free(conc);
	}
	if(setDouble(intResultsRef, "RankValueDosage", hist.fh)==ND_FAILURE) return;
	if(x_numDosageToSave>0) {
		if(set2DFloatArray(intResultsRef, "NodeDosage", dosage, nsteps, nnodes)==ND_FAILURE) return;
		for(i=0;i<nsteps;i++) {
			free(dosage[i]);
		}
		free(dosage);
		free(avgDose);
	}
	// additions for TEVA Version 1 ...
	if(addVariable(intResultsRef,"MaxConcentration",  FLOAT_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"MaxDosage",         FLOAT_TYPE )==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(intResultsRef,"TotalInfected",     FLOAT_TYPE )==ND_FAILURE) return;
		if(addVariable(intResultsRef,"TotalFatalities",   FLOAT_TYPE )==ND_FAILURE) return;
		if(addVariable(intResultsRef,"HighFatalityNodes", INT_TYPE )==ND_FAILURE) return;
		if(addVariable(intResultsRef,"NumNodesWithFatalities", INT_TYPE )==ND_FAILURE) return;
	}
	if(addVariable(intResultsRef,"SimulationID",      STRING_TYPE)==ND_FAILURE) return;
	if(addVariable(intResultsRef,"numInjections",     INT_TYPE)==ND_FAILURE) return;

	if(setFloat(intResultsRef,"MaxConcentration", maxConc)==ND_FAILURE) return;
	if(setFloat(intResultsRef,"MaxDosage", maxDosage)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(setFloat(intResultsRef,"TotalInfected", (hist.fh/x_mem->dr->frate)+totInfected)==ND_FAILURE) return;
		if(setFloat(intResultsRef,"TotalFatalities", hist.fh)==ND_FAILURE) return;
	}
	// doses over threshold...
	if(x_mem->dot) {
		if(addVariable(intResultsRef,"TotDoseOverThreshold", INT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setIntArray(intResultsRef,"TotDoseOverThreshold", x_mem->dot->totOver,x_mem->dot->numThresh)==ND_FAILURE) return;

		if(addVariable(intResultsRef,"NodesWithDoseOverThreshold", INT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setIntArray(intResultsRef,"NodesWithDoseOverThreshold", x_mem->dot->nodesOver,x_mem->dot->numThresh)==ND_FAILURE) return;

		if(addVariable(intResultsRef,"NumDoseOverThresholds",     INT_ARRAY_2D_TYPE)==ND_FAILURE) return;
		if(set2DIntArray(intResultsRef,"NumDoseOverThresholds",    x_mem->dot->numOver,x_mem->dot->numThresh,nnodes)==ND_FAILURE) return;
	}
	// binned data
	if(x_mem->doseBins) {
		if(addVariable(intResultsRef,"BinnedDoses",     INT_ARRAY_TYPE)==ND_FAILURE) return;
		if(setIntArray(intResultsRef,"BinnedDoses",    x_mem->doseBins->data,x_mem->doseBins->numBins)==ND_FAILURE) return;
	}

	if(x_mem->dr != NULL) {
		int k;
		float tf=0,ttf;
		float *fatalities = (float *)calloc(nnodes,sizeof(float));
		int minNodesFor90pctFatalities=0;
		int numNodesWithFatalities=0;

		for(k=0;k<nnodes;k++) {
			fatalities[k]=x_na.f[k]*x_mem->node[k].pop;
			if(fatalities[k]>0)
				numNodesWithFatalities++;
			tf += fatalities[k];
		}
		ttf=tf;
		qsort(fatalities,nnodes,sizeof(float),sortFloatDesc);
		tf*=0.9f;
		minNodesFor90pctFatalities=0;
		for(k=0;k<nnodes && tf > 0;k++) {
			minNodesFor90pctFatalities++;
			tf -= fatalities[k];
		}
		if(getString(simResultsRef,"injectionID",&simID)==ND_FAILURE) return;
		simIDLen=strlen(simID);
		sprintf(msg,"simID: %%s  total fatalities: %g (%g) 90%% fatalities: %g nodes: %d\n",ttf,hist.fh,ttf*.9f,minNodesFor90pctFatalities);
		mLen=strlen(msg)+simIDLen+5;
		msg=realloc(msg,mLen*sizeof(char));
		sprintf(msg,"simID: %s  total fatalities: %g (%g) 90%% fatalities: %g nodes: %d\n",simID,ttf,hist.fh,ttf*.9f,minNodesFor90pctFatalities);
		ANL_UTIL_LogFine(env,"teva.analysis.server",msg);
		nd_free(simID);
		fflush(stdout);

		free(fatalities);
		if(setInt(intResultsRef,"NumNodesWithFatalities", numNodesWithFatalities)==ND_FAILURE) return;
		if(setInt(intResultsRef,"HighFatalityNodes", minNodesFor90pctFatalities)==ND_FAILURE) return;
	}

	if(getString(simResultsRef,"injectionID",&simID)==ND_FAILURE) return;
	if(setString(intResultsRef,"SimulationID", simID )==ND_FAILURE) return;
	nd_free(simID);

	if(getInt(simResultsRef,"numInjections",&nInj)==ND_FAILURE) return;
	if(setInt(intResultsRef,"numInjections", nInj )==ND_FAILURE) return;
	free(msg);
}

/* 
 * called once after the scenario has been completed
 */
void HIAnalysis_ia_shutdown(NamedDataRef *analysisOptionsRef)
{
	freeXA(&x_na,x_mem->net->info->numNodes);
	freeHIAMemory(x_mem,IntermediateAnalysis);
}



void writeFileData(char *nodeID,FILE *fp,float *data, float pop,int totsteps)
{
	int i;
	fprintf(fp,"%s",nodeID);
	for(i=0;i<totsteps;i++) {
		fprintf(fp,"\t%f",data[i]*pop);
	}
	fprintf(fp,"\n");
}

int sortFloatDesc(const void *a, const void *b)
{
	float f=*((float *)b) - *((float *)a);
	return (f<0?-1:(f>0?1:0));
}
