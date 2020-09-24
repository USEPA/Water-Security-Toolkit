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

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <float.h>
#include <list>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <sp/impacts.h>
#include "tso2ImpactAnalysis.h"

extern "C" {
#include "loggingUtils.h"
#include "NamedData.h"
#include "ExternalAnalysis.h"

extern char *getfullpath(const char *pathname, char *resolved_path, size_t maxLength);

static PMem x_mem;


static JNIEnv *x_env;

static int x_nSims;

static std::string outputFilePrefix;  // not used
static int detectionDelay=0;
static double minQuality=0.0;
static bool mcSelected=false;
static bool dmcSelected=false;
static bool vcSelected=false;
static bool dvcSelected=false;
static bool nfdSelected=false;
static bool tdSelected=false;
static bool dtdSelected=false;
static bool ecSelected=false;
static bool decSelected=false;
static bool pkSelected=false;
static bool dpkSelected=false;
static bool peSelected=false;
static bool dpeSelected=false;
static bool pdSelected=false;
static bool dpdSelected=false;
static std::string sensorInputFilename;
static std::list<int> sensorLocations;
static int numNodes;
static std::string nodeMapFileName;
static std::string scenarioMapFileName;
static std::ofstream nodeMapFile;
static std::ofstream scenarioMapFile;
static std::vector<std::string> nodeIndexToIDMap;
static std::map<std::string,int> nodeIDToIndexMap; // maps node textual IDs to 0-based TEVA node indicies
static clock_t start, stop;
static std::vector<ObjectiveBase*> theObjectives;
		int numImpacts;


/* 
 * called once after the first simulaiton has been completed (so demands are there)
 */
void tso2ImpactAnalysis_aggr_initialize(NamedDataRef *analysisOptionsRef, 
				   NamedDataRef *simResultsRef)
{
	// at this point, simResultsRef does not conatin any quality, but will contain
	// demands however
	int nnodes;
	int nsteps;
	x_nSims = 0;
	x_mem = loadOptions(analysisOptionsRef, simResultsRef,Aggregation,
      		       outputFilePrefix,
      		       detectionDelay,
      		       minQuality,
      		       mcSelected,
      		       dmcSelected,
      		       vcSelected,
      		       dvcSelected,
      		       nfdSelected,
      		       tdSelected,
      		       dtdSelected,
      		       ecSelected,
      		       decSelected,
      		       pkSelected,
      		       dpkSelected,
      		       pdSelected,
      		       dpdSelected,
      		       peSelected,
      		       dpeSelected);
	// x_mem->net,x_mem->dr,x_mem->node
	PLinkInfo dummy=NULL;	// no link info in simResultsRef yet
        initTso2ImpactModuleData(
		analysisOptionsRef, simResultsRef,
                numNodes, start, stop,
                x_mem->node->info, dummy,
                x_mem->net->info, x_mem->tso,
                outputFilePrefix,
                detectionDelay,
                minQuality,
                mcSelected,
                dmcSelected,
                vcSelected,
                dvcSelected,
                nfdSelected,
                tdSelected,
                dtdSelected,
                ecSelected,
                decSelected,
                pkSelected,
                dpkSelected,
                pdSelected,
                dpdSelected,
                peSelected,
                dpeSelected,
		x_mem,
                sensorLocations,
                nodeMapFileName,
                scenarioMapFileName,
                nodeMapFile,
                scenarioMapFile,
                theObjectives,
                nodeIndexToIDMap,
                nodeIDToIndexMap);
	if(peSelected || pkSelected || pdSelected || dpeSelected || dpdSelected || dpkSelected) {
		Population(x_mem->net,x_mem->popData,x_mem->node);

		dumpHIAInputParameters(x_mem);
	}
	nnodes = x_mem->net->info->numNodes;
	nsteps = x_mem->net->info->numSteps;

	std::vector<ObjectiveBase*>::iterator it;
	for (it=theObjectives.begin(); it!=theObjectives.end();it++) {
		ObjectiveBase *thisObjective=(*it);
		thisObjective->initializeForAggregationServer(analysisOptionsRef, simResultsRef);
	}
}

/* 
 * called once after every simulation with the results of the intermediate analysis
 */
void tso2ImpactAnalysis_aggr_newResults(NamedDataRef *analysisOptionsRef,NamedDataRef *intResultsRef)
{
	JNIEnv *env = analysisOptionsRef->env;
	char msg[256];
	int speciesIndex;

	x_env=env;

	x_nSims++;
	if(getInt(analysisOptionsRef, "SpeciesIndex", &speciesIndex)==ND_FAILURE) return;
	if (0) ANL_UTIL_LogSevere(x_env,"anl.teva", "test error!");

	sprintf(msg,"tso2Impacts: Processing simulation results for scenario# %d", x_nSims);
	ANL_UTIL_LogFine(env,"teva.analysis.server",msg);
	sprintf(msg,"tso2Impacts: Aggregation: newResults:");
	ANL_UTIL_LogFine(env,"teva.analysis.server",msg);

	int numInjections = 0;
//	if(getString(intResultsRef,"injectionID",&simID)==ND_FAILURE) return;
	if(getInt(intResultsRef,"numInjections",&numInjections)==ND_FAILURE) 
		return;
	PSourceData source;
	int *sourceIndices;
	int *sourceStartTimes;
	int *sourceStopTimes;
	int *sourceTypes;
	int *sourceSpeciesIndices;
	float *sourceStrengths;
	if(getIntArray(intResultsRef, "injectionIndices", &sourceIndices)==ND_FAILURE)
		return;
	if(getIntArray(intResultsRef, "injectionStartTimes", &sourceStartTimes)==ND_FAILURE)
		return;
	if(getIntArray(intResultsRef, "injectionStopTimes", &sourceStopTimes)==ND_FAILURE)
		return;
	if(getIntArray(intResultsRef, "injectionTypes", &sourceTypes)==ND_FAILURE)
		return;
	if(getIntArray(intResultsRef, "injectionSpeciesIndices", &sourceSpeciesIndices)==ND_FAILURE)
		return;
	if(getFloatArray(intResultsRef, "injectionStrengths", &sourceStrengths)==ND_FAILURE)
		return;
	// it is important to note here that the SourceData structure is not completely populated
	// here.  the sourceNodeID field is not set.

	source=(PSourceData)calloc(numInjections,sizeof(SourceData));
	source->nsources=numInjections;
	source->source=(PSource)calloc(numInjections,sizeof(Source));
	for (int i=0; i<numInjections; i++) {
		source->source[i].sourceNodeIndex = sourceIndices[i]+1;
		source->source[i].sourceStart     = sourceStartTimes[i];
		source->source[i].sourceStop      = sourceStopTimes[i];
		source->source[i].sourceType      = sourceTypes[i];
		source->source[i].speciesIndex    = sourceSpeciesIndices[i];
		source->source[i].sourceStrength  = sourceStrengths[i];
	}
	nd_free(sourceIndices);
	nd_free(sourceStartTimes);
	nd_free(sourceStopTimes);
	nd_free(sourceTypes);
	nd_free(sourceSpeciesIndices);
	nd_free(sourceStrengths);

	std::vector<ObjectiveBase*>::iterator it;
	for (it=theObjectives.begin(); it!=theObjectives.end();it++) {
		ObjectiveBase *thisObjective=(*it);
		const char *str = thisObjective->impactFilenameSuffix().c_str();
		int *witness;
		int *td;
		double *impact;
		char numImpactsStr[256];
		char witnessStr[256];
		char tdStr[256];
		char impactStr[256];
		sprintf(numImpactsStr,"%s_numImpacts", str);
		sprintf(witnessStr,"%s_witnessID", str);
		sprintf(tdStr,"%s_timeToDetection", str);
		sprintf(impactStr,"%s_impact", str);
		if(getInt(intResultsRef,(char*)numImpactsStr, &numImpacts)
						==ND_FAILURE) {
			sprintf(msg,"Unable to retrieve %s",numImpactsStr);
			ANL_UTIL_LogSevere(env,"teva.analysis.server",msg);
			return;
		}
		if(getIntArray(intResultsRef,(char*)witnessStr, &witness)
						==ND_FAILURE) {
			sprintf(msg,"Unable to retrieve %s",witnessStr);
			ANL_UTIL_LogSevere(env,"teva.analysis.server",msg);
			return;
		}
		if(getIntArray(intResultsRef,(char*)tdStr, &td)
						==ND_FAILURE) {
			sprintf(msg,"Unable to retrieve %s",tdStr);
			ANL_UTIL_LogSevere(env,"teva.analysis.server",msg);
			return;
		}
		if(getDoubleArray(intResultsRef,(char*)impactStr, &impact)
						==ND_FAILURE) {
			sprintf(msg,"Unable to retrieve %s",impactStr);
			ANL_UTIL_LogSevere(env,"teva.analysis.server",msg);
			return;
		}
		for (int j=0; j<numImpacts; j++) {
		    thisObjective->updateImpacts(witness[j]-1,impact[j],td[j]);
		}
		nd_free(witness);
		nd_free(td);
		nd_free(impact);
		/***** Need to use species Index here instead of 0 for third arg!!!! ********/
		thisObjective->outputScenarioImpactData(x_nSims,source,speciesIndex);
		thisObjective->processAggregationServerData(intResultsRef,source);
		thisObjective->resetForScenario();
	}
	free(source->source);
	free(source);
	sprintf(msg,"tso2Impacts: Processed simulation results for scenario# %d", x_nSims);
	ANL_UTIL_LogFine(env,"teva.analysis.server",msg);
}


/* 
 * called usually only once after the scenario is done to obtain the completed analysis results
 */
void tso2ImpactAnalysis_aggr_getResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef)
{
	int i;
	char **ids;
	int nnodes=x_mem->net->info->numNodes;
	ids = (char **)calloc(nnodes,sizeof(char*));
	for(i=0;i<nnodes;i++) {
		ids[i]=x_mem->node[i].info->id;
	}
	if(addVariable(resultsRef,"NodeIDs",      STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(setStringArray(resultsRef,"NodeIDs",ids,nnodes)==ND_FAILURE) return;
	free(ids);

	int nlinks=x_mem->net->info->numLinks;
	ids = (char **)calloc(nlinks,sizeof(char*));
	for(i=0;i<nlinks;i++) {
		ids[i]=x_mem->link[i].info->id;
	}
	if(addVariable(resultsRef,"LinkIDs",      STRING_ARRAY_TYPE)==ND_FAILURE) return;
	if(setStringArray(resultsRef,"LinkIDs",ids,nlinks)==ND_FAILURE) return;
	free(ids);

	std::vector<ObjectiveBase*>::iterator it;
	for (it=theObjectives.begin(); it!=theObjectives.end();it++) {
		ObjectiveBase *thisObjective=(*it);
		thisObjective->getAggregationResults(analysisOptionsRef,resultsRef);
	}
}

/* 
 * called usually only once after the scenario is done to obtain the completed analysis results
 */
void tso2ImpactAnalysis_aggr_writeResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn) {
	char absoluteOutputDir[1024],absoluteSourceDir[1024];
	JNIEnv *env = analysisOptionsRef->env;
	char msg[256];
	char tfn[256];
	char *prefix;
	FILE *fp;
	int i;
	x_env=env;

	//
	// ignore the empty parameters and use the data we can already
	// access.
	//
	
	sprintf(msg,"in aggr_writeResults(): dir=%s",dir);
	ANL_UTIL_LogFine(env,"teva.analysis.server",msg);
	getfullpath(dir,absoluteOutputDir,1024);
	sprintf(msg,"in aggr_writeResults(): absoluteOutputDir=%s",absoluteOutputDir);
	ANL_UTIL_LogFine(env,"teva.analysis.server",msg);
	getfullpath(".",absoluteSourceDir,1024);
	sprintf(msg,"in aggr_writeResults(): absoluteSourceDir=%s",absoluteSourceDir);
	ANL_UTIL_LogFine(env,"teva.analysis.server",msg);

	if(getString(analysisOptionsRef,"outputFileRoot",&prefix)==ND_FAILURE) return;

	sprintf(tfn,"%s.nodemap",prefix);
	strcat(fn,tfn);
	fp=fopen(tfn,"w");
	for(i=0;i<x_mem->net->info->numNodes;i++) {
	  fprintf(fp,"%d %s\n",i+1,x_mem->nodeinfo[i].id);
	}
	fclose(fp);
	std::vector<ObjectiveBase*>::iterator it;
	for (it=theObjectives.begin(); it!=theObjectives.end();it++) {
		ObjectiveBase *thisObjective=(*it);
		const char *sfx = thisObjective->impactFilenameSuffix().c_str();
		char baseFn[256];
		sprintf(baseFn,"%s_%s",prefix,sfx);
		if(strlen(fn) > 0) strcat(fn,",");
		strcat(fn,baseFn);
		if(strlen(dir)>0)
		  sprintf(tfn,"%s/%s",dir,baseFn);
		else
		  sprintf(tfn,"%s",baseFn);
		// if the destination directory is the same as the current working 
		// directory (where all the impact files are written), we don't need to do
		// anything else here
		if(strcmp(absoluteOutputDir,absoluteSourceDir)!=0) {
		  sprintf(msg,"in aggr_writeResults(): moving file %s to %s",baseFn,tfn);
		  ANL_UTIL_LogFine(env,"teva.analysis.server",msg);
		  rename(baseFn,tfn);
		}

		sprintf(baseFn,"%s_%s.id",prefix,sfx);
		if(strlen(fn) > 0) strcat(fn,",");
		strcat(fn,baseFn);
		if(strlen(dir)>0)
		  sprintf(tfn,"%s/%s",dir,baseFn);
		else
		  sprintf(tfn,"%s",baseFn);
		// if the destination directory is the same as the current working 
		// directory (where all the impact files are written), we don't need to do
		// anything else here
		if(strcmp(absoluteOutputDir,absoluteSourceDir)!=0) {
		  sprintf(msg,"in aggr_writeResults(): moving file %s to %s",baseFn,tfn);
		  ANL_UTIL_LogFine(env,"teva.analysis.server",msg);
		  rename(baseFn,tfn);
		}

	}
	nd_free(prefix);
}

/* 
 * called once after the scenario has been completed
 */
void tso2ImpactAnalysis_aggr_shutdown(NamedDataRef *analysisOptionsRef)
{
	char msg[256];

	JNIEnv *env = analysisOptionsRef->env;

	sprintf(msg,"There were %d simulations", x_nSims);
	ANL_UTIL_LogInfo(env,"teva.analysis.server",msg);

	finalizeTheObjectives(theObjectives);


	// should be last in case any others need data from x_mem
	freeMemory(x_mem,Aggregation);
/*
	free(x_aveNonzero);
*/
}
};
