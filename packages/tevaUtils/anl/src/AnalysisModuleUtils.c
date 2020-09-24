#include "AnalysisModuleUtils.h"

LIBEXPORT(int) initializeNetwork(void *simResultsRef, int speciesIndex,
								 PNetInfo *pnet, PNodeInfo *pnodeinfo, PLinkInfo *plinkinfo)
{
	int 
		i,
		nnodes,
		nlinks,
		ntanks,
		njunctions,
		nsteps,
		simStart,
		nspecies;
	int *tankIndices; /* used when building nodes */
	float
		stepsize;

	PNetInfo net;
	PNodeInfo nodeinfo;
	PLinkInfo linkinfo;
	char **nodeIDs;
	char **linkIDs;
	int *linkStarts;
	int *linkEnds;
	float *linkLengths;
	float *linkDiameters;

	if(getInt(simResultsRef, "numTimeSteps", &nsteps)==ND_FAILURE) return 0;
	if(getInt(simResultsRef, "numNodes", &nnodes)==ND_FAILURE) return 0;
	if(getInt(simResultsRef, "numLinks", &nlinks)==ND_FAILURE) return 0;
	if(getInt(simResultsRef, "numJunctions", &njunctions)==ND_FAILURE) return 0;
	if(getInt(simResultsRef, "simStart", &simStart)==ND_FAILURE) return 0;
	if(getFloat(simResultsRef, "stepSize", &stepsize)==ND_FAILURE) return 0;
	if(getInt(simResultsRef, "numSpecies", &nspecies)==ND_FAILURE) return 0;

	/** network parameters */
	net = (PNetInfo)calloc(1,sizeof(NetInfo));
	if(pnet != NULL) *pnet=net;
	net->numJunctions=njunctions;
	net->numLinks=nlinks;
	net->numNodes=nnodes;
	net->numSteps=nsteps;
	net->stepSize=stepsize;
	net->simStart=simStart;
	net->numSpecies=nspecies;
	net->qualResults=(PQualData)calloc(1,sizeof(QualData));
	net->qualResults->nodeC=(float ***)calloc(nspecies,sizeof(float **));
	net->qualResults->linkC=(float ***)calloc(nspecies,sizeof(float **));
	net->qualResults->nodeC[speciesIndex]=(float **)calloc(nnodes,sizeof(float*));
	for(i=0;i<nnodes;i++) {
		net->qualResults->nodeC[speciesIndex][i]=(float *)calloc(net->numSteps,sizeof(float));
	}
	net->hydResults = (PHydData)calloc(1,sizeof(HydData));


	nodeinfo=(PNodeInfo)calloc(nnodes,sizeof(NodeInfo));
	*pnodeinfo=nodeinfo;
	if(getStringArray(simResultsRef, "nodeIDs", &nodeIDs)==ND_FAILURE) return 0;
	for(i=0;i<nnodes;i++) {
		strcpy(nodeinfo[i].id,nodeIDs[i]);
		nodeinfo[i].nz = (int *)calloc(net->numSpecies,sizeof(int));
	}
	nd_freeStringArray(nodeIDs,nnodes);

	linkinfo=(PLinkInfo)calloc(nlinks,sizeof(LinkInfo));
	*plinkinfo=linkinfo;

	if(getStringArray(simResultsRef, "linkIDs", &linkIDs)==ND_FAILURE) return 0;
	if(getIntArray(simResultsRef, "linkStartNodeIdx", &linkStarts)==ND_FAILURE) return 0;
	if(getIntArray(simResultsRef, "linkEndNodeIdx",   &linkEnds)==ND_FAILURE) return 0;
	if(getFloatArray(simResultsRef, "linkLengths", &linkLengths)==ND_FAILURE) return 0;
	if(getFloatArray(simResultsRef, "linkDiameters", &linkDiameters)==ND_FAILURE) return 0;
	for(i=0;i<nlinks;i++) {
		linkinfo[i].from=linkStarts[i];
		linkinfo[i].to = linkEnds[i];
		linkinfo[i].length = linkLengths[i];
		linkinfo[i].diameter = linkDiameters[i];
		strcpy(linkinfo[i].id,linkIDs[i]);
		// ignore vertices - not supplied in named data struct anyway
	}
	nd_freeStringArray(linkIDs,nlinks);
	nd_free(linkStarts);
	nd_free(linkEnds);
	nd_free(linkLengths);
	{
		int hasDemand,hasFlow,hasVel;
		if(hasData(simResultsRef,"linkFlow",FLOAT_ARRAY_2D_TYPE,&hasFlow)==ND_FAILURE) {
			return 0;
		}
		if(hasData(simResultsRef,"linkVelocity",FLOAT_ARRAY_2D_TYPE,&hasVel)==ND_FAILURE) {
			return 0;
		}

		if(hasData(simResultsRef,"demands",FLOAT_ARRAY_2D_TYPE,&hasDemand)==ND_FAILURE) {
			return 0;
		}
		if(hasDemand) {
			float **q;
			int j;
			q=net->hydResults->demand=(float **)calloc(net->numSteps,sizeof(float *));
			for(j=0;j<net->numSteps;j++) {
				q[j]=(float*)calloc(nnodes,sizeof(float));
			}
		}
		if(hasFlow) {
			float **f;
			int j;
			f=net->hydResults->flow=(float **)calloc(net->numSteps,sizeof(float *));
			for(j=0;j<net->numSteps;j++) {
				f[j]=(float*)calloc(nlinks,sizeof(float));
			}
		}
		if(hasVel) {
			float **v;
			int j;
			v=net->hydResults->velocity=(float **)calloc(net->numSteps,sizeof(float *));
			for(j=0;j<net->numSteps;j++) {
				v[j]=(float*)calloc(nlinks,sizeof(float));
			}
		}
		for(i=0;i<nsteps;i++) {
			float *tres;
			int j;
			if(hasDemand) {
				float **q;
				q=net->hydResults->demand;
				/** IMPORTANT!!! Need to free these later via call to nd_free */
				if(getFloatArrayAtIndex(simResultsRef,"demands",i,&tres)==ND_FAILURE) {
					return 0;
				}
				for(j=0;j<nnodes;j++) {
					q[i][j] = tres[j];
				}
				nd_free(tres);
			}
			if(hasFlow) {
				float **f=net->hydResults->flow;
				if(getFloatArrayAtIndex(simResultsRef,"linkFlow",i,&tres)==ND_FAILURE) { return 0; }
				for(j=0;j<nlinks;j++) {
					f[i][j] = tres[j];
				}
				nd_free(tres);
			}
			if(hasVel) {
				float **v=net->hydResults->velocity;
				if(getFloatArrayAtIndex(simResultsRef,"linkVelocity",i,&tres)==ND_FAILURE) { return 0; }
				for(j=0;j<nlinks;j++) {
					v[i][j] = tres[j];
				}
				nd_free(tres);
			}
		}
	}
	{
		int *lens;
		if(getIntArray(simResultsRef, "DemandProfileLengths", &lens)==ND_FAILURE) return 0;
		for(i=0;i<nnodes;i++) {
			float *tres;
			nodeinfo[i].demandProfileLength=lens[i];
			if(lens[i]>0) {
				if(getFloatArrayAtIndex(simResultsRef,"DemandProfiles",i,&tres)==ND_FAILURE) { return 0; }
				nodeinfo[i].demandProfile=(float*)malloc(lens[i]*sizeof(float));
				memcpy(nodeinfo[i].demandProfile,tres,lens[i]*sizeof(float));
				nd_free(tres);
			}
		}
		nd_free(lens);
	}
	if(getInt(simResultsRef, "numTanks", &ntanks)==ND_FAILURE) return 0;
	net->numTanks=ntanks;
	if(getIntArray(simResultsRef, "tankIndices", &tankIndices)==ND_FAILURE) return 0;
	for(i=0;i<ntanks;i++)
		nodeinfo[tankIndices[i]].type = tank;
	nd_free(tankIndices);

	if(getStringArray(simResultsRef, "nodeIDs", &nodeIDs)==ND_FAILURE) return 0;
	for(i=0;i<nnodes;i++) {
		if (nodeinfo[i].type != tank) nodeinfo[i].type=junction;
		strcpy(nodeinfo[i].id,nodeIDs[i]);
	}
	nd_freeStringArray(nodeIDs,nnodes);
	return 1;
}

