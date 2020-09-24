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

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include <cmath>
#include <fstream>
#include <sp/scenario.h>
#include "sp/MassConsumedObjective.h"
#include "PopulationExposedObjective.h"
#include "PopulationKilledObjective.h"
#include "PopulationDosedObjective.h"
#include "sp/NumberFailedDetectionsObjective.h"
#include "sp/TimeToDetectionObjective.h"
#include "sp/VolumeContaminatedWaterConsumedObjective.h"
#include "sp/ExtentOfContaminationObjective.h"
#include "Detection.h"

extern "C"
{


}

const std::string NODE_MAP_FILE_SUFFIX = "nodemap";
const std::string SCENARIO_MAP_FILE_SUFFIX = "scenariomap";
char emsg[STRMAX];


void processScenario(std::vector<ObjectiveBase*> &theObjectives,
                     int thisScenarioIndex,
                     double detectionDelay,
                     int detectionConfidence,
                     double minimumQuality,
                     PNetInfo net, PNodeInfo nodes, PLinkInfo links,
                     PSourceData source, void *aggrInput, int speciesIndex)
{
   Detection *det = NULL;

   if (detectionConfidence > 1)
   {
      // detection occurs at a node only after N nodes (including the source
      // node) have detected the contaminant
      det = new DetectionNWay(detectionConfidence);
   }
   else
   {
      // detection occurs at a node when water quality exceeds minimumQuality
      det = new DetectionMinQuality(minimumQuality);
   }

   det->SetSources(source);
   det->SetSpeciesIndex(speciesIndex);
   det->SetNodes(nodes);
   det->SetNetwork(net);
   det->Calculate();

   // detection delay units are assumed to be in minutes

   double reportStep((int)((net->stepSize)*60)); // units are in minutes

   int detectionDelaySteps = int(ceil(double(detectionDelay) / reportStep));

   // track this for verification purposes - we should always have at least 2:
   // one for the source node index and one for the dummy node .
   int numEntries(0);

   // initialize the objectives for this scenario computation.
   bool needLinkData=false;
   for (std::vector<ObjectiveBase*>::iterator iter = theObjectives.begin(); iter != theObjectives.end(); iter++)
   {
      (*iter)->resetForScenario(net, nodes, links, source);
      needLinkData |= (*iter)->needLinkData();
   }

   // step through the contaminant concentration time-series, updating
   // the global/aggregate impact for each objective at each step.

   float**c = net->qualResults->nodeC[speciesIndex];
   for (int i = 0; i < net->numSteps; i++)
   {
      // skip the initial water quality assignments - the 0-impact cases for
      // source nodes are handled by the detection object and will given to
      // addNominalImpact

      if (i > 0)
      {
         // update aggregate node impacts incurred during the previous report step.
         for (int j = 0; j < net->numNodes; j++)
         {
            double thisQuality(c[j][i]);

            // TODO: what should we do if we get a negative quality value?
            if (thisQuality > 0.0)
            {
               for (std::vector<ObjectiveBase*>::iterator iter = theObjectives.begin(); iter != theObjectives.end(); iter++)
               {
                  (*iter)->updateImpactStatisticsDueToNode(j, thisQuality, net, nodes, links, i, reportStep);
               }
            }
         }

		 float**c = net->qualResults->nodeC[speciesIndex];
		 float**flow = net->hydResults->flow;

         if(needLinkData) {
            // update aggregate edge impacts incurred during the previous report step.
            for (int j = 0; j < net->numLinks; j++)
            {
               int fromNodeIndex = links[j].from - 1;
               int toNodeIndex = links[j].to - 1;

               double fromNodeQuality = c[fromNodeIndex][i];
               double toNodeQuality = c[toNodeIndex][i];

               double thisQuality =
                  ((fromNodeQuality > 0.0) && (flow[i][j] > 0.0)) ? fromNodeQuality :
                  (((toNodeQuality > 0.0) && (flow[i][j] < 0.0)) ? toNodeQuality : 0.0);

               if (thisQuality > 0.0)
               {
                  for (std::vector<ObjectiveBase*>::iterator iter = theObjectives.begin(); iter != theObjectives.end(); iter++)
                  {
                     (*iter)->updateImpactStatisticsDueToLink(j, thisQuality, net, nodes, links, i, reportStep);
                  }
               }
            }
         }
      }

      // Get list of nodes that were first contaminated at timestep i - detectionDelaySteps
      // and note their impact due to possibly delayed response

      int trueContaminationTime = i - detectionDelaySteps;
      int numSteps = int(det->detectionNodeList.size());

      if ((trueContaminationTime >= 0) && (numSteps >= trueContaminationTime - 1))
      {

         std::vector<int> detNodes = det->detectionNodeList[trueContaminationTime];

         if (detNodes.size() > 0)
         {
            std::sort(detNodes.begin(), detNodes.end());
            for (std::vector<ObjectiveBase*>::iterator iter = theObjectives.begin(); iter != theObjectives.end(); iter++)
            {
               ObjectiveBase *thisObjective = (*iter);
               for (int j = 0; j < int(detNodes.size()); j++)
               {
                  thisObjective->addNominalImpact(detNodes[j], trueContaminationTime, reportStep);
               }
            }
            numEntries += int(detNodes.size());
         }
      }
   }

   // if detection time plus response delay is greater than nsteps, include that now

   for (int trueContaminationTime = net->numSteps - detectionDelaySteps;
         trueContaminationTime < net->numSteps; trueContaminationTime++)
   {
      std::vector<int> detNodes = det->detectionNodeList[trueContaminationTime];
      if (detNodes.size() > 0)
      {
         std::sort(detNodes.begin(), detNodes.end());
         for (std::vector<ObjectiveBase*>::iterator iter = theObjectives.begin(); iter != theObjectives.end(); iter++)
         {
            ObjectiveBase *thisObjective = (*iter);

            for (int j = 0; j < int(detNodes.size()); j++)
            {
               thisObjective->addNominalImpact(detNodes[j], trueContaminationTime, reportStep);
            }
         }
      }
   }

   delete det;

   // add entries for the case where an event is undetected
   for (std::vector<ObjectiveBase*>::iterator iter = theObjectives.begin(); iter != theObjectives.end(); iter++)
   {
      // the 0th time-step is the set of initial conditions
      if ((*iter)->penalize_detection_failures)
         (*iter)->addUndetectedImpact(net->numSteps - 1, reportStep);
      else
         (*iter)->addUndetectedImpact(net->numSteps - 1, -reportStep);
   }

   numEntries++;

   if ((numEntries < 2) && (detectionConfidence < 2))
   {
      std::cout << "***ERROR - Only one entry generated for event scenario index=" << source->source[0].sourceNodeIndex << std::endl;
   }

   // finalize the objectives for this scenario computation.
   for (size_t k = 0; k < theObjectives.size(); k++)
   {
      if (aggrInput)
      {
         theObjectives[k]->constructAggregationServerData(aggrInput,
               source);
      }
      else
      {
         theObjectives[k]->finalizeForScenario(thisScenarioIndex, source, speciesIndex);
      }
   }
}
