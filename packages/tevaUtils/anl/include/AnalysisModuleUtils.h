#ifndef _ANALYSIS_MODULE_UTILS_H_
#define _ANALYSIS_MODULE_UTILS_H_

#include "erd.h"
#include "NamedData.h"

LIBEXPORT(int) initializeNetwork(void *simResultsRef, int speciesIndex,
								 PNetInfo *pnet, PNodeInfo *pnodeinfo, PLinkInfo *plinkinfo);
LIBEXPORT(void) freeNetwork(PNetInfo *pnet, PNodeInfo *pnodeinfo, PLinkInfo *plinkinfo);
LIBEXPORT(void) loadFlow(NamedDataRef *simResultsRef, PNetInfo net);
LIBEXPORT(void) loadQuality(NamedDataRef *simResultsRef, PNetInfo net, PNodeInfo nodeinfo, int speciesIndex);

#endif
