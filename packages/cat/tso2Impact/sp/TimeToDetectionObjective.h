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

#ifndef tevasp_TimeToDetectionObjective_h
#define tevasp_TimeToDetectionObjective_h

#include "ObjectiveBase.h"


/// Trait/Functional class for the time-to-detection objective //
class TimeToDetectionObjective : public ObjectiveBase
{
public:

   /// Constructor.
   TimeToDetectionObjective(void):
         myTotalTime(0.0),
         myStartTime(0.0)
   {
      ourImpactOutputFileSuffix = "td.impact";
      ourEvaluationOutputFileSuffix = "td.eval";
   }

   /// Copy constructor.
   TimeToDetectionObjective(const TimeToDetectionObjective &other):
         ObjectiveBase(other),
         myTotalTime(other.myTotalTime),
         myStartTime(other.myStartTime)
   {
      ourImpactOutputFileSuffix = "td.impact";
      ourEvaluationOutputFileSuffix = "td.eval";
   }

   /// Destructor.
   virtual ~TimeToDetectionObjective(void)
   {
      // no dynamic memory to worry about
   }

   /// Assignment operator.
   TimeToDetectionObjective &operator=(const TimeToDetectionObjective &other)
   {
      if (this != &other)
      {
         ObjectiveBase::operator=(other);
         myTotalTime = other.myTotalTime;
         myStartTime = other.myStartTime;
      }
      return *this;
   }

   ///
   virtual void init(const std::string &outputPrefix, PNetInfo net, PNodeInfo nodes, PLinkInfo links, double detectionDelay, int numScenarios = -1)
   {
      ObjectiveBase::init(outputPrefix, net, nodes, links, detectionDelay, numScenarios);
      myTotalTime = 0.0;
      myStartTime = 0.0;
      responseDelay = detectionDelay;
   }

   ///
   virtual void resetForScenario(PNetInfo net, PNodeInfo nodes, PLinkInfo links,
                                 PSourceData sources)
   {
      ObjectiveBase::resetForScenario(net, nodes, links, sources);
      myStartTime = sources->source[0].sourceStart / 60.0;
      myTotalTime = 0.0;
   }

   ///
   virtual void addNominalImpact(int thisNodeIndex, int thisTimeStep, double reportStep)
   {
      double relTime = thisTimeStep * reportStep - myStartTime;

      // WEH - The consensus amongst the developers is that td should not
      //       include the detection delay.  Perhaps this class should be
      //       generalized to compute both td and td+response-delay, but
      //       for now it appears that measure can be directly employed
      //       within the the SPOT sp script and GUI.
      //
      //// thisTimeStep is time of contamination, true impact is
      //// time of contamination plus detection delay
      //
      //relTime += responseDelay;

      updateImpacts(thisNodeIndex, relTime, thisTimeStep*reportStep);

      updateSensorEvaluation(thisNodeIndex, thisTimeStep*reportStep);
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
         myTotalTime = maxTimeStep * reportStep - myStartTime;
         updateImpacts(UNDETECTED_NODE_INDEX, myTotalTime, maxTimeStep*reportStep);
         updateSensorEvaluation(UNDETECTED_NODE_INDEX, myTotalTime);
      }
   }

private:

   /// The number of minutes since the event was initiated.
   /// The units are in minutes. defaults to 0.0.
   double myTotalTime;

   double myStartTime;

   double responseDelay;

};

#endif
