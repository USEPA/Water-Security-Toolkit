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

#ifndef tevasp_ObjectiveBase_h
#define tevasp_ObjectiveBase_h

#ifdef _MSC_VER
#include <algorithm>
#endif

#include <cassert>
#include <list>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <assert.h>
#include <assert.h>

#include <utilib/Any.h>

int read_impact_preamble(std::istream& istr, int& nnodes, int delay);

int read_next_impact(std::istream& istr, int eventIndex,
                     std::map<int, double>& next_event_impacts,
                     std::vector<std::pair<int, int> > &next_event_sequence,
                     int delayIndex);

#include <sp/Impact.h>
#include <sp/ObjectiveVisitor.h>
extern "C"
{
#include "toolkit.h"
#include "tevautil.h"
}


/// The common virtual base class for all objectives.
class ObjectiveBase
{
public:

   /// Constructor.
   ObjectiveBase(void):
         myScenarioIDFile(0),
         penalize_detection_failures(true),
         myScenarioImpactOutputFile(0),
         myScenarioCount(0),
         myImpactAtDetection(0.0),
         myAggregateImpactsAtDetection(0.0)
   {
   }

   /// Copy construtor.
   ObjectiveBase(const ObjectiveBase &other):
         myScenarioIDFile(0),
         penalize_detection_failures(true),
         mySensorLocations(other.mySensorLocations),
         myScenarioImpactOutputFile(0),
         myScenarioCount(other.myScenarioCount),
         myImpacts(other.myImpacts),
         myImpactAtDetection(other.myImpactAtDetection),
         myAggregateImpactsAtDetection(other.myAggregateImpactsAtDetection),
         scenario_aggr(other.scenario_aggr)
   {
   }

   /// Destructor.
   virtual ~ObjectiveBase(void)
   {
      if ((myScenarioImpactOutputFile != 0) || (myScenarioIDFile != 0))
         // This should have already been called by finalizeTheObjectives().
         // Perhaps this should be an error.
         finalize();

      myImpacts.clear();
   }

   /// Assignment operator.
   ObjectiveBase &operator=(const ObjectiveBase &other)
   {
      if (this != &other)
      {
         myScenarioImpactOutputFile = 0;
         myScenarioCount = other.myScenarioCount;
         myImpacts = other.myImpacts;
         myImpactAtDetection = other.myImpactAtDetection;
         myAggregateImpactsAtDetection = other.myAggregateImpactsAtDetection;
         mySensorLocations = other.mySensorLocations;
      }
      return *this;
   }

   /// Perform a generic update of the Objective data.
   /// Currently, this is used by health impact measures to provide
   /// an update of health impact statistics.
   /// By default, this method is not called in tso2Impact.
   virtual void update(utilib::Any) {}

   /// Set the sensor locations for this objective.
   void setSensorLocations(const std::list<int> &theLocations)
   {
      mySensorLocations = theLocations;
   }

