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
#include <stdlib.h>
#include <string.h>

#include "ta_utils.h"
#include "ta_input.h"

/*
**-----------------------------------------------------------
**  loadInputFile
**  Input:   filename containing data to load and valid
**           keywords for the file
**  Output:  Pointer to input file data contents (for later
**           processing)
**  Purpose: Loads data file contents into memory for later
**           processing
**-----------------------------------------------------------
*/
PInputFileData loadInputFile(const char *fn, char **validKeywords)
{
    char  
        line[MAXLINE+1];        /* Line from input data file */
    int  ntokens = 0;            /* # tokens in input line */
    char *tok[MAXTOKS];         /* Array of token strings */
	FILE *inp;
	PInputFileData ifd;

	if((inp=fopen(fn,"r"))==NULL) {
		ta_error(1,"Unable to open input file: %s\n",fn);
	}
	ifd = (PInputFileData)calloc(1,sizeof(InputFileData));
	ifd->srcFileName = (char *)calloc(strlen(fn)+1,sizeof(char));
        strcpy(ifd->srcFileName, fn);

    while (fgets(line,MAXLINE,inp) != NULL)
    {
		char **tokens;
		char *wline;
		int crlf;
		
		PInputLine iline;
		int i;
        /* Make copy of line and scan for tokens */
		crlf = strcspn(line,"\n\r");
		line[crlf]='\0';
		wline = (char *)calloc(strlen(line)+1,sizeof(char));
        strcpy(wline,line);
        ntokens = TAgettokens(wline,tok);

        /* Skip blank lines and comments */
        if (ntokens == 0 || *tok[0] == ';') {
			free(wline);
			continue;
		}

        /* Check if max. length exceeded */
        if (strlen(line) >= MAXLINE) {ta_error(1,"Max input line length exceeded");}

		ifd->num++;
		ifd->inputLines = (InputLine*)realloc(ifd->inputLines,ifd->num*sizeof(InputLine));
		iline = &ifd->inputLines[ifd->num-1];
		iline->line = wline;
		tokens = (char **)calloc(ntokens,sizeof(char *));
		iline->ntokens = ntokens;
		iline->tokens = tokens;
		for(i=0;i<ntokens;i++) tokens[i] = tok[i];
		iline->keyword = tokens[0];
		iline->idx = TAfindmatch(iline->keyword,validKeywords);
        if(iline->idx == -1) {
			ta_error(1,"Unknown keyword: %s\n",iline->keyword);
		}

    }   /* End of while */
	fclose(inp);
	return ifd;
}

/*
**-----------------------------------------------------------
**  freeInputFileData
**  Input:   Pointer to input file data contents 
**  Output:  none
**  Purpose: releases all memory previously allocated to hold
**           input file contents
**-----------------------------------------------------------
*/
LIBEXPORT(void) freeInputFileData(PInputFileData *ifd) {
	int i;

	for(i=0;i<(*ifd)->num;i++) {
		PInputLine il = &(*ifd)->inputLines[i];
		free(il->tokens);
		free(il->line);
	}
	free((*ifd)->inputLines);
	free(*ifd);
	*ifd=NULL;

}
/*
**--------------------------------------------------------------
**  TAGETTOKENS
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
int  TAgettokens(char *s, char **tok)
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
        m = strcspn(s,SEPSTR);                  /* Find token length */
        if (m == 0) s++;                                /* No token found */
        else
        {
            if (*s == '"')               /* Token begins with quote */
            {
                s++;                                            /* Start token after quote */
                m = strcspn(s,"\"\n\r");    /* Find end quote (or EOL) */
            }
            s[m] = '\0';                                 /* Null-terminate the token */
            tok[n] = s;                                  /* Save pointer to token */
            n++;                                                 /* Update token count */
            s += m+1;                                    /* Begin next token */
        }
        len -= m+1;                                         /* Update length of s */
    }
    return(n);
}


/*
**-----------------------------------------------------------
**  OpenTSOFile
**  Input:   Pointer to input file data contents containing
**           TSO file information and the keyword that
**           contains the TSO file information
**  Output:  Pointer to the TSO data structure from the TSO API
**  Purpose: Opens the tso database specified in the input
**           file data
**-----------------------------------------------------------
*/
LIBEXPORT(PTSO) OpenTSOFile(PInputFileData ifd,char *tsoKeyword)
{
    char  
	    tsoname[MAXFNAME+1],
		tsopattern[MAXFNAME+1],
		*patt=0;
	char tsoversion=-1,
		tsostoragemethod=-1;
	PTSO tso=0;
	int i;

    /* Get TSO file name */
	for(i=0;i<ifd->num;i++)
    {
		PInputLine il = &ifd->inputLines[i];
        /* Process line depending on input keyword */
		int ntokens = il->ntokens;
		char **tok=il->tokens;
		if(strcasecmp(tsoKeyword,il->keyword)==0) {
			// ntokens = 2: single filename w/o verson & storage method
			// ntokens = 3: multi-part file w/o verson & storage method
			// ntokens = 4: single filename w verson & storage method
			// ntokens = 5: multi-part file w verson & storage method
			if(ntokens < 2 || ntokens > 5)
				ta_error(1,"Invalid intput TSONAME\n");
			if(ntokens == 2 || ntokens == 4) {
				strcpy(tsoname,tok[1]);
				patt = NULL;
			} else {
				strcpy(tsoname,tok[1]);
				strcpy(tsopattern,tok[2]);
				patt = tsopattern;
			}
			if(ntokens ==4) {
				tsoversion = (char)atoi(tok[2]);
				tsostoragemethod = (char)atoi(tok[3]);
			} else if (ntokens==5) {
				tsoversion = (char)atoi(tok[3]);
				tsostoragemethod = (char)atoi(tok[4]);
			}
		}
    }   /* End of while */
	if(patt == NULL && !TSO_isTSO(tsoname))
		return NULL;
	if(tsoversion == -1 && tsostoragemethod == -1) {
		tso = TSO_Open(tsoname,patt);
	} else if (tsoversion != -1 && tsostoragemethod != -1) {
		tso = TSO_OpenNoHeader(tsoname,patt,tsoversion,tsostoragemethod);
	} else {
		ta_error(1,"Either both version & storage method must be specified or neither");
	}
	return tso;
}
/*
**-----------------------------------------------------------
**  OpenERDFile
**  Input:   Pointer to input file data contents containing
**           ERD file information and the keyword that
**           contains the ERD file information
**  Output:  Pointer to the ERD data structure from the TSO API
**  Purpose: Opens the erd database specified in the input
**           file data
**-----------------------------------------------------------
*/
LIBEXPORT(PERD) OpenERDFile(PInputFileData ifd,char *erdKeyword, int flags)
{
    char  erdname[MAXFNAME+1];
	PERD erd=NULL;
	int i;

    /* Get ERD file name */
	for(i=0;i<ifd->num;i++)
    {
		PInputLine il = &ifd->inputLines[i];
        /* Process line depending on input keyword */
		int ntokens = il->ntokens;
		char **tok=il->tokens;
		if(strcasecmp(erdKeyword,il->keyword)==0) {
			// ntokens = 2: single filename w/o verson & storage method
			if(ntokens != 2)
				ta_error(1,"Invalid intput TSONAME\n");
			strcpy(erdname,tok[1]);
		}
    }   /* End of while */
	if(!ERD_isERD(erdname))
		return NULL;

	ERD_open(&erd,erdname,flags);
	return erd;
}

