#include <iostream>
#include <stdlib.h>
#include <fstream>

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif
#include "version.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef TEVA_SPOT_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <utilib/exception_mngr.h>
#include <utilib/stl_auxiliary.h>
#include <utilib/OptionParser.h>
#include "sp/impacts.h"

#if defined(WIN32) && !defined(S_ISDIR)
#define __S_ISTYPE(mode, mask)   (((mode) & S_IFMT) == (mask))
#define S_ISDIR(mode)    __S_ISTYPE((mode), S_IFDIR)
#endif


inline utilib::UnPackBuffer& operator>> (utilib::UnPackBuffer& buff, Mem* data)        {return buff;}

inline utilib::PackBuffer& operator<< (utilib::PackBuffer& buff, Mem* data)
{return buff;}

inline istream& operator>> (istream& iss, Mem* data)        {return iss;}

inline ostream& operator<< (ostream& os, Mem* data)
{return os;}

#define TIMED_FILE_FORMAT 1


/// Argument processing for the standalone version
void process_arguments(int argc, char** argv,
                       std::string&outputFilePrefix,
                       std::string& erdFile,
                       double& minQuality,
                       bool& no_print_Nodemap,
		       int& start_time,
		       int& signals_report_time);

int readDBInfo(
   const std::string& dbFile,
   int& numNodes,
   PNodeInfo& nodes,
   PLinkInfo& links,
   PNetInfo& net,
   PERD& erd, 
   int tsoLoadFlags,
   std::string species, int *speciesIndex);

void writeSignalsFile(int ScenarioIndex,
		      double minimumQuality,
		      PNetInfo net, PNodeInfo nodes, PLinkInfo links,
		      PSourceData source,int speciesIndex,
		      std::ostream& outFile,
		      std::vector<int>& recorded_times,
		      int erd_report_time);

int main(int argc, char** argv)
{
    const std::string SUFFIX_INDEX_FILE = "idx";
    const std::string SUFFIX_SIGNALS_FILE = "signals";
    const std::string SUFFIX_INFO_FILE = "infonet";
	try
	{
		PMem assessMem;
		PSourceData source;
		std::string outputFilePrefix;
		std::string erdFile;
		std::string nodeMapFileName;
		std::ofstream scenarioMapFile;
        std::string species;
        int speciesIndex;
        double minQuality;
        bool write_index_file=false;
        bool no_print_Nodemap=false;
	int start_time = 0;
	int signals_report_time = -1;

	std::vector<std::string> nodeIndexToIDMap;
	std::map<std::string, int> nodeIDToIndexMap; // maps node textual IDs to 0-based TEVA node indicies

	process_arguments(argc, argv,
			  outputFilePrefix,
			  erdFile,
			  minQuality,
			  no_print_Nodemap,
			  start_time,
			  signals_report_time);

        int numNodes;
        PERD erd=NULL;
        PTSO tso=NULL;
        PNodeInfo nodes;
        PLinkInfo links;
        PNetInfo net;
        int dbLoadFlags = READ_QUALITY | READ_DEMANDPROFILES;

        std::string signalsFileName = outputFilePrefix + "." + SUFFIX_SIGNALS_FILE;
        std::ofstream outFileSignals(signalsFileName.c_str());

        std::string idxFileName = outputFilePrefix + "." + SUFFIX_INDEX_FILE;
        std::ofstream outFileIdx(idxFileName.c_str());    

        if (readDBInfo(erdFile, numNodes, nodes, links, net, erd, dbLoadFlags, species, &speciesIndex) )
        {
            return 1;
        }

	// check the timing parameters
	int time_step_erd((net->stepSize)*60);
	if(signals_report_time>0)
	{
	  if(signals_report_time%time_step_erd!=0)
	  {
	    std::cerr << "The signals report time must be multiple of the ERD report time\n";
	    std::cerr << "Signals report " << signals_report_time << " ERD report " << time_step_erd << "n";
	    return 1;
	  }
	}
	else
	{
	  signals_report_time = time_step_erd;
	}

	//Adjust start time 
	start_time -= start_time%time_step_erd;
	
	//build list of times to record
	std::vector<int> recorded_times;
	for (int i = 0; i < net->numSteps; ++i)
	{
	  if(i*time_step_erd>=start_time)
	  {
	    if((i*time_step_erd)%signals_report_time==0)
	    {
	      recorded_times.push_back(i);
	    }
	  }
	}

        if (!no_print_Nodemap)
        {

            nodeIndexToIDMap.resize(numNodes);
            nodeMapFileName = outputFilePrefix + "." + SUFFIX_INFO_FILE;
            std::ofstream nodeMapFile(nodeMapFileName.c_str());

            double report_step((int)((net->stepSize)*60));
            int num_steps = net->numSteps;
            //nodeMapFile << num_steps << " " << report_step << "\n";
	    nodeMapFile << recorded_times.size() << " " << start_time << " " << static_cast<double>(signals_report_time) << "\n";
            for (int i = 0; i < net->numNodes; i++)
            {
               nodeMapFile << i + 1 << " " << nodes[i].id << std::endl;
               nodeIndexToIDMap[i] = nodes[i].id;
               nodeIDToIndexMap[nodes[i].id] = i;
            }
            nodeMapFile.close();
        }

        //mark index file with -1 if its greateer than 2G
        int mark_file = 0;
        std::string SourceTypeWords[4] = {"CONC", "MASS", "SETPOINT", "FLOWPACED"};
        if(erd!=NULL)
        {
            int n_contaminations = get_count(erd,tso);
            std::cout << "Contamination scenarios = " <<n_contaminations<<"\n"; 
            outFileIdx << mark_file << "\n";
            for(int s =0;s<n_contaminations;++s)
            {
                int offset = outFileSignals.tellp();
                //std::cout << offset << "\n";
                if(offset<0)
                {
                    mark_file=1;
                }
                if(loadSimulationResults(s,erd,tso,net,nodes,&source))
                {
                    int nsources = source->nsources;
                    PSource psrc = source->source;

                    outFileIdx << s << " " << offset << " " << nsources
                    << " " << psrc->sourceNodeIndex << " ";
                    if(psrc->sourceType < 4 && psrc->sourceType >= 0)
                    {
                        outFileIdx << SourceTypeWords[psrc->sourceType] << " " << psrc->sourceStrength << " ";
                    }
                    else
                    {
                        std::cout << "TODO: WARNING source type not found. MASS wrote it instead\n";
                        outFileIdx << "MASS" << " " << psrc->sourceStrength << " ";
                    }
                    outFileIdx << psrc->sourceStart << " " << psrc->sourceStop << "\n";
                    writeSignalsFile(s,
				     minQuality,
				     net,  
				     nodes,  
				     links, 
				     source,
				     speciesIndex,
				     outFileSignals,
				     recorded_times,
				     time_step_erd);
                }
                else
                {
                    std::cout << "ERROR scenario TODO\n";
                    return 1;
                }
            }
            ERD_close(&erd);
        }


    outFileIdx.seekp(0);
    outFileIdx << mark_file <<"\n" << 0 << " ";

    outFileIdx.close();
    outFileSignals.close();   
    }
	STD_CATCH(;)
    std::cout <<"DONE\n";
	return 0;
}


