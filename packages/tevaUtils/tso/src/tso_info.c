/*
 * Copyright � 2008 UChicago Argonne, LLC
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
#include <stdarg.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tevautil.h"

int main(int argc, char **argv)
{
	char *file;
	int debug=0;
	PNodeInfo nodes;
	PLinkInfo links;
	PNetInfo net;
	PTSO tso=NULL;
	PERD erd=NULL;
	int printCount=0;
	int printInjections=0;
	PSourceData source;

	if(argc <2 || argc >4) {
		printf("usage: erdinfo file [-printCount] [-printInjections]\n");
		exit(9);
	}

	file = argv[1];
	if(argc > 2) {
		int ai = 2;
		while(ai<argc) {
			if(strcmp(argv[ai],"-printCount")==0) {
				printCount=1;
			} else if(strcmp(argv[ai],"-printInjections")==0) {
				printInjections=1;
			} else if(strcmp(argv[ai],"-debug")==0) {
				debug=1;
			} else {
				printf("usage: erdinfo ERDorTSOfile [-printCount] [-printInjections]\n");
				exit(9);
			}
			ai++;
		}
	}

	if(ERD_isERD(file)) {
		ERD_open(&erd,file,READ_QUALITY);
		net=erd->network;
		nodes=erd->nodes;
		links=erd->links;
	} else if(TSO_isTSO(file)) {
		tso = TSO_Open(file,NULL);
		TSO_ReadPrologue(tso,&net,&nodes,&links,READ_ALL);
	} else {
		fprintf(stderr,"Invalid ERD/TSO file: %s\n",file);
		exit(9);
	}

	fprintf(stdout,"File Version    %d\n",erd!=NULL?erd->header[2]:tso->fileVersion);
	fprintf(stdout,"Storage Method  %s\n",erd!=NULL?ERD_GetCompressionDesc(erd):TSO_GetCompressionDesc(tso));

	fprintf(stdout,"Is multi-part   %s\n",erd!=NULL?"yes":(tso->numFiles>0?"yes":"no"));

	fprintf(stdout,"\n");
	fprintf(stdout,"nsteps          %d\n",net->numSteps);
	fprintf(stdout,"stepsize (hrs)  %f\n",net->stepSize);
	fprintf(stdout,"nnodes          %d\n",net->numNodes);
	fprintf(stdout,"ntanks          %d\n",net->numTanks);
	fprintf(stdout,"nlinks          %d\n",net->numLinks);
	fprintf(stdout,"njunctions      %d\n",net->numJunctions);

	if(printCount || printInjections) {
		int idwidth=MAX_ID_LENGTH;
		int s,numScenarios;
		char *srcType[] = {"CONC","MASS"};
		int count=0;
		if(debug)
			printf("before TSO_ReadSimulationResults()\n");
		numScenarios=(erd != NULL?erd->qualSimCount:tso->count);
		for(s=0;s<numScenarios;s++) {
			int i;
			loadSimulationResults(s,erd,tso,net,nodes,&source);
			if(printInjections) {
				int nsources = source->nsources;
				PSource psrc = source->source;
				fprintf(stdout,"ensemble[%5d]:",s);
				for(i=0;i<nsources;i++) {
					int nhdr=i==0?1:17;
					fprintf(stdout,"%*s%*s %5d %s %2d %6ld %6ld %9.4e\n",nhdr," ",16,
									psrc->sourceNodeID,psrc->sourceNodeIndex,
									srcType[psrc->sourceType],psrc->speciesIndex,
									psrc->sourceStart,psrc->sourceStop,psrc->sourceStrength);
				}
				if(nsources>1)
					fprintf(stdout,"\n");
			}
			if(debug)
				printf("   read simulation results: %d\n",numScenarios);
		}
		fprintf(stdout,"Num Simulations %d\n",numScenarios);
	}
	if(tso!=NULL) {
		TSO_Close(&tso);
		TSO_ReleaseNetworkData(&nodes,&links,net);
		TSO_ReleaseNetworkInfo(&net);
	} else {
		ERD_close(&erd);
	}
}
