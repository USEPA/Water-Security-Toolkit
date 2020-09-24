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
/* System Headers */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>

/* MS/Epanet header */
//#ifdef WIN32
//#ifdef MSEPANET  /* Multi-species or Normal Epanet header */
//#include "epanet2ms.h"
//#else
//#include "epanet2.h"
//#endif
//#endif

//#ifdef __linux__

#ifdef MSEPANET  /* Multi-species or Normal Epanet header */
#include "toolkitms.h"
#else
#include "toolkit.h"
#endif

//#endif

/* TEVAUtils headers */
#include "tevautil.h"
#include "enl.h"
//#include "enutil.h"

#define TSMAJORVERSION  1
#define TSMINORVERSION  2

/* Global Conversion macros */
#define TEVAFILEOPEN(x,f) if ((x) == NULL) { TEVASimError(1,"Error opening file: %s",(f));}
#define MEMCHECK(x) if ((x) == NULL) { TEVASimError(1,"Allocating memory"); }
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))     /* minimum of x and y    */
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))     /* maximum of x and y    */
#define MOD(x,y) ((x)%(y))                    /* x modulus y           */

/* Function Prototypes */
#ifdef __cplusplus
extern "C" {
#endif
  void run_flushsim(const char* tsgfname, const char* tsifname, const char* erdfname, const char* epanetinpfname, const char* epanetoutfname, const char* decfname, const char *msxfname, const char *msxspecies, bool isMERLION, int merlion_max_rhs, bool merlion_ignore_warnings, bool isRLE);
void TEVASimError(int exitCode,const char *errmsg, ...);
char  *TEVAclocktime(char*, long);

void writeTSI(PNetInfo net, PNodeInfo *nodes, PSource sources, FILE *simgen, FILE *simin);
int setSource(PSource sources, PNetInfo net, FILE *simin);
void expandGen(int nnodes, PNodeInfo *nodes, int sourcetype, int speciesindex, float sourcestrength, long starttime, 
               long stoptime, int numsources, int is, char **nodetok, PSource sources, 
               FILE *simin);

#ifdef __cplusplus
};
#endif

