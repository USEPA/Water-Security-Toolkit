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
#ifndef _TEVA_ASSESS_H_
#define _TEVA_ASSESS_H_

#include "tevautil.h"
#include "ta_input.h"
#include "ta_utils.h"
#include "spline_if.h"

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


/*
**-------------------------------------------------------------------
** Sample or sensor technology
**-------------------------------------------------------------------
*/
enum SampleType
{
	realtime,       /* Realtime continuous measurement */
	composite,      /* Composite grab sampling */
	filtered        /* Continuous filtered sampling */
};

/*
**-------------------------------------------------------------------
** Sensor locations
**-------------------------------------------------------------------
*/
enum SensorType
{
	ignore,         /* no sensor here */
	existing,       /* Current sensor location */
	potential,      /* Potential new sensor location */
	selected        /* newly selected sensor here */
};

/*
**-------------------------------------------------------------------
** Ingestion quantity model
** (Placeholder - only DEMAND available)
**-------------------------------------------------------------------
*/ 
enum IngType
{
	demand
};

/*
**-------------------------------------------------------------------
** Characteristic dose model
** (Placeholder - only TOTAL available)
**-------------------------------------------------------------------
*/ 
enum DoseType
{
	totalmass
};
/*
**-------------------------------------------------------------------
** Dose-response model model
** (Old will be removed in the future - included only for now for
**  testing)
**-------------------------------------------------------------------
*/ 
enum ResponseType 
{
	Probit,
	Old
};
/*
**-------------------------------------------------------------------
** Dose-response model model
** (Old will be removed in the future - included only for now for
**  testing)
**-------------------------------------------------------------------
*/ 
enum Population 
{
	DemandBased,
	FileBased
};

enum IngestionTiming
{
	IMATUS, IMRandom, IMFixed5, IMFixed2, IMDemand, IMUserFixed
};
enum IngestionVolume {
	VMMean, VMRandom, VMDemand
};
typedef struct
{
	char *nodeid;
	float population;
	int   used;
} NodePopulation, *PNodePopulation;

typedef struct
{
	enum  DoseType dtype;
	enum  ResponseType responseType;
	char *speciesName;
	int   speciesIndex;
// for sigmoid DoesResponse
	float a;
	float m;
	float n;
	float tau;
// for Probit
    float ld50;
	float beta;

	float bodymass;
	int   normalize;
	int   nL;
	int   nF;
	int   maxsteps;
	float frate;
	float protectlimit;
} DR;
typedef DR *PDR;

typedef struct {
	enum  Population population;
	PNodePopulation pop;
	int   popLength;
	float pcu;
}PopulationData, *PopulationDataPtr;

typedef struct {
	union VolumeData {
		double volume;  // will not have data for VM_DEMAND/IM_DEMAND case
		float *volumes; // Only used for VM_DEMAND/IM_DEMAND
	} v;
	double *times;
	int ntimes;
	float response;  // only set for
} IndividualIngestionData, *IndividualIngestionDataPtr;

typedef struct {
	enum IngestionVolume volumeMode;
	enum IngestionTiming timingMode;
	SplineDataPtr fVol;
	SplineDataPtr *fTiming;

	float meanVolume;  // only used for volumeMode=VMMean
	float phi;         // only used for volumeMode=VMDemand
	int numUserTimes;  // only used for timingMode=IMUserFixed
	float *userTimes; // only used for timingMode=IMUserFixed
	int numNodes;
	int maxsteps;
	int nsteps;
	float **nodeDoses;  // temporary 2-d work array (saves lots of time due to alloc/free - esp. free)
	IndividualIngestionDataPtr **ingestionData;
} IngestionData, *IngestionDataPtr;

typedef struct {
	enum  SensorType type;
	enum  SampleType sample;
    int  rank;
	float limit;
	float vol;
	int  freq;
	int  start;
	int  delay;
} Sensor, *PSensor;

typedef struct
{
	PNodeInfo info;
	int pop;
	float *rho;
	Sensor sensor;
} Node;
typedef Node *PNode;

typedef struct
{
	PLinkInfo info;
} Link;
typedef Link *PLink;

typedef struct
{ 
//	FILE *taistream;
	FILE *taostream;
//	char tainame[MAXFNAME];
	char taoname[MAXFNAME];
	PTSO tso;
} IO;
typedef IO *PIO;

typedef struct
{
	PNetInfo info;
	int nscenario;
} Net;
typedef Net *PNet;

typedef struct
{
	float *dos;
	float *res;
	float *s;
	float *i;
	float *d;
	float *f;
} XA;
typedef XA *PXA;

typedef struct
{
	float *dos;
	float *res;
	float *s;
	float *i;
	float *d;
	float *f;
} TA;
typedef TA *PTA;

/** NEW **/
typedef struct HIST
{
	char *simID;
	char *injDef;
	char *injNodeType;
	float testh;
	float fh;
	float dosh;
	float resh;
	int *dot;
	int *doseBins;
	struct HIST *next;
} HIST;
typedef HIST *PHIST;
/** End NEW **/

