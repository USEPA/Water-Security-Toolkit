#include <booster/BoosterDataWriterAMPL.hpp>
#include <booster/BoosterNetworkSimulator.hpp>
#include <booster/BoosterQualityOptions.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/MerlionDefines.hpp>
#include <merlion/SparseMatrix.hpp>

#include <merlionUtils/TaskTimer.hpp>

#include <set>
#include <exception>
#include <algorithm>
#include <fstream>

using namespace merlionUtils;

int main(int argc, char** argv)
{  
   // Handle the optional inputs to this executable
   BoosterQualityOptions options;
   options.ParseInputs(argc, argv);
   
   // Helpful interface for setting up the booster station simulations
   BoosterNetworkSimulator net(options.disable_warnings);

   if (options.logging) {
      net.StartLogging(options.output_prefix+"boosterquality.log");  
      options.PrintSummary(net.Log());
   }

   std::cout << "\n@@@ PARSING INPUT FILES @@@" << std::endl;
   // Read Chlorine network info
   if ((options.inp_filename != "") && (options.wqm_filename != "")) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Both a wqm file and an inp file cannot be specified for the boosters." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   if (options.inp_filename != "") {
      // Read an epanet input file
      merlionUtils::TaskTimer Run_Hydraulics;
      net.ReadINPFile(options.inp_filename,
                      options.sim_duration_min,
                      options.qual_step_min,
                      options.epanet_output_filename,
                      options.merlion_save_filename,
                      -1,
                      -1,
                      options.ignore_merlion_warnings,
                      options.decay_k);
      Run_Hydraulics.StopAndPrint(std::cout, "\n\t- Built booster Merlion Water Quality Model ");
   }
   else if (options.wqm_filename != "") {
      if (options.decay_k != -1.0f) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A booster decay coefficient cannot be specified when using the --booster-wqm option." << std::endl;
         std::cerr << std::endl;
         return 1;
      }
      merlionUtils::TaskTimer Read_WQM;
      // read a merlion water quality model file
      net.ReadWQMFile(options.wqm_filename);
      Read_WQM.StopAndPrint(std::cout, "\n\t- Read booster Merlion Water Quality Model ");
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing network input file for the boosters." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   /////////////////


   // Read in the booster station specs
   if (options.booster_filename != "") {
      merlionUtils::TaskTimer Read_BoosterFile;
      // read in the file defining he booster station parameters
      // this function also defines the booster candidates
      // depending on what is in the booster .spec file. Default is all
      // NZD junctions
      net.ReadBoosterFile(options.booster_filename);
      Read_BoosterFile.StopAndPrint(std::cout, "\n\t- Read water booster spec file ");
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing booster station specs file." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   
   // We instantiate the data writer with pointers to the following
   // objects so function calls can be less cluttered with arguments
   // The data writer will be used to write the resulting data files
   // for the optimization problem
   BoosterDataWriter* writer(NULL);
   writer = new BoosterDataWriterAMPL(&net, &net, &net, &options);

   writer->WriteWQHeadersFile(0, net.Model().n_steps-1);
   writer->WriteWQNodeTypesFile();
   writer->WriteWQModelFile(0, net.Model().n_steps-1);
   writer->WriteBoosterCandidatesFile(0, net.Model().n_steps-1);
   writer->WriteBoosterInjectionsFile(0, net.Model().n_steps-1);

   writer->clear();
   delete writer;
   writer = NULL;
   
   net.StopLogging();

   // Free all other memory being used
   net.StopLogging();
   net.clear();
   
   return 0;
}
