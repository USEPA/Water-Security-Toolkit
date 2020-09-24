/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top SPOT directory.
 *  _________________________________________________________________________
 */

`include <stdlib.h>
#include <string.h>

#include "HelloAnalysis.h"

extern "C" {
#include "nrutil.h"
#include "NamedData.h"


PMem loadOptions(NamedDataRef *analysisOptionsRef, 
		 NamedDataRef *simResultsRef, ModuleType componentType)
{
	int 
		i,
		nnodes,
		ntanks,
		njunctions,
		nsteps,
		hello;	// GUI input
	int *tankIndices; 
	float stepsize;

	PMem mem;
	PSource   sources;
	PNetInfo  net;
	PNodeInfo  node;
	char **nodeIDs;
/* 	float**   nodedata;  */
	float tf;

	if(getInt(analysisOptionsRef, "Hello", &hello)==ND_FAILURE) 
		return NULL;
	if(getInt(simResultsRef, "numTimeSteps", &nsteps)==ND_FAILURE) 
		return NULL;
	if(getInt(simResultsRef, "numNodes", &nnodes)==ND_FAILURE) 
		return NULL;
	if(getInt(simResultsRef, "numJunctions", &njunctions)==ND_FAILURE) 
		return NULL;
	if(getFloat(simResultsRef, "stepSize", &stepsize)==ND_FAILURE) 
		return NULL;


	mem = (PMem)calloc(1,sizeof(Mem));

	/** network parameters */
	net = (PNetInfo)calloc(1,sizeof(NetInfo));
	mem->net   = net;
	mem->hello = hello;
/*	net->fltmax=fltmax; */
	net->njunctions=njunctions;
/*	net->nlinks=nlinks;  */
	net->nnodes=nnodes;
	net->nsteps=nsteps;
	net->stepsize=stepsize;
	
	if(getStringArray(simResultsRef, "nodeIDs", &nodeIDs)==ND_FAILURE) 
		return NULL;
	node = (PNodeInfo)calloc(nnodes,sizeof(NodeInfo));
	mem->node=node;
	for(i=0;i<nnodes;i++) {
		node[i].c = (float **)calloc(1,sizeof(float*));
		node[i].c[0] = vector(0,nsteps-1);
		node[i].q = vector(0,nsteps-1);
		node[i].id = (char *)calloc(strlen(nodeIDs[i])+1,sizeof(char));
		strcpy(node[i].id,nodeIDs[i]);
		node[i].nz = (int *)calloc(1,sizeof(int)); // for now only one species...
	}
	nd_freeStringArray(nodeIDs,nnodes);


	
	/** Really only need to allocate the next structure in the intermediate analysis server */
	if(componentType == IntermediateAnalysis) {
		sources = (PSource)calloc(MAXSOURCES,sizeof(Source));/* Sources */
		mem->sources=sources;
	}
	if(componentType==IntermediateAnalysis || componentType==Aggregation) {
	}
	if(node[j].q!=NULL) {
		for(i=0;i<nsteps;i++) {
			float *tres;
			int j;
			/** IMPORTANT!!! Need to free these later via call to nd_free */
			if(getFloatArrayAtIndex(simResultsRef,"demands",i,&tres)
			   ==ND_FAILURE) {
				return NULL;
			}
			for(j=0;j<nnodes;j++) {
				node[j].q[i] = tres[j];
			}
			nd_free(tres);
		}
	}
	if(getInt(simResultsRef, "numTanks", &ntanks)==ND_FAILURE) return NULL;
	net->ntanks=ntanks;
	if(getIntArray(simResultsRef, "tankIndices", &tankIndices)==ND_FAILURE)
		return NULL;
	for(i=0;i<ntanks;i++)
		node[tankIndices[i]-1].ntype = tank;
	nd_free(tankIndices);

	if(getStringArray(simResultsRef, "nodeIDs", &nodeIDs)==ND_FAILURE) 
		return NULL;
	for(i=0;i<nnodes;i++) {
		if (node[i].ntype != tank) node[i].ntype=junction;
		strcpy(node[i].id,nodeIDs[i]);
	}
	nd_freeStringArray(nodeIDs,nnodes);
	for(int j=0;j<nnodes;j++) {
		mem->node[j].nz[0] = 0;
	}
	return mem;
}

void freeMemory(PMem mem, ModuleType componentType) {
	int i;
	for(i=0;i<mem->net->nnodes;i++) {
		free_vector(mem->node[i].c[0],0,mem->net->nsteps-1);
		free(mem->node[i].c);
		free_vector(mem->node[i].q,0,mem->net->nsteps-1);
		free(mem->node[i].nz);
		free(mem->node[i].id);
	}
	free(mem->node);


	/** Really only need to allocate the next structure in the intermediate analysis server */
	if(componentType == IntermediateAnalysis) {
		free(mem->sources);
	}

	if(componentType == IntermediateAnalysis) {
	}
	free(mem->net);
	free(mem);
}

};
