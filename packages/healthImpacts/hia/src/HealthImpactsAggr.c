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
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#ifdef WIN32
#include <io.h>
#else
#if defined(__linux__) || defined(__CYGWIN__) || defined(__APPLE__)
#include <errno.h>
#endif
#endif
#include <float.h>
#include <sys/stat.h>
#include "loggingUtils.h"
#include "DiskCachedData.h"
#include "HealthImpacts.h"
#include "ExternalAnalysis.h"
#include "NamedData.h"

#define INCLUDE_DOT_PERCENTILES 1

static PMem x_mem;
static int x_scenarioCount;
static int x_numScenarios;
static PXA x_xa;
static PHIST x_prevHist;
static double x_worstScenario;

static WorstCasePtr x_worstConc;
static WorstCasePtr x_worstDosage;

//static int x_NumWorstConcToKeep;
//static int x_numWorstConc;
//static double x_nthWorstConcScenario;
//static WorstCaseDataPtr x_worstConcScenarios;

//static int x_NumWorstDosageToKeep;
//static int x_numWorstDosage;
//static double x_nthWorstDosageScenario;
//static WorstCaseDataPtr x_worstDosageScenarios;

static PXA x_worstIDF;
static float *maxConcentration;
static float *maxDosage;
static float *totalInfected;
static float *totalFatalities;
static int *highFatalityNodes;
static int *numNodesWithFatalities;
static int x_hasMultipleInjections;
static int **x_BinnedDoses;
static int **x_totDoseOverThresh;
static int **x_nodesWithDoseOverThresh;
static char **x_nodeTypes=NULL;
static DiskCachedDataPtr x_Fatalities;
static DiskCachedDataPtr x_Dosages;
#if INCLUDE_DOT_PERCENTILES
static DiskCachedDataPtr *x_NumDoseOverThresh;
#endif
static DiskCachedDataPtr x_sidfData;

static JNIEnv *x_env;


static int x_nCalls;
static int x_nRetrievals;

int sortFloatAsc(const void *a, const void *b);
int sortIntAsc(const void *a, const void *b);

void computeSummary(float *scenData, int numScen, int *percentiles, float *rv, int numPercentiles);
void computeIntegerSummary(int *scenData, int numScen, int* percentiles, int *rv, int numPercentiles);
void saveFatalityPercentiles(NamedDataRef *resultsRef, DiskCachedDataPtr fatalities, DiskCachedDataPtr dosages, DiskCachedDataPtr sidf, int* percentiles, char *percentileDescs[], int numPercentiles);
void saveTotalDoseOverThreshPercentiles(NamedDataRef *resultsRef, DiskCachedDataPtr *totDoseOverThreshBySrc, int numThresh, int* percentiles, char *percentileDescs[], int numPercentiles);
void addCumulativeDistributionPlotData(NamedDataRef *resultsRef, DiskCachedDataPtr fatalities);

static int x_initFloatArray(void *ref, char *varName, float *data, int len);
static int x_initIntArray(void *ref, char *varName, int *data, int len);
static float **getNodeData(void *ref,char *name,int nsteps,int nnodes);
void freeConcentrationData(float **conc, int nsteps);

static __file_pos_t x_getFilePosition(FILE *fp);

static void x_xaToFloatArray(PXA xa,int nsteps,float *xa_data);
static void x_floatArrayToXA(PXA xa,int nsteps,float *xa_data);


int writeOKCFile(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn);
int writeAllIDFData(const char *dir, char *fn, DiskCachedDataPtr sidf,int nsteps) ;

static void *x_realloc(void *p,size_t s,char *id) {
	void *tp;
	tp=realloc(p,s);
	if(tp==NULL) {
		char t_msg[256];
		sprintf(t_msg,"Unable to reallocate %d bytes for %s.  errno=%d\nerror=%s",
			s,id,errno,strerror(errno));
		ANL_UTIL_LogSevere(x_env,"teva.analysis.server",t_msg);
	}
	p=tp;
	return tp;
}
static void *x_calloc(size_t num, size_t s,char *id) {
	void *p=calloc(num,s);
	if(p==NULL) {
		char t_msg[256];
		sprintf(t_msg,"Unable to allocate %d bytes for %s.  errno=%d\nerror=%s",
			s,id,errno,strerror(errno));
		ANL_UTIL_LogSevere(x_env,"teva.analysis.server",t_msg);
	}
	return p;
}
void freeWorstCaseDiskCache(JNIEnv *env,WorstCasePtr *wcp)
{
	char logMsg[1024];
	struct stat stats;

	fstat(fileno((*wcp)->fpcache),&stats);
	fclose((*wcp)->fpcache);

	sprintf(logMsg,"Cache file: %s size: %ld\n",(*wcp)->fn,stats.st_size);
	ANL_UTIL_LogInfo(env,"teva.analysis.server",logMsg);
	if(unlink((*wcp)->fn)==-1) {
		sprintf(logMsg,"Unable to remove cache file: %s: (%d) %s\n",(*wcp)->fn,errno,strerror(errno));
		ANL_UTIL_LogSevere(env,"teva.analysis.server",logMsg);
	}
	free(*wcp);
	*wcp=NULL;
}
WorstCasePtr initializeWorstCase(NamedDataRef *analysisOptionsRef, char *type)
{
	WorstCasePtr wcp=NULL;
	int num;
	char name[128];
	sprintf(name,"NumWorst%sToKeep",type);
	if(getInt(analysisOptionsRef, name, &num)==ND_FAILURE) return NULL;
	if(num==0) return NULL;
	wcp=(WorstCasePtr)calloc(1,sizeof(WorstCase));
	wcp->numToKeep=num;
	wcp->num=0;
	wcp->nthWorst=0;
	wcp->worst=NULL;
	sprintf(wcp->fn,"w_%s.cache",type);
	wcp->fpcache=fopen(wcp->fn,"w+b");
	wcp->nnodes = x_mem->net->info->numNodes;
	wcp->nsteps = x_mem->net->info->numSteps;
	return wcp;
}

/* 
 * called once after the first simulaiton has been completed (so demands are there)
 */
void HIAnalysis_aggr_initialize(NamedDataRef *analysisOptionsRef, NamedDataRef *simResultsRef)
{
	// at this point, simResultsRef does not conatin any quality, but will contain
	// demands however
	int nnodes;
	int nsteps;

	x_env=analysisOptionsRef->env;

	x_mem = loadHIAOptions(analysisOptionsRef, simResultsRef,Aggregation);
	if(getInt(simResultsRef, "NumSimulations", &x_numScenarios)==ND_FAILURE) return;
	if(getStringArray(simResultsRef,"nodeTypes",&x_nodeTypes)==ND_FAILURE) return;
	Population(x_mem->net,x_mem->popData,x_mem->node);

	dumpHIAInputParameters(x_mem);
	x_worstConc=initializeWorstCase(analysisOptionsRef,"Concentration");
	x_worstDosage=initializeWorstCase(analysisOptionsRef,"Dosage");
	nnodes = x_mem->net->info->numNodes;
	nsteps = x_mem->net->info->numSteps;
	if(x_mem->dr!=NULL) {
		nsteps+=x_mem->dr->nL+x_mem->dr->nF;
	}
	x_scenarioCount=0;
	x_xa = (PXA)x_calloc(1,sizeof(XA),"x_xa");
	if(x_mem->dr != NULL) {
		x_worstIDF = (PXA)x_calloc(1,sizeof(XA),"x_worstIDF");
		initXA(x_worstIDF,nsteps);
	} else {
		x_worstIDF=NULL;
	}
	x_prevHist = NULL;
	x_worstScenario=-1;
	initXA(x_xa,nsteps);

	maxConcentration=NULL;
	maxDosage=NULL;
	x_Dosages=initDiskCachedData(x_env,"dosage.cache",x_mem->net->info->numNodes,x_numScenarios);
	if(x_mem->dot) {
		int i;
		x_totDoseOverThresh=(int **)x_calloc(x_mem->dot->numThresh,sizeof(int *),"x_totDoseOverThresh");
		x_nodesWithDoseOverThresh=(int **)x_calloc(x_mem->dot->numThresh,sizeof(int *),"x_nodesWithDoseOverThresh");
		for(i=0;i<x_mem->dot->numThresh;i++) {
		  x_totDoseOverThresh[i]=(int *)x_calloc(x_numScenarios,sizeof(int)," x_totDoseOverThresh[i]");
		  x_nodesWithDoseOverThresh[i]=(int *)x_calloc(x_numScenarios,sizeof(int)," x_nodesWithDoseOverThresh[i]");
		}
	} else {
		x_totDoseOverThresh=NULL;
		x_nodesWithDoseOverThresh=NULL;
	}
	x_BinnedDoses=NULL;
	
	x_nCalls=0;
	x_nRetrievals=0;

	if(x_mem->dr != NULL) {
		x_Fatalities = initDiskCachedData(x_env,"fatalities.cache",x_mem->net->info->numNodes,x_numScenarios);
	} else {
		x_Fatalities = NULL;
	}
#if INCLUDE_DOT_PERCENTILES
	if(x_mem->dot) {
		int i;
		x_NumDoseOverThresh = (DiskCachedDataPtr*)x_calloc(x_mem->dot->numThresh,sizeof(DiskCachedDataPtr),"x_NumDoseOverThresh");
		for(i=0;i<x_mem->dot->numThresh;i++) {
		  char fn[80];
		  sprintf(fn,"doseover_%02d.cache",i);
		  x_NumDoseOverThresh[i]=initDiskCachedData(x_env,fn,x_mem->net->info->numNodes,x_numScenarios);
//		  x_totDoseOverThresh[i]=(int *)x_calloc(x_numScenarios,sizeof(int)," x_totDoseOverThresh[i]");
		}
	} else {
		x_NumDoseOverThresh=NULL;
	}
#endif
	maxConcentration=(float*)x_calloc(x_numScenarios,sizeof(float),"maxConcentration");
	maxDosage=(float *)x_calloc(x_numScenarios,sizeof(float),"maxDosage");
	if(x_mem->dr != NULL) {
		x_sidfData = initDiskCachedData(x_env,"sidf.cache",nsteps,x_numScenarios);
		totalInfected=(float *)x_calloc(x_numScenarios,sizeof(float),"totalInfected");
		totalFatalities=(float *)x_calloc(x_numScenarios,sizeof(float),"totalFatalities");
		highFatalityNodes=(int *)x_calloc(x_numScenarios,sizeof(int),"highFatalityNodes");
		numNodesWithFatalities=(int *)x_calloc(x_numScenarios,sizeof(int),"numNodesWithFatalities");
	} else {
		x_sidfData = NULL;
		totalInfected=NULL;
		totalFatalities=NULL;
		highFatalityNodes=NULL;
		numNodesWithFatalities=NULL;
	}

	if(x_mem->doseBins) {
		int i;
		x_BinnedDoses=(int**)x_calloc(x_numScenarios,sizeof(int*),"x_BinnedDoses");
		for(i=0;i<x_numScenarios;i++) {
			x_BinnedDoses[i]=(int *)x_calloc(x_mem->doseBins->numBins,sizeof(int),"x_BinnedDoses[x_scenarioCount-1]");
		}
	}
	x_env=NULL;
}

void x_positionFile(FILE *fp,__file_pos_t offs)
{
    fpos_t pos;
        int rv;
        memset(&pos,0,sizeof(fpos_t));
#ifdef __linux__
        pos.__pos = (int64_t)offs;
#else
#ifdef WIN32
        pos = (fpos_t)offs;
#endif
#endif
        rv=fsetpos(fp,&pos);
        if(rv!=0)
                printf("%d: %s\n",errno,strerror(errno));
}
__file_pos_t x_getFilePosition(FILE *fp)
{
    __file_pos_t fpos;
    fpos_t pos;
    memset(&pos,0,sizeof(fpos_t));
    fgetpos(fp,&pos);
#ifdef __linux__
    fpos= pos.__pos;
#else
#ifdef WIN32
    fpos = pos;
#endif
#endif
    return fpos;
}

