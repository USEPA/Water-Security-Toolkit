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
#include <memory.h>
#include "spline_if.h"
#include "teva-assess.h"
#include "runif_if.h"

#include <stdlib.h>

char *VolumeModeDesc[]={"VM_MEAN","VM_RANDOM","VM_DEMAND"};
char *VolumeModeAbbr[]={"M","R","D"};
char *IngestionModeDesc[]={"IM_ATUS","IM_RANDOM","IM_FIXED5","IM_FIXED2","IM_DEMAND","IM_USERFIXED"};
char *IngestionModeAbbr[]={"A","R","5","2","D","U"};

SplineDataPtr vIngestSetup();
SplineDataPtr *ingestTimesSetup();
void ingestVolume(IngestionDataPtr idp, IndividualIngestionDataPtr *iidp,PNode node, PNet net,int pop, SplineDataPtr g, int volMode, float phi);
void ingestTimes(int pop, IndividualIngestionDataPtr *idp[], float stepsize,int simStartHour, SplineDataPtr *invcdf, int times, int volMode, float *utimes, int nutimes);
void adjustTimes(double *times, int ntimes,int simStartHour,float stepsize);

LIBEXPORT(void) initializeIngestionData(IngestionDataPtr p, PNet net, PNode nodes, PDR dr, PopulationDataPtr pd)
{
	int i;
	int maxPop=0;

	int nsteps=net->info->numSteps;
	printf("TimingMode: %s  VolumeMode: %s\n",IngestionModeDesc[p->timingMode],VolumeModeDesc[p->volumeMode]);
	fflush(stdout);
	p->numNodes=net->info->numNodes;
	for(i=0;i<p->numNodes;i++) {
		int pop=(int)nodes[i].pop;
		if(pop>maxPop) maxPop=pop;
	}
	if(dr != NULL) {
		p->maxsteps=dr->maxsteps;
	} else {
		p->maxsteps=net->info->numSteps;
	}
	p->nsteps=net->info->numSteps;
	p->nodeDoses=(float **)calloc(maxPop+1,sizeof(float*));
	for(i=0;i<maxPop;i++) {
		p->nodeDoses[i]=(float *)calloc(p->maxsteps,sizeof(float));
	}
	p->nodeDoses[maxPop]=NULL;

	p->ingestionData = (IndividualIngestionDataPtr**) calloc(p->numNodes,sizeof(IndividualIngestionDataPtr*));
	if(p->volumeMode==VMRandom) {
		p->fVol = vIngestSetup();
	} else {
		p->fVol = NULL;
	}
	if(p->timingMode==IMATUS) {
		p->fTiming=ingestTimesSetup();
	} else {
		p->fTiming=NULL;
	}
	if(p->volumeMode==VMDemand) {
		IngestionRate(net,nodes);
	}
	for(i=0;i<p->numNodes;i++) {
		int pi;
		ingestTimes((int)nodes[i].pop,&(p->ingestionData[i]),net->info->stepSize,net->info->simStart/3600,p->fTiming,p->timingMode,p->volumeMode,p->userTimes,p->numUserTimes);
		ingestVolume(p,p->ingestionData[i],&nodes[i],net,(int)nodes[i].pop,p->fVol,p->volumeMode,p->phi);
		if(p->timingMode==IMATUS|| p->timingMode==IMRandom) {
			for(pi=0;pi<nodes[i].pop;pi++) {
				p->ingestionData[i][pi]->response=(float)runif();
			}
		}
	}
}

LIBEXPORT(float *) createUserFixedTimesArray(IngestionDataPtr ing, float *inputTimes, int numTimes)
{
	ing->numUserTimes=numTimes;
	ing->userTimes=(float*)calloc(ing->numUserTimes,sizeof(float));
	memcpy(ing->userTimes,inputTimes,ing->numUserTimes*sizeof(float));
}

SplineDataPtr vIngestSetup()
{
	double wi[]={0,9,65,130,355,785,1375,2069,2600,4273};
	double p[] ={0,1,5,10,25,50,75,90,95,99};
	SplineDataPtr g=init_spline(3,10,p,wi);

	return g;
}