   /// Initialize the impact structures, open the output file, etc.
   /// This method is called once, before individual scenario impacts
   /// are computed.
   // TNT: modified to write out the number of scenarios in the file (will not write it out if the value is -1).
   // Currently if an impact file is generated while running the simulations in TEVA, the number of simulations
   // will NOT be written to the file.  It will be written via tso2Impact and from TEVA if the tso2Impact
   // is run after the ensemble is run (as in sensor placement)
   virtual void init(const std::string &outputPrefix, PNetInfo net, PNodeInfo nodes, PLinkInfo links, double detectionDelay, int numScenarios = -1)
   {
      nnodes = net->numNodes;
      if (outputPrefix != "")
      {
         impactOutputFileName = outputPrefix + "_" + impactFilenameSuffix();
         myScenarioImpactOutputFile = new std::ofstream(impactOutputFileName.c_str());
         if (!(*myScenarioImpactOutputFile))
         {
            std::cerr << "***Failed to open output file=" << impactOutputFileName << std::endl;
            exit(1);
         }
         else
         {
            (*myScenarioImpactOutputFile) << net->numNodes
            << std::endl;
            /*  TNT ins, JWB del -- see comment in tso2Impact.cpp
            if(numScenarios == -1) {
               (*myScenarioImpactOutputFile) << std::endl;
            } else {
               (*myScenarioImpactOutputFile) << " " << numScenarios << std::endl;
            }
            */
            (*myScenarioImpactOutputFile) << 1 << " " << int(detectionDelay) << std::endl;
         }

         std::string scenarioIDFileName(outputPrefix + "_" + impactFilenameSuffix() + ".id");
         myScenarioIDFile = new std::ofstream(scenarioIDFileName.c_str());
         if (!(*myScenarioIDFile))
         {
            std::cerr << "***Failed to open output file=" << scenarioIDFileName << std::endl;
            exit(1);
         }
      }
      myScenarioCount = 0;
      //myImpacts.reserve(net->nnodes);
      myImpactAtDetection = -1.0;
      myAggregateImpactsAtDetection = 0.0;

#ifdef OBJECTIVE_VISITOR
      std::list<ObjectiveVisitor*>::iterator vcurr = visitors.begin();
      std::list<ObjectiveVisitor*>::iterator vend  = visitors.end();
      while (vcurr != vend)
      {
         (*vcurr)->impactOutputFileName = impactOutputFileName;
         vcurr++;
      }
#endif
   }

   ///
   virtual void resetForScenario()
   {
      myScenarioCount++;
      myImpactAtDetection = -1.0;
      myImpacts.clear();
   }

   /// Reset the local variables for impact compuations for a new event
   /// scenario.  The arguments are used by some objectives and not others.
   virtual void resetForScenario(PNetInfo net, PNodeInfo nodes, PLinkInfo links,
		   PSourceData source)
   {
      this->resetForScenario();
   }

   /// Finalize all data (e.g., output) for the current event.
   virtual void finalizeForScenario(int scenarioIndex, PSourceData sources, int speciesIndex)
   {
      outputScenarioImpactData(scenarioIndex, sources, speciesIndex);
      //outputScenarioEvaluationData(scenarioIndex,sources);
      myAggregateImpactsAtDetection += myImpactAtDetection;
   }

   /// Output the impacts for the current scenario.
   virtual void outputScenarioImpactData(int scenarioIndex, PSourceData sources, int speciesIndex)
   {
      if (myScenarioIDFile)
      {
         //
         // the internal node output order is sorted first on time-hit,
         // and then on node ID. the global order is based on impact
         // first, then node ID; UNDETECTED_NODE_INDEX node entries always appear
         // last.
         //
#ifdef IMPOSE_IMPACT_NODE_ORDER
         std::sort(myImpacts.begin(), myImpacts.end(), std::less<Impact>());
#endif
		 if (sources != NULL) {
           int sourceNodeIndex=-1;
		   int sourceNodeStart=-1;
		   // find the source node index by looking at the list of sources
		   // and finding the (hopefully only) one that is the source of the 
		   // 
           for (int i=0; i<sources->nsources; i++){
             // if this is the species we are detecting
             if(sources->source[i].speciesIndex==speciesIndex) {
		  	   if(sourceNodeIndex == -1) {
                 sourceNodeIndex=sources->source[i].sourceNodeIndex;
			   } else {
				   if(sourceNodeIndex != sources->source[i].sourceNodeIndex) {
					   std::cerr << "*** WARNING *** Multiple source node indices for scenario: "<< scenarioIndex << std::endl;
				   }
			   }
			   if(sourceNodeStart == -1) {
                 sourceNodeStart=sources->source[i].sourceStart;
			   } else {
				   if(sourceNodeStart != sources->source[i].sourceStart) {
				     // print out a warning...
					   std::cerr << "*** WARNING *** Multiple start times for scenario: "<< scenarioIndex << std::endl;
				   }
			   }
			 }
  		     if(sourceNodeIndex == -1 && sourceNodeStart == -1) {
               if(sources->nsources == 1) {
		  		 sourceNodeIndex=sources->source[i].sourceNodeIndex;
				 sourceNodeStart=sources->source[i].sourceStart;
		  	   } else {
                 std::cerr << "*** WARNING *** Multiple start times/source nodes for scenario: "<< scenarioIndex << std::endl;
			   }
			 }
		   }
           (*myScenarioIDFile) << scenarioIndex << " " << sourceNodeIndex << " " << sourceNodeStart << std::endl;
		 }
	  }

#ifdef OBJECTIVE_VISITOR
      std::list<ObjectiveVisitor*>::iterator vcurr = visitors.begin();
      std::list<ObjectiveVisitor*>::iterator vend  = visitors.end();
      while (vcurr != vend)
      {
         (*vcurr)->outputScenarioImpactData(myImpacts, scenarioIndex);
         vcurr++;
      }
#endif

      if (myScenarioImpactOutputFile)
      {
         std::list<Impact>::iterator curr = myImpacts.begin();
         std::list<Impact>::iterator end  = myImpacts.end();
         while (curr != end)
         {
            // NOTE: the current .impact file format doesn't handle multiple
            // source node indicies the scenario indicies are 1-based (along
            // with the node indicies, enforced upon impact entry creation)
            (*myScenarioImpactOutputFile) << scenarioIndex << " "
            << curr->nodeID
            << " " << curr->timeOfDetection << " "
            << curr->impact << std::endl;
            curr++;
         }
      }
   }

