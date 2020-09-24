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
#include "version.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef TEVA_SPOT_HAVE_UNISTD_H
#include <unistd.h>
#endif
extern "C"
{
#include "tai.h"
}
#include <sp/scenario.h>
#include <utilib/exception_mngr.h>
#include <utilib/stl_auxiliary.h>
#include <utilib/OptionParser.h>
#include "sp/impacts.h"
#include "sp/MassConsumedObjective.h"
#include "PopulationExposedObjective.h"
#include "PopulationKilledObjective.h"
#include "PopulationDosedObjective.h"
#include "sp/NumberFailedDetectionsObjective.h"
#include "sp/TimeToDetectionObjective.h"
#include "sp/VolumeContaminatedWaterConsumedObjective.h"
#include "sp/ExtentOfContaminationObjective.h"
#include "sp/TimeExtentOfContaminationObjective.h"

#if defined(WIN32) && !defined(S_ISDIR)
#define __S_ISTYPE(mode, mask)   (((mode) & S_IFMT) == (mask))
#define S_ISDIR(mode)    __S_ISTYPE((mode), S_IFDIR)
#endif

#if 0
void debug()
{
   std::list<double> tmp;
   cin >> tmp;
   cout << tmp;
//utilib::AnyReference foo;
//foo = tmp;
//cin >> foo;
//cout << foo;
}
#endif


inline utilib::UnPackBuffer& operator>> (utilib::UnPackBuffer& buff, Mem* data)        {return buff;}

inline utilib::PackBuffer& operator<< (utilib::PackBuffer& buff, Mem* data)
{return buff;}

inline istream& operator>> (istream& iss, Mem* data)        {return iss;}

inline ostream& operator<< (ostream& os, Mem* data)
{return os;}


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
void SetupObjectives(
   PNodeInfo&nodes,
   PLinkInfo&links,
   PNetInfo&net,
   PTSO&tso,
   PERD&erd,
   std::string&outputFilePrefix,
   int&detectionDelay,
   bool&mcSelected,
   bool&dmcSelected,
   bool&vcSelected,
   bool&dvcSelected,
   bool&nfdSelected,
   bool&tdSelected,
   bool&dtdSelected,
   bool&ecSelected,
   bool&tecSelected,
   bool&decSelected,
   std::string&dvfFileName,
   bool&pkSelected,
   bool&dpkSelected,
   bool&pdSelected,
   bool&dpdSelected,
   bool&peSelected,
   bool&dpeSelected,
   std::string&taiInputFilename,
   PMem& assessMem,
   std::list<int>&sensorLocations,
   std::vector<ObjectiveBase*>&theObjectives,
   std::map<std::string, int>&nodeIDToIndexMap
)

