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
#include "ta_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>


/*
**-----------------------------------------------------------
**  ta_error
**  Input:   exit code and varargs (printf-like) format string
**  Output:  None
**  Purpose: display the error message and exit
**-----------------------------------------------------------
*/
void ta_error(int exitCode,const char *errmsg, ...)
{
	va_list ap;
	fprintf(stderr,"\n\n********************************************************************************\n\n");
	va_start(ap,errmsg);
	vfprintf(stderr,errmsg,ap);
	va_end(ap);
	fprintf(stderr,"\n\n********************************************************************************\n\n");
	if(exitCode>0)
		exit(exitCode);
}

/*
**-----------------------------------------------------------
**  TAGETFLOAT
**  Input:   *s = character string
**  Output:  *y = floating point number
**                   returns 1 if conversion successful, 0 if not
**  Purpose: converts string to floating point number
**-----------------------------------------------------------
*/
int  TAgetfloat(char *s, float *y)
{
    char *endptr;
    *y = (float) strtod(s,&endptr);
    if (*endptr > 0) return(0);
    return(1);
}


/*
**-----------------------------------------------------------
**  TAGETLONG
**  Input:   *s = character string
**  Output:  *y = long int number
**                   returns 1 if conversion successful, 0 if not
**  Purpose: converts string to long integer (base 10)
**-----------------------------------------------------------
*/
int  TAgetlong(char *s, long *y)
{
    char *endptr;
    *y = strtol(s,&endptr,10);
    if (*endptr > 0) return(0);
    return(1);
}


/*
**-----------------------------------------------------------
**  TAGETINT
**  Input:   *s = character string
**  Output:  *y = int number
**                   returns 1 if conversion successful, 0 if not
**  Purpose: converts string to integer (base 10)
**-----------------------------------------------------------
*/
int  TAgetint(char *s, int *y)
{
    char *endptr;
    *y = (int)strtol(s,&endptr,10);
    if (*endptr > 0) return(0);
    return(1);
}

/*
**--------------------------------------------------------------
**  TAFINDMATCH
**  Input:   *line= line from input file
**           *keyword[] = list of NULL terminated keywords
**  Output:  returns index of matching keyword or
**           -1 if no match found
**  Purpose: determines which keyword appears on input line
**--------------------------------------------------------------
*/
int  TAfindmatch(char *line, char *keyword[])
{
    int i = 0;
    while (keyword[i] != NULL)
    {
        if (TAmatch(line,keyword[i])) return(i);
        i++;
    }
    return(-1);
}


/*
**--------------------------------------------------------------
**  TAMATCH
**  Input:   *str = string being    searched
**           *substr = substring being searched for
**  Output:  returns 1 if substr found in str, 0 if not
**  Purpose: sees if substr matches any part of str
**
**          (Not case sensitive)
**--------------------------------------------------------------
*/
int  TAmatch(char *str, char *substr)
{
    int i,j;

    /*** Updated 9/7/00 ***/
    /* Fail if substring is empty */
    if (!substr[0]) return(0);

    /* Skip leading blanks of str. */
    for (i=0; str[i]; i++) {
        if (str[i] != ' ') break;
    }

    /* Check if substr matches remainder of str. */
    for (i=i,j=0; substr[j]; i++,j++) {
        if (!str[i] || UCHAR(str[i]) != UCHAR(substr[j]))
            return(0);
    }
    return(1);
}

/* not available on linux; case-invariant version of strcmp */

#if defined(__linux__) || (defined(CYGWIN) && !defined(MINGW))

int _stricmp (char *s, char *t) 
{
  int d = 0;
  do 
    {
      d = toupper(*s) - toupper(*t);
    } 
  while (*s++ && *t++ && !d);
  return (d);
}
#elif ( defined(__WINDOWS__) || defined(WIN32)) && !defined(MINGW)


LIBEXPORT(int) strcasecmp (char *s, char *t)
{
	return stricmp(s,t);
}

#endif

