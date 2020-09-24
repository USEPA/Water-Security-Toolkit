/*
	erdinfo
	this program simply gathers information about an EPAnet results database (erd) and 
	prints it to stdout.  it's basically like stat for erd.
*/

#include <stdio.h>
#include <erd.h>


int main (int argc, char * argv[])
{
printf("\n");						// initial line break for output

if (argc != 2) {					// bad input checking
	printf("Usage: \"erdinfo inp_dir\"\n");
	return 1;
}


PERD db;						// set pointer to Database structure
char *message;
int errorCode = 0;
int i, h, q, qualCount, hydCount, numSpeciesStored, rawCount; 
char inpFilename[1024];
char msxFilename[1024];


printf(":: Database info for directory: %s\n", argv[1]);

errorCode = ERD_open(&db, argv[1]);			// open ERD database

if(errorCode) {						// check error opening
	message = ERD_getErrorMessage(errorCode);
	printf(":: Problem opening database. %s\n\n", message);
	return 1;
}

hydCount = ERD_getHydSimCount(db);			// get hydraulic groups

numSpeciesStored = db->network->numSpecies;
printf(":: Species: %i (",numSpeciesStored);
for(i=0; i < numSpeciesStored; i++) {
	if (i != 0 )
		printf(", ");
	printf("%s",db->network->species[i]->id);
	if (db->network->species[i]->index == -1)
		printf(" <not stored>");
}
printf(")\n");
printf(":: Sim Duration: %d\n",db->network->simDuration);

printf(":: Hydraulic Groups: %i\n",hydCount);
printf(":: Quality results: %i\n",db->qualSimCount);

rawCount = 0;

for(h = 0; h < hydCount; h++) { 			// for each group, get info on quality results
	qualCount = ERD_getQualSimCountFor(h, db);
	printf("::: Group %i Quality Simulations: %i\n",(h+1),qualCount);
	for(q = 0; q < qualCount; q++) {
		strcpy(inpFilename, ERD_getINPFilename(db, rawCount));
		strcpy(msxFilename, ERD_getMSXFilename(db, rawCount));
		printf("::::: result set %i Generated from %s and %s\n",rawCount+1,inpFilename,msxFilename);
		rawCount ++;
	}
} 

errorCode = ERD_close(&db);
 
if(errorCode) { 
	message = ERD_getErrorMessage(errorCode); 
	printf(":: Problem closing database. %s\n\n", message); 
} 

printf("\n");
return 0;
}
