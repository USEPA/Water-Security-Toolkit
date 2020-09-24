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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "tevautil.h"
#include "enutil.h"

/* Methods local to this file */
static int  ENTUgettokens(char*, char**);
static int  ENTUfindmatch(char*, char**);
static int  ENTUmatch(char*, char*);
static int  ENTUgetfloat(char *, float *);
static int  ENTUgetlong(char *, long *);
static int  ENTUgetint(char *, int *);
static void enutilerror(int exitCode,const char *errmsg, ...);
static void ENTUExpandGen(int, PNodeInfo, int, int, float, long, long, int, int, char**, PSource, FILE*);

static void MEMCHECK(void *x,char *msg) {
	if(x == NULL)
		enutilerror(1,"Allocating memory: %s",msg);
}

void DLLEXPORT ENTU_GetLinkHData(PLinkInfo links, PNetInfo net, int time, long tstep)
/*
**-----------------------------------------------------------
**  Input:   TSO network and link data structure, current
**           timestep
**  Output:  None
**  Purpose: Accumulates time-averaged TSO hydraulic data in link structure
**           corresponding to zero-based time index, time.
**-----------------------------------------------------------
*/
{
    int i;
    float w,lv;

    w = (float)tstep/(float)net->reportstep; /* weight = fraction of report time step */
    for (i = 1; i <= net->nlinks; i++) {
        ENCHECK( ENgetlinkvalue(i, EN_FLOW, &lv) );
        links[i-1].flow[time] += w*lv;
        ENCHECK( ENgetlinkvalue(i, EN_VELOCITY, &lv) );
        links[i-1].velocity[time] += w*lv;
    }
}


void  DLLEXPORT ENTU_GetNodeHData(PNodeInfo nodes, PNetInfo net, int time, long tstep)
/*
**-----------------------------------------------------------
**  Input:   TSO network and node structures, time step
**  Output:  None
**  Purpose: Stores TSO hydraulic data in node structure
**           corresponding to zero-based time index, time.
**-----------------------------------------------------------
*/
{
    int i;
    float w,nv;

    w = (float)tstep/(float)net->reportstep; /* weight = fraction of report time step */
    for (i = 1; i <= net->nnodes; i++) {
        ENCHECK( ENgetnodevalue(i, EN_DEMAND, &nv) );
        nodes[i-1].q[time] += w*nv;
    }
}


void  DLLEXPORT ENTU_GetNodeQData(PNodeInfo nodes, PNetInfo net, int time, long tstep)
/*
**-----------------------------------------------------------
**  Input:   TSO network and node structures, time step
**  Output:  None
**  Purpose: Stores TSO quality data in node structure
**           corresponding to zero-based time index, time.
**-----------------------------------------------------------
*/
{

#ifdef MSEPANET
    int i,is;
    float w,nv;
    w = (float)tstep/(float)net->reportstep; /* weight = fraction of report time step */
    if(net->qualcode==EN_MULTIS) {
        for (i = 1; i <= net->nnodes; i++) {
            for (is = 1; is <= net->nbulkspecies; is++) {
                ENCHECK( ENMSgetnodequality(i, is, EN_QUALITY, &nv) );
                nodes[i-1].c[is-1][time] += w*nv;
            }
        }
    } else {
        for (i = 1; i <= net->nnodes; i++) {
            ENCHECK( ENgetnodevalue(i, EN_QUALITY, &nv) );
            nodes[i-1].c[0][time] += w*nv;
        }
    }
#else
    int i;
    float w,nv;
    w = (float)tstep/(float)net->reportstep; /* weight = fraction of report time step */
    for (i = 1; i <= net->nnodes; i++) {
        ENCHECK( ENgetnodevalue(i, EN_QUALITY, &nv) );
        nodes[i-1].c[0][time] += w*nv;
    }
#endif
}

void  DLLEXPORT ENTU_GetLinkQData(PLinkInfo links, PNetInfo net, int time, long tstep)
/*
**-----------------------------------------------------------
**  Input:   TSO network and link structures, time step
**  Output:  None
**  Purpose: Stores TSO quality data in link structure
**           corresponding to zero-based time index, time.
**-----------------------------------------------------------
*/
{

/* If we stored link data for pipe species it would look something like ... */
//
//#ifdef MSEPANET
//    int i,is;
//    if( net->qualcode==EN_MULTIS ) {
//        for (i = 1; i <= net->nlinks; i++) {
//            for (is = net->nbulkspecies+1; is <= net->nspecies; is++) {
//                ENCHECK( ENMSgetlinkquality(i, is, &links[i-1].c[is-net->nbulkspecies-1][time]) );
//            }
//        }
//    } else {
//        for (i = 1; i <= net->nlinks; i++) {
//            ENCHECK( ENgetlinkvalue(i, EN_QUALITY, &links[i-1].c[0][time])
//        }
//    }
//#endif
}

