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

#ifndef tevasp_ModuleVolumeContaminatedWaterConsumedObjective_h
#define tevasp_ModuleVolumeContaminatedWaterConsumedObjective_h

#include <sp/VolumeContaminatedWaterConsumedObjective.h>
#include "NamedData.h"
#include "ExternalAnalysis.h"


class ModuleVolumeContaminatedWaterConsumedObjective : public VolumeContaminatedWaterConsumedObjective
{
public:
   ModuleVolumeContaminatedWaterConsumedObjective(void) : VolumeContaminatedWaterConsumedObjective() 
   {
   }

   ModuleVolumeContaminatedWaterConsumedObjective(const ModuleVolumeContaminatedWaterConsumedObjective& other):
			VolumeContaminatedWaterConsumedObjective(other)
   {
   }

   ModuleVolumeContaminatedWaterConsumedObjective& operator=(
			const ModuleVolumeContaminatedWaterConsumedObjective &other)
   {
	if (this!=&other)
		VolumeContaminatedWaterConsumedObjective::operator=(other);
	return *this;
   }

   void constructAggregationServerData(void *aggData, PSourceData sourceData)
   {
	 // not using std::string since c_str() method returns const pointer
	 // that "addVariable can't handle.
	 char numImpactsStr[256];	// silly - why doesn't getIntArray
					// tell us the size of the array?
	 char witnessStr[256];
	 char tdStr[256];
	 char impactStr[256];
         sprintf(numImpactsStr, "%s_numImpacts",impactFilenameSuffix().c_str());
         sprintf(witnessStr, "%s_witnessID", impactFilenameSuffix().c_str());
         sprintf(tdStr, "%s_timeToDetection", impactFilenameSuffix().c_str());
         sprintf(impactStr, "%s_impact", impactFilenameSuffix().c_str());
	
	 NamedDataRef *aggrData = (NamedDataRef*) aggData;
	 addVariable(aggrData, (char*)numImpactsStr, INT_TYPE);
	 addVariable(aggrData, (char*)witnessStr, INT_ARRAY_TYPE);
	 addVariable(aggrData, (char*)tdStr,      INT_ARRAY_TYPE);
	 addVariable(aggrData, (char*)impactStr,  DOUBLE_ARRAY_TYPE);
	 int sz = myImpacts.size();
	 int *witnessID 	= new int[sz];
	 int *timeOfDetection 	= new int[sz];
	 double *impact 	= new double[sz];

         #ifdef IMPOSE_IMPACT_NODE_ORDER
         std::sort(myImpacts.begin(),myImpacts.end(),std::less<Impact>());
         #endif
	 if(setInt(aggrData,(char*)numImpactsStr, sz)==ND_FAILURE)
	      return;

         std::list<Impact>::iterator imp = myImpacts.begin();

         int i=0;
         while (imp != myImpacts.end()){
           witnessID[i] = imp->nodeID;
           timeOfDetection[i] = (int) imp->timeOfDetection;
           impact[i] = imp->impact;
           imp++;
           i++;
         }

	 setIntArray(aggrData,    (char*)witnessStr, witnessID, sz);
	 setIntArray(aggrData,    (char*)tdStr, timeOfDetection, sz);
	 setDoubleArray(aggrData, (char*)impactStr, impact, sz);
	 delete [] witnessID;
	 delete [] timeOfDetection;
	 delete [] impact;
   }
		
};

#endif
