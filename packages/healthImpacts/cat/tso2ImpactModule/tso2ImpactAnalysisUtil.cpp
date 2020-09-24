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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


#include <sp/impacts.h>

#include <sp/ObjectiveBase.h>
#include <PopulationExposedObjective.h>
#include <PopulationKilledObjective.h>
#include <PopulationDosedObjective.h>
#include <sp/NumberFailedDetectionsObjective.h>
#include <sp/TimeToDetectionObjective.h>
#include <sp/VolumeContaminatedWaterConsumedObjective.h>
#include <sp/ExtentOfContaminationObjective.h>

#include "tso2ImpactAnalysis.h"
#include "ModuleMassConsumedObjective.h"
#include "ModulePopulationExposedObjective.h"
#include "ModuleExtentOfContaminationObjective.h"
#include "ModuleVolumeContaminatedWaterConsumedObjective.h"
#include "ModuleTimeToDetectionObjective.h"
#include "ModulePopulationKilledObjective.h"
#include "ModulePopulationDosedObjective.h"
#include "ModuleNumberFailedDetectionsObjective.h"
extern "C" {

#include "loggingUtils.h"
#include "NamedData.h"
#include "HealthImpacts.h"

bool get_analysisOptions(NamedDataRef *analysisOptionsRef,
			 NamedDataRef *simResultsRef,
      		       std::string&outputFilePrefix,
      		       int&detectionDelay,
      		       double&minQuality,
      		       bool&mcSelected,
      		       bool&dmcSelected,
      		       bool&vcSelected,
      		       bool&dvcSelected,
      		       bool&nfdSelected,
      		       bool&tdSelected,
      		       bool&dtdSelected,
      		       bool&ecSelected,
      		       bool&decSelected,
      		       bool&pkSelected,
      		       bool&dpkSelected,
      		       bool&pdSelected,
      		       bool&dpdSelected,
      		       bool&peSelected,
      		       bool&dpeSelected,
		       PMem& mem,
		       ModuleType componentType)
{
	char     *tstr;
	char msg[256];
	JNIEnv *env = analysisOptionsRef->env;

	ANL_UTIL_LogFiner(env,"teva.analysis.server","*****************get_analysisOptions");

	if(getString(analysisOptionsRef, "outputFileRoot", &tstr)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get outputFileRoot\n");
		return false;
	}
	ANL_UTIL_LogFiner(env,"teva.analysis.server","*****************got file root");
	outputFilePrefix = tstr;
	ANL_UTIL_LogFiner(env,"teva.analysis.server","*****************set file prefix");
	if(getInt(analysisOptionsRef, "detectionDelay", 
		  &detectionDelay)==ND_FAILURE)  {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get detectionDelay");
		return false;
	}

	sprintf(msg,"*****************got detectionDelay: %d", detectionDelay);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	if(getDouble(analysisOptionsRef, "minQuality", 
		  &minQuality)==ND_FAILURE)  {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get minQuality");
		return false;
	}
	sprintf(msg,"*****************got minQuality: %f", minQuality);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	int mcs;
	if(getInt(analysisOptionsRef, "mcSelected", 
		  &mcs)==ND_FAILURE)  {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get mcSelected");
		return false;
	}
	mcSelected = (mcs != 0);
	sprintf(msg,"*****************got mcSelected: %d", mcs);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);
	if(getInt(analysisOptionsRef, "dmcSelected",
		  &mcs)==ND_FAILURE)  {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get dmcSelected");
		return false;
	}
	dmcSelected = (mcs != 0);
	sprintf(msg,"*****************got dmcSelected: %d", mcs);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	int vcs;
	if(getInt(analysisOptionsRef, "vcSelected", 
		  &vcs)==ND_FAILURE)  {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get vcSelected");
		return false;
	}
	vcSelected = (vcs != 0);
	sprintf(msg,"*****************got vcSelected: %d", vcs);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);
	if(getInt(analysisOptionsRef, "dvcSelected",
		  &vcs)==ND_FAILURE)  {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get dvcSelected");
		return false;
	}
	dvcSelected = (vcs != 0);
	sprintf(msg,"*****************got dvcSelected: %d", vcs);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	int nfds;
	if(getInt(analysisOptionsRef, "nfdSelected", 
		  &nfds)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get nfdSelected");
		return false;
	}
	nfdSelected = (nfds != 0);
	sprintf(msg,"*****************got nfdSelected: %d", nfds);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	int ecs;
	if(getInt(analysisOptionsRef, "ecSelected", 
		  &ecs)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get ecSelected");
		return false;
	}
	ecSelected = (ecs != 0);
	sprintf(msg,"*****************got ecSelected: %d", ecs);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);
	if(getInt(analysisOptionsRef, "decSelected",
		  &ecs)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get decSelected");
		return false;
	}
	decSelected = (ecs != 0);
	sprintf(msg,"*****************got decSelected: %d", ecs);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	int pks;
	if(getInt(analysisOptionsRef, "pkSelected", 
		  &pks)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get pkSelected");
		return false;
	}
	pkSelected = (pks != 0);
	sprintf(msg,"*****************got pkSelected: %d", pks);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);
	if(getInt(analysisOptionsRef, "dpkSelected",
		  &pks)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get dpkSelected");
		return false;
	}
	dpkSelected = (pks != 0);
	sprintf(msg,"*****************got dpkSelected: %d", pks);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	int pds;
	if(getInt(analysisOptionsRef, "pdSelected", 
		  &pds)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get pdSelected");
		return false;
	}
	pdSelected = (pds != 0);
	sprintf(msg,"*****************got pdSelected: %d", pds);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);
	if(getInt(analysisOptionsRef, "dpdSelected",
		  &pds)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get dpdSelected");
		return false;
	}
	dpdSelected = (pds != 0);
	sprintf(msg,"*****************got dpkSelected: %d", pds);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	int pes;
	if(getInt(analysisOptionsRef, "peSelected", 
		  &pes)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get peSelected");
		return false;
	}
	peSelected = (pes != 0);
	sprintf(msg,"*****************got peSelected: %d", pes);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);
	if(getInt(analysisOptionsRef, "dpeSelected",
		  &pes)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get dpeSelected");
		return false;
	}
	dpeSelected = (pes != 0);
	sprintf(msg,"*****************got dpeSelected: %d", pes);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

	int tds;
	if(getInt(analysisOptionsRef, "tdSelected", 
		  &tds)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get tdSelected");
		return false;
	}
	tdSelected = (tds != 0);
	sprintf(msg,"*****************got tdSelected: %d", tds);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);
	if(getInt(analysisOptionsRef, "dtdSelected",
		  &tds)==ND_FAILURE) {
		ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get dtdSelected");
		return false;
	}
	dtdSelected = (tds != 0);
	sprintf(msg,"*****************got dtdSelected: %d", tds);
	ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);


	if(peSelected || pkSelected || pdSelected || dpeSelected || dpdSelected || dpkSelected) {

		NamedDataRef *hiaData;
		if(getNamedData(analysisOptionsRef, "HIAParameters", &hiaData)==ND_FAILURE) {
			ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get HIAParameters named data");
			return false;
		}
		sprintf(msg,"*****************got HIAParameters %08x",hiaData);
		ANL_UTIL_LogFiner(env,"teva.analysis.server",msg);

		mem=loadHIAOptions(hiaData,simResultsRef,componentType);
		if(pdSelected) {
			// throw away values loaded from HIA parameters
			freeDoseOverThresholdData(mem);
			mem->dot=allocateDoseOverThresholdWithIDs(analysisOptionsRef,mem);
			// and use the ones in tso2impact parameters
		}
		ANL_UTIL_LogFiner(env,"teva.analysis.server","get_analysisOptions done");
	} else {
		int speciesIndex;
		if(getInt(analysisOptionsRef, "SpeciesIndex", &speciesIndex)==ND_FAILURE) {
			ANL_UTIL_LogSevere(env,"teva.analysis.server","*****************failed to get SpeciesIndex");
			return false;
		}
		mem=allocateBaseMemory(simResultsRef,speciesIndex,0);
	}
	return true;
}