void  DLLEXPORT ENTU_ClearNodeQData(PNodeInfo nodes, PNetInfo net)
/*
**-----------------------------------------------------------
**  Input:   TSO network and node structures
**  Output:  None
**  Purpose: Clears concentration data.  This is called before
**           each new source, since data is now being
**           accumulated rather than replaced
**-----------------------------------------------------------
*/
{

	int in,is,it;
	for (in = 0; in < net->nnodes; in++) {
		for(it=0;it<net->nsteps;it++) {
			for (is=0; is < net->nsources; is++) {
				nodes[in].c[is][it]=0;
			}
		}
	}
}

void  DLLEXPORT ENTU_GetNodeXY(PNodeInfo nodes, PLinkInfo links, PNetInfo net, char *epanetfname)
/*
**--------------------------------------------------------------
**  Input:   nodes  =  pointer to NodeInfo structure
**           links  =  pointer to LinkInfo structure
**           net =  pointer to NetInfo structure
**           *epanetfname = pointer to epanet input file name
**  Output:  returns nodes and links structures filled with coordinates
**           and vertices stored in epanet inp file.
**           if no coordinate found in [COORD] section of data
**           file then associated xnode,ynode are unchanged.
**           if no vertices found in [VERTICES] section of data
**           file then associated nv, vx, vy are unchanged.
**  Purpose: reads Epanet data file to get node coordinate info
**           (no toolkit function for this task).  Modeled
**           closely after readdata() Epanet function.
**--------------------------------------------------------------
*/
{
    char  line[MAXLINE+1],     /* Line from input data file       */
          wline[MAXLINE+1];    /* Working copy of input line      */
    int   sect,                /* Data sections                   */
          ntokens = 0,         /* # tokens in input line          */
          nodeindex,           /* node index                      */
          linkindex,           /* link index                      */
          nv;
    FILE *EPANETIN;            /* File pointer to epanet input    */
    char *tok[MAXTOKS];        /* Array of token strings          */
    float x,y;                 /* coordinates                     */

#ifdef MSEPANET
    char *secttxt[]         
     = {"[TITL", "[JUNC", "[RESE",
        "[TANK", "[PIPE", "[PUMP",
        "[VALV", "[CONT", "[RULE",
        "[DEMA", "[SOUR", "[EMIT",
        "[PATT", "[CURV", "[QUAL",
        "[STAT", "[ROUG", "[ENER",
        "[REAC", "[MIXI", "[REPO",
        "[TIME", "[OPTI", "[COOR",
        "[VERT", "[LABE", "[BACK",
        "[TAGS", "[END",  "[MSVA", 
        "[MSPI", "[MSTA", "[MSOP",
        NULL};

#else
    char *secttxt[]         
     = {"[TITL", "[JUNC", "[RESE",
        "[TANK", "[PIPE", "[PUMP",
        "[VALV", "[CONT", "[RULE",
        "[DEMA", "[SOUR", "[EMIT",
        "[PATT", "[CURV", "[QUAL",
        "[STAT", "[ROUG", "[ENER",
        "[REAC", "[MIXI", "[REPO",
        "[TIME", "[OPTI", "[COOR",
        "[VERT", "[LABE", "[BACK",
        "[TAGS", "[END",  NULL};
#endif

    /* Open epanet input file */
    EPANETIN = fopen( epanetfname, "rt" );
    if (EPANETIN == NULL) enutilerror(1,"Opening Epanet input file");

    sect = -1;
    /* Read each line from input file. */
    while (fgets(line,MAXLINE,EPANETIN) != NULL)
    {

        /* Make copy of line and scan for tokens */
        strcpy(wline,line);
        ntokens = ENTUgettokens(wline,tok);

        /* Skip blank lines and comments */
        if (ntokens == 0) continue;
        if (*tok[0] == ';') continue;

        /* Check if max. length exceeded */
        if (strlen(line) >= MAXLINE) enutilerror(1,"Epanet input file error.");

        /* Check if at start of a new input section */
        if (*tok[0] == '[')
        {
            sect = ENTUfindmatch(tok[0],secttxt);
            if (sect >= 0)
            {
                if (sect == _END) break;
                continue;
            }
            else enutilerror(1,"Epanet input file error");
        }

        /* Otherwise process input if in Coords. section */
        else if (sect == _COORDS)
        {
            if ( ntokens != 3 ) enutilerror(1,"Epanet input file error");
            ENCHECK( ENgetnodeindex(tok[0],&nodeindex) );
            if ( nodeindex > net->nnodes ) enutilerror(1,"Epanet input file error");
            if ( ENTUgetfloat(tok[1],&x) ) {
                nodes[nodeindex-1].x = x;
            }
            else enutilerror(1,"Epanet input file error");
            if ( ENTUgetfloat(tok[2],&y) ) {
                nodes[nodeindex-1].y = y;
            }
            else enutilerror(1,"Epanet input file error");
        }

        /* Otherwise process input if in Vertices section */
        else if (sect == _VERTICES)
        {
            if ( ntokens != 3 ) enutilerror(1,"Epanet input file error");
            ENCHECK( ENgetlinkindex(tok[0],&linkindex) );
            if ( linkindex > net->nlinks ) enutilerror(1,"Epanet input file error");
            nv=++links[linkindex-1].nv;
            MEMCHECK(links[linkindex-1].vx=(float *)realloc(links[linkindex-1].vx,nv*sizeof(float)),"links.vx in ENTU_GetNodeXY");
            MEMCHECK(links[linkindex-1].vy=(float *)realloc(links[linkindex-1].vy,nv*sizeof(float)),"links.vy in ENTU_GETNodeXY");
            if ( ENTUgetfloat(tok[1],&x) ) {
                links[linkindex-1].vx[nv-1] = x;
            }
            else enutilerror(1,"Epanet input file error");
            if ( ENTUgetfloat(tok[2],&y) ) {
                links[linkindex-1].vy[nv-1] = y;
            }
            else enutilerror(1,"Epanet input file error");
        }

    }   /* End of while */
	fclose(EPANETIN);

}     /* End of GetNodeXY  */

