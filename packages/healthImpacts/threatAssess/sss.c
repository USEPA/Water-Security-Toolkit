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

#ifdef __linux__
#include <strings.h>
#endif

#include "nrutil.h"
#include "sss.h"
#include "teva-assess.h"
#include "ta_utils.h"
#include "ta_input.h"

enum SensorSetKeywordTypes
{
	_SAMPLELOC,     /* NodeID|ALL <sensor/sample location code: ignore|existing|potential */
};
char 
    *sensorSetKeywords[]   = {
    "SAMPLELOC",       /* sample location (node ID) */
    NULL
};
/*
**-----------------------------------------------------------
**  loadSSSFileData
**  Input:   sss (Sensor Solution Set) data file name
**  Output:  Pointer to input file data contents (for later
**           processing)
**  Purpose: Loads data file contents into memory for later
**           processing
**-----------------------------------------------------------
*/
LIBEXPORT(PInputFileData) loadSSSFileData(const char *sssFile)
{
	return loadInputFile(sssFile,sensorSetKeywords);
}
/*
**-----------------------------------------------------------
**  processSensorSetData
**  Input:   Pointer to input file data contents
**  Output:  Updated node/sensor infornmation based on data
**           file contents
**  Purpose: Processes sensor set data and updates sensor info
**-----------------------------------------------------------
*/
LIBEXPORT(void) processSensorSetData(PInputFileData sssIFD, PMem mem)
{
	int i;

	for(i=0;i<sssIFD->num;i++)
	{
		int sensorIDX;
		PInputLine il = &sssIFD->inputLines[i];
        /* Process line depending on input keyword */
        int sect = il->idx;
		int ntokens = il->ntokens;
		char **tok=il->tokens;
		PSensor sensor;

        switch( sect )
        {
        case _SAMPLELOC:
            if( ntokens != 9 ) ta_error(1,"Invalid input SAMPLELOC");

            if( (sensorIDX=getnodeindex(mem->net,mem->node,tok[1])) == -1 )
                ta_error(1,"Invalid SAMPLELOC sensor/sample node ID");
            sensor = &mem->node[sensorIDX].sensor;
            if(!strcasecmp(tok[2],"POTENTIAL"))
                sensor->type = potential;
            else if(!strcasecmp(tok[2],"EXISTING"))
                sensor->type = existing;
            else if(!strcasecmp(tok[2],"IGNORED"))
                sensor->type = ignore;
            else if(!strcasecmp(tok[2],"SELECTED"))
                sensor->type = selected;

            if(!strcasecmp(tok[3],"REALTIME"))
                sensor->sample = realtime;
            else if(!strcasecmp(tok[3],"COMPOSITE"))
                sensor->sample = composite;
            else if(!strcasecmp(tok[3],"FILTERED"))
                sensor->sample = filtered;
            else ta_error(1,"Invalid input SAMPLELOC (sub-field: sample type)");
            if ( !TAgetfloat(tok[4],&sensor->limit) ) ta_error(1,"Invalid input SAMPLELOC (sub-field: limit)");
            if ( !TAgetfloat(tok[5],&sensor->vol  ) ) ta_error(1,"Invalid input SAMPLELOC (sub-field: vol)");
            if ( !TAgetint  (tok[6],&sensor->freq ) ) ta_error(1,"Invalid input SAMPLELOC (sub-field: freq)");
            if ( !TAgetint  (tok[7],&sensor->start) ) ta_error(1,"Invalid input SAMPLELOC (sub-field: start)");
            if ( !TAgetint  (tok[8],&sensor->delay) ) ta_error(1,"Invalid input SAMPLELOC (sub-field: delay)");

            break;
		default:
            ta_error(1,"Invalid SSS keyword");
        }
    }   /* End of while */


}

