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

#ifndef tevasp_PopulationDosedObjective_h
#define tevasp_PopulationDosedObjective_h

#include "sp/ObjectiveBase.h"
extern "C" {
#include "teva-assess.h"
}


/// Trait/Functional class for the population exposed (i.e., sick) objective
class PopulationDosedObjective : public ObjectiveBase
{
public:

  /// Constructor.
  PopulationDosedObjective(PMem mem):
    myCurrentDosed(0.0),
    myLastComputedTimeStep(-1),
    myLastImpactTimeStep(-1)
  {
    myDoseThresholdIdx = -1;  // signifies to only use one threshold (for standalone tso2Impacts)
    assessMem = mem;
    myNetInfo = NULL;
    ourImpactOutputFileSuffix="pd.impact";
    ourEvaluationOutputFileSuffix="pd.eval";
  }

  PopulationDosedObjective(PMem mem, int idx):
    myCurrentDosed(0.0),
    myLastComputedTimeStep(-1),
    myLastImpactTimeStep(-1)
  {
    myDoseThresholdIdx = idx;
    assessMem = mem;
    myNetInfo = NULL;
    char tmps[40];
    sprintf(tmps,"pd_%s.impact",mem->dot->threshIDs[myDoseThresholdIdx]);
    ourImpactOutputFileSuffix=tmps;
    sprintf(tmps,"pd_%s.eval",mem->dot->threshIDs[myDoseThresholdIdx]);
    ourEvaluationOutputFileSuffix=tmps;
  }
  /// Copy constructor.
  PopulationDosedObjective(const PopulationDosedObjective &other):
    ObjectiveBase(other),
    myCurrentDosed(other.myCurrentDosed),
    myDoseThresholdIdx(other.myDoseThresholdIdx),
    myTAIFileName(other.myTAIFileName),
    myLastComputedTimeStep(other.myLastComputedTimeStep),
    myLastImpactTimeStep(other.myLastImpactTimeStep)
  {
    // we don't copy assessMem and myNetInfo
  }

  /// Destructor.
  virtual ~PopulationDosedObjective(void)
  {
  }

  /// Assignment operator.
  PopulationDosedObjective &operator=(const PopulationDosedObjective &other)
  {
    if(this!=&other)
      {
	ObjectiveBase::operator=(other);
	myCurrentDosed=other.myCurrentDosed;
	myDoseThresholdIdx=other.myDoseThresholdIdx;
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
        myCurrentDosed=0.0;

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
    myCurrentDosed=0.0;

    myNetInfo = net;

    myLastComputedTimeStep=-1;
    myLastImpactTimeStep=-1;
  }

  ///
  virtual void resetForScenario(PNetInfo net, PNodeInfo nodes, PLinkInfo links,
				PSourceData sources)
  {
    ObjectiveBase::resetForScenario(net,nodes,links,sources);
    myCurrentDosed=0.0;

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
	updateNumberDosed(myLastImpactTimeStep);
	myLastComputedTimeStep=myLastImpactTimeStep;
      }

    updateImpacts(thisNodeIndex,myCurrentDosed,thisTimeStep*reportStep);
    
    updateSensorEvaluation(thisNodeIndex,myCurrentDosed);
  }

  ///
  virtual void addUndetectedImpact(int maxTimeStep,double reportStep)
  {
    // the threat simulator code uses INT_MAX as the
    // 'not detected' parameter value.
    if(myLastComputedTimeStep!=maxTimeStep)
      {
	updateNumberDosed(INT_MAX);
	myLastComputedTimeStep=maxTimeStep;
      }

    if (reportStep < 0.0) {
       updateImpacts(UNDETECTED_NODE_INDEX,0.0,-maxTimeStep*reportStep);
       updateSensorEvaluation(UNDETECTED_NODE_INDEX,0.0);
       }
    else {
       updateImpacts(UNDETECTED_NODE_INDEX,myCurrentDosed,maxTimeStep*reportStep);
       updateSensorEvaluation(UNDETECTED_NODE_INDEX,myCurrentDosed);
       }
  }

protected:

  /// The number of sickened for the current scenario.
  /// This defaults to 0.0.
  double myCurrentDosed;

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

  /// holds the index of the dose threshold we are using for this Dosed objective
  /// This value (for now) will be -1 for the stand-alone executable
  /// so the filename generated will be compatible with sp defaults.
  /// the output filename will have the suffix: pd_NN.impact, with NN replaced
  /// by the 2-digit index value.
  /// It will be between 0 and assessMem->dot->numThresh-1
  int myDoseThresholdIdx;

private:

  /// update the number dosed from the pre-computed HIA data
  void updateNumberDosed(int theNewTimeStep)
  {
	  int tidx=myDoseThresholdIdx==-1?0:myDoseThresholdIdx;
	  int totsteps = assessMem->dr!=NULL?assessMem->dr->maxsteps:assessMem->net->info->numSteps;

	  if (theNewTimeStep > totsteps){
     theNewTimeStep = assessMem->net->info->numSteps-1;
    }
	myCurrentDosed = assessMem->dot->numOverByTime[tidx][theNewTimeStep];
  }

};

#endif
