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

#include <limits.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include "tso2ImpactAnalysis.h"
#include <sp/impacts.h>

extern "C" {
#include "ExternalAnalysis.h"
#include "NamedData.h"


static PMem x_mem;
static int x_nscenarios;

static std::string outputFilePrefix;
static std::string tsoFileOrDirectoryName;
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

/* 
 * called once after the first simulaiton has been completed 
 * (so demands are there)
 */
int tso2ImpactAnalysis_ia_initialize(NamedDataRef *analysisOptionsRef, 
		 		     NamedDataRef *simResultsRef)
{
	// at this point, simResultsRef does not conatin any quality, 
	// but will contain demands 
	x_mem = loadOptions(analysisOptionsRef, simResultsRef,
		       IntermediateAnalysis,
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
	x_nscenarios=0;
	PLinkInfo dummy=NULL;	// no link info in simResultsRef yet
	std::string s = "";
        initTso2ImpactModuleData(
		analysisOptionsRef, simResultsRef,
                numNodes, start, stop,
                x_mem->node->info, dummy,
                x_mem->net->info, x_mem->tso,
                s,
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

	    initializeIngestionData(x_mem->ingestionData,x_mem->net,x_mem->node,x_mem->dr,x_mem->popData);
//	IngestionRate(x_mem->net,x_mem->node);
		dumpHIAInputParameters(x_mem);
	}
	return NON_REMOTE;	// pass results back by value
}

/* 
 * called once after every simulation is done to perform the 
 * intermediate analysis.  The data placed in the intResultsRef will 
 * be passed along to the aggregation server
 */
void tso2ImpactAnalysis_ia_newSimResults(NamedDataRef *analysisOptionsRef,
				    NamedDataRef *simResultsRef, 
				    NamedDataRef *intResultsRef)
{
	int i,j, nsteps, nnodes;
	float **c;
	char *simID;
	char *injDef;
	int speciesIndex;
	int nSpecies=x_mem->net->info->numSpecies;

	if(getInt(analysisOptionsRef, "SpeciesIndex", &speciesIndex)==ND_FAILURE) return;
	x_nscenarios++;
	nsteps=x_mem->net->info->numSteps;
	nnodes=x_mem->net->info->numNodes;

	for(j=0;j<nnodes;j++) {
		for(i=0;i<nSpecies;i++) {
			x_mem->node[j].info->nz[i] = 0;
		}
	}
	c=x_mem->net->info->qualResults->nodeC[speciesIndex];
	for(i=0;i<nsteps;i++) {
		float *tres;
		if(getFloatArrayAtIndex(simResultsRef,"quality",
					i,&tres)==ND_FAILURE) {
			return;
		}
		for(j=0;j<nnodes;j++) {
			float tc = tres[j];
			if(tc > 0) x_mem->node[j].info->nz[speciesIndex] = 1;
			c[j][i] = tc;
		}
		nd_free(tres);
	}
	int numInjections = 0;
	if(getString(simResultsRef,"injectionID",&simID)==ND_FAILURE) return;
	if(getString(simResultsRef,"injectionDef",&injDef)==ND_FAILURE) return;
	if(getInt(simResultsRef,"numInjections",&numInjections)==ND_FAILURE) 
		return;
	PSourceData source;
	int *sourceIndices;
	int *sourceStartTimes;
	int *sourceStopTimes;
	int *sourceTypes;
	int *sourceSpeciesIndices;
	float *sourceStrengths;
	if(getIntArray(simResultsRef, "injectionIndices", &sourceIndices)==ND_FAILURE)
		return;
	if(getIntArray(simResultsRef, "injectionStartTimes", &sourceStartTimes)==ND_FAILURE)
		return;
	if(getIntArray(simResultsRef, "injectionStopTimes", &sourceStopTimes)==ND_FAILURE)
		return;
	if(getFloatArray(simResultsRef, "injectionStrengths", &sourceStrengths)==ND_FAILURE)
		return;
	if(getIntArray(simResultsRef, "injectionTypes", &sourceTypes)==ND_FAILURE)
		return;
	if(getIntArray(simResultsRef, "injectionSpeciesIndices", &sourceSpeciesIndices)==ND_FAILURE)
		return;

	source=(PSourceData)calloc(1,sizeof(SourceData));
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

	// if either of the HIA objectives are chosen, compute the necessary
	// HIA data.  Do it here rather than replicating the now more complex
	// logic introduced by the addition of the other ingestion methods
	if(peSelected ||dpeSelected || pkSelected || dpkSelected || pdSelected|| dpdSelected) {
		HIST hist;
		int calcDR;
		int nsteps=x_mem->net->info->numSteps;
		int nnodes=x_mem->net->info->numNodes;
		int totsteps = x_mem->dr!=NULL?x_mem->dr->maxsteps:x_mem->net->info->numSteps;

		calcDR=x_mem->dr!=NULL;

		resetXA(x_mem->xa,totsteps);
		resetDoseOverThreshold(x_mem);
		resetDoseBinData(x_mem);
		/* Time-dependent cumulative expected dose and disease response */
		IngestionDataPtr ing=x_mem->ingestionData;
		for(int i=0;i<nnodes;i++) {
		  int pop = x_mem->node[i].pop;
		  if(x_mem->node[i].info->nz[speciesIndex] && pop > 0) {
			  Dose(ing->ingestionData[i],ing,&x_mem->node[i], x_mem->net,nsteps,x_mem->u,x_mem->dot,i,x_mem->doseBins,speciesIndex,NULL);
			  if(calcDR) {
				  Response(ing->ingestionData[i],ing,x_mem->dr,&x_mem->node[i],x_mem->u);
				  DiseaseRM(x_mem->net,x_mem->dr,x_mem->u);
			  }
		  }
		  Accumulate(i,x_mem->node[i].info->nz[0],x_mem->net,x_mem->dr,x_mem->node[i].pop,x_mem->u,x_mem->xa,x_mem->ta,&hist);
		}
	}
	// the second argument to processScenario is the simulation id,
	// an int in the standalone tso2Impact which doesn't exist in 
	// simResultsRef.  The analysis server will assign a unique 
	// identifier to the sim results when it gets them.  Pass a 0
	// to processScenario -- it doesn't need to know a sim id in the
	// module.
        //
        // Note: this uses the default detectionConfidence of '1'
        //
	processScenario(theObjectives, 0, detectionDelay, 1, minQuality,
			x_mem->net->info, x_mem->nodeinfo, x_mem->linkinfo, 
			source, intResultsRef, speciesIndex);
	// processScenario called ObjectiveBase::construct

	if(addVariable(intResultsRef,"injectionID",  STRING_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"injectionDef",  STRING_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"numInjections",  INT_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"injectionIndices",  INT_ARRAY_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"injectionStartTimes",  INT_ARRAY_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"injectionStopTimes",  INT_ARRAY_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"injectionStrengths",  FLOAT_ARRAY_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"injectionTypes",  INT_ARRAY_TYPE )==ND_FAILURE) return;
	if(addVariable(intResultsRef,"injectionSpeciesIndices",  INT_ARRAY_TYPE )==ND_FAILURE) return;

	if(setString(intResultsRef,"injectionID",   simID)==ND_FAILURE) return;
	if(setString(intResultsRef,"injectionDef",   injDef)==ND_FAILURE) return;
	if(setInt(intResultsRef,   "numInjections", numInjections)==ND_FAILURE) return;
	if(setIntArray(intResultsRef,  "injectionIndices",    sourceIndices,   numInjections)==ND_FAILURE) return;
	if(setIntArray(intResultsRef,  "injectionStartTimes", sourceStartTimes,numInjections)==ND_FAILURE) return;
	if(setIntArray(intResultsRef,  "injectionStopTimes",  sourceStopTimes, numInjections)==ND_FAILURE) return;
	if(setIntArray(intResultsRef,  "injectionTypes",  sourceTypes, numInjections)==ND_FAILURE) return;
	if(setIntArray(intResultsRef,  "injectionSpeciesIndices",  sourceSpeciesIndices, numInjections)==ND_FAILURE) return;
	if(setFloatArray(intResultsRef,"injectionStrengths",  sourceStrengths, numInjections)==ND_FAILURE) return;

	nd_free(sourceIndices);
	nd_free(sourceStartTimes);
	nd_free(sourceStopTimes);
	nd_free(sourceTypes);
	nd_free(sourceSpeciesIndices);
	nd_free(sourceStrengths);
	nd_free(simID);
	nd_free(injDef);
	free(source->source);
	free(source);
}

/* 
 * called once after the scenario has been completed
 */
void tso2ImpactAnalysis_ia_shutdown(NamedDataRef *analysisOptionsRef)
{
	finalizeTheObjectives(theObjectives);
	freeMemory(x_mem,IntermediateAnalysis);
}
};