void DLLEXPORT ENTU_EpanetError(int errnum)
/*
**-----------------------------------------------------------
**  Input:   errnum = epanet error number
**  Purpose: Epanet error handler
**-----------------------------------------------------------
*/
{
      char errmsg[MAXERRMSG];

    ENgeterror(errnum,errmsg,MAXERRMSG);
    fprintf(stderr,"EPAnet run-time error...\n");
    fprintf(stderr,"%s\n",errmsg);
    fprintf(stderr,"...now exiting to system...\n");
    exit(1);
}

static int  ENTUgettokens(char *s, char **tok)
/*
**--------------------------------------------------------------
**  Input:   *s = string to be tokenized
**  Output:  returns number of tokens in s
**  Purpose: scans string for tokens, saving pointers to them
**           in Tok[]
**
** Tokens can be separated by the characters listed in SEPSTR
** (spaces, tabs, newline, carriage return) which is defined
** in TYPES.H. Text between quotes is treated as a single token.
**--------------------------------------------------------------
*/
{
    int  len, m, n;
    char *c;

    /* Begin with no tokens */
    for (n=0; n<MAXTOKS; n++) tok[n] = NULL;
    n = 0;

    /* Truncate s at start of comment */
    c = strchr(s,';');
    if (c) *c = '\0';
    len = strlen(s);

    /* Scan s for tokens until nothing left */
    while (len > 0 && n < MAXTOKS)
    {
        m = strcspn(s,SEPSTR);         /* Find token length */
        len -= m+1;                    /* Update length of s */
        if (m == 0) s++;               /* No token found */
        else
        {
            if (*s == '"')               /* Token begins with quote */
            {
                s++;                       /* Start token after quote */
                m = strcspn(s,"\"\n\r");   /* Find end quote (or EOL) */
            }
            s[m] = '\0';                 /* Null-terminate the token */
            tok[n] = s;                  /* Save pointer to token */
            n++;                         /* Update token count */
            s += m+1;                    /* Begin next token */
        }
    }
    return(n);
}                        /* End of ENTUgettokens */



static int  ENTUfindmatch(char *line, char *keyword[])
/*
**--------------------------------------------------------------
**  Input:   *line      = line from input file
**           *keyword[] = list of NULL terminated keywords
**  Output:  returns index of matching keyword or
**           -1 if no match found
**  Purpose: determines which keyword appears on input line
**--------------------------------------------------------------
*/
{
    int i = 0;
    while (keyword[i] != NULL)
    {
        if (ENTUmatch(line,keyword[i])) return(i);
        i++;
    }
    return(-1);
}                        /* end of ENTUfindmatch */



static int  ENTUmatch(char *str, char *substr)
/*
**--------------------------------------------------------------
**  Input:   *str    = string being searched
**           *substr = substring being searched for
**  Output:  returns 1 if substr found in str, 0 if not
**  Purpose: sees if substr matches any part of str
**
**      (Not case sensitive)
**--------------------------------------------------------------
*/
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
}                        /* end of ENTUmatch */


static int  ENTUgetfloat(char *s, float *y)
/*
**-----------------------------------------------------------
**  Input:   *s = character string
**  Output:  *y = floating point number
**           returns 1 if conversion successful, 0 if not
**  Purpose: converts string to floating point number
**-----------------------------------------------------------
*/
{
    char *endptr;
    *y = (float) strtod(s,&endptr);
    if (*endptr > 0) return(0);
    return(1);
}

static int  ENTUgetint(char *s, int *y)
/*
**-----------------------------------------------------------
**  Input:   *s = character string
**  Output:  *y = int number
**           returns 1 if conversion successful, 0 if not
**  Purpose: converts string to long integer (base 10)
**-----------------------------------------------------------
*/
{
    char *endptr;
    *y = (int)strtol(s,&endptr,10);
    if (*endptr > 0) return(0);
    return(1);
}


