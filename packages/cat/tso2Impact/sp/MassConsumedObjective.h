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

#ifndef tevasp_MassConsumedObjective_h
#define tevasp_MassConsumedObjective_h

#include <string>
#include <fstream>
#include "ObjectiveBase.h"

const double LITERS_PER_GAL = 3.7854;


/// Trait/Functional class for the mass consumed objective
class MassConsumedObjective : public ObjectiveBase
{
public:

   /// Constructor.
   MassConsumedObjective(void):
         myAggregateConsumption(0.0)
   {
      ourImpactOutputFileSuffix = "mc.impact";
      ourEvaluationOutputFileSuffix = "mc.eval";
   }

   /// Copy constructor.
   MassConsumedObjective(const MassConsumedObjective &other):
         ObjectiveBase(other),
         myAggregateConsumption(other.myAggregateConsumption)
   {
      ourImpactOutputFileSuffix = "mc.impact";
      ourEvaluationOutputFileSuffix = "mc.eval";
   }

   /// Assignment operator.
   MassConsumedObjective &operator=(const MassConsumedObjective &other)
   {
      if (this != &other)
      {
         ObjectiveBase::operator=(other);
         myAggregateConsumption = other.myAggregateConsumption;
      }
      return *this;
   }

   /// Destructor.
   virtual ~MassConsumedObjective(void)
   {
      // no dynamic memory to worry about
   }

   ///
   virtual void init(const std::string &outputPrefix, PNetInfo net, PNodeInfo nodes, PLinkInfo links, double detectionDelay, int numScenarios = -1)
   {
      ObjectiveBase::init(outputPrefix, net, nodes, links, detectionDelay, numScenarios);
      myAggregateConsumption = 0.0;
   }

   ///
   virtual void resetForScenario(PNetInfo net, PNodeInfo nodes, PLinkInfo links,
                                 PSourceData sources)
   {
      ObjectiveBase::resetForScenario(net, nodes, links, sources);
      myAggregateConsumption = 0.0;
   }

   ///
   virtual void updateImpactStatisticsDueToNode(int thisNodeIndex, double thisNodeQuality, PNetInfo net, PNodeInfo nodes, PLinkInfo links, int thisTimeStep, double reportStep)
   {
		if (thisTimeStep > 0) {
			thisTimeStep--;
		}
	   double thisNodeDemand(net->hydResults->demand[thisTimeStep][thisNodeIndex]);
      if ((thisNodeDemand > 0.0) && (nodes[thisNodeIndex].type != tank))
      {
         // conversion is liters->gallons
         myAggregateConsumption = myAggregateConsumption +
                                  (thisNodeQuality * LITERS_PER_GAL * thisNodeDemand * reportStep);
      }
   }

   ///
   virtual void addNominalImpact(int thisNodeIndex, int thisTimeStep, double reportStep)
   {
      updateImpacts(thisNodeIndex, myAggregateConsumption, thisTimeStep*reportStep);

      updateSensorEvaluation(thisNodeIndex, myAggregateConsumption);
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
         updateImpacts(UNDETECTED_NODE_INDEX, myAggregateConsumption, maxTimeStep*reportStep);
         updateSensorEvaluation(UNDETECTED_NODE_INDEX, myAggregateConsumption);
      }
   }

private:

   /// Units are in gallons. defaults to 0.0.
   double myAggregateConsumption;

};

#endif
