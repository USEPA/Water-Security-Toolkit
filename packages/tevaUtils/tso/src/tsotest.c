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
#include <time.h>
#include "tevautil.h"


int main(int argc, char **argv)
{
	char *directoryOrFile,
		*pattern=NULL,
		*outputFile=NULL;
	char storageMethod=-1,
		fileVersion=-1,
		outputStorageMethod=-1,
		outputFileVersion=-1;
	PNodeInfo nodes;
	PLinkInfo links;
	PNetInfo net;
	PTSO tso;
	PTSO tsoOut=NULL;
	PSource sources;
	clock_t start,stop;
	int n;
	int spaceFillStrings=0;

	sources=(PSource)calloc(MAXSOURCES,sizeof(Source));

	if(argc <2 || argc >10) {
		printf("usage: tsotest directoryOrFile [patternIfDirectory] [-sStorageMethod -fFileVersion] [-o outputFile -osStorageMethod [-ofFileVersion]] [-spaceFillStrings]\n");
		exit(9);
	}

	directoryOrFile = argv[1];
	if(argc > 2) {
		int ai = 2;
		if(argv[ai][0] != '-') {
			pattern = argv[ai++];
		}
		while(ai<argc) {
			if(strcmp(argv[ai],"-spaceFillStrings")==0) {
				spaceFillStrings = 1;
			} else if(strncmp(argv[ai],"-v",2)==0) {
				fileVersion = atoi(&argv[ai][2]);
			} else if(strncmp(argv[ai],"-of",3)==0) {
				outputFileVersion = atoi(&argv[ai][3]);
			} else if(strncmp(argv[ai],"-os",3)==0) {
				outputStorageMethod = atoi(&argv[ai][3]);
			} else if(strncmp(argv[ai],"-o",2)==0) {
				outputFile = argv[++ai];
			} else if(strncmp(argv[ai],"-v",2)==0) {
				fileVersion = atoi(&argv[ai][2]);
			} else if(strncmp(argv[ai],"-s",2)==0) {
				storageMethod = atoi(&argv[ai][2]);
			} else {
				printf("usage: tsotest directoryOrFile [patternIfDirectory] [-sStorageMethod] [-fFileVersion\n");
				exit(9);
			}
			ai++;
		}

	}

	start = clock();

	if(fileVersion == -1 && storageMethod == -1)
		tso = TSO_Open(directoryOrFile,pattern);
	else if (fileVersion != -1 && storageMethod != -1)
		tso = TSO_OpenNoHeader(directoryOrFile,pattern,fileVersion,storageMethod);
	else {
		printf("Both storage method and file version must be set or not set\n");
		exit(9);
	}
	TSO_ReadPrologue(tso,&net,&nodes,&links);
	if(outputFile != NULL) {
		if(outputFileVersion > -1)
			tsoOut = TSO_CreateSpecificVersion(outputFile,outputStorageMethod,outputFileVersion,net);
		else
			tsoOut = TSO_Create(outputFile,outputStorageMethod,net);
		tsoOut->spaceFillStrings=spaceFillStrings;
		TSO_WritePrologue(tsoOut,net,nodes,links);
	}
//	n=1;
//	while(TSO_ReadSimulationResults(tso,net,nodes,sources)>0) {
//		if( (n % 1000==0)) { printf("|"); fflush(stdout); }
//		else if( (n % 500==0)) { printf("+"); fflush(stdout); }
//		else if( (n % 100==0)) { printf("."); fflush(stdout); }
//		n++;
//		if(tsoOut != NULL)
//			TSO_WriteSimResults(tsoOut,net,sources,nodes);
//	}
	n=0;
	while(n<tso->count) {
		TSO_GetSimulationResults(n,tso,net,nodes,sources);
		n++;
		if( (n % 1000==0)) { printf("|"); fflush(stdout); }
		else if( (n % 500==0)) { printf("+"); fflush(stdout); }
		else if( (n % 100==0)) { printf("."); fflush(stdout); }
		if(tsoOut != NULL)
			TSO_WriteSimResults(tsoOut,net,sources,nodes);
	}
	TSO_Close(&tso);
	if(tsoOut != NULL)
		TSO_Close(&tsoOut);
	TSO_ReleaseNetworkData(&nodes,&links,net);
	TSO_ReleaseNetworkInfo(&net);
	free(sources);
	stop = clock();
	printf("processed input in %7.3f seconds\n", (float)(stop-start)/(float)CLOCKS_PER_SEC);
}