static int  ENTUgetlong(char *s, long *y)
/*
**-----------------------------------------------------------
**  Input:   *s = character string
**  Output:  *y = long int number
**           returns 1 if conversion successful, 0 if not
**  Purpose: converts string to long integer (base 10)
**-----------------------------------------------------------
*/
{
    char *endptr;
    *y = strtol(s,&endptr,10);
    if (*endptr > 0) return(0);
    return(1);
}

/*
**--------------------------------------------------------------
**  enutilerror
**  Input:   exit code
**           errmsg: printf-like varargs for the error message
**           to print before exiting
**  Output:  none
**  Purpose: Print an error message and exit the program
**--------------------------------------------------------------
*/
static void enutilerror(int exitCode,const char *errmsg, ...)
{
	va_list ap;

	fprintf(stderr,"\n\n********************************************************************************\n\n");
	va_start(ap,errmsg);
	vfprintf(stderr,errmsg,ap);
	va_end(ap);
	fprintf(stderr,"\n\n********************************************************************************\n\n");

	exit(exitCode);
}

void  DLLEXPORT ENTU_WriteTSI(PNetInfo net, PNodeInfo nodes, PSource sources, FILE *simgen, FILE *simin)
/*
**--------------------------------------------------------------
**  Input:   simin = pointer to simulation input stream
**           simgen = pointer to simulation generator stream
**  Output:
**           Returns error code, or 0 if successful
**  Purpose: Processes simulation generator file to create
**           threat simulation input (TSI) file.
**--------------------------------------------------------------
*/
{
    char line[MAXLINE+1],     /* Line from input data file       */
        wline[MAXLINE+1];     /* Working copy of input line      */
    int nnodes=net->nnodes,   /* Number of nodes                 */
        sourcetype = 0,       /* source type                     */
        speciesindex,         /* source species index            */
        ntokens = 0,          /* # tokens in input line          */
        numsources;           /* number of sources               */
    float sourcestrength;     /* source strength                 */
    long starttime,           /* start time                      */
        stoptime;             /* stop time                       */
    char *tok[MAXTOKS];       /* Array of token strings          */

    /* Read a line from simulation generator input file. */
    while (fgets(line,MAXLINE,simgen) != NULL)
    {

        /* Make copy of line and scan for tokens */
        strcpy(wline,line);
        ntokens = ENTUgettokens(wline,tok);

        /* Skip blank lines and comments */
        if (ntokens == 0) continue;
        if (*tok[0] == ';') continue;

        /* Check if max. length exceeded */
        if (strlen(line) >= MAXLINE) enutilerror(1,"TSG file error");

        /* Otherwise process input line */
#ifdef MSEPANET
        numsources = ntokens - 5;                                                  /* number of sources         */
        if (numsources > MAXSOURCES || numsources < 1) enutilerror(1,"TSG file error");
        if ( ENTUmatch(tok[numsources],"CONCEN") ) {sourcetype=EN_CONCEN;}
        else if ( ENTUmatch(tok[numsources],"SETPOINT") ) {sourcetype=EN_SETPOINT;}
        else if ( ENTUmatch(tok[numsources],"FLOWPACED") ) {sourcetype=EN_FLOWPACED;}
        else if ( ENTUmatch(tok[numsources],"MASS") ) {sourcetype=EN_MASS;}
        else enutilerror(1,"TSG file error");
        if ( net->qualcode==EN_MULTIS ) {
            if ( ENMSgetspeciesindex(tok[numsources+1],&speciesindex) ) enutilerror(1,"TSG file error: bad species ID");  /* source species ID */
        } else {
            speciesindex=0;
        }
        if ( !ENTUgetfloat(tok[numsources+2],&sourcestrength) ) enutilerror(1,"TSG file error");/* source strength            */
        if ( !ENTUgetlong(tok[numsources+3],&starttime) )  enutilerror(1,"TSG file error");     /* source start time          */
        if ( !ENTUgetlong(tok[numsources+4],&stoptime) ) enutilerror(1,"TSG file error");       /* source stop time           */
#else
        numsources = ntokens - 4;                                                  /* number of sources         */
        if (numsources > MAXSOURCES || numsources < 1) enutilerror(1,"TSG file error");
        if ( ENTUmatch(tok[numsources],"CONCEN") ) {sourcetype=EN_CONCEN;}
        else if ( ENTUmatch(tok[numsources],"SETPOINT") ) {sourcetype=EN_SETPOINT;}
        else if ( ENTUmatch(tok[numsources],"FLOWPACED") ) {sourcetype=EN_FLOWPACED;}
        else if ( ENTUmatch(tok[numsources],"MASS") ) {sourcetype=EN_MASS;}
        else enutilerror(1,"TSG file error");
        speciesindex=0;
        if ( !ENTUgetfloat(tok[numsources+1],&sourcestrength) ) enutilerror(1,"TSG file error");/* source strength            */
        if ( !ENTUgetlong(tok[numsources+2],&starttime) )  enutilerror(1,"TSG file error");     /* source start time          */
        if ( !ENTUgetlong(tok[numsources+3],&stoptime) ) enutilerror(1,"TSG file error");       /* source stop time           */
#endif

        ENTUExpandGen(nnodes, nodes, sourcetype, speciesindex, sourcestrength, starttime, stoptime, numsources, 0, tok, sources, simin);

    }   /* End of while */

    rewind(simin);
}   /* End of WriteTSI  */

