/*
 * Copyright (c) 2008 UChicago Argonne, LLC
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
#ifndef _INFSTRIMPACTS_H_
#define _INFSTRIMPACTS_H_
#include <stdio.h>

#include "infrstr_assess.h"
#include "ExternalAnalysis.h"
#include "DiskCachedData.h"

JNIEXPORT PII_Data loadIIAOptions(void *analysisOptionsRef, void *simResultsRef, ModuleType componentType);
JNIEXPORT void freeIIAMemory(PII_Data iid, ModuleType componentType);

JNIEXPORT int IIAnalysis_ia_initialize(NamedDataRef *analysisOptionsRef, NamedDataRef *simResultsRef);
JNIEXPORT void IIAnalysis_ia_newSimResults(NamedDataRef *analysisOptionsRef,NamedDataRef *simResultsRef, NamedDataRef *intResultsRef);
JNIEXPORT void IIAnalysis_ia_shutdown(NamedDataRef *analysisOptionsRef);

JNIEXPORT void IIAnalysis_aggr_initialize(NamedDataRef *analysisOptionsRef, NamedDataRef *simResultsRef);
JNIEXPORT void IIAnalysis_aggr_newResults(NamedDataRef *analysisOptionsRef,NamedDataRef *intResultsRef);
JNIEXPORT void IIAnalysis_aggr_getResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef);
JNIEXPORT void IIAnalysis_aggr_writeResults(NamedDataRef *analysisOptionsRef,NamedDataRef *resultsRef, const char *dir, char *fn);
JNIEXPORT void IIAnalysis_aggr_shutdown(NamedDataRef *analysisOptionsRef);

#endif
