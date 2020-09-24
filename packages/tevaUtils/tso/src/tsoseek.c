#include <stdarg.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tevautil.h"


int main(int argc, char **argv)
{
	char *directoryOrFile,
		*pattern=NULL;
	int idx=-1;
	int ai=0;
	int debug=0;
	PNodeInfo nodes;
	PLinkInfo links;
	PNetInfo net;
	PTSO tso;
	PSource sources;
	int printCount=0;
	int printInjections=0;

	sources=(PSource)calloc(MAXSOURCES,sizeof(Source));

	if(argc != 3) {
		printf("usage: tsoseek directoryOrFile [-idx INDEX | -id ID]\n");
		exit(9);
	}

	directoryOrFile = argv[1];
	ai=2;
	while(ai<argc) {
		if(strcmp(argv[ai],"-idx")==0) {
			idx=atoi(argv[++ai]);
		} else if(strcmp(argv[ai],"-id")==0) {
			id=argv[++ai];
		} else {
			printf("usage: tsoseek directoryOrFile [-idx INDEX | -id ID]\n");
			exit(9);
		}
		ai++;
	}

	tso = TSO_Open(directoryOrFile,NULL);

	TSO_ReadPrologue(tso,&net,&nodes,&links);
	fprintf(stdout,"File Version    %d\n",tso->fileVersion);
	fprintf(stdout,"Storage Method  %d\n",tso->storageMethod);

	fprintf(stdout,"Is multi-part   %s\n",tso->numFiles>0?"yes":"no");

	fprintf(stdout,"\n",tso->storageMethod);
	fprintf(stdout,"nsteps          %d\n",net->nsteps);
	fprintf(stdout,"stepsize (hrs)  %f\n",net->stepsize);
	fprintf(stdout,"nnodes          %d\n",net->nnodes);
	fprintf(stdout,"ntanks          %d\n",net->ntanks);
	fprintf(stdout,"nlinks          %d\n",net->nlinks);
	fprintf(stdout,"njunctions      %d\n",net->njunctions);

	sources=(PSource)calloc(MAXSOURCES,sizeof(Source));
//	if(id==-1)
//	    id=findIndex(,tso1,tso2,sources1,sources2,scens2);
    TSO_GetSimulationResults(id,tso,net,nodes,sources);

	
	TSO_Close(&tso);
	TSO_ReleaseNetworkData(&nodes,&links,net);
	TSO_ReleaseNetworkInfo(&net);
	free(sources);

}