SplineDataPtr *ingestTimesSetup()
{
	SplineDataPtr *invcdf=(SplineDataPtr*)calloc(12,sizeof(SplineDataPtr));
	double P3[]={0.5, 0.75, 1, 1.75, 2.5, 3.75, 5, 7.5, 10, 15, 20, 25, 30, 
		          35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 92.5, 95,
				  96.25, 97.5, 98.25, 99, 99.25, 99.5};

	// quantiles for meal 1

	double ST1[] ={4.07732292327533, 4.22123375072951, 4.35685327753238,
				   4.67762902764424, 4.88119117182683, 5.12403651465802,
				   5.32702463772763, 5.63965743000573, 5.86089249060754,
				   6.18137556214991, 6.45647838187913, 6.6937823230358,
				   6.89852251901101, 7.08210205500141, 7.26918805345663,
				   7.46887461424356, 7.6810681283296, 7.89674349146262,
				   8.108326536353, 8.36000862089145, 8.67527335085119,
				   8.9870421310423, 9.33782796827557, 9.9244593221242,
				   10.8040001835011, 11.4853185132876, 12.0769047227513,
				   12.3845014380221, 12.8758509308402,13.3934661714434,
				   14.3475387278110, 14.9787163296501, 15.7964436663339};

	// quantiles for meal 2

	double ST21[] = {5.9056,6.0154,6.1136,6.4616,6.8855,7.6576,8.2159,9.2234,9.9549,10.7024,11.0468,11.31,11.5271,11.6921,11.8151,11.9128,11.9988,12.0822,12.1707,12.2735,12.4032,12.5766,12.8136,13.0855,13.5041,13.9032,14.5992,15.3005,16.7452,17.3506,18.1949,18.6386,18.9542};
	double ST22[] = {7.7178,8.0197,8.2785,8.8808,9.2452,9.9522,10.3156,10.8328,11.0823,11.3812,11.5565,11.7098,11.8346,11.9284,12.0081,12.0857,12.171,12.2749,12.402,12.5433,12.7161,12.9254,13.1318,13.4515,14.0395,14.4082,15.2085,15.7463,16.794,17.4741,18.2843,18.6583,19.2227};
	double ST23[] = {9.4066,9.6453,9.8112,10.187,10.4659,10.7597,10.9427,11.1904,11.3699,11.6234,11.7986,11.9381,12.0636,12.1873,12.3176,12.4612,12.6222,12.8008,12.9941,13.2091,13.4677,13.7812,14.1443,14.6587,15.5641,16.1885,17.1038,17.5372,17.9616,18.2343,18.7292,19.0702,19.6929};
	double ST24[] = {10.5638,10.729,10.8493,11.0946,11.2626,11.47,11.6321,11.8915,12.1068,12.4765,12.8051,13.1136,13.4177,13.7333,14.0752,14.4498,14.8474,15.2525,15.662,16.0845,16.5297,17.0047,17.5194,18.093,18.7249,19.071,19.4851,19.7594,20.1645,20.6158,21.8539,22.4096,22.8775};
	double ST25[] = {12.1829,12.3789,12.5361,12.9037,13.2008,13.6171,13.9466,14.4082,14.7368,15.2514,15.7002,16.1262,16.5317,16.9012,17.2271,17.5172,17.7846,18.0419,18.3015,18.5773,18.8893,19.2666,19.7374,20.2921,20.9364,21.3422,21.9038,22.2828,22.7278,23.0381,23.4297,23.6012,23.8175};

	// quantiles for meal 3

	double ST31[]={9.4879,9.903,10.1651,10.6748,11,11.3389,11.5723,11.938,12.2851,13.2814,15.076,15.8365,16.2886,16.6274,16.8984,17.1327,17.3471,17.5492,17.7429,17.9329,18.1264,18.3353,18.5811,18.9112,19.432,19.8082,20.3305,20.7116,21.2142,21.6177,22.2317,22.5423,22.937};
	double ST32[]={14.1035,14.4269,14.7097,15.1715,15.5424,15.8961,16.1163,16.4415,16.6688,16.9458,17.1424,17.3259,17.498,17.6573,17.8052,17.9426,18.0771,18.2206,18.3803,18.5553,18.7502,18.9645,19.2035,19.5237,19.9668,20.2476,20.6327,20.916,21.2786,21.5396,22.0119,22.2973,22.6939};
	double ST33[]={15.0904,15.603,15.8268,16.1439,16.3402,16.5688,16.7232,16.9383,17.1084,17.4033,17.6446,17.8411,18.0127,18.175,18.3362,18.4988,18.665,18.8394,19.0269,19.2364,19.4821,19.7564,20.0383,20.3634,20.8028,21.0874,21.4946,21.8684,22.4018,22.9699,23.8417,24.099,24.4606};
	double ST34[]={15.9624,16.1421,16.2795,16.5612,16.7447,16.9563,17.1115,17.346,17.532,17.8437,18.1232,18.3914,18.6557,18.9203,19.1885,19.4607,19.7341,20.0062,20.28,20.5636,20.8671,21.2005,21.5811,22.0501,22.6866,23.1155,23.8551,25.1375,25.8734,26.2971,26.8739,27.073,27.2791};
	double ST35[]={17.2042,17.3591,17.4783,17.7373,17.9254,18.1669,18.361,18.6768,18.9406,19.3847,19.7554,20.0775,20.3738,20.662,20.9532,21.2506,21.5527,21.8604,22.1833,22.5417,22.9712,23.5424,24.4024,25.2711,26.0337,26.4785,26.956,27.1978,27.4573,27.6377,27.8652,27.9632,28.0842};

	invcdf[ 0] = init_spline(3,33,P3,ST1);
	invcdf[ 1] = init_spline(3,33,P3,ST21);
	invcdf[ 2] = init_spline(3,33,P3,ST22);
	invcdf[ 3] = init_spline(3,33,P3,ST23);
	invcdf[ 4] = init_spline(3,33,P3,ST24);
	invcdf[ 5] = init_spline(3,33,P3,ST25);
	invcdf[ 6] = init_spline(3,33,P3,ST31);
	invcdf[ 7] = init_spline(3,33,P3,ST32);
	invcdf[ 8] = init_spline(3,33,P3,ST33);
	invcdf[ 9] = init_spline(3,33,P3,ST34);
	invcdf[10] = init_spline(3,33,P3,ST35);
	invcdf[11] = NULL;

	return invcdf;
}
static int timecmp(const void *a, const void *b)
{
	double v = *((double*)a)-*((double*)b);
	return v < 0 ? -1 : (v > 0 ? 1 : 0);
}
void ingestVolume(IngestionDataPtr idp, IndividualIngestionDataPtr *iidp,PNode node, PNet net,int pop, SplineDataPtr g, int volMode, float phi)
{
	int i;
	int timingMode=idp->timingMode;

	if(pop==0) return;
	if(volMode==VMMean) {
		if(timingMode==IMFixed5 || timingMode==IMFixed2 || timingMode==IMUserFixed) pop=1;
		for(i=0;i<pop;i++) {
			if(iidp[i] != NULL)
				iidp[i]->v.volume=idp->meanVolume;
		}
	} else if (volMode==VMRandom) {
//		if(timingMode==IMFixed5 || timingMode==IMFixed2) pop=1;
		for(i=0;i<pop;i++) {
			if(iidp[i] != NULL)
				iidp[i]->v.volume=eval_spline(g,runif()*100)/1000.0;
		}
	} else if (volMode == VMDemand) {
		int nsteps=net->info->numSteps;
		float *rho=node->rho;
//		float phi=dr->phi;
		float stepsize=net->info->stepSize;
		float *v;
		v=iidp[0]->v.volumes=(float *)calloc(net->info->numSteps,sizeof(float));
		for(i=0;i<nsteps;i++) {
			// rho * phi * stepsize
			v[i]=rho[i]*phi*stepsize;
		}
	}
}
void ingestTimes(int pop, IndividualIngestionDataPtr *idp[], float stepsize, int simStartHour, SplineDataPtr *invcdf, int times,int volMode, float *utimes, int nutimes)
{
	int i;

	if(pop==0) return;
	if(times==IMFixed5 || times==IMFixed2 || times==IMUserFixed) {
		int tPop;
		double *itimes;
		int ntimes;
		if(times==IMFixed2) {
			double tt[]={12.0,21.0,-1,-1,-1};
			ntimes=2;
			itimes=(double*)calloc(ntimes,sizeof(double));
			memcpy(itimes,tt,ntimes*sizeof(double));
		} else if (times==IMFixed5) {
			double tt[]={7.0,9.5,12,15,18};
			ntimes=5;
			itimes=(double*)calloc(ntimes,sizeof(double));
			memcpy(itimes,tt,ntimes*sizeof(double));
		} else if (times==IMUserFixed) {
			int ti;
			ntimes=nutimes;
			itimes=(double*)calloc(ntimes,sizeof(double));
			// utimes are floats and itimes are doubles - can't memcpy
			for(ti=0;ti<ntimes;ti++) {
				itimes[ti]=utimes[ti];
			}
		}
		if(volMode==VMRandom) {
			tPop=pop;
		} else {
			tPop=1;
		}
		*idp = (IndividualIngestionDataPtr*)calloc(tPop,sizeof(IndividualIngestionDataPtr));
		adjustTimes(itimes,ntimes,simStartHour,stepsize);
		for(i=0;i<tPop;i++) {
			IndividualIngestionDataPtr p = (IndividualIngestionDataPtr)calloc(1,sizeof(IndividualIngestionData));
			p->ntimes=ntimes;
			p->times=calloc(p->ntimes,sizeof(double));
			memcpy(p->times,itimes,p->ntimes*sizeof(double));
			(*idp)[i]=p;
		}
		free(itimes);
	} else if(times==IMRandom) {
		*idp = (IndividualIngestionDataPtr*)calloc(pop,sizeof(IndividualIngestionDataPtr));
		for(i=0;i<pop;i++) {
			int j;
			IndividualIngestionDataPtr p = (IndividualIngestionDataPtr)calloc(1,sizeof(IndividualIngestionData));
			(*idp)[i]=p;
			p->ntimes=5;
			for(j=0;j<p->ntimes;j++) {
				p->times[j] = (((int)(runif()*240))/10.0);
			}
			qsort(p->times,p->ntimes,sizeof(double),timecmp);
			adjustTimes(p->times,p->ntimes,simStartHour,stepsize);
		}
	} else if(times==IMATUS) { 
		*idp = (IndividualIngestionDataPtr*)calloc(pop,sizeof(IndividualIngestionDataPtr));
		for(i=0;i<pop;i++) {
			IndividualIngestionDataPtr p = (IndividualIngestionDataPtr)calloc(1,sizeof(IndividualIngestionData));
			SplineDataPtr sdp=NULL;
			double t0,t2,t4;
			int j;
			int needToSort=0;
			(*idp)[i]=p;
			p->ntimes=5;
			p->times=(double*)calloc(p->ntimes,sizeof(double));
			// set breakfast time
			p->times[0]=eval_spline(invcdf[0],runif()*100);

			// set lunch time (based on when breakfast was...)
			t0=p->times[0];
			if(t0 < 6) sdp=invcdf[1];
			else if(t0 <  8) sdp=invcdf[2];
			else if(t0 < 10) sdp=invcdf[3];
			else if(t0 < 12) sdp=invcdf[4];
			else if(t0 < 28) sdp=invcdf[5];
			t2=eval_spline(sdp,runif()*100);
			while(t2-t0 < 0) {
				t2=eval_spline(sdp,runif()*100);
			}
			p->times[1]=(t2+t0)/2;
			p->times[2]=t2;

			// set dinner time (based on when lunch was...)
			t2=p->times[2];
			if(t2 < 11) sdp=invcdf[6];
			else if(t2 < 13) sdp=invcdf[7];
			else if(t2 < 15) sdp=invcdf[8];
			else if(t2 < 17) sdp=invcdf[9];
			else if(t2 < 28) sdp=invcdf[10];
			t4=eval_spline(sdp,runif()*100);
			while(t4-t2 < 0) {
				t4=eval_spline(sdp,runif()*100);
			}
			p->times[3]=(t4+t2)/2;
			p->times[4]=t4;

			needToSort=0;
			for(j=4;j>=0;j--) {
				if(p->times[j] >= 24) {
					needToSort=1; p->times[j]-=24;
				}
			}
			if(needToSort)
				qsort(p->times,5,sizeof(double),timecmp);
			adjustTimes(p->times,5,simStartHour,stepsize);


		}
	} else if(times==IMDemand) {
		IndividualIngestionDataPtr p = (IndividualIngestionDataPtr)calloc(1,sizeof(IndividualIngestionData));
		*idp = (IndividualIngestionDataPtr*)calloc(1,sizeof(IndividualIngestionDataPtr));
		(*idp)[0]=p;
	}
}
void adjustTimes(double *times, int ntimes,int simStartHour,float stepsize) {
	int i;
	for(i=0;i<ntimes;i++) {
		times[i]=times[i]-simStartHour;
		if(times[i]<0) times[i]+=24;
		times[i]/=stepsize;
	}
	qsort(times,ntimes,sizeof(double),timecmp);
}

