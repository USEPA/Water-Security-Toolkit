#include <measure_gen/MeasureDataWriter.hpp>
#include <measure_gen/MeasureSimOptions.hpp>
#include <measure_gen/MeasureNetworkSimulator.hpp>

#include <merlionUtils/TaskTimer.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/MerlionDefines.hpp>
#include <merlion/SparseMatrix.hpp>
#include <time.h>


void run_simulations(MeasureGenNetworkSimulator& net, const MeasureGenOptions& options, MeasureGenDataWriter& writer);

int main(int argc, char** argv)
{  

   // Handle the optional inputs to this executable
   MeasureGenOptions options; // This should probably be replaced with the options parser in utilib
   options.parse_inputs(argc, argv);
   
   // Helpful interface for setting up the measure station simulations 
   //Net with noise
   MeasureGenNetworkSimulator net(options.disable_warnings);

   if (options.logging) {
      net.StartLogging(options.output_prefix + "measuregen.log");
      options.print_summary(net.Log());
   }

   std::cout << "\n@@@ PARSING INPUT FILES @@@" << std::endl;
   /*******************net with noise****************************/
   if ((options.inp_filename != "") && (options.wqm_filename != "")) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Both a wqm file and an inp file cannot be specified." << std::endl;
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
                      options.sigma,
                      options.seed,
                      options.ignore_merlion_warnings,
                      options.decay_k);
      Run_Hydraulics.StopAndPrint(std::cout, "\n\t- Ran hydraulic model with Epanet ");     
   }
   else if (options.wqm_filename != "") {
      if (options.decay_k != -1.0f) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A decay coefficient cannot be specified when using the --wqm option." << std::endl;
         std::cerr << std::endl;
         return 1;
      }
      if (options.sigma != -1.0f) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A demand noise scale cannot be specified when using the --wqm option." << std::endl;
         std::cerr << std::endl;
         return 1;
      }
      if (options.seed != -1.0f) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A demand noise seed cannot be specified when using the --wqm option." << std::endl;
         std::cerr << std::endl;
         return 1;
      }
      merlionUtils::TaskTimer Read_WQM;
      // read a merlion water quality model file
      net.ReadWQMFile(options.wqm_filename);
      Read_WQM.StopAndPrint(std::cout, "\n\t- Read water quality data ");
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing network input file." << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   if (options.sensor_filename != "") {
      merlionUtils::TaskTimer Read_Sensors;
      // Read the file defining the set of sensors
      net.ReadSensorFile(options.sensor_filename);
      Read_Sensors.StopAndPrint(std::cout,"\n\t- Read sensor file ");
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing sensor layout file." << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   if (options.tsg_filename != "") {
      merlionUtils::TaskTimer Read_tsg;
      // Read in the injection scenarios define by the *.tsg file
      std::cout << "@ " << options.tsg_filename << std::endl;
      net.ReadTSGFile(options.tsg_filename);
      Read_tsg.StopAndPrint(std::cout, "\n\t- Read scenario data in .tsg file");
   }
   else if (options.tsi_filename != "") {
      merlionUtils::TaskTimer Read_tsi;
      // Read in the injection scenarios define by the *.tsi file
      if (options.isDefault("tsi-species-id")) {
         net.ReadTSIFile(options.tsi_filename);
      }
      else {
         if (options.tsi_species_id < 0) {
            std::cerr << std::endl;
            std::cerr << "ERROR: A TSI species id must be positive" << std::endl;
            std::cerr << std::endl;
            return 1;
         }
         net.ReadTSIFile(options.tsi_filename, false, options.tsi_species_id);
      }
      Read_tsi.StopAndPrint(std::cout, "\n\t- Read scenario data in .tsi file");
   }
   else if (options.scn_filename != "") {
      merlionUtils::TaskTimer Read_scn;
      // Read in the injection scenarios define by the *.scn file
      net.ReadSCNFile(options.scn_filename);
      Read_scn.StopAndPrint(std::cout, "\n\t- Read scenario data in .scn file");
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing scenario file." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   int scen_file_cnt = (options.tsg_filename != "")?(1):(0);
   scen_file_cnt += (options.tsi_filename != "")?(1):(0);
   scen_file_cnt += (options.scn_filename != "")?(1):(0);
   if (scen_file_cnt > 1) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Too many scenario options specified. Choose one: TSG, TSI, SCN" << std::endl;
      std::cerr << std::endl;
      return 1;
   }


   if ((options.print_concentrations)&&((options.FNR>0.0f)||(options.FPR>0.0f))) {
      std::cerr << std::endl;
      std::cerr << "ERROR: FNR and FPR cannot be specified for concentration measurements. Only binary measurements can have a FNR or FPR" << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   // We instantiate the data writer with pointers to the following
   // objects so function calls can be less cluttered with arguments
   // The data writer will be used to write the resulting data files
   // for the optimization problem
   MeasureGenDataWriter writer(&net, &options);
   
   if(options.sigma != -1)
   {
      //Net without noise
      MeasureGenNetworkSimulator net_not_noise;
      // read the inp file 
      net_not_noise.ReadINPFile(options.inp_filename,
         options.sim_duration_min,
         options.qual_step_min,
         options.epanet_output_filename,
         options.merlion_save_filename);
      const merlionUtils::MerlionModelContainer& model_not_noise(net_not_noise.Model());
      writer.printDemands(model_not_noise);
   }

   std::cout << "\n@@@ SIMULATING INJECTIONS @@@" << std::endl;
   

   run_simulations(net, options, writer);

   net.StopLogging();

   // Free all other memory being used
   writer.clear();
   net.clear();

   std::cout << "\nDONE" << std::endl;
   return 0;
}

