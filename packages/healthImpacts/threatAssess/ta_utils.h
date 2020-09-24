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
#include "tevautil.h" // for LIBEXPORT def
#define UCHAR(x) (((x) >= 'a' && (x) <= 'z') ? ((x)&~32) : (x))

/* Function Prototypes */
LIBEXPORT(void) ta_error(int exitCode,const char *msg,...);

/* Function Prototypes */
int   TAgettokens(char*, char**);
int   TAfindmatch(char*, char**);
int   TAmatch(char*, char*);
int   TAgetfloat(char*, float*);
int   TAgetlong(char*, long*);
int   TAgetint(char*, int*);

/* not available on unix */
#ifdef __linux__

int _stricmp (char *s, char *t);

#elif ( defined(__WINDOWS__) || defined(WIN32)) && !defined(MINGW)
LIBEXPORT(int) strcasecmp (char *s, char *t);

#endif
