/*
 * Copyright (c) 2012 UChicago Argonne, LLC
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
/* System Headers */
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <float.h>

/* TEVA headers */
#include "tevautil.h"
#include "infrstr_assess.h"

int processOptions(char **argv, int nargs,PII_Data iid);
void writePrimaryOutputFile(PII_Data iid, char *fn);
void writePipeDiameterOutputFiles(PII_Data iid);
void writePipeDiameterOutputFile(PII_Data iid, char *fn, int threshIdx, int pct);

void initThresholds(PII_Data iid,char **threshS,int num)
{
	int i=0;
	float curMinThreshold=FLT_MAX;
	iid->pot=AllocatePFOTData(num,iid->net->numLinks);
	for(i=0;i<num;i++) {
		iid->pot->thresholds[i]=(float)atof(threshS[i]);
		if(iid->pot->thresholds[i] < curMinThreshold)
			curMinThreshold=iid->pot->thresholds[i];
	}
	iid->pot->minThreshold=curMinThreshold;
}
char *createSimID(PSourceData source,PNodeInfo nodeinfo, char *simID) {
	int i;
	PIIAResults p = (PIIAResults)calloc(1,sizeof(IIAResults));
	simID[0]=0;
	for(i=0;i<source->nsources;i++) {
		if(i>0) strcat(simID,",");
		strcat(simID,nodeinfo[source->source[i].sourceNodeIndex-1].id);
	}
	return simID;
}
/*
**--------------------------------------------------------------
** Module:  ISTRUCT-ASSESS
** Usage:   istruct-assess.exe -erd
** Input:   
** Output:
** Purpose:
**---------------------------------------------------------------
*/
int main(int argc, char **argv)
{
    int
        i,num,s,
        scenario;
    time_t
        cpustart,
        cpustop;
    double
        cpuelapsed;

    /* Data model - see mem structure */
    II_Data   iid;
    PTSO      tso=NULL;
    PERD      erd;
    PNodeInfo nodeinfo;
    PLinkInfo linkinfo;
    PNetInfo  net;
	int diamBreakdown;

	PIIAResults
		thisIIA,
		prevIIA;

    PSourceData source;
	char *erdname;

	memset(&iid,0,sizeof(II_Data));
    time( &cpustart );

    if((argc<=2)) {
		printf("***Incorrect invocation\n");
		printf("***Correct usage is: teva-assess <erd-file> [-specie specie_name] -thresholds <list of thresholds>\n");
		exit(1);
	}
	erdname=argv[1];
	// first try as erd, then tso
	if(ERD_isERD(erdname)) {
		ERD_open(&iid.erd,erdname,READ_DEMANDS | READ_QUALITY | READ_DEMANDPROFILES | READ_LINKFLOW);
		iid.net=iid.erd->network;
		iid.nodeinfo=iid.erd->nodes;
		iid.linkinfo=iid.erd->links;
	} else if(TSO_isTSO(erdname)) {
		iid.tso = TSO_Open(erdname,NULL);
		TSO_ReadPrologue(iid.tso,&iid.net,&iid.nodeinfo,&iid.linkinfo,READ_DEMANDS | READ_QUALITY | READ_LINKFLOW);
	} else {
		printf("**** %s is not an ERD or TSO file\n",erdname);
		exit(1);
	}
	if(iid.tso != NULL || ERD_getVersion(iid.erd) < 13) {
		printf("\n\nDatabase file does not have link diameters - pipe diameter breakdown will not be output\n\n");
		diamBreakdown=0;
	} else {
		diamBreakdown=1;
	}
	InitializeIAMemory(&iid);
	if(!processOptions(&argv[2],argc-2,&iid)) {
		printf("Error processing optiopns\n");
		exit(1);
	}

    /* Copy data model addresses from mem for convenience */
    net=iid.net;
	erd=iid.erd;
	tso=iid.tso;
	nodeinfo=iid.nodeinfo;
	linkinfo=iid.linkinfo;
	prevIIA = NULL;

    /*****************************************************/
    /*  Infrastructure Impact analysis                   */
    /*****************************************************/
    printf("Infrastructure Impact Analysis for Scenario..\n");
    scenario=0;
	num=get_count(erd,tso);
	for(s=0;s<num;s++) {
		if(loadSimulationResults(s,erd,tso,net,nodeinfo,&source)) {
			char simID[100];
			int j;
			scenario++;

			resetPipeFeetOverThreshold(&iid);

			pipesContaminated(&iid);

			thisIIA=allocateIIAResultsData(createSimID(source,iid.nodeinfo,simID),&iid);
			if(prevIIA==NULL) {
				iid.iiaResults = thisIIA;
			} else {
				prevIIA->next=thisIIA;
			}
			prevIIA = thisIIA;

			for(i=0;i<iid.pot->numThresh;i++) {
				thisIIA->pfot[i]=iid.pot->potd.feetOver[i];
				for(j=0;j<iid.pot->numDiameters;j++) {
					thisIIA->pfotd[i][j]=iid.pot->potdDiam[j]->potd.feetOver[i];
				}
			}

			if (MOD(scenario,100)==0) {
				time( &cpustop );
				cpuelapsed = difftime(cpustop,cpustart);
				printf("%5u %6.0fs\n",scenario,cpuelapsed);
				fflush(stdout);
			}
		}
    }  /* End of while */
    iid.nscenario=scenario;
    printf("\n");

    time( &cpustop );
    cpuelapsed = difftime(cpustop,cpustart);
    printf("\nElapsed time for infrastructure impact analysis (seconds): %e\n",cpuelapsed);

	writePrimaryOutputFile(&iid,"pfot.txt");
	writePipeDiameterOutputFiles(&iid);
	freeIIAResultsData(iid.iiaResults,iid.pot->numThresh);
	FreeIAMemory(&iid);
	if(erd!=NULL) {
		ERD_close(&erd);
	} else {
		TSO_Close(&tso);
		TSO_ReleaseNetworkData(&nodeinfo,&linkinfo,iid.net);
		TSO_ReleaseNetworkInfo(&iid.net);
	}
    return(0);
}
void writePipeDiameterOutputFiles(PII_Data iid)
{
	int t;
	for(t=0;t<iid->pot->numThresh;t++) {
		char fn[256];
		sprintf(fn,"pfotByDiameter_%02d.txt",t);
		writePipeDiameterOutputFile(iid,fn,t,0);
		sprintf(fn,"pfotByDiameterPct_%02d.txt",t);
		writePipeDiameterOutputFile(iid,fn,t,1);
	}
}
void writePipeDiameterOutputFile(PII_Data iid, char *fn, int threshIdx, int pct)
{
	FILE *fp;
	PIIAResults t;
	int i;
	fp=fopen(fn,"w");
	fprintf(fp,"Threshold:\t%f\n",iid->pot->thresholds[threshIdx]);
	fprintf(fp,"SimID\tAll (%g ft)",iid->pot->potd.totalFeet);
	for(i=0;i<iid->pot->numDiameters;i++) {
		fprintf(fp,"\t%g in (%g ft)",iid->pot->potdDiam[i]->diameter,iid->pot->potdDiam[i]->potd.totalFeet);
	}
	fprintf(fp,"\n");
	for(t=iid->iiaResults;t!= NULL;t=t->next) {
		fprintf(fp,"%s",t->simID);
		fprintf(fp,"\t%f",(pct?t->pfot[threshIdx]/iid->pot->potd.totalFeet:t->pfot[threshIdx]));
		for(i=0;i<iid->pot->numDiameters;i++) {
			fprintf(fp,"\t%f",(pct?t->pfotd[threshIdx][i]/iid->pot->potdDiam[i]->potd.totalFeet:t->pfotd[threshIdx][i]));
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
}


void writePrimaryOutputFile(PII_Data iid, char *fn)
{
	FILE *fp;
	PIIAResults t;
	int i;
	fp=fopen(fn,"w");
	fprintf(fp,"SimID");
	for(i=0;i<iid->pot->numThresh;i++) {
		fprintf(fp,"\t%f",iid->pot->thresholds[i]);
	}
	fprintf(fp,"\n");
	for(t=iid->iiaResults;t!= NULL;t=t->next) {
		fprintf(fp,"%s",t->simID);
		for(i=0;i<iid->pot->numThresh;i++) {
			fprintf(fp,"\t%f",t->pfot[i]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
}

int getSpecieIndex(char *specieName,PII_Data iid)
{
	iid->speciesName=(char *)calloc(strlen(specieName)+1,sizeof(char));
	strcpy(iid->speciesName,specieName);
	iid->speciesIndex=ERD_getSpeciesIndex(iid->erd,specieName);
	return 1;
}
int processOptions(char **argv,int nargs,PII_Data iid)
{
	int i;
	for(i=0;i<nargs;) {
		if(strcmp(argv[i],"-specie")==0) {
			if(!getSpecieIndex(argv[i+1],iid)) {
				return 0;
			}
			i+=2;
		} else if(strcmp(argv[i],"-thresholds")==0) {
			initThresholds(iid,&argv[i+1],nargs-i-1);
			i+=nargs-i;
		}
	}
	if(iid->speciesName==NULL) {
		iid->speciesName=(char *)calloc(4,sizeof(char));
		strcpy(iid->speciesName,"N/S");
		iid->speciesIndex=0;
	}
	return 1;
}

