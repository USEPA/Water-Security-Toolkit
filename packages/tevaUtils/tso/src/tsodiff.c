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
#include <math.h>
#include "tevautil.h"

#define RMSENORM 1.0  /* If RMS concentration time series > this value, RMSE will be normalized by it */
void dumpSources(char *title,PTSO tso, PERD erd);

static char *usageString="usage: tsodiff file1 file2\n";
int compareLoaded(int *scens1,int *scens2,PTSO tso1,PTSO tso2,PERD erd1, PERD erd2);
int findTSOIndex(PTSO tso1, PTSO tso2, PSourceData source, int *scensUsed);
int findERDIndex(PERD erd2, PSourceData source1, int *scensUsed);
int compareSources(PSourceData source1,PSourceData source2, PTSO tso1, PTSO tso2,float strengthTol);
void resetBins(int *counts,int numBins);
void updateBins(double absv,double *bins,int *counts,int numBins);
void printBins(FILE *fp,double *bins,int *counts,int numBins);
void getSourceDescription(PSourceData source, PTSO tso, char *sDesc);
int compareInt(int i1,int i2, char *type);
int compareFloat(float f1,float f2, char *type);
int compareLong(long l1,long l2, char *type);
int compare2DFloat(float **f1,float **f2,int nrows, int ncols, char *type, char *elemType);

#define SOURCE_SAME          0x00
#define SOURCE_DIFF_ID       0x01
#define SOURCE_DIFF_START    0x02
#define SOURCE_DIFF_STOP     0x04
#define SOURCE_DIFF_STRENGTH 0x08
#define SOURCE_DIFF_TYPE     0x10
#define SOURCE_DIFF_SPECIES  0x20
#define SOURCE_DIFF_NUM      0x40

char *getDiffString(int diff) {
	static char res[256];
	int first=1;
	res[0]=0;
	if(diff==SOURCE_SAME) return "Same";
	if(diff & SOURCE_DIFF_ID)       { if(!first) {strcat(res,", "); } strcat(res,"ID"); first=0; }
	if(diff & SOURCE_DIFF_START)    { if(!first) {strcat(res,", "); } strcat(res,"Start"); first=0; }
	if(diff & SOURCE_DIFF_STOP)     { if(!first) {strcat(res,", "); } strcat(res,"Stop"); first=0; }
	if(diff & SOURCE_DIFF_STRENGTH) { if(!first) {strcat(res,", "); } strcat(res,"Strength"); first=0; }
	if(diff & SOURCE_DIFF_TYPE)     { if(!first) {strcat(res,", "); } strcat(res,"Type"); first=0; }
	if(diff & SOURCE_DIFF_SPECIES)  { if(!first) {strcat(res,", "); } strcat(res,"Species"); first=0; }
	if(diff & SOURCE_DIFF_NUM)      { if(!first) {strcat(res,", "); } strcat(res,"Num"); first=0; }
	return res;
}
/*
**--------------------------------------------------------------
** Module:  TSODIFF
** Usage:   tsodiff <file1.tso> <file2.tso>
** Input:   Two different TSO files to compare
** Output:  None
** Purpose: Compare two TSO files to characterize differences.
** JGU 7/22/05
**---------------------------------------------------------------
*/

struct ErrorTracker {
	float value;
	int node;
	int time;
	int species;
};

