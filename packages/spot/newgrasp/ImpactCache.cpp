/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the LGPL License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top software directory.
 *  _________________________________________________________________________
 */

#include <teva_config.h>
#include "ImpactCache.h"
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#ifdef TEVA_SPOT_HAVE_UNISTD_H
# include <unistd.h>
#endif

using namespace NewSensorPlacement;

ImpactCache::ImpactCache(int nnodes, int nevents, int max)
{
    numNodes=nnodes;
	numEvents = nevents;
    
    mode=CREATING;
    maxLoaded=max;
    numLoaded=0;
    fpEventMajor=fopen("impevent.bin","wb");
	eventArraysIndex.resize(nevents);
	nodeArraysIndex.resize(nnodes);
	eventArraysLoaded=NULL;
	nodeArraysLoaded=NULL;

}
void ImpactCache::startCreation()
{
	colData = new double[numNodes*numEvents];
	newEventRow(0);
}
void ImpactCache::newEventRow(int eventNum)
{
	ImpactCacheData *icd = new ImpactCacheData(eventNum,numNodes);
	eventArraysIndex[eventNum] = icd;
	double *data = icd->getData();
	for(int ii=0;ii<numNodes;ii++) data[ii]=DBL_MAX;
	numLoaded++;
//	printf("N %4d: numLoaded: %2d\n",eventNum,numLoaded);
}
// eventNum is previousEvent id!
void ImpactCache::newEvent(int eventNum, double undetectedImpact)
{
	if(mode ==CREATING) {
//		std::cout << "eventnum: " << eventNum << std::endl;
		double *data = eventArraysIndex[eventNum-1]->getData();
		for(int ii=0;ii<numNodes;ii++) {
			if(data[ii]==DBL_MAX) {
				setImpact(eventNum-1,ii,undetectedImpact);
			}
		}
		if(numLoaded == maxLoaded) {
			// now write them all out
			writeImpactData();
		}
		if(eventNum!=numEvents)
			newEventRow(eventNum);
	}
}
void ImpactCache::writeImpactData()
{
	int i;
	for(i=0;i<numEvents && eventArraysIndex[i]==NULL; i++);
	for(;i<numEvents && eventArraysIndex[i]!=NULL; i++) {
		double *data = eventArraysIndex[i]->getData();
		fwrite(data,sizeof(double),numNodes,fpEventMajor);
		ImpactCacheData *p=eventArraysIndex[i];
		eventArraysIndex[i] = NULL;
		delete p;
		numLoaded--;
//		printf("W %4d: numLoaded: %2d\n",i,numLoaded);
	}
}
void ImpactCache::setImpact(int c, int f, double impact)
{
	if(mode ==CREATING) {
		eventArraysIndex[c]->getData()[f] = impact;
		int i=f*numEvents+c;
		colData[i] = impact;
	}	
}
void ImpactCache::positionFile(FILE *fp,int row, int col, int rowLen)
{
    fpos_t pos;
	memset(&pos,0,sizeof(fpos_t));
	long offs = (((long)row)*rowLen+col)*sizeof(double);
#ifdef __linux__
	pos.__pos = (int64_t)offs;
#else
#ifdef WIN32
	pos = (fpos_t)offs;
#endif
#endif
	fsetpos(fp,&pos);
}

void ImpactCache::created()
{
	writeImpactData();
	std::cout << "writing node-major data...\n";	
    fpNodeMajor=fopen("impnode.bin","wb");
	fwrite(colData,sizeof(double)*numNodes,numEvents,fpNodeMajor);
	delete []colData;
	std::cout << "done.\n";	
	
	fclose(fpEventMajor);
	fclose(fpNodeMajor);
    fpEventMajor=fopen("impevent.bin","rb");
    fpNodeMajor=fopen("impnode.bin","rb");
	
	setEventMajor();
}
double ImpactCache::getImpact(int c, int f)
{
	if(mode == EVENT_MAJOR) {
		double *data = getImpactData(c,eventArraysIndex,&eventArraysLoaded,fpEventMajor,numNodes);
//		if(data[f]> 60000.0) std::cout << "potentially invalid data: c=" << c+1 << " f=" << f+1 << "val=" << data[f] << std::endl;
		return data[f];
	} else if (mode == NODE_MAJOR) {
		double *data = getImpactData(f,nodeArraysIndex,&nodeArraysLoaded,fpNodeMajor,numEvents);
//		if(data[c]> 60000.0) std::cout << "potentially invalid data: c=" << c+1 << " f=" << f+1 << "val=" << data[c] << std::endl;
		return data[c];
	}
}
double *ImpactCache::getImpactData(int i, std::vector<ImpactCacheData*> &index, ImpactCacheData** loaded, FILE *fp, int dataElemCount)
{
	ImpactCacheData *icd=index[i];
	if(icd==NULL) {
//		std::cout << "index " << i << " not loaded...\n";
		icd = new ImpactCacheData(i,dataElemCount);
		index[i] = icd;
		positionFile(fp,i,0,dataElemCount);
		fread(icd->getData(),sizeof(double),dataElemCount,fp);
		icd->next = *loaded;
		if(*loaded!=NULL) (*loaded)->prev = icd;
		*loaded=icd;
		numLoaded++;
	} else {
		if(icd != *loaded) {  // if it isn't already the first
			if(icd->next != NULL) {
				icd->next->prev = icd->prev;
			}
			icd->prev->next = icd->next;
			icd->prev=NULL;
			icd->next = *loaded;
			if(*loaded != NULL) {
				icd->next->prev=icd;
			}
			*loaded = icd;
		}
	}
	if(numLoaded>maxLoaded) {
		ImpactCacheData *p=*loaded;
		while(p->next != NULL) p=p->next;
		index[p->getIndex()] = NULL;
		p->prev->next=NULL;
		delete p;
		numLoaded--;
	}
	return icd->getData();
}
void ImpactCache::setEventMajor() {
	if(mode != EVENT_MAJOR) {
		clearCacheData(nodeArraysIndex,&nodeArraysLoaded);
	}
	mode=EVENT_MAJOR;
}
void ImpactCache::setNodeMajor() {
	if(mode != NODE_MAJOR) {
		clearCacheData(eventArraysIndex,&eventArraysLoaded);
	}
	mode=NODE_MAJOR;
}

void ImpactCache::clearCacheData(std::vector<ImpactCacheData*> &index, ImpactCacheData** loaded) {
	ImpactCacheData *p = *loaded;
	while(p != NULL) {
		index[p->getIndex()] = NULL;
		ImpactCacheData *t = p;
		p=p->next;
		numLoaded--;
		delete t;
	}
	*loaded=NULL;
}

ImpactCache::~ImpactCache(void)
{
	clearCacheData(nodeArraysIndex,&nodeArraysLoaded);
	clearCacheData(eventArraysIndex,&eventArraysLoaded);
	fclose(fpEventMajor);
	fclose(fpNodeMajor);
    unlink("impevent.bin");
    unlink("impnode.bin");
	// free memory from lists & maps
	// close and delete files
}