PMem loadOptions(NamedDataRef *analysisOptionsRef, 
		 NamedDataRef *simResultsRef, ModuleType componentType,
      		 std::string&outputFilePrefix,
      		 int&detectionDelay,
      		 double&minQuality,
      		 bool&mcSelected,
      		 bool&dmcSelected,
      		 bool&vcSelected,
      		 bool&dvcSelected,
      		 bool&nfdSelected,
      		 bool&tdSelected,
      		 bool&dtdSelected,
      		 bool&ecSelected,
      		 bool&decSelected,
      		 bool&pkSelected,
      		 bool&dpkSelected,
      		 bool&pdSelected,
      		 bool&dpdSelected,
      		 bool&peSelected,
             bool&dpeSelected)
{
	JNIEnv *env = analysisOptionsRef->env;

	printf("*****************loadOptions\n");

	PMem       mem;

	if (!get_analysisOptions(analysisOptionsRef,
				 simResultsRef,
      		       outputFilePrefix,
      		       detectionDelay,
      		       minQuality,
      		       mcSelected,
      		       dmcSelected,
      		       vcSelected,
      		       dvcSelected,
      		       nfdSelected,
      		       tdSelected,
      		       dtdSelected,
      		       ecSelected,
      		       decSelected,
      		       pkSelected,
      		       dpkSelected,
      		       pdSelected,
      		       dpdSelected,
      		       peSelected,
      		       dpeSelected,
		       mem,
		       componentType))
		return NULL;

	ANL_UTIL_LogFiner(env,"teva.analysis.server","*****************loadOptions: done\n");
	return mem;
}

