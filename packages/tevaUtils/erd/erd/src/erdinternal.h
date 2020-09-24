#ifndef _ERDINTERNAL_H_
#define _ERDINTERNAL_H_

// --- define WINDOWS

#undef WINDOWS
#ifdef _WIN32
  #define WINDOWS
#endif
#ifdef __WIN32__
  #define WINDOWS
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(__MINGW32__) || defined(__linux__) || defined(__CYGWIN__)  || defined(__APPLE__) 
#include <unistd.h>
#else
#ifdef WINDOWS
#define S_ISDIR(m)   (((m) & S_IFMT) == S_IFDIR)  //Windows does not define S_ISDIR
#endif
#endif

#include "erd.h"
#include "teva.h"
#include "dpx.h"

#define IDX_QUAL 0x00
#define IDX_HYD 0x01

#define VELOCITY_ON 0x04
#define FLOW_ON 0x02
#define DEMAND_ON 0x01
#define PRESSURE_ON 0x08
#define PROFILE_ON 0x10
#define ALL_ON (VELOCITY_ON | FLOW_ON | DEMAND_ON | PRESSURE_ON | PROFILE_ON)

#define ISSET_VELOCITY(x) (x & VELOCITY_ON)
#define ISSET_FLOW(x) (x & FLOW_ON)
#define ISSET_DEMAND(x) (x & DEMAND_ON)
#define ISSET_PRESSURE(x) (x & PRESSURE_ON)
#define ISSET_DEMANDPROFILE(x) (x & PROFILE_ON)

#define SET_VELOCITY(x) (x |= VELOCITY_ON)
#define SET_FLOW(x) (x |= FLOW_ON)
#define SET_DEMAND(x) (x |= DEMAND_ON)
#define SET_PRESSURE(x) (x |= PRESSURE_ON)
#define SET_DEMANDPROFILE(x) (x |= PROFILE_ON)

#define IS_COMP_RLE(x) ((x)->header[2] == rle)
#define IS_COMP_LZMA(x) ((x)->header[2] == lzma)

typedef struct
{
	int first;
	int last;
} Group, *PGroup;

/* Epanet section type */
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

static void erdError(int exitCode, const char *errmsg, ...)
{
	va_list ap;

	fprintf(stderr, "\n\n********************************************************************************\n\n");
	va_start(ap, errmsg);
	vfprintf(stderr, errmsg, ap);
	va_end(ap);
	fprintf(stderr, "\n\n********************************************************************************\n\n");

	if(exitCode >= 0) 
		exit(exitCode);
}

static void MEMCHECK(void *x, char *msg)
{
	if(x == NULL)
		erdError(1, "Allocating memory: %s", msg);
}

// Internal (Not Exported) Function Prototypes

// Operating with hydraulic and quality files and simulation data
PHydraulicsSim getHydSimIdx(PQualitySim qualSimIdx, PERD db);
int getQualSimIdxFor(int hydSimIdx, int qualIdxInGroup, PERD database);
char *getHydDataFilename(PERD db, PHydraulicsSim hydSim);
char *getQualDataFilename(PERD db, PQualitySim qualSimIdx);

// File Names
char *getBaseName(const char *directory, char *baseName);
void getPrologueFilename(PERD db, char *prologueFilename, int withDir);
void getIndexFilename(PERD db, char *indexFilename, int withDir);
void getHydFilename(PERD db, int fileIdx, char *hydFilename, int withDir);
void getQualFilename(PERD db, int fileIdx, char *qualFilename, int withDir);

// Writing and Reading Database
int writePrologue(PERD database);
int readPrologue(PERD db);
int writeIndex(PERD db);
int readIndex(PERD db);
int getNLHydData(PHydraulicsSim hydSimIdx, PERD db);
int ensureQualityFileOpened(PQualitySim qualSim, PERD db);
int getNLQualData(PQualitySim qualSimIdx, PERD db);
int getDecompressedQualData(PERD db, FILE *);
int readAndDecompressRLE(FILE *stream, PERD db);
int compressAndStoreRLE(float *dtc, int numSteps, FILE *stream);
int compressAllAndStoreRLE(PERD db, FILE *stream);
int readAndDecompressLZMA(FILE *stream, PERD db);
int getHydResults(PHydraulicsSim hydSim, PERD database);
int getQualResults(PQualitySim qualSim, PERD database);

// Initializing Database
PERD initializeDatabase(const char *erdName);
void getBaseNameAndDirectory(PERD db, char *fn);
static PNetInfo initializeNetInfo(int ns);
static void initializeNetworkData(PNodeInfo *nodes, PLinkInfo *links, PNetInfo net);
int setAppRWFunctions(PERD db, enum OutputFrom application);

// Memory management
void releaseNetInfo(PNetInfo network);
void freeQual(int numSpecies, int numLinks, int numNodes, PQualData qual, PSpeciesData *species);
void freeHyd(int numSteps, PHydData hyd);

// Utility
int getDatabaseType(PERD db);
int currentHyd(PHydraulicsSim hydSim, PERD db);
int currentQual(PQualitySim qualSimIdx, PERD db);
void positionFile(FILE *fp,__file_pos_t offs);
__file_pos_t fileOffset(FILE *fp);
void sortByGroup(PQualitySim *list, int size);

// TEVA Legacy code
void loadSources(FILE *stream, PSource sources, PQualitySim qualSim) ;

#endif
