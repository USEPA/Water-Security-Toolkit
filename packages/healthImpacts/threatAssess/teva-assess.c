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
#include "teva-assess.h"
#include "ta_input.h"
#include "tai.h"
#include "sss.h"
#include "ta_utils.h"
#include "nrutil.h"
#include "runif_if.h"


void ApplyThreshold(PNode node,PNet net,float threshold, int speciesIndex);

/*
**--------------------------------------------------------------
** Module:  TEVA-ASSESS
** Usage:   teva-assess.exe <input.tai>
** Input:   
** Output:
** Purpose:
**---------------------------------------------------------------
*/
int main(int argc, char **argv)
{
    int
        i,num,s,
        dtime,
        dtmin,
        scenario;
    time_t
        cpustart,
        cpustop;
    double
        cpuelapsed;
	char *taiFile,
		*sssFile;

    /* Data model - see mem structure */
    Mem       mem;
    PIO       io;
    PTSO      tso;
    PERD      erd;
    PDR       dr;
	PopulationDataPtr pd;
    PNode     node;
    PLink     link;
	PNetInfo  netinfo;
    PNodeInfo nodeinfo;
    PLinkInfo linkinfo;
    PNet      net;
    PXA       xa;
	IngestionDataPtr ing;
	/** NEW  **/
	XA	interimXA;
	PHIST
		thisHist,
		prevHist;
	/** End NEW **/
    PTA       ta;
    PU        u;
    PSourceData source;
	PInputFileData taiIFD,sssIFD;

	memset(&mem,0,sizeof(Mem));
    time( &cpustart );

    if((argc!=2)&&
       (argc!=3))
      {
	printf("***Incorrect invocation\n");
	printf("***Correct usage is: teva-assess <tai-input-file> [sensor-file]\n");
	return 0;
      }

    printf("T H R E A T  E N S E M B L E  V U L N E R A B I L I T Y  A N A L Y S I S\n");
    printf("                               (T E V A)\n");
    printf("               H E A L T H  I M P A C T  A N A L Y Z E R\n");
    printf("                              Version 1.0\n\n");

	set_seed(cpustart);
	printf("Seed: %ld\n",cpustart);
	taiFile = argv[1];
	sssFile = argc==3?argv[2]:NULL;
	taiIFD = loadTAIFileData(taiFile);
	if(sssFile != NULL)
		sssIFD = loadSSSFileData(sssFile);
	// first try as erd, then tso
	mem.erd = OpenERDFile(taiIFD,"TSONAME",READ_DEMANDS | READ_QUALITY | READ_DEMANDPROFILES);
	if(mem.erd!=NULL) {
		netinfo=mem.erd->network;
		mem.nodeinfo=mem.erd->nodes;
		mem.linkinfo=mem.erd->links;
	} else {
		mem.tso = OpenTSOFile(taiIFD,"TSONAME");
		TSO_ReadPrologue(mem.tso,&netinfo,&mem.nodeinfo,&mem.linkinfo,READ_DEMANDS | READ_QUALITY);
	}
	//
	InitializeAssessMemory(&mem,netinfo);
	processTAIData(taiIFD,&mem);
	freeInputFileData(&taiIFD);
	if(sssFile != NULL) {
		processSensorSetData(sssIFD,&mem);
		freeInputFileData(&sssIFD);
	}
    /* Copy data model addresses from mem for convenience */
    io=mem.io;
    dr=mem.dr;
	pd=mem.popData;
    node=mem.node;
    link=mem.link;
    net=mem.net;
    xa=mem.xa;
    ta=mem.ta;
    u=mem.u;
	ing=mem.ingestionData;
	tso=mem.tso;
	erd=mem.erd;
	nodeinfo=mem.nodeinfo;
	linkinfo=mem.linkinfo;
	/** NEW **/
	prevHist = NULL;
	/** End NEW **/

    /* Estimate Population */
    Population(net,pd,node);

	initializeIngestionData(ing,net, node, dr,pd);
    /*****************************************************/
    /*  Health Impact analysis for existing sensors      */
    /*****************************************************/
    printf("Health Impact Analysis for Scenario..\n");
    scenario=0;
    initialize(net,dr,xa,ta);
	/** NEW **/
	initXA(&interimXA,dr->maxsteps);
	/** End NEW **/
	printf("Concentration Threshold: %f\n",mem.concentrationThreshold);
	num=get_count(erd,tso);
	for(s=0;s<num;s++) {
		if(loadSimulationResults(s,erd,tso,net->info,nodeinfo,&source)) {
			/** NEW **/
			char tstr[100];
			ApplyThreshold(node,net,mem.concentrationThreshold,dr->speciesIndex);

			resetXA(&interimXA,dr->maxsteps);
			/** End NEW **/

			scenario++;
			/* Sampling network detection time is minimum among all sensors */
			dtmin=INT_MAX;
			for(i=0;i<net->info->numNodes;i++) {
				if((node[i].sensor.type==existing || node[i].sensor.type==selected) && node[i].info->nz[0]) {
					if( (dtime=Detect(net,&node[i],i,dr->speciesIndex))<dtmin ) dtmin=dtime;
				}
			}
			/** NEW **/
			/* for each scenario read, enlarge size of histogram data structures */
			/* if (analyzed-1 > net->nnodes-1){
			MEMCHECK(hist = (PHIST)realloc(hist, (size_t) (analyzed+1)*sizeof(HIST)));  } */
			thisHist = (PHIST)calloc(1,sizeof(HIST));
			tstr[0]=0;
			for(i=0;i<source->nsources;i++) {
				if(i>0) strcat(tstr,",");
				strcat(tstr,node[source->source[i].sourceNodeIndex-1].info->id);
			}
			thisHist->simID = (char *)calloc(strlen(tstr)+1,sizeof(char));
			strcpy(thisHist->simID,tstr);
			if(prevHist==NULL) {
				mem.hist = thisHist;
			} else {
				prevHist->next=thisHist;
			}
			prevHist = thisHist;
			/** End NEW **/

			/* Time-dependent cumulative expected dose and disease response */
			resetDoseOverThreshold(&mem);
			resetDoseBinData(&mem);
			for(i=0;i<net->info->numNodes;i++) {
				int pop=node[i].pop;
				if(node[i].info->nz[0] && pop>0) {
					Dose(ing->ingestionData[i],ing,&node[i],net,dtmin,u,mem.dot,i,mem.doseBins,mem.dr->speciesIndex,NULL);
					Response(ing->ingestionData[i],ing,dr,&node[i],u);
					DiseaseRM(net,dr,u);
				}
				/** UPDATED **/
				Accumulate(i,node[i].info->nz[0],net,dr,node[i].pop,u,&interimXA,ta,thisHist); /* RM 11/9/04 add scenario remove ta*/
				/** End UPDATED **/
			}

			/** NEW **/
			AccumulateFinal(net,dr,xa,&interimXA);
			if(mem.dot) {
				thisHist->dot=(int *)calloc(mem.dot->numThresh,sizeof(int));
				for(i=0;i<mem.dot->numThresh;i++) {
					thisHist->dot[i]=mem.dot->totOver[i];
				}
			}
			if(mem.doseBins) {
				thisHist->doseBins=(int *)calloc(mem.doseBins->numBins,sizeof(int));
				for(i=0;i<mem.doseBins->numBins;i++) {
					thisHist->doseBins[i]=mem.doseBins->data[i];
				}
			}
			
			/** End NEW **/
			if (MOD(scenario,100)==0) {
				time( &cpustop );
				cpuelapsed = difftime(cpustop,cpustart);
				printf("%5u %6.0fs\n",scenario,cpuelapsed);
				fflush(stdout);
			}
		}
    }  /* End of while */
    net->nscenario=scenario;
    printf("\n");

    /* Simple ASCII Output          */
    /* Ensemble Averages */
    for(i=0;i<dr->maxsteps;i++) {
        xa->dos[i]/=(float)net->nscenario;
        xa->res[i]/=(float)net->nscenario;
        xa->s[i]/=(float)net->nscenario;
        xa->i[i]/=(float)net->nscenario;
        xa->d[i]/=(float)net->nscenario;
        xa->f[i]/=(float)net->nscenario;
    }
    for(i=0;i<net->info->numNodes;i++) {
        ta->dos[i]/=(float)net->nscenario;
        ta->res[i]/=(float)net->nscenario;
        ta->s[i]/=(float)net->nscenario;
        ta->i[i]/=(float)net->nscenario;
        ta->d[i]/=(float)net->nscenario;
        ta->f[i]/=(float)net->nscenario;
    }
	if(mem.io->taoname[0] != '\0') {
		PHIST p;
	    FILEOPEN(mem.io->taostream = fopen(mem.io->taoname, "wt"));
	    fprintf(io->taostream,"%d\n",net->info->numNodes);
		fprintf(io->taostream,"%d\n",dr->maxsteps);
		fprintf(io->taostream,"%d\n",net->nscenario);

		for(i=0;i<dr->maxsteps;i++) fprintf(io->taostream,"%e ",xa->s[i]);
		fprintf(io->taostream,"\n");
		for(i=0;i<dr->maxsteps;i++) fprintf(io->taostream,"%e ",xa->i[i]);
		fprintf(io->taostream,"\n");
		for(i=0;i<dr->maxsteps;i++) fprintf(io->taostream,"%e ",xa->d[i]);
		fprintf(io->taostream,"\n");
		for(i=0;i<dr->maxsteps;i++) fprintf(io->taostream,"%e ",xa->f[i]);
		fprintf(io->taostream,"\n");
		for(i=0;i<dr->maxsteps;i++) fprintf(io->taostream,"%e ",xa->dos[i]);
		fprintf(io->taostream,"\n");
		for(i=0;i<dr->maxsteps;i++) fprintf(io->taostream,"%e ",xa->res[i]);
		fprintf(io->taostream,"\n");
		/** MODIFIED **/
		for(i=0;i<net->info->numNodes;i++) {
			fprintf(io->taostream,"%d\t%s\t%e\t%e\t%e\t%e\t%e\t%e\n",
				i,mem.node[i].info->id,ta->s[i],ta->i[i],ta->d[i],ta->f[i],ta->dos[i],ta->res[i]);
		}
		/** End MODIFIED **/
		/** NEW **/
		fprintf(io->taostream,"Scenario Results\n");

		fprintf(io->taostream,"ID\tFatalities\tDosage\tResponse");
		if(mem.dot) {
			fprintf(io->taostream,"\t");
			for(i=0;i<mem.dot->numThresh;i++){
				fprintf(io->taostream,"\tDoseOver_%f",mem.dot->thresholds[i]);
			}
		}
		fprintf(io->taostream,"\n");
		for(p=mem.hist;p!=NULL;p=p->next) {
			fprintf(io->taostream,"%s\t%f\t%f\t%f",p->simID,p->fh,p->dosh,p->resh);
			if(mem.dot) {
				fprintf(io->taostream,"\t");
				for(i=0;i<mem.dot->numThresh;i++){
					fprintf(io->taostream,"\t%d",p->dot[i]);
				}
			}
			fprintf(io->taostream,"\n");
		}
		if(mem.doseBins) {
			PDoseBinData db=mem.doseBins;
			fprintf(io->taostream,"DoseHistogram\n");
			fprintf(io->taostream,"ID\tResponse");
			fprintf(io->taostream,"\t");
			if(db->responseBins) {
				for(i=1;i<db->numBins;i++) {
					fprintf(io->taostream,"\t(%.3f,%.3f]",db->responseBins[i-1],db->responseBins[i]);
				}
				fprintf(io->taostream,"\t>%.3f\n",db->responseBins[i-1]);
			}
			fprintf(io->taostream,"\t\tMin Dosage");
			for(i=0;i<db->numBins;i++) {
				fprintf(io->taostream,"\t%f",db->doseBins[i]);
			}
			fprintf(io->taostream,"\n");
			fprintf(io->taostream,"\t\tMax Dosage");
			for(i=1;i<db->numBins;i++) {
				fprintf(io->taostream,"\t%f",db->doseBins[i]);
			}
			fprintf(io->taostream,"\n");
			for(p=mem.hist;p!=NULL;p=p->next) {
				fprintf(io->taostream,"%s\t%f\t",p->simID,p->resh);
				for(i=0;i<db->numBins;i++) {
					fprintf(io->taostream,"\t%d",p->doseBins[i]);
				}
				fprintf(io->taostream,"\n");
			}		
		}
		
		fclose(io->taostream);
		/** Wnd NEW **/
	}
    time( &cpustop );
    cpuelapsed = difftime(cpustop,cpustart);
    printf("\nElapsed time for sensor location / health impact analysis (seconds): %e\n",cpuelapsed);
    printf("T E V A  Normal  Termnation\n");

	freeXA(&interimXA,dr->maxsteps);
	FreeAssessMemory(&mem);
	freeHistogramData(mem.hist);
	if(erd!=NULL) {
		ERD_close(&erd);
	} else {
		TSO_Close(&tso);
		TSO_ReleaseNetworkData(&nodeinfo,&linkinfo,netinfo);
		TSO_ReleaseNetworkInfo(&netinfo);
	}
    return(0);
}

void ApplyThreshold(PNode node,PNet net,float threshold, int speciesIndex)
{
	int nnodes = net->info->numNodes;
	int i,t;
    for(i=0;i<nnodes;i++) {
		float *c = net->info->qualResults->nodeC[speciesIndex][i];
		int nz=0;
		for(t=0;t<net->info->numSteps;t++) {
			if(c[t] < threshold) c[t]=0;
			if(c[t]>0) nz=1;
		}
		node[i].info->nz[0]=nz;
    }
}
