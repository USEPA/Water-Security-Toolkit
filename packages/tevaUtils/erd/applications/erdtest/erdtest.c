/*
erdtest
this program compares hydraulic and quality information in an ERD to EPANET/MSX solution

Note that if WDSSim is used to produce the ERD, then results are not expected to be exact
unless the report time step is equal to the MSX time step, cause WDSSim averages 
sub-report time step results.
*/

#include <stdio.h>
#include "erd.h"
#include "epanet2.h"
#include "epanetmsx.h"
#include "enl.h"
#include <math.h>

#define MIN(x,y) (((x)<=(y)) ? (x) : (y))		/* minimum of x and y */
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))		/* maximum of x and y */
#define MOD(x,y) ((x)%(y))						/* x modulo y */
#define ABS(x)   (((x)< 0 )  ? (-x): (x))       /* Absolute value */


static void MEMCHECK(void *x, char *msg);
int runHydraulics(char* inputFile, char* hydFile);
int runQuality(PERD db, char* inputFile, char* qualFile, char* hydFile, float ***cn, float ***cl);

int main (int argc, char * argv[])
{
	char *erdDirectory;

	PERD db;						// set pointer to Database structure
	PHydData hydResults;
	PQualData qualResults;
	char *message;
	int errorCode = 0;
	char inpFilename[MAXFNAME];
	char msxFilename[MAXFNAME];
	char hydFile[MAXFNAME];

	double avgE, sdevE, sumE, sumE2, minE, maxE, a;

	int i, j, h, s, t, n, simCount, numSpeciesStored, timeStepCount, numNodes, numLinks;
	float ***epanetNodeQualResults;
	float ***epanetLinkQualResults;

	printf("\n");					// initial line break for output
	if (argc != 2) {				// bad input checking
		fprintf(stdout,"Usage: \"%s erd_dir/ \"\n",argv[0]);
		return 1;
	}
	erdDirectory = argv[1];

	errorCode = ERD_open(&db, erdDirectory);			// open ERD database
	if(errorCode) {							            // check error opening
		message = ERD_getErrorMessage(errorCode);
		printf(":: Problem opening database. %s\n\n", message);
		return 1;
	}

	// allocate array for concentration data
	numSpeciesStored = db->network->numSpecies;
	numNodes = db->network->numNodes;
	numLinks = db->network->numLinks;
	timeStepCount = db->network->numSteps;
	// This is wasteful but...
	MEMCHECK( epanetNodeQualResults = (float ***)calloc(numSpeciesStored,sizeof(float **)), "epanetQualResults" );
	for(i = 0; i < numSpeciesStored; i++) {
		MEMCHECK( epanetNodeQualResults[i] = (float **)calloc(timeStepCount,sizeof(float *)), "epanetQualResults[]" );
		for(j = 0; j < timeStepCount; j++) {
			MEMCHECK( epanetNodeQualResults[i][j] = (float *)calloc(numNodes,sizeof(float)), "epanetQualResults[][]" );
		}
	}
	MEMCHECK( epanetLinkQualResults = (float ***)calloc(numSpeciesStored,sizeof(float **)), "epanetQualResults" );
	for(i = 0; i < numSpeciesStored; i++) {
		MEMCHECK( epanetLinkQualResults[i] = (float **)calloc(timeStepCount,sizeof(float *)), "epanetQualResults[]" );
		for(j = 0; j < timeStepCount; j++) {
			MEMCHECK( epanetLinkQualResults[i][j] = (float *)calloc(numLinks,sizeof(float)), "epanetQualResults[][]" );
		}
	}

	simCount = ERD_getQualSimCount(db);					// get total number of quality simulations in ERD
	printf(":: Database in directory: %s\n", erdDirectory);
	printf(":: Number of quality simulations: %d\n",simCount);

	for(h = 0; h < simCount; h++) { 

		// Get the hydraulic and quality input filenames that produced the sim results
		strcpy(inpFilename, ERD_getINPFilename(db, h));
		strcpy(msxFilename, ERD_getMSXFilename(db, h));

		printf(":::: result set %i Generated from %s and %s\n",h+1,inpFilename,msxFilename);

		// Get all the results for the quality simulation index, h
		printf(":::::: getting data from ERD\n");
		errorCode = ERD_getResults(h,db,&hydResults,&qualResults);
		if(errorCode) {						// check error
			message = ERD_getErrorMessage(errorCode);
			printf(":: Problem getting ERD results.  %d: %s\n\n", errorCode, message);
			return 1;
		}

		// run the hydraulic simulation using epanet / msx
		printf(":::::: getting data from epanet simulation\n");
		errorCode = runHydraulics(inpFilename, hydFile);
		if(errorCode) {						// check error
			printf(":: Problem getting Epanet hydraulic results. \n\n");
			return 1;
		}

		// run the quality simulation using epanet / msx
		errorCode = runQuality(db, inpFilename, msxFilename, hydFile, epanetNodeQualResults, epanetLinkQualResults);
		if(errorCode) {						// check error
			printf(":: Problem getting Epanet quality results. \n\n");
			return 1;
		}

		// Do the comparison and print error statistics
		// Form the errors and sums
		sumE = 0;
		sumE2 = 0;
		minE = FLT_MAX;
		maxE = 0;
		for(s=0; s<numSpeciesStored; s++) {
			if( db->network->species[s]->type == bulk ) 
			{
				for(t=0; t<timeStepCount; t++) {
					for(n=0; n<numNodes; n++) {
						a = epanetNodeQualResults[s][t][n] - qualResults->nodeC[s][n][t];
						sumE += a;
						sumE2 += a*a;
						if( ABS(a) < minE ) minE = a;
						if( ABS(a) > maxE ) maxE = a;
					}
				}
			}
			else
			{
				for(t=0; t<timeStepCount; t++) {
					for(n=0; n<numLinks; n++) {
						a = epanetLinkQualResults[s][t][n] - qualResults->linkC[s][n][t];
						sumE += a;
						sumE2 += a*a;
						if( ABS(a) < minE ) minE = a;
						if( ABS(a) > maxE ) maxE = a;
					}
				}
			}
		}
		avgE = sumE/(double)(numSpeciesStored*timeStepCount*numNodes);
		sdevE = sqrt(sumE2/(double)(numSpeciesStored*timeStepCount*numNodes) - avgE*avgE);
		printf(":::::::: Errors - max: %.6e, min: %.6e, mean: %.6e, stddev: %.6e\n",maxE,minE,avgE,sdevE);

	}

	// Close the database
	errorCode = ERD_close(&db);

	if(errorCode) { 
		message = ERD_getErrorMessage(errorCode); 
		printf(":: Problem closing database. %s\n\n", message); 
	} 

	return 0;
}

