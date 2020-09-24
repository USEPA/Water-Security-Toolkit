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
#include <string.h>
#include "NamedData.h"
#include "HealthImpacts.h"
#include "AnalysisModuleUtils.h"

enum SensorType getSensorType(char *typeStr);
enum SampleType getSampleType(char *sampleTypeStr);

JNIEXPORT PMem allocateBaseMemory(void *simResultsRef, int speciesIndex, int needRho)
{
	PMem mem=(PMem)calloc(1,sizeof(Mem));
	int nnodes;
	int nlinks;
	int nsteps;
	int i;
	mem->net=(PNet)calloc(1,sizeof(Net));
	initializeNetwork(simResultsRef,speciesIndex,&mem->net->info, &mem->nodeinfo, &mem->linkinfo);
	nnodes=mem->net->info->numNodes;
	nlinks=mem->net->info->numLinks;
	nsteps=mem->net->info->numSteps;
	mem->node = (PNode)calloc(nnodes,sizeof(Node));
	for(i=0;i<nnodes;i++) {
		if(needRho)
			mem->node[i].rho= (float *)calloc(nsteps,sizeof(float));
		mem->node[i].info = &mem->nodeinfo[i];  //share the one from mem
		mem->node[i].pop = 0;
	}

	mem->link = (PLink)calloc(nlinks,sizeof(Link));
	for(i=0;i<nlinks;i++) {
		mem->link[i].info = &mem->linkinfo[i];  //share the one from mem
	}
	return mem;
}
JNIEXPORT PMem loadHIAOptions(void *analysisOptionsRef, void *simResultsRef, ModuleType componentType)
{
	PMem mem;
	PNode     node;
	PNet      net;
	PXA       xa;
	PTA       ta;
	PU        u;
	PDR dr=NULL;
	int calcDR;
	PopulationDataPtr pd;
	IngestionDataPtr ing;
	int speciesIndex;
	char *tstr;
	float tf;
	int i;
	int maxsteps;
	float *population;
	char **popNodes;
	int popLength;
	int np;

	if(getInt(analysisOptionsRef, "SpeciesIndex", &speciesIndex)==ND_FAILURE) return NULL;
	mem=allocateBaseMemory(simResultsRef,speciesIndex,1);

	net=mem->net;
	node=mem->node;

	if(getInt(analysisOptionsRef, "CalcDoseResponse", &calcDR)==ND_FAILURE) return NULL;

	if(calcDR) {
		/** Dose-response info */
		dr = (PDR)calloc(1,sizeof(DR));
		mem->dr=dr;

		if(getString(analysisOptionsRef, "DoseResponseType", &tstr)==ND_FAILURE) return NULL;
		dr->responseType=Probit;
		if(strcmp(tstr,"probit")==0) dr->responseType=Probit;
		else if(strcmp(tstr,"old")==0) dr->responseType=Old;
		nd_free(tstr);

		// sigmoid
		if(getFloat(analysisOptionsRef, "DoseResponseA", &dr->a)==ND_FAILURE) return NULL;
		if(getFloat(analysisOptionsRef, "DoseResponseM", &dr->m)==ND_FAILURE) return NULL;
		if(getFloat(analysisOptionsRef, "DoseResponseN", &dr->n)==ND_FAILURE) return NULL;
		if(getFloat(analysisOptionsRef, "DoseResponseTau", &dr->tau)==ND_FAILURE) return NULL;
		//probit
		if(getFloat(analysisOptionsRef, "LD50", &dr->ld50)==ND_FAILURE) return NULL;
		if(getFloat(analysisOptionsRef, "Beta", &dr->beta)==ND_FAILURE) return NULL;

		if(getFloat(analysisOptionsRef, "BodyMass", &dr->bodymass)==ND_FAILURE) return NULL;
		if(getInt(analysisOptionsRef, "Normalize", &dr->normalize)==ND_FAILURE) return NULL;
		if(getInt(analysisOptionsRef, "LatencyTime", &dr->nL)==ND_FAILURE) return NULL;
		if(getInt(analysisOptionsRef, "FatalityTime", &dr->nF)==ND_FAILURE) return NULL;
		if(getFloat(analysisOptionsRef, "FatalityRate", &dr->frate)==ND_FAILURE) return NULL;
		if(getString(analysisOptionsRef, "DoseType", &tstr)==ND_FAILURE) return NULL;
		if(strcmp(tstr,"total")==0) dr->dtype=totalmass;
		nd_free(tstr);
		dr->nL=ROUND(dr->nL/mem->net->info->stepSize);
		dr->nF=ROUND(dr->nF/mem->net->info->stepSize);
		dr->maxsteps = net->info->numSteps + dr->nL + dr->nF;       /* Max number   of time steps for disease response */
		if(getInt(analysisOptionsRef, "SpeciesIndex", &dr->speciesIndex)==ND_FAILURE) return NULL;
		if(getString(analysisOptionsRef, "PrimarySpecies", &tstr)==ND_FAILURE) return NULL;
		dr->speciesName=(char *)calloc(strlen(tstr)+1,sizeof(char));
		strcpy(dr->speciesName,tstr);
		nd_free(tstr);
	}
	maxsteps=(dr != NULL?dr->maxsteps:net->info->numSteps);
	ing=(IngestionDataPtr)calloc(1,sizeof(IngestionData));
	mem->ingestionData=ing;

	if(getFloat(analysisOptionsRef, "IngestionRate", &tf)==ND_FAILURE) return NULL;
	ing->phi=(float)(tf/24.);
	
	if(getString(analysisOptionsRef, "IngestionTimingType", &tstr)==ND_FAILURE) return NULL;
	if(strcmp(tstr,"demand")==0) {
		ing->timingMode=IMDemand;
		ing->volumeMode=VMDemand;
	} else if(strcmp(tstr,"atus")==0) {
		ing->timingMode=IMATUS;
	} else if(strcmp(tstr,"fixed5")==0) {
		ing->timingMode=IMFixed5;
	} else if(strcmp(tstr,"userfixed")==0) {
		float *fvals=NULL;
		int numTimes;
		ing->timingMode=IMUserFixed;
		if(getInt(analysisOptionsRef,"NumIngestionTimes",&numTimes)==ND_FAILURE) return NULL;
		if(getFloatArray(analysisOptionsRef,"IngestionTimes",&fvals)==ND_FAILURE) return NULL;
		createUserFixedTimesArray(ing,fvals,numTimes);
		nd_free(fvals);
	}
	nd_free(tstr);
	if(getString(analysisOptionsRef, "IngestionVolumeType", &tstr)==ND_FAILURE) return NULL;
	if(strcmp(tstr,"demand")==0) {
		ing->volumeMode=VMDemand;
	} else if(strcmp(tstr,"random")==0) {
		ing->volumeMode=VMRandom;
	} else if(strcmp(tstr,"mean")==0) {
		ing->volumeMode=VMMean;
		if(getFloat(analysisOptionsRef, "IngestionRate", &ing->meanVolume)==ND_FAILURE) return NULL;
	}
	nd_free(tstr);


	pd=(PopulationDataPtr)calloc(1,sizeof(PopulationData));
	mem->popData=pd;
//	if(getString(analysisOptionsRef, "Population", &tstr)==ND_FAILURE) return NULL;
//	if(strcmp(tstr,"demand")==0) pd->population=DemandBased;
//	else if(strcmp(tstr,"scenario")==0) pd->population=FileBased;
	pd->population=FileBased;
//	nd_free(tstr);

//	if(pd->population==FileBased) {

	if(getFloatArray(simResultsRef, "population", &population)==ND_FAILURE) return NULL; 
	if(getStringArray(simResultsRef, "popNodes", &popNodes)==ND_FAILURE) return NULL; 
	if(getInt(simResultsRef, "popLength", &popLength)==ND_FAILURE) return NULL; 

	pd->pop = (PNodePopulation)calloc(popLength,sizeof(NodePopulation));
	pd->popLength=popLength;
	for(np=0;np<popLength;np++) {
		PNodePopulation p = &pd->pop[np];
		p->nodeid = (char *)calloc(strlen(popNodes[np])+1,sizeof(char));
		strcpy(p->nodeid,popNodes[np]);
		p->population=population[np];
		p->used=0;
	}
	nd_free(population);
	nd_freeStringArray(popNodes,popLength);
//	} else {
//		if(getFloat(analysisOptionsRef, "UsageRate", &pd->pcu)==ND_FAILURE) return NULL;
//	}

	xa = (PXA)calloc(1,sizeof(XA));                        /* spatial averages */
	initXA(xa,maxsteps);
	mem->xa=xa;
	
	ta = (PTA)calloc(1,sizeof(TA));                        /* spatial averages */
	initTA(ta,net->info->numNodes);
	mem->ta=ta;

	/** Really only need to allocate the next structure in the intermediate analysis server */
	if(componentType == IntermediateAnalysis) {
		u = (PU)calloc(1,sizeof(U));                           /* state variables */
		u->dos = (float *)calloc(maxsteps,sizeof(float));    /* Basic dose-respone state variables */
		u->res = (float *)calloc(maxsteps,sizeof(float));
		u->s = (float *)calloc(maxsteps,sizeof(float));
		u->i = (float *)calloc(maxsteps,sizeof(float));
		u->d = (float *)calloc(maxsteps,sizeof(float));
		u->f = (float *)calloc(maxsteps,sizeof(float));
		mem->u=u;
		for(i=0;i<(int)(maxsteps);i++) {
			u->dos[i] = 0;				/* Spatial averages */
			u->res[i] = 0;				
			u->s[i] = 0;
			u->i[i] = 0;
			u->d[i] = 0;
			u->f[i] = 0;
		}
	}

	if(componentType == IntermediateAnalysis || componentType == Aggregation) {
		int numSensors;
		if(getInt(simResultsRef, "numSensors", &numSensors)==ND_FAILURE) return NULL;
		if(numSensors > 0) {
			char **sensorData=NULL;
			if(getStringArray(simResultsRef, "sensors", &sensorData)==ND_FAILURE) return NULL;
			for(i=0;i<numSensors;i++) {
				char sd[1024],*token;
				int idx;
				strcpy(sd,sensorData[i]);
				token = strtok(sd,","); // SAMPLELOC keyword
				token = strtok(NULL,",");  // nodeid
				idx = getnodeindex(net,node,token);
				if(idx != -1) {
					PNodeInfo nodeInfo = node[idx].info;
					PSensor sensor = &node[idx].sensor;
					token = strtok(NULL,",");  // sensor type (EXISTING | IGNORED | POTENTIAL | SELCTED)
					sensor->type=getSensorType(token);
					token = strtok(NULL,",");  // sensor sample type (REALTIME | COMPOSITE | FILTERED)
					sensor->sample=getSampleType(token);
					sensor->limit=(float)atof(strtok(NULL,","));
					sensor->vol=(float)atof(strtok(NULL,","));
					sensor->freq=atoi(strtok(NULL,","));
					sensor->start=atoi(strtok(NULL,","));
					sensor->delay=atoi(strtok(NULL,","));
				}
			}

		}
	}
	mem->dot=allocateDoseOverThreshold(analysisOptionsRef,mem);
	if(calcDR) {
	  float *bins;
	  int numBins;

	  if(getInt(analysisOptionsRef, "NumDoseBins", &numBins)==ND_FAILURE) return NULL;
	  if(numBins>0) {
	    char *dbType; // Dose or Response;
	    int isResponse=0;
		PDoseBinData pdb=NULL;
	    if(getFloatArray(analysisOptionsRef, "DoseBins", &bins)==ND_FAILURE) return NULL;
	    if(getString(analysisOptionsRef, "DoseBinType", &dbType)==ND_FAILURE) return NULL;

        pdb = (PDoseBinData)calloc(1,sizeof(DoseBinData));
        pdb->numBins=numBins;
        pdb->responseBins=NULL;
		isResponse=strcasecmp(dbType,"Response")==0;
		pdb->doseBins=(float*)calloc(numBins,sizeof(float));
		if(calcDR) {
			pdb->responseBins=(float*)calloc(numBins,sizeof(float));
		}
        for(i=0;i<numBins;i++) {
          if(isResponse) {
			  if(calcDR) {
				  pdb->doseBins[i]=ResponseDose(bins[i],mem->dr);
				  pdb->responseBins[i]=bins[i];
			  }
		  } else {
			  pdb->doseBins[i]=bins[i];
			  if(calcDR) {
				  pdb->responseBins[i]=DoseResponse(bins[i],mem->dr);
			  }
		  }
		}
        pdb->data=(int*)calloc(numBins,sizeof(int));
        mem->doseBins=pdb;
		nd_free(dbType);
		nd_free(bins);
	  }
	}
	return mem;
}
JNIEXPORT PDoseOverThresh allocateDoseOverThreshold(void *analysisOptionsRef, PMem mem) {
  float *thresholds;
  int i;
  int numThresh,numDThresh,numRThresh;
  int nnodes;
  PDoseOverThresh pdot=NULL;
  if(getInt(analysisOptionsRef, "NumDoseThresholds", &numDThresh)==ND_FAILURE) return NULL;
  if(mem->dr != NULL) {
    if(getInt(analysisOptionsRef, "NumResponseThresholds", &numRThresh)==ND_FAILURE) return NULL;
  } else {
	  numRThresh=0;
  }
  numThresh=numDThresh+numRThresh;
  if(numThresh>0) {
    int n=0;
    pdot=(PDoseOverThresh)calloc(1,sizeof(DoseOverThresh));
    pdot->numThresh=numThresh;
    pdot->thresholds=(float*)calloc(numThresh,sizeof(float));
    pdot->threshIDs=(char**)calloc(numThresh,sizeof(char *));
    if(numDThresh > 0) {
      if(getFloatArray(analysisOptionsRef, "DoseThreshold", &thresholds)==ND_FAILURE) return NULL;
      for(i=0;i<numDThresh;i++,n++) {
        char tmp[512];
		pdot->thresholds[n]=thresholds[i];
		sprintf(tmp,"%f",pdot->thresholds[n]);
		pdot->threshIDs[n]=(char *)calloc(strlen(tmp)+1,sizeof(char));
		strcpy(pdot->threshIDs[n],tmp);
	  }
	  nd_free(thresholds);
	}
	if(numRThresh > 0) {
      if(getFloatArray(analysisOptionsRef, "ResponseThresholds", &thresholds)==ND_FAILURE) return NULL;
	  for(i=0;i<numRThresh;i++,n++) {
        char tmp[512];
		pdot->thresholds[n]=ResponseDose(thresholds[i],mem->dr);
		sprintf(tmp,"%f",pdot->thresholds[n]);
		pdot->threshIDs[n]=(char *)calloc(strlen(tmp)+1,sizeof(char));
		strcpy(pdot->threshIDs[n],tmp);
	  }
	  nd_free(thresholds);
	}
	pdot->numOver = (int **)calloc(numThresh,sizeof(int*));
	pdot->numOverByTime = (int **)calloc(numThresh,sizeof(int*));
	nnodes = mem->net->info->numNodes;
	for(i=0;i<numThresh;i++) {
      pdot->numOver[i] = (int *)calloc(nnodes,sizeof(int));
	  pdot->numOverByTime[i] = (int *)calloc(mem->net->info->numSteps,sizeof(int));
	}
	pdot->totOver = (int *)calloc(numThresh,sizeof(int));
	pdot->nodesOver = (int *)calloc(numThresh,sizeof(int));
  }
  return pdot;
}

