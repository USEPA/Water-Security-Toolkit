/*
printqual
this program simply prints out quality information for an ERD to a CSV-format file.
*/

#include <stdio.h>
#include <erd.h>

int main (int argc, char * argv[])
{
	char *erdDirectory, *outFileName;

	FILE *fileP;					// for output file
	PERD db;						// set pointer to Database structure
	char *message;
	int errorCode = 0;

	int q, h, qualCount, hydCount, numSpeciesStored, currentSpecie, timeStep, timeStepCount, numNodes, currentNodeIndex;
	PQualData myQualResults;
	PHydData myHydResults;
	float myQuality;

	printf("\n");					// initial line break for output
	if (argc != 3) {				// bad input checking
		fprintf(stdout,"Usage: \"%s erd_dir/ out_file.csv\"\n",argv[0]);
		return 1;
	}
	erdDirectory = argv[1];
	outFileName = argv[2];

	printf(":: Database in directory: %s\n", erdDirectory);

	errorCode = ERD_open(&db, erdDirectory);			// open ERD database


	if(errorCode) {							// check error opening
		message = ERD_getErrorMessage(errorCode);
		printf(":: Problem opening database. %s\n\n", message);
		return 1;
	}




	if (!(fileP = fopen(outFileName,"w"))) {
		printf(":: Problem opening file for output.\n\n");
		return 1;
	}


	hydCount = ERD_getHydSimCount(db);					// get hydraulic groups

	fprintf(fileP, "hydCount,qualCount,currentNodeIndex,timeStep,currentSpecie,myQuality\n");	// print first row of csv file (column headings)

	for(h = 0; h < hydCount; h++) { 
		errorCode = ERD_getHydResults(h,db,&myHydResults);
		if(errorCode) {						// check error
			message = ERD_getErrorMessage(errorCode);
			printf(":: Problem getting hyd results.  %d: %s\n\n", errorCode, message);
			return 1;
		}



		printf(":: getting qual count for group %i: ",h+1);
		qualCount = ERD_getQualSimCountFor(h, db);
		printf("%i\n",qualCount);

		// set up array for storing concentration data across quality simulations
		numSpeciesStored = db->network->numSpecies;
		numNodes = db->network->numNodes;
		timeStepCount = db->network->numSteps;

		for(q = 0; q < qualCount; q++) {
			errorCode = ERD_getQualResults(h, q, db, &myQualResults);
			if(errorCode) {						// check error
				message = ERD_getErrorMessage(errorCode);
				printf(":: Problem getting qual results.  %d: %s\n\n", errorCode, message);
				return 1;
			}
			for (currentSpecie=0; currentSpecie < numSpeciesStored; currentSpecie++) {
				if( db->network->species[currentSpecie]->type == bulk ) 
				{
					// Bulk species - defined on nodes
					for (currentNodeIndex=0; currentNodeIndex<numNodes; currentNodeIndex++) {
						for (timeStep=0; timeStep < timeStepCount; timeStep++) {
							myQuality = myQualResults->nodeC[currentSpecie][currentNodeIndex][timeStep];
							fprintf(fileP,"%i,%i,%i,%i,%i,%.8e\n",h,q,currentNodeIndex,timeStep,currentSpecie,myQuality);
						}
					}
				}
				else
				{
					// Wall species - defined on links
					for (currentNodeIndex=0; currentNodeIndex<numNodes; currentNodeIndex++) {
						for (timeStep=0; timeStep < timeStepCount; timeStep++) {
							myQuality = myQualResults->linkC[currentSpecie][currentNodeIndex][timeStep];
							fprintf(fileP,"%i,%i,%i,%i,%i,%.8e\n",h,q,currentNodeIndex,timeStep,currentSpecie,myQuality);
						}
					}
				}
			}
		}
	}


	errorCode = ERD_close(&db);

	if(errorCode) { 
		message = ERD_getErrorMessage(errorCode); 
		printf(":: Problem closing database. %s\n\n", message); 
	} 
	fclose(fileP);
	printf(":: Done.  Results written in %s\n\n",outFileName);
	return 0;
}