void wcdpFree(const void *data)
{
	WorstCaseDataPtr wcdp = (WorstCaseDataPtr)data;
	free(wcdp->cData.simID);
	free(wcdp->cData.injDef);
	free(wcdp->cData.injNodeType);
	free(wcdp);
}
void wccdpFree(WorstCaseCachedDataPtr wccdp,int nsteps,int nnodes)
{
	int i;
	for(i=0;i<nsteps;i++) {
		free(wccdp->data[i]);
	}
	free(wccdp->data);
	freeXA(&(wccdp->na),nnodes);
	free(wccdp);
}
void dumpList(WorstCaseDataPtr head) {
	char msg[256];
	WorstCaseDataPtr t;
	for(t=head;t != NULL;t=t->next) {
		sprintf(msg,"%08x: %25.8lf",t,t->cData.val);
		ANL_UTIL_LogSevere(x_env,"anl.teva",msg);
	}
}
WorstCaseDataPtr removeLastElement(WorstCaseDataPtr* head) {
	WorstCaseDataPtr t=*head,p=NULL;
	while(t->next != NULL) { p = t; t=t->next; }
	if(p!=NULL) {
		p->next = NULL;
	} else {
		*head = NULL;
	}
	return t;
}
WorstCaseDataPtr getLastElement(WorstCaseDataPtr head) {
	WorstCaseDataPtr t=head,p=NULL;
	while(t->next != NULL) t=t->next;
	return t;
}
void freeWCDPData(WorstCaseDataPtr *head) {
	WorstCaseDataPtr t=*head;
	while(t != NULL) {
		WorstCaseDataPtr p=t;
		t=t->next;
		wcdpFree(p);
	}
}
void insertList(WorstCaseDataPtr data, WorstCaseDataPtr *head) {
	WorstCaseDataPtr t,p;
	t=*head;
	p=NULL;
	while(t != NULL && data->cData.val < t->cData.val) {
		p=t;
		t=t->next;
	}
	if(p!=NULL) {
		p->next = data;
	} else {
		*head = data;
	}
	data->next = t;
}

/***********************************************
void setResults(ConcentrationDataPtr head, void *resultsRef) 
{
	ConcentrationDataPtr cdp;
	int i;
	char name[256];

	for(cdp=head,i=0;cdp != NULL; cdp=cdp->next,i++) {

		sprintf(name,"SimID[%02d]",i);
		if(addVariable(resultsRef,name, STRING_TYPE) ==ND_FAILURE) return;
		if(setString(resultsRef,name, cdp->simID) ==ND_FAILURE) return;

		sprintf(name,"InjDef[%02d]",i);
		if(addVariable(resultsRef,name, STRING_TYPE) ==ND_FAILURE) return;
		if(setString(resultsRef,name, cdp->injDef) ==ND_FAILURE) return;

		sprintf(name,"RankValue[%02d]",i);
		if(addVariable(resultsRef,name, DOUBLE_TYPE) ==ND_FAILURE) return;
		if(setDouble(resultsRef,name, cdp->rankValue) ==ND_FAILURE) return;

		sprintf(name,"NodeConcentration[%02d]",i);
		if(addVariable(resultsRef,name,     FLOAT_ARRAY_2D_TYPE) ==ND_FAILURE) return;
		if(set2DFloatArray(resultsRef,name, cdp->conc,cdp->nsteps, cdp->nnodes) ==ND_FAILURE) return;

		sprintf(name,"ReceptorDosage[%02d]",i);
		if(addVariable(resultsRef,name,     FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setFloatArray(resultsRef,name, cdp->na.dos, cdp->nnodes) ==ND_FAILURE) return;
	
		sprintf(name,"ReceptorResponse[%02d]",i);
		if(addVariable(resultsRef,name,     FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setFloatArray(resultsRef,name, cdp->na.res, cdp->nnodes) ==ND_FAILURE) return;
		
		sprintf(name,"ReceptorS[%02d]",i);
		if(addVariable(resultsRef,name,     FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setFloatArray(resultsRef,name, cdp->na.s, cdp->nnodes) ==ND_FAILURE) return;
		
		sprintf(name,"ReceptorI[%02d]",i);
		if(addVariable(resultsRef,name,     FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setFloatArray(resultsRef,name, cdp->na.i, cdp->nnodes) ==ND_FAILURE) return;
		
		sprintf(name,"ReceptorD[%02d]",i);
		if(addVariable(resultsRef,name,     FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setFloatArray(resultsRef,name, cdp->na.d, cdp->nnodes) ==ND_FAILURE) return;
		
		sprintf(name,"ReceptorF[%02d]",i);
		if(addVariable(resultsRef,name,     FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setFloatArray(resultsRef,name, cdp->na.f, cdp->nnodes) ==ND_FAILURE) return;
	
	}
}
**********************************************************/

void writeCachedData(WorstCaseCachedDataPtr wccdp,FILE *fp,int nnodes,int nsteps)
{
	int t;
	for(t=0;t<nsteps;t++) {
		fwrite(wccdp->data[t],sizeof(float),nnodes,fp);
	}
	fwrite(wccdp->na.dos,sizeof(float),nnodes,fp);
	fwrite(wccdp->na.res,sizeof(float),nnodes,fp);
	fwrite(wccdp->na.s,sizeof(float),nnodes,fp);
	fwrite(wccdp->na.i,sizeof(float),nnodes,fp);
	fwrite(wccdp->na.d,sizeof(float),nnodes,fp);
	fwrite(wccdp->na.f,sizeof(float),nnodes,fp);
}
void getWorstCaseCachedData(WorstCasePtr wcp, WorstCaseDataPtr wcdp, WorstCaseCachedDataPtr wccdp)
{
	int t;
	int nsteps=wcp->nsteps;
	int nnodes=wcp->nnodes;
	FILE *fp=wcp->fpcache;
	x_positionFile(wcp->fpcache,wcdp->cData.fileOffset);
	wccdp->data=(float**)calloc(nsteps,sizeof(float*));
	for(t=0;t<nsteps;t++) {
		wccdp->data[t]=(float*)calloc(nnodes,sizeof(float));
		fread(wccdp->data[t],sizeof(float),nnodes,fp);
	}
	initXA(&wccdp->na,nnodes);
	fread(wccdp->na.dos,sizeof(float),nnodes,fp);
	fread(wccdp->na.res,sizeof(float),nnodes,fp);
	fread(wccdp->na.s,sizeof(float),nnodes,fp);
	fread(wccdp->na.i,sizeof(float),nnodes,fp);
	fread(wccdp->na.d,sizeof(float),nnodes,fp);
	fread(wccdp->na.f,sizeof(float),nnodes,fp);
}
void cacheWorstCaseData(WorstCasePtr wcp, WorstCaseDataPtr wcdp, WorstCaseCachedDataPtr wccdp)
{
	WorstCaseDataPtr removed=NULL;
	if(wcp->num == wcp->numToKeep) {
		// replace an element in the cache...
		// remove the last element
		removed=removeLastElement(&wcp->worst);
		wcp->num--;
	}
	insertList(wcdp,&wcp->worst);
	wcp->num++;
	if(removed != NULL) {
		x_positionFile(wcp->fpcache,removed->cData.fileOffset);
	}
	wcdp->cData.fileOffset=x_getFilePosition(wcp->fpcache);
	writeCachedData(wccdp,wcp->fpcache,wcp->nnodes,wcp->nsteps);
	wcp->nthWorst=getLastElement(wcp->worst)->cData.val;
}
void updateWorstCase(WorstCasePtr wcp,NamedDataRef *intResultsRef,char *type)
{
	char msg[256];

	if(wcp != NULL && wcp->numToKeep>0) {
		JNIEnv *env=intResultsRef->env;
		double rank;
		char varName[256];
		sprintf(varName,"RankValue%s",type);
		if(getDouble(intResultsRef,varName, &rank)==ND_FAILURE) return;
		wcp->numCalls++;
		sprintf(msg,"Rank: %lf nthWorstRank: %lf",rank,wcp->nthWorst);
		ANL_UTIL_LogFine(env,"teva.analysis.server",msg);
		if(rank>wcp->nthWorst || wcp->num < wcp->numToKeep) {
			char *tstr;
			float *tdata;
			int nsteps=wcp->nsteps;
			int nnodes=wcp->nnodes;
			WorstCaseCachedDataPtr wccdp=(WorstCaseCachedDataPtr)calloc(1,sizeof(WorstCaseCachedData));
			WorstCaseDataPtr wcdp = (WorstCaseDataPtr)x_calloc(1,sizeof(WorstCaseData),"wcdp");

			wcp->numRetrievals++;
			sprintf(msg,"Retrieving %s data: %d (total calls: %d)",type,wcp->numRetrievals,wcp->numCalls);
			ANL_UTIL_LogFine(env,"teva.analysis.server",msg);

			wcdp->cData.val=rank;
			if(getString(intResultsRef,"Hist_SimID", &(tstr))==ND_FAILURE) return;
			wcdp->cData.simID=(char*)x_calloc(strlen(tstr)+1,sizeof(char),"cdp->simID");
			strcpy(wcdp->cData.simID,tstr);
			nd_free(tstr);
			if(getString(intResultsRef,"Hist_InjDef", &(tstr))==ND_FAILURE) return;
			wcdp->cData.injDef=(char*)x_calloc(strlen(tstr)+1,sizeof(char),"cdp->injDef");
			strcpy(wcdp->cData.injDef,tstr);
			nd_free(tstr);
			if(getString(intResultsRef,"Hist_InjNodeType", &(tstr))==ND_FAILURE) return;
			wcdp->cData.injNodeType=(char*)x_calloc(strlen(tstr)+1,sizeof(char),"cdp->injNodeType");
			strcpy(wcdp->cData.injNodeType,tstr);
			nd_free(tstr);

			sprintf(varName,"Node%s",type);
			wccdp->data = getNodeData(intResultsRef,varName,nsteps,nnodes);
			initXA(&wccdp->na,nnodes);
			if(getFloatArray(intResultsRef,"Dose_n",&tdata)==ND_FAILURE) return;
			memcpy(wccdp->na.dos,tdata,nnodes*sizeof(float));
			nd_free(tdata);
			if(x_mem->dr != NULL) {
				if(getFloatArray(intResultsRef,"Response_n",&tdata)==ND_FAILURE) return;
				memcpy(wccdp->na.res,tdata,nnodes*sizeof(float));
				nd_free(tdata);
				if(getFloatArray(intResultsRef,"S_n",&tdata)==ND_FAILURE) return;
				memcpy(wccdp->na.s,tdata,nnodes*sizeof(float));
				nd_free(tdata);
				if(getFloatArray(intResultsRef,"I_n",&tdata)==ND_FAILURE) return;
				memcpy(wccdp->na.i,tdata,nnodes*sizeof(float));
				nd_free(tdata);
				if(getFloatArray(intResultsRef,"D_n",&tdata)==ND_FAILURE) return;
				memcpy(wccdp->na.d,tdata,nnodes*sizeof(float));
				nd_free(tdata);
				if(getFloatArray(intResultsRef,"F_n",&tdata)==ND_FAILURE) return;
				memcpy(wccdp->na.f,tdata,nnodes*sizeof(float));
				nd_free(tdata);
			}
			cacheWorstCaseData(wcp,wcdp,wccdp);
			wccdpFree(wccdp,nsteps,nnodes);
		}
	}
}
/*
 * called once after every simulation with the results of the intermediate analysis
 */