void freeMemory(PMem mem, ModuleType componentType) {

	freeHIAMemory(mem,componentType);
}

}; // extern "C"

// **********************************************************************
// **initTso2ImpactAfterTsoRead() ***************************************
// **********************************************************************
// **
// **      This function had to be duplicated in the tso2ImpactModule
// **      world in order to compile the standalone tso2Impact without 
// **      libANLUtils.so dependency.  In other words, this function
// **      can't be included in libsp.a because it would conflict with
// **      the version in libTso2ImpactModule.so. 
// **********************************************************************
void initTso2ImpactModuleData(
	NamedDataRef *analysisOptionsRef, 
	NamedDataRef *simResultsRef, 
	int& numNodes, clock_t&start, clock_t& stop, 
	PNodeInfo&nodes, PLinkInfo&links,
	PNetInfo&net, PTSO&tso,
	std::string&outputFilePrefix,
	int&detectionDelay,
	double&minQuality,
	bool&mcSelected,
	bool&dmcSelected,
	bool&vcSelected,
	bool&dvcSelected,
	bool&nfdSelected,
	bool&tdSelected,
	bool&dtdSelected,
	bool&ecSelected,
	bool&decSelected,
	bool&pkSelected,
	bool&dpkSelected,
	bool&pdSelected,
	bool&dpdSelected,
	bool&peSelected,
	bool&dpeSelected,
	PMem&assessMem,
	std::list<int>&sensorLocations,
	std::string&nodeMapFileName,
	std::string&scenarioMapFileName,
	std::ofstream&nodeMapFile,
	std::ofstream&scenarioMapFile,
	std::vector<ObjectiveBase*>&theObjectives,
        std::vector<std::string>&nodeIndexToIDMap,
        std::map<std::string,int>&nodeIDToIndexMap)

