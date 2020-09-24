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
#ifndef EXTERNAL_ANALYSIS_H
#define EXTERNAL_ANALYSIS_H

#include "NamedData.h"

typedef struct Functions {
	void (*aggr_initialize)(NamedDataRef *, NamedDataRef *);
	void (*aggr_newResults)(NamedDataRef *, NamedDataRef *);
	void (*aggr_getResults)(NamedDataRef *, NamedDataRef *);
	void (*aggr_writeResults)(NamedDataRef *, NamedDataRef *, const char *, char *);
	void (*aggr_shutdown)(NamedDataRef *);

	int  (*ia_initialize)(NamedDataRef *, NamedDataRef *);
	void (*ia_newSimResults)(NamedDataRef *,NamedDataRef *, NamedDataRef *);
	void (*ia_shutdown)(NamedDataRef *);

} Functions, *FunctionsPtr;

typedef enum ModuleType
{
        Aggregation,
        IntermediateAnalysis
} ModuleType;



/* return values from ia_initialize method */
#define NON_REMOTE 0
#define REMOTE     1

#endif