static void ENTUExpandGen(int nnodes, PNodeInfo nodes, int sourcetype, int speciesindex, float sourcestrength, long starttime, 
               long stoptime, int numsources, int is, char **nodetok, PSource sources, 
               FILE *simin)
/*
**--------------------------------------------------------------
**  Purpose: Recursive processing of TSG file source node tokens
**--------------------------------------------------------------
*/
{
    int i, itok, nodetype;
//    float demand;

	if (is < numsources-1) {  /* Working down the stack of node tokens */

        if ( ENTUmatch(nodetok[is],"ALL") ) {
            for (i=1; i<=nnodes; i++) {
                ENCHECK(ENgetnodeid(i,sources[is].SourceNodeID) );
                ENCHECK(ENgetnodetype(i,&nodetype) );
                if (nodetype == EN_JUNCTION) {
                    ENTUExpandGen(nnodes, nodes, sourcetype, speciesindex, sourcestrength, starttime, stoptime, numsources, is+1, nodetok, sources, simin);
                }
            }
        }
        else if ( ENTUmatch(nodetok[is],"NZD") ) {
            for (i=1; i<=nnodes; i++) {
				int j;
				int hasDemand=0;
                ENCHECK(ENgetnodeid(i,sources[is].SourceNodeID) );
                ENCHECK(ENgetnodetype(i,&nodetype) );
				for(j=0;!hasDemand&&j<nodes[i].demandProfileLength; j++) {
					if(nodes[i].demandProfile[j] != 0)
						hasDemand=1;
				}
                if (nodetype == EN_JUNCTION && hasDemand) {
                    ENTUExpandGen(nnodes, nodes, sourcetype, speciesindex, sourcestrength, starttime, stoptime, numsources, is+1, nodetok, sources, simin);
                }
            }
        }
        else {
            ENCHECK(ENgetnodeindex(nodetok[is],&i));
            strcpy(sources[is].SourceNodeID,nodetok[is]);
            ENTUExpandGen(nnodes, nodes, sourcetype, speciesindex, sourcestrength, starttime, stoptime, numsources, is+1, nodetok, sources, simin);
        }
    }

    else {  /* Last node token */

        if ( ENTUmatch(nodetok[is],"ALL") ) {
            for (i=1; i<=nnodes; i++) {
                ENCHECK(ENgetnodeid(i,sources[is].SourceNodeID) );
                ENCHECK(ENgetnodetype(i,&nodetype) );
                if (nodetype == EN_JUNCTION) {
                    for (itok=0; itok < numsources; itok++) {
                        FILEWRITE(fprintf(simin," %-15s %-15d %-15d %-15e %-15d %-15d",sources[itok].SourceNodeID,sourcetype,speciesindex,sourcestrength,starttime,stoptime),0);
                    }
                    fprintf(simin,"\n");
                }
            }
        }
        else if ( ENTUmatch(nodetok[is],"NZD") ) {
            for (i=1; i<=nnodes; i++) {
				int j;
				int hasDemand=0;
                ENCHECK(ENgetnodeid(i,sources[is].SourceNodeID) );
                ENCHECK(ENgetnodetype(i,&nodetype) );
				for(j=0;!hasDemand&&j<nodes[i-1].demandProfileLength; j++) {
					if(nodes[i-1].demandProfile[j] != 0)
						hasDemand=1;
				}
                if (nodetype == EN_JUNCTION && hasDemand) {
                    for (itok=0; itok < numsources; itok++) {
                        FILEWRITE(fprintf(simin," %-15s %-15d %-15d %-15e %-15d %-15d",sources[itok].SourceNodeID,sourcetype,speciesindex,sourcestrength,starttime,stoptime),0);
                    }
                    fprintf(simin,"\n");
                }
            }
        }
        else {
            ENCHECK(ENgetnodeindex(nodetok[is],&i));
            strcpy(sources[is].SourceNodeID,nodetok[is]);
            for (itok=0; itok < numsources; itok++) {
                FILEWRITE(fprintf(simin," %-15s %-15d %-15d %-15e %-15d %-15d",sources[itok].SourceNodeID,sourcetype,speciesindex,sourcestrength,starttime,stoptime),0);
            }
            fprintf(simin,"\n");
        }
    }
}  /* End of ENTUExpandGen */