typedef struct
{
	float *dos;
	float *res;
	float *s;
	float *i;
	float *d;
	float *f;
} U;
typedef U *PU;

typedef struct {
  int numBins;
  float *responseBins;
  float *doseBins; // bins will have been converted to doses before being assigned - length: numBins
  int *data; // length: numBins;
} DoseBinData, *PDoseBinData;

typedef struct {
  int numThresh; // number of thresholds specified
  float *thresholds;  // individual thresholds;
  char **threshIDs;  // threshold IDs - used in creating output filenames...
  int **numOver;  // first dimension: numThresh  second dimension: nnodes
  int *totOver; // dimension: numThresh - total over the threshold for the entire scenario
  int *nodesOver; // dimension: numThresh - total nodes with a dose over the threshold for the entire scenario
  int **numOverByTime; // first dimension: numThres second dimension: 
} DoseOverThresh, *PDoseOverThresh;

typedef struct
{
	PTSO         tso;
	PERD         erd;
	PIO          io;
	PDR          dr;
	PNode        node;
	PLink        link;
	PopulationDataPtr popData;
	IngestionDataPtr  ingestionData;
	PNodeInfo    nodeinfo;
	PLinkInfo    linkinfo;
	PNet         net;
	PXA          xa;
	PTA          ta;
	/** NEW **/
	PHIST	     hist; 
	/** End NEW **/
	PU           u;
	float        concentrationThreshold;
	PDoseOverThresh dot;
	PDoseBinData  doseBins;
} Mem;
typedef Mem *PMem;

extern char *VolumeModeDesc[];
extern char *VolumeModeAbbr[];
extern char *IngestionModeDesc[];
extern char *IngestionModeAbbr[];

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
LIBEXPORT(int) Detect(PNet,PNode,int,int speciesIndex);
float cavg(int, int, int, float*);
LIBEXPORT(void) Population(PNet net,PopulationDataPtr pdp,PNode node);
LIBEXPORT(void) initializeIngestionData(IngestionDataPtr p, PNet net, PNode nodes, PDR dr, PopulationDataPtr pd);
LIBEXPORT(float *) createUserFixedTimesArray(IngestionDataPtr ing, float *inputTimes, int numTimes);
void  IngestionRate(PNet,PNode);
void  ResponseOld(PNet,PDR,PU);
LIBEXPORT(void) Response(IndividualIngestionDataPtr *idp, IngestionDataPtr ingData, PDR dr,PNode node, PU u);

/*void  DoseOld(int,PNet,PDR,PU,PNode,float);*/
LIBEXPORT(void) Dose(IndividualIngestionDataPtr *idp, IngestionDataPtr ingData, PNode node, PNet net, int detectTime, PU u, PDoseOverThresh dot, int nodeidx, PDoseBinData doseBins,int speciesIndex, float *avgDose);

LIBEXPORT(float) DoseResponse(float,PDR);
LIBEXPORT(float) ResponseDose(float response, PDR dr);
void  Disease(PNet,PDR,PU);
/** Regan's modifications to the Disease progression code **/
LIBEXPORT(void) DiseaseRM(PNet,PDR,PU);
LIBEXPORT(void) initialize(PNet,PDR,PXA,PTA);
LIBEXPORT(int)  getnodeindex(PNet,PNode,char*);
//void  Accumulate(int,int,PNet,PDR,float,PU,PXA,PTA);
/** UPDATE **/
LIBEXPORT(void) Accumulate(int inod,int nz,PNet net,PDR dr,int pop,PU u,PXA xa,PTA ta,PHIST hist);
/** End UPDATED **/
/** NEW **/
LIBEXPORT(void) AccumulateFinal(PNet net,PDR dr,PXA totalXA,PXA interimXA);
/** End NEW **/
float findmax(int,float*);

Mem InitializeAssessment(char *taiFile, char *sssFile);
LIBEXPORT(void) processTAIData(PInputFileData taiIFD, PMem mem);
LIBEXPORT(void) InitializeAssessMemory(PMem mem,PNetInfo netinfo);
LIBEXPORT(void) dumpHIAInputParameters(PMem mem);

/** NEW **/
LIBEXPORT(void) resetDoseOverThreshold(PMem mem);
LIBEXPORT(void) resetDoseBinData(PMem mem);
LIBEXPORT(void) initXA(PXA xa, int len);
LIBEXPORT(void) resetXA(PXA xa, int len);
LIBEXPORT(void) freeXA(PXA xa, int len);
LIBEXPORT(PHIST) allocateHistData(char *id);
LIBEXPORT(void) freeHistogramData(PHIST hist);
LIBEXPORT(void) initTA(PTA ta, int len);
LIBEXPORT(void) resetTA(PTA ta, int len);
LIBEXPORT(void) freeTA(PTA ta, int len);
void initU(PU u, int len);
void resetU(PU u, int len);
void freeU(PU u, int len);
LIBEXPORT(void) FreeAssessMemory(PMem mem);
LIBEXPORT(void) freeIngestionData(IngestionDataPtr idp, PNode nodes);
LIBEXPORT(void) set_seed(unsigned long seed);
/** End NEW **/

#endif