int main(int argc, char **argv)
{
	char *file1,
        *file2;
	char storageMethod=-1,
		fileVersion=-1;
	int loaded,loaded1,loaded2;
    int id1,id2;
	int numScenarios;
    int *scens1,*scens2;
    int sameprologue;
    int ai,i,j,k,maxnodeidx,maxspecie,samesource;
    float a,e,maxrmse,rmse,rms1,rms2;
    int scenario,maxscenario;
    float maxscenariormse;
	int *counts;
	double bins[]={10,1,1e-2,1e-5,1e-10,0};
	int numBins=6;
	FILE *fp;
	PHydData hydData1,hydData2;
	PQualData qualData1,qualData2;

	PNodeInfo nodes1,nodes2;
	PLinkInfo links1,links2;
	PNetInfo net1,net2;
	PTSO tso1,tso2;
//	PSource sources1,sources2;
	PSourceData source1,source2;
	PERD erd1,erd2;
	int is_yaml, is_strict;

	tso1=tso2=NULL;
	erd1=erd2=NULL;

	if(argc < 3 || argc > 5) {
		printf(usageString);
		exit(9);
	}

    ai=1;
    file1 = argv[ai++];
    file2 = argv[ai++];
    
    is_yaml = 0;
    is_strict = 0;
    if ((argc > 3) && (!strcmp(argv[ai],"--strict"))) {
	is_strict = 1;
    }
    if ((argc > 3) && (!strcmp(argv[ai],"--yaml"))) {
	fp=fopen("stats.yml","w");
	is_yaml = 1;
    }
    else {
	fp=fopen("stats.txt","w");
    }
    counts=(int *)calloc(numBins+1,sizeof(int));
    

	if(ERD_isERD(file1)) {
		printf("Opening ERD File: %s\n",file1);
		ERDCHECK(ERD_open(&erd1,file1,READ_IGNORE_ERROR | READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL));
		net1=erd1->network;
		nodes1=erd1->nodes;
		links1=erd1->links;
	} else if(TSO_isTSO(file1)) {
		printf("Opening TSO File: %s\n",file1);
		tso1 = TSO_Open(file1,NULL);
		TSO_ReadPrologue(tso1,&net1,&nodes1,&links1,READ_IGNORE_ERROR | READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL);
	} else {
		fprintf(stderr,"File is not a TSO or ERD: %s\n",file1);
	}
	if(ERD_isERD(file2)) {
		printf("Opening ERD File: %s\n",file2);
		ERDCHECK(ERD_open(&erd2,file2,READ_IGNORE_ERROR |READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL));
		net2=erd2->network;
		nodes2=erd2->nodes;
		links2=erd2->links;
	} else if(TSO_isTSO(file2)) {
		printf("Opening TSO File: %s\n",file2);
		tso2 = TSO_Open(file2,NULL);
		TSO_ReadPrologue(tso2,&net2,&nodes2,&links2,READ_IGNORE_ERROR | READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL);
	} else {
		fprintf(stderr,"File is not a TSO or ERD: %s\n",file2);
	}

    /* Check prologue size info */
    printf("Analyzing Prologue\n");
    sameprologue=1;
	sameprologue &= compareInt(get_count(erd1,tso1),get_count(erd2,tso2),"Num Injections: ");
	sameprologue &= compareInt(net1->numJunctions,net2->numJunctions,    "NJunctions:     ");
	sameprologue &= compareInt(net1->numLinks,net2->numLinks,            "NLinks:         ");
	sameprologue &= compareInt(net1->numNodes,net2->numNodes,            "NNodes:         ");
	sameprologue &= compareInt(net1->numSteps,net2->numSteps,            "NSteps:         ");
	sameprologue &= compareInt(net1->numTanks,net2->numTanks,            "NTanks:         ");
    if((erd1 != NULL || (tso1!=NULL&&tso1->fileVersion >= 4)) && 
	   (erd2 != NULL || (tso2!=NULL&&tso2->fileVersion >= 4))) {
		sameprologue &= compareInt(net1->numSpecies,net2->numSpecies,    "NSpecies:       ");
	}
	sameprologue &= compareFloat(net1->stepSize,net2->stepSize,          "StepSize:       ");
	if((erd1 != NULL || (tso1!=NULL&&tso1->fileVersion >= 6)) && 
	   (erd2 != NULL || (tso2!=NULL&&tso2->fileVersion >= 6))) {
		sameprologue &= compareInt(net1->qualCode,net2->qualCode,        "QualityCode:    ");
		sameprologue &= compareLong(net1->reportStart,net2->reportStart, "ReportStart:    ");
		sameprologue &= compareLong(net1->reportStep,net2->reportStep,   "ReportStep:     ");
		sameprologue &= compareLong(net1->simDuration,net2->simDuration, "SimDuration:    ");
	}
    if (!sameprologue) {
        printf("Files %s and %s have different prologues\n",file1,file2);
        exit(9);
    }

    /* Specie Prologue info */
    if((erd1 != NULL || (tso1!=NULL&&tso1->fileVersion >= 4)) && 
	   (erd2 != NULL || (tso2!=NULL&&tso2->fileVersion >= 4))) {
        for(i=0;i<net1->numSpecies;i++) {
			if(strcmp(net1->species[i]->id,net2->species[i]->id)) 
                printf("Warning: Specie %d ID mismatch: %s != %s\n",i+1,net1->species[i],net2->species[i]);
        }
    }

	hydData1=erd1!=NULL?erd1->network->hydResults:net1->hydResults;
	hydData2=erd2!=NULL?erd2->network->hydResults:net2->hydResults;
    /* Node Prologue info */
    for(i=0;i<net1->numNodes;i++) {
        if(strcmp(nodes1[i].id,nodes2[i].id))
            printf("Warning: Node %d ID mismatch: %s != %s\n",i+1,nodes1[i].id,nodes2[i].id);
    }
	compare2DFloat(hydData1->demand,hydData2->demand,net1->numSteps,net1->numNodes,"demand","node");

    /* Link Prologue info */
    if((erd1 != NULL || (tso1!=NULL&&tso1->fileVersion >= 4)) && 
	   (erd2 != NULL || (tso2!=NULL&&tso2->fileVersion >= 4))) {
		   compare2DFloat(hydData1->velocity,hydData2->velocity,net1->numSteps,net1->numLinks,"velocity","link");
		   compare2DFloat(hydData1->flow,hydData2->flow,net1->numSteps,net1->numLinks,"flow","link");

    }
    if((erd1 != NULL || (tso1!=NULL&&tso1->fileVersion >= 6)) && 
	   (erd2 != NULL || (tso2!=NULL&&tso2->fileVersion >= 6))) {
        for(i=0;i<net1->numLinks;i++) {
            if(links1[i].length != links2[i].length) 
                printf("Warning: Link %d length mismatch: %e != %e\n",i+1,links1[i].length,links2[i].length);
        }
    }
    
    /* Scenario concentrations */
    printf("\nAnalyzing Scenario Concentrations: [Max RMSE/RMSC] [Max Node] [Max Specie]\n");
    if (is_yaml) {
	fprintf(fp,"# Analysis of Scenario Concentration Differences\n");
    }
    else {
	fprintf(fp,"\nAnalyzing Scenario Concentrations\n");
	fprintf(fp,"Scenario\tMax RMSE/RMSC\tMax Node\tMax Specie\tnum w/c>0");
	for(i=0;i<numBins;i++) {
		fprintf(fp,"\tnum w/dc>%lf",bins[i]);
	}
	fprintf(fp,"\tnum w/dc<=%lf\n",bins[i-1]);
    }

    scenario=1;
    maxscenario=0;
    maxscenariormse=0.;
    id1=0;
	numScenarios=get_count(erd1,tso1);
    scens1=(int*)calloc(numScenarios,sizeof(int));
    scens2=(int*)calloc(numScenarios,sizeof(int));
    while ( id1 < numScenarios) {
		char scenDesc[1024];
		loaded1=loadSimulationResults(id1,erd1,tso1,net1,nodes1,&source1);
		if(tso2!=NULL) {
			id2=findTSOIndex(tso1,tso2,source1,scens2);
		} else {
			id2=findERDIndex(erd2,source1,scens2);
		}
		if(id2 != -1) {
			loaded2=loadSimulationResults(id2,erd2,tso2,net2,nodes2,&source2);
			if(loaded2) scens2[id2]=1;
			if(loaded1) scens1[id1]=1;
		} else {
			loaded2=0;
		}
		

		printf("loaded 1: %d, loaded 2: %d\n", loaded1, loaded2);
		/* Sources */
		if(loaded1 && loaded2) {
			qualData1=erd1!=NULL?erd1->network->qualResults:net1->qualResults;
			qualData2=erd2!=NULL?erd2->network->qualResults:net2->qualResults;
			samesource = compareSources(source1, source2, tso1, tso2,(float)0);

			if(samesource!=SOURCE_SAME) {
				printf("Warning: Inconsistent source description for scenario %6d\n",scenario);
				if(samesource & SOURCE_DIFF_NUM)       printf("   Number of sources\n");
				if(samesource & SOURCE_DIFF_ID)        printf("   Source Node ID\n");
				if(samesource & SOURCE_DIFF_START)     printf("   Start Time\n");
				if(samesource & SOURCE_DIFF_STOP)      printf("   Stop Time\n");
				if(samesource & SOURCE_DIFF_STRENGTH)  printf("   Source Strength\n");
				if(samesource & SOURCE_DIFF_TYPE)      printf("   Source Type\n");
				if(samesource & SOURCE_DIFF_SPECIES)   printf("   Source Species\n");
			} else {
				int printed=0;
				int gt0;
				float *rmse_nt=(float*)calloc(net1->numSpecies,sizeof(float));
				struct ErrorTracker* max_abse = (struct ErrorTracker*)calloc(net1->numSpecies,sizeof(*max_abse));
				maxnodeidx=0;
				maxspecie=0;
				maxrmse=0.;

				resetBins(counts,numBins);
				gt0=0;
				for(j=0;j<net1->numSpecies;j++) {
					float **c1=qualData1->nodeC[j];
					float **c2=qualData2->nodeC[j];
					max_abse[j].value = 0.;
					max_abse[j].species = j;
					rmse_nt[j] = 0.;
					for(i=0;i<net1->numNodes;i++) {
						rmse=0.;
						rms1=0.;
						rms2=0.;
						for(k=0;k<net1->numSteps;k++) {
							float cv1=c1[i][k];
							float cv2=c2[i][k];
							if(cv1>0 || c2>0) gt0++;
							e=(float)(cv1-cv2);
							if(fabs(e)> 1e-10 && (cv2 == 0.0 || cv1 == 0)) {
								if(!printed)
									printf("non-zero vs. zero concentration values: scen,species,node,t: %d,%d,%s,%d tso1: %e tso2: %e\n",
									scenario,j,nodes1[i].id,k,cv1,cv2);
								printed++;
							} else {
								if(printed>1)
									printf("there were %d additional messages regarding the non-zero vs. zero concentration values\n",
									printed);
								printed=0;
							}
							if (fabs(e) > max_abse[j].value) {
								max_abse[j].value = fabs(e);
								max_abse[j].node = i;
								max_abse[j].time = k;
							}
							updateBins(fabs(e),bins,counts,numBins);
							rmse+=e*e;
							rms1+=cv1*cv1;
							rms2+=cv2*cv2;
						}
						rmse_nt[j]+=rmse;
						rmse=(float)sqrt(rmse/(float)net1->numSteps);
						rms1=(float)sqrt(rms1/(float)net1->numSteps);
						rms2=(float)sqrt(rms2/(float)net1->numSteps);
						if((a=(float)((rms1+rms2)/2.0)) > RMSENORM) rmse/=a;
						if(rmse>maxrmse) {
							maxrmse=rmse;
							maxnodeidx=i+1;
							maxspecie=j+1;
						}
						
					}
					rmse_nt[j]=(float)sqrt(rmse_nt[j]/(((float)net1->numSteps)*((float)net1->numNodes)));

				}
				printf("Scenario %6d: %12e %6d %4d\n",scenario,maxrmse,maxnodeidx,maxspecie);
				getSourceDescription(source1, tso1, scenDesc);
				if (is_yaml) {
					fprintf(fp,"- scenario: %d\n",scenario);
					fprintf(fp,"  source description: \"%s\"\n",scenDesc);
					for (j=0;j<net1->numSpecies;j++) {
						fprintf(fp,"  species %s: {\n", net1->species[j]->id);
						fprintf(fp,"               rmse: %e,\n", rmse_nt[j]);
						fprintf(fp,"               max abse: {Value: %e, Timestep: %d, Node: %s}\n", max_abse[j].value, max_abse[j].time, nodes1[max_abse[j].node].id);
						fprintf(fp,"              }\n");
					}
				}
				else {
					fprintf(fp,"%6d\t%12e\t%6d\t%4d\t%7d\t",scenario,maxrmse,maxnodeidx,maxspecie,gt0);
					printBins(fp,bins,counts,numBins);
					fprintf(fp,"\t%s\n",scenDesc);
				}
				fflush(fp);
				if(maxrmse>maxscenariormse) {
					maxscenariormse=maxrmse;
					maxscenario=scenario;
				}
				free(max_abse);
				free(rmse_nt);
			}
		} /** end of if loaded1 && loaded2 */
		else {
		  if (is_strict) {
		    fprintf(stderr,"A scenario failed to load.\n");
		    return 1;
		  }
                }
		id1++;
		scenario++;
	} /* end of while */

    compareLoaded(scens1,scens2,tso1,tso2,erd1,erd2);
	if(tso1!= NULL) TSO_Close(&tso1);
	if(tso2!= NULL) TSO_Close(&tso2);
	if(erd1!= NULL) ERD_close(&erd1);
	if(erd2!= NULL) ERD_close(&erd2);
	fclose(fp);
    printf("\nMaximum (RMS error)/(RMS conc) = %12e at scenario %6d\n",maxscenariormse,maxscenario);
	free(scens1);
	free(scens2);
	free(counts);
	
    return 0;
}
void printUnused(int *scens,PTSO tso,PERD erd) {
	int i,s,errors,nsources;
	PSource sources;
	PSourceData sd;
	int num=get_count(erd,tso);
	errors=0;
	for(i=0;i<num;i++) {
		if(!scens[i]) {
			if(!errors) {
				errors=1;
				printf("unused scenarios from TSO #1:\n");
			}
			sd=tso==NULL?(PSourceData)erd->qualSim[i]->appData:tso->index[i]->sourceData;
			nsources=sd->nsources;
			sources=sd->source;
			for(s=0;s<nsources;s++) {
				printf("  %s%s",(s>0?",":""),sources[s].sourceNodeID);
			}
		}
	}
}
int compareLoaded(int *scens1,int *scens2,PTSO tso1,PTSO tso2,PERD erd1, PERD erd2) {
	int errors;
	errors=0;
	printUnused(scens1,tso1,erd1);
	printUnused(scens2,tso2,erd2);
	return 0;
}
/* Find the index in tso2 that matches the source info for erd1/tso1 */
int findTSOIndex(PTSO tso1, PTSO tso2, PSourceData source1, int *scensUsed)
{
  int i;
  PSource ps;
  for(i=0;i<tso2->count;i++) {
    if(!scensUsed[i]) {
      PDataIndex idx=tso2->index[i];
      // now find out if this one is the same as index...
      int samesource= compareSources(source1, idx->sourceData, tso1, tso2,0.001f);
      if(samesource==SOURCE_SAME) {
      	return i;
      }
    }
  }
  ps = &source1->source[0];
  printf("Source 1: %8s %8d %8d %8.3f %d\n",ps->sourceNodeID,ps->sourceStart,ps->sourceStop,ps->sourceStrength,ps->sourceType);
  printf("Source 2: Not Found\n");
  return -1;
}
int findERDIndex(PERD erd2, PSourceData source1, int *scensUsed)
{
	int i;
	PSource ps;
	ps = &source1->source[0];
	for(i=0;i<erd2->qualSimCount;i++) {
		if(!scensUsed[i]) {
			PQualitySim qs=erd2->qualSim[i];
			PSourceData idx=(PSourceData)qs->appData;
			// now find out if this one is the same as index...
			int samesource= compareSources(source1, idx, NULL, NULL,0.001f);
			if(samesource==SOURCE_SAME) {
				return i;
			}
		}
	}
	ps = &source1->source[0];
	printf("Source 1: %8s %8d %8d %8.3f %d\n",ps->sourceNodeID,ps->sourceStart,ps->sourceStop,ps->sourceStrength,ps->sourceType);
	printf("Source 2: Not Found\n");
	return -1;
}
void getSourceDescription(PSourceData source, PTSO tso, char *sDesc)
{
	int i;
	char fmtStr[1024];
	PSource sources;

	fmtStr[0]='\0';
	sDesc[0]='\0';
	strcat(fmtStr,"%s");  // SourceNodeID
	strcat(fmtStr," %f (%d - %d)"); // Source Strength (SourceStart - SourceStop)
	sources=source->source;
	for(i=0;i<source->nsources;i++) {
		char tstr[1024];
		if(i>0) strcat(sDesc," ");
		sprintf(tstr,fmtStr,sources[i].sourceNodeID,sources[i].sourceStrength,sources[i].sourceStart,sources[i].sourceStop);
		strcat(sDesc,tstr);
	}
}
int compareSources(PSourceData source1,PSourceData source2, PTSO tso1, PTSO tso2,float strengthTol)
{
	int i,samesource=SOURCE_DIFF_NUM;
	int nsources1=source1->nsources;
	int nsources2=source2->nsources;
	PSource sources1=source1->source;
	PSource sources2=source2->source;
	if(nsources1 == nsources2) {
		samesource=SOURCE_SAME;
		for(i=0;i<nsources1;i++) {
			if(strcmp(sources1[i].sourceNodeID,sources2[i].sourceNodeID)) samesource|=SOURCE_DIFF_ID;
			// tso will be NULL if it is an ERD file
			if((tso1==NULL || tso1->fileVersion >= 2) && (tso2==NULL || tso2->fileVersion >= 2)) {
				if(sources1[i].sourceStart!=sources2[i].sourceStart) samesource|=SOURCE_DIFF_START;
				if(sources1[i].sourceStop!=sources2[i].sourceStop) samesource|=SOURCE_DIFF_STOP;
				if(strengthTol==0) {
					if(sources1[i].sourceStrength!=sources2[i].sourceStrength) {
						samesource|=SOURCE_DIFF_STRENGTH;
					}
				} else if(fabs(sources1[i].sourceStrength-sources2[i].sourceStrength)>strengthTol) {
						samesource|=SOURCE_DIFF_STRENGTH;
				}
			}
			// tso will be NULL if it is an ERD file
			if((tso1==NULL || tso1->fileVersion >= 4) && (tso2==NULL || tso2->fileVersion >= 4)) {
				if(sources1[i].sourceType!=sources2[i].sourceType) samesource|=SOURCE_DIFF_TYPE;
				if(sources1[i].speciesIndex!=sources2[i].speciesIndex) samesource|=SOURCE_DIFF_SPECIES;
			}
		}
	}
	return samesource;
}