{
   ///////////////////////////////////////
   // initialize the various objectives //
   ///////////////////////////////////////

   if (mcSelected == true)
   {
      MassConsumedObjective *mcObj = new MassConsumedObjective;
      theObjectives.push_back(mcObj);
   }
   if (dmcSelected == true)
   {
      MassConsumedObjective *mcObj = new MassConsumedObjective;
      mcObj->no_failure_penalty();
      theObjectives.push_back(mcObj);
   }
   if (vcSelected == true)
   {
      VolumeContaminatedWaterConsumedObjective *vcObj = new VolumeContaminatedWaterConsumedObjective;
      theObjectives.push_back(vcObj);
   }
   if (dvcSelected == true)
   {
      VolumeContaminatedWaterConsumedObjective *vcObj = new VolumeContaminatedWaterConsumedObjective;
      vcObj->no_failure_penalty();
      theObjectives.push_back(vcObj);
   }
   if (nfdSelected == true)
   {
      NumberFailedDetectionsObjective *nfdObj = new NumberFailedDetectionsObjective;
      theObjectives.push_back(nfdObj);
   }
   if (tdSelected == true)
   {
      TimeToDetectionObjective *tdObj = new TimeToDetectionObjective;
      theObjectives.push_back(tdObj);
   }
   if (dtdSelected == true)
   {
      TimeToDetectionObjective *tdObj = new TimeToDetectionObjective;
      tdObj->no_failure_penalty();
      theObjectives.push_back(tdObj);
   }
   if ((ecSelected == true) || (decSelected == true) || (tecSelected == true))
   {

      std::map<std::pair<int, int>, double> pipeLengthsIndexBased;

      std::cout << "Extracting pipe lengths from TSO/ERD prologoue" << std::endl << std::endl;

      for (int i = 0; i < net->numLinks; i++)
      {
         // by convention, valves and pumps have lengths equal to 0 in the TSO prologue
         if (links[i].length > 0)
         {
            pipeLengthsIndexBased[std::make_pair(int(links[i].from - 1), int(links[i].to - 1))] = links[i].length;
         }
      }

      if (ecSelected)
      {
         ExtentOfContaminationObjective *ecObj = new ExtentOfContaminationObjective;
         ecObj->setPipeLengths(pipeLengthsIndexBased);
         theObjectives.push_back(ecObj);
      }
      if (tecSelected)
      {
         TimeExtentOfContaminationObjective *ecObj = new TimeExtentOfContaminationObjective;
         ecObj->setPipeLengths(pipeLengthsIndexBased);
         theObjectives.push_back(ecObj);
      }
      if (decSelected)
      {
         ExtentOfContaminationObjective *ecObj = new ExtentOfContaminationObjective;
         ecObj->setPipeLengths(pipeLengthsIndexBased);
         ecObj->no_failure_penalty();
         theObjectives.push_back(ecObj);
      }
   }
   if (pkSelected == true || peSelected == true || pdSelected == true || dpkSelected || dpeSelected || dpdSelected)
   {
      PInputFileData taiData = loadTAIFileData(taiInputFilename.c_str());
      assessMem = new Mem;
	  assessMem->tso=tso;
	  assessMem->erd=erd;
      assessMem->linkinfo = links;
      assessMem->nodeinfo = nodes;
      InitializeAssessMemory(assessMem, net);
      processTAIData(taiData, assessMem);
      freeInputFileData(&taiData);
      Population(assessMem->net, assessMem->popData, assessMem->node);
      initializeIngestionData(assessMem->ingestionData, assessMem->net, assessMem->node, assessMem->dr, assessMem->popData);

   }
   if (pkSelected == true)
   {
      PopulationKilledObjective *pkObj = new PopulationKilledObjective(assessMem);
      theObjectives.push_back(pkObj);
   }
   if (dpkSelected == true)
   {
      PopulationKilledObjective *pkObj = new PopulationKilledObjective(assessMem);
      pkObj->no_failure_penalty();
      theObjectives.push_back(pkObj);
   }
   if (peSelected == true)
   {
      PopulationExposedObjective *peObj = new PopulationExposedObjective(assessMem);
      theObjectives.push_back(peObj);
   }
   if (dpeSelected == true)
   {
      PopulationExposedObjective *peObj = new PopulationExposedObjective(assessMem);
      peObj->no_failure_penalty();
      theObjectives.push_back(peObj);
   }
   if (pdSelected == true)
   {
      if (assessMem->dot == NULL || assessMem->dot->numThresh == 0)
      {
         std::cerr << "***No DOSE_THRESHOLDS specified in input file: " << taiInputFilename.c_str() << std::endl;
         std::cerr << "   Required by --pd" << std::endl;
         exit(1);
      }
      else if (assessMem->dot->numThresh > 1)
      {
         std::cerr << "***Only one DOSE_THRESHOLD can be specified for --pd objective.  input file: " << taiInputFilename.c_str() << std::endl;
         exit(1);
      }
      PopulationDosedObjective *pdObj = new PopulationDosedObjective(assessMem);
      theObjectives.push_back(pdObj);
   }
   if (dpdSelected == true)
   {
      if (assessMem->dot == NULL || assessMem->dot->numThresh == 0)
      {
         std::cerr << "***No DOSE_THRESHOLDS specified in input file: " << taiInputFilename.c_str() << std::endl;
         std::cerr << "   Required by --dpd" << std::endl;
         exit(1);
      }
      else if (assessMem->dot->numThresh > 1)
      {
         std::cerr << "***Only one DOSE_THRESHOLD can be specified for --dpd objective.  input file: " << taiInputFilename.c_str() << std::endl;
         exit(1);
      }
      PopulationDosedObjective *pdObj = new PopulationDosedObjective(assessMem);
      pdObj->no_failure_penalty();
      theObjectives.push_back(pdObj);
   }

   for (size_t i = 0; i < theObjectives.size(); i++)
   {
      theObjectives[i]->setSensorLocations(sensorLocations);
      //TNT: Added the number of simulations that the tso file contains (to be written to the impact file)
	  theObjectives[i]->init(outputFilePrefix, net, nodes, links, detectionDelay, erd!=NULL?erd->qualSimCount:tso->count);
   }
}

void updateTheObjectives(
   PNodeInfo&nodes,
   PLinkInfo&links,
   PNetInfo&net,
   PTSO&tso,
   PERD&erd,
   bool reinitialize,
   std::string&taiInputFilename,
   PMem& assessMem,
   std::vector<ObjectiveBase*> theObjectives)
{
   ///////////////////////////////////////
   // for pe, pk, re-initialize the various objectives //
   ///////////////////////////////////////
   if (reinitialize)
   {
      free(assessMem);
      PInputFileData taiData = loadTAIFileData(taiInputFilename.c_str());
      assessMem = new Mem;
      assessMem->linkinfo = links;
      assessMem->nodeinfo = nodes;
      InitializeAssessMemory(assessMem, net);
      processTAIData(taiData, assessMem);
      freeInputFileData(&taiData);
      Population(assessMem->net, assessMem->popData, assessMem->node);
      initializeIngestionData(assessMem->ingestionData, assessMem->net, assessMem->node, assessMem->dr, assessMem->popData);

   }
   for (size_t i = 0; i < theObjectives.size(); i++)
   {
      utilib::Any av(assessMem);
      theObjectives[i]->update(av);
   }
}



