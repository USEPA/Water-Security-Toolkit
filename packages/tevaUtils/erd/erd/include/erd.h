#ifndef _ERD_H_
#define _ERD_H_

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
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0
/* ERD File Version Number:
 * 12: first ERD version.  It has been set to 11, but that is the last TSO version, so to avoid
 *     confusion, I am changing it to 12.
 * 13: added pipe diameter (used for infrastructire impacts analysis)
 */
#define ERD_VERSION  13
/* FIRST_ERD_VERSION is used to determine if a file is a TSO format or ERD format */
#define FIRST_ERD_VERSION  12

#define MAX_ID_LENGTH 64	/* Maximum length of node, link, and species IDs */
#define MAXFNAME 1024		/* Maximum filename length */
#define HEADER_SIZE 32		/* Size of database header, in prologue file */
#define MAXLINE 10239		/* Maximum number of characters in an input file line */
#define MAXTOK 500			/* Maximum number of tokens on a line */
#define SEPSTR " \t\n\r"	/* Token separation string */

#define MULTISPECIES 4		/* EPANET-MSX qualcode */
#define MAXTOKS 500			/* Maximum input line tokens */
#define MAXSOURCES 100      /* Maximum number of sources - TEVA */

// Error codes returned
#define DB_UNABLE_TO_POSITION    -1
#define DB_DATA_BUFFER_TOO_SMALL -2
#define DB_INVALID_SOURCE_ID     -3

static int ERRNUM;
#define ERDCHECK(x) (((ERRNUM = x) > 0) ? (ERD_Error(ERRNUM)) : (0))

#ifdef WINDOWS
#include <io.h>
typedef __int64 __file_pos_t;
#else
#if defined(__linux__) || defined(__CYGWIN__)  || defined(__APPLE__) 
#include <sys/types.h>
typedef int64_t __file_pos_t; /* In Darwin (OS X), off_t is always 64-bit int anyway */
#endif
#endif

#ifdef WINDOWS
  #ifdef __cplusplus
  #define LIBEXPORT(type) extern "C" __declspec(dllexport) type __stdcall
  #else
  #define LIBEXPORT(type) __declspec(dllexport) type __stdcall
  #endif
#else
#define LIBEXPORT(type) type
#endif

/* Specie Types */
enum SpecieTypes
{
	bulk,
	wall
};

/* Node Types */
enum NodeTypes
{
	junction,
	tank
};

/* Applications that use this database for output */
enum OutputFrom
{
	teva,
	epanetdpx,
	undefinedApp
};

enum CompressionLevel
{
	none,     /* not supported.
			     Original version � all concentration data stored � one record
				 for each time step.*/
	nonzero,  /* not supported.
			     Jim�s storage reduction scheme � essentially one record for
				 each node that has at least one non-zero value. */
	rle_zero, /* not supported.
			     Each row of data is further reduced in size by doing a
				 modified run length encoding. */
	rle,      /* Each row is further reduced by also looking for repeating
			     float values and storing information to rebuild that rather
				 than store them all. */
	lzma      /* LZMA encode the concentration data. */
};
/* Database status */
enum DbStatus
{
	created,
	opened,
	undefined
};

/* Species Data */
typedef struct
{
	char id[MAX_ID_LENGTH + 1];				/* Species name */
	int index;								/* MSX Species index; -1 indicates 'species not stored' */
	enum SpecieTypes type;					/* Specie type - 'bulk' or 'wall' */
} SpeciesData, *PSpeciesData;

/* Source Data - Used only in TEVA format */
typedef struct
{
	char sourceNodeID[MAX_ID_LENGTH + 1];		/* Node ID */
	int sourceNodeIndex;					/* Node index */
	int sourceType;							/* Source type */
	int speciesIndex;						/* Species index */
	long sourceStart;						/* Source start time */
	long sourceStop;						/* Source stop time */
	float sourceStrength;					/* Source strength */
} Source, *PSource;

typedef struct
{
	int nsources;							/* Number of sources */
	PSource source;							/* Source data */
} SourceData, *PSourceData;

/* Node Data */
typedef struct
{
	enum NodeTypes type;					/* Junction or Tank */
	float x;								/* X position */
	float y;								/* Y position */
	char id[MAX_ID_LENGTH + 1];				/* Node ID */
											/* Demand profile represents the repeating demands that 
											   are specified in the EPANET input file.  The length
											   is calculated by the LCM of all demand patterns for
											   the node.  The actual values are the sum (at each
											   time step) of all the base demands multiplied by the
											   demand multiplier.  This was added to ensure consistent
											   average demands regardless of the simulation length */
	float *demandProfile;					/* Demand profile */
	int demandProfileLength;				/* Length of demand profile */
	int *nz;								/* Specie indices that contain non-zero data */
} NodeInfo, *PNodeInfo;