void resetBins(int *counts,int numBins) {
	int i;
	for(i=0;i<numBins+1;i++) counts[i]=0;
}
void updateBins(double absv,double *bins,int *counts,int numBins)
{
	int i;
	for(i=0;i<numBins;i++) {
		if(absv > bins[i]) {
			counts[i]++;
			return;
		}
	}
	counts[i]++;
}
void printBins(FILE *fp,double *bins,int *counts,int numBins)
{
	int i;
	for(i=0;i<numBins+1;i++) {
		fprintf(fp,"\t%6d",counts[i]);
	}
}

int compareInt(int i1,int i2, char *type)
{
	printf("%s%13d %13d [%s]\n",type,i1,i2,i1==i2?"OK":"***");
	return i1==i2;
}
int compareFloat(float f1,float f2, char *type)
{
	printf("%s%8e %8e [%s]\n",type,f1,f2,f1==f2?"OK":"***");
	return f1==f2;
}
int compareLong(long l1,long l2, char *type)
{
	printf("%s%13d %13d [%s]\n",type,l1,l2,l1==l2?"OK":"***");
	return l1==l2;
}
int compare2DFloat(float **f1,float **f2,int nrows, int ncols, char *type, char *etype)
{
	float maxerr,e;
	int i,j,maxidx,maxstep;
	maxerr=0.;
	maxidx=0;
	maxstep=0;
	if(f1==NULL && f2==NULL) return 1;
	if(f1==NULL && f2!=NULL) return 0;
	if(f1!=NULL && f2==NULL) return 0;

	for(i=0;i<nrows;i++) {
		for(j=0;j<ncols;j++) {
            if((e=(float)fabs(f1[i][j]-f2[i][j])) > maxerr) {
                maxerr = e;
                maxidx = i+1;
                maxstep = j+1;
            }
        }
	}
    if(maxerr > 0.)
        printf("Max. abs. %s discrepancy = %e at %s idx %d and time step %d\n",type,maxerr,etype,maxidx,maxstep);
    else
        printf("Identical %s %s [OK]\n",etype,type);
	return maxerr > 0;
}