   virtual void constructAggregationServerData(void *aggrData, PSourceData sources)
   {
   }
   virtual void getAggregationResults(void *options, void *aggrData)
   {
   }
   virtual void processAggregationServerData(void *aggrData, PSourceData sources)
   {
   }
   virtual void initializeForAggregationServer(void *options, void *simResults)
   {
   }
   virtual void cleanupFromAggregationServer(void *options, void *simResults)
   {
   }

   /// Return the impact filename suffix for this objective.
   virtual const std::string &impactFilenameSuffix(void)
   {
      return ourImpactOutputFileSuffix;
   }

   /// Return the evaluation filename suffix for this objective.
   virtual const std::string &evaluationFilenameSuffix(void)
   {
      return ourEvaluationOutputFileSuffix;
   }

   /// Update impact statistics to reflect the indicated water quality at
   /// the indicated node.
   virtual void updateImpactStatisticsDueToNode(int thisNodeIndex, double thisNodeQuality, PNetInfo net, PNodeInfo nodes, PLinkInfo links, int thisTimeStep, double reportStep)
   {
      // default behavior is a no-op; not all objectives track node-related
      // impacts.
   }

   /// Update impact statistics to reflect the water quality measures at the
   /// link end-points.
   virtual void updateImpactStatisticsDueToLink(int thisLinkIndex, double thisLinkQuality, PNetInfo net, PNodeInfo nodes, PLinkInfo links, int thisTimeStep, double reportStep)
   {
      // default behavior is a no-op; not all objectives track link-related
      // impacts.
   }
   virtual bool needLinkData() {
	   return false;
   }

   /// Add an impact entry for the input node given the current scenario impact.
   virtual void addNominalImpact(int thisNodeIndex, int thisTimeStep, double reportStep) = 0;

   /// Add an impact entry for the case of an undetected event.
   virtual void addUndetectedImpact(int maxTimeStep, double reportStep) = 0;

   /// Geneate/write any cross-scenario statistics, close the output file,
   /// and perform any cleanup of memory.
   virtual void finalize(void)
   {
      if (myScenarioImpactOutputFile)
      {
         myScenarioImpactOutputFile->close();
         delete myScenarioImpactOutputFile;
         myScenarioImpactOutputFile = 0;
      }
      if (myScenarioIDFile)
      {
         myScenarioIDFile->close();
         delete myScenarioIDFile;
         myScenarioIDFile = 0;
      }
#ifdef OBJECTIVE_VISITOR
      std::list<ObjectiveVisitor*>::iterator vcurr = visitors.begin();
      std::list<ObjectiveVisitor*>::iterator vend  = visitors.end();
      while (vcurr != vend)
      {
         (*vcurr)->finalize();
         vcurr++;
      }
#endif
      myImpacts.clear();
   }

