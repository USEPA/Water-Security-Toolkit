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

#ifndef impact_instance_h
#define impact_instance_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <utilib/BitArray.h>

#include "basics.h"
#include "distance.h"
#include "instance.h"
#include "ImpactCache.h"

class PMImpactInstance : public PMInstance 
{
 private:
  
  double myOracleTime;
  PMDistanceOracle *myOracle;

  // 0 = sparse, 1 = dense. defaults to sparse.
  int myImpactRepresentation;

  // should I compute/store arrival time sequence info? 
  // only useful for imperfect sensors, so default is false.
  bool myStoreArrivalSequences;

  // this is retained persistently from the input impact file, simply because it's cheap.
  std::map<int,double> myUndetectedImpacts;     

  void reset(void);
  void fatal(const char *func, const char *msg);

  // TNT
  // myBinaryImpactFilePtr and positionImpFile are both used by impact representation 2
  FILE *myBinaryImpactFilePtr;
  void positionImpFile(long offs) {
    fpos_t pos;
	memset(&pos,0,sizeof(fpos_t));
#ifdef __linux__
	pos.__pos = (int64_t)offs;
#else
#ifdef WIN32
	pos = (fpos_t)offs;
#endif
#endif
	fsetpos(myBinaryImpactFilePtr,&pos);
  }
  // TNT: impactCacge is used by impact representation 3
  ImpactCache *impactCache;

 protected:

  // the sparse matrix representation of the impacts. the 0th
  // entries are are un-used, as the scenario indicies are 1-based.
  std::vector<std::map<int,double> > myDetectedImpactsSparse;
  std::vector<double> myUndetectedImpactsSparse;

  // the dense matrix representation of the impacts. the 0th
  // entries are un-used, as the scenario indicies are 1-based.
  // first index is customer/scenario, second index is
  // facility/detection location.
  std::vector<std::vector<double> > myDetectedImpactsDense;
  std::vector<double> myUndetectedImpactsDense;

  // the arrival sequences for each injection - not needed for
  // nominal p-median cases, but are very useful (although
  // they could be inferred) for imperfect-sensor variants.
  // the first key is the scenario index, 1-based; the 0th
  // entry is unused. the second entries is a vector of 
  // (node-id,arrival time) pairs, sorted in non-decreasing
  // order of arrival time. the node ids are 1-based. -1 
  // node indicies (representing the undetected virtual node)
  // do appear in the sequence, although entries with values
  // of 0 obviously don't.
  std::vector<std::vector<std::pair<int,double> > > myArrivalSequence;

  // compiled version of above for constant-time access.
  // DBL_MAX indicates not in set. first index is scenario,
  // second is node. 
  // NOTE: although incongrous, the compiled arrival times
  //       don't contain information about the "-1" witness
  //       node, due to the array representation. this 
  //       discprenacy should probably be corrected at 
  //       some point (maybe map the -1 time to the 0th
  //       entry, which is currently unused).
  std::vector<std::vector<double> > myArrivalTimesCompiled;

  int myN; //number of customers
  int myM; //number of potential facilities
  int myP; //number of facilities we are aiming for

  void resetDistances(void);

  void initOracle(void) 
    {
      myOracle=new PMDistanceOracle(this);
    }

  inline void checkFacility(int f) 
    {
      if((f<1) || (f>myM)) 
	{
	  fprintf (stderr, "ERROR: facility %d is out of range.\n", f);
	  exit(1);
	}
    }

  inline void checkUser(int u) 
    {
      if (u<1 || u>myM) 
	{
	  fprintf (stderr, "ERROR: user %d is out of range.\n", u);
	  exit(-1);
	}
    }

 public:
 ImpactCache* getImpactCache() { return impactCache;}
  PMImpactInstance(void);		
  PMImpactInstance (PMInstance *original, int *of, int *oc);
  virtual ~PMImpactInstance(void);		

