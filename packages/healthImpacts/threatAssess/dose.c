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
#include "teva-assess.h"
#include <stdlib.h>
#include <memory.h>

void timingBasedDose(IndividualIngestionDataPtr *idp, IngestionDataPtr ingData, int detectTime, PNode node, PNet net, PU u, FILE *fp, PDoseOverThresh dot, int nodeIdx,PDoseBinData dBins,int speciesIdx, float *avgDose);
void demandBasedDose(IndividualIngestionDataPtr idp, IngestionDataPtr ingData, int detectTime, PNode node, PNet net, PU u, FILE *fp, PDoseOverThresh dot, int dotSize,PDoseBinData dBins, int speciesIdx, float *avgDose);

LIBEXPORT(void) resetDoseOverThreshold(PMem mem) {
	int i;
	if(mem->dot==NULL) return;
	for(i=0;i<mem->dot->numThresh;i++) {
		mem->dot->totOver[i]=0;
		mem->dot->nodesOver[i]=0;
		memset(mem->dot->numOver[i],0,mem->net->info->numNodes*sizeof(int));
		memset(mem->dot->numOverByTime[i],0,mem->net->info->numSteps*sizeof(int));
	}
}

LIBEXPORT(void) resetDoseBinData(PMem mem) {
	if(mem->doseBins==NULL) return;
	memset(mem->doseBins->data,0,mem->doseBins->numBins*sizeof(int));
}

