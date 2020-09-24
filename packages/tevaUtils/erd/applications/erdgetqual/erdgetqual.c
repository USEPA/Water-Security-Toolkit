/*
erdgetqual
this program dumps the quality simulation data from an erd database to a binary file
in an uncompressed format.
*/

#include <stdio.h>
#include <string.h>
#include "erd.h"

static void MEMCHECK(void *x, char *msg);

int main (int argc, char * argv[])
{
	char *erdDirectory, *specieID, *erdinp, *erdmsx, *erdout;

	PERD db;						// set pointer to Database structure
	PHydData hydResults;
	PQualData qualResults;
	PSpeciesData *species;
	char *message;
	int errorCode = 0, found = 0, foundspecie = 0;
	char inpFilename[MAXFNAME];
	char msxFilename[MAXFNAME];
	FILE *wqout;

	int h, s, n, simCount, numSpeciesStored, timeStepCount, numNodes, numLinks;

	printf("\n");					// initial line break for output
	if (argc != 6) {				// bad input checking
		fprintf(stdout,"Usage: \"%s erd_dir/ specieID inpfile msxfile outfile\"\n",argv[0]);
		return 1;
	}
	erdDirectory = argv[1];
	specieID = argv[2];
	erdinp = argv[3];
	erdmsx = argv[4];
	erdout = argv[5];

	errorCode = ERD_open(&db, erdDirectory);			// open ERD database
	if(errorCode) {							            // check error opening
		message = ERD_getErrorMessage(errorCode);
		printf(":: Problem opening database. %s\n\n", message);
		return 1;
	}

	numSpeciesStored = db->network->numSpecies;
	numNodes = db->network->numNodes;
	numLinks = db->network->numLinks;
	timeStepCount = db->network->numSteps;

	simCount = ERD_getQualSimCount(db);					// get total number of quality simulations in ERD
	printf(":: Database in directory: %s\n", erdDirectory);
	printf(":: Number of quality simulations: %d\n",simCount);

	for(h = 0; h < simCount; h++) { 

		// Get the hydraulic and quality input filenames that produced the sim results
		strcpy(inpFilename, ERD_getINPFilename(db, h));
		strcpy(msxFilename, ERD_getMSXFilename(db, h));

		if( !found & !strcmp(inpFilename,erdinp) & !strcmp(msxFilename,erdmsx) ) 
		{
			found = 1;
			printf(":::: found result set %i Generated from %s and %s\n",h+1,inpFilename,msxFilename);

			// Get all the results for the quality simulation index, h
			printf(":::::: getting data from ERD\n");
			errorCode = ERD_getResults(h,db,&hydResults,&qualResults);
			if(errorCode) {						// check error
				message = ERD_getErrorMessage(errorCode);
				printf(":: Problem getting ERD results.  %d: %s\n\n", errorCode, message);
				return 1;
			}

			// print results to the binary output file
			wqout = fopen(erdout,"wb");
			if(wqout == NULL) {
				printf(":: Problem opening output file %s\n\n",erdout);
				return 1;
			}

			species = db->network->species;
			for(s=0; s<db->network->numSpecies; s++) 
			{
				if(!foundspecie & !strcmp(species[s]->id,specieID))
				{
					foundspecie = 1;
					printf(":::: found results for specie %s\n",specieID);

					if(species[s]->type == bulk) 
					{
						if(fwrite(&numNodes,sizeof(int),1,wqout) != 1) {
							printf(":: Problem writing water quality output results to file %s\n\n",erdout);
							exit(1);
						}
						if(fwrite(&timeStepCount,sizeof(int),1,wqout) != 1) {
							printf(":: Problem writing water quality output results to file %s\n\n",erdout);
							exit(1);
						}
						for(n=0; n<numNodes; n++)
						{
							if(fwrite(qualResults->nodeC[s][n],sizeof(float),timeStepCount,wqout) != timeStepCount) {
								printf(":: Problem writing water quality output results to file %s\n\n",erdout);
								exit(1);
							}
						}
					}
					else
					{
						if(fwrite(&numLinks,sizeof(int),1,wqout) != 1) {
							printf(":: Problem writing water quality output results to file %s\n\n",erdout);
							exit(1);
						}
						if(fwrite(&timeStepCount,sizeof(int),1,wqout) != 1) {
							printf(":: Problem writing water quality output results to file %s\n\n",erdout);
							exit(1);
						}
						for(n=0; n<numLinks; n++)
						{
							if(fwrite(qualResults->linkC[s][n],sizeof(float),timeStepCount,wqout) != timeStepCount) {
								printf(":: Problem writing water quality output results to file %s\n\n",erdout);
								exit(1);
							}

						}
					}

				}
			}
			if( !foundspecie ) {
				printf(":: no result set found for specie %s\n",specieID);
			}
		}
	}

	if( !found ) {
		printf(":: no result set found for filenames %s and %s\n",erdinp,erdmsx);
	}
	// Close the database
	errorCode = ERD_close(&db);

	if(errorCode) { 
		message = ERD_getErrorMessage(errorCode); 
		printf(":: Problem closing database. %s\n\n", message); 
	} 

	return 0;
}

static void MEMCHECK(void *x, char *msg)
{
	if(x == NULL) {
		fprintf(stderr, "Allocating memory: %s", msg);
		exit(1);
	}
}
