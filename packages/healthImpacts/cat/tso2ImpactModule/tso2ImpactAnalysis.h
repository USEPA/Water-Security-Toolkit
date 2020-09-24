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

#ifndef TSO2IMPACTANALYSIS_H
#define TSO2IMPACTANALYSIS_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <sp/ObjectiveBase.h>

extern "C" {

#include "ExternalAnalysis.h"
#include "AnalysisModuleUtils.h"
#include "tevautil.h"
#include "teva-assess.h"

/* Global Data Types */

/*
typedef struct
{
	PTSO         tso;
	PNodeInfo    node;
	PSource      sources;
	PNetInfo     net;
} tso2ImpactMem;
typedef tso2ImpactMem *Ptso2ImpactMem;
*/

PMem loadOptions(NamedDataRef *analysisOptionsRef, 
		 NamedDataRef *simResultsRef, ModuleType componentType,
      		 std::string&outputFilePrefix,
      		 int&detectionDelay,
      		 double&minQuality,
      		 bool&mcSelected,
      		 bool&dmcSelected,
      		 bool&vcSelected,
      		 bool&dvcSelected,
      		 bool&nfdSelected,
      		 bool&tdSelected,
      		 bool&dtdSelected,
      		 bool&ecSelected,
      		 bool&decSelected,
      		 bool&pkSelected,
      		 bool&dpkSelected,
      		 bool&pdSelected,
      		 bool&dpdSelected,
      		 bool&peSelected,
      		 bool&dpeSelected);

void initTso2ImpactModuleData(
	NamedDataRef *analysisOptionsRef, 
	NamedDataRef *simResultsRef, 
	int& numNodes, clock_t&start, clock_t& stop, 
	PNodeInfo&nodes, PLinkInfo&links,
	PNetInfo&net, PTSO&tso,
	std::string&outputFilePrefix,
	int&detectionDelay,
	double&minQuality,
	bool&mcSelected,
	bool&dmcSelected,
	bool&vcSelected,
	bool&dvcSelected,
	bool&nfdSelected,
	bool&tdSelected,
	bool&dtdSelected,
	bool&ecSelected,
	bool&decSelected,
	bool&pkSelected,
	bool&dpkSelected,
	bool&pdSelected,
	bool&dpdSelected,
	bool&peSelected,
	bool&dpeSelected,
	PMem&assessMem,
	std::list<int>&sensorLocations,
	std::string&nodeMapFileName,
	std::string&scenarioMapFileName,
	std::ofstream&nodeMapFile,
	std::ofstream&scenarioMapFile,
	std::vector<ObjectiveBase*>&theObjectives,
        std::vector<std::string>&nodeIndexToIDMap,
        std::map<std::string,int>&nodeIDToIndexMap);

void freeMemory(PMem mem, ModuleType componentType);


int tso2ImpactAnalysis_ia_initialize(NamedDataRef *analysisOptionsRef, 
				NamedDataRef *simResultsRef);
void tso2ImpactAnalysis_ia_newSimResults(NamedDataRef *analysisOptionsRef,
				    NamedDataRef *simResultsRef, 
				    NamedDataRef *intResultsRef);
void tso2ImpactAnalysis_ia_shutdown(NamedDataRef *analysisOptionsRef);

void tso2ImpactAnalysis_aggr_initialize(NamedDataRef *analysisOptionsRef, 
				   NamedDataRef *simResultsRef);
void tso2ImpactAnalysis_aggr_newResults(NamedDataRef *analysisOptionsRef,
				   NamedDataRef *intResultsRef);
void tso2ImpactAnalysis_aggr_getResults(NamedDataRef *analysisOptionsRef,
				   NamedDataRef *resultsRef);
void tso2ImpactAnalysis_aggr_writeResults(NamedDataRef *analysisOptionsRef,
				     NamedDataRef *resultsRef, 
				     const char *dir, char *fn);
void tso2ImpactAnalysis_aggr_shutdown(NamedDataRef *analysisOptionsRef);

};

#endif
