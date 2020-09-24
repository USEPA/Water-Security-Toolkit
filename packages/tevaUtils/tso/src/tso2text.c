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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tevautil.h"

typedef struct Options {
	char **ids;
	char *ident;
	int numToWrite;
	char *outputFileBase;
	int useExponentialNotation;
	int onlyNonZeroConc;
    int averageAtTimeStep;
} Options, *POptions;

int processTSO(char *tsoName, POptions opts);
int processERD(char *erdName, POptions opts);

int needToWriteID(char *id, char **ids, int len);
int processResults(int *wroteDemands, PNetInfo net, PNodeInfo nodes, PLinkInfo links, int numSources, PSource sources, POptions opts);

int addConcentrations(PNetInfo net, int averageAtTimeStep, float** concentrationTotals);
void writeConcentrationAveragesFile(PNetInfo net, int numModels, float** totalConcentrations, PNodeInfo nodes, POptions opts);

static char *erdUsageString="usage: tso2text erdName [-e] [-nz] [-ident STRING] [-n numToWrite | -ids [id1 id2 ... idn]\n  -e prints the concentrations in scientific notation\n  -nz only prints non-zero concentration values\n  either -n or -ids is required\n  -avgAt [timestep] Print average concentrations instead of per-scenario concentrations\n    The average is written for the indicated 0-based time step.\n\n";
int erdmain(int argc, char **argv);
int tsomain(int argc, char **argv);

int main(int argc, char **argv)
{
	Options opts;
	char *erdName=NULL;
	int wroteDemands=0;
	PERD erd=NULL;

	PNodeInfo nodes;
	PLinkInfo links;
	PNetInfo net;
	PTSO tso=NULL;
	PTSO tsoOut=NULL;
	PSourceData source;

	clock_t start,stop;
	int n,i,num;

    float** concentrationTotals = NULL;

	memset(&opts,0,sizeof(Options));

	if(argc <3) {
		printf(erdUsageString);
		exit(9);
	}

	opts.outputFileBase = "conc";
	opts.numToWrite=-1;
    opts.averageAtTimeStep = -1;

	erdName = argv[1];
	if(argc > 2) {
		int ai = 2;
		while(ai<argc) {
			if(strncmp(argv[ai],"-e",2)==0) {
				opts.useExponentialNotation = 1;
			} else if(strncmp(argv[ai],"-nz",3)==0) {
				opts.onlyNonZeroConc=1;
			} else if(strncmp(argv[ai],"-n",2)==0) {
				opts.numToWrite = atoi(argv[++ai]);
			} else if(strncmp(argv[ai],"-avgAt",6)==0) {
				opts.averageAtTimeStep = atoi(argv[++ai]);
			} else if(strncmp(argv[ai],"-ident",6)==0) {
				opts.ident=argv[++ai];
			} else if(strncmp(argv[ai],"-ids",4)==0) {
				opts.numToWrite=0;
				while(++ai < argc && argv[ai][0] != '-') {
					opts.ids = (char **)realloc(opts.ids,(opts.numToWrite+1)*sizeof(char *));
					opts.ids[opts.numToWrite] = argv[ai];
					opts.numToWrite++;
				}
			} else {
				printf(erdUsageString);
				exit(9);
			}
			ai++;
		}
	}
	if(opts.numToWrite == -1) {
		printf("either the -n or -ids must be specified\n");
		printf(erdUsageString);
		exit(9);
	}

	if(ERD_isERD(erdName)) {
		ERD_open(&erd,erdName,READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL | READ_IGNORE_ERROR);
		net=erd->network;
		nodes=erd->nodes;
		links=erd->links;
	} else if(TSO_isTSO(erdName)) {
		tso = TSO_Open(erdName,NULL);
		TSO_ReadPrologue(tso,&net,&nodes,&links,READ_ALL);
	} else {
		fprintf(stderr,"Invalid ERD/TSO file: %s\n",erdName);
		exit(9);
	}

	if (opts.averageAtTimeStep > -1) {
		concentrationTotals = malloc(net->numSpecies * sizeof(float*));
		for (i=0; i < net->numSpecies; i++) {
			concentrationTotals[i] = malloc(net->numNodes * sizeof(float));
			memset(concentrationTotals[i], 0, net->numNodes * sizeof(float));
		}
	}

	start = clock();
	i=0;
	n=0;
	num=get_count(erd,tso);
	for(i=0;i<num;i++) {
		if(n < opts.numToWrite) {
			if(loadSimulationResults(i,erd,tso,net,nodes,&source)) {
				if(opts.averageAtTimeStep > -1) {
					n += addConcentrations(net, opts.averageAtTimeStep, concentrationTotals);
				} else {
					int numSources=source->nsources;
					PSource sources=source->source;
					n+=processResults(&wroteDemands,net,nodes,links,numSources,sources,&opts);
				}
			}
		}
	}
	if(opts.averageAtTimeStep > -1) {
		writeConcentrationAveragesFile(net, n, concentrationTotals, nodes, &opts);
	}
	if(erd!=NULL) {
		ERD_close(&erd);
	} else {
		TSO_Close(&tso);
		TSO_ReleaseNetworkData(&nodes,&links,net);
		TSO_ReleaseNetworkInfo(&net);
	}
	stop = clock();
	printf("processed input in %7.3f seconds\n", (float)(stop-start)/(float)CLOCKS_PER_SEC);
	return 0;
}
int needToWriteID(char *id, char **ids, int len) {
	int i;
	for(i=0;i<len;i++) {
		if(strcmp(id,ids[i])==0) return 1;
	}
	return 0;
}