LIBEXPORT(void) freeNetwork(PNetInfo *pnet, PNodeInfo *pnodeinfo, PLinkInfo *plinkinfo)
{
	int i;
	PNetInfo net=*pnet;
	int nnodes=net->numNodes;
	PNodeInfo nodeinfo=*pnodeinfo;
	PLinkInfo linkinfo=*plinkinfo;

	for(i=0;i<nnodes;i++) {
		free(nodeinfo[i].nz);
		if(nodeinfo[i].demandProfileLength>0) {
			free(nodeinfo[i].demandProfile);
		}
	}
	free(nodeinfo);  *pnodeinfo=NULL;
	free(linkinfo); *plinkinfo=NULL;

	{
		int nsteps=net->numSteps;
		int nlinks=net->numLinks;
		PQualData qd = net->qualResults;
		PHydData hd = net->hydResults;
		for(i=0;i<net->numSpecies;i++) {
			int n;
			for(n=0;n<nnodes;n++) {
				if(qd->nodeC != NULL && qd->nodeC[i] != NULL) free(qd->nodeC[i][n]);
			}
			for(n=0;n<nlinks;n++) {
				if(qd->linkC != NULL && qd->linkC[i] != NULL) free(qd->linkC[i][n]);
			}
			if(qd->linkC != NULL && qd->linkC[i] != NULL) free(qd->linkC[i]);
			if(qd->nodeC != NULL && qd->nodeC[i] != NULL) free(qd->nodeC[i]);
		}
		if(qd->linkC != NULL) free(qd->linkC);
		if(qd->nodeC != NULL) free(qd->nodeC);
		free(net->qualResults);
		net->qualResults=NULL;
		for(i=0;i<nsteps;i++) {
			if(hd->flow!= NULL) free(hd->flow[i]);
			if(hd->velocity != NULL) free(hd->velocity[i]);
			if(hd->demand != NULL) free(hd->demand[i]);
			if(hd->pressure != NULL) free(hd->pressure[i]);
			if(hd->linkStatus != NULL) free(hd->linkStatus[i]);
		}
		free(net->hydResults);
		net->hydResults=NULL;
	}
	free(net);
	*pnet=NULL;
}


