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
#ifndef _TEVAUTIL_H_
#define _TEVAUTIL_H_

#include <stdio.h>
#include <float.h>
#include "erd.h"
/*#include "tevatypes.h" */
#include "tso.h"

/*DEF_ERD #define MAXFNAME    512      *//* Maximum chars in file name */
#define MAXSPECIEID 16       /* Maximum chars in Specie ID */

/*
**-------------------------------------------------------------------
** Exported methods
**-------------------------------------------------------------------
*/
/* TSO.C */
LIBEXPORT(int) TSO_isTSO(const char *tsoName);
LIBEXPORT(PTSO) TSO_Open(char *filenameOrDirectory, char *pattern);
LIBEXPORT(PTSO) TSO_OpenNoHeader(char *filenameOrDirectory, char *pattern, char fileVersion, char storageMethod);
LIBEXPORT(void) TSO_PositionAfterPrologue(PTSO tso);
LIBEXPORT(void) TSO_ReadPrologue(PTSO tso, PNetInfo*, PNodeInfo *nodes,PLinkInfo *links, int flags);
LIBEXPORT(int) TSO_ReadSimulationResults(PTSO tso, PNetInfo net,  PNodeInfo nodes);
LIBEXPORT(int) TSO_GetSimulationResults(int id, PTSO tso, PNetInfo net, PNodeInfo nodes);
LIBEXPORT(int) TSO_GetRawSimulationResults(int id, PTSO tso, PNetInfo net, PNodeInfo nodes, char *buf, int len);
LIBEXPORT(PTSO) TSO_Create(char *filename, int compressionLevel,PNetInfo net);
LIBEXPORT(PTSO) TSO_CreateSpecificVersion(char *filename, int compressionLevel, int fileVersion, PNetInfo ne);
LIBEXPORT(int) TSO_WritePrologue(PTSO tso, PNetInfo net, PNodeInfo nodes, PLinkInfo links);
LIBEXPORT(int) TSO_WriteSimResults(PTSO tso, PNetInfo net,PTEVAIndexData source, PNodeInfo nodes);
LIBEXPORT(void) TSO_Close(PTSO *tso);
LIBEXPORT(PNetInfo) TSO_AllocateNetworkInfo(int nspecies);
LIBEXPORT(void) TSO_ReleaseNetworkInfo(PNetInfo *net);
LIBEXPORT(void) TSO_AllocateNetworkData(PNodeInfo *nodes, PLinkInfo *links, PNetInfo net, int flags);
LIBEXPORT(void) TSO_ReleaseNetworkData(PNodeInfo *nodes, PLinkInfo *links, PNetInfo net);
LIBEXPORT(char *) TSO_GetCompressionDesc(PTSO db);
LIBEXPORT(int) TSO_getCompressionLevel(PTSO db);

/* TSO compatibility "helpers" */
#define get_count(erd,tso) (erd!=NULL?erd->qualSimCount:tso->count)

LIBEXPORT(int) loadSimulationResults(int id,PERD erd,PTSO tso,PNetInfo net,PNodeInfo nodes,PTEVAIndexData *source);

#endif