int DLLEXPORT ENTU_SetSource(PSource sources, PNetInfo net, FILE *simin)
/*
**--------------------------------------------------------------
**  Input:   TSO Sources and Network structures, and TSI file ptr.
**  Output:  structure sources contains name of Epanet source
**           node ID, source start time (seconds), source stop
**           time (seconds), source strength, and species index
**           for all sources.  net->nsources
**           contains the number of sources.
**           Returns 1 if successful; 0 if not
**  Purpose: Reads one line of threat scenario input file and returns
**           source parameters
**--------------------------------------------------------------
*/
{
    int   i,is;
    char  line[MAXLINE+1],     /* Line from input data file       */
          wline[MAXLINE+1];    /* Working copy of input line      */
    int   ntokens = 0,         /* # tokens in input line          */
          tokspersource = 6,   /* # tokens to describe one source */
          nreadtoks;           /* # of read tokens                */
    char  *tok[MAXTOKS];       /* Array of token strings          */

    /* Read a line from input file. */
    while (fgets(line,MAXLINE,simin) != NULL)
    {

        /* Make copy of line and scan for tokens */
        strcpy(wline,line);
        ntokens = ENTUgettokens(wline,tok);

        /* Skip blank lines and comments */
        if (ntokens == 0) continue;
        if (*tok[0] == ';') continue;

        /* Check if max. length exceeded */
        if (strlen(line) >= MAXLINE) enutilerror(1,"TSI file error");

        /* Check for proper number of tokens */
        if ( MOD(ntokens,tokspersource) != 0 ) enutilerror(1,"TSI file error");

        /* Otherwise process input line */
        if ( (net->nsources = ntokens/tokspersource) > MAXSOURCES) enutilerror(1,"TSI file error");          /* Max # of quality sources   */
        for (is = 0; is < net->nsources; is++) {
            nreadtoks = is*tokspersource;
            if ( strlen(tok[nreadtoks]) > 64 ) enutilerror(1,"TSI file error: Node ID too long");
            ENCHECK( ENgetnodeindex(tok[nreadtoks],&sources[is].SourceNodeIndex) );                    /* Node index of source       */
            strcpy(sources[is].SourceNodeID,tok[nreadtoks]);                                           /* source node ID             */
            if ( !ENTUgetint(tok[nreadtoks+1],&sources[is].SourceType) ) enutilerror(1,"TSI file error"); /* Source type                */
            if ( sources[is].SourceType < EN_CONCEN || sources[is].SourceType > EN_FLOWPACED ) enutilerror(1,"TSI file error");
            if ( !ENTUgetint(tok[nreadtoks+2],&sources[is].SpeciesIndex) ) enutilerror(1,"TSI file error"); /* Species Index                */
            if ( sources[is].SpeciesIndex < 0 || sources[is].SpeciesIndex > net->nspecies ) enutilerror(1,"TSI file error");
            if ( !ENTUgetfloat(tok[nreadtoks+3],&sources[is].SourceStrength) ) enutilerror(1,"TSI file error");  /* source strength            */
            if ( !ENTUgetlong(tok[nreadtoks+4],&sources[is].SourceStart) ) enutilerror(1,"TSI file error");      /* start time                 */
            if ( sources[is].SourceStart < 0 || sources[is].SourceStart > net->simduration ) enutilerror(1,"TSI file error: SourceStart");
            if ( !ENTUgetlong(tok[nreadtoks+5],&sources[is].SourceStop) ) enutilerror(1,"TSI file error");       /* stop time                  */
            if ( sources[is].SourceStop < sources[is].SourceStart || sources[is].SourceStop > net->simduration ) enutilerror(1,"TSI file error: SourceStop");
        }
        for (i = 1; i <= net->nnodes; i++ ) { /* Override ALL *.inp source and WQ information */
#ifdef MSEPANET
            if( net->qualcode==EN_MULTIS ) {  /* Initial quality is retained for MS simulations */
                for (is = 1; is <= net->nbulkspecies; is++) {
                    ENCHECK( ENMSsetnodequality(i,is,EN_SOURCETYPE,EN_CONCEN) );  /* CONCEN type                */
                    ENCHECK( ENMSsetnodequality(i,is,EN_SOURCEQUAL,0.0) );        /* Zero all sources           */
                    ENCHECK( ENMSsetnodequality(i,is,EN_SOURCEPAT,0) );           /* No pattern                 */
                }
//                for (is = 1; is <= net->nspecies; is++) {
//                    ENCHECK( ENMSsetnodequality(i,is,EN_INITQUAL,0) );            /* Initial quality = 0        */
//                }
            } else {
                ENCHECK( ENsetnodevalue(i,EN_SOURCETYPE,EN_CONCEN) );  /* CONCEN type                */
                ENCHECK( ENsetnodevalue(i,EN_SOURCEQUAL,0.0) );        /* Zero all sources           */
                ENCHECK( ENsetnodevalue(i,EN_SOURCEPAT,0) );           /* No pattern                 */
                ENCHECK( ENsetnodevalue(i,EN_INITQUAL,0) );            /* Initial quality = 0        */
            }
#else
            ENCHECK( ENsetnodevalue(i,EN_SOURCETYPE,EN_CONCEN) );  /* CONCEN type                */
            ENCHECK( ENsetnodevalue(i,EN_SOURCEQUAL,0.0) );        /* Zero all sources           */
            ENCHECK( ENsetnodevalue(i,EN_SOURCEPAT,0) );           /* No pattern                 */
            ENCHECK( ENsetnodevalue(i,EN_INITQUAL,0) );            /* Initial quality = 0        */
#endif
    }
    return(1);
    }   /* End of while */
  return(0);
}     /* End of ENTU_SetSource  */