void HIAnalysis_aggr_newResults(NamedDataRef *analysisOptionsRef,NamedDataRef *intResultsRef)
{
	int i;
	int nnodes = x_mem->net->info->numNodes;
	int nsteps = x_mem->dr!=NULL?x_mem->dr->maxsteps:x_mem->net->info->numSteps;
	int numModelTimeSteps = x_mem->net->info->numSteps;
	char *tstr;
	double rank;
	float *tdata;
	int *binnedData;
	PHIST hist = NULL;
	JNIEnv *env = analysisOptionsRef->env;
	int numInj;
	char *msg=calloc(256,sizeof(char));
	x_env=env;

	if(getInt(intResultsRef,"numInjections", &numInj)==ND_FAILURE) return;
	if(numInj>1) x_hasMultipleInjections=1;

	updateWorstCase(x_worstConc,intResultsRef,"Concentration");
	updateWorstCase(x_worstDosage,intResultsRef,"Dosage");
	if(getDouble(intResultsRef,"RankValueConcentration", &rank)==ND_FAILURE) return;


	if(getString(intResultsRef,"Hist_SimID", &(tstr))==ND_FAILURE) return;
	hist = allocateHistData(tstr);
	nd_free(tstr);
	if(getFloat(intResultsRef,"Hist_testh", &(hist->testh))==ND_FAILURE) return;
	if(getFloat(intResultsRef,"Hist_Dos",   &(hist->dosh ))==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(getFloat(intResultsRef,"Hist_Resp",  &(hist->resh ))==ND_FAILURE) return;
		if(getFloat(intResultsRef,"Hist_F",     &(hist->fh   ))==ND_FAILURE) return;
	}
	if(x_prevHist == NULL)
		x_mem->hist = hist;
	else
		x_prevHist->next = hist;
	x_prevHist = hist;
	
	msg=realloc(msg,(strlen(hist->simID)+32)*sizeof(char));
	sprintf(msg," processing %s",hist->simID);
	ANL_UTIL_LogFine(env,"teva.analysis.server",msg);

	if(getString(intResultsRef,"Hist_InjDef", &(tstr))==ND_FAILURE) return;
	hist->injDef=(char*)calloc(strlen(tstr)+1,sizeof(char));
	strcpy(hist->injDef,tstr);
	nd_free(tstr);

	if(getString(intResultsRef,"Hist_InjNodeType", &(tstr))==ND_FAILURE) return;
	hist->injNodeType=(char*)calloc(strlen(tstr)+1,sizeof(char));
	strcpy(hist->injNodeType,tstr);
	nd_free(tstr);

	x_scenarioCount++;

	if(x_mem->dr != NULL) {
		if(getFloatArray(intResultsRef,"F_n",&tdata)==ND_FAILURE) return;
		cacheData(intResultsRef->env,x_Fatalities,hist->fh,hist->simID,hist->injDef,tdata,nnodes*sizeof(float),"fatalities");
		nd_free(tdata);
	}

	if(getFloatArray(intResultsRef,"Dose_n",&tdata)==ND_FAILURE) return;
	cacheData(intResultsRef->env,x_Dosages,hist->dosh,hist->simID,hist->injDef,tdata,nnodes*sizeof(float),"dosage");
	nd_free(tdata);

	x_initFloatArray(intResultsRef,"Dose_t",    x_xa->dos,nsteps);
	if(x_mem->dr != NULL) {
		float *xa_data=(float*)x_calloc(6*nsteps,sizeof(float),"xa_data");
		x_initFloatArray(intResultsRef,"Response_t",x_xa->res,nsteps);
		x_initFloatArray(intResultsRef,"S_t",       x_xa->s,  nsteps);
		x_initFloatArray(intResultsRef,"I_t",       x_xa->i,  nsteps);
		x_initFloatArray(intResultsRef,"D_t",       x_xa->d,  nsteps);
		x_initFloatArray(intResultsRef,"F_t",       x_xa->f,  nsteps);

		x_xaToFloatArray(x_xa,nsteps,xa_data);
		cacheData(intResultsRef->env,x_sidfData,hist->fh,hist->simID,hist->injDef,xa_data,6*nsteps*sizeof(float),"sidf_data");
		free(xa_data);
	}

	/* Sum state variables over all nodes and all scenarios */

	for(i=0;i<nsteps;i++) {
		x_mem->xa->dos[i] += x_xa->dos[i];
	}
	if(x_mem->dr != NULL) {
		for(i=0;i<nsteps;i++) {
			x_mem->xa->res[i] += x_xa->res[i];
			x_mem->xa->s[i]   += x_xa->s[i];
			x_mem->xa->i[i]   += x_xa->i[i];
			x_mem->xa->d[i]   += x_xa->d[i];
			x_mem->xa->f[i]   += x_xa->f[i];
		}
	}

	if(x_mem->dr != NULL && rank > x_worstScenario) {
		memcpy(x_worstIDF->dos,x_xa->dos,nsteps*sizeof(float));
		memcpy(x_worstIDF->res,x_xa->res,nsteps*sizeof(float));
		memcpy(x_worstIDF->s,x_xa->s,nsteps*sizeof(float));
		memcpy(x_worstIDF->i,x_xa->i,nsteps*sizeof(float));
		memcpy(x_worstIDF->d,x_xa->d,nsteps*sizeof(float));
		memcpy(x_worstIDF->f,x_xa->f,nsteps*sizeof(float));
		x_worstScenario=rank;
	}

	if(getFloat(intResultsRef,"MaxConcentration", &(maxConcentration[x_scenarioCount-1]))==ND_FAILURE) return;
	if(getFloat(intResultsRef,"MaxDosage", &(maxDosage[x_scenarioCount-1]))==ND_FAILURE) return;
	if(x_mem->dot) {
		int *tdot,*ndot;
		if(getIntArray(intResultsRef,"TotDoseOverThreshold",&tdot)==ND_FAILURE) return;
		if(getIntArray(intResultsRef,"NodesWithDoseOverThreshold",&ndot)==ND_FAILURE) return;
		for(i=0;i<x_mem->dot->numThresh;i++) {
		  int *tidata;
		  char typeS[32];
		  x_totDoseOverThresh[i][x_scenarioCount-1]=tdot[i];
		  x_nodesWithDoseOverThresh[i][x_scenarioCount-1]=ndot[i];

#if INCLUDE_DOT_PERCENTILES
		  if(getIntArrayAtIndex(intResultsRef,"NumDoseOverThresholds",i,&tidata)==ND_FAILURE) return;
		  sprintf(typeS,"dot_%02d",i);
		  cacheData(intResultsRef->env,x_NumDoseOverThresh[i],(float)x_totDoseOverThresh[i][x_scenarioCount-1],hist->simID,hist->injDef,tidata,nnodes*sizeof(float),typeS);
		  nd_free(tidata);
#endif
		}
		nd_free(tdot);
		nd_free(ndot);
	}
	if(x_mem->doseBins) {
		if(getIntArray(intResultsRef,"BinnedDoses",&binnedData)==ND_FAILURE) return;
		memcpy(x_BinnedDoses[x_scenarioCount-1],binnedData,x_mem->doseBins->numBins*sizeof(int));
		nd_free(binnedData);
	}
	if(x_mem->dr != NULL) {
		if(getFloat(intResultsRef,"TotalInfected", &(totalInfected[x_scenarioCount-1]))==ND_FAILURE) return;
		if(getFloat(intResultsRef,"TotalFatalities", &(totalFatalities[x_scenarioCount-1]))==ND_FAILURE) return;
		if(getInt(intResultsRef,"HighFatalityNodes", &(highFatalityNodes[x_scenarioCount-1]))==ND_FAILURE) return;
		if(getInt(intResultsRef,"NumNodesWithFatalities", &(numNodesWithFatalities[x_scenarioCount-1]))==ND_FAILURE) return;
	}
	x_env=NULL;
	free(msg);
}


/* 
 * called usually only once after the scenario is done to obtain the completed analysis results
 */
