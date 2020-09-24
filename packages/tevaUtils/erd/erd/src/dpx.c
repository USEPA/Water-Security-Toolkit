#include "erdinternal.h"

/*
 * Input: Database pointer
 * Output: Number of .INP files used in the series of simulations that created the database
 * Purpose: See Output
 */
/*
int LIBEXPORT ERD_getINPFileCount(PERD db)
{
	return db->qualSimCount;
}
*/
/*
 * Input: Database pointer
 * Output: Number of .MSX files used in the series of simulations that created the database
 * Purpose: See Output
 */
/*
int LIBEXPORT ERD_getMSXFileCount(PERD db)
{
	int i,
		count = 0;
	
	for(i = 0; i < db->qualSimCount; i++) {
		PDPXIndexData d = db->qualSim[i]->appData;
		if(d->msxInputFile != NULL)
			count++;
	}
	
	return count;
}
*/
/*
 * Input: Database pointer, results index
 * Output: Name of .INP file that was used for indexed result
 * Purpose: See Output
 */
/*
char LIBEXPORT *ERD_getINPFilename(PERD db, int index)
{
	char *value;
	PDPXIndexData d = db->qualSim[index]->appData;
	value = d->inputFile;
	return value;
}
*/
/*
 * Input: Database pointer, results index
 * Output: Name of .MSX file that was used for indexed result
 * Purpose: See Output
 */
/*
char LIBEXPORT *ERD_getMSXFilename(PERD db, int index)
{
	char *value;
	PDPXIndexData d = db->qualSim[index]->appData;
	return d->msxInputFile;
	return value;
}
*/

int readDPXIndexData(PERD db, PQualitySim data, FILE *stream)
{
	// We allocate memory for the DPX application data here
	// and return the pointer via the PQualitySim appData field

	int errorCode = 0,
		length = 0;
	PDPXIndexData d;
	db=db;  // to remove warning about unreferenced local parameter
	MEMCHECK(d = (PDPXIndexData)calloc(1, sizeof(DPXIndexData)), "d in readDPXIndexData");
	if(fread(&length, sizeof(int), 1, stream) != 1)
		errorCode = 726;
	MEMCHECK(d->inputFile = (char *)calloc(length, sizeof(char)), "d->inputFile in readDPXIndexData");
	if(fread(d->inputFile, sizeof(char), length, stream) != length)
		errorCode = 726;
	if(fread(&length, sizeof(int), 1, stream) != 1)
		errorCode = 726;
	MEMCHECK(d->msxInputFile = (char *)calloc(length, sizeof(char)), "d->msxInputFile in readDPXIndexData");
	if(fread(d->msxInputFile, sizeof(char), length, stream) != length)
		errorCode = 726;
	data->appData = d;
	
	return errorCode;
}

int writeDPXIndexData(void *data, FILE *stream)
{
	int errorCode = 0,
		length = 0;
	PDPXIndexData d = (PDPXIndexData)data;
	char *fn;

	// Epanet input file
	// Strip the directory path
	fn = strrchr(d->inputFile, FNTOK);
	if(fn == NULL)
		fn = d->inputFile;
	else
		fn++;
	length = (int)strlen(fn) + 1; 
	if(fwrite(&length, sizeof(int), 1, stream) != 1)
		errorCode = 726;
	if(fwrite(fn, sizeof(char), length, stream) != length)
		errorCode = 726;

	// Epanet MSX input file - may or may not be present
	if(!strlen(d->msxInputFile) == 0) {
		// Strip the directory path
		fn = strrchr(d->msxInputFile, FNTOK);
		if(fn == NULL)
			fn = d->msxInputFile;
		else
			fn++;
		length = (int)strlen(fn) + 1;
		if(fwrite(&length, sizeof(int), 1, stream) != 1)
			errorCode = 726;
		if(fwrite(fn, sizeof(char), length, stream) != length)
			errorCode = 726;
	}
	else {
		length = 0;
		if(fwrite(&length, sizeof(int), 1, stream) != 1)
			errorCode = 726;
	}

	return errorCode;
}

LIBEXPORT(PDPXIndexData) newDPXIndexData(char *inputFilename, char *msxInputFilename)
{
	PDPXIndexData dpx;
	
	MEMCHECK(dpx = (PDPXIndexData)calloc(1, sizeof(DPXIndexData)), "dpx in newDPXIndexData");
	dpx->inputFile = (char *)calloc(MAXFNAME, sizeof(char));
	dpx->msxInputFile = (char *)calloc(MAXFNAME, sizeof(char));
			
	strncpy(dpx->inputFile, inputFilename, MAXFNAME);
	if(!strncmp(msxInputFilename, "", MAXFNAME) || msxInputFilename != NULL)
		strncpy(dpx->msxInputFile, msxInputFilename, MAXFNAME);
	else
		dpx->msxInputFile = "";
		
	return dpx;
}
void freeDPXIndexData(void **data)
{
	PDPXIndexData dpx=(PDPXIndexData)*data;
	free(dpx->msxInputFile);
	free(dpx->inputFile);
	free(dpx);
	*data=NULL;
}