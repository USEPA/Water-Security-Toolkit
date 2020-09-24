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

#ifndef tevasp_TimeExtentOfContaminationObjective_h
#define tevasp_TimeExtentOfContaminationObjective_h

#include <map>
#include "ObjectiveBase.h"


/// Trait/Functional class for the extent-of-contamination objective //
class TimeExtentOfContaminationObjective : public ObjectiveBase
{
public:

   /// Constructor.
   TimeExtentOfContaminationObjective(void):
         myContaminatedPipeFeet(0.0)
   {
      ourImpactOutputFileSuffix = "tec.impact";
      ourEvaluationOutputFileSuffix = "tec.eval";
   }

   /// Copy constructor.
   TimeExtentOfContaminationObjective(const TimeExtentOfContaminationObjective &other):
         ObjectiveBase(other),
         myContaminatedPipeFeet(other.myContaminatedPipeFeet),
         myPipeLengths(other.myPipeLengths),
         myLinkHits(other.myLinkHits)
   {
      ourImpactOutputFileSuffix = "tec.impact";
      ourEvaluationOutputFileSuffix = "tec.eval";
   }


   /// Destructor.
   virtual ~TimeExtentOfContaminationObjective(void)
   {
      // no dynamic memory to worry about
   }

   /// Assignment operator.
   TimeExtentOfContaminationObjective &operator=(const TimeExtentOfContaminationObjective &other)
   {
      if (this != &other)
      {
         ObjectiveBase::operator=(other);
         myContaminatedPipeFeet = other.myContaminatedPipeFeet;
         myPipeLengths = other.myPipeLengths;
         myLinkHits = other.myLinkHits;
      }
      return *this;
   }

   ///
   virtual void init(const std::string &outputPrefix, PNetInfo net, PNodeInfo nodes, PLinkInfo links, double detectionDelay, int numScenarios = -1)
   {
      ObjectiveBase::init(outputPrefix, net, nodes, links, detectionDelay, numScenarios);
      myContaminatedPipeFeet = 0.0;
      myLinkHits = std::vector<bool>(net->numLinks, false);
   }

   ///
   void setPipeLengths(const std::map<std::pair<int, int>, double> &theLengths)
   {
      myPipeLengths = theLengths;
   }

   ///
   virtual void resetForScenario(PNetInfo net, PNodeInfo nodes, PLinkInfo links,
                                 PSourceData sources)
   {
      ObjectiveBase::resetForScenario(net, nodes, links, sources);
      myContaminatedPipeFeet = 0.0;
      for (size_t i = 0; i < myLinkHits.size(); i++)
      {
         myLinkHits[i] = false;
      }
   }

   virtual bool needLinkData() {
	   return true;
   }
   ///
   virtual void updateImpactStatisticsDueToLink(int thisLinkIndex, double thisLinkQuality, PNetInfo net, PNodeInfo nodes, PLinkInfo links, int thisTimeStep, double reportStep)
   {
      std::pair<int, int> key(std::make_pair(links[thisLinkIndex].from - 1, links[thisLinkIndex].to - 1));

      if (myPipeLengths.find(key) == myPipeLengths.end())
      {
            // skipping valve/pump/other dynamic link entries
      }
      else
      {
         myContaminatedPipeFeet += myPipeLengths[key];
      }
      myLinkHits[thisLinkIndex] = true;
   }

   ///
   virtual void addNominalImpact(int thisNodeIndex, int thisTimeStep, double reportStep)
   {
      updateImpacts(thisNodeIndex, myContaminatedPipeFeet, thisTimeStep*reportStep);

      updateSensorEvaluation(thisNodeIndex, myContaminatedPipeFeet);
   }

   ///
   virtual void addUndetectedImpact(int maxTimeStep, double reportStep)
   {
      if (reportStep < 0.0)
      {
         updateImpacts(UNDETECTED_NODE_INDEX, 0.0, -maxTimeStep*reportStep);
         updateSensorEvaluation(UNDETECTED_NODE_INDEX, 0.0);
      }
      else
      {
         updateImpacts(UNDETECTED_NODE_INDEX, myContaminatedPipeFeet, maxTimeStep*reportStep);
         updateSensorEvaluation(UNDETECTED_NODE_INDEX, myContaminatedPipeFeet);
      }
   }

private:

   /// The aggregate length of pipe that is currently contaminated.
   /// Units are in feet. defaults to 0.0.
   double myContaminatedPipeFeet;

   /// Provided externally - maps pipe indicies in the form of from/to TEVA
   /// node index pairs to pipe lengths, in feet.
   std::map<std::pair<int, int>, double> myPipeLengths;

   /// A map to indicate whether or not links (0-based TEVA indicies)
   /// have been "hit" during the current scenario.
   std::vector<bool> myLinkHits;

};

#endif