  bool sparseRepresentationEnabled(void)const
    {
      return (myImpactRepresentation==0);
    }
  bool denseRepresentationEnabled(void)const
    {
      return (myImpactRepresentation==1);
    }
  void enableSparseRepresentation(void)
    {
      myImpactRepresentation=0;
    }
  void enableDenseRepresentation(void)
    {
      myImpactRepresentation=1;
    }
  void enableFileBasedRepresentation(void)
    {
      myImpactRepresentation=2;
    }
  void enableCachedRepresentation(void)
    {
      myImpactRepresentation=3;
    }

  bool arrivalSequenceStorageEnabled(void)const
    {
      return (myStoreArrivalSequences==true);
    }
  void enableArrivalSequenceStorage(void)
    {
      myStoreArrivalSequences=true;
    }
  void disableArrivalSequenceStorage(void)
    {
      myStoreArrivalSequences=true;
    }

  const std::vector<std::vector<std::pair<int,double> > > &arrivalSequences(void)const
    {
      return myArrivalSequence;
    }

  virtual double getOracleTime(void) 
    {
      return myOracle->getInitTime();
    }

  virtual int getMetric(void) 
    {
      return IMPACT;
    }

  virtual IntDouble *getCloser(int i, double v) 
    {
      return myOracle->getCloser(i,v);
    }

  void readImpact(FILE *file,int _p, bool createArrival=true)
	{
	utilib::BitArray tmp;
	std::map<int,int> dummy;
	readImpact(file,_p,tmp,dummy,createArrival);
	}

  void readImpact(FILE *file,int _p, utilib::BitArray& feasible_locations, std::map<int,int>& reduced_repn_map, bool createArrival=true);

  void printMatrix (FILE *file);

  //  this doesn't make any sense in the context of PMM-like applications
  virtual double getFacDist(int f, int g) 
    {
      std::cout << "***** WARNING - SHOULDN'T BE CALLING THIS!!" << std::endl;
      //    if(f>m || g>m) fatal ("getFacDist", "facility number out of range");
      //    return d[f][g]; //assumes facilities and users are the same thing
      return 0.0;
    }

  inline double getUndetectedImpact(int c)const
    {
      if(myImpactRepresentation==0)
	{
	  return myUndetectedImpactsSparse[c];
	}
      else
	{
	  return myUndetectedImpactsDense[c];
	}
    }

  inline double getArrivalTime(int c, int f)const
    {
      return myArrivalTimesCompiled[c][f];
    }
		
  virtual double getDist(int c, int f) 
    {
      if((c==0)||(f==0))
      	{
      	  return POPSTAR_INFINITY;
      	}

      if(myImpactRepresentation==0)
	{
	  const std::map<int,double> &thisImpactMap(myDetectedImpactsSparse[c]);
	  std::map<int,double>::const_iterator impactIter(thisImpactMap.find(f));
	  if(impactIter==thisImpactMap.end())
	    {
	      // the scenario is undetectable - assign to worse-case impact
	      return myUndetectedImpactsSparse[c];
	    }
	  else
	    {
	      return (*impactIter).second;
	    }
	}
      else if(myImpactRepresentation==1)
	{
	  return myDetectedImpactsDense[c][f];
	}
      else if(myImpactRepresentation==2)
	{
		positionImpFile( ((c-1) * myM + (f-1)) * sizeof(double));
		double impVal;
		fread(&impVal,sizeof(double),1,myBinaryImpactFilePtr);
	  return impVal;
	}
      else if(myImpactRepresentation==3)
	{
		return impactCache->getImpact(c-1,f-1);
	}
    }

  virtual double getFacCost(int f)
    {
      return 0.0;
    }

  virtual int getM(void)const
    {
      return myM;
    } 
  
  virtual int getN(void)const
    {
      return myN;
    }

  virtual int getP(void)const
    {
      return myP;
    }

  virtual void setP(int _p) 
    {
      myP=_p;
    }
};

#endif