/* Link Data */
typedef struct
{
	int from;								/* Link from node */
	int to;									/* Link to node */
	float length;							/* Link length */
	float diameter;                         /* diameter of link */
	char id[MAX_ID_LENGTH + 1];				/* Link ID */
	int nv;									/* Number of vertices */
	float *vx;								/* X coordinates of vertices */
	float *vy;								/* Y coordinates of vertices */
	int *nz;                                /* Specie indices that contain non-zero data */
} LinkInfo, *PLinkInfo;
/* Hydraulics Data */
typedef struct
{
	float **flow;							/* Link flows - time, link */
	float **velocity;						/* Link velocities - time, link */
	float **demand;							/* Node demands - time, node */
	float **pressure;						/* Node head - time, node */
	int **linkStatus;						/* Link Status - time, link (link indexed by NetInfo controlLink index -- base 0) */
} HydData, *PHydData;

/* Water Quality Data 
   Note that for normal epanet simulation the specie is naturally a bulk specie.
   For epanetmsx the type can be either wall or bulk, which determines whether 
   they are stored as link data (wall) or node data (bulk).  The nodeC and linkC
   arrays are fully allocated for all species indices, so that specie index is
   still the way to access these data.  But the [time][node/link]
   memory is only allocated for the appropriate type, not for both.
*/
typedef struct
{
	float ***nodeC;							/* Node concentrations - species, node, time */
	float ***linkC;							/* Link concentrations - species, link, time */
} QualData, *PQualData;


/* Network Data */
typedef struct
{
	long simDuration;						/* Duration of the simulation */
	long simStart;							/* Start time of the simulation */
	long reportStart;						/* Report start time */
	long reportStep;						/* Report time step */
	int numNodes;							/* Number of nodes */
	int numLinks;							/* Number of links */
	int numTanks;							/* Number of tanks */
	int numJunctions;						/* Number of junctions */
	int numSpecies;							/* Number of species */
	PSpeciesData *species;					/* Species data */
	int numSteps;							/* Number of reporting time steps */
	int qualCode;							/* EPANET/MSX quality code */
	float stepSize;							/* Size of reporting step */
	float fltMax;							/* Maximum value of float */
	int numControlLinks;					/* Number of controlled links */
	int *controlLinks;						/* Vector of controlled erd link indices -- base 0 */
	PHydData hydResults;					/* Hydraulics results */
	PQualData qualResults;					/* Water quality results */
} NetInfo, *PNetInfo;

/* Simulation Structures:
   There are two types of indices defined here:
   Hydraulics and Quality Simulation Indices
      Numbered 0 to hydSimCount and qualSimCount, respectively, 
	  these index into the respective HydraulicsSim and QualitySim 
	  structures to access information about where data is stored,
	  and in the case of Quality simulation data, what are the associated
	  hydraulics simulation indices that go along with the quality simulation.
   Hydraulics and Quality File Indices
      Numbered 0 to hydFileCount and qualFileCount, respectively, these
	  index into *hydFileNames[] and *qualFileNames[] (see ERD structure, below)
	  to access the file names where the associated hydraulic and simulation
	  data are stored.
*/
/* Hydraulics Simulation Index Data */
typedef struct
{
	int fileIdx;							/* Hydraulics File Index */
	int written;							/* Flag indicating whether network hydraulics data were written */
	int length;								/* Length (bytes) of hydraulics data */
	__file_pos_t offset;					/* Data offset (bytes) in hydraulics file */
} HydraulicsSim, *PHydraulicsSim;

/* Quality Simulation Index Data */
typedef struct
{
	int fileIdx;						    /* Quality File Index */
	int written;							/* Flag indicating whether network quality data were written */
	int hydSimIdx;							/* Index of associated hydraulics simulation */
	__file_pos_t offset;					/* Data offset (bytes) in water quality data file */
	unsigned int length;								/* Length (bytes) of water quality data */
	void *appData;							/* Data specific to the application that made the database */
} QualitySim, *PQualitySim;

/* Database */
typedef struct ERD
{
	char header[HEADER_SIZE];				/* Database header */
											/* byte     data              values
											 * 0        header flag       0xff
											 * 1        file version      ERD_VERSION
											 * 2        compression level See enum CompressionLevel
											 * 3        bytes for node/   64
											 *          link/species ID
											 * 4        Application flag  0 - TEVA
											 *                            1 - epanetDPX
											 *                            2 - undefined
											 * 5        Hydraulic Storage bit field
											 *                            00000 - no hydraulic data stored
											 *                            00001 - node demands
											 *                            00010 - link flows
											 *                            00100 - link velocities
											 *                            01000 - node pressure
											 *                            10000 - demand profiles
											 * 6-31     Unused
											 */
	FILE *hydStream;						/* Hydraulics file stream */
	FILE **qualStreams;						/* Water quality file stream */
	FILE *indexStream;						/* Index file stream */
	FILE *prologueStream;					/* Prologue file stream */
	enum DbStatus status;					/* Database status - created or existing */
	int hydFileOpened;                      /* Index of hydraulics data file opened for read/write */
	PQualitySim qualSimInMemory;			/* Pointer to quality simulation index in memory (during read) */
	PHydraulicsSim hydSimInMemory;			/* Pointer to hydraulic simulation index in memory (during read) */
	int qualFileCount;						/* Number of water quality data files */
	int hydFileCount;						/* Number of hydraulics data files */
	int qualSimCount;						/* Number of water quality results in database */
	int hydSimCount;						/* Number of hydraulics results in database */
	char *baseName;							/* Base name for all database files */
	char *directory;						/* Location of this database */
	char **hydFilenames;					/* Names of hydraulics data files */
	char **qualFilenames;					/* Names of water quality data files */
	PHydraulicsSim *hydSim;					/* Pointer to hydraulics simulation data */
	PQualitySim *qualSim;					/* Pointer to water quality simulation data */
	PNetInfo network;						/* Network data - only one network */
	PNodeInfo nodes;						/* Network node data */
	PLinkInfo links;						/* Network link data */
	int readFlags;
	int (*readFunction)(struct ERD *, PQualitySim, FILE *);	/* Index reader function */
	int (*writeFunction)(void *, FILE *);	/* Index writer function */
	void (*freeFunction)(void **);	/* free function */
} ERD, *PERD;

