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

#ifndef tevasp_PopulationExposedObjective_h
#define tevasp_PopulationExposedObjective_h

#include "sp/ObjectiveBase.h"
extern "C" {
#include "teva-assess.h"
}

/// Trait/Functional class for the population exposed (i.e., sick) objective
class PopulationExposedObjective : public ObjectiveBase
{
public:

  /// Constructor.
  PopulationExposedObjective(PMem mem):
    myCurrentSickened(0.0),
    myLastComputedTimeStep(-1),
    myLastImpactTimeStep(-1)
  {
    assessMem = mem;
    myNetInfo = NULL;
    ourImpactOutputFileSuffix="pe.impact";
    ourEvaluationOutputFileSuffix="pe.eval";
  }

  /// Copy constructor.
  PopulationExposedObjective(const PopulationExposedObjective &other):
    ObjectiveBase(other),
    myCurrentSickened(other.myCurrentSickened),
    myTAIFileName(other.myTAIFileName),
    myLastComputedTimeStep(other.myLastComputedTimeStep),
    myLastImpactTimeStep(other.myLastImpactTimeStep)
  {
    // we don't copy assessMem and myNetInfo
    ourImpactOutputFileSuffix="pe.impact";
    ourEvaluationOutputFileSuffix="pe.eval";
  }

  /// Destructor.
  virtual ~PopulationExposedObjective(void)
  {
  }

  /// Assignment operator.
  PopulationExposedObjective &operator=(const PopulationExposedObjective &other)
  {
    if(this!=&other)
      {
	ObjectiveBase::operator=(other);
	myCurrentSickened=other.myCurrentSickened;
	myTAIFileName=other.myTAIFileName;
	myLastComputedTimeStep=other.myLastComputedTimeStep;
	myLastImpactTimeStep=other.myLastImpactTimeStep;
      }
    return *this;
  }

  virtual void update(utilib::Any v) 
  {
	// we'll assume that the old assessMem was deleted 
	assessMem = utilib::anyval_cast<PMem>(v);
        myCurrentSickened=0.0;

    	//myNetInfo = net;	we assume that multiple .tso files will
	//			concern the same network

    	myLastComputedTimeStep=-1;
    	myLastImpactTimeStep=-1;
  }

  ///
  void setTAIFileName(const char *theFileName)
  {
    myTAIFileName=theFileName;
  }

  ///
  void setTSOFileOrDirectory(const char *theName)
  {
    myTSOFileOrDirectory=theName;
  }

  ///
  void setTSOPattern(const char *thePattern)
  {
    myTSOPattern=thePattern;
  }


  ///
  virtual void init(const std::string &outputPrefix,PNetInfo net,PNodeInfo nodes,PLinkInfo links,double detectionDelay, int numScenarios=-1)
  {
    if (myNetInfo){
      return; // init should not be called twice, memory would leak
    }

    ObjectiveBase::init(outputPrefix,net,nodes,links,detectionDelay,numScenarios);
    myCurrentSickened=0.0;

    myNetInfo = net;

    myLastComputedTimeStep=-1;
    myLastImpactTimeStep=-1;
  }

  ///
  virtual void resetForScenario(PNetInfo net, PNodeInfo nodes, PLinkInfo links,
				PSourceData sources)
  {
    ObjectiveBase::resetForScenario(net,nodes,links,sources);
    myCurrentSickened=0.0;

    myLastComputedTimeStep=-1;
    myLastImpactTimeStep=-1;

    assessMem->net->info = net;
    assessMem->nodeinfo = nodes;
  }

  ///
  virtual void updateImpactStatisticsDueToNode(int thisNodeIndex,double thisNodeQuality,PNetInfo net, PNodeInfo nodes,PLinkInfo links,int thisTimeStep,double reportStep)
  {
    myLastImpactTimeStep=thisTimeStep;
  }

  ///
  virtual void addNominalImpact(int thisNodeIndex,int thisTimeStep,double reportStep)
  {
    if(myLastComputedTimeStep!=myLastImpactTimeStep)
      {
	updateNumberSickened(myLastImpactTimeStep);
	myLastComputedTimeStep=myLastImpactTimeStep;
      }

    updateImpacts(thisNodeIndex,myCurrentSickened,thisTimeStep*reportStep);
    
    updateSensorEvaluation(thisNodeIndex,myCurrentSickened);
  }

  ///
  virtual void addUndetectedImpact(int maxTimeStep,double reportStep)
  {
    // the threat simulator code uses INT_MAX as the
    // 'not detected' parameter value.
    if(myLastComputedTimeStep!=maxTimeStep)
      {
	updateNumberSickened(INT_MAX);
	myLastComputedTimeStep=maxTimeStep;
      }

    if (reportStep < 0.0) {
       updateImpacts(UNDETECTED_NODE_INDEX,0.0,-maxTimeStep*reportStep);
       updateSensorEvaluation(UNDETECTED_NODE_INDEX,0.0);
    }
    else {
       updateImpacts(UNDETECTED_NODE_INDEX,myCurrentSickened,maxTimeStep*reportStep);
       updateSensorEvaluation(UNDETECTED_NODE_INDEX,myCurrentSickened);
    }
  }

protected:

  /// The number of sickened for the current scenario.
  /// This defaults to 0.0.
  double myCurrentSickened;

  /// The name of the TAI input file.
  /// This defaults to "".
  std::string myTAIFileName;
  
  /// The "mother" data structure for the health impacts code
  PMem assessMem;

  ///
  PNetInfo  myNetInfo;

  /// The last time-step for which health impacts were actually computed.
  /// THis defaults to -1. This enables caching of results for
  /// all nodes at which detection occurs for a single time-step.
  int myLastComputedTimeStep;

  /// The last time-step for which an impact due to a non-zero
  /// quality concentration at a node was reported. the current 
  /// implementation simply uses the Murray et al. code as-is,
  /// without incremental (and computationally more efficient)
  /// computation of the fatality count. by tracking this value,
  /// it is possible to compute health impacts only when new
  /// node hits are recorded, in the addNominalImpact() and 
  /// addUndetectedImpact() methods. defauls to -1.
  int myLastImpactTimeStep;

  /// The tso input file names. in maintaining the Murry et al.
  /// code base, it is necessary to simultaneously open and 
  /// read the same TSO File that is read in the main tso2Impact
  /// process-scenario loop. both values default to "".
  std::string myTSOFileOrDirectory;

  ///
  std::string myTSOPattern;

private:

  /// update the number sickened from the pre-computed HIA data
  void updateNumberSickened(int theNewTimeStep)
  {
    if (theNewTimeStep > assessMem->dr->maxsteps){
     theNewTimeStep = assessMem->dr->maxsteps-1;
    }
	myCurrentSickened = assessMem->xa->res[theNewTimeStep];
  }

};

#endif