void run_simulations(MeasureGenNetworkSimulator& net, const MeasureGenOptions& options, MeasureGenDataWriter& writer)
{
   const merlionUtils::MerlionModelContainer& model(net.Model());

   std::cout << "\nNetwork Stats:\n";
   std::cout << "\tNumber of Junctions                  - " << model.junctions.size() << "\n";
   std::cout << "\tNumber of Nonzero Demand Junctions   - " << model.nzd_junctions.size() << "\n";
   std::cout << "\tNumber of Tanks                      - " << model.tanks.size() << "\n";
   std::cout << "\tNumber of Reservoirs                 - " << model.reservoirs.size() << "\n";
   std::cout << "\tWater Quality Timestep (minutes)     - " << model.qual_step_minutes << "\n";
   std::cout << std::endl;

   float *tox_mass_inj_gpmin(new float[model.N]);
   
   BlasInitZero(model.N, tox_mass_inj_gpmin);

   const merlionUtils::InjScenList& Scenarios = net.InjectionScenarios();
   if (!Scenarios.empty()) {
      if (Scenarios.size() > 1) {
         std::cout << std::endl;
         std::cout << "WARNING: measuregen application will only use the first\n" <<
            "         scenario specified in the scenario list. The current scenario\n" <<
            "         list has " << Scenarios.size() << " scenarios." << std::endl;
         std::cout << std::endl;
      }

      merlionUtils::InjScenario& scen = *(Scenarios.front());
      merlionUtils::InjectionList& injlist = scen.Injections();
   
      //std::cout << std::endl;
      //scen.Print(std::cout);
      //std::cout << std::endl;
   
      float mass_injected_g;
      int max_row_index;
      scen.SetArray(model, tox_mass_inj_gpmin, mass_injected_g, max_row_index);
     
      // Solver the linear system
      usolve ( model.N,
         0,
         max_row_index,
         model.G->Values(),
         model.G->iRows(),
         model.G->pCols(),
         tox_mass_inj_gpmin );
   
      // Create a reference variable to make units make more obvious. The linear solver
      // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
      // use this reference when it make sense for units (e.g below the linear solve).
      float*& tox_conc_gpm3 = tox_mass_inj_gpmin;
      //writer.WriteProblem(tox_conc_gpm3);
      if (options.print_concentrations){
        writer.WriteConcentrations(tox_conc_gpm3);
      }
      else {
      writer.WriteBinaryMeasurements(tox_conc_gpm3);
      }
   
            
      // reset the sparse set of locations where the injections modifed the array,
      // these may have been outside the region of interest in the linear solver,
      // in which case, the BlasResetZero code after this loop would not have
      // zero that region of the array
      scen.ClearArray(model,tox_mass_inj_gpmin);
      // not really necessary unless more simulation will be performed
      // reset to zero only the values where chl_conc_gpm3
      // was possibly modified by linear solver, this saves time
      const int num_elements = max_row_index+1;
      BlasResetZero(num_elements, tox_conc_gpm3);
     
      std::cout << "Total Mass Injected (grams): " << mass_injected_g << std::endl;

   }
   else {
      std::cerr << std::endl;
      std::cerr << "inversionsim ERROR: Scenario list is empty" << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   delete [] tox_mass_inj_gpmin;
   tox_mass_inj_gpmin = NULL;
   //rhs.clear();
}

void addErrorToMeasurements(int N, int *tox_binary_status, float FNR, float FPR, float seed){




}
