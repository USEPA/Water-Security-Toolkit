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

#ifndef tevasp_PopulationKilledObjective_h
#define tevasp_PopulationKilledObjective_h

#include "sp/ObjectiveBase.h"
extern "C" {
#include "teva-assess.h"
}


/// Trait/Functional class for the Murray et al. health impacts - 
/// fatalities - objective
class PopulationKilledObjective : public ObjectiveBase
{
public:

  // Constructor.
  PopulationKilledObjective(PMem mem):
    myCurrentFatalities(0.0),
    myLastComputedTimeStep(-1),
    myLastImpactTimeStep(-1)
  {
    assessMem = mem;
    myNetInfo = NULL;
    ourImpactOutputFileSuffix="pk.impact";
    ourEvaluationOutputFileSuffix="pk.eval";
  }

  // Copy constructor.
  PopulationKilledObjective(const PopulationKilledObjective &other):
    ObjectiveBase(other),
    myCurrentFatalities(other.myCurrentFatalities),
    myTAIFileName(other.myTAIFileName),
    myLastComputedTimeStep(other.myLastComputedTimeStep),
    myLastImpactTimeStep(other.myLastImpactTimeStep)
  {
    // we don't copy assessMem and myNetInfo
    ourImpactOutputFileSuffix="pk.impact";
    ourEvaluationOutputFileSuffix="pk.eval";
  }

  /// Destructor.
  virtual ~PopulationKilledObjective()
  {
  }

  // Assignment operator.
  PopulationKilledObjective &operator=(const PopulationKilledObjective &other)
  {
    if(this!=&other)
      {
	ObjectiveBase::operator=(other);
	myCurrentFatalities=other.myCurrentFatalities;
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
    ObjectiveBase::init(outputPrefix,net,nodes,links,detectionDelay,numScenarios);
    myCurrentFatalities=0.0;

    myLastComputedTimeStep=-1;
    myLastImpactTimeStep=-1;
  }

  ///
  virtual void resetForScenario(PNetInfo net, PNodeInfo nodes, PLinkInfo links,
				PSourceData sources)
  {
    ObjectiveBase::resetForScenario(net,nodes,links,sources);
    myCurrentFatalities=0.0;

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
	recompueNumFatalities(myLastImpactTimeStep);
	myLastComputedTimeStep=myLastImpactTimeStep;
      }

    updateImpacts(thisNodeIndex,myCurrentFatalities,thisTimeStep*reportStep);
    
    updateSensorEvaluation(thisNodeIndex,myCurrentFatalities);
  }

  ///
  virtual void addUndetectedImpact(int maxTimeStep,double reportStep)
  {
    // the threat simulator code uses INT_MAX as the
    // 'not detected' parameter value.
    if(myLastComputedTimeStep!=maxTimeStep)
      {
	recompueNumFatalities(INT_MAX);
	myLastComputedTimeStep=maxTimeStep;
      }

    if (reportStep < 0.0) {
       updateImpacts(UNDETECTED_NODE_INDEX,0.0,-maxTimeStep*reportStep);
       updateSensorEvaluation(UNDETECTED_NODE_INDEX,0.0);
    }
    else {
       updateImpacts(UNDETECTED_NODE_INDEX,myCurrentFatalities,maxTimeStep*reportStep);
       updateSensorEvaluation(UNDETECTED_NODE_INDEX,myCurrentFatalities);
    }
  }

protected:

  /// The number of fatalities for the current scenario.
  /// Defaults to 0.0.
  double myCurrentFatalities;

  /// The name of the TAI input file.
  /// Defaults to "".
  std::string myTAIFileName;
  
  /// The "mother" data structure for the health impacts code
  PMem assessMem;
  ///
  PNetInfo  myNetInfo;

  /// The last time-step for which health impacts were actually computed.
  /// This defaults to -1. This enables caching of results for
  /// all nodes at which detection occurs for a single time-step.
  int myLastComputedTimeStep;

  /// The last time-step for which an impact due to a non-zero
  /// quality concentration at a node was reported. The current 
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

  /// update the number of fatalities from the pre-computed HIA data
  void recompueNumFatalities(int theNewTimeStep)
  {
    if (theNewTimeStep > assessMem->dr->maxsteps){
     theNewTimeStep = assessMem->dr->maxsteps-1;
    } else {
    	// this calculation works because this offsets the fatality data in xa->
    	// by the time from ingestion to response (nL) and then the time from
    	// reponse to fataility (nF)
    	theNewTimeStep=theNewTimeStep+assessMem->dr->nL+assessMem->dr->nF;
    }
	myCurrentFatalities = assessMem->xa->f[theNewTimeStep];
  }

};

#endif
