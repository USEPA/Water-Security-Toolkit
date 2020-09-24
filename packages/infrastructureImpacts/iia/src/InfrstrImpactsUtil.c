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
#include "ExternalAnalysis.h"
#include "AnalysisModuleUtils.h"
#include "NamedData.h"
#include "InfstrImpacts.h"

enum SensorType getSensorType(char *typeStr);
enum SampleType getSampleType(char *sampleTypeStr);

JNIEXPORT PII_Data loadIIAOptions(void *analysisOptionsRef, void *simResultsRef, ModuleType componentType)
{
	PII_Data iid;
	int numThresh;
	float *fvals;
	int i;

	iid=(PII_Data)calloc(1,sizeof(II_Data));
	if(getInt(analysisOptionsRef, "SpeciesIndex", &iid->speciesIndex)==ND_FAILURE) return NULL;
	initializeNetwork(simResultsRef,iid->speciesIndex,&iid->net, &iid->nodeinfo, &iid->linkinfo);

	if(getInt(analysisOptionsRef,"NumConcentrationThresholds",&numThresh)==ND_FAILURE) return NULL;
	iid->pot=AllocatePFOTData(numThresh,iid->net->numLinks);
	// inputs to IIA Module:
	// Concentration thresholds & num
	// list of scenarios (injection definitions) to save results for

	if(getFloatArray(analysisOptionsRef,"ConcentrationThresholds",&fvals)==ND_FAILURE) return NULL;
	for(i=0;i<iid->pot->numThresh;i++) {
		iid->pot->thresholds[i]=fvals[i];
	}
	nd_free(fvals);
	return iid;
}

JNIEXPORT void freeIIAMemory(PII_Data iid, ModuleType componentType) {

	FreePFOTData(&iid->pot);
	freeNetwork(&iid->net, &iid->nodeinfo, &iid->linkinfo);
}