void dumpSources(char *title,PTSO tso, PERD erd) {
	FILE *fp;
	char fname[256];
	sprintf(fname,"%s.src.txt",title);
	fp=fopen(fname,"w");

	if(tso!=NULL) {
		int i,j;
		for(i=0;i<tso->count;i++) {
			int nsources=tso->index[i]->sourceData->nsources;
			PSource sources=tso->index[i]->sourceData->source;
			for(j=0;j<nsources;j++) {
				PSource ps=&sources[j];
				fprintf(fp,"[%d %s %d %d %8.3f %d]",ps->speciesIndex,ps->sourceNodeID,ps->sourceStart,ps->sourceStop,ps->sourceStrength,ps->sourceType);
			}
			fprintf(fp,"\n");
		}
	} else {
		int i,j;
		for(i=0;i<erd->qualSimCount;i++) {
			PSourceData index=(PSourceData)erd->qualSim[i]->appData;
			int nsources=index->nsources;
			PSource sources=index->source;
			for(j=0;j<nsources;j++) {
				PSource ps=&sources[j];
				fprintf(fp,"[%d %s %d %d %8.3f %d]",ps->speciesIndex,ps->sourceNodeID,ps->sourceStart,ps->sourceStop,ps->sourceStrength,ps->sourceType);
			}
			fprintf(fp,"\n");
		}
	}
}