JNIEXPORT PDoseOverThresh allocateDoseOverThresholdWithIDs(void *analysisOptionsRef, PMem mem) {
  // this one is called from tso2Impact module
  int i,nnodes;
  double *thresholds;
  int numThresh;
  PDoseOverThresh pdot=NULL;
  if(getInt(analysisOptionsRef, "numDoseThresholds", &numThresh)==ND_FAILURE) return NULL;
  if(numThresh>0) {
    int n=0;
    char **threshIDs=NULL;
    pdot=(PDoseOverThresh)calloc(1,sizeof(DoseOverThresh));
    pdot->numThresh=numThresh;
    pdot->thresholds=(float*)calloc(numThresh,sizeof(float));
    pdot->threshIDs=(char **)calloc(numThresh,sizeof(char *));
    if(getDoubleArray(analysisOptionsRef, "doseThresholds", &thresholds)==ND_FAILURE) return NULL;
    if(getStringArray(analysisOptionsRef, "doseThresholdIDs",&threshIDs)==ND_FAILURE) return NULL;
    for(i=0;i<numThresh;i++) {
      pdot->thresholds[i]=(float)thresholds[i];
      pdot->threshIDs[i]=(char *)calloc(strlen(threshIDs[i])+1,sizeof(char));
      strcpy(pdot->threshIDs[i],threshIDs[i]);
    }
    nd_freeStringArray(threshIDs, numThresh);
    nd_free(thresholds);
  }
  pdot->numOver = (int **)calloc(numThresh,sizeof(int*));
  pdot->numOverByTime = (int **)calloc(numThresh,sizeof(int*));
  nnodes=mem->net->info->numNodes;
  for(i=0;i<numThresh;i++) {
    pdot->numOver[i] = (int *)calloc(mem->net->info->numNodes,sizeof(int));
    pdot->numOverByTime[i] = (int *)calloc(mem->net->info->numSteps,sizeof(int));
  }
  pdot->totOver = (int *)calloc(numThresh,sizeof(int));
  pdot->nodesOver = (int *)calloc(numThresh,sizeof(int));
  return pdot;
}
JNIEXPORT void freeDoseOverThresholdData(PMem mem) {
  int i;
  if(mem->dot != NULL) {
    for(i=0;i<mem->dot->numThresh;i++) {
      free(mem->dot->numOver[i]);
      free(mem->dot->numOverByTime[i]);
      free(mem->dot->threshIDs[i]);
    }
    free(mem->dot->numOver);
    free(mem->dot->numOverByTime);
    free(mem->dot->totOver);
    free(mem->dot->nodesOver);
    free(mem->dot->thresholds);
    free(mem->dot->threshIDs);
    free(mem->dot);
    mem->dot=NULL;
  }
}
JNIEXPORT void freeHIAMemory(PMem mem, ModuleType componentType) {
	int i;
	int nnodes=mem->net->info->numNodes;

	// do this first (or at least before mem->node is freed!)
//ANL_UTIL_LogInfo(env,"teva.analysis.server","freeHIAMemory: 01");
	if(componentType == IntermediateAnalysis) {
		freeIngestionData(mem->ingestionData,mem->node);
	}
	free(mem->ingestionData);

	if(mem->dr!=NULL) {
		freeXA(mem->xa,mem->dr->maxsteps);
		free(mem->xa);
		freeTA(mem->ta,nnodes);
		free(mem->ta);
	}
	/** Really only need to allocate the next structure in the intermediate analysis server */
	// if mem-> is NULL, that indicates that memory allocation did not include HIA-related items
	if(componentType == IntermediateAnalysis && mem->dr != NULL) {
		int len = mem->dr->maxsteps-1;
		free(mem->u->s);
		free(mem->u->i);
		free(mem->u->d);
		free(mem->u->f);
		free(mem->u->dos);
		free(mem->u->res);
		free(mem->u);
	}

	if(mem->popData != NULL && mem->popData->popLength > 0) {
		for(i=0;i<mem->popData->popLength;i++) {
			free(mem->popData->pop[i].nodeid);
		}
		free(mem->popData->pop);
	}
	free(mem->popData);
	if(mem->dr != NULL && mem->dr->speciesName != NULL)
		free(mem->dr->speciesName);
	free(mem->dr);

	freeDoseOverThresholdData(mem);

	if(mem->doseBins) {
		if(mem->doseBins->responseBins) free(mem->doseBins->responseBins);
		if(mem->doseBins->doseBins) free(mem->doseBins->doseBins);
		if(mem->doseBins->data) free(mem->doseBins->data);
		free(mem->doseBins); 
	}

	for(i=0;i<nnodes;i++) {
		if(mem->node[i].rho != NULL) {
			free(mem->node[i].rho);
		}
	}
	free(mem->node);
	free(mem->link);

	freeNetwork(&mem->net->info, &mem->nodeinfo, &mem->linkinfo);
	free(mem->net);
	free(mem);
}

enum SensorType getSensorType(char *typeStr)
{
	if(stricmp(typeStr,"existing")==0) return existing;
	if(stricmp(typeStr,"ignored")==0) return ignore;
	if(stricmp(typeStr,"potential")==0) return potential;
	if(stricmp(typeStr,"selected")==0) return selected;
	return ignore;
}
enum SampleType getSampleType(char *sampleTypeStr)
{
	if(stricmp(sampleTypeStr,"filtered")==0) return filtered;
	if(stricmp(sampleTypeStr,"composite")==0) return composite;
	if(stricmp(sampleTypeStr,"realtime")==0) return realtime;
	return realtime;
}