void HIAnalysis_aggr_getResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef)
{
    char *sensortypes[]={"Ignored","Existing","Potential","Selected"};
	int i;
	int nnodes = x_mem->net->info->numNodes;
	int nsteps = x_mem->dr!=NULL?x_mem->dr->maxsteps:x_mem->net->info->numSteps;
	PHIST tHist;
	float *hData,*time,*f,*d,*r,*pop;
	int **dot;
	int percentiles[]={10, 25, 50, 75, 90, 100};
	char *rowNames[]={"10th percentile", "25th percentile", "50th percentile", "75th percentile", "90th percentile", "100th percentile", "Mean"};
	int numPercentiles=6;
	int dotPercentiles[]={10, 25, 50, 75, 90, 95, 100};
	char *dotPctRowNames[]={"10th percentile", "25th percentile", "50th percentile", "75th percentile", "90th percentile", "95th percentile", "100th percentile", "Mean"};
	int dotNumPercentiles=7;
	char *nodeDisp;
	char **sData,**ids,**sensorTypes,**nodeTypes;
	int numDThresh,numRThresh;
	float *pd_val;
	char** pd_id;

	JNIEnv *env=resultsRef->env;

	if(addVariable(resultsRef,"Length_t",   INT_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"Length_n",   INT_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"Dose_t",     FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(resultsRef,"Response_t", FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"S_t",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"I_t",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"D_t",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"F_t",        FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"time",       FLOAT_ARRAY_TYPE)==ND_FAILURE) return;

	if(addVariable(resultsRef,"NSteps",       INT_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"NNodes",       INT_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"NodeIDs",      STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"NodeTypes",    STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"NumScenarios", INT_TYPE)==ND_FAILURE) return;

	if(addVariable(resultsRef,"Hist_testh",     FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	if(addVariable(resultsRef,"Hist_Dos",       FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(resultsRef,"Hist_F",         FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(addVariable(resultsRef,"Hist_Resp",      FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"Hist_SimID",     STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"Hist_InjDef",    STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"Hist_InjNodeType",    STRING_ARRAY_TYPE)==ND_FAILURE) return;

	if(addVariable(resultsRef,"InjNodeDisplay",        BYTE_ARRAY_TYPE)  ==ND_FAILURE) return;
	if(addVariable(resultsRef,"Map_NodeIDs",           STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"Map_SensorTypes",       STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"Map_Dos",               FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(resultsRef,"Map_F",                 FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(addVariable(resultsRef,"Map_Resp",              FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"NumDoseThresholds",              INT_TYPE) ==ND_FAILURE) return;
	if(addVariable(resultsRef,"DoseThresholds",                 FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(resultsRef,"NumResponseThresholds",          INT_TYPE) ==ND_FAILURE) return;
		if(addVariable(resultsRef,"ResponseThresholds",             FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(addVariable(resultsRef,"DoseThresholdResponses",    FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"ResponseThresholdResponses",FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(getInt(analysisOptionsRef, "NumResponseThresholds", &numRThresh)==ND_FAILURE) return;
	}
	if(getInt(analysisOptionsRef, "NumDoseThresholds", &numDThresh)==ND_FAILURE) return;

	pd_id=(char **)calloc(numDThresh,sizeof(char *));
	pd_val=(float *)calloc(numDThresh,sizeof(float));
	for(i=0;i<numDThresh;i++) {
	  char varName[80];
	  pd_id[i]=(char *)calloc(3,sizeof(char));
	  sprintf(pd_id[i],"%02d",i);
	  pd_val[i]=x_mem->dot->thresholds[i];
	  sprintf(varName,"Map_DoseOverThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	  sprintf(varName,"DoseThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, FLOAT_TYPE) ==ND_FAILURE) return;
	  if(setFloat(resultsRef,varName,x_mem->dot->thresholds[i]) ==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"pd_val", FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	if(setFloatArray(resultsRef,"pd_val",pd_val,numDThresh) ==ND_FAILURE) return;
	if(addVariable(resultsRef,"pd_id", STRING_ARRAY_TYPE) ==ND_FAILURE) return;
	if(setStringArray(resultsRef,"pd_id",pd_id,numDThresh) ==ND_FAILURE) return;
	for(i=0;i<numDThresh;i++) {
	  free(pd_id[i]);
	}
	free(pd_id);
	free(pd_val);
	for(i=0;i<numRThresh;i++) {
	  char varName[80];
	  sprintf(varName,"Map_ResponseOverThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	  sprintf(varName,"ResponseThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, FLOAT_TYPE) ==ND_FAILURE) return;
	  if(setFloat(resultsRef,varName,x_mem->dot->thresholds[i+numDThresh]) ==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"Population",     FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;

	if(addVariable(resultsRef,"MaxConcentration",               FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	if(addVariable(resultsRef,"MaxDosage",                      FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(resultsRef,"TotalInfected",                  FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"TotalFatalities",                FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"HighFatalityNodes",              INT_ARRAY_TYPE   )==ND_FAILURE) return;
		if(addVariable(resultsRef,"NumNodesWithFatalities",         INT_ARRAY_TYPE   )==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"MaxConcentrationBySource",       FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	if(addVariable(resultsRef,"MaxDosageBySource",              FLOAT_ARRAY_TYPE )==ND_FAILURE) return;

	for(i=0;i<numDThresh;i++) {
	  char varName[80];
	  sprintf(varName,"TotalDoseOverThresholdBySource_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	  sprintf(varName,"TotalNodesOverThresholdBySource_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	}
	for(i=0;i<numRThresh;i++) {
	  char varName[80];
	  sprintf(varName,"TotalResponseOverThresholdBySource_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	}

	if(addVariable(resultsRef,"WorstScenario_Dosage",   FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(resultsRef,"TotalInfectedBySource",          FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"TotalFatalitiesBySource",        FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"HighFatalityNodesBySource",      INT_ARRAY_TYPE   )==ND_FAILURE) return;
		if(addVariable(resultsRef,"NumNodesWithFatalitiesBySource", INT_ARRAY_TYPE   )==ND_FAILURE) return;
		if(addVariable(resultsRef,"DummyXVals",                     INT_ARRAY_TYPE   )==ND_FAILURE) return;
		if(addVariable(resultsRef,"WorstScenario_Response", FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"WorstScenario_S",        FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"WorstScenario_I",        FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"WorstScenario_D",        FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"WorstScenario_F",        FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"RowNames",                  STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"DoseOverThresholdRowNames", STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(addVariable(resultsRef,"MaxConcentrationSummary",   FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	if(addVariable(resultsRef,"MaxDosageSummary",          FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(addVariable(resultsRef,"TotalInfectedSummary",    FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(addVariable(resultsRef,"TotalFatalitiesSummary",  FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	}
	for(i=0;i<numDThresh;i++) {
	  char varName[80];
	  sprintf(varName,"TotalDoseOverThresholdSummary_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	}
	for(i=0;i<numRThresh;i++) {
	  char varName[80];
	  sprintf(varName,"TotalResponseOverThresholdSummary_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	}

	pop = (float*)x_calloc(nnodes,sizeof(float),"pop");
	for(i=0;i<nnodes;i++) {
		pop[i] = x_mem->node[i].pop;
	}
	if(setFloatArray(resultsRef,"Population",    pop,nnodes)==ND_FAILURE) return;
	free(pop);

	time = (float*)calloc(nsteps,sizeof(float));
	for(i=0;i<nsteps;i++) {
		time[i]=(float)i*x_mem->net->info->stepSize;
		x_xa->dos[i] = x_mem->xa->dos[i] / x_scenarioCount;
	}
	if(x_mem->dr != NULL) {
		for(i=0;i<nsteps;i++) {
			x_xa->res[i] = x_mem->xa->res[i] / x_scenarioCount;
			x_xa->s[i]   = x_mem->xa->s[i]   / x_scenarioCount;
			x_xa->i[i]   = x_mem->xa->i[i]   / x_scenarioCount;
			x_xa->d[i]   = x_mem->xa->d[i]   / x_scenarioCount;
			x_xa->f[i]   = x_mem->xa->f[i]   / x_scenarioCount;
		}
	}
	if(setInt(resultsRef,"Length_n",    nnodes)==ND_FAILURE) return;
	if(setInt(resultsRef,"Length_t",    nsteps)==ND_FAILURE) return;
	if(setFloatArray(resultsRef,"Dose_t",    x_xa->dos,nsteps)==ND_FAILURE) return;
	if(setFloatArray(resultsRef,"time",      time,     nsteps)==ND_FAILURE) return;
	free(time);
	if(x_mem->dr != NULL) {
		if(setFloatArray(resultsRef,"Response_t",x_xa->res,nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"S_t",       x_xa->s,  nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"I_t",       x_xa->i,  nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"D_t",       x_xa->d,  nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"F_t",       x_xa->f,  nsteps)==ND_FAILURE) return;
	}
	if(setInt(resultsRef,"NSteps",      x_mem->net->info->numSteps)==ND_FAILURE) return;
	if(setInt(resultsRef,"NNodes",      x_mem->net->info->numNodes)==ND_FAILURE) return;
	if(setInt(resultsRef,"NumScenarios",x_scenarioCount)==ND_FAILURE) return;

	nodeDisp = (char*)calloc(x_mem->net->info->numNodes,sizeof(char));
	if(x_mem->dr != NULL) {
		f = (float*)calloc(x_mem->net->info->numNodes,sizeof(float));
		r = (float*)calloc(x_mem->net->info->numNodes,sizeof(float));
	}
	d = (float*)calloc(x_mem->net->info->numNodes,sizeof(float));
	if(x_mem->dot) {
		dot=(int**)calloc(x_mem->dot->numThresh,sizeof(int*));
		for(i=0;i<x_mem->dot->numThresh;i++) {
		  dot[i]=(int*)calloc(x_mem->net->info->numNodes,sizeof(int));
		}
	} else {
		dot=NULL;
	}
	ids = (char **)calloc(x_mem->net->info->numNodes,sizeof(char*));
	nodeTypes = (char **)calloc(x_mem->net->info->numNodes,sizeof(char*));
	sensorTypes = (char **)calloc(nnodes,sizeof(char *));
	for(i=0;i<nnodes;i++) {
		char *st = sensortypes[x_mem->node[i].sensor.type];
		ids[i]=x_mem->node[i].info->id;
		nodeTypes[i]=x_nodeTypes[i];
		sensorTypes[i] = st;
	}
	if(setStringArray(resultsRef,"NodeIDs",ids,nnodes)==ND_FAILURE) return;
	if(setStringArray(resultsRef,"NodeTypes",nodeTypes,nnodes)==ND_FAILURE) return;
	if(setStringArray(resultsRef,"Map_SensorTypes",sensorTypes,nnodes)==ND_FAILURE) return;
	// do nt free individual elements because they were never allocate - they just point to the constant
	free(sensorTypes);
	for(i=0,tHist=x_mem->hist;tHist != NULL;tHist=tHist->next,i++) {
	  int j;
		int idx = getnodeindex(x_mem->net,x_mem->node,tHist->simID);
		if(idx > -1) {
			nodeDisp[idx] = 1;
			d[idx]=tHist->dosh;
			if(x_mem->dr != NULL) {
				f[idx]=tHist->fh;
				r[idx]=tHist->resh;
			}
			if(dot) {
				for(j=0;j<x_mem->dot->numThresh;j++) {
				  dot[j][idx]=x_totDoseOverThresh[j][i];
				}
			}
		}
	}
	if(setStringArray(resultsRef,"Map_NodeIDs",ids,  x_mem->net->info->numNodes)==ND_FAILURE) return;
	free(ids);
	if(!x_hasMultipleInjections) {
		if(setByteArray(resultsRef,"InjNodeDisplay", nodeDisp,  x_mem->net->info->numNodes)==ND_FAILURE) return;
	}
	free(nodeDisp);
	if(setFloatArray(resultsRef,"Map_Dos",   d,  x_mem->net->info->numNodes)==ND_FAILURE) return;
	free(d);
	if(x_mem->dr != NULL) {
		if(setFloatArray(resultsRef,"Map_F",     f,  x_mem->net->info->numNodes)==ND_FAILURE) return;
		free(f);
		if(setFloatArray(resultsRef,"Map_Resp",  r,  x_mem->net->info->numNodes)==ND_FAILURE) return;
		free(r);
	}

	if(setInt(resultsRef,"NumDoseThresholds",numDThresh)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		if(setInt(resultsRef,"NumResponseThresholds",numRThresh)==ND_FAILURE) return;
	}
	if(x_mem->dot) {
		float *resp;
		if(setFloatArray(resultsRef,"DoseThresholds",x_mem->dot->thresholds,numDThresh)==ND_FAILURE) return;
	
		if(x_mem->dr != NULL) {
			if(setFloatArray(resultsRef,"ResponseThresholds",&x_mem->dot->thresholds[numDThresh],numRThresh)==ND_FAILURE) return;
			if(numDThresh > 0) {
				resp=(float *)calloc(numDThresh,sizeof(float));
				for(i=0;i<numDThresh;i++) {
					resp[i]=DoseResponse(x_mem->dot->thresholds[i],x_mem->dr);
				}
				if(setFloatArray(resultsRef,"DoseThresholdResponses",resp,numDThresh)==ND_FAILURE) return;
				free(resp);
			}
			if(numRThresh>0) {
				resp=(float *)calloc(numRThresh,sizeof(float));
				for(i=0;i<numRThresh;i++) {
					resp[i]=DoseResponse(x_mem->dot->thresholds[i+numDThresh],x_mem->dr);
				}
				if(setFloatArray(resultsRef,"ResponseThresholdResponses",resp,numRThresh)==ND_FAILURE) return;
				free(resp);
			}
		}
		for(i=0;i<numDThresh;i++) {
		  char varName[80];
		  sprintf(varName,"Map_DoseOverThreshold_%02d",i);
		  if(setIntArray(resultsRef,varName,  dot[i],  x_mem->net->info->numNodes)==ND_FAILURE) return;
		  free(dot[i]);
		}
		for(i=0;i<numRThresh;i++) {
		  char varName[80];
		  sprintf(varName,"Map_ResponseOverThreshold_%02d",i);
		  if(setIntArray(resultsRef,varName,  dot[i+numDThresh],  x_mem->net->info->numNodes)==ND_FAILURE) return;
		  free(dot[i+numDThresh]);
		}
		free(dot);
	}
	hData = (float*)calloc(x_scenarioCount,sizeof(float));
	for(i=0,tHist=x_mem->hist;tHist != NULL;i++,tHist=tHist->next) hData[i] = tHist->dosh;
	if(setFloatArray(resultsRef,"Hist_Dos",   hData,  x_scenarioCount)==ND_FAILURE) return;
	for(i=0,tHist=x_mem->hist;tHist != NULL;i++,tHist=tHist->next) hData[i] = tHist->testh;
	if(setFloatArray(resultsRef,"Hist_testh", hData,  x_scenarioCount)==ND_FAILURE) return;
	if(x_mem->dr != NULL) {
		for(i=0,tHist=x_mem->hist;tHist != NULL;i++,tHist=tHist->next) hData[i] = tHist->fh;
		if(setFloatArray(resultsRef,"Hist_F",     hData,  x_scenarioCount)==ND_FAILURE) return;
		for(i=0,tHist=x_mem->hist;tHist != NULL;i++,tHist=tHist->next) hData[i] = tHist->resh;
		if(setFloatArray(resultsRef,"Hist_Resp",     hData,  x_scenarioCount)==ND_FAILURE) return;
	}
	free(hData);

	sData = (char **)calloc(x_scenarioCount,sizeof(char *));
	for(i=0,tHist=x_mem->hist;tHist != NULL;i++,tHist=tHist->next) sData[i] = tHist->simID;
	if(setStringArray(resultsRef,"Hist_SimID", sData,  x_scenarioCount)==ND_FAILURE) return;
	free(sData);

	sData = (char **)calloc(x_scenarioCount,sizeof(char *));
	for(i=0,tHist=x_mem->hist;tHist != NULL;i++,tHist=tHist->next) sData[i] = tHist->injDef;
	if(setStringArray(resultsRef,"Hist_InjDef", sData,  x_scenarioCount)==ND_FAILURE) return;
	free(sData);

	sData = (char **)calloc(x_scenarioCount,sizeof(char *));
	for(i=0,tHist=x_mem->hist;tHist != NULL;i++,tHist=tHist->next) sData[i] = tHist->injNodeType;
	if(setStringArray(resultsRef,"Hist_InjNodeType", sData,  x_scenarioCount)==ND_FAILURE) return;
	free(sData);

	if(setFloatArray(resultsRef,"MaxConcentrationBySource", maxConcentration,  x_scenarioCount)==ND_FAILURE) return;
	if(setFloatArray(resultsRef,"MaxDosageBySource", maxDosage,  x_scenarioCount)==ND_FAILURE) return;
	if(x_mem->dot) {
		for(i=0;i<numDThresh;i++) {
			char varName[80];
		  sprintf(varName,"TotalDoseOverThresholdBySource_%02d",i);
		  if(setIntArray(resultsRef,varName,  x_totDoseOverThresh[i],  x_scenarioCount)==ND_FAILURE) return;
		  sprintf(varName,"TotalNodesOverThresholdBySource_%02d",i);
		  if(setIntArray(resultsRef,varName,  x_nodesWithDoseOverThresh[i],  x_scenarioCount)==ND_FAILURE) return;
		}
		for(i=0;i<numRThresh;i++) {
		  char varName[80];
		  sprintf(varName,"TotalResponseOverThresholdBySource_%02d",i);
		  if(setIntArray(resultsRef,varName,  x_totDoseOverThresh[i+numDThresh],  x_scenarioCount)==ND_FAILURE) return;
		}
	}	
	// binned dosage data...
	if(x_mem->doseBins == NULL) {
		if(addVariable(resultsRef,"BDNumBins",     INT_TYPE)==ND_FAILURE) return;
		if(setInt(resultsRef,"BDNumBins",0)==ND_FAILURE) return;
	} else {
		if(addVariable(resultsRef,"BDNumBins",     INT_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"BinnedDoses",   INT_ARRAY_2D_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"BDMinDoses",    FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"BDMaxDoses",    FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"BDMinResponses",FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
		if(addVariable(resultsRef,"BDMaxResponses",FLOAT_ARRAY_TYPE)==ND_FAILURE) return;

		if(setInt(resultsRef,"BDNumBins",x_mem->doseBins->numBins)==ND_FAILURE) return;
		if(set2DIntArray(resultsRef,"BinnedDoses",x_BinnedDoses,x_scenarioCount,x_mem->doseBins->numBins)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"BDMinDoses",x_mem->doseBins->doseBins,x_mem->doseBins->numBins)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"BDMaxDoses",&(x_mem->doseBins->doseBins[1]),x_mem->doseBins->numBins-1)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"BDMinResponses",x_mem->doseBins->responseBins,x_mem->doseBins->numBins)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"BDMaxResponses",&(x_mem->doseBins->responseBins[1]),x_mem->doseBins->numBins-1)==ND_FAILURE) return;
	}
	if(x_mem->dr != NULL) {
		if(setFloatArray(resultsRef,"TotalInfectedBySource", totalInfected,  x_scenarioCount)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"TotalFatalitiesBySource", totalFatalities,  x_scenarioCount)==ND_FAILURE) return;
		if(setIntArray(resultsRef,"HighFatalityNodesBySource", highFatalityNodes,  x_scenarioCount)==ND_FAILURE) return;
		if(setIntArray(resultsRef,"NumNodesWithFatalitiesBySource", numNodesWithFatalities,  x_scenarioCount)==ND_FAILURE) return;
	}
	qsort(maxConcentration,x_scenarioCount,sizeof(float),sortFloatAsc);
	if(setFloatArray(resultsRef,"MaxConcentration", maxConcentration,  x_scenarioCount)==ND_FAILURE) return;

	qsort(maxDosage,x_scenarioCount,sizeof(float),sortFloatAsc);
	if(setFloatArray(resultsRef,"MaxDosage", maxDosage,  x_scenarioCount)==ND_FAILURE) return;
	
	if(x_mem->dr != NULL) {
		qsort(totalInfected,x_scenarioCount,sizeof(float),sortFloatAsc);
		if(setFloatArray(resultsRef,"TotalInfected", totalInfected,  x_scenarioCount)==ND_FAILURE) return;
		
		qsort(totalFatalities,x_scenarioCount,sizeof(float),sortFloatAsc);
		if(setFloatArray(resultsRef,"TotalFatalities", totalFatalities,  x_scenarioCount)==ND_FAILURE) return;

		qsort(highFatalityNodes,x_scenarioCount,sizeof(int),sortFloatAsc);
		if(setIntArray(resultsRef,"HighFatalityNodes", highFatalityNodes,  x_scenarioCount)==ND_FAILURE) return;
		
		qsort(numNodesWithFatalities,x_scenarioCount,sizeof(int),sortFloatAsc);
		if(setIntArray(resultsRef,"NumNodesWithFatalities", numNodesWithFatalities,  x_scenarioCount)==ND_FAILURE) return;
	}
	if(x_mem->dot) {
		for(i=0;i<x_mem->dot->numThresh;i++) {
			qsort(x_totDoseOverThresh[i],x_scenarioCount,sizeof(int),sortFloatAsc);
		}
	}
	// not for this one ... yet, but needs to be sorted before summary is computed below.
      	// if(setIntArray(resultsRef,"NumNodesWithFatalities", numNodesWithFatalities,  x_scenarioCount)==ND_FAILURE) return;

	if(x_mem->dr != NULL) {
		int k;
		int *xvals=(int *)calloc(x_scenarioCount,sizeof(int));
		for(k=0;k<x_scenarioCount;k++) xvals[k]=k+1;
		if(setIntArray(resultsRef,"DummyXVals", xvals,  x_scenarioCount)==ND_FAILURE) return;
		free(xvals);

		if(setFloatArray(resultsRef,"WorstScenario_Dosage",   x_worstIDF->dos,nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"WorstScenario_Response", x_worstIDF->res,nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"WorstScenario_S",        x_worstIDF->s,  nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"WorstScenario_I",        x_worstIDF->i,  nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"WorstScenario_D",        x_worstIDF->d,  nsteps)==ND_FAILURE) return;
		if(setFloatArray(resultsRef,"WorstScenario_F",        x_worstIDF->f,  nsteps)==ND_FAILURE) return;
	}

//	if(getInt(analysisOptionsRef, "NumPercentiles", &numPercentiles)==ND_FAILURE) return;
//	if(getIntArray(analysisOptionsRef, "Percentiles", &percentiles)==ND_FAILURE) return;

	{
		float *tval = (float*)calloc(numPercentiles+1,sizeof(float));
		if(setStringArray(resultsRef,"RowNames",                  rowNames,numPercentiles+1)==ND_FAILURE) return;
		if(setStringArray(resultsRef,"DoseOverThresholdRowNames", dotPctRowNames,dotNumPercentiles+1)==ND_FAILURE) return;

		computeSummary(maxConcentration,x_scenarioCount,percentiles,tval,numPercentiles);
		if(setFloatArray(resultsRef,"MaxConcentrationSummary", tval,numPercentiles+1)==ND_FAILURE) return;

		computeSummary(maxDosage,x_scenarioCount,percentiles,tval,numPercentiles);
		if(setFloatArray(resultsRef,"MaxDosageSummary", tval,numPercentiles+1)==ND_FAILURE) return;

		if(x_mem->dr != NULL) {
			computeSummary(totalInfected,x_scenarioCount,percentiles,tval,numPercentiles);
			if(setFloatArray(resultsRef,"TotalInfectedSummary", tval,numPercentiles+1)==ND_FAILURE) return;

			computeSummary(totalFatalities,x_scenarioCount,percentiles,tval,numPercentiles);
			if(setFloatArray(resultsRef,"TotalFatalitiesSummary", tval,numPercentiles+1)==ND_FAILURE) return;
		}
		if(x_mem->dot) {
			int *tival = (int*)calloc(dotNumPercentiles+1,sizeof(int));
			for(i=0;i<numDThresh;i++) {
			  char varName[80];
			  sprintf(varName,"TotalDoseOverThresholdSummary_%02d",i);
			  computeIntegerSummary(x_totDoseOverThresh[i],x_scenarioCount,dotPercentiles,tival,dotNumPercentiles);
			  if(setIntArray(resultsRef,varName, tival,dotNumPercentiles+1)==ND_FAILURE) return;
			}
			for(i=0;i<numRThresh;i++) {
			  char varName[80];
			  sprintf(varName,"TotalResponseOverThresholdSummary_%02d",i);
			  computeIntegerSummary(x_totDoseOverThresh[i+numDThresh],x_scenarioCount,dotPercentiles,tival,dotNumPercentiles);
			  if(setIntArray(resultsRef,varName, tival,dotNumPercentiles+1)==ND_FAILURE) return;
			}
			free(tival);
		}
		free(tval);

		if(x_mem->dr != NULL) {
			SortCachedData(x_Fatalities);
			SortCachedData(x_sidfData);
		}
		SortCachedData(x_Dosages);
#if INCLUDE_DOT_PERCENTILES
		if(x_mem->dot) {
			for(i=0;i<x_mem->dot->numThresh;i++) {
				SortCachedData(x_NumDoseOverThresh[i]);
			}
		}
#endif
		if(x_mem->dr != NULL) {
			saveFatalityPercentiles(resultsRef,x_Fatalities,x_Dosages,x_sidfData,percentiles,rowNames,numPercentiles);
		}
#if INCLUDE_DOT_PERCENTILES
		if(x_mem->dot) {
			saveTotalDoseOverThreshPercentiles(resultsRef,x_NumDoseOverThresh,x_mem->dot->numThresh,percentiles,rowNames,numPercentiles);
		}
#endif
	}
	if(x_mem->dr != NULL) {
		addCumulativeDistributionPlotData(resultsRef,x_Fatalities);
	}
//	nd_free(percentiles);

}
void addCumulativeDistributionPlotData(NamedDataRef *resultsRef, DiskCachedDataPtr fatalities) 
{
	int num=fatalities->num;
	float *probs=(float*)calloc(num,sizeof(float));
	float *f=(float*)calloc(num,sizeof(float));
	int i;
	
	for(i=0;i<num;i++) {
		probs[i] = ((i+1)-0.375f) / (num+0.25f);
		f[i] = fatalities->data[i]->val;
	}
	if(addVariable(resultsRef,"CDP_Probabilities", FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	if(addVariable(resultsRef,"CDP_Fatalities", FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	if(setFloatArray(resultsRef,"CDP_Probabilities", probs,num)==ND_FAILURE) return;
	if(setFloatArray(resultsRef,"CDP_Fatalities", f,num)==ND_FAILURE) return;
	free(probs);
	free(f);
	
}
void computeSummary(float *scenData, int numScen, int* percentiles, float *rv, int numPercentiles)
{
	float tot=0;
	int i;
	for(i=0;i<numScen;i++) tot += scenData[i];
	for(i=0;i<numPercentiles;i++) {
		int idx = (int)ceil(percentiles[i]/100.0*numScen)-1;
		rv[i] = scenData[idx];
	}
	rv[numPercentiles] = tot/numScen;
}
void computeIntegerSummary(int *scenData, int numScen, int* percentiles, int *rv, int numPercentiles)
{
	int tot=0;
	int i;
	for(i=0;i<numScen;i++) tot += scenData[i];
	for(i=0;i<numPercentiles;i++) {
		int idx = (int)ceil(percentiles[i]/100.0*numScen)-1;
		rv[i] = scenData[idx];
	}
	rv[numPercentiles] = (int)(((tot*1.0f)/numScen)+0.5);
}
void saveTotalDoseOverThreshPercentiles(NamedDataRef *resultsRef, DiskCachedDataPtr *totDoseOverThreshBySrc, int numThresh, int* percentiles, char *percentileDescs[], int numPercentiles)
{
	int i,t;
	JNIEnv *env=resultsRef->env;
	for(t=0;t<numThresh;t++) {
	  int num = totDoseOverThreshBySrc[t]->num;
	  int *data = (int*)calloc(totDoseOverThreshBySrc[t]->size,sizeof(int));
	  for(i=0;i<numPercentiles;i++) {
		int n;
		char varName[1024];
		int totDoseOverThresh=0;
		int idx = (int)ceil(percentiles[i]/100.0*num)-1;
		
		sprintf(varName,"TotalDoseOverThreshold_%02d_ByReceptor-%s",t,percentileDescs[i]);
		getCachedData(totDoseOverThreshBySrc[t],idx,data,totDoseOverThreshBySrc[t]->size*sizeof(float));
		for(n=0;n<totDoseOverThreshBySrc[t]->size;n++) {
			totDoseOverThresh += data[n];
		}

		if(addVariable(resultsRef,varName, INT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setIntArray(resultsRef,varName, data,totDoseOverThreshBySrc[t]->size)==ND_FAILURE) return;

		sprintf(varName,"%s TotalDoseOverThreshold_%02d Injection",percentileDescs[i],t);
		if(addVariable(resultsRef,varName, STRING_TYPE )==ND_FAILURE) return;
		if(setString(resultsRef,varName, totDoseOverThreshBySrc[t]->data[idx]->simID)==ND_FAILURE) return;

		sprintf(varName,"%s TotalDoseOverThreshold_%02d",percentileDescs[i],t);
		if(addVariable(resultsRef,varName, INT_TYPE )==ND_FAILURE) return;
		if(setInt(resultsRef,varName, totDoseOverThresh)==ND_FAILURE) return;
		
		sprintf(varName,"%s pd_%02d InjectionDef",percentileDescs[i],t);
		if(addVariable(resultsRef,varName, STRING_TYPE )==ND_FAILURE) return;
		if(setString(resultsRef,varName, totDoseOverThreshBySrc[t]->data[idx]->injDef)==ND_FAILURE) return;
	  }
	  free(data);
	}
}
void saveFatalityPercentiles(NamedDataRef *resultsRef, DiskCachedDataPtr fatalitiesBySrc, DiskCachedDataPtr dosageBySrc, DiskCachedDataPtr sidf, int* percentiles, char *percentileDescs[], int numPercentiles)
{
	int i;
	int num = fatalitiesBySrc->num;
	float *data = (float*)calloc(fatalitiesBySrc->size,sizeof(float));
	float *xa_data = (float*)calloc(6*sidf->size,sizeof(float));

	PXA txa = (PXA)calloc(1,sizeof(XA));
	initXA(txa,sidf->size);
	for(i=0;i<numPercentiles;i++) {
		int n;
		float totFatalities=0;
		char varName[1024];
		int idx = (int)ceil(percentiles[i]/100.0*num)-1;
		sprintf(varName,"TotalFatalitiesByReceptor-%s",percentileDescs[i]);
		getCachedData(fatalitiesBySrc,idx,data,fatalitiesBySrc->size*sizeof(float));
		for(n=0;n<fatalitiesBySrc->size;n++) {
			data[n] *= x_mem->node[n].pop;
			totFatalities += data[n];
		}
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName, data,fatalitiesBySrc->size)==ND_FAILURE) return;

		sprintf(varName,"%s Injection",percentileDescs[i]);
		if(addVariable(resultsRef,varName, STRING_TYPE )==ND_FAILURE) return;
		if(setString(resultsRef,varName, fatalitiesBySrc->data[idx]->simID)==ND_FAILURE) return;
		
		sprintf(varName,"%s InjectionDef",percentileDescs[i]);
		if(addVariable(resultsRef,varName, STRING_TYPE )==ND_FAILURE) return;
		if(setString(resultsRef,varName, fatalitiesBySrc->data[idx]->injDef)==ND_FAILURE) return;
		
		sprintf(varName,"%s Fatalities",percentileDescs[i]);
		if(addVariable(resultsRef,varName, FLOAT_TYPE )==ND_FAILURE) return;
		if(setFloat(resultsRef,varName, totFatalities)==ND_FAILURE) return;

		getCachedData(sidf,idx,xa_data,sidf->size*6*sizeof(float));
		x_floatArrayToXA(txa,sidf->size,xa_data);

		sprintf(varName,"DosageByReceptor-%s",percentileDescs[i]);
		getCachedData(dosageBySrc,idx,data,dosageBySrc->size*sizeof(float));
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName, data,dosageBySrc->size)==ND_FAILURE) return;

		sprintf(varName,"%s Dosage Injection",percentileDescs[i]);
		if(addVariable(resultsRef,varName, STRING_TYPE )==ND_FAILURE) return;
		if(setString(resultsRef,varName, dosageBySrc->data[idx]->simID)==ND_FAILURE) return;

		sprintf(varName,"%s Dosage InjectionDef",percentileDescs[i]);
		if(addVariable(resultsRef,varName, STRING_TYPE )==ND_FAILURE) return;
		if(setString(resultsRef,varName, dosageBySrc->data[idx]->injDef)==ND_FAILURE) return;

		sprintf(varName,"%s_Dosage",percentileDescs[i]);
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName, txa->dos,sidf->size)==ND_FAILURE) return;

		sprintf(varName,"%s_Response",percentileDescs[i]);
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName, txa->res,sidf->size)==ND_FAILURE) return;

		sprintf(varName,"%s_S",percentileDescs[i]);
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName, txa->s,sidf->size)==ND_FAILURE) return;

		sprintf(varName,"%s_I",percentileDescs[i]);
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName, txa->i,sidf->size)==ND_FAILURE) return;

		sprintf(varName,"%s_D",percentileDescs[i]);
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName, txa->d,sidf->size)==ND_FAILURE) return;
		
		sprintf(varName,"%s_F",percentileDescs[i]);
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName, txa->f,sidf->size)==ND_FAILURE) return;

	}
	free(xa_data);
	freeXA(txa,sidf->size);
	free(txa);
	free(data);
}
int writeHIAFile(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
	int nt,
		i,
		nnodes,
		nscenarios,
		nsteps;
	XA xa;
	char **simids;
	float *tfa,*tda,*tra,*ttha;
	char fname[256];
	FILE *fp;
	JNIEnv *env=resultsRef->env;

	if(x_mem->dr == NULL) return 1;
	sprintf(fname,"%s/HIA.txt",dir);
	fp = fopen(fname,"w");
	if(fn[0]!='\0')
	  strcat(fn,",");
	strcat(fn,"HIA.txt");
		
	if(getInt(resultsRef,"Length_t",&nsteps)==ND_FAILURE) return 0;
	if(getInt(resultsRef,"Length_n",&nnodes)==ND_FAILURE) return 0;
	if(getInt(resultsRef,"NSteps",&nt)==ND_FAILURE) return 0;
	if(getInt(resultsRef,"NumScenarios",&nscenarios)==ND_FAILURE) return 0;

	initXA(&xa,nsteps);

	if(!x_initFloatArray(resultsRef,"Dose_t",    xa.dos,nsteps)) return 0;
	if(!x_initFloatArray(resultsRef,"Response_t",xa.res,nsteps)) return 0;
	if(!x_initFloatArray(resultsRef,"S_t",       xa.s,  nsteps)) return 0;
	if(!x_initFloatArray(resultsRef,"I_t",       xa.i,  nsteps)) return 0;
	if(!x_initFloatArray(resultsRef,"D_t",       xa.d,  nsteps)) return 0;
	if(!x_initFloatArray(resultsRef,"F_t",       xa.f,  nsteps)) return 0;
	
	fprintf(fp,"%d\n",nnodes);
	fprintf(fp,"%d\n",nsteps);
	fprintf(fp,"%d\n",nscenarios);
	for(i=0;i<nsteps;i++) fprintf(fp,"%f ",xa.s[i]);
	fprintf(fp,"\n");
	for(i=0;i<nsteps;i++) fprintf(fp,"%f ",xa.i[i]);
	fprintf(fp,"\n");
	for(i=0;i<nsteps;i++) fprintf(fp,"%f ",xa.d[i]);
	fprintf(fp,"\n");
	for(i=0;i<nsteps;i++) fprintf(fp,"%f ",xa.f[i]);
	fprintf(fp,"\n");
	for(i=0;i<nsteps;i++) fprintf(fp,"%f ",xa.dos[i]);
	fprintf(fp,"\n");
	for(i=0;i<nsteps;i++) fprintf(fp,"%f ",xa.res[i]);
	fprintf(fp,"\n");

	freeXA(&xa,nsteps);

	tfa = (float *)calloc(nscenarios,sizeof(float));
	tda = (float *)calloc(nscenarios,sizeof(float));
	tra = (float *)calloc(nscenarios,sizeof(float));
	ttha = (float *)calloc(nscenarios,sizeof(float));


	if(!x_initFloatArray(resultsRef,"Hist_F",     tfa,  nscenarios)) return 0;
	for(i=0;i<nscenarios;i++) fprintf(fp,"%f ",tfa[i]);
	fprintf(fp,"\n");
	if(!x_initFloatArray(resultsRef,"Hist_Dos",   tda,  nscenarios)) return 0;
	for(i=0;i<nscenarios;i++) fprintf(fp,"%f ",tda[i]);
	fprintf(fp,"\n");
	if(!x_initFloatArray(resultsRef,"Hist_Resp",   tra,  nscenarios)) return 0;
	for(i=0;i<nscenarios;i++) fprintf(fp,"%f ",tra[i]);
	fprintf(fp,"\n");
	if(!x_initFloatArray(resultsRef,"Hist_testh", ttha,  nscenarios)) return 0;
	for(i=0;i<nscenarios;i++) fprintf(fp,"%f ",ttha[i]);
	fprintf(fp,"\n");

	if(getStringArray(resultsRef,"Hist_SimID",&simids)==ND_FAILURE) return 0;
	for(i=0;i<nscenarios;i++) fprintf(fp,"%s ",simids[i]);
	fprintf(fp,"\n");
	nd_freeStringArray(simids,nscenarios);

	free(tfa);
	free(tda);
	free(tra);
	free(ttha);

	fclose(fp);  // close standard output file
	return 1;
}
int writeHIADataFile(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
	int i,
		nscenarios;
	char fname[256];
	FILE *fp;
	JNIEnv *env=resultsRef->env;
	float *tda, *tra, *maxConc, *maxDose;
	char **simids;
	char **injdefs;
	char **injNodeTypes;
	float *totInf, *totFatal;
    int *nFatalNodes;

	if(x_mem->dr == NULL) return 1;

	sprintf(fname,"%s/HIA_data.txt",dir);
	fp = fopen(fname,"w");
	if(fn[0]!='\0')
	  strcat(fn,",");
	strcat(fn,"HIA_data.txt");

	if(getInt(resultsRef,"NumScenarios",&nscenarios)==ND_FAILURE) return 0;

	tda = (float *)calloc(nscenarios,sizeof(float));
	tra = (float *)calloc(nscenarios,sizeof(float));
	maxConc           = (float *)calloc(nscenarios,sizeof(float));
	maxDose           = (float *)calloc(nscenarios,sizeof(float));
	totInf            = (float *)calloc(nscenarios,sizeof(float));
	totFatal          = (float *)calloc(nscenarios,sizeof(float));
	nFatalNodes       = (int   *)calloc(nscenarios,sizeof(int  ));

	if(!x_initFloatArray(resultsRef,"MaxConcentrationBySource",      maxConc,          nscenarios)) return 0;
	if(!x_initFloatArray(resultsRef,"MaxDosageBySource",             maxDose,          nscenarios)) return 0;
	if(!x_initFloatArray(resultsRef,"TotalInfectedBySource",         totInf,           nscenarios)) return 0;
	if(!x_initFloatArray(resultsRef,"TotalFatalitiesBySource",       totFatal,         nscenarios)) return 0;
	if(!x_initIntArray(  resultsRef,"NumNodesWithFatalitiesBySource",nFatalNodes,      nscenarios)) return 0;
	if(!x_initFloatArray(resultsRef,"Hist_Dos",   tda,  nscenarios)) return 0;
	if(!x_initFloatArray(resultsRef,"Hist_Resp",   tra,  nscenarios)) return 0;

	if(getStringArray(resultsRef,"Hist_SimID",&simids)==ND_FAILURE) return 0;
	if(getStringArray(resultsRef,"Hist_InjDef",&injdefs)==ND_FAILURE) return 0;
	if(getStringArray(resultsRef,"Hist_InjNodeType",&injNodeTypes)==ND_FAILURE) return 0;

	fprintf(fp,"ID\tType\tMax Concentration\tMax Dosage\tTotal Infected\tFatalities\tNodes for 90%% Fatalities\tTotal Dosage\tTotal Response\n");
	for(i=0;i<nscenarios;i++) {
	  fprintf(fp,"%s %s\t%s\t%f\t%f\t%f\t%f\t%d\t%f\t%f\n",
		  simids[i],injdefs[i],injNodeTypes[i],maxConc[i],maxDose[i],totInf[i],totFatal[i],nFatalNodes[i],tda[i],tra[i]);
	}

	fclose(fp);  // close standard output file
	nd_freeStringArray(simids,nscenarios);
	nd_freeStringArray(injdefs,nscenarios);
	nd_freeStringArray(injNodeTypes,nscenarios);
	free(maxConc          );
	free(maxDose          );
	free(totInf           );
	free(totFatal         );
	free(nFatalNodes      );
	free(tda);
	free(tra);
	return 1;
}

int writeDoseOverThreshold(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn, char *baseName, char *varNameBase) {
	char fname[256];
	FILE *fp;
//	char msg[256];
	char varName[256];
	JNIEnv *env=resultsRef->env;
	int numDThresh,i,j,nscenarios;
	char **simids,**injdefs,**injNodeTypes;
	float *tra=NULL;
	int **dot;
	float *resp;

	if(getInt(resultsRef,"NumScenarios",&nscenarios)==ND_FAILURE) return 0;
	if(getInt(analysisOptionsRef, "NumDoseThresholds", &numDThresh)==ND_FAILURE) return 0;
	if(numDThresh==0) return 1;
	if(getStringArray(resultsRef,"Hist_SimID",&simids)==ND_FAILURE) return 0;
	if(getStringArray(resultsRef,"Hist_InjDef",&injdefs)==ND_FAILURE) return 0;
	if(getStringArray(resultsRef,"Hist_InjNodeType",&injNodeTypes)==ND_FAILURE) return 0;
	if(x_mem->dr != NULL) {
		tra = (float *)calloc(nscenarios,sizeof(float));
		if(!x_initFloatArray(resultsRef,"Hist_Resp",   tra,  nscenarios)) return 0;
	}
	dot=(int **)calloc(numDThresh,sizeof(int *));
	for(i=0;i<numDThresh;i++) {
		sprintf(varName,"%s_%02d",varNameBase,i);
		if(getIntArray(resultsRef,varName,&dot[i])==ND_FAILURE) return 0;
	}
	sprintf(fname,"%s/%s",dir,baseName);
	fp = fopen(fname,"w");
	if(fn[0]!='\0')
	  strcat(fn,",");
	strcat(fn,baseName);
	fprintf(fp,"ID\tType\tResponse\tDose=>");
	for(i=0;i<numDThresh;i++) {
		float dose;
		sprintf(varName,"DoseThreshold_%02d",i);
		if(getFloat(resultsRef,varName,&dose)==ND_FAILURE) return 0;
		fprintf(fp,"\t%g",dose);
	}
	fprintf(fp,"\n");
	fprintf(fp,"\t\t\tResponse=>");
	if(x_mem->dr != NULL) {
		if(getFloatArray(resultsRef,"DoseThresholdResponses", &resp)==ND_FAILURE) return 0;
		for(i=0;i<numDThresh;i++) {
			fprintf(fp,"\t%g",resp[i]);
		}
		nd_free(resp);
	}
	fprintf(fp,"\n");
	for(i=0;i<nscenarios;i++) {
		fprintf(fp,"%s %s\t%s\t",simids[i],injdefs[i],injNodeTypes[i]);
		if(x_mem->dr != NULL) {
			fprintf(fp,"%f\t",tra[i]);
		} else {
			fprintf(fp,"\t");
		}
		for(j=0;j<numDThresh;j++) {
			fprintf(fp,"\t%d",dot[j][i]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);  // close standard output file
	if(tra != NULL) {
		free(tra);
	}
	nd_freeStringArray(simids,nscenarios);
	for(i=0;i<numDThresh;i++) {
		nd_free(dot[i]);
	}
	free(dot);
	return 1;
}
int writeResponseOverThreshold(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
	char fname[256];
	FILE *fp;
//	char msg[256];
	JNIEnv *env=resultsRef->env;
	char varName[256];
	int numRThresh,i,j,nscenarios;
	char **simids,**injdefs,**injNodeType;
	float *tra;
	int **dot;
	float *resp;

	if(getInt(resultsRef,"NumScenarios",&nscenarios)==ND_FAILURE) return 0;
	if(getInt(analysisOptionsRef, "NumResponseThresholds", &numRThresh)==ND_FAILURE) return 0;
	if(numRThresh==0) return 1;
	if(getStringArray(resultsRef,"Hist_SimID",&simids)==ND_FAILURE) return 0;
	if(getStringArray(resultsRef,"Hist_InjDef",&injdefs)==ND_FAILURE) return 0;
	if(getStringArray(resultsRef,"Hist_InjNodeType",&injNodeType)==ND_FAILURE) return 0;
	tra = (float *)calloc(nscenarios,sizeof(float));
	if(!x_initFloatArray(resultsRef,"Hist_Resp",   tra,  nscenarios)) return 0;
	dot=(int **)calloc(numRThresh,sizeof(int *));
	for(i=0;i<numRThresh;i++) {
		sprintf(varName,"TotalResponseOverThresholdBySource_%02d",i);
		if(getIntArray(resultsRef,varName,&dot[i])==ND_FAILURE) return 0;
	}

	sprintf(fname,"%s/HIA_ResponseOverThreshold.txt",dir);
	fp = fopen(fname,"w");
	if(fn[0]!='\0')
	  strcat(fn,",");
	strcat(fn,"HIA_ResponseOverThreshold.txt");

	fprintf(fp,"ID\tType\tResponse\tDose=>");
	for(i=0;i<numRThresh;i++) {
		float dose;
		sprintf(varName,"ResponseThreshold_%02d",i);
		if(getFloat(resultsRef,varName,&dose)==ND_FAILURE) return 0;
		fprintf(fp,"\t%g",dose);
	}
	fprintf(fp,"\n");
	fprintf(fp,"\t\t\tResponse=>");
	if(getFloatArray(resultsRef,"ResponseThresholdResponses", &resp)==ND_FAILURE) return 0;
	for(i=0;i<numRThresh;i++) {
		fprintf(fp,"\t%g",resp[i]);
	}
	nd_free(resp);
	fprintf(fp,"\n");
	for(i=0;i<nscenarios;i++) {
		fprintf(fp,"%s %s\t%s\t%f\t",simids[i],injdefs[i],injNodeType[i],tra[i]);
		for(j=0;j<numRThresh;j++) {
			fprintf(fp,"\t%d",dot[j][i]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);  // close standard output file
	free(tra);
	nd_freeStringArray(simids,nscenarios);
	for(i=0;i<numRThresh;i++) {
		nd_free(dot[i]);
	}
	free(dot);
	return 1;
}

int writeBinnedDoses(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
	char fname[256];
	FILE *fp;
	JNIEnv *env=resultsRef->env;
	int**binnedDoses;
	char **simids,**injdefs;
	char **injNodeTypes;
	int numBins,nscenarios,i,s;
	float *minD,*maxD,*minR,*maxR;


	if(getInt(resultsRef,"NumScenarios",&nscenarios)==ND_FAILURE) return 0;
	if(getInt(resultsRef,"BDNumBins",&numBins)==ND_FAILURE) return 0;
	if(numBins==0) return 1;
	if(getStringArray(resultsRef,"Hist_SimID",&simids)==ND_FAILURE) return 0;
	if(getStringArray(resultsRef,"Hist_InjDef",&injdefs)==ND_FAILURE) return 0;
	if(getStringArray(resultsRef,"Hist_InjNodeType",&injNodeTypes)==ND_FAILURE) return 0;
	binnedDoses=(int **)calloc(nscenarios,sizeof(int **));
	for(i=0;i<nscenarios;i++ ) {
		if(getIntArrayAtIndex(resultsRef,"BinnedDoses",i,&binnedDoses[i])==ND_FAILURE) return 0;
	}
	if(getFloatArray(resultsRef,"BDMinDoses",&minD)==ND_FAILURE) return 0;
	if(getFloatArray(resultsRef,"BDMaxDoses",&maxD)==ND_FAILURE) return 0;
	if(getFloatArray(resultsRef,"BDMinResponses",&minR)==ND_FAILURE) return 0;
	if(getFloatArray(resultsRef,"BDMaxResponses",&maxR)==ND_FAILURE) return 0;

	sprintf(fname,"%s/HIA_BinnedDoses.txt",dir);
	fp = fopen(fname,"w");
	if(fn[0]!='\0')
	  strcat(fn,",");
	strcat(fn,"HIA_BinnedDoses.txt");
	// first header line
	fprintf(fp,"ID\tType\tMinDose");
	for(i=0;i<numBins;i++) fprintf(fp,"\t%g",minD[i]);
	fprintf(fp,"\n");
	// second header line
	fprintf(fp,"\tMaxDose");
	for(i=0;i<numBins-1;i++) fprintf(fp,"\t%g",maxD[i]);
	fprintf(fp,"\n");
	// third header line
	fprintf(fp,"\tMinResponse");
	for(i=0;i<numBins;i++) fprintf(fp,"\t%g",minR[i]);
	fprintf(fp,"\n");
	// fourth header line
	fprintf(fp,"\tMaxResponse");
	for(i=0;i<numBins-1;i++) fprintf(fp,"\t%g",maxR[i]);
	fprintf(fp,"\n");
	for(s=0;s<nscenarios;s++) {
		fprintf(fp,"%s %s\t%s\t",simids[s],injdefs[s],injNodeTypes[s]);
		for(i=0;i<numBins;i++) {
			fprintf(fp,"\t%d",binnedDoses[s][i]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);  // close standard output file
	
	nd_freeStringArray(simids,nscenarios);
	nd_freeStringArray(injdefs,nscenarios);
	nd_freeStringArray(injNodeTypes,nscenarios);
	nd_free(minD);
	nd_free(maxD);
	nd_free(minR);
	nd_free(maxR);
	for(i=0;i<nscenarios;i++) {
		nd_free(binnedDoses[i]);
	}
	free(binnedDoses);
	return 1;
}
int writeWorstCaseFiles(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, WorstCasePtr wcp,const char *dir, char *fn,char *type, char *fullType) {
	FILE *fp;
	JNIEnv *env=resultsRef->env;
	int i,nt;
	WorstCaseDataPtr head;
	int calcDR = x_mem->dr != NULL;

	if(wcp == NULL) return 1;
	head = wcp->worst;
	if(head != NULL) {
		int nnodes;
		char **nodeIDs;
		float *pop;
		WorstCaseDataPtr wcdp;
		// write out individual files for worst scenarios
		if(getInt(resultsRef,"NNodes",&nnodes)==ND_FAILURE) return 0;
		if(getInt(resultsRef,"NSteps",&nt)==ND_FAILURE) return 0;
		if(getStringArray(resultsRef,"NodeIDs",&nodeIDs)==ND_FAILURE) return 0;
		if(getFloatArray(resultsRef,"Population",&pop)==ND_FAILURE) return 0;
		for(wcdp=head,i=0;wcdp != NULL;wcdp=wcdp->next,i++) {
			char fname[256];
			float **data,*dos,*res,*s,*ii,*d,*f;
			int n,t;
			WorstCaseCachedDataPtr wccd=(WorstCaseCachedDataPtr)calloc(1,sizeof(WorstCaseCachedData));

			getWorstCaseCachedData(wcp,wcdp,wccd);
			sprintf(fname,"%s/HIA_%s_%03d.txt",dir,type,i+1);
			fp=fopen(fname,"w");

			sprintf(fname,"HIA_%s_%03d.txt",type,i+1);
			if(fn[0]!='\0')
			  strcat(fn,",");
			strcat(fn,fname);

			fprintf(fp,"Simulation ID\t%s %s\t%s\n",wcdp->cData.simID,wcdp->cData.injDef,wcdp->cData.injNodeType);

			fprintf(fp,"Total %s\t%lf\n",fullType,wcdp->cData.val);

			dos=wccd->na.dos;
			res=wccd->na.res;
			s=wccd->na.s;
			ii=wccd->na.i;
			d=wccd->na.d;
			f=wccd->na.f;

			data=wccd->data;
			fprintf(fp,"receptor\tpop\tdosage\tresponse\ts\ti\td\tf");
			for(t=0;t<nt;t++) { fprintf(fp,"\t%d",t);}
			fprintf(fp,"\n");
			for(n=0;n<nnodes;n++) {
				fprintf(fp,"%s\t%f",nodeIDs[n],pop[n]);
				if(calcDR) {
					fprintf(fp,"\t%g\t%g\t%f\t%f\t%f\t%f",dos[n],res[n],s[n],ii[n],d[n],f[n]);
				} else {
					fprintf(fp,"\t%g\t\t\t\t\t",dos[n]);
				}
				for(t=0;t<nt;t++) {
					fprintf(fp,"\t%g",data[t][n]);
				}
				fprintf(fp,"\n");
			}
			wccdpFree(wccd,wcp->nsteps,nnodes);
			fclose(fp);
		}
		nd_freeStringArray(nodeIDs,nnodes);
		nd_free(pop);
	}
	return 1;
}
/* 
 * called usually only once after the scenario is done to obtain the completed analysis results
 */
void HIAnalysis_aggr_writeResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
		int nsteps;
	int calcDR = x_mem->dr!=NULL;

	JNIEnv *env=resultsRef->env;
	x_env=env;
	if(!writeHIAFile(analysisOptionsRef,resultsRef,dir,fn)) return;
	if(!writeHIADataFile(analysisOptionsRef,resultsRef,dir,fn)) return;
	if(!writeDoseOverThreshold(analysisOptionsRef,resultsRef,dir,fn,"HIA_DoseOverThreshold.txt","TotalDoseOverThresholdBySource")) return;
	if(!writeDoseOverThreshold(analysisOptionsRef,resultsRef,dir,fn,"HIA_NodesWithDoseOverThreshold.txt","TotalNodesOverThresholdBySource")) return;
	if(!writeResponseOverThreshold(analysisOptionsRef,resultsRef,dir,fn)) return;
	if(!writeBinnedDoses(analysisOptionsRef,resultsRef,dir,fn)) return;
	if(!writeWorstCaseFiles(analysisOptionsRef,resultsRef,x_worstConc,dir,fn,"conc","Fatalities")) return;
	if(!writeWorstCaseFiles(analysisOptionsRef,resultsRef,x_worstDosage,dir,fn,"dose","Dosage")) return;

	if(!writeOKCFile(analysisOptionsRef,resultsRef,dir,fn)) return;
	if(getInt(resultsRef,"NSteps",&nsteps)==ND_FAILURE) return;
	if(!writeAllIDFData(dir,fn,x_sidfData,nsteps)) return;
	x_env=NULL;
}
/*
typedef struct CachedData {
    float val;
	char *simID;
	long fileOffset;
} CachedData, *CachedDataPtr;

typedef struct DiskCachedData {
	FILE *fp;
	char fn[256];
	int num;
	size_t size;
	CachedDataPtr *data;
}DiskCachedData, *DiskCachedDataPtr;
*/
int writeAllIDFData(const char *dir, char *fn, DiskCachedDataPtr sidf,int nsteps) 
{
	char filename[256];
	int i,j;
	FILE *fp;
	float *xa_data;
	PXA txa;
	if(x_mem->dr == NULL) return 1;


	xa_data = (float*)calloc(6*sidf->size,sizeof(float));
	txa = (PXA)calloc(1,sizeof(XA));
	initXA(txa,sidf->size);

	
	sprintf(filename,"%s/IDF.txt",dir);
	fp = fopen(filename,"w");
	if(fn[0]!='\0')
	  strcat(fn,",");
	strcat(fn,"IDF.txt");
	
	for(i=0;i<sidf->num;i++) {
		CachedDataPtr cdp = sidf->data[i];
		fprintf(fp,"Injection: %s %s\n",cdp->simID,cdp->injDef);
		getCachedData(sidf,i,xa_data,sidf->size*6*sizeof(float));
		x_floatArrayToXA(txa,sidf->size,xa_data);
		fprintf(fp,"I"); for(j=0;j<sidf->size;j++) { fprintf(fp,"\t%f",txa->i[j]); } fprintf(fp,"\n");
		fprintf(fp,"D"); for(j=0;j<sidf->size;j++) { fprintf(fp,"\t%f",txa->d[j]); } fprintf(fp,"\n");
		fprintf(fp,"F"); for(j=0;j<sidf->size;j++) { fprintf(fp,"\t%f",txa->f[j]); } fprintf(fp,"\n");
	}
	fclose(fp);
	freeXA(txa,sidf->size);
	free(txa);
	free(xa_data);
	return 1;
}
int writeOKCFile(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn)
{
	int i,
		nnodes,
		nscenarios;
	float *tc,*td,*ti,*tf;
	int *tn;
	float minC,minD,minI,minF,minN;
	float maxC,maxD,maxI,maxF,maxN;
	FILE *fp;
	char fname[256];
	char **simids;

	if(x_mem->dr == NULL) return 1;

	if(getInt(resultsRef,"Length_n",&nnodes)==ND_FAILURE) return 0;
	if(getInt(resultsRef,"NumScenarios",&nscenarios)==ND_FAILURE) return 0;

	tc = (float *)calloc(nscenarios,sizeof(float));
	td = (float *)calloc(nscenarios,sizeof(float));
	ti = (float *)calloc(nscenarios,sizeof(float));
	tf = (float *)calloc(nscenarios,sizeof(float));
	tn = (int *)calloc(nscenarios,sizeof(int));

	if(!x_initFloatArray(resultsRef,"MaxConcentrationBySource",       tc,  nscenarios)) return 0;
	if(!x_initFloatArray(resultsRef,"MaxDosageBySource",              td,  nscenarios)) return 0;
	if(!x_initFloatArray(resultsRef,"TotalInfectedBySource",          ti,  nscenarios)) return 0;
	if(!x_initFloatArray(resultsRef,"TotalFatalitiesBySource",        tf,  nscenarios)) return 0;
	if(!x_initIntArray(  resultsRef,"NumNodesWithFatalitiesBySource", tn,  nscenarios)) return 0;
	if(getStringArray(resultsRef,"Hist_SimID",&simids)==ND_FAILURE) return 0;

	sprintf(fname,"%s/HIA.okc",dir);
	fp = fopen(fname,"w");
	if(fn[0]!='\0')
	  strcat(fn,",");
	strcat(fn,"HIA.okc");

	fprintf(fp,"5 %d\n",nscenarios);
	fprintf(fp,"MaxConcentration\n");
	fprintf(fp,"MaxDosage\n");
	fprintf(fp,"Infected\n");
	fprintf(fp,"Fatalities\n");
	fprintf(fp,"FatalNodes\n");

	minC=minD=minI=minF=minN=FLT_MAX;
	maxC=maxD=maxI=maxF=maxN=-FLT_MAX;
	for(i=0;i<nscenarios;i++) {
		if(tc[i]<minC) minC=tc[i];
		if(td[i]<minD) minD=td[i];
		if(ti[i]<minI) minI=ti[i];
		if(tf[i]<minF) minF=tf[i];
		if(tn[i]<minN) minN=(float)tn[i];

		if(tc[i]>maxC) maxC=tc[i];
		if(td[i]>maxD) maxD=td[i];
		if(ti[i]>maxI) maxI=ti[i];
		if(tf[i]>maxF) maxF=tf[i];
		if(tn[i]>maxN) maxN=(float)tn[i];
	}
	fprintf(fp," %f %f 1",minC*0.9,maxC*1.1);
	fprintf(fp," %f %f 1",minD*0.9,maxD*1.1);
	fprintf(fp," %f %f 1",minI*0.9,maxI*1.1);
	fprintf(fp," %f %f 1",minF*0.9,maxF*1.1);
	fprintf(fp," %f %f 1\n",minN*0.9,maxN*1.1);

	for(i=0;i<nscenarios;i++)
		fprintf(fp,"%f %f %f %f %f\n",tc[i],td[i],ti[i],tf[i],tn);

	nd_freeStringArray(simids,nscenarios);

	free(tc);
	free(td);
	free(ti);
	free(tf);
	free(tn);
	fclose(fp);
	return 1;
}
static float **getNodeData(void *ref,char *name,int nsteps,int nnodes) {
	int i;
	float **data = (float**)x_calloc(nsteps,sizeof(float*),"node_data");
	for(i=0;i<nsteps;i++) {
		float *tres;
		data[i] = (float*)x_calloc(nnodes,sizeof(float),"node_data[i]");
		/** IMPORTANT!!! Need to free these later via call to nd_free */
		if(getFloatArrayAtIndex(ref,name,i,&tres)==ND_FAILURE) {
			return NULL;
		}
		memcpy(data[i],tres,nnodes*sizeof(float));
		nd_free(tres);
	}
	return data;
}
void freeConcentrationData(float **conc, int nsteps) {
	int i;
	for(i=0;i<nsteps;i++) {
		free(conc[i]);
	}
	free(conc);
}
/* 
 * called once after the scenario has been completed
 */
void HIAnalysis_aggr_shutdown(NamedDataRef *analysisOptionsRef)
{
	int nnodes = x_mem->net->info->numNodes;
	int nsteps = x_mem->dr!=NULL?x_mem->dr->maxsteps:x_mem->net->info->numSteps;
	int i;
	char msg[256];

	JNIEnv *env = analysisOptionsRef->env;
	x_env=env;
	sprintf(msg,"Retrieved concentration data in %d of %d simulations",x_nRetrievals,x_nCalls);
	ANL_UTIL_LogInfo(env,"teva.analysis.server",msg);

	nd_free(x_nodeTypes);
	x_nodeTypes=NULL;
	freeHistogramData(x_mem->hist);
	freeXA(x_xa,nsteps);
	free(x_xa);
	if(x_worstIDF != NULL) {
		freeXA(x_worstIDF,nsteps);
		free(x_worstIDF);
	}
	if(x_worstConc != NULL) {
		freeWCDPData(&x_worstConc->worst);
		freeWorstCaseDiskCache(env,&x_worstConc);
	}
	if(x_worstDosage != NULL) {
		freeWCDPData(&x_worstDosage->worst);
		freeWorstCaseDiskCache(env,&x_worstDosage);
	}
	if(x_mem->dot) {
		for(i=0;i<x_mem->dot->numThresh;i++) {
			free(x_totDoseOverThresh[i]);
			free(x_nodesWithDoseOverThresh[i]);
#if INCLUDE_DOT_PERCENTILES
			freeDiskCachedData(analysisOptionsRef->env,&x_NumDoseOverThresh[i],x_numScenarios);
#endif
		}
		free(x_totDoseOverThresh);
		free(x_nodesWithDoseOverThresh);
#if INCLUDE_DOT_PERCENTILES
		free(x_NumDoseOverThresh);
#endif
	}
	if(x_BinnedDoses != NULL) {
		for(i=0;i<x_numScenarios;i++) {
			free(x_BinnedDoses[i]);
		}
		free(x_BinnedDoses);
	}
	if(x_Fatalities != NULL) {
		freeDiskCachedData(analysisOptionsRef->env,&x_Fatalities,x_numScenarios);
	}
	if(x_Dosages != NULL) {
		freeDiskCachedData(analysisOptionsRef->env,&x_Dosages,x_numScenarios);
	}
	if(x_sidfData != NULL) {
		freeDiskCachedData(analysisOptionsRef->env,&x_sidfData,x_numScenarios);
	}
	free(maxConcentration);
	free(maxDosage);
	if(totalInfected != NULL) free(totalInfected);
	if(totalFatalities != NULL) free(totalFatalities);
	if(highFatalityNodes != NULL) free(highFatalityNodes);
	if(numNodesWithFatalities != NULL) free(numNodesWithFatalities);

	// should be last in case any others need data from x_mem
	freeHIAMemory(x_mem,Aggregation);
//	btFree(&x_worstScenarios,cdpFree);
	x_env=NULL;
}

static int x_initFloatArray(void *ref, char *varName, float *data, int len) {
	float *tres;
	if(getFloatArray(ref,varName,&tres)==ND_FAILURE) return ND_FAILURE;
	memcpy(data,tres,sizeof(float)*len);
	nd_free(tres);
	return ND_SUCCESS;
}


static int x_initIntArray(void *ref, char *varName, int *data, int len) {
	int *tres;
	if(getIntArray(ref,varName,&tres)==ND_FAILURE) return ND_FAILURE;
	memcpy(data,tres,sizeof(int)*len);
	nd_free(tres);
	return ND_SUCCESS;
}

int sortFloatAsc(const void *a, const void *b)
{
	float f=*((float *)a) - *((float *)b);
	return (f<0?-1:(f>0?1:0));
}
int sortIntAsc(const void *a, const void *b)
{
	return *((int*)a) - *((int*)b);
}

static void x_xaToFloatArray(PXA xa,int nsteps,float *xa_data)
{
	memcpy(&xa_data[0*nsteps],xa->dos,nsteps*sizeof(float));
	memcpy(&xa_data[1*nsteps],xa->res,nsteps*sizeof(float));
	memcpy(&xa_data[2*nsteps],xa->s  ,nsteps*sizeof(float));
	memcpy(&xa_data[3*nsteps],xa->i  ,nsteps*sizeof(float));
	memcpy(&xa_data[4*nsteps],xa->d  ,nsteps*sizeof(float));
	memcpy(&xa_data[5*nsteps],xa->f  ,nsteps*sizeof(float));
}
// xa is assumed to be properly allocated
static void x_floatArrayToXA(PXA xa,int nsteps,float *xa_data)
{
	memcpy(xa->dos,&xa_data[0*nsteps],nsteps*sizeof(float));
	memcpy(xa->res,&xa_data[1*nsteps],nsteps*sizeof(float));
	memcpy(xa->s  ,&xa_data[2*nsteps],nsteps*sizeof(float));
	memcpy(xa->i  ,&xa_data[3*nsteps],nsteps*sizeof(float));
	memcpy(xa->d  ,&xa_data[4*nsteps],nsteps*sizeof(float));
	memcpy(xa->f  ,&xa_data[5*nsteps],nsteps*sizeof(float));
}