/// Argument processing for the standalone version
void process_arguments(int argc, char** argv,
                       std::string&outputFilePrefix,
                       std::vector<std::string>&erdFiles,
                       int&detectionDelay,
                       int&detectionConfidence,
                       std::list<double>&minQuality,
                       bool&  mcSelected,
                       bool& dmcSelected,
                       bool&  vcSelected,
                       bool& dvcSelected,
                       bool& nfdSelected,
                       bool&  tdSelected,
                       bool& dtdSelected,
                       bool&  ecSelected,
                       bool& tecSelected,
                       bool& decSelected,
                       bool&  pkSelected,
                       bool& dpkSelected,
                       bool&  pdSelected,
                       bool& dpdSelected,
                       bool&  peSelected,
                       bool& dpeSelected,
                       std::string& species,
                       std::vector<std::string>&taiInputFilename,
                       std::string&sensorInputFilename,
                       std::list<int>&sensorLocations,
		       std::string&dvfFileName)
{
   utilib::OptionParser options;

   options.add_usage("tso2Impact [options] <output-prefix> <tso-directory-or-files> <tai-directory-or-files>");
   options.description = "An application that reads a TSO file (and associated TAI file if health impacts are to be computed) and creates one or more impact files that are used to formulate sensor placement problems.";


   options.add('d', "detectionLimit", minQuality, "A list of thresholds needed to perform detection with a sensor.  There must be one threshold for each .tso file. The units of these detection limits depend on the units of the contaminant simulated for each TSO file (e.g. number of cells of a biological agent).");

   options.add('r', "responseTime", detectionDelay, "This option indicates the number of minutes that are needed to respond to the detection of a continant.  As the response time increases, the impact increases because the contaminant affects the network for a greater length of time. Unit: minutes.");

   options.add("detectionConfidence", detectionConfidence, "The number of sensors that must detect an event before the impacts are calculated.  Normally this is 1 sensor.");

   options.add("mc", mcSelected, "If this option is specified, an impact file will be generated for the `mass consumed' measure.");
   options.add("dmc", dmcSelected, "If this option is specified, an impact file will be generated for the `detected mass consumed' measure.");

   options.add("vc", vcSelected, "If this option is specified, an impact file will be generated for the `volume consumed' measure.");
   options.add("dvc", dvcSelected, "If this option is specified, an impact file will be generated for the `detected volume consumed' measure.");

   options.add("td", tdSelected, "If this option is specified, an impact file will be generated for the `time-to-detection' measure.");
   options.add("dtd", dtdSelected, "If this option is specified, an impact file will be generated for the `detected time-to-detection' measure.");

   options.add("nfd", nfdSelected, "If this option is specified, an impact file will be generated for the `number-of-failed detections' measure.");

   options.add("ec", ecSelected, "If this option is specified, an impact file will be generated for the `extent of contamination' measure.");
   options.add("tec", tecSelected, "If this option is specified, an impact file will be generated for the `timed extent of contamination' measure.");
   options.add("dec", decSelected, "If this option is specified, an impact file will be generated for the `detected extent of contamination' measure.");

   options.add("pk", pkSelected, "If this option is specified, an impact file wll be generated for the `population killed' measure.  This is an intensive measure to compute.");

   options.add("dpk", dpkSelected, "If this option is specified, an impact file wll be generated for the `detected population killed' measure.  This is an intensive measure to compute.");

   options.add("pd", pdSelected, "If this option is specified, an impact file wll be generated for the `population dosed' measure.  This is an intensive measure to compute.");

   options.add("dpd", dpdSelected, "If this option is specified, an impact file wll be generated for the `detected population dosed' measure.  This is an intensive measure to compute.");

   options.add("pe", peSelected, "If this option is specified, an impact file wll be generated for the `population exposed' measure.  This is an intensive measure to compute.");

   options.add("dpe", dpeSelected, "If this option is specified, an impact file wll be generated for the `detected population exposed' measure.  This is an intensive measure to compute.");

   options.add("species", species, "Name of species to use in computing impacts.");

   options.add_argument("output-prefix", "The prefix used for all files generated by tso2Impact.");

   options.add_argument("erd-or-tso-file", "This argument indicates either a TSO or ERD file.");

   options.add_argument("tai-directory-or-file", "This argument indicates a TAI file name.  The TAI input file is a threat_assess input that specifies parameters like dosage, response, lethality, etc.  There should be one TAI file for each TSO file.");

   options.add("dvf", dvfFileName, "This is used to correct demands generated for flushing response calculations");

   options.epilog = "Note that options like `responseTime' can be specified with the syntax `--responseTime 10.0' or `--responseTime=10.0'.";

   std::string version = create_version("tso2Impact", __DATE__, __TIME__);
   options.version(version);
   utilib::OptionParser::args_t args = options.parse_args(argc, argv);

   if (options.help_option())
   {
      options.write(std::cout);
      exit(1);
   }
   if (options.version_option())
   {
      options.print_version(std::cout);
      exit(1);
   }

   //
   // Check for errors in the command-line arguments
   //
   if (args.size() < 2)
   {
      options.write(std::cerr);
      exit(1);
   }

   utilib::OptionParser::args_t::iterator curr = args.begin();
   utilib::OptionParser::args_t::iterator end  = args.end();
   curr++;
   outputFilePrefix = *curr;
   curr++;

   bool healthImpact = pkSelected || peSelected || pdSelected ||
                       dpkSelected || dpeSelected || dpdSelected;

   if (!healthImpact)
   {
      while (curr != end)
      {
         erdFiles.push_back(*curr);
         curr++;
      }
   }
   else
   {
      std::vector<std::string>fnames;
      while (curr != end)
      {
         fnames.push_back(*curr);
         curr++;
      }
      int nnames = fnames.size();

      if (nnames < 2)
      {
         std::cerr << "***At least one TSO and one TAI file are required" << std::endl;
         exit(1);
      }
      struct stat sbuf;
      int rc = stat(fnames[0].c_str(), &sbuf);

      if (S_ISDIR(sbuf.st_mode))
      {
         erdFiles.push_back(fnames[0]);
         for (int i = 1; i < nnames; i++)
         {
            taiInputFilename.push_back(fnames[i]);
         }
      }
      else
      {
         int neach = static_cast<int>(nnames / 2.0);

         if ((nnames & 1) != 0)
         {
            std::cerr << "***One TAI input file required for each TSO file" << std::endl;
            exit(1);
         }

         for (int i = 0; i < neach; i++)
         {
            erdFiles.push_back(fnames[i]);
            taiInputFilename.push_back(fnames[i+neach]);
         }
      }
   }
#if 0

   while (curr != end)
   {
      tsoFileOrDirectoryName.push_back(*curr);
      curr++;
   }

#endif
   if (mcSelected)
      std::cout << "Generating mass consumed impact file" << std::endl;
   if (dmcSelected)
      std::cout << "Generating detected mass consumed impact file" << std::endl;
   if (vcSelected)
      std::cout << "Generating volume of contaminated water consumed impact file" << std::endl;
   if (dvcSelected)
      std::cout << "Generating detected volume of contaminated water consumed impact file" << std::endl;
   if (nfdSelected)
      std::cout << "Generating number of failed detections impact file" << std::endl;
   if (tdSelected)
      std::cout << "Generating time to detection impact file" << std::endl;
   if (dtdSelected)
      std::cout << "Generating detected time to detection impact file" << std::endl;
   if (ecSelected)
      std::cout << "Generating extent of contamination impact file" << std::endl;
   if (tecSelected)
      std::cout << "Generating timed extent of contamination impact file" << std::endl;
   if (decSelected)
      std::cout << "Generating detected extent of contamination impact file" << std::endl;

   if (pkSelected)
      std::cout << "Generating population killed impact file" << std::endl;
   if (dpkSelected)
      std::cout << "Generating detected population killed impact file" << std::endl;
   if (pdSelected)
      std::cout << "Generating population dosed impact file" << std::endl;
   if (dpdSelected)
      std::cout << "Generating detected population dosed impact file" << std::endl;
   if (peSelected)
      std::cout << "Generating population exposed impact file" << std::endl;
   if (dpeSelected)
      std::cout << "Generating detected population exposed impact file" << std::endl;

   if (detectionDelay < 0)
   {
      std::cerr << "***The response time must be non-negative" << std::endl;
      exit(1);
   }
   std::cout << "Detection delay (in minutes)=" << detectionDelay << std::endl;

#if 0
   else if (strcmp(argv[currentArgIndex], "-minQuality") == 0)
   {
      currentArgIndex++;
      while (currentArgIndex < argc && argv[currentArgIndex][0] != '-')
      {
         double this_qual = atof(argv[currentArgIndex++]);
         std::cout << "next qual (cai:" << currentArgIndex << "): "
                   << this_qual << std::endl;
         if (this_qual < 0.0)
         {
            std::cerr << "***The minimum water quality must be "
                      "non-negative" << std::endl;
            exit(1);
         }
         minQuality.push_back(this_qual);
      }
      if (currentArgIndex < argc && argv[currentArgIndex][0] == '-')
         currentArgIndex--;
      std::cout << "Water quality threshold(s)=[";
      for (int i = 0; i < minQuality.size(); i++)
      {
         std::cout << minQuality[i] << " ";
      }
      std::cout << std::endl;
   }
#endif

   ////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////
   // read in the sensor locations, if any have been specified //
   // NOTE: the indicies in the input file are 1-based, and    //
   //       are converted to 0-based indicies for internal use //
   // NOTE: the program assumes the indices in the input file  //
   //       are identical to those found in the tso2Impact     //
   //       nodemap output file.                               //
   //////////////////////////////////////////////////////////////

#if 0
   if (sensorInputFilename != "")
   {
      std::ifstream sensorInputFile(sensorInputFilename.c_str());
      if (!sensorInputFile)
      {
         std::cerr << "***Failed to open sensor input file=" << sensorInputFilename << std::endl;
         exit(1);
      }

      for (;;)
      {
         int thisSensorIndex;
         sensorInputFile >> thisSensorIndex;
         if (!sensorInputFile)
         {
            break;
         }
         sensorLocations.push_back(thisSensorIndex - 1);
      }

      std::cout << "Number of sensor locations=" << sensorLocations.size() << std::endl;
      sensorInputFile.close();
   }
#endif

   //////////////////////////////////////////////////////////////
}