{
      ///////////////////////////////////////
      // initialize the various objectives //
      ///////////////////////////////////////
      std::cout << "initTso2ImpactAfterTsoRead" << std::endl;
//      std::cerr << net->nnodes << std::endl;
//      for (int i=0; i<net->nnodes; i++) {
//	std::cerr << i << " " << nodes[i].id << " " << nodes[i].q[0] << " " 
//	     << nodes[i].ntype << std::endl;
//      }

      if(mcSelected==true)
	{
	  ModuleMassConsumedObjective *mcObj=new ModuleMassConsumedObjective;
	  theObjectives.push_back(mcObj);
	}
      if(dmcSelected==true)
	{
	  ModuleMassConsumedObjective *mcObj=new ModuleMassConsumedObjective;
	  mcObj->no_failure_penalty();
	  theObjectives.push_back(mcObj);
	}
      if(vcSelected==true)
	{
	  ModuleVolumeContaminatedWaterConsumedObjective *vcObj=new ModuleVolumeContaminatedWaterConsumedObjective;
	  theObjectives.push_back(vcObj);
	}
      if(dvcSelected==true)
	{
	  ModuleVolumeContaminatedWaterConsumedObjective *vcObj=new ModuleVolumeContaminatedWaterConsumedObjective;
	  vcObj->no_failure_penalty();
	  theObjectives.push_back(vcObj);
	}
      if(nfdSelected==true)
	{
	  ModuleNumberFailedDetectionsObjective *nfdObj=new ModuleNumberFailedDetectionsObjective;
	  theObjectives.push_back(nfdObj);
	}
      if(tdSelected==true)
	{
	  ModuleTimeToDetectionObjective *tdObj=new ModuleTimeToDetectionObjective;
	  theObjectives.push_back(tdObj);
	}
      if(dtdSelected==true)
	{
	  ModuleTimeToDetectionObjective *tdObj=new ModuleTimeToDetectionObjective;
	  tdObj->no_failure_penalty();
	  theObjectives.push_back(tdObj);
	}
	  std::map<std::pair<int,int>,double> pipeLengthsIndexBased;
std::cout << "before setting pipe lengths1" << std::endl;
      if(ecSelected==true || decSelected==true)
	{
	  
	  int *linkStartIndices;
	  int *linkEndIndices;
	  float *linkLengths;

	  if(getIntArray(simResultsRef, "linkStartNodeIdx", &linkStartIndices)==ND_FAILURE) 
		return;
	  if(getIntArray(simResultsRef, "linkEndNodeIdx", &linkEndIndices)==ND_FAILURE) 
		return;
	  if(getFloatArray(simResultsRef, "linkLengths", &linkLengths)==ND_FAILURE) 
		return;

	  for(int i=0;i<net->numLinks;i++) {
	    // by convention, valves and pumps have lengths equal to 0 in the TSO prologue
	    if(linkLengths[i]>0)
	      {
		pipeLengthsIndexBased[std::make_pair(int(linkStartIndices[i]-1),int(linkEndIndices[i]-1))]=linkLengths[i];
	      }
	  }
	  nd_free(linkStartIndices);
	  nd_free(linkEndIndices);
	  nd_free(linkLengths);
	}
    if(ecSelected==true) {
 	  ModuleExtentOfContaminationObjective *ecObj=new ModuleExtentOfContaminationObjective;
 	  ecObj->setPipeLengths(pipeLengthsIndexBased);
 	  theObjectives.push_back(ecObj);
 	}
    if(decSelected==true) {
 	  ModuleExtentOfContaminationObjective *ecObj=new ModuleExtentOfContaminationObjective;
 	  ecObj->setPipeLengths(pipeLengthsIndexBased);
 	  ecObj->no_failure_penalty();
 	  theObjectives.push_back(ecObj);
 	}
    if(pkSelected==true)
	{
	  ModulePopulationKilledObjective *pkObj=new ModulePopulationKilledObjective(assessMem);
	  theObjectives.push_back(pkObj);
	}
    if(dpkSelected==true)
	{
	  ModulePopulationKilledObjective *pkObj=new ModulePopulationKilledObjective(assessMem);
	  pkObj->no_failure_penalty();
	  theObjectives.push_back(pkObj);
	}
    if(pdSelected==true)
	{
    	  for(int i=0;i<assessMem->dot->numThresh;i++) {
    		  ModulePopulationDosedObjective *pdObj=new ModulePopulationDosedObjective(assessMem,i);
    		  theObjectives.push_back(pdObj);
    	  }
	}
    if(dpdSelected==true)
	{
       for(int i=0;i<assessMem->dot->numThresh;i++) {
          ModulePopulationDosedObjective *pdObj=new ModulePopulationDosedObjective(assessMem,i);
          pdObj->no_failure_penalty();
          theObjectives.push_back(pdObj);
      }
	}
    if(peSelected==true)
	{
	  ModulePopulationExposedObjective*peObj = new ModulePopulationExposedObjective(assessMem);
	  theObjectives.push_back(peObj);
	}
    if(dpeSelected==true)
	{
	  ModulePopulationExposedObjective*peObj = new ModulePopulationExposedObjective(assessMem);
	  peObj->no_failure_penalty();
	  theObjectives.push_back(peObj);
	}

      //TNT added the number of simulations
	  int numEvents;
	  if(getInt(simResultsRef, "NumSimulations", &numEvents)==ND_FAILURE) 
		return;

      for(size_t i=0;i<theObjectives.size();i++)
	{
	  theObjectives[i]->setSensorLocations(sensorLocations);
	  theObjectives[i]->init(outputFilePrefix,net,nodes,links,detectionDelay,numEvents);
	}
      std::cout << "initTso2ImpactAfterTsoRead: done" << std::endl;

}