char* getDataFormatString(POptions opt) 
{
    if(opt->useExponentialNotation)
        return "\t%e";
    else
        return "\t%g";
}

int processResults(int *wroteDemands, PNetInfo net, PNodeInfo nodes, PLinkInfo links, int numSources, PSource sources, POptions opt)
{
	char *fmtStr=getDataFormatString(opt);

	if(!*wroteDemands) {
		char fn[256];
		PHydData hydData = net->hydResults;
		int nn,t;
		FILE *fp;

		sprintf(fn,"demands%s%s.txt",opt->ident==NULL?"":"_",opt->ident==NULL?"":opt->ident);
		fp = fopen(fn,"w");
		if(net->hydResults->demand == NULL) {
			fprintf(fp,"demands not stored in DB\b");
		} else {
			fprintf(fp,"NumReceptorNodes\t%d\n",net->numNodes);
			fprintf(fp,"NumSteps\t%d\n",net->numSteps);
			for(nn=0;nn<net->numNodes;nn++) {
				fprintf(fp,"%s",nodes[nn].id);
				for(t=0;t<net->numSteps;t++) {
					fprintf(fp,fmtStr,hydData->demand[t][nn]);
				}
				fprintf(fp,"\n");
			}
		}
		fclose(fp);

		sprintf(fn,"flow%s%s.txt",opt->ident==NULL?"":"_",opt->ident==NULL?"":opt->ident);
		fp = fopen(fn,"w");
		if(hydData->flow == NULL) {
			fprintf(fp,"Flow data not stored in DB\n");
		} else {
			fprintf(fp,"NumLinks\t%d\n",net->numLinks);
			fprintf(fp,"NumSteps\t%d\n",net->numSteps);
			for(nn=0;nn<net->numLinks;nn++) {
				fprintf(fp,"%s\t%s",nodes[links[nn].from-1].id,nodes[links[nn].to-1].id);
				for(t=0;t<net->numSteps;t++) {
					fprintf(fp,fmtStr,hydData->flow[t][nn]);
				}
				fprintf(fp,"\n");
			}
		}
		fclose(fp);

		sprintf(fn,"velocity%s%s.txt",opt->ident==NULL?"":"_",opt->ident==NULL?"":opt->ident);
		fp = fopen(fn,"w");
		if(hydData->velocity == NULL) {
			fprintf(fp,"Velocity data not stored in DB\n");
		} else {
			fprintf(fp,"NumLinks\t%d\n",net->numLinks);
			fprintf(fp,"NumSteps\t%d\n",net->numSteps);
			for(nn=0;nn<net->numLinks;nn++) {
				fprintf(fp,"%s\t%s",nodes[links[nn].from-1].id,nodes[links[nn].to-1].id);
				for(t=0;t<net->numSteps;t++) {
					fprintf(fp,fmtStr,hydData->velocity[t][nn]);
				}
				fprintf(fp,"\n");
			}
		}
		fclose(fp);
		*wroteDemands=1;
	}
	if(opt->ids == NULL || needToWriteID(sources[0].sourceNodeID,opt->ids,opt->numToWrite)) {
		char fn[256];
		int nn,t;
		FILE *fp,*fpbin;
		char *srcID=sources[0].sourceNodeID;
		float strength = sources[0].sourceStrength;
		long start = sources[0].sourceStart;
		long stop = sources[0].sourceStop;
		int s;
		sprintf(fn,"%s%s%s_%s_%.0f_%ld_%ld.txt",opt->outputFileBase,opt->ident==NULL?"":"_",opt->ident==NULL?"":opt->ident,srcID,strength,start,stop);
		fp = fopen(fn,"w");
		fprintf(fp,"SourceNodeID\t%s\n",srcID);
		fprintf(fp,"NumReceptorNodes\t%d\n",net->numNodes);
		fprintf(fp,"NumSteps\t%d\n",net->numSteps);
		fprintf(fp,"Source strength\t%e\n",sources[0].sourceStrength);
		fprintf(fp,"Source start\t%ld\n",sources[0].sourceStart);
		fprintf(fp,"Source stop\t%ld\n",sources[0].sourceStop);

		sprintf(fn,"%s%s%s_%s_%.0f_%ld_%ld.bin",opt->outputFileBase,opt->ident==NULL?"":"_",opt->ident==NULL?"":opt->ident,srcID,strength,start,stop);
		fpbin = fopen(fn,"wb");
	fwrite(&net->numSpecies,sizeof(float),1,fpbin);
	fwrite(&net->numNodes,sizeof(float),1,fpbin);
	fwrite(&net->numLinks,sizeof(float),1,fpbin);
	fwrite(&net->numSteps,sizeof(float),1,fpbin);
		for(s=0;s<net->numSpecies; s++) {
	int type;
			int sidx=net->species[s]->index;
	int idlen=(int)strlen(net->species[s]->id);
	fwrite(&idlen,sizeof(int),1,fpbin);
	fwrite(net->species[s]->id,sizeof(char),idlen,fpbin);
			// if the speciesdata was saved
			if(sidx != -1) {
				if(net->species[s]->type==bulk) {
					fprintf(fp,"%s Node Concentrations\n",net->species[s]->id);
	type=0;
	fwrite(&type,sizeof(int),1,fpbin);
					for(nn=0;nn<net->numNodes;nn++) {
						float **c=net->qualResults->nodeC[sidx];
						fprintf(fp,"%s\t%s",nodes[nn].id,net->species[s]->id);
	fwrite(c[nn],sizeof(float),net->numSteps,fpbin);
						for(t=0;t<net->numSteps;t++) {
							if(c[nn][t]>0 || opt->onlyNonZeroConc==0) {
								fprintf(fp,fmtStr,c[nn][t]);
							} else {
								fprintf(fp,"\t");
							}
						}
						fprintf(fp,"\n");
					}
				} else { // link data...
					fprintf(fp,"%s Link Concentrations\n",net->species[s]->id);
	type=0;
	fwrite(&type,sizeof(int),1,fpbin);
					for(nn=0;nn<net->numLinks;nn++) {
						float **c=net->qualResults->linkC[sidx];
						fprintf(fp,"%s\t%s",nodes[nn].id,net->species[s]->id);
	fwrite(c[nn],sizeof(float),net->numSteps,fpbin);
						for(t=0;t<net->numSteps;t++) {
							fprintf(fp,fmtStr,c[nn][t]);
						}
						fprintf(fp,"\n");
					}
				}
			}
		}
	fclose(fpbin);
		fclose(fp);
		return 1;
	}
	return 0;
}