void modifyTsoInfo(
   int& numNodes,
   PNodeInfo& nodes,
   PLinkInfo& links,
   PNetInfo& net,
   const std::string& dvfFileName)
{
  if (dvfFileName.size() != 0) {
     FILE *decfile = NULL;
     int prnDebugFile, detectTime, responseDelay, pipeDelay, flushLen;
     float flushrate, currate;
     float cvtime, ovtime;
     int ival, cidx, pidx, nidx, nFlushNodes, nClosePipes, nodeID;
     int *flushNodes, *closePipes;
     decfile = fopen((char*)dvfFileName.c_str(), "r");
     // read in decision variables
     fscanf(decfile,"%d",&prnDebugFile);
     fscanf(decfile,"%d",&detectTime);
     fscanf(decfile,"%d",&responseDelay);
     fscanf(decfile,"%d",&pipeDelay);
     fscanf(decfile,"%f",&flushrate);
     fscanf(decfile,"%d",&flushLen);
     fscanf(decfile,"%d",&ival);
     // Read decision variables for flushing
     fscanf(decfile,"%d",&nFlushNodes);
     flushNodes = (int*)calloc(1+nFlushNodes,sizeof(int));
     if(!flushNodes){
       printf("ERROR\n");
       printf("Insufficient memory to load decision variables for\n");
       printf("analysis. Check your system memory.\n");
       return;
     } 
     for (pidx = 0; pidx < nFlushNodes; pidx++)
       {
	 fscanf(decfile,"%d",&ival);
	 flushNodes[pidx] = ival;
       }
     // Read decision variables for closing pipes
     fscanf(decfile,"%d",&nClosePipes);
     closePipes = (int*)calloc(1+nClosePipes,sizeof(int));
     if(!closePipes){
       printf("ERROR\n");
       printf("Insufficient memory to load decision variables for\n");
       printf("analysis. Check your system memory.\n");
       return;
     } 
     for (pidx = 0; pidx < nClosePipes; pidx++)
       {
	 fscanf(decfile,"%d",&ival);
	 closePipes[pidx] = ival;
       }
     fclose(decfile);
     flushLen = flushLen;
     responseDelay = responseDelay ;
     pipeDelay = pipeDelay;
     detectTime = detectTime;
     cidx = 0;
     pidx = 1;
     cvtime = (float)((detectTime+responseDelay)/net->stepSize); // TODO: Check this - is this for pipes or nodes?
     ovtime = (float)((detectTime+responseDelay+flushLen)/net->stepSize);
     std::cout << "Demands modified between " << cvtime << " and " << ovtime << std::endl;
     std::cout << "Adjusting node hydraulic information" << std::endl << std::endl;
	 float **demands=net->hydResults->demand;
     for ( nidx = 0; nidx < nFlushNodes; nidx++)
       {
	 nodeID = flushNodes[nidx];
	 if (nodeID-1 < numNodes) {
	 std::cout << "Node (index=" << nodeID << ",ID=" << nodes[nodeID-1].id << ") ";
	 for (int j = 0; j < net->numSteps; j++)
	   {
	     if ( j >= (int) cvtime && j < (int) ovtime ) {
	       currate = demands[j][nodeID-1];
	       if (currate > 0) {
		 currate = currate - flushrate;
		 demands[j][nodeID-1] = currate;
	       }
	     }
	     std::cout << demands[j][nodeID-1] << " ";
	   }
	 std::cout << std::endl;
	 }
       }
     std::cout << std::endl;
   }
}

