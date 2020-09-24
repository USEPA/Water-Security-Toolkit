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
#ifndef _TA_INPUT_H_
#define _TA_INPUT_H_

#include "tevautil.h"

/* Global Constants */
#define   SEPSTR    " \t\n\r"  /* Token separator characters */

typedef struct
{
	char *line;
	char *keyword;
	int idx;
	int ntokens;
	char **tokens;
	int used;
}InputLine, *PInputLine;

typedef struct
{
	int num;
	char *srcFileName;
	PInputLine inputLines;
} InputFileData, *PInputFileData;


PInputFileData loadInputFile(const char *inp, char **validKeywords);
LIBEXPORT(void) freeInputFileData(PInputFileData *ifd);
LIBEXPORT(PTSO) OpenTSOFile(PInputFileData taiIFD,char *tsoKeyword);
LIBEXPORT(PERD) OpenERDFile(PInputFileData taiIFD,char *erdKeyword, int flags);

#endif