/*
 * Function: int runHydraulics(PERD db, char* inputFile, char* hydFile)
 * Input: Database pointer, EPANET input file name
 * Output: char* hydFile = the binary EPANET hydraulic solution filename
 * Returns: If an error occurs, a non-zero error code value
 * Purpose: Run hydraulic simulatio
 */

int runHydraulics(char* inputFile, char* hydFile) {

	char	outFilename[MAXFNAME];

	strcpy(outFilename,tmpnam(NULL));
	strcpy(hydFile,tmpnam(NULL));

	ENCHECK(ENopen(inputFile, outFilename, ""));	
	ENCHECK(ENsolveH());
	ENCHECK(ENsavehydfile(hydFile));
	ENCHECK(ENclose());

	return 0;

} // end function runHydraulics


/*
 * Function: int runQuality(PERD db, char* inputFile, char* qualFile, char* hydFile, float ***c)
 * Input: Database pointer, hydraulic input file name, quality input file name (msx or EPANET), hydraulic solution filename
 * Output: results saved to concentration array c
 * Returns: If an error occurs, a non-zero error code value
 * Purpose: Run quality simulation
 */

int runQuality(PERD db, char* inputFile, char* qualFile, char* hydFile, float ***cn, float ***cl) {

	long 	time,
		reportStep,
		timeStep,
		timeLeft;
		
	int 	type,
		errorCode = 0, 
		numSpeciesStored,
		numNodes,
		numLinks,
		timeStepCount,
		msx = 0,
		s, t, n;
		
	char 	outFilename[MAXFNAME], units[16];

	double c1, aTol, rTol;
	float c2;

	PNetInfo net = db->network;
	numSpeciesStored = net->numSpecies;
	numNodes = net->numNodes;
	numLinks = net->numLinks;
	timeStepCount = net->numSteps;
	reportStep = net->reportStep;

	// test if we are in EPANET or MSX mode...

	strcpy(outFilename,tmpnam(NULL));
	ENCHECK(ENopen(inputFile, outFilename, ""));

	if(net->qualCode == MULTISPECIES) {
		MSXCHECK(MSXopen(qualFile));
		MSXCHECK(MSXusehydfile(hydFile));
		MSXCHECK(MSXinit(0));
		time = 0;
		t = 0;
		// Save the initial concentration data to c[s][0][n]
		for(s=0; s<numSpeciesStored; s++) {
			MSXCHECK( MSXgetspecies(s+1, &type, units, &aTol, &rTol) );
			if( type == MSX_BULK ) {
				for(n=0; n<numNodes; n++) {
					MSXCHECK( MSXgetqual(MSX_NODE, n+1, s+1, &c1) );
					cn[s][t][n] = (float)c1;
				}
			}
			else
			{
				for(n=0; n<numLinks; n++) {
					MSXCHECK( MSXgetqual(MSX_LINK, n+1, s+1, &c1) );
					cl[s][t][n] = (float)c1;
				}
			}
		}
		do {
			MSXCHECK(MSXstep(&time, &timeLeft));
			if(MOD(time, net->reportStep) == 0) 
			{
				t++;
				// Save the concentration data to c[s][t][n]
				for(s=0; s<numSpeciesStored; s++) 
				{
					MSXCHECK( MSXgetspecies(s+1, &type, units, &aTol, &rTol) );
					if( type == MSX_BULK ) 
					{
						for(n=0; n<numNodes; n++) {
							MSXCHECK( MSXgetqual(MSX_NODE, n+1, s+1, &c1) );
							cn[s][t][n] = (float)c1;
						}
					}
					else
					{
						for(n=0; n<numLinks; n++) {
							MSXCHECK( MSXgetqual(MSX_LINK, n+1, s+1, &c1) );
							cl[s][t][n] = (float)c1;
						}
					}
				}
			}
		}
		while(timeLeft > 0);

		MSXCHECK(MSXclose());
	}
	else {
		ENCHECK(ENopenQ());
		ENCHECK(ENusehydfile(hydFile));
		ENCHECK(ENinitQ(1));
		t = 0;
		do {
			ENCHECK(ENrunQ(&time));
			if(MOD(time, net->reportStep) == 0) 
			{
				// Save the concentration data to c[s][t][n]
				for(n=0; n<numNodes; n++) {
					ENCHECK( ENgetnodevalue( n+1, EN_QUALITY, &c2 ) );
					cn[0][t][n] = c2;
				}
				t++;
			}

			ENCHECK(ENnextQ(&timeStep));
		}
		while(timeStep > 0);
		ENCHECK(ENcloseQ());
	}

	ENCHECK( ENclose() );
	return 0;
} // end function runQuality




static void MEMCHECK(void *x, char *msg)
{
	if(x == NULL) {
		fprintf(stderr, "Allocating memory: %s", msg);
		exit(1);
	}
}