/*
**--------------------------------------------------------------
**  ENTU_Initialize
**  Input:   current epanet filename (char*), tso filename (char*),
**           Addresses of tso, net, node, and link structures 
**           (PTSO*, PNetInfo*, PNodeInfo*, PLinkInfo*)
**  Output:  Allocated TSO data structures for the currently open
**           epanet model.
**  Purpose: Allocate memory for new NetInfo, PipeInfo,
**           and LinkInfo data structures for a given network,
**           and sets NetInfo structure values.  Must be preceeded
**           by ENopen() call.
**  JGU 7/11/05
**--------------------------------------------------------------
*/
void DLLEXPORT ENTU_Initialize(char *epanetfname, char *tsofname, int storageMethod, int tsofileversion, PTSO *tso, PNetInfo *net, PNodeInfo *nodes, PLinkInfo *links, PSource *sources) {
	PNodeInfo tnodes;
	PLinkInfo tlinks;
    PNetInfo  tnet;
    int i,nspecies,nbulkspecies,fromnode,tonode,nodetype,qualcode,tracenode;
    float length;

    /* Allocate PNetInfo and assign structure values */
    ENCHECK( ENgetqualtype(&qualcode,&tracenode) );                     /* Water quality simulation type */
#ifdef MSEPANET
    if( qualcode == EN_MULTIS ) {
        ENCHECK( ENgetcount(EN_SPECIESCOUNT, &nspecies) );              /* # water quality species */
        ENCHECK( ENgetcount(EN_BULKSPECIESCOUNT, &nbulkspecies) );      /* # bulk water quality species */
    } else {
        nspecies=1;
        nbulkspecies=1;
    }
#else
    nspecies=1;
    nbulkspecies=1;
#endif
    tnet=*net = TSO_AllocateNetworkInfo(nspecies);                      /* allocates PNetInfo and initializes nspecies within */
    ENCHECK( ENgetcount(EN_NODECOUNT, &tnet->nnodes) );                 /* # Nodes */
    ENCHECK( ENgetcount(EN_LINKCOUNT, &tnet->nlinks) );                 /* # Links */
    ENCHECK( ENgetcount(EN_TANKCOUNT, &tnet->ntanks) );                 /* # Tanks */
    ENCHECK( ENgettimeparam(EN_REPORTSTEP, &tnet->reportstep) );        /* Report start time */
    ENCHECK( ENgettimeparam(EN_REPORTSTART, &tnet->reportstart) );      /* Report time step */
    ENCHECK( ENgettimeparam(EN_DURATION, &tnet->simduration) );         /* Sim duration */
    tnet->nsteps = 1 + (tnet->simduration - tnet->reportstart)/(tnet->reportstep);  /* # Report steps */
    tnet->njunctions = tnet->nnodes - tnet->ntanks;                     /* # Junctions */
	tnet->stepsize = (float)(tnet->reportstep)/60.0f/60.0f;             /* Report time step (hours) */
    tnet->fltmax = FLT_MAX;                                             /* Max float value (used to indicate missing data) */
    tnet->nbulkspecies = nbulkspecies;                                  /* # bulk water quality species */
    tnet->qualcode = qualcode;                                          /* quality type flag */
	for(i=0;i<tnet->nspecies;i++) {                                     /* Specie IDs */
        memset( tnet->species[i], '\0', 16 );
#ifdef MSEPANET
        if( qualcode==EN_MULTIS ) ENCHECK( ENMSgetspeciesid(i+1, tnet->species[i]) );
        else strcpy(tnet->species[i],"Unknown");
#else
        strcpy(tnet->species[i],"Unknown");
#endif
	}

    /* Allocate PNodeInfo and PLinkInfo and assign structure values */
    TSO_AllocateNetworkData(nodes,links,tnet,READ_ALL);
    tnodes=*nodes, tlinks=*links;
    /* Node data */
    for (i=0; i<tnet->nnodes; i++) {
        ENCHECK( ENgetnodetype(i+1,&nodetype) );
        if ( nodetype == EN_TANK || nodetype == EN_RESERVOIR ) {
            tnodes[i].ntype = tank;
        } else {
            tnodes[i].ntype = junction;
		}
		tnodes[i].x = tnet->fltmax;
		tnodes[i].y = tnet->fltmax;
        ENCHECK( ENgetnodeid(i+1,tnodes[i].id) );
    }
    /* Link data */
    for (i=0; i<tnet->nlinks; i++) {                                        
        ENCHECK( ENgetlinknodes(i+1,&fromnode,&tonode) );
        ENCHECK( ENgetlinkvalue(i+1,EN_LENGTH,&length) );
        ENCHECK( ENgetlinkid(i+1,tlinks[i].id) );
		tlinks[i].from=fromnode;
		tlinks[i].to=tonode;
        tlinks[i].length=length;
        tlinks[i].nv=0;
        tlinks[i].vx=NULL;
        tlinks[i].vy=NULL;
    }
    /* Process INP file to get node coordinates -- No toolkit function :( */
    ENTU_GetNodeXY(tnodes, tlinks, tnet, epanetfname);

    /* Allocate TSO */
	*tso = TSO_CreateSpecificVersion(tsofname,storageMethod,tsofileversion,tnet);

    /* Allocate Source data */
	*sources=(PSource)calloc(MAXSOURCES,sizeof(Source));
}

