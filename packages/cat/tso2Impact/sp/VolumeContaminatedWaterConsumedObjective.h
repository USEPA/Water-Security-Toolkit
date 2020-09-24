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

#ifndef tevasp_VolumeContaminatedWaterConsumedObjective_h
#define tevasp_VolumeContaminatedWaterConsumedObjective_h

#include "ObjectiveBase.h"

/// Trait/Functional class for the volume of contamined water consumed objective
class VolumeContaminatedWaterConsumedObjective : public ObjectiveBase
{
public:

   /// Constructor.
   VolumeContaminatedWaterConsumedObjective(void):
         myAggregateConsumption(0.0)
   {
      ourImpactOutputFileSuffix = "vc.impact";
      ourEvaluationOutputFileSuffix = "vc.eval";
   }

   /// Copy constructor.
   VolumeContaminatedWaterConsumedObjective(const VolumeContaminatedWaterConsumedObjective &other):
         ObjectiveBase(other),
         myAggregateConsumption(other.myAggregateConsumption)
   {
      ourImpactOutputFileSuffix = "vc.impact";
      ourEvaluationOutputFileSuffix = "vc.eval";
   }

   /// Destructor.
   virtual ~VolumeContaminatedWaterConsumedObjective(void)
   {
      // no dynamic memory to worry about
   }

   /// Assignment operator.
   VolumeContaminatedWaterConsumedObjective &operator=(const VolumeContaminatedWaterConsumedObjective &other)
   {
      if (this != &other)
      {
         ObjectiveBase::operator=(other);
         myAggregateConsumption = other.myAggregateConsumption;
      }
      return *this;
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
         myAggregateConsumption += (thisNodeDemand * reportStep);
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

   /// units are in gallons. defaults to 0.0.
   double myAggregateConsumption;

};

#endif
