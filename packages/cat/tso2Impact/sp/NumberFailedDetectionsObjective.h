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

#ifndef tevasp_NumberFailedDetectsionObjective_h
#define tevasp_NumberFailedDetectsionObjective_h

#include "ObjectiveBase.h"


/// Trait/Functional class for the number of failed detections objective //
class NumberFailedDetectionsObjective : public ObjectiveBase
{
public:

   /// Constructor.
   NumberFailedDetectionsObjective(void)
   {
      ourImpactOutputFileSuffix = "nfd.impact";
      ourEvaluationOutputFileSuffix = "nfd.eval";
   }

   /// Copy constructor.
   NumberFailedDetectionsObjective(const NumberFailedDetectionsObjective &other):
         ObjectiveBase(other)
   {
      ourImpactOutputFileSuffix = "nfd.impact";
      ourEvaluationOutputFileSuffix = "nfd.eval";
   }

   /// Destructor.
   virtual ~NumberFailedDetectionsObjective(void)
   {
      // no dynamic memory to worry about
   }

   /// Assignment operator.
   NumberFailedDetectionsObjective &operator=(const NumberFailedDetectionsObjective &other)
   {
      if (this != &other)
      {
         ObjectiveBase::operator=(other);
      }
      return *this;
   }

   ///
   virtual void init(const std::string &outputPrefix, PNetInfo net, PNodeInfo nodes, PLinkInfo links, double detectionDelay, int numScenarios = -1)
   {
      ObjectiveBase::init(outputPrefix, net, nodes, links, detectionDelay, numScenarios);
   }

   ///
   virtual void resetForScenario(PNetInfo net, PNodeInfo nodes, PLinkInfo links,
                                 PSourceData sources)
   {
      ObjectiveBase::resetForScenario(net, nodes, links, sources);
   }

   ///
   virtual void updateImpactStatisticsDueToNode(int thisNodeIndex, double thisNodeQuality, PNetInfo net, PNodeInfo nodes, PLinkInfo links, int thisTimeStep, double reportStep)
   {
      // this is a no-op - if a node is hit, then the event is detected -
      // no compuation is required.
   }

   ///
   virtual void addNominalImpact(int thisNodeIndex, int thisTimeStep, double reportStep)
   {
      updateImpacts(thisNodeIndex, 0.0, thisTimeStep*reportStep); // 0.0 = false
      updateSensorEvaluation(thisNodeIndex, 0.0);
   }

   ///
   virtual void addUndetectedImpact(int maxTimeStep, double reportStep)
   {
      updateImpacts(UNDETECTED_NODE_INDEX, 1.0, maxTimeStep*reportStep); // 1.0 = true
      updateSensorEvaluation(UNDETECTED_NODE_INDEX, 1.0);
   }

};

#endif
