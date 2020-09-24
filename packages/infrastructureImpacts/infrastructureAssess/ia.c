/* System Headers */
//#include <stdlib.h>
//#include <ctype.h>
//#include <stddef.h>
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <time.h>
//#include <limits.h>
//#include <float.h>

/* TEVA headers */
#include "tevautil.h"
#include "infrstr_assess.h"
int findPOTIndex(float diameter, PPipeOverThreshDataByDiameter *potdiam, int imin, int imax);
PPipeOverThreshData getPOTDiam(float diameter, PPipeOverThresh pot);

/*************************************************************************/
/*
**-----------------------------------------------------------
**  InitializeAssessMemory
**  Input:   pointers to data items
**  Output:  data for health impact assessment
**  Purpose: Initializes memory needed for health impact assessment
**-----------------------------------------------------------
*/
LIBEXPORT(void) InitializeIAMemory(PII_Data iid)
{
    int   
        nnodes,
        nlinks,
        ntanks,
        njunctions,
        nsteps;
    float 
        stepsize,
        fltmax;
	PNetInfo netinfo;

	netinfo=iid->net;
	nnodes = netinfo->numNodes;
	nsteps = netinfo->numSteps;
	nlinks = netinfo->numLinks;
	fltmax = netinfo->fltMax;
	njunctions = netinfo->numJunctions;
	ntanks = netinfo->numTanks;
	stepsize = netinfo->stepSize;

    /* Allocate memory depending on network size */
	iid->speciesName=NULL;
    iid->nscenario=0;
}
LIBEXPORT(void) FreeIAMemory(PII_Data iid)
{
	FreePFOTData(&iid->pot);
}
LIBEXPORT(PIIAResults) allocateIIAResultsData(char *simID, PII_Data iid) {
	int i;
	PIIAResults p = (PIIAResults)calloc(1,sizeof(IIAResults));
	p->simID = (char *)calloc(strlen(simID)+1,sizeof(char));
	strcpy(p->simID,simID);
	p->pfot=(float *)calloc(iid->pot->numThresh,sizeof(int));
	p->pfotd=(float**)calloc(iid->pot->numThresh,sizeof(float*));
	for(i=0;i<iid->pot->numThresh;i++) {
		p->pfotd[i]=(float *)calloc(iid->pot->numDiameters,sizeof(float));
	}
	return p;
}

LIBEXPORT(void) freeIIAResultsData(PIIAResults hist, int numThresh) {
	PIIAResults t=hist;
	while(t!=NULL) {
		int i;
		PIIAResults next=t->next;
		free(t->simID);
		free(t->pfot);
		for(i=0;i<numThresh;i++) {
			free(t->pfotd[i]);
		}
		free(t->pfotd);
		free(t);
		t=next;
	}
}
LIBEXPORT(void) resetPipeFeetOverThreshold(PII_Data iid) {
	int i,j,l,nlinks;
	PPipeOverThresh pot=iid->pot;
	PPipeOverThreshData potd=&pot->potd;
	PPipeOverThreshDataByDiameter *potDiam=pot->potdDiam;

	int numThresh=pot->numThresh;

	nlinks=iid->net->numLinks;

	for(i=0;i<numThresh;i++) {
		potd->feetOver[i]=0;
		potd->linksOver[i]=0;
		for(l=0;l<nlinks;l++) {
			pot->pipeFeetOver[i][l]=0;
		}
		for(j=0;j<pot->numDiameters;j++) {
			potDiam[j]->potd.feetOver[i]=0;
			potDiam[j]->potd.linksOver[i]=0;
		}
	}
}
LIBEXPORT(void) pipesContaminated(PII_Data iid) {
    int l,t,th;
	int nsteps=iid->net->numSteps;
	int nlinks=iid->net->numLinks;
	PLinkInfo linkInfo=iid->linkinfo;
	float **c=iid->net->qualResults->nodeC[iid->speciesIndex];
	float **flow=iid->net->hydResults->flow;
	PPipeOverThresh pot=iid->pot;
	PPipeOverThreshData potd=&pot->potd;
	PPipeOverThreshDataByDiameter *potDiam=pot->potdDiam;
	PPipeOverThreshData potdd;
	int lcMethod=1;
	int calcTotal=potd->totalFeet==0;

	for(l=0;l<nlinks;l++) {
		PLinkInfo li=&linkInfo[l];
    	for(t=0;t<nsteps;t++) {
			int iflow;
			float lc=0;
			float lflow=flow[t][l];

			if(lcMethod==0) {
				if(lflow > 0) {
					// use from node concentration
					lc=c[li->from-1][t];
					iflow=1;
				} else if(lflow < 0) {
					// use to node concentration
					lc=c[li->to-1][t];
					iflow=-1;
				} else {
					// concentration = 0;
					lc=0;
					iflow=0;
				}
			} else {
				iflow=0;
				lc=(c[li->from-1][t]+c[li->to-1][t])/2;
			}
			if(lc>pot->minThreshold) {
				for(th=0;th<pot->numThresh;th++) {
	    			if(lc > pot->thresholds[th]) {
						pot->pipeFeetOver[th][l]=li->length;
	    			}
				}
			}
		}
    }
	for(l=0;l<nlinks;l++) {
		PLinkInfo li=&linkInfo[l];
		if(li->length>0) {
			potdd=getPOTDiam(li->diameter,pot);
			for(th=0;th<pot->numThresh;th++) {
				float pfot=pot->pipeFeetOver[th][l];
				if(pfot>0) {
					potd->feetOver[th] += pfot;
					potd->linksOver[th]++;
					potdd->feetOver[th] += pfot;
					potdd->linksOver[th]++;
				}
			}
			if(calcTotal) {
				potd->totalFeet+=li->length;
				potdd->totalFeet+=li->length;
			}
		}
	}
}
int findPOTIndex(float diameter, PPipeOverThreshDataByDiameter *potdiam, int imin, int imax) {
	int imid;
	if(imax<imin) {
		return -(imin+1);
	}
	imid=imin+((imax-imin)/2);
	if(potdiam[imid]->diameter>diameter) {
		return findPOTIndex(diameter,potdiam,imin,imid-1);
	} else if(potdiam[imid]->diameter<diameter) {
		return findPOTIndex(diameter,potdiam,imid+1,imax);
	} else {
		return imid;
	}
}

