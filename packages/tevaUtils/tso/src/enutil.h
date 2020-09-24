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
#ifndef _ENUTIL_H_
#define _ENUTIL_H_

#include <stdio.h>
#include <float.h>
#include "tevatypes.h"

//#ifdef WIN32

//#ifdef MSEPANET  /* Multi-species or Normal Epanet header */
//#include "epanet2ms.h"
//#else
//#include "epanet2.h"
//#endif

//#else

//#if defined(__linux__) || defined(__CYGWIN__)

#ifdef MSEPANET  /* Multi-species or Normal Epanet header */
#include "toolkitms.h"
#else
#include "toolkit.h"
#endif

//#endif

//#endif

#define MAXLINE  10239       /* Max input line chars       */
#define MAXTOKS  500         /* Max input line tokens      */
#define SEPSTR    " \t\n\r"  /* Token separator characters */
#define MAXERRMSG   100      /* Max size of epanet err message    */

int ERRNUM;
#define ENCHECK(x) (((ERRNUM = x) > 0) ? (ENTU_EpanetError(ERRNUM)) : (0))
#define UCHAR(x) (((x) >= 'a' && (x) <= 'z') ? ((x)&~32) : (x))
#define MOD(x,y) ((x)%(y))
#define FILEWRITE(x,y) (((x) < (unsigned int)(y)) ? (enutilerror(1,"Too few bytes written")) : (0))

/* Epanet section type */
#ifdef MSEPANET
enum SectType    {_TITLE,    _JUNCTIONS,_RESERVOIRS,
                  _TANKS,    _PIPES,    _PUMPS,
                  _VALVES,   _CONTROLS, _RULES,
                  _DEMANDS,  _SOURCES,  _EMITTERS,
                  _PATTERNS, _CURVES,   _QUALITY,
                  _STATUS,   _ROUGHNESS,_ENERGY,
                  _REACTIONS,_MIXING,   _REPORT,
                  _TIMES,    _OPTIONS,  _COORDS,
                  _VERTICES, _LABELS,   _BACKDROP,
                  _TAGS,     _END,      _MSVAR,    
                  _MSPIPE,   _MSTANK,   _MSOPTIONS};
#else
enum SectType    {_TITLE,    _JUNCTIONS,_RESERVOIRS,
                  _TANKS,    _PIPES,    _PUMPS,
                  _VALVES,   _CONTROLS, _RULES,
                  _DEMANDS,  _SOURCES,  _EMITTERS,
                  _PATTERNS, _CURVES,   _QUALITY,
                  _STATUS,   _ROUGHNESS,_ENERGY,
                  _REACTIONS,_MIXING,   _REPORT,
                  _TIMES,    _OPTIONS,  _COORDS,
                  _VERTICES, _LABELS,   _BACKDROP,
                  _TAGS,     _END};
#endif
  
/*
**-------------------------------------------------------------------
** Exported methods
**-------------------------------------------------------------------
*/
#ifdef DLL
  #ifdef __cplusplus
  #define DLLEXPORT extern "C" __declspec(dllexport) __stdcall
  #else
  #define DLLEXPORT __declspec(dllexport) __stdcall
  #endif
#elif defined(CYGWIN)
  #define DLLEXPORT __stdcall
#else
  #ifdef __cplusplus
  #define DLLEXPORT
//  #define DLLEXPORT extern "C"
  #else
  #define DLLEXPORT
  #endif
#endif

/* ENUTIL.C */
void DLLEXPORT ENTU_Initialize(char *epanetfname, char *tsofname, int storageMethod, int tsofileversion, PTSO *tso, PNetInfo *net, PNodeInfo *nodes, PLinkInfo *links, PSource *sources);
void DLLEXPORT ENTU_CreateDemandPatterns(PNetInfo net, PNodeInfo nodes);
void DLLEXPORT ENTU_GetLinkQData(PLinkInfo, PNetInfo, int, long );
void DLLEXPORT ENTU_GetNodeQData(PNodeInfo, PNetInfo, int, long );
void DLLEXPORT ENTU_ClearNodeQData(PNodeInfo, PNetInfo );
void DLLEXPORT ENTU_GetLinkHData(PLinkInfo, PNetInfo, int, long );
void DLLEXPORT ENTU_GetNodeHData(PNodeInfo, PNetInfo, int, long );
void DLLEXPORT ENTU_GetNodeXY(PNodeInfo, PLinkInfo, PNetInfo, char*);
void DLLEXPORT ENTU_EpanetError(int);
void DLLEXPORT ENTU_WriteTSI(PNetInfo, PNodeInfo, PSource, FILE*, FILE*);
int  DLLEXPORT ENTU_SetSource(PSource, PNetInfo, FILE*);

#endif