LIBEXPORT(void) freeIngestionData(IngestionDataPtr idp, PNode nodes) {
	int i,j,nodePop;
	float **nd;

	if(idp==NULL) return;

	nd=idp->nodeDoses;
	while(*nd != NULL) {
		free(*nd);
		nd++;
	}
	free(idp->nodeDoses);
	if(idp->timingMode==IMUserFixed) {
		free(idp->userTimes);
	}
	for(i=0;i<idp->numNodes;i++) {
		IndividualIngestionDataPtr *iidpp;
		iidpp=idp->ingestionData[i];
		if(iidpp != NULL) {
			if(idp->timingMode==IMDemand || ((idp->timingMode==IMFixed5 || idp->timingMode==IMFixed2 || idp->timingMode==IMUserFixed) && idp->volumeMode==VMMean)) {
				nodePop=1;
			} else {
				nodePop=(int)nodes[i].pop;
			}
			for(j=0;j<nodePop;j++) {
				if(idp->volumeMode==VMDemand)
					free(iidpp[j]->v.volumes);
				if(iidpp[j]->times!=NULL)
					free(iidpp[j]->times);
				free(iidpp[j]);
			}
			free(iidpp);
		}
	}
	free(idp->ingestionData);
	if(idp->fVol != NULL)
		free_spline(idp->fVol);
	if(idp->fTiming != NULL) {
		SplineDataPtr *sdp = idp->fTiming;
		while(*sdp != NULL)  {
			free_spline(*sdp);
			sdp++;
		}
		free(idp->fTiming);
	}
}
