/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the LGPL License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top software directory.
 *  _________________________________________________________________________
 */

/**************************************************************
 * class PMImpactInstance: 
 **************************************************************/

#include "instance.h"
#include "impact_instance.h"
#include "bossa_timer.h"
#include <string.h>

#include <iostream>
#include <iomanip>
#include <algorithm>

#include <float.h>

#include <time.h>

#ifdef TEVA_SPOT_HAVE_UNISTD_H
#include <unistd.h>
#endif

/*------------------------------------------
  | constructor: produces an empty instance,
  | with p = 0
  *-----------------------------------------*/ 
PMImpactInstance::PMImpactInstance(void) 
{
  reset();
}

void PMImpactInstance::reset(void) 
{
  impactCache=NULL;
  myOracleTime=0.0;
  myOracle=0;   

  myImpactRepresentation=0;
  myStoreArrivalSequences=false;

  myUndetectedImpacts.clear();

  myN=myM=myP=0;  // customers, potential facilites, actual facilities: all zero
}


/*-----------------------------------------------------------
  | constructor: produces a restricted version of an existing
  | instance, with only a subset of the facilities and of
  | the customers
  *----------------------------------------------------------*/

PMImpactInstance::PMImpactInstance(PMInstance *original, int *of, int *oc) 
{
  std::cout << "***** WARNING - SHOULDN'T BE CALLING THIS!!" << std::endl;
  //  reset();

  //   n = oc[0]; //number of customers in this instance
  //   m = of[0]; //number of facilities in this instance
  //   p = 0;     //just guessing some value
  //   int i;
	
  //   d = new double * [n+1];
  //   for (i=0; i<=n; i++) {
  //     d[i] = new double [m+1];
  //     d[i][0] = POPSTAR_INFINITY;
  //   }
  //   for (i=1; i<=m; i++) d[0][i] = POPSTAR_INFINITY; 
	
  //   //---------------
  //   // set distances
  //   //---------------
  //   int c0, f0, c, f;
  //   for (c=1; c<=n; c++) {
  //     c0 = oc[c]; 
  //     for (f=1; f<=m; f++) {
  //       f0 = of[f];
  //       d[c][f] = original->getDist(c0,f0);
  //     }
  //   }

  //   initOracle();
}

/*------------------------------------------
  | printMatrix: outputs the distance matrix
  | (even if there is no such thing)
  *-----------------------------------------*/

void PMImpactInstance::printMatrix(FILE *file) 
{
  std::cout << "***** WARNING - SHOULDN'T BE CALLING THIS!!" << std::endl;
  // TBD - REDO!
  //  int c, f;
  //  for (c=1; c<=n; c++) {
  //    for (f=1; f<=m; f++) {
  //      double d = getDist(c,f);			
  //      if (d==POPSTAR_INFINITY) fprintf (file, "--- ");
  //      else fprintf (file, "%f ", d);
  //    }
  //    fprintf (file, "\n");
  //  }
}


/*-----------------------------------------------
  | fatal: prints error message and exits program
  *----------------------------------------------*/

void PMImpactInstance::fatal(const char *func, const char *msg) 
{
  fprintf (stderr, "PMImpactInstance::%s: %s.\n", func, msg);
  exit(-1);
}


/*-------------------------------------------------
  | resetDistances: sets all distances to infinity
  |
  | except those from a city to itself (set to 0);
  | this routine assumes each city is a potential
  | facility
  *------------------------------------------------*/

void PMImpactInstance::resetDistances(void) 
{	
  //  std::cout << "PMImpactInstance::resetDistances() - begin" << std::endl;

  //   int f, u;

  //   //reset distances
  //   d[0][0] = POPSTAR_INFINITY;
  //   for (f=1; f<=m; f++) d[0][f] = POPSTAR_INFINITY * POPSTAR_INFINITY;
	
  //   for (u=1; u<=n; u++) {
  //     d[u][0] = POPSTAR_INFINITY * POPSTAR_INFINITY;
  //     for (f=1; f<=m; f++) d[u][f] = POPSTAR_INFINITY;	
  //   }

  //   for (f=1; f<=m; f++) d[f][f] = 0; //distance to itself is zero
	
  //   //note that we can use the fact that d[0][x] = d[x][0] = POPSTAR_INFINITY
  //   //except for x = 0
  //  std::cout << "PMImpactInstance::resetDistances() - end" << std::endl;
}