typedef struct PatternInfo {
	char *id;
	int numValues;
	float *mult;
} PatternInfo, *PatternInfoPtr;
typedef struct Patterns {
	int num;
	PatternInfoPtr *patterns;
} Patterns, *PatternsPtr;

int gcd(int a, int b)
{
	if(b==0) return a;
	return gcd(b,a % b);
}

int lcm(int a, int b)
{
	return (a*b)/gcd(a,b);
}
PatternInfoPtr x_getPattern(int pattID,PatternsPtr patterns)
{
	int id=pattID-1; // epanet indices are 1-based
	if(id+1>=patterns->num) {
		int i;
		// increase the patterns array...
		patterns->patterns=(PatternInfoPtr*)realloc(patterns->patterns,(id+1)*sizeof(PatternInfoPtr));
		for(i=patterns->num;i<id+1;i++) {
			patterns->patterns[i]=NULL;
		}
		patterns->num=id+1;
	}
	if(patterns->patterns[id] == NULL) {
		int plen,pi;
		PatternInfoPtr p=(PatternInfoPtr)calloc(1,sizeof(PatternInfo));
		patterns->patterns[id]=p;
//		printf("1\n");
		ENCHECK( ENgetpatternlen(pattID, &plen));
//		printf("2 %d\n",plen);
		p->numValues=plen;
		p->mult=(float*)calloc(plen,sizeof(float));
		for(pi=0;pi<plen;pi++) {
//			printf("3 %d %d\n",pattID,pi);
			ENCHECK( ENgetpatternvalue(pattID, pi+1, &p->mult[pi]));
//			printf("4 %f\n",p->mult[pi]);
		}
	}
	return patterns->patterns[id];
}
void DLLEXPORT ENTU_CreateDemandPatterns(PNetInfo net, PNodeInfo nodes)
{
	int n;
	PatternsPtr patterns=(PatternsPtr)calloc(1,sizeof(Patterns));
	for(n=0;n<net->nnodes;n++) {
		int enIdx=n+1,d;
		PatternInfoPtr *allPatts;
		float *demands;
		float *combinedDemands=NULL;
		PNodeInfo node=&nodes[n];
		int numDemands;
		int combinedLength=0;

		ENCHECK( ENgetnumdemands(enIdx,&numDemands));
		allPatts=(PatternInfoPtr *)calloc(numDemands,sizeof(PatternInfoPtr));
		demands=(float *)calloc(numDemands,sizeof(float));
		
		for(d=0;d<numDemands;d++) {
			int pattID;
			ENgetdemandpattern(enIdx,d,&pattID);
			allPatts[d]=x_getPattern(pattID,patterns);
			ENgetbasedemand(enIdx,d,&demands[d]);
			if(combinedLength==0) combinedLength=allPatts[d]->numValues;
			else combinedLength=lcm(combinedLength,allPatts[d]->numValues);
		}
		combinedDemands=(float*)calloc(combinedLength,sizeof(float));
		for(d=0;d<numDemands;d++) {
			float baseDemand=demands[d];
			PatternInfoPtr patt=allPatts[d];
			float*mult=patt->mult;
			int i=0;
			while(i<combinedLength) {
				int j;
				for(j=0; j<patt->numValues;j++,i++) {
					combinedDemands[i]+=baseDemand*mult[j];
				}
			}
		}
		node->demandProfile=combinedDemands;
		node->demandProfileLength=combinedLength;
		free(allPatts);
		free(demands);
	}
	for(n=0;n<patterns->num;n++) {
		if(patterns->patterns[n] != NULL) {
			free(patterns->patterns[n]->mult);
			free(patterns->patterns[n]);
		}
	}
	free(patterns->patterns);
	free(patterns);
}