int addConcentrations(PNetInfo net, int averageAtTimeStep, float** concentrationTotals) {
    int s;
    int nn;
    float **c;

    for(s=0; s<net->numSpecies; s++) {
        int sidx=net->species[s]->index;
        // if the species data was not saved, or is not bulk, skip it
        if(sidx == -1 || net->species[s]->type!=bulk) {
            continue; 
        }

        c=net->qualResults->nodeC[sidx];
        for(nn=0; nn<net->numNodes; nn++) {
            concentrationTotals[s][nn] += c[nn][averageAtTimeStep];
        }
    }
    return 1;
}

void writeConcentrationAveragesFile(PNetInfo net, int numModels, float** totalConcentrations, PNodeInfo nodes, POptions opts) {
    char fn[256];
    FILE* fp;
    char *fmtStr = getDataFormatString(opts);
    float divide_by = (float)numModels;
    int nn, s;

    sprintf(fn,"%s%sconcentration_averages_%d.txt", 
        opts->ident==NULL?"":opts->ident, opts->ident==NULL?"":"_",
        opts->averageAtTimeStep);
    fp = fopen(fn,"w");

    for(nn = 0; nn < net->numNodes; nn++) {
        fprintf(fp, nodes[nn].id);
        for(s=0; s<net->numSpecies; s++) {
            float avgConc = totalConcentrations[s][nn] / divide_by;
            fprintf(fp, fmtStr, avgConc);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}