   ///
   enum {UNDETECTED_NODE_INDEX = -1};

   /// The output file to which the scenario information is
   /// written.
   std::ofstream *myScenarioIDFile;

   /// Updates the scenario impact entries.
   /// Node indicies are 0-based, translated into 1-based once impact
   /// entries are created.  This routine was originally protected, but
   /// the analysis module already knows the impact values and is not
   /// compiling aggregate statistics.  Therefore, it needs to call this
   /// routine directly rather than via addNominalImpact.
   virtual void updateImpacts(int thisNodeIndex, double thisImpact, double thisTime)
   {
      Impact newImpact;
      if (thisNodeIndex == UNDETECTED_NODE_INDEX)
      {
         newImpact.nodeID = UNDETECTED_NODE_INDEX;
      }
      else
      {
         newImpact.nodeID = thisNodeIndex + 1;
      }
      newImpact.impact = thisImpact;
      newImpact.timeOfDetection = thisTime;
      myImpacts.push_back(newImpact);
   }

   ///
   void no_failure_penalty()
   {
      penalize_detection_failures = 0;
      ourImpactOutputFileSuffix = "d" + ourImpactOutputFileSuffix;
      ourEvaluationOutputFileSuffix = "d" + ourEvaluationOutputFileSuffix;
   }

   ///
   bool penalize_detection_failures;

protected:

   //// Locations of sensors in the network.
   std::list<int> mySensorLocations;

   /// Returns true if there is a sensor at the indicated node?
   bool sensorAt(int theNodeIndex)const
   {
      std::list<int>::const_iterator iter = std::find(mySensorLocations.begin(), mySensorLocations.end(), theNodeIndex);
      return iter != mySensorLocations.end();
   }


   /// Update the sensor evaluation statistics.
   /// A node index equal to UNDETECTED_NODE_INDEX indicates that
   /// the event was not detected. Node indicies are assumed
   /// to be 0-based.
   virtual void updateSensorEvaluation(int thisNodeIndex, double thisImpact)
   {
      if (myImpactAtDetection == -1.0)
      {
         // this scenario hasn't been detected yet
         if ((sensorAt(thisNodeIndex) == true) ||
               (thisNodeIndex == UNDETECTED_NODE_INDEX))
         {
            myImpactAtDetection = thisImpact;
         }
      }
   }

   ///
   std::vector<std::vector<int> > computeScenarioAggregation()
   {
      std::vector<std::vector<int> > result;
      return result;
   }


protected:

   /// The output file to which the impacts are written.
   /// This defaults to 0, and we assume that this class
   /// can delete this object.
   std::ofstream *myScenarioImpactOutputFile;
   std::string impactOutputFileName;

   /// The number of scenarios processed thus far.
   /// This defaults to 0, indicating unassigned.
   int myScenarioCount;

   /// A list of impacts for an individual event.
   std::list<Impact> myImpacts;

   /// Network-wide impact at point of detection by
   /// first contaminated node with a sensor.
   /// This defaults to -1.0, indicating unassigned.
   /// This value is relative to the current scenario.
   double myImpactAtDetection;

   /// Aggregate of myImpactAtDetection over all of
   /// the scenarios encountered thus far.
   /// Thisdefaults to 0.0, indicating unassigned.
   double myAggregateImpactsAtDetection;

   ///
   int nnodes;

   ///
   bool scenario_aggr;

   ///
   std::string ourImpactOutputFileSuffix;

   ///
   std::string ourEvaluationOutputFileSuffix;

#ifdef OBJECTIVE_VISITOR
   ///
   std::list<ObjectiveVisitor*> visitors;
#endif

};

#endif
