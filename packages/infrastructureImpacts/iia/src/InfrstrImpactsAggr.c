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
#if defined(__linux__) || defined(__CYGWIN__)
#include <errno.h>
#endif
#endif
#include <float.h>
#include <sys/stat.h>
#include "loggingUtils.h"
#include "DiskCachedData.h"
#include "InfstrImpacts.h"
#include "ExternalAnalysis.h"
#include "NamedData.h"

static PII_Data x_iid;
static int x_scenarioCount;
static int x_numScenarios;
static int x_hasMultipleInjections;
static PIIAResults x_prevIIAResults;
static float **x_feetOverThresh; // dimensions: [threshold][scenario]
static int **x_linksOverThresh;  // dimensions: [threshold][scenario]
static float ***x_feetOverThreshByDiam; // dimensions: [threshold][diameter][scenario]
static int ***x_linksOverThreshByDiam;  // dimensions: [threshold][diameter][scenario]

static DiskCachedDataPtr *x_PipeFeetOverThresh;

static JNIEnv *x_env;

int sortFloatAsc(const void *a, const void *b);
//int sortIntAsc(const void *a, const void *b);

//static int x_initFloatArray(void *ref, char *varName, float *data, int len);
//static int x_initIntArray(void *ref, char *varName, int *data, int len);
//static float **getNodeData(void *ref,char *name,int nsteps,int nnodes);
void computeSummary(float *scenData, int numScen, int* percentiles, float *rv, int numPercentiles);
void savePipeFeetOverThreshPercentiles(NamedDataRef *resultsRef, DiskCachedDataPtr *pipeFeetOverThreshBySrc, int numThresh, int* percentiles, char *percentileDescs[], int numPercentiles);
static __file_pos_t x_getFilePosition(FILE *fp);

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

/* 
 * called once after the first simulaiton has been completed (so demands are there)
 */
