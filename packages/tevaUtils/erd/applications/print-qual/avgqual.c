/*
avgqual
this program simply prints out quality information for an ERD to a CSV-format file.
*/

#include <stdio.h>
#include <erd.h>

int main (int argc, char * argv[])
{
	char *erdDirectory, *outFileName;
	int nodeIndexArgument, specieIndexArgument;

	FILE *fileP;					// for output file
	PERD db;						// set pointer to Database structure
	char *message;
	int errorCode = 0;

	int i, q, h, qualCount, hydCount, timeStep, timeStepCount;
	PQualData myQualResults;
	PHydData myHydResults;
	float concentrationSum, avgQual;
	float *qualArray;

	printf("\n");					// initial line break for output
	if (argc != 5) {				// bad input checking
		fprintf(stdout,"Usage: \"%s erd_dir/ node_index specie_index out_file.csv\"\n",argv[0]);
		return 1;
	}
	erdDirectory = argv[1];
	nodeIndexArgument = atoi(argv[2]);
	specieIndexArgument = atoi(argv[3]);
	outFileName = argv[4];

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


	hydCount = ERD_getHydGroupCount(db);					// get hydraulic groups

	for(h = 0; h < hydCount; h++) { 
		errorCode = ERD_getHydResults(h,db,&myHydResults);
		if(errorCode) {						// check error
			message = ERD_getErrorMessage(errorCode);
			printf(":: Problem getting hyd results.  %d: %s\n\n", errorCode, message);
			return 1;
		}

		fprintf(fileP, "Time_Step,Avg_Concentration\n");			// print first row of csv file (column headings)

		printf(":: getting qual count for group %i: ",h);
		qualCount = ERD_getQualResultsCount(h, db);
		printf("%i\n",qualCount);

		// set up array for storing concentration data across quality simulations
		qualArray = (float *)calloc(qualCount, sizeof(float));
		if(qualArray == NULL) {
			return 1;
		}

		timeStepCount = db->network->numSteps;
		for (timeStep=0; timeStep < timeStepCount; timeStep++) {
			for(q = 0; q < qualCount; q++) {
				errorCode = ERD_getQualResults(h, q, db, &myQualResults);
				if(errorCode) {						// check error
					message = ERD_getErrorMessage(errorCode);
					printf(":: Problem getting qual results.  %d: %s\n\n", errorCode, message);
					return 1;
				}
				qualArray[q] = myQualResults->concentration[specieIndexArgument][timeStep][nodeIndexArgument];
			}

			concentrationSum = 0;

			for (i=0; i < qualCount; i++) {
				concentrationSum += qualArray[i];
			}


			avgQual = concentrationSum / (float)qualCount;

			fprintf(fileP,"%i,%'.16f\n",timeStep,avgQual);

		}
		free(qualArray);
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