/***************************************** Methods ******************************************/

void writeSignalsFile(int ScenarioIndex,
                     double minimumQuality,
                     PNetInfo net, PNodeInfo nodes, PLinkInfo links,
                     PSourceData source,int speciesIndex,
		     std::ostream& outFile,
		     std::vector<int>& recorded_times,
		     int erd_report_time)
{
    double reportStep((int)((net->stepSize)*60)); // units are in minutes
    float**c = net->qualResults->nodeC[speciesIndex];
    outFile << ScenarioIndex << " ";
#if(!TIMED_FILE_FORMAT)
    for (int j = 0; j < net->numNodes; j++)
    {   
         //for (int i = 0; i < net->numSteps; i++)
         for(std::vector<int>::iterator it = recorded_times.begin() ; it != recorded_times.end(); ++it)
         {
	    int i = *it; 
            double Quality(c[j][i]);
            if (Quality > minimumQuality)
            {
	      outFile << j+1 << " " << static_cast<double>(erd_report_time)*i << " "; 
               //outFile << j+1 << " " << i*reportStep << " "; 
            }
         }
     }
    outFile << "\n\n";
#else
    // This is what it is used in signals pywst
    outFile << "\n";
    // for (int i = 0; i < net->numSteps; i++)
    for(std::vector<int>::iterator it = recorded_times.begin() ; it != recorded_times.end(); ++it)
    {   
        int i = *it;
	outFile << static_cast<double>(erd_report_time)*i << " ";
	
        for (int j = 0; j < net->numNodes; j++)
         {
            double Quality(c[j][i]);
            if (Quality > minimumQuality)
            {
               outFile << j+1 << " "; 
            }
         }
         outFile << "\n";
     }
    outFile << "\n\n";
#endif
}


int readDBInfo(
   const std::string& dbFile,
   int& numNodes,
   PNodeInfo& nodes,
   PLinkInfo& links,
   PNetInfo& net,
   PERD& erd, 
   int tsoLoadFlags,
   std::string species, int *speciesIndex)
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
    }  
  else 
  {
    std::cerr << "ERROR: file '" << dbFile << "' does not appear to be a ERD file." << std::endl;
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

   return 0;
}

void process_arguments(int argc, char** argv,
                       std::string&outputFilePrefix,
                       std::string& erdFile,
                       double& minQuality,
                       bool& no_print_Nodemap,
		       int& start_time,
		       int& signals_report_time)
{
   utilib::OptionParser options;

   options.add_usage("erd2Signals [options] <output-prefix> <threshold> <erd-directory-or-files>");
   options.description = "An application that reads an ERD file and creates a data file with locations that give a positive signal at each scenario.";

   options.add("no-nodemap-file", no_print_Nodemap, "If this option is specified, no file with node map information will be printed");

   options.add("startTime", start_time, "This option indicates the number of minutes at which the executable start recording contamination data.  As the start time increases, the size of the signals files decreases. Unit: minutes.");

   options.add("reportTime", signals_report_time, "This option indicates the elapsed time between two recordigns of tamination data.  As the reprt time increases, the size of the signals files decreases. Unit: minutes.");

   options.add_argument("output-prefix", "The prefix used for all files generated by erd2Signals.");

   options.add_argument("threshold", "Threshold to determine positive signals.  The units of this threhold depend on the units of the contaminant simulated ERD.");

   options.add_argument("erd-file", "This argument indicates an ERD file.");

   options.epilog = "TODO: Epilog";

   std::string version = create_version("erd2Signals", __DATE__, __TIME__);
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
   if (args.size() < 3)
   {
      options.write(std::cerr);
      exit(1);
   }

   utilib::OptionParser::args_t::iterator curr = args.begin();
   utilib::OptionParser::args_t::iterator end  = args.end();
   curr++;
   outputFilePrefix = *curr;
   curr++;

   //set threshold
   minQuality = atof(curr->c_str());
   curr++;

   // set ERD
   erdFile = *curr;
   curr++;
   
}