LIBEXPORT(PPipeOverThreshDataByDiameter) allocatePipeOverThreshDataByDiameter(float diameter, int numThresh)
{
	PPipeOverThreshDataByDiameter potdd=(PPipeOverThreshDataByDiameter)calloc(1,sizeof(PipeOverThreshDataByDiameter));
	potdd->diameter=diameter;
	potdd->potd.feetOver=(float*)calloc(numThresh,sizeof(float));
	potdd->potd.linksOver=(int*)calloc(numThresh,sizeof(int));
	return potdd;
}

LIBEXPORT(void) addPOT(float diameter, PPipeOverThresh pot, int dIndex)
{
	int i;
	PPipeOverThreshDataByDiameter potdd=NULL;
	pot->potdDiam=(PPipeOverThreshDataByDiameter *)realloc(pot->potdDiam,(pot->numDiameters+1)*sizeof(PPipeOverThreshDataByDiameter));
	for(i=pot->numDiameters;i>dIndex;i--) {
		pot->potdDiam[i]=pot->potdDiam[i-1];
	}
	pot->potdDiam[dIndex]=allocatePipeOverThreshDataByDiameter(diameter,pot->numThresh);
	pot->numDiameters++;
}
PPipeOverThreshData getPOTDiam(float diameter, PPipeOverThresh pot)
{
	int dIndex;
	if(pot->potdDiam == NULL) {
		addPOT(diameter,pot,0);
		dIndex=0;
	} else {
		dIndex=findPOTIndex(diameter,pot->potdDiam,0,pot->numDiameters-1);
		if(dIndex<0) {
			dIndex=(-dIndex)-1;
			addPOT(diameter,pot,dIndex);
		}
	}
	return &pot->potdDiam[dIndex]->potd;
}

LIBEXPORT(PPipeOverThresh) AllocatePFOTData(int numThresh,int numLinks)
{
	int i;
	PPipeOverThresh pot=(PPipeOverThresh)calloc(1,sizeof(PipeOverThresh));
	pot->potd.feetOver=(float*)calloc(numThresh,sizeof(float));
	pot->potd.linksOver=(int*)calloc(numThresh,sizeof(int));
	pot->numThresh=numThresh;
	pot->thresholds=(float *)calloc(numThresh,sizeof(float));
	pot->threshIDs=(char**)calloc(numThresh,sizeof(char*));
	pot->pipeFeetOver=(float **)calloc(numThresh,sizeof(float*));
	for(i=0;i<numThresh;i++) {
		pot->threshIDs[i]=(char *)calloc(3,sizeof(char));
		sprintf(pot->threshIDs[i],"%02d",i);
		pot->pipeFeetOver[i]=(float *)calloc(numLinks,sizeof(float));
	}
	return pot;
}
LIBEXPORT(void) FreePFOTData(PPipeOverThresh *ppot) {
	int i;
	PPipeOverThresh pot=*ppot;
	free(pot->potd.feetOver);
	free(pot->potd.linksOver);
	free(pot->thresholds);
	for(i=0;i<pot->numThresh;i++) {
		free(pot->threshIDs[i]);
		free(pot->pipeFeetOver[i]);
	}
	free(pot->threshIDs);
	free(pot->pipeFeetOver);
	for(i=0;i<pot->numDiameters;i++) {
		free(pot->potdDiam[i]->potd.feetOver);
		free(pot->potdDiam[i]->potd.linksOver);
		free(pot->potdDiam[i]);
	}
	free(pot->potdDiam);
	*ppot=NULL;
}