LIBEXPORT(void) Dose(IndividualIngestionDataPtr *idp, IngestionDataPtr ingData, PNode node, PNet net, int detectTime, PU u, PDoseOverThresh dot, int nodeidx, PDoseBinData dBins, int speciesIdx, float *avgDose)
{
	if((int)node->pop==0) return;
	if(ingData->timingMode == IMDemand) {
		demandBasedDose(idp[0],ingData,detectTime,node,net,u,NULL,dot,nodeidx,dBins,speciesIdx,avgDose);
	} else {
		timingBasedDose(idp,ingData,detectTime,node,net,u,NULL,dot,nodeidx,dBins,speciesIdx,avgDose);
	}
}
void demandBasedDose(IndividualIngestionDataPtr idp,IngestionDataPtr ingData, int detectTime,PNode node, PNet net, PU u,FILE *fp, PDoseOverThresh dot, int nodeidx,PDoseBinData dBins, int speciesIdx, float *avgDose)
{
    int i;
    float *dos=ingData->nodeDoses[0];
	float *v=idp->v.volumes;
    int maxSteps=ingData->maxsteps;
	float *c=net->info->qualResults->nodeC[speciesIdx][nodeidx];
    float maxDose;

	if(detectTime>ingData->nsteps) detectTime=ingData->nsteps;
    dos[0]=v[0]*c[0];
    for(i=1;i<detectTime;i++) { 
        float dose=v[i]*c[i];
        if(avgDose!=NULL) avgDose[i]=dose;
        dos[i]=dos[i-1]+dose;
    }
    for(i=detectTime;i<maxSteps;i++) {
      dos[i]=dos[detectTime-1];
    }
    if(dot != NULL) {
    	int j;
	    for(i=0;i<dot->numThresh;i++) {
	    	for(j=0;j<ingData->nsteps;j++) {
	    		if(dos[j] > 0 && dos[j] > dot->thresholds[i]) {
				  dot->numOverByTime[i][j]+=node->pop;
	    		}
			}
		}
    }
	maxDose=dos[maxSteps-1];
    if(dot != NULL && maxDose>0) {
	    for(i=0;i<dot->numThresh;i++) {
			if(maxDose > dot->thresholds[i]) {
				dot->numOver[i][nodeidx]=node->pop;
				dot->totOver[i] += node->pop;
				dot->nodesOver[i]++;
			}
		}
    }
    if(dBins != NULL && maxDose > 0) {
      // now do the histogram
      // bins[0]=0, data[0] contains value between bins[0] and bins[1]
      // data[1] contains value between bins[1] and bins[2]
      // data[numBins-1] contains value over bins[numBins-1];
      for(i=1;i<dBins->numBins && maxDose >= dBins->doseBins[i];i++);
      dBins->data[i-1] += node->pop;
    }
    memcpy(u->dos,dos,maxSteps*sizeof(float));

	if(fp!=NULL) {
		int di;
		int nsteps=ingData->nsteps;
		float **nodeDoses=ingData->nodeDoses;
		fprintf(fp,"Node: %s\t%s\t%s\n",node->info->id,IngestionModeDesc[ingData->timingMode],VolumeModeDesc[ingData->volumeMode]);

		fprintf(fp,"\n");

		fprintf(fp,"t\tConcentration\tv\tdose\n");

		for(di=0;di<nsteps;di++) {
			fprintf(fp,"%d\t%f\t%f\t%f\n",di,c[di],v[di],dos[di]);
		}
		for(;di<maxSteps;di++) {
			fprintf(fp,"%d\t\t\t%f\n",di,dos[di]);
		}
		fprintf(fp,"\n");
	}
}
void timingBasedDose(IndividualIngestionDataPtr *idp, IngestionDataPtr ingData, int detectTime, PNode node, PNet net, PU u, FILE *fp, PDoseOverThresh dot, int nodeidx,PDoseBinData dBins,int speciesIdx, float *avgDose)
{
	float portionSize;
	int i,di,j;
	int nstepsPerDay=(int)(24.0/net->info->stepSize);
	int nsteps = net->info->numSteps;
	int start=net->info->reportStart; // not sure if this is what I want here - need this to be the actual hour the sim starts (e.g. 6 AM)
	int nodePop;
	int tPop;
	float **nodeDoses=ingData->nodeDoses;
	float *udos=u->dos;
	float *c=net->info->qualResults->nodeC[speciesIdx][nodeidx];
	int offset,ti;
	int nIngestionTimes=ingData->timingMode==IMFixed2?2:(ingData->timingMode==IMFixed5?5:ingData->numUserTimes);
	int *dosedPop;
	int *popTimes;

	switch(ingData->timingMode) {
		case IMFixed2:
			nIngestionTimes=2;
			break;
		case IMFixed5:
			nIngestionTimes=5;
			break;
		case IMUserFixed:
			nIngestionTimes=ingData->numUserTimes;
			break;
		case IMATUS:
			nIngestionTimes=5;
			break;
		case IMRandom:
			nIngestionTimes=5;
			break;
	}
	if(avgDose != NULL) {
		dosedPop= (int *)calloc(nsteps,sizeof(int));
		popTimes= (int *)calloc(nsteps,sizeof(int));
		memset(avgDose,0,nsteps*sizeof(float));
	}
	memset(udos,0,ingData->maxsteps*sizeof(float));
	if((ingData->timingMode==IMFixed5 || ingData->timingMode==IMFixed2 || ingData->timingMode==IMUserFixed) && ingData->volumeMode==VMMean) {
		nodePop=1;
	  	tPop=node->pop;
	} else {
		nodePop=node->pop;
	  	tPop=1;
	}

	for(i=0;i<nodePop;i++) {
		IndividualIngestionDataPtr iidp=idp[i];
		double *times = iidp->times;
		float *dose;
		float maxDose;

		dose=nodeDoses[i];
		memset(dose,0,ingData->maxsteps*sizeof(float));
		ti=0;
		offset=0;
		portionSize=(float)(iidp->v.volume/nIngestionTimes);
		// first compute doses when the individual drinks...
		for(ti=0;ti<nIngestionTimes;ti++) {
			int t=(int)times[ti];
			while(t<nsteps && t<detectTime) {
				float thisDose=c[t] * portionSize;
				dose[t] += thisDose;
				if(avgDose != NULL) {
					popTimes[t]+=1;
					avgDose[t]+=thisDose;
					if(popTimes[t]==1) {
						// if this person didn't already receive a dose this time interval
						dosedPop[t]++;
					}
				}
				t+=nstepsPerDay;
			}
		}
		// then fill in the "gaps"
		udos[0]+=dose[0];
		for(di=1;di<ingData->maxsteps;di++) {
			if(dose[di]==0) dose[di]=dose[di-1];
			else dose[di]+=dose[di-1];
			udos[di]+=dose[di];
		}
		maxDose=dose[ingData->maxsteps-1];
		if(dot != NULL) {
		  for(di=0;di<ingData->nsteps;di++) {
			float tdose=dose[di];
			for(j=0;j<dot->numThresh;j++) {
			  if(tdose > 0 && tdose > dot->thresholds[j]) {
			    dot->numOverByTime[j][di]+=tPop;
			  }
			}
		  }

		  for(j=0;j<dot->numThresh;j++) {
		    if(maxDose > dot->thresholds[j]) {
		      dot->numOver[j][nodeidx]+=tPop;
		      dot->totOver[j] += tPop;
		    }
		  }
		}
		// now do the histogram
		if(dBins != NULL) {
		  for(di=1;di<dBins->numBins && maxDose >= dBins->doseBins[di];di++);
		  dBins->data[di-1]+=tPop;
        }
	}
	for(ti=0;ti<ingData->maxsteps;ti++) {
		udos[ti]/=nodePop;
	}
	if(dot != NULL) {
		for(j=0;j<dot->numThresh;j++) {
		  if(dot->numOver[j][nodeidx] > 0) {
			dot->nodesOver[j]++;
		  }
		}
	}
	if(avgDose != NULL) {
		if(nodePop>1) {
			for(i=0;i<nsteps;i++) {
				if(dosedPop[i] > 0) {
					avgDose[i] = avgDose[i]/dosedPop[i];
				}
			}
		}
		free(popTimes);
		free(dosedPop);
	}

	if(fp!=NULL) {
		fprintf(fp,"Node: %s\t%s\t%s\n",node->info->id,IngestionModeDesc[ingData->timingMode],VolumeModeDesc[ingData->volumeMode]);

		for(ti=0;ti<nIngestionTimes;ti++) {
			fprintf(fp,"\t");
			for(i=0;i<nodePop;i++) {
				fprintf(fp,"\t%d",(int)idp[i]->times[ti]);
			}
			fprintf(fp,"\n");
		}
		fprintf(fp,"\t");
		for(i=0;i<nodePop;i++) {
			fprintf(fp,"\t%f",idp[i]->v.volume);
		}
		fprintf(fp,"\n\n");

		fprintf(fp,"t\tConcentration");
		for(i=0;i<nodePop;i++) {
			fprintf(fp,"\t%d",i);
		}
		fprintf(fp,"\n");

		for(di=0;di<ingData->maxsteps;di++) {
			fprintf(fp,"%d\t%f",di,di<nsteps?c[di]:0);
			for(i=0;i<nodePop;i++) {
				fprintf(fp,"\t%f",ingData->nodeDoses[i][di]);
			}
			fprintf(fp,"\n");
		}
		fprintf(fp,"\n");
	}
}
