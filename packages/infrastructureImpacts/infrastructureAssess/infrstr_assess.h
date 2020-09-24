/*
 * Copyright (c) 2012 UChicago Argonne, LLC
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
#ifndef _TEVA_ASSESS_H_
#define _TEVA_ASSESS_H_

#include "tevautil.h"

/* Global Conversion macros */
#define FILEOPEN(x) (((x) == NULL) ? (ta_error(1,"Opening files")) : (0))
#define MEMCHECK(x) (((x) == NULL) ? (ta_error(1,"Allocating memory")) : (0))
#define RES(x)  (((x) < 0) ? (0) : (res[x]))
#define INT(x)   ((int)(x))                   /* integer portion of x  */
#define FRAC(x)  ((x)-(int)(x))               /* fractional part of x  */
#define ABS(x)   (((x)<0) ? -(x) : (x))       /* absolute value of x   */
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))     /* minimum of x and y    */
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))     /* maximum of x and y    */
#define ROUND(x) (((x)>=0) ? (int)((x)+.5) : (int)((x)-.5)) /* round-off of x        */
#define MOD(x,y) ((x)%(y))                    /* x modulus y           */
#define SGN(x)   (((x)<0) ? (-1) : (1))       /* sign of x             */

/* Global Data Types */

typedef unsigned int UINT;


typedef struct IIAResults
{
	char *simID;
	char *injDef;
	float *pfot;   // pipe feet over threshold.  dimension: numThresh
	float **pfotd; // pipe feet over threshold by pipe diameter.  dimensions: numThresh,numDiameters 
	struct IIAResults *next;
} IIAResults, *PIIAResults;

typedef struct {
  float *feetOver; // dimension: numThresh - total feet of pipe over the threshold for the entire scenario
  int *linksOver;  // number of links with with concentration over each threshold for entire scenario
  float totalFeet;
} PipeOverThreshData, *PPipeOverThreshData;

typedef struct {
  float diameter;
  PipeOverThreshData potd;
} PipeOverThreshDataByDiameter, *PPipeOverThreshDataByDiameter;

typedef struct {
  int numThresh; // number of thresholds specified
  float *thresholds;  // individual thresholds;
  float minThreshold;
  char **threshIDs;  // threshold IDs - used in creating output filenames...
  float **pipeFeetOver; // feet of each pipe contaminated over the threshold (currently 0 or the length of the pipe).  dimensions: threshold,link
  PipeOverThreshData potd;
  int numDiameters;
  PPipeOverThreshDataByDiameter *potdDiam;
} PipeOverThresh, *PPipeOverThresh;

typedef struct
{
	PTSO         tso;
	PERD         erd;
	PNetInfo     net;
	int          nscenario;
	char         *speciesName;
	int          speciesIndex;
	PNodeInfo    nodeinfo;
	PLinkInfo    linkinfo;
	PIIAResults  iiaResults; 
	PPipeOverThresh pot;
} II_Data, *PII_Data;

#ifndef LIBEXPORT
#ifdef WINDOWS
  #ifdef __cplusplus
  #define LIBEXPORT(type) extern "C" __declspec(dllexport) type __stdcall
  #else
  #define LIBEXPORT(type) __declspec(dllexport) type __stdcall
  #endif
#else
#define LIBEXPORT(type) type
#endif
#endif

/* Function Prototypes */
LIBEXPORT(void) InitializeIAMemory(PII_Data iid);
LIBEXPORT(void) FreeIAMemory(PII_Data iid);
LIBEXPORT(void) resetPipeFeetOverThreshold(PII_Data iid);
LIBEXPORT(void) pipesContaminated(PII_Data iid);
LIBEXPORT(void) freeIIAResultsData(PIIAResults hist, int numThresh);
LIBEXPORT(void) addPOT(float diameter, PPipeOverThresh pot, int dIndex);
LIBEXPORT(PPipeOverThresh) AllocatePFOTData(int numThresh,int numLinks);
LIBEXPORT(PPipeOverThreshDataByDiameter) allocatePipeOverThreshDataByDiameter(float diameter, int numThresh);
LIBEXPORT(void) FreePFOTData(PPipeOverThresh *ppot);
LIBEXPORT(PIIAResults) allocateIIAResultsData(char *simID, PII_Data iid);
#endif