LIBEXPORT(void) loadQuality(NamedDataRef *simResultsRef, PNetInfo net, PNodeInfo nodeinfo, int speciesIndex)
{
	float **c;
	int i,j;
	int nnodes=net->numNodes;
	int nsteps=net->numSteps;
	int nSpecies=net->numSpecies;

	for(j=0;j<nnodes;j++) {
		for(i=0;i<nSpecies;i++) {
			nodeinfo[j].nz[i] = 0;
		}
	}

	c=net->qualResults->nodeC[speciesIndex];
	for(i=0;i<nsteps;i++) {
		float *tres;
		/** IMPORTANT!!! Need to free these later via call to nd_free */
		if(getFloatArrayAtIndex(simResultsRef,"quality",i,&tres)==ND_FAILURE) {
			return;
		}
		for(j=0;j<nnodes;j++) {
			float tc = tres[j];
		
			if(tc > 0) nodeinfo[j].nz[speciesIndex] = 1;
			c[j][i] = tc;
		}
		nd_free(tres);
	}
}

LIBEXPORT(void) loadFlow(NamedDataRef *simResultsRef, PNetInfo net)
{
	float **f=net->hydResults->flow;
	int nlinks=net->numLinks;
	int nsteps=net->numSteps;
	int i,j;
	f=net->hydResults->flow;
	for(i=0;i<nsteps;i++) {
		float *tres;
		/** IMPORTANT!!! Need to free these later via call to nd_free */
		if(getFloatArrayAtIndex(simResultsRef,"linkFlow",i,&tres)==ND_FAILURE) {
			return;
		}
		for(j=0;j<nlinks;j++) {
			float tf = tres[j];
			f[i][j] = tf;
		}
		nd_free(tres);
	}
}