/* Application Specific Data Structures for Index File Read/Write */

typedef struct /* DPX */
{
	char *inputFile;						/* EPANET input filename */
	char *msxInputFile;						/* EPANET-MSX input filename */
} DPXIndexData, *PDPXIndexData;

/* redefining SourceData here as TEVAIndex data purely for semantic purposes.
 * Places where it is accessing the "IndexData" will use this, where every place
 * accessing the data as source data should use the other def - this should make
 * the intent in the source clear. */
typedef SourceData TEVAIndexData; /* TEVA */
typedef PSourceData PTEVAIndexData; /* TEVA */

// ERD_ReadProlog flags
#define READ_QUALITY        (0x00000001)
#define READ_DEMANDS        (0x00000002)
#define READ_LINKFLOW       (0x00000004)
#define READ_LINKVEL        (0x00000008)
#define READ_PRESSURE       (0x00000010)
#define READ_DEMANDPROFILES (0x00000020)
/* READ_IGNORE_ERROR is a special flag that should only be used when the user
 * will be ensuring that data that is not stored will be handled in user code.
 */
#define READ_IGNORE_ERROR   (0x80000000)
#define READ_ALL  (READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL | READ_PRESSURE | READ_DEMANDPROFILES)

/* API Function Prototypes (erd.c) */
/* Creating, opening, closing database and setting its characteristics */
LIBEXPORT(int) ERD_isERD(const char *erdName);
LIBEXPORT(int) ERD_create(PERD *database, const char *erdName, enum OutputFrom application, enum CompressionLevel compLevel);
LIBEXPORT(int) ERD_open(PERD *database, const char *directory, int flags);
LIBEXPORT(int) ERD_close(PERD *database);
LIBEXPORT(int) ERD_setHydStorage(PERD db, int velocity, int flow, int demand, int pressure, int profile);
LIBEXPORT(int) ERD_getVersion(PERD database);

/* Creating and writing database files */
LIBEXPORT(int) ERD_newHydFile(PERD database);
LIBEXPORT(int) ERD_newQualFile(PERD database);
LIBEXPORT(int) ERD_writeHydResults(PERD database);
LIBEXPORT(int) ERD_writeQualResults(PERD database, void *appdata);
LIBEXPORT(int) ERD_clearQualityData(PERD database);

/* Retrieving information from the database */
LIBEXPORT(int) ERD_getQualSimCount(PERD database);
LIBEXPORT(int) ERD_getHydSimCount(PERD database);
LIBEXPORT(int) ERD_getQualSimCountFor(int hydSimIdx, PERD database);
LIBEXPORT(PNetInfo) ERD_getNetworkData(PERD database);
LIBEXPORT(int) ERD_getResults(PQualitySim qualSim, PERD database);
LIBEXPORT(int) ERD_getRawSimulationResults(PQualitySim qualSimIdx, PERD database, PSource sources, char *buffer, int length);
LIBEXPORT(void*) ERD_getApplicationData(PERD database, int index);
LIBEXPORT(int) ERD_getERDcontrolLinkIndex(PERD database, int epanetIndex);
LIBEXPORT(char *) ERD_GetCompressionDesc(PERD db);
LIBEXPORT(int) ERD_getCompressionLevel(PERD db);
LIBEXPORT(int) ERD_getSpeciesIndex(PERD database, const char *speciesName);
LIBEXPORT(int) ERD_UTIL_positionFile(FILE *fp,__file_pos_t offs);
LIBEXPORT(__file_pos_t) ERD_UTIL_getFilePosition(FILE *fp);
LIBEXPORT(PQualData) ERD_UTIL_initQual(PNetInfo network, int flags);
LIBEXPORT(PHydData) ERD_UTIL_initHyd(PNetInfo network, int flags);

/* Error handling  - also see above macro definition ERDERROR() */
LIBEXPORT(void) ERD_Error(int errorCode);
LIBEXPORT(char*) ERD_getErrorMessage(int errorCode);

/* Creating application specific data containers for index read/write functions */
LIBEXPORT(PDPXIndexData) newDPXIndexData(char *inputFilename, char *msxInputFilename);
LIBEXPORT(PTEVAIndexData) newTEVAIndexData(int numSources, PSource source);

#endif
