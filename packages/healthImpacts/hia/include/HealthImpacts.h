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
#include <stdio.h>

#include "teva-assess.h"
#include "ExternalAnalysis.h"
#include "DiskCachedData.h"

typedef struct WorstCaseCachedData {
	float **data;
	XA na;
} WorstCaseCachedData, *WorstCaseCachedDataPtr;

typedef struct WorstCaseData {
	CachedData cData;
	struct WorstCaseData *next;
} WorstCaseData, *WorstCaseDataPtr;

typedef struct WorstCase {
	int numToKeep;
	int num;
	int numCalls;
	int numRetrievals;
	double nthWorst;
	WorstCaseDataPtr worst;
	FILE *fpcache;
	char fn[256];
	int nnodes;
	int nsteps;
} WorstCase, *WorstCasePtr;

JNIEXPORT PMem initializeMem(void *simResultsRef, int speciesIndex);
JNIEXPORT PMem loadHIAOptions(void *analysisOptionsRef, void *simResultsRef, ModuleType componentType);
JNIEXPORT PMem allocateBaseMemory(void *simResultsRef, int speciesIndex, int needRho);
JNIEXPORT void freeHIAMemory(PMem mem, ModuleType componentType);
JNIEXPORT PDoseOverThresh allocateDoseOverThresholdWithIDs(void *analysisOptionsRef, PMem mem);
JNIEXPORT PDoseOverThresh allocateDoseOverThreshold(void *analysisOptionsRef, PMem mem);
JNIEXPORT void freeDoseOverThresholdData(PMem mem);

JNIEXPORT int HIAnalysis_ia_initialize(NamedDataRef *analysisOptionsRef, NamedDataRef *simResultsRef);
JNIEXPORT void HIAnalysis_ia_newSimResults(NamedDataRef *analysisOptionsRef,NamedDataRef *simResultsRef, NamedDataRef *intResultsRef);
JNIEXPORT void HIAnalysis_ia_shutdown(NamedDataRef *analysisOptionsRef);

JNIEXPORT void HIAnalysis_aggr_initialize(NamedDataRef *analysisOptionsRef, NamedDataRef *simResultsRef);
JNIEXPORT void HIAnalysis_aggr_newResults(NamedDataRef *analysisOptionsRef,NamedDataRef *intResultsRef);
JNIEXPORT void HIAnalysis_aggr_getResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef);
JNIEXPORT void HIAnalysis_aggr_writeResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn);
JNIEXPORT void HIAnalysis_aggr_shutdown(NamedDataRef *analysisOptionsRef);