/*******************************************************
 * 
 * read file in impact format, a partially specified distance matrix
 *
 *******************************************************/
// TNT: Added a boolean flag to optionally load the arrival data (some progs don't need it, and it can take a lot of memory)
void PMImpactInstance::readImpact(FILE *input, int _p, utilib::BitArray& feasible_locations, std::map<int,int>& reduced_repn_map, bool createArrival) 
{
	//  std::cout << "PMImpactInstance::readImpact() - begin" << std::endl;
	// TNT: Added cpu and wall clock timers for reporting impact file processing info to log file
	clock_t start,stop;
	time_t wcstart,wcstop;

	const int LINESIZE(256);
	char buffer[LINESIZE+1];
	// TNT: Added: numEvents is loaded from the impact file if it is there (the second value on the first line)
	int numEvents;
	myN=0;
	myM=0;
	myP=_p;

	// TNT: Added
	start = clock();	
	time( &wcstart );
	resetDistances();
	
	// working variables for the input - not needed once the
	// sparse or dense matrix forms are compiled.
	std::map<int,std::map<int,double> > inputDetectedImpacts;
	
	// grab the total number of candidate facilities
	// from the first line in the impact file. you 
	// don't want to compute it from the highest index
	// you see in the impact file, as some nodes aren't
	// represented there (e.g., those with zero demand
	// or those that can't be reached by an injection).
	// The second value on the first line (not there in older versions
	// of the impact file) contains the number of events contained in the imapct
	// file
	fgets(buffer,LINESIZE,input);
	int nvals = sscanf(buffer,"%d %d",&myM, &numEvents);
	if(nvals < 1 || nvals > 2) {
		fatal("readImpact", "Incorrectly formatted input file");
	}
	myM = feasible_locations.nbits();
	
	// skip the second line, which details the delay 
	// information and the number of impacts per scenario
	fgets(buffer,LINESIZE,input);

	int prevScenarioIndex(-1);

	// create the empty 0th-entry of the arrival sequence
	if(createArrival) {
		myArrivalSequence.reserve(myM+1);
		myArrivalSequence.push_back(std::vector<std::pair<int,double> >());
	}
	// TNT
	// Added code to process impact file representations 2 (file-based) and 3 (cache-based)
	// Representation 2 stores the 2-dimensional impact data in a binary file on disk
	// that is used every time a request comes in for an impact value - an offset is calculated
	// the file pointer is positioned to that offset and a single double value is read.
	// this method is fairly inefficient, but it allows for the greatest number
	// of problems to be solved
	// the imp array is only used for representation=2
	// Representation 3 stores the impact data in a cache (See ImpactCache.cpp and .h)
        std::vector<double> imp;
	std::cout << "myImpactRepresentation=" << myImpactRepresentation << "\n";
	if(myImpactRepresentation==2) {
		imp.resize(myM);
		for(int ii=0;ii<myM;ii++) imp[ii]=POPSTAR_INFINITY;
		myBinaryImpactFilePtr = fopen("imp.bin","wb");
		std::cout << "opened imp.bin: " << myBinaryImpactFilePtr << "\n";
	} else if (myImpactRepresentation==3) {
		if(nvals == 1) {
			// older style impact file - no event count - need to scan through the file to determine how many events...
			std::cout << "need to determine number of events - not in impact file...\n";
			numEvents=-1;
			int scenarioIndex;
			while(fgets(buffer, LINESIZE, input)) {
				if(sscanf(buffer,"%d",&scenarioIndex)==1) {
					if(scenarioIndex > numEvents) numEvents = scenarioIndex;
				}
			}
			rewind(input);
			fgets(buffer,LINESIZE,input);
			fgets(buffer,LINESIZE,input);
		}
		std::cout << numEvents << " in impact file\n";
		impactCache=new ImpactCache(myM,numEvents,100);
		impactCache->startCreation();
	}

	while(fgets(buffer, LINESIZE, input)) {
		// std::cout << "Input line=" << buffer << std::endl;
		int scenarioIndex;
		int nodeIndex;
		int detectionTime;
		double thisImpact;
		if(sscanf(buffer,"%d %d %d %lf",&scenarioIndex,&nodeIndex,&detectionTime,&thisImpact)==4) {
			if(scenarioIndex>myN) {
				myN=scenarioIndex;
			}
			// std::cout << "Read line - scenario index=" << scenarioIndex << " node index=" << nodeIndex << " impact=" << thisImpact << " detection time=" << detectionTime << std::endl;
			// TNT
			if(prevScenarioIndex != scenarioIndex && prevScenarioIndex!=-1) {
				if(myImpactRepresentation==2) {
					double undetected=myUndetectedImpacts[prevScenarioIndex];
					for(int ii=0;ii<myM;ii++) {
						if(imp[ii]==POPSTAR_INFINITY) imp[ii] = undetected;
					}
					fwrite(&(imp[0]),sizeof(double),myM,myBinaryImpactFilePtr);
					fflush(myBinaryImpactFilePtr);
					for(int ii=0;ii<myM;ii++) imp[ii]=POPSTAR_INFINITY;
				} else if (myImpactRepresentation==3) {
					impactCache->newEvent(prevScenarioIndex,myUndetectedImpacts[prevScenarioIndex]);
				}
			}
			if(nodeIndex==-1) {
				if(myUndetectedImpacts.find(scenarioIndex)!=myUndetectedImpacts.end()) {
					std::cerr << "***Undetected impact for scenario index=" << scenarioIndex << " already defined" << std::endl;
					exit(1);
				}
				myUndetectedImpacts[scenarioIndex]=thisImpact;
			} else {
				if (feasible_locations(nodeIndex-1)) {
				   nodeIndex = reduced_repn_map[nodeIndex];
				   if(myImpactRepresentation==2) {
					imp[nodeIndex-1] = thisImpact;
				   } else if(myImpactRepresentation==3) {
					impactCache->setImpact(scenarioIndex-1,nodeIndex-1,thisImpact);
				   } else {
					if(inputDetectedImpacts.find(scenarioIndex)==inputDetectedImpacts.end()) {
						inputDetectedImpacts[scenarioIndex]=std::map<int,double>();
					}
				
					std::map<int,double> &thisSubmap=inputDetectedImpacts[scenarioIndex];
				
					if(thisSubmap.find(nodeIndex)!=thisSubmap.end()) {
						std::cerr << "***Detected impact for scenario index=" << scenarioIndex << ", node index=" << nodeIndex << " already defined" << std::endl;
						exit(1);
					}
					thisSubmap[nodeIndex]=thisImpact;
				   }
				}
				else {
				   // This is a hack, to allow us to recognize the fact that we've already
				   // mapped the nodeIndex value.
				   nodeIndex = -2;
				}
			}
			if(createArrival && (nodeIndex != -2)) {
				// update the arrival sequences
				if(scenarioIndex!=prevScenarioIndex) {
					myArrivalSequence.push_back(std::vector<std::pair<int,double> >());
				}
				myArrivalSequence[scenarioIndex].push_back(std::make_pair(nodeIndex,detectionTime));
			}
			prevScenarioIndex=scenarioIndex;
		} else {
			fatal("readImpact", "Incorrectly formatted input file");
		}
	}
	if(myImpactRepresentation==2) {
		double undetected=myUndetectedImpacts[myN];
		for(int ii=0;ii<myM;ii++) {
			if(imp[ii]==POPSTAR_INFINITY) imp[ii] = undetected;
		}
		fwrite(&(imp[0]),sizeof(double),myM,myBinaryImpactFilePtr);
		fflush(myBinaryImpactFilePtr);
		fclose(myBinaryImpactFilePtr);
		myBinaryImpactFilePtr = fopen("imp.bin","rb");
	} else if (myImpactRepresentation==3) {
		impactCache->newEvent(prevScenarioIndex,myUndetectedImpacts[prevScenarioIndex]);
		impactCache->created();
	}
	stop = clock();
	time( &wcstop );
	float readTime = (float)(stop-start)/(float)CLOCKS_PER_SEC;
	double readWC = difftime(wcstop,wcstart);
	std::cout << "Time to read impact file: " << readTime << " " << readWC << "\n";
	// create sparse or dense versions for faster lookup.
	start = clock();
	time( &wcstart );
	
	if(myImpactRepresentation==0) {
		myDetectedImpactsSparse=std::vector<std::map<int,double> >(myN+1,std::map<int,double>());
		myUndetectedImpactsSparse=std::vector<double>(myN+1,0.0);
		
		for(std::map<int,double>::iterator iter=myUndetectedImpacts.begin();iter!=myUndetectedImpacts.end();iter++) {
			int thisScenarioIndex=(*iter).first;
			double thisImpact=(*iter).second;
			myUndetectedImpactsSparse[thisScenarioIndex]=thisImpact;
		}
		for(std::map<int,std::map<int,double> >::iterator iter=inputDetectedImpacts.begin();iter!=inputDetectedImpacts.end();iter++) {
			int thisScenarioIndex=(*iter).first;
			const std::map<int,double> &thisImpactMap=(*iter).second;
			myDetectedImpactsSparse[thisScenarioIndex]=thisImpactMap;
			inputDetectedImpacts[thisScenarioIndex].clear();
		}
	} else if(myImpactRepresentation==1) {
		myDetectedImpactsDense=std::vector<std::vector<double> >(myN+1,std::vector<double>(myM+1,0.0));
		
		for(int i=1;i<=myN;i++) {
			const std::map<int,double> &detectedImpacts=inputDetectedImpacts[i];
			double undetectedImpact=myUndetectedImpacts[i];
			for(int j=1;j<=myM;j++) {
				std::map<int,double>::const_iterator iter=detectedImpacts.find(j);
				if(iter==detectedImpacts.end()) {
					myDetectedImpactsDense[i][j]=undetectedImpact;
				} else {
					myDetectedImpactsDense[i][j]=(*iter).second;
				}
			}
			inputDetectedImpacts[i].clear();
		}
		myUndetectedImpactsDense=std::vector<double>(myN+1,0.0);
		for(int i=1;i<=myN;i++) {
			myUndetectedImpactsDense[i]=myUndetectedImpacts[i];
		}
	} else if(myImpactRepresentation==2) {
		myUndetectedImpactsDense=std::vector<double>(myN+1,0.0);
		for(int i=1;i<=myN;i++) {
			myUndetectedImpactsDense[i]=myUndetectedImpacts[i];
		}
	} else if(myImpactRepresentation==3) {
		myUndetectedImpactsDense=std::vector<double>(myN+1,0.0);
		for(int i=1;i<=myN;i++) {
			myUndetectedImpactsDense[i]=myUndetectedImpacts[i];
		}
	}

	stop = clock();
	time( &wcstop );
	float cvtTime = (float)(stop-start)/(float)CLOCKS_PER_SEC;
	double cvtWC = difftime(wcstop,wcstart);
	std::cout << "Time to convert impact data: " << cvtTime << " " << cvtWC << "\n";

	if(myStoreArrivalSequences==true) {
		myArrivalTimesCompiled=std::vector<std::vector<double> >(myN+1,std::vector<double>(myM+1,DBL_MAX));
		
		for(int i=1;i<=myN;i++) {
			const std::vector<std::pair<int,double> > &thisArrivalSequence=myArrivalSequence[i];
			for(size_t j=0;j<thisArrivalSequence.size();j++) {
				int thisNode(thisArrivalSequence[j].first);
				// dummy witness arrival times aren't currently stored - see note in header
				if(thisNode!=-1) {
					double thisArrivalTime(thisArrivalSequence[j].second);
					myArrivalTimesCompiled[i][thisNode]=thisArrivalTime;
				}
			}
		}
    }
    start = clock();
	time( &wcstart );
	if(impactCache != NULL) impactCache->setEventMajor();
	initOracle();
	stop = clock();
	time( &wcstop );
	float oracleTime = (float)(stop-start)/(float)CLOCKS_PER_SEC;
	double oracleWC = difftime(wcstop,wcstart);
	std::cout << "Time to init oracle: " << oracleTime << " " << oracleWC << "\n";
}



/*******************************************
 *
 * destructor: deallocates distance matrix,
 * gets rid of distance oracle
 *
 *******************************************/

PMImpactInstance::~PMImpactInstance(void) 
{
  delete myOracle;
  myUndetectedImpacts.clear();
  // TNT: Added code to clean up cache data
  if(impactCache != NULL) delete impactCache;
  if(myImpactRepresentation==2) unlink("imp.bin");
}