int readDBInfo(
   const std::string& outputFilePrefix,
   const std::string& dbFile,
   int& numNodes,
   PNodeInfo& nodes,
   PLinkInfo& links,
   PNetInfo& net,
   PTSO& tso, PERD& erd, int tsoLoadFlags,
   const std::string& dvfFileName,std::string species, int *speciesIndex)
{
	if( ERD_isERD(dbFile.c_str()) && ! ERD_open(&erd,dbFile.c_str(),tsoLoadFlags) ) {
		net=erd->network;
		nodes=erd->nodes;
		links=erd->links;
		std::cout << "Current file=" << erd->directory << "/" << erd->baseName << std::endl;
		std::cout << "Storage method=" << ERD_GetCompressionDesc(erd) << std::endl;
		if(species.length()==0) {
			if(erd->network->qualCode == 4) { // our internal qaulcode for MSX
				std::cerr << "** No species specified and the ERD file was genereated using EPANET-MSX.  Using the first stored species: " << erd->network->species[0]->id << std::endl;
			}
			*speciesIndex=0;
		} else {
			*speciesIndex=ERD_getSpeciesIndex(erd,species.c_str());
			if(*speciesIndex == -1) {
				std::cerr << "Species: " << species << " not found or not saved. Species:" << std::endl;
				for(int i=0;i<erd->network->numSpecies;i++) {
					PSpeciesData sd = erd->network->species[i];
					std::cerr << "  " << sd->id << (sd->index==-1?" (not stored)": " (stored)") << std::endl;
				}
				EXCEPTION_MNGR(runtime_error, "Invalid species specified.");
			}
		}
	} else if(TSO_isTSO(dbFile.c_str())) {
	   std::cout << std::endl << "Loading TSO File..." << std::endl;
	   tso = TSO_Open((char*)dbFile.c_str(), (char*)0);

	   TSO_ReadPrologue(tso, &net, &nodes, &links,tsoLoadFlags);
	   std::cout << "Current file=" << tso->curFile << std::endl;
	   std::cout << "Storage method=" << tso->storageMethod << std::endl;
	   std::cout << "File version=" << tso->fileVersion << std::endl;
	   if(species.length()!=0) {
		   std::cout << "  Species specified, but ignored for TSO file." << std::endl;
	   }
	   *speciesIndex=0; // always 0 for TSO
	} else {
           std::cerr << "ERROR: file '" << dbFile << "' does not appear to be a TSO/ERD file." << std::endl;
           return 1;
        }
   numNodes = net->numNodes;


   std::cout << std::endl;
   std::cout << "Number of nodes=" << net->numNodes << std::endl;
   std::cout << "Number of links=" << net->numLinks << std::endl;
   std::cout << "Number of tanks=" << net->numTanks << std::endl;
   std::cout << "Number of junctions=" << net->numJunctions << std::endl;
   std::cout << "Number of steps=" << net->numSteps << std::endl;
   std::cout << "Number of species=" << net->numSpecies << std::endl;
   for (int i = 0; i < net->numSpecies; i++)
   {
      PSpeciesData sd = net->species[i];
	  std::cout << "  Species=" << sd->id << (sd->index==-1?" (not stored)": " (stored)") << std::endl;
   }
   std::cout << "Step size (hours)=" << net->stepSize << std::endl; // units appear to be in hours!!!
   std::cout << "Floatmax=" << net->fltMax << std::endl;
   std::cout << std::endl;

#ifdef OUTPUT_TSO_DETAIL

   std::cout << std::endl;
   std::cout << "Node hydraulic information" << std::endl << std::endl;
   for (int i = 0; i < net->nnodes; i++)
   {
      std::cout << "Node (index=" << i + 1 << ",ID=" << nodes[i].id << ") ";
      std::cout << "Demands= ";
      for (int j = 0; j < net->nsteps; j++)
      {
         std::cout << nodes[i].q[j] << " ";
      }
      std::cout << std::endl;
   }
   std::cout << std::endl;

   std::cout << std::endl;
   std::cout << "Link hydraulic information" << std::endl << std::endl;
   for (int i = 0; i < net->nlinks; i++)
   {
      std::cout << "Link (Index=" << links[i].from << ",ID=" << nodes[(links[i].from) - 1].id << " => Index=" << links[i].to << ",ID=" << nodes[(links[i].to) - 1].id << ")" << std::endl;
      std::cout << "Velocities= ";
      for (int j = 0; j < net->nsteps; j++)
      {
         std::cout << links[i].velocity[j] << " ";
      }
      std::cout << std::endl;
      std::cout << "Flows= ";
      for (int j = 0; j < net->nsteps; j++)
      {
         std::cout << links[i].flow[j] << " ";
      }
      std::cout << std::endl;
   }
   std::cout << std::endl;

#endif
   return 0;
}