void IIAnalysis_aggr_initialize(NamedDataRef *analysisOptionsRef, NamedDataRef *simResultsRef)
{
	// at this point, simResultsRef does not conatin any quality, but will contain
	// demands however
	int i;
	x_env=analysisOptionsRef->env;
	if(getInt(simResultsRef, "NumSimulations", &x_numScenarios)==ND_FAILURE) return;
	x_scenarioCount=0;

	x_iid = loadIIAOptions(analysisOptionsRef, simResultsRef,Aggregation);
	x_feetOverThresh=(float **)x_calloc(x_iid->pot->numThresh,sizeof(float *),"x_feetOverThresh");
	x_linksOverThresh=(int **)x_calloc(x_iid->pot->numThresh,sizeof(int *),"x_linksOverThresh");
	for(i=0;i<x_iid->pot->numThresh;i++) {
	  x_feetOverThresh[i]=(float *)x_calloc(x_numScenarios,sizeof(float)," x_feetOverThresh[i]");
	  x_linksOverThresh[i]=(int *)x_calloc(x_numScenarios,sizeof(int)," x_linksOverThresh[i]");
	}

	x_PipeFeetOverThresh = (DiskCachedDataPtr*)x_calloc(x_iid->pot->numThresh,sizeof(DiskCachedDataPtr),"x_PipeFeetOverThresh");
	for(i=0;i<x_iid->pot->numThresh;i++) {
	  char fn[80];
	  sprintf(fn,"pipefeetover_%02d.cache",i);
	  x_PipeFeetOverThresh[i]=initDiskCachedData(x_env,fn,x_iid->net->numLinks,x_numScenarios);
	}
	x_prevIIAResults=NULL;
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


/*
 * called once after every simulation with the results of the intermediate analysis
 */
void IIAnalysis_aggr_newResults(NamedDataRef *analysisOptionsRef,NamedDataRef *intResultsRef)
{
	int i;
	float *tpfot;
	  int *nlot;
	int **lotdd;
	float **pfotd;
	char *tstr;
	PIIAResults iiaRes = NULL;
	JNIEnv *env = analysisOptionsRef->env;
	int numInj,nlinks;
	char *msg=calloc(256,sizeof(char));
	char *simID;
	x_env=env;

	nlinks = x_iid->net->numLinks;
	if(getInt(intResultsRef,"numInjections", &numInj)==ND_FAILURE) return;
	if(numInj>1) x_hasMultipleInjections=1;

	if(getString(intResultsRef,"SimID", &simID)==ND_FAILURE) return;
	iiaRes = allocateIIAResultsData(simID,x_iid);
	nd_free(simID);

	if(x_iid->pot->potd.totalFeet==0) {
		// first time through - get all pipe feet data;
		// have to do the initializations related to numDiameters here because
		// numDiams is not passed in module options or simResults.
		float *pfd,*pd;
		int numDiameters;
		int j;
		if(getInt(intResultsRef,"NumDiameters", &numDiameters)==ND_FAILURE) return;
		if(getFloat(intResultsRef,"TotalPipeFeet",&x_iid->pot->potd.totalFeet)==ND_FAILURE) return;
		if(getFloatArray(intResultsRef,"TotalPipeFeetByDiameter",&pfd)==ND_FAILURE) return;
		if(getFloatArray(intResultsRef,"PipeDiameters",&pd)==ND_FAILURE) return;
		for(j=0;j<numDiameters;j++) {
			addPOT(pd[j], x_iid->pot, j);
			x_iid->pot->potdDiam[j]->potd.totalFeet = pfd[j];
		}

		nd_free(pfd);
		nd_free(pd);
		x_feetOverThreshByDiam=(float ***)x_calloc(x_iid->pot->numThresh,sizeof(float **),"x_feetOverThreshByDiam");
		x_linksOverThreshByDiam=(int ***)x_calloc(x_iid->pot->numThresh,sizeof(int **),"x_linksOverThreshByDiam");
		for(i=0;i<x_iid->pot->numThresh;i++) {
			x_feetOverThreshByDiam[i]=(float **)x_calloc(x_iid->pot->numDiameters,sizeof(float *)," x_feetOverThreshByDiam[i]");
			x_linksOverThreshByDiam[i]=(int **)x_calloc(x_iid->pot->numDiameters,sizeof(int *)," x_linksOverThreshByDiam[i]");
			for(j=0;j<x_iid->pot->numDiameters;j++) {
				x_feetOverThreshByDiam[i][j]=(float *)x_calloc(x_numScenarios,sizeof(float)," x_feetOverThreshByDiam[i][j]");
				x_linksOverThreshByDiam[i][j]=(int *)x_calloc(x_numScenarios,sizeof(int)," x_linksOverThreshByDiam[i][j]");
			}
		}
	}
	if(x_prevIIAResults == NULL)
		x_iid->iiaResults = iiaRes;
	else
		x_prevIIAResults->next = iiaRes;
	x_prevIIAResults = iiaRes;
	
	msg=realloc(msg,(strlen(iiaRes->simID)+32)*sizeof(char));
	sprintf(msg," processing %s",iiaRes->simID);
	ANL_UTIL_LogFine(env,"teva.analysis.server",msg);

	if(getString(intResultsRef,"InjDef", &(tstr))==ND_FAILURE) return;
	iiaRes->injDef=(char*)calloc(strlen(tstr)+1,sizeof(char));
	strcpy(iiaRes->injDef,tstr);
	nd_free(tstr);

	x_scenarioCount++;
	x_iid->nscenario++;
	lotdd=(int **)calloc(x_iid->pot->numThresh,sizeof(int));
	pfotd=(float **)calloc(x_iid->pot->numThresh,sizeof(float));
	if(getFloatArray(intResultsRef,"TotPipeFeetOverThreshold",&tpfot)==ND_FAILURE) return;
	if(getIntArray(intResultsRef,"LinksOverThreshold",&nlot)==ND_FAILURE) return;
	for(i=0;i<x_iid->pot->numThresh;i++) {
	  int j;
	  float *pfot;
      float *tpfotd;
	  int *nlotd;
	  char typeS[32];

	  if(x_scenarioCount==21)
		  printf("");
	  iiaRes->pfot[i]=tpfot[i];
	  
	  if(getFloatArrayAtIndex(intResultsRef,"TotPipeFeetOverThresholdByDiameter",i,&tpfotd)==ND_FAILURE) return;
	  if(getIntArrayAtIndex(intResultsRef,"LinksOverThresholdByDiameter",i,&nlotd)==ND_FAILURE) return;
	  for(j=0;j<x_iid->pot->numDiameters;j++) {
	    x_feetOverThreshByDiam[i][j][x_scenarioCount-1]=tpfotd[j];
	    x_linksOverThreshByDiam[i][j][x_scenarioCount-1]=nlotd[j];
	  }
	  nd_free(tpfotd);
	  nd_free(nlotd);

	  x_feetOverThresh[i][x_scenarioCount-1]=tpfot[i];
	  x_linksOverThresh[i][x_scenarioCount-1]=nlot[i];

	  if(getFloatArrayAtIndex(intResultsRef,"PipeFeetOverThresholds",i,&pfot)==ND_FAILURE) return;
	  sprintf(typeS,"pfot_%02d",i);
	  cacheData(x_env,x_PipeFeetOverThresh[i],(float)x_feetOverThresh[i][x_scenarioCount-1],iiaRes->simID,iiaRes->injDef,pfot,nlinks*sizeof(float),typeS);
	  nd_free(pfot);
	}
	nd_free(tpfot);
	nd_free(nlot);

	x_env=NULL;
	free(msg);
}

static int x_getnodeindex(PNetInfo net,PNodeInfo node,char *nid)
{
    int i;

    for(i=0;i<net->numNodes;i++) {
        if(!strcmp(nid,node[i].id)) return(i);
    }
    return(-1);
}


/* 
 * called usually only once after the scenario is done to obtain the completed analysis results
 */
void IIAnalysis_aggr_getResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef)
{
	int i,j;
	PIIAResults tiiaRes;
	int percentiles[]={10, 25, 50, 75, 90, 100};
	char *rowNames[]={"10th percentile", "25th percentile", "50th percentile", "75th percentile", "90th percentile", "100th percentile", "Mean"};
	int numPercentiles=6;
	int numThresh;
	int numDiameters;
	float *ct_val;
	char** ct_id, **diam_id;
	char **simids, **injdefs;
	char *nodeDisp;
	char **ids;
	float **pf;
	int **nl;
	float totalFeet=x_iid->pot->potd.totalFeet;
	PIIAResults iiar;
	int nnodes=x_iid->net->numNodes;
	int nlinks=x_iid->net->numLinks;
	JNIEnv *env=resultsRef->env;
	x_env=env;


	if(addVariable(resultsRef,"NumScenarios",  INT_TYPE)==ND_FAILURE) return;
	if(setInt(resultsRef,"NumScenarios",x_iid->nscenario) ==ND_FAILURE) return;

	simids=(char **)calloc(x_iid->nscenario,sizeof(char *));
	injdefs=(char **)calloc(x_iid->nscenario,sizeof(char *));
	i=0;
	for(iiar=x_iid->iiaResults; iiar != NULL;iiar=iiar->next) {
		simids[i]=iiar->simID;
		injdefs[i]=iiar->injDef;
		i++;
	}
	if(addVariable(resultsRef,"SimIDs",     STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(setStringArray(resultsRef,"SimIDs",simids,x_iid->nscenario) ==ND_FAILURE) return;
	if(addVariable(resultsRef,"InjDefs",    STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(setStringArray(resultsRef,"InjDefs",injdefs,x_iid->nscenario) ==ND_FAILURE) return;
	free(simids);
	free(injdefs);

	numDiameters=x_iid->pot->numDiameters;
	numThresh=x_iid->pot->numThresh;
	ct_id=(char **)calloc(numThresh,sizeof(char *));
	ct_val=(float *)calloc(numThresh,sizeof(float));
	for(i=0;i<numThresh;i++) {
	  char varName[80];
	  ct_id[i]=(char *)calloc(3,sizeof(char));
	  sprintf(ct_id[i],"%02d",i);
	  ct_val[i]=x_iid->pot->thresholds[i];
	  sprintf(varName,"ConcentrationThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, FLOAT_TYPE) ==ND_FAILURE) return;
	  if(setFloat(resultsRef,varName,x_iid->pot->thresholds[i]) ==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"ct_val", FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	if(setFloatArray(resultsRef,"ct_val",ct_val,numThresh) ==ND_FAILURE) return;
	if(addVariable(resultsRef,"ct_id", STRING_ARRAY_TYPE) ==ND_FAILURE) return;
	if(setStringArray(resultsRef,"ct_id",ct_id,numThresh) ==ND_FAILURE) return;
	for(i=0;i<numThresh;i++) {
	  free(ct_id[i]);
	}
	free(ct_id);
	free(ct_val);

	ids = (char **)calloc(x_iid->net->numNodes,sizeof(char*));
	for(i=0;i<nnodes;i++) {
		ids[i]=x_iid->nodeinfo[i].id;
	}
	if(addVariable(resultsRef,"NodeIDs",      STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(setStringArray(resultsRef,"NodeIDs",ids,nnodes)==ND_FAILURE) return;
	free(ids);
	ids = (char **)calloc(x_iid->net->numLinks,sizeof(char*));
	for(i=0;i<nlinks;i++) {
		ids[i]=x_iid->linkinfo[i].id;
	}
	if(addVariable(resultsRef,"LinkIDs",      STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(setStringArray(resultsRef,"LinkIDs",ids,nlinks)==ND_FAILURE) return;
	free(ids);

	nl=(int **)calloc(x_iid->pot->numThresh,sizeof(int *));
	pf=(float **)calloc(x_iid->pot->numThresh,sizeof(float *));
	for(i=0;i<x_iid->pot->numThresh;i++) {
		nl[i]=(int *)calloc(x_iid->net->numNodes,sizeof(int));
		pf[i]=(float *)calloc(x_iid->net->numNodes,sizeof(float));
	}
	nodeDisp = (char*)calloc(x_iid->net->numNodes,sizeof(char));
	for(i=0,tiiaRes=x_iid->iiaResults;tiiaRes != NULL;tiiaRes=tiiaRes->next,i++) {
		int idx = x_getnodeindex(x_iid->net,x_iid->nodeinfo,tiiaRes->simID);
		if(idx > -1) {
			nodeDisp[idx] = 1;
			for(j=0;j<x_iid->pot->numThresh;j++) {
				pf[j][idx]=x_feetOverThresh[j][i];
				nl[j][idx]=x_linksOverThresh[j][i];
			}
		}
	}

	if(!x_hasMultipleInjections) {
		if(addVariable(resultsRef,"InjNodeDisplay",      BYTE_ARRAY_TYPE)==ND_FAILURE) return;
		if(setByteArray(resultsRef,"InjNodeDisplay", nodeDisp,  nnodes)==ND_FAILURE) return;
	}
	free(nodeDisp);

	if(addVariable(  resultsRef,"ConcentrationThresholds", FLOAT_ARRAY_TYPE)==ND_FAILURE) return;
	if(setFloatArray(resultsRef,"ConcentrationThresholds",x_iid->pot->thresholds,numThresh)==ND_FAILURE) return;
	if(addVariable(resultsRef,"NumThresholds", INT_TYPE)==ND_FAILURE) return;
	if(setInt(     resultsRef,"NumThresholds",numThresh)==ND_FAILURE) return;
	if(addVariable(resultsRef,"PipeFeet", FLOAT_TYPE)==ND_FAILURE) return;
	if(setFloat(   resultsRef,"PipeFeet",x_iid->pot->potd.totalFeet)==ND_FAILURE) return;
	
	diam_id=(char **)calloc(numDiameters,sizeof(char *));
	for(i=0;i<numDiameters;i++) {
	  char varName[80];
	  diam_id[i]=(char *)calloc(3,sizeof(char));
	  sprintf(diam_id[i],"%02d",i);
	  sprintf(varName,"PipeDiameter_%02d",i);
	  if(addVariable(resultsRef,varName, FLOAT_TYPE) ==ND_FAILURE) return;
	  if(setFloat(resultsRef,varName,x_iid->pot->potdDiam[i]->diameter) ==ND_FAILURE) return;
	  sprintf(varName,"PipeFeet_%02d",i);
	  if(addVariable(resultsRef,varName, FLOAT_TYPE) ==ND_FAILURE) return;
	  if(setFloat(resultsRef,varName,x_iid->pot->potdDiam[i]->potd.totalFeet) ==ND_FAILURE) return;
	}
	if(addVariable(resultsRef,"diam_id", STRING_ARRAY_TYPE) ==ND_FAILURE) return;
	if(setStringArray(resultsRef,"diam_id",diam_id,numDiameters) ==ND_FAILURE) return;
	for(i=0;i<x_iid->pot->numDiameters;i++) {
	  free(diam_id[i]);
	}
	free(diam_id);

	for(i=0;i<numThresh;i++) {
	  char varName[80];
	  float *pct=(float *)calloc(x_scenarioCount,sizeof(float));
	  for(j=0;j<x_scenarioCount;j++) {
		  pct[j]=x_feetOverThresh[i][j]/totalFeet;
	  }
	  sprintf(varName,"PctPipeFeetOverThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	  if(setFloatArray(resultsRef,varName,  pct,  x_scenarioCount)==ND_FAILURE) return;
	  sprintf(varName,"MapPipeFeetOverThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	  if(setFloatArray(resultsRef,varName,  pf[i],  x_iid->net->numNodes)==ND_FAILURE) return;
	  sprintf(varName,"PipeFeetOverThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
	  if(setFloatArray(resultsRef,varName,  x_feetOverThresh[i],  x_scenarioCount)==ND_FAILURE) return;
	  sprintf(varName,"MapNumLinksOverThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	  if(setIntArray(resultsRef,varName,  nl[i],  x_iid->net->numNodes)==ND_FAILURE) return;
	  sprintf(varName,"NumLinksOverThreshold_%02d",i);
	  if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
	  if(setIntArray(resultsRef,varName,  x_linksOverThresh[i],  x_scenarioCount)==ND_FAILURE) return;
	  for(j=0;j<numDiameters;j++) {
	    int k;
		float ft=x_iid->pot->potdDiam[j]->potd.totalFeet;
		memset(pct,0,x_scenarioCount*sizeof(float));
		for(k=0;k<x_scenarioCount;k++) {
		  pct[k]=x_feetOverThreshByDiam[i][j][k]/ft;
		}
		sprintf(varName,"PctPipeFeetOverThreshold_%02d_Diameter_%02d",i,j);
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName,  pct,  x_scenarioCount)==ND_FAILURE) return;
		sprintf(varName,"PipeFeetOverThreshold_%02d_Diameter_%02d",i,j);
		if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setFloatArray(resultsRef,varName,  x_feetOverThreshByDiam[i][j],  x_scenarioCount)==ND_FAILURE) return;
		sprintf(varName,"NumLinksOverThreshold_%02d_Diameter_%02d",i,j);
		if(addVariable(resultsRef,varName, INT_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setIntArray(resultsRef,varName,  x_linksOverThreshByDiam[i][j],  x_scenarioCount)==ND_FAILURE) return;
	  }
	  free(pct);
	}
	for(i=0;i<x_iid->pot->numThresh;i++) {
		free(nl[i]);
		free(pf[i]);
	}
	free(nl);
	free(pf);

	for(i=0;i<x_iid->pot->numThresh;i++) {
		qsort(x_feetOverThresh[i],x_scenarioCount,sizeof(int),sortFloatAsc);
	}

	{
		float *tval = (float*)calloc(numPercentiles+1,sizeof(float));
		if(addVariable(resultsRef,"RowNames", STRING_ARRAY_TYPE) ==ND_FAILURE) return;
		if(setStringArray(resultsRef,"RowNames", rowNames,numPercentiles+1)==ND_FAILURE) return;

		for(i=0;i<numThresh;i++) {
		  char varName[80];
		  computeSummary(x_feetOverThresh[i],x_scenarioCount,percentiles,tval,numPercentiles);
		  sprintf(varName,"TotalPipeFeetOverThresholdSummary_%02d",i);
		  if(addVariable(resultsRef,varName, FLOAT_ARRAY_TYPE) ==ND_FAILURE) return;
		  if(setFloatArray(resultsRef,varName, tval,numPercentiles+1)==ND_FAILURE) return;
		}
		free(tval);

		for(i=0;i<x_iid->pot->numThresh;i++) {
			SortCachedData(x_PipeFeetOverThresh[i]);
		}
		savePipeFeetOverThreshPercentiles(resultsRef,x_PipeFeetOverThresh,x_iid->pot->numThresh,percentiles,rowNames,numPercentiles);
	}
	x_env=NULL;
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
void savePipeFeetOverThreshPercentiles(NamedDataRef *resultsRef, DiskCachedDataPtr *pipeFeetOverThreshBySrc, int numThresh, int* percentiles, char *percentileDescs[], int numPercentiles)
{
	int i,t;
	JNIEnv *env=resultsRef->env;
	for(t=0;t<numThresh;t++) {
	  int num = pipeFeetOverThreshBySrc[t]->num;
	  float *data = (float*)calloc(pipeFeetOverThreshBySrc[t]->size,sizeof(float));
	  char **cdata = (char **)calloc(pipeFeetOverThreshBySrc[t]->size,sizeof(char *));
	  for(i=0;i<numPercentiles;i++) {
		int n;
		char varName[1024];
		float pipeFeetOverThresh=0;
		int idx = (int)ceil(percentiles[i]/100.0*num)-1;
		
		sprintf(varName,"PipeFeetOverThreshold_%02d_ByReceptor-%s",t,percentileDescs[i]);
		getCachedData(pipeFeetOverThreshBySrc[t],idx,data,pipeFeetOverThreshBySrc[t]->size*sizeof(float));
		for(n=0;n<pipeFeetOverThreshBySrc[t]->size;n++) {
			pipeFeetOverThresh += data[n];
			cdata[n]=(data[n] > 0?"O":"U");
		}

		if(addVariable(resultsRef,varName, STRING_ARRAY_TYPE )==ND_FAILURE) return;
		if(setStringArray(resultsRef,varName, cdata, pipeFeetOverThreshBySrc[t]->size)==ND_FAILURE) return;

		sprintf(varName,"%s PipeFeetOverThreshold_%02d Injection",percentileDescs[i],t);
		if(addVariable(resultsRef,varName, STRING_TYPE )==ND_FAILURE) return;
		if(setString(resultsRef,varName, pipeFeetOverThreshBySrc[t]->data[idx]->simID)==ND_FAILURE) return;
		
		sprintf(varName,"%s PipeFeetOverThreshold_%02d",percentileDescs[i],t);
		if(addVariable(resultsRef,varName, FLOAT_TYPE )==ND_FAILURE) return;
		if(setFloat(resultsRef,varName, pipeFeetOverThresh)==ND_FAILURE) return;
		
		sprintf(varName,"%s ct_%02d InjectionDef",percentileDescs[i],t);
		if(addVariable(resultsRef,varName, STRING_TYPE )==ND_FAILURE) return;
		if(setString(resultsRef,varName, pipeFeetOverThreshBySrc[t]->data[idx]->injDef)==ND_FAILURE) return;
	  }
	  free(data);
	  free(cdata);
	}
}
int writeIIAFile(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
	int i,
		numThresh,
		nscenarios;
	char **simids;
	float *thresholds;
	float **pipeFeetOverThresh;
	char fname[256];
	FILE *fp;
	JNIEnv *env=resultsRef->env;

	sprintf(fname,"%s/IIA.txt",dir);
	fp = fopen(fname,"w");
	if(fn[0]!='\0')
	  strcat(fn,",");
	strcat(fn,"IIA.txt");

	if(getInt(resultsRef,"NumScenarios",&nscenarios)==ND_FAILURE) return 0;
	if(getInt(resultsRef,"NumThresholds",&numThresh)==ND_FAILURE) return 0;
	if(getFloatArray(resultsRef,"ConcentrationThresholds",&thresholds)==ND_FAILURE) return 0;
	pipeFeetOverThresh=(float **)calloc(numThresh,sizeof(float*));
	for(i=0;i<numThresh;i++) {
		char varName[32];
		sprintf(varName,"PipeFeetOverThreshold_%02d",i);
		if(getFloatArray(resultsRef,varName,&pipeFeetOverThresh[i])==ND_FAILURE) return 0;
	}
	if(getStringArray(resultsRef,"SimIDs",&simids)==ND_FAILURE) return 0;
	fprintf(fp,"SimID");
	for(i=0;i<numThresh;i++) {
		fprintf(fp,"\t%f",thresholds[i]);
	}
	fprintf(fp,"\n");
	for(i=0;i<nscenarios;i++) {
		int t;
		fprintf(fp,"%s",simids[i]);
		for(t=0;t<numThresh;t++) {
			fprintf(fp,"\t%f",pipeFeetOverThresh[t][i]);
		}
		fprintf(fp,"\n");
	}
	for(i=0;i<numThresh;i++) {
		nd_free(pipeFeetOverThresh[i]);
	}
	free(pipeFeetOverThresh);
	nd_freeStringArray(simids,nscenarios);
	fclose(fp);  // close standard output file
	return 1;
}

/* 
 * called usually only once after the scenario is done to obtain the completed analysis results
 */
void IIAnalysis_aggr_writeResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
	x_env=resultsRef->env;
	if(!writeIIAFile(analysisOptionsRef,resultsRef,dir,fn)) return;
	x_env=NULL;
}
/* 
 * called once after the scenario has been completed
 */
void IIAnalysis_aggr_shutdown(NamedDataRef *analysisOptionsRef)
{
	int i,j;
	x_env=analysisOptionsRef->env;
	for(i=0;i<x_iid->pot->numThresh;i++) {
		free(x_feetOverThresh[i]);
		free(x_linksOverThresh[i]);
		freeDiskCachedData(analysisOptionsRef->env,&x_PipeFeetOverThresh[i],x_numScenarios);
		for(j=0;j<x_iid->pot->numDiameters;j++) {
			free(x_feetOverThreshByDiam[i][j]);
			free(x_linksOverThreshByDiam[i][j]);
		}
		free(x_feetOverThreshByDiam[i]);
		free(x_linksOverThreshByDiam[i]);
	}
	free(x_feetOverThresh);
	free(x_linksOverThresh);
	free(x_PipeFeetOverThresh);
	free(x_feetOverThreshByDiam);
	free(x_linksOverThreshByDiam);

	freeIIAMemory(x_iid,Aggregation);
	x_env=NULL;
}

int sortFloatAsc(const void *a, const void *b)
{
	float f=*((float *)a) - *((float *)b);
	return (f<0?-1:(f>0?1:0));
}
