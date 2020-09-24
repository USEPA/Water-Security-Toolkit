#ifndef ENL_H_
#define ENL_H_

#include "erd.h"
#include "epanet2.h"
#include "epanetmsx.h"

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


//static int ERRNUM;

#define MAXERRMSG 100
#define UCHAR(x) (((x) >= 'a' && (x) <= 'z') ? ((x)&~32) : (x))
#define ENCHECK(x) (((ERRNUM = x) > 0) ? (epanetError(ERRNUM)) : (0))
#define MSXCHECK(x) (((ERRNUM = x) > 0) ? (epanetmsxError(ERRNUM)) : (0))

LIBEXPORT(int)  epanetmsxError(int errorCode);
LIBEXPORT(int)  epanetError(int errorCode);
LIBEXPORT(int)  ENL_getNetworkData(PERD database, const char *inputFile, const char *msxInputFile,char *msx_species);
LIBEXPORT(int)  ENL_getHydResults(int time, long timeStep, PERD database);
LIBEXPORT(int)  ENL_getQualResults(int time, long timeStep, PERD database);
LIBEXPORT(void) ENL_createDemandProfiles(PERD db);
LIBEXPORT(int)  ENL_saveInitQual(PNetInfo net, double ***initQual);
LIBEXPORT(int)  ENL_restoreInitQual(PNetInfo net, double **initQual);
LIBEXPORT(int)  ENL_releaseInitQual(PNetInfo net, double ***initQual);
LIBEXPORT(int)  ENL_setSource(PSourceData source, PNetInfo net, FILE *simin,int isMSX);
LIBEXPORT(void) ENL_writeTSI(PNetInfo net, PNodeInfo nodes, PSource sources, FILE *simgen, FILE *simin);

#endif /*ENL_H_*/