int main(int argc, char** argv)
{
   try
   {
      PMem assessMem;
      PSourceData source;

      std::string outputFilePrefix;
      std::vector<std::string> erdFiles;
      int detectionDelay = 0;
      int detectionConfidence = 1;
      std::list<double> minQuality;
      bool mcSelected = false;
      bool dmcSelected = false;
      bool vcSelected = false;
      bool dvcSelected = false;
      bool nfdSelected = false;
      bool tdSelected = false;
      bool dtdSelected = false;
      bool ecSelected = false;
      bool tecSelected = false;
      bool decSelected = false;
      bool pkSelected = false;
      bool dpkSelected = false;
      bool pdSelected = false;
      bool dpdSelected = false;
      bool peSelected = false;
      bool dpeSelected = false;
      std::string species;
	  int speciesIndex;
      std::vector<std::string> taiInputFilename;
      std::string sensorInputFilename;
      std::string nodeMapFileName;
      std::string dvfFileName;
      std::list<int> sensorLocations;
      std::ofstream scenarioMapFile;
      std::vector<std::string> nodeIndexToIDMap;
      std::map<std::string, int> nodeIDToIndexMap; // maps node textual IDs to 0-based TEVA node indicies
      std::vector<ObjectiveBase*> theObjectives;

      process_arguments(argc, argv,
                        outputFilePrefix, erdFiles,
                        detectionDelay, detectionConfidence,
                        minQuality, mcSelected, dmcSelected, vcSelected, dvcSelected,
                        nfdSelected, tdSelected, dtdSelected, ecSelected, tecSelected, decSelected,
                        pkSelected, dpkSelected,
                        pdSelected, dpdSelected,
                        peSelected, dpeSelected,
						species,
                        taiInputFilename,
                        sensorInputFilename, sensorLocations, dvfFileName);


      std::cout << "main: after process_arguments: " << std::endl;

      //
      // Iterate through all TSO files
      //
      bool first_time = true;
      std::vector<std::string>::iterator curr_db = erdFiles.begin();
      std::vector<std::string>::iterator  end_db = erdFiles.end();
      std::vector<std::string>::iterator curr_tai = taiInputFilename.begin();
      std::vector<std::string>::iterator end_tai =  taiInputFilename.end();
      if ((pkSelected || peSelected || pdSelected) &&
		  (taiInputFilename.size() != erdFiles.size()))
      {
         EXCEPTION_MNGR(runtime_error, "the number of .tai files must match "
                        "the number of .tso files");
      }


      int scenarioIndex = 0;
      int curr_db_index = 0; // which .tso or .erd file are we processing?
      std::list<double>::iterator minQuality_curr = minQuality.begin();
      std::list<double>::iterator minQuality_end = minQuality.end();
      while (curr_db != end_db)
      {
         //
         // Read in the current TSO file
         //
         int numNodes;

         PTSO tso=NULL;
		 PERD erd=NULL;
		 PNodeInfo nodes;
         PLinkInfo links;
         PNetInfo net;
         int dbLoadFlags=READ_QUALITY | READ_DEMANDPROFILES;
         if( ecSelected || decSelected || tecSelected) {
           dbLoadFlags|=READ_LINKFLOW|READ_LINKVEL;
         }
         if( vcSelected || dvcSelected | mcSelected | dmcSelected) {
           dbLoadFlags|=READ_DEMANDS;
         }
         if(peSelected || pkSelected || pdSelected || dpeSelected || dpdSelected || dpkSelected) {
             // to be on the safe side, also load demands from the TSO file if any HIA metric is selected.
        	 // it is needed for the demand-based ingestion model in HIA and we don't know yet if it is
        	 // specified in any of the tai files
        	 dbLoadFlags |= READ_DEMANDS;
         }

         if ( readDBInfo(outputFilePrefix, *curr_db, numNodes, nodes, links, 
                         net, tso, erd, dbLoadFlags, dvfFileName, 
                         species, &speciesIndex) )
            return 1;

         modifyTsoInfo(numNodes, nodes, links, net, dvfFileName);

         double minQual = 0.0;
         if (minQuality.size()) minQual = *minQuality_curr;
         std::string taiFilename;
         if (pkSelected || peSelected || pdSelected || dpkSelected || dpeSelected || dpdSelected)
         {
            taiFilename = *curr_tai++;
         }
         //
         // If this is the first time through the loop, then initialize some data.
         //
         if (first_time)
         {
            std::string scenarioMapFileName = outputFilePrefix + "." + SCENARIO_MAP_FILE_SUFFIX;
            scenarioMapFile.open(scenarioMapFileName.c_str());
            //
            // NOTE: internal to tso2Impact.cc, the node indicies are 0-based, in
            //       correspondence with the TEVA node/link arrays. More annoyingly,
            //       the indicies maintained in the TEVA data structures are 1-based,
            //       requiring corrections when accessing the data structures.
            //       ALL external writes perform a 1-based translation.
            //
            // a simple file to assist in translation between (1-based)
            // auxilliary node indicies and the real node names.
            //
            nodeIndexToIDMap.resize(numNodes);
            nodeMapFileName = outputFilePrefix + "." + NODE_MAP_FILE_SUFFIX;
            std::ofstream nodeMapFile(nodeMapFileName.c_str());
            for (int i = 0; i < net->numNodes; i++)
            {
               nodeMapFile << i + 1 << " " << nodes[i].id << std::endl;
               nodeIndexToIDMap[i] = nodes[i].id;
               nodeIDToIndexMap[nodes[i].id] = i;
            }

            nodeMapFile.close();
            //
            // Setup the list of objectives
            //
            SetupObjectives(
               nodes,
               links,
               net,
               tso,
               erd,
               outputFilePrefix,
               detectionDelay,
               mcSelected,
               dmcSelected,
               vcSelected,
               dvcSelected,
               nfdSelected,
               tdSelected,
               dtdSelected,
               ecSelected,
	       tecSelected,
               decSelected,
	       dvfFileName,
               pkSelected,
               dpkSelected,
               pdSelected,
               dpdSelected,
               peSelected,
               dpeSelected,
               taiFilename,
               assessMem,
               sensorLocations,
               theObjectives,
               nodeIDToIndexMap);

			// temporary fix
			if (pkSelected || peSelected || pdSelected || dpkSelected || dpeSelected || dpdSelected)
	            assessMem->dr->speciesIndex=speciesIndex;
            first_time = false;
		 }
         else if (pkSelected || peSelected || pdSelected || dpkSelected || dpeSelected || dpdSelected)
         {
            updateTheObjectives(nodes, links, net, tso, erd,
                                pkSelected || pdSelected || peSelected || dpkSelected || dpdSelected || dpeSelected,
                                taiFilename, assessMem, theObjectives);
         }
		 if (pkSelected || peSelected || pdSelected || dpkSelected || dpeSelected || dpdSelected) {
			 if(speciesIndex != assessMem->dr->speciesIndex) {
				 std::cerr << "*** The species specified in the TAI file (" << assessMem->dr->speciesName << 
					 ") is different than the species specified on the command-line (" << species << ")." << std::endl;
				exit(1);
			}
		 }
         //////////////////////////////////////////////////////////////
         int numScenarios = 0;
         clock_t stop, start = clock();
         int num=get_count(erd,tso);
         for(int s=0;s<num;s++) {
            if(loadSimulationResults(s,erd,tso,net,nodes,&source))
            {
               scenarioIndex++;
               numScenarios++;

               std::cout << "Processing simulation results for scenario=" << numScenarios << std::endl;

#ifdef OUTPUT_TSO_DETAIL

            std::cout << "Number of sources=" << net->nsources << std::endl;
            std::cout << "Number of species=" << net->nspecies << std::endl;
            std::cout << std::endl;

            for (int i = 0; i < net->nsources; i++)
            {
               // recall this index is 1-based
               std::cout << "Source node index=" << sources[i].SourceNodeIndex << ", ID=" << sources[i].SourceNodeID << std::endl;
               std::cout << "Source node type=" << sources[i].SourceType << std::endl;
               std::cout << "Source node start=" << sources[i].SourceStart << std::endl;
               std::cout << "Source node stop=" << sources[i].SourceStop << std::endl;
               std::cout << "Source node strength=" << sources[i].SourceStrength << std::endl;
               std::cout << std::endl;
            }

            for (int i = 0; i < net->nnodes; i++)
            {
               std::cout << "Node index=" << i + 1 << ", ID=" << nodes[i].id << " qualities=";
               for (int j = 0; j < net->nsteps; j++)
               {
                  std::cout << nodes[i].c[0][j] << " ";
               }
               std::cout << std::endl;
            }

#endif
               // if a HIA objective is chosen, compute the necessary
               // HIA data.  Do it here rather than replicating the now more complex
               // logic introduced by the addition of the other ingestion methods
               if (peSelected || pkSelected || pdSelected || dpeSelected || dpdSelected || dpkSelected)
               {
                  HIST hist;
                  int nsteps = assessMem->net->info->numSteps;
                  int nnodes = assessMem->net->info->numNodes;
                  resetXA(assessMem->xa, assessMem->dr->maxsteps);
                  resetDoseOverThreshold(assessMem);
                  resetDoseBinData(assessMem);
                  /* Time-dependent cumulative expected dose and disease response */
                  IngestionDataPtr ing = assessMem->ingestionData;
                  for (int i = 0; i < nnodes; i++)
                  {
                     int pop = assessMem->node[i].pop;
                     if (assessMem->node[i].info->nz[0] && pop > 0.0f)
                     {
						 Dose(ing->ingestionData[i], ing, &assessMem->node[i], assessMem->net, nsteps, assessMem->u, assessMem->dot, i, assessMem->doseBins,assessMem->dr->speciesIndex, NULL);
                        Response(ing->ingestionData[i], ing, assessMem->dr, &assessMem->node[i], assessMem->u);
                        if (pkSelected || dpkSelected)
                        {
                           DiseaseRM(assessMem->net, assessMem->dr, assessMem->u);
                        }
                     }
                     Accumulate(i, assessMem->node[i].info->nz[0], assessMem->net, assessMem->dr, assessMem->node[i].pop, assessMem->u, assessMem->xa, assessMem->ta, &hist);
                  }
               }
               processScenario(theObjectives, scenarioIndex,
                               detectionDelay, detectionConfidence, minQual,
							   net, nodes, links, source, NULL, speciesIndex);
               for (int i = 0; i < source->nsources; i++)
                  scenarioMapFile << source->source[i].sourceNodeIndex << " " <<
                  source->source[i].sourceNodeID << " " <<
                  source->source[i].sourceType << " " <<
                  source->source[i].sourceStart << " " <<
                  source->source[i].sourceStop << " " <<
                  source->source[i].sourceStrength << std::endl;
            }
		 }
         ////////////////////////////////
         // clean up the TSO interface //
         ////////////////////////////////
         if(erd!=NULL) {
            ERD_close(&erd);
         } else {
            TSO_Close(&tso);
            TSO_ReleaseNetworkData(&nodes,&links,net);
            TSO_ReleaseNetworkInfo(&net);
         }
         stop = clock();
         printf("Processed input in %7.3f seconds\n", (float)(stop - start) / (float)CLOCKS_PER_SEC);

         std::cout << std::endl;
         std::cout << "Number of scenarios=" << numScenarios << std::endl;
         std::cout << std::endl;

         if (minQuality.size()) minQuality_curr++;
         curr_db++;
         curr_db_index++;
      }

      std::cout << std::endl;
      std::cout << "Total number of scenarios=" << scenarioIndex << std::endl;
      std::cout << std::endl;

      ///////////////////////////
      // dump final stats/info //
      ///////////////////////////

      std::cout << "Node map file written to file=" << nodeMapFileName << std::endl;
      std::cout << std::endl;


      for (size_t i = 0; i < theObjectives.size(); i++)
      {
         std::cout << "Impact outputs written to file=" << outputFilePrefix << "_" << theObjectives[i]->impactFilenameSuffix() << std::endl;
      }



      ////////////////////////////////////////
      // perform cleanup for each objective //
      ////////////////////////////////////////

      finalizeTheObjectives(theObjectives);


      if (peSelected == true || pkSelected == true || pdSelected == true || dpeSelected == true || dpkSelected == true || dpdSelected == true)
      {
         //FreeAssessMemory(assessMem);
      }
   }

   STD_CATCH(;)

   return 0;
}
