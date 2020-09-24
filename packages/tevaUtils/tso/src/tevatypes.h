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
/* TEVA/TSO constants and data types */
#ifdef __cplusplus
#include <cstdio>
#endif
#include <sys/types.h>

#ifndef _TEVATYPES_H_
#define _TEVATYPES_H_

#define MAXSOURCES  100       /* Maximum number of sources  */
#define MAXCHARID  64
// 11 adds sim start time
#define MIN_TSOFILEVERSION 8
#define TSOFILEVERSION   11
#define TSOSTORAGEMETHOD 3    /* default if not specified */

#if defined(WIN32)
typedef __int64 __file_pos_t ;
#else
#if defined(__linux__) || defined(CYGWIN)
typedef int64_t __file_pos_t;
#endif
#endif
/*
**-------------------------------------------------------------------
** Node type
**-------------------------------------------------------------------
*/
enum NodeType
{
	junction,
	tank,
};

/*
**-------------------------------------------------------------------
** Source data
**-------------------------------------------------------------------
*/
typedef struct
{
	char  SourceNodeID[64+1];
	int   SourceNodeIndex;
	int   SourceType;
	int   SpeciesIndex;
	long   SourceStart;
	long   SourceStop;
	float SourceStrength;
} Source, *PSource;

/*
**-------------------------------------------------------------------
** Source Index data
**-------------------------------------------------------------------
*/
typedef struct {
	int id;
	int fileIndex;
	__file_pos_t fileOffset;
	int dataLength;
	PTEVAIndexData sourceData;
//	int nsources;
//	PSource source;
} DataIndex, *PDataIndex;
/*
**-------------------------------------------------------------------
** Group header data
**-------------------------------------------------------------------
*/
typedef struct {
    char groupType;
    short count;
} GroupHeader, *PGroupHeader;

/*
**-------------------------------------------------------------------
** TSO file data
**-------------------------------------------------------------------
*/
typedef struct {
	char header[32];
	FILE *stream;
	FILE **streams;
	FILE *indexStream;
	FILE *headerStream;
	char *tsodir;
	char *baseName;
	int count;
	int numFiles;
	PDataIndex *index;
	char **names;
	int curFile;
	// 0 = all data - time x nodes
	// 1 = nodes x time - only nodes w/non-zero
	// 2 = nodes x time - only nodes w/non-zero, grouped
	int storageMethod;
	// 1 = orignal version
	// 2 = added source start/stop/strength to each simulation output
	// 3 = added file header
	int fileVersion;
	float *linkdata;
	int nBytesForNodeID;
	fpos_t pos_cdata;
	float *nodedata;
	// whether or not to fill strings in binary file with spaces
	// or leave them as null characters
	int spaceFillStrings;
} TSO, *PTSO;

/*
**-------------------------------------------------------------------
** Network data
**-------------------------------------------------------------------
*/
typedef struct
{
	int nnodes;         /* Total number of nodes (junctions + tanks) */
	int nlinks;         /* Number of links */
	int ntanks;         /* Number of tank nodes */
	int njunctions;     /* Number of junction nodes */
	int nsteps;         /* Number of reporting time steps */
	int nsources;       /* Number of water quality sources */
    int qualcode;       /* integer type of MS/Epanet quality simulation */
	int nspecies;       /* Number of water quality species */
    int nbulkspecies;   /* Number of bulk water quality species */
	char  **species;    /* Names of water quality species */
	float stepsize;     /* Time step size for water quality results (hours) */
    long simduration;    /* Total Epanet simulation duration (seconds) */
    long reportstart;    /* Start of Epanet water quality results reporting period (seconds) */
    long reportstep;     /* Epanet Water quality results time step size (seconds) */
    long simstart;   /* Simulation start time (in seconds from midnight) */
	float fltmax;       /* Max floating point number */
} NetInfo, *PNetInfo;

/*
**-------------------------------------------------------------------
** Node data
**-------------------------------------------------------------------
*/
typedef struct
{
	enum  NodeType ntype;
	float x;
	float y;
	float *demandProfile; /* Demand patterns applied to base demand for this node */
	int demandProfileLength;
	float *q;
	float **c;
	int *nz; // one for each species
	char  *id;
} NodeInfo, *PNodeInfo;

/*
**-------------------------------------------------------------------
** Link data
**-------------------------------------------------------------------
*/
typedef struct
{
	int  from;         /* From node index */
	int  to;           /* To node index */
    float length;      /* link length */
	float *velocity;   /* velocity over time */
	float *flow;       /* flow rate over time */
    char *id;          /* Link ID */
    unsigned int nv;            /* Number of vertices */
    float *vx;         /* Vertices: x coord */
    float *vy;         /* Vertices: y coord */
} LinkInfo, *PLinkInfo;

#endif
