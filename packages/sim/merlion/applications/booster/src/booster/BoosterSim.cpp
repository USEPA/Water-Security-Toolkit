#include <booster/BoosterDataWriterAMPL.hpp>
#include <booster/BoosterDataWriterPYOMO.hpp>
#include <booster/BoosterDataWriterPYSP.hpp>
#include <booster/BoosterNetworkSimulator.hpp>
#include <booster/BoosterSimOptions.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/MerlionDefines.hpp>
#include <merlion/SparseMatrix.hpp>

#include <merlionUtils/TaskTimer.hpp>

#include <set>
#include <exception>
#include <algorithm>
#include <fstream>

#ifdef BOOSTER_MPI
#include "mpi.h"
#endif

using namespace merlionUtils;

//forward declarations
void PerformBoosterSimulations(const BoosterNetworkSimulator& chl_net,
                               const NetworkSimulator& tox_net,
			       const BoosterSimOptions& options,
			       BoosterDataWriter& writer);
void PerformDosageSimulations(const BoosterNetworkSimulator& chl_net,
                              const NetworkSimulator& tox_net,
                              const BoosterSimOptions& options,
                              BoosterDataWriter& writer);
void PerformBoosterDosageSimulations(const BoosterNetworkSimulator& chl_net,
                                     const NetworkSimulator& tox_net,
                                     const BoosterSimOptions& options,
                                     BoosterDataWriter& writer);
void PostprocessRows(std::map<std::vector<int>, float>& reduced_z_rows, std::multimap<float, std::vector<int> >& postp_reduced_z_rows, int repeat, float tol);
void GreedySearch(std::multimap<float, std::vector<int> >& submodf, std::map<int, double>& greed_sol, double max_value);

int main(int argc, char** argv)
{  
#ifdef BOOSTER_MPI
   MPI::Init(argc, argv);
#endif
   // Handle the optional inputs to this executable
   BoosterSimOptions options;
   options.ParseInputs(argc, argv);
   
   // Helpful interface for setting up the booster station simulations
   BoosterNetworkSimulator chl_net(options.disable_warnings);
   NetworkSimulator tox_net(true);

   if (options.logging) {
      chl_net.StartLogging(options.output_prefix+"boostersim.log");  
      tox_net.StartLogging(options.output_prefix+"boostersim.log");  
      options.PrintSummary(chl_net.Log());
   }

   std::cout << "\n@@@ PARSING INPUT FILES @@@" << std::endl;
   // Read Chlorine network info
   if ((options.chl_inp_filename != "") && (options.chl_wqm_filename != "")) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Both a wqm file and an inp file cannot be specified for the boosters." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   if (options.chl_inp_filename != "") {
      // Read an epanet input file
      merlionUtils::TaskTimer Run_Hydraulics;
      chl_net.ReadINPFile(options.chl_inp_filename,
                          options.sim_duration_min,
                          options.qual_step_min,
                          options.chl_epanet_output_filename,
                          options.chl_merlion_save_filename,
                          -1,
                          -1,
                          options.ignore_merlion_warnings,
                          options.chl_decay_k);
      Run_Hydraulics.StopAndPrint(std::cout, "\n\t- Built booster Merlion Water Quality Model ");
   }
   else if (options.chl_wqm_filename != "") {
      if (options.chl_decay_k != -1.0f) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A booster decay coefficient cannot be specified when using the --booster-wqm option." << std::endl;
         std::cerr << std::endl;
         return 1;
      }
      merlionUtils::TaskTimer Read_WQM;
      // read a merlion water quality model file
      chl_net.ReadWQMFile(options.chl_wqm_filename);
      Read_WQM.StopAndPrint(std::cout, "\n\t- Read booster Merlion Water Quality Model ");
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing network input file for the boosters." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   /////////////////

   // Read Toxin network info
   if ((options.tox_inp_filename != "") && (options.tox_wqm_filename != "")) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Both a wqm file and an inp file cannot be specified for the toxin." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   if (options.tox_inp_filename != "") {
      // Read an epanet input file
      merlionUtils::TaskTimer Run_Hydraulics;
      tox_net.ReadINPFile(options.tox_inp_filename,
                          options.sim_duration_min,
                          options.qual_step_min,
                          options.tox_epanet_output_filename,
                          options.tox_merlion_save_filename,
                          -1,
                          -1,
                          options.ignore_merlion_warnings,
                          options.tox_decay_k);
      Run_Hydraulics.StopAndPrint(std::cout, "\n\t- Built toxin Merlion Water Quality Model ");
   }
   else if (options.tox_wqm_filename != "") {
      if (options.tox_decay_k != -1.0f) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A toxin decay coefficient cannot be specified when using the --tox-wqm option." << std::endl;
         std::cerr << std::endl;
         return 1;
      }
      merlionUtils::TaskTimer Read_WQM;
      // read a merlion water quality model file
      tox_net.ReadWQMFile(options.tox_wqm_filename);
      Read_WQM.StopAndPrint(std::cout, "\n\t- Read toxin Merlion Water Quality Model ");
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing network input file for the toxin." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   ///////////////


   // **A weak way of checking that the models match (except for, of course, their decay coefficients)
   if ((tox_net.Model().N != chl_net.Model().N) ||
       (tox_net.Model().n_nodes != chl_net.Model().n_nodes) ||
       (tox_net.Model().n_links != chl_net.Model().n_links) ||
       (tox_net.Model().n_steps != chl_net.Model().n_steps) ||
       (tox_net.Model().qual_steps_per_hour != chl_net.Model().qual_steps_per_hour)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Toxin and Booster models don't match in network layout or simulation length." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }


   // Read in the booster station specs
   if (options.booster_filename != "") {
      merlionUtils::TaskTimer Read_BoosterFile;
      // read in the file defining he booster station parameters
      // this function also defines the booster candidates
      // depending on what is in the booster .spec file. Default is all
      // NZD junctions
      chl_net.ReadBoosterFile(options.booster_filename);
      Read_BoosterFile.StopAndPrint(std::cout, "\n\t- Read water booster spec file ");
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing booster station specs file." << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   bool detection_times_provided(false);
   // Read in the Scenarios
   if (options.tsg_filename != "") {
      merlionUtils::TaskTimer Read_Scen;
      // Read in the injection scenarios define by the *.tsg file
      tox_net.ReadTSGFile(options.tsg_filename);
      Read_Scen.StopAndPrint(std::cout, "\n\t- Read scenario data in .tsg file");
   }
   else if (options.tsi_filename != "") {
      merlionUtils::TaskTimer Read_tsi;
      // Read in the injection scenarios define by the *.tsi file
      if (options.isDefault("tsi-species-id")) {
         tox_net.ReadTSIFile(options.tsi_filename);
      }
      else {
         if (options.tsi_species_id < 0) {
            std::cerr << std::endl;
            std::cerr << "ERROR: A TSI species id must be positive" << std::endl;
            std::cerr << std::endl;
            return 1;
         }
         tox_net.ReadTSIFile(options.tsi_filename, false, options.tsi_species_id);
      }
      Read_tsi.StopAndPrint(std::cout, "\n\t- Read scenario data in .tsi file");
   }
   else if (options.scn_filename != "") {
      merlionUtils::TaskTimer Read_Scen;
      // Read in the injection scenarios define by the *.scn file
      tox_net.ReadSCNFile(options.scn_filename);
      Read_Scen.StopAndPrint(std::cout, "\n\t- Read scenario data in .scn file");
   }
   else if (options.dscn_filename != "") {
      merlionUtils::TaskTimer Read_Scen;
      // Read in the injection scenarios define by the *.dscn file
      tox_net.ReadSCNFile(options.dscn_filename);
      Read_Scen.StopAndPrint(std::cout, "\n\t- Read scenario data in .dscn file");
      detection_times_provided = true;
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
   scen_file_cnt += (options.dscn_filename != "")?(1):(0);
   if (scen_file_cnt > 1) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Too many scenario options specified. Choose one: TSG, TSI, SCN, DSCN" << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   if (options.scenario_weights_filename != "") {
      merlionUtils::TaskTimer Read_Weights;
      tox_net.ReadScenarioWeightsFile(options.scenario_weights_filename); 
      Read_Weights.StopAndPrint(std::cout, "\n\t- Read Scenario Weights File ");
   }

   // Read in the sensor node list or inverse
   if (options.wqm_inverse_filename != "") {
      merlionUtils::TaskTimer Read_Inverse;
      tox_net.ReadInverse(options.wqm_inverse_filename);
      Read_Inverse.StopAndPrint(std::cout, "\n\t- Read Merlion Inverse File ");
   }
   else if (options.sensor_filename != "") {
      merlionUtils::TaskTimer Read_Sensors;
      // Read the file defining the set of sensors
      tox_net.ReadSensorFile(options.sensor_filename);
      Read_Sensors.StopAndPrint(std::cout,"\n\t- Read sensor file ");
   }
   else if (!detection_times_provided) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing sensor or inverse file which is required when detection times are not provided." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   if ((options.sensor_filename != "") && (options.wqm_inverse_filename != "")) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Both a sensor file and an inverse file cannot be specified." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   
   std::cout << "\n@@@ DETERMINING SCENARIO DETECT TIMES @@@" << std::endl;
   if ((options.wqm_inverse_filename == "") && (!detection_times_provided)) {
      merlionUtils::TaskTimer Build_Inverse;
      
      // Build reduced inverse so sensor node detections can be rapidly
      // determined for each injection
      int min_inj_timestep = tox_net.Model().n_steps;
      for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, tox_net.InjectionScenarios()) {
         int scen_start = (*p_scen)->EarliestInjectionTimestep(tox_net.Model());
         if (scen_start < min_inj_timestep) {
            min_inj_timestep = scen_start;
         }
      }
      
      // Build a list of node/time tuple where rows of the inverse are needed.
      if (tox_net.SensorNodeIDS().empty() && tox_net.GrabSampleIDS().empty()) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A non-empty list of sensor nodes or grab samples is required when "
	           << "using a tsg or scn file." << std::endl;
         std::cerr << std::endl;
         return 1;
      }
      
      tox_net.GenerateReducedInverse(min_inj_timestep,tox_net.Model().n_steps-1,0,1,true);
      Build_Inverse.StopAndPrint(std::cout, "\n\t- Inverse Built ");
   }
   
   merlionUtils::TaskTimer Classify;
   // Apply the sensor layout and generate the new scenario data
   // based on detection times
   tox_net.ClassifyScenarios(detection_times_provided,
                             tox_net.Model().n_steps-1,
                             options.detection_tol_gpm3,
                             options.detection_interval_min);
   chl_net.CopyScenarioList(tox_net);
   chl_net.ClassifyBoosterScenarios(true,
                                    !(options.disable_scenario_aggregation),
                                    options.detection_tol_gpm3,
                                    options.detection_interval_min);
   Classify.StopAndPrint(std::cout, "\n\t- Classified Scenarios ");

   if (options.demand_percapita_m3pmin ||
       options.ingestion_rate_m3pmin) {
      merlionUtils::TaskTimer GeneratePopUsageData;
      tox_net.GeneratePopulationUsageData(options.demand_percapita_m3pmin,
                                          options.ingestion_rate_m3pmin);
      GeneratePopUsageData.StopAndPrint(std::cout, "\n\t- Generated Population Usage Data");
   }

   // Free the memory for the reduced system inverse
   tox_net.ResetInverse();
   // Free the memory for the list of sensors
   tox_net.ResetSensorData();
   
   // Assume the mass consumed for the non-detected scenarios is equal to 
   // the mass injected.
   double nd_total_mass_injected_g(0.0);
   for STL_CONST_ITERATE(InjScenList, pos, stop, chl_net.NonDetectedScenarios()) {
      nd_total_mass_injected_g += (*pos)->MassInjected(chl_net.Model());
      (*pos)->AddImpact((*pos)->MassInjected(chl_net.Model()),"Mass Consumed Grams");
   }
   
   // Booster scenarios will have all toxin injections stop no later than 
   // sim_stop_time which represents the 'no-drink-order'
   double db_total_mass_injected_g(0.0);
   for STL_CONST_ITERATE(BoosterScenList, p_bscen, pbscen_stop, chl_net.BoosterScenarios()) {
      const InjScenList& toxin_scenarios = (*p_bscen)->ToxinScenarios();
      for STL_CONST_ITERATE(InjScenList, p_tox_scen, p_tox_scen_stop, toxin_scenarios) {
         db_total_mass_injected_g += (*p_tox_scen)->MassInjected(chl_net.Model());
      }
   }
   // The set of detected scenarios will have been tagged with the amount
   // injected by their original (non-shortened) injection profiles
   double d_total_mass_injected_g(0.0);
   for STL_CONST_ITERATE(InjScenList, pos, stop, chl_net.DetectedScenarios()) {
      d_total_mass_injected_g += (*pos)->Impact("Original Mass Injected Grams");
   }
   
   // We instantiate the data writer with pointers to the following
   // objects so function calls can be less cluttered with arguments
   // The data writer will be used to write the resulting data files
   // for the optimization problem
   BoosterDataWriter* writer(NULL);
   if (options.output_PYSP) {
      writer = new BoosterDataWriterPYSP(&chl_net, &chl_net, &tox_net, &options);
   }
   else if (options.output_PYOMO) {
      writer = new BoosterDataWriterPYOMO(&chl_net, &chl_net, &tox_net, &options);
   }
   else {
      writer = new BoosterDataWriterAMPL(&chl_net, &chl_net, &tox_net, &options);
   }
   if (options.output_PYSP && options.output_PYOMO) {
      std::cerr << std::endl;
      std::cerr << "ERROR: both PYOMO and PYSP output options can not be selected." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   
   // Write results files which don't require
   // any information from the simulations done below
   std::cout << "\n@@@ WRITING PRE-SIMULATION DATA FILES @@@" << std::endl;
   merlionUtils::TaskTimer WritePreData;

   int latest_timestep = 0;
   int earliest_timestep = chl_net.Model().n_steps-1;
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net.BoosterScenarios()) {
      int first_injection_timestep(chl_net.Model().N-1);
      const InjScenList& toxin_scenarios = (*p_boost)->ToxinScenarios();
      for STL_CONST_ITERATE(InjScenList, p_tox, p_tox_stop, toxin_scenarios) {
         int t = (*p_tox)->EarliestInjectionTimestep(chl_net.Model());
	 if (t < first_injection_timestep) {
	    first_injection_timestep = t;
	 }
      }
      if ((*p_boost)->sim_stop_timestep > latest_timestep) {latest_timestep = (*p_boost)->sim_stop_timestep;}
      if (first_injection_timestep < earliest_timestep) {earliest_timestep = first_injection_timestep;}
   }

   writer->WriteWQHeadersFile(earliest_timestep, latest_timestep);
   writer->WriteWQDemandsFile(earliest_timestep, latest_timestep);
   writer->WriteWQTankVolumesFile(earliest_timestep, latest_timestep);

   writer->WriteBoosterWQModelFile(earliest_timestep, latest_timestep);
   writer->WriteBoosterCandidatesFile(earliest_timestep, latest_timestep);
   writer->WriteBoosterInjectionsFile(earliest_timestep, latest_timestep);

   writer->WriteToxinWQModelFile(earliest_timestep, latest_timestep);
   writer->WriteToxinInjectionsFile();

   writer->WritePopulationDosageFile(earliest_timestep, latest_timestep);

   writer->WriteScenarioSummaryFile();

   WritePreData.StopAndPrint(std::cout, "\n\t- Wrote data files ");
   
   if (!options.detections_only) {
      double booster_seconds = 0.0;

      std::cout << "\n@@@ SIMULATING BOOSTER STATIONS @@@" << std::endl;
      merlionUtils::TaskTimer SimAndSave;
      PerformBoosterSimulations(chl_net, tox_net, options, *writer); 
      SimAndSave.StopAndSave(booster_seconds);
   
      double sims_per_second = ((chl_net.BoosterCandidates().size()+1.0)*chl_net.BoosterScenarios().size())/booster_seconds;
      std::cout << "\n\t- Finished Booster Station Simulations (time in seconds): " << booster_seconds << std::endl;
      std::cout << "\n\t- Approximate Simulations Per Second: " << sims_per_second << std::endl;
   }

   if (options.demand_percapita_m3pmin ||
       options.ingestion_rate_m3pmin) {
      double dosage_seconds = 0.0;

      std::cout << "\n@@@ SIMULATING NODE DOSAGE WITH TOXIN SCENARIOS @@@" << std::endl;
      merlionUtils::TaskTimer SimAndSave;
      if (options.disable_scenario_aggregation) {
         PerformDosageSimulations(chl_net, tox_net, options, *writer);
      }
      else {
         PerformBoosterDosageSimulations(chl_net, tox_net, options, *writer);
      }
      SimAndSave.StopAndSave(dosage_seconds);
   
      double sims_per_second = (chl_net.BoosterScenarios().size())/dosage_seconds;
      std::cout << "\n\t- Finished Booster Station Simulations (time in seconds): " << dosage_seconds << std::endl;
      std::cout << "\n\t- Approximate Simulations Per Second: " << sims_per_second << std::endl;
   }
   
   std::ofstream out;
   out.precision(12);
   out.setf(std::ios::scientific,std::ios::floatfield);
   out.open(std::string(options.output_prefix+"boostersim.yml").c_str(), std::ios_base::out | std::ios_base::trunc);
   out << "detected scenarios: {count: " << chl_net.DetectedScenarios().size() << ", mass injected grams: " << db_total_mass_injected_g << ", orig mass injected grams: " << d_total_mass_injected_g << "}\n";
   out << "discarded scenarios: {count: " << chl_net.DiscardedScenarios().size() << "}\n";
   out << "non-detected scenarios: {count: " << chl_net.NonDetectedScenarios().size() << ", mass injected grams: " << nd_total_mass_injected_g << "}\n";
   out.close();

   writer->clear();
   delete writer;
   writer = NULL;

   chl_net.StopLogging();
   tox_net.StopLogging();
   
   // Free all other memory being used
   chl_net.clear();
   tox_net.clear();


#ifdef BOOSTER_MPI
   MPI::Finalize();
#endif
   return 0;
}



void PerformBoosterSimulations(const BoosterNetworkSimulator& chl_net, const NetworkSimulator& tox_net, const BoosterSimOptions& options, BoosterDataWriter& writer)
{
   const float zero_conc_gpm3 = std::max(options.zero_conc_tol_gpm3,ZERO_CONC_GPM3);

   const BoosterNetworkSimulator& net = chl_net;
   const MerlionModelContainer& model = chl_net.Model();
   const MerlionModelContainer& tox_model = tox_net.Model();

   int num_booster_stations = net.BoosterCandidates().size();
   std::cout << "\nNetwork Stats:\n";
   std::cout << "\tNumber of Booster Station Candidates - " << num_booster_stations << "\n";
   std::cout << "\tNumber of Junctions                  - " << model.junctions.size() << "\n";
   std::cout << "\tNumber of Nonzero Demand Junctions   - " << model.nzd_junctions.size() << "\n";
   std::cout << "\tNumber of Tanks                      - " << model.tanks.size() << "\n";
   std::cout << "\tNumber of Reservoirs                 - " << model.reservoirs.size() << "\n";
   std::cout << "\tWater Quality Timestep (minutes)     - " << model.qual_step_minutes << "\n";
   std::cout << std::endl;

   // A shortcut for specifying all possible 
   // right hand sides in one call of the multiple
   // right hand side triangular solver
   int nrhs = options.max_rhs;
   if (nrhs <= 0) {
      nrhs = num_booster_stations; 
   }

   // Allocate the right hand side and solution vector for toxin injection sims
   float* tox_mass_inj_gpmin = new float[model.N];
   BlasInitZero(model.N, tox_mass_inj_gpmin);

   // Allocate memory for the rhs and solution vector for the booster sims
   // depending on the number right hand sides specified in options
   float* chl_mass_inj_gpmin(NULL);
   if (nrhs > 1) {
      int TMP_RHS = (nrhs<num_booster_stations)?nrhs:num_booster_stations;
      chl_mass_inj_gpmin = new float[model.N * TMP_RHS];
      BlasInitZero(model.N*TMP_RHS, chl_mass_inj_gpmin);
   }
   else {
      chl_mass_inj_gpmin = new float[model.N];
      BlasInitZero(model.N, chl_mass_inj_gpmin);
   }

   std::vector<std::vector<int> > z_rows(model.N);
   std::map<std::vector<int>, float> reduced_z_rows;
   std::set<int> positive_demand_u_ids;
   for (int u_idx = 0; u_idx < model.N; ++u_idx) {
      if (model.demand_ut_m3[u_idx] > 0.0f) {
         positive_demand_u_ids.insert(u_idx);
      }
   }
   std::vector<int> nz_tox_loop;
   nz_tox_loop.reserve(model.N/10);

   // keep track of size of optimization problem
   // defined by the data files being written
   unsigned long long rows_orig(0);	// constraints
   unsigned long long rows_reduced(0);
   unsigned long long nnz_orig(0);     
   unsigned long long nnz_reduced(0);

   std::map<int,float> single_booster_impacts_g;
   float total_pre_detect_mc_g = 0.0;

   // The main solve loop for booster station simulations
   // Outer loop simulates toxin injection scenarios
   // Inner loop simulates booster station injection applied to toxin scenarios
   // Scenario results are written at the end of each iteration of the outer toxin
   // loop. Files are written such that once an iteration end, files can be used in MIP
   // formulations as is. This is to accommodate for long simulations.
#ifdef BOOSTER_MPI
   int mpi_cntr = 0;
#endif
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, net.BoosterScenarios()) {

#ifdef BOOSTER_MPI
       if (((mpi_cntr++)%(MPI::COMM_WORLD.Get_size())) != MPI::COMM_WORLD.Get_rank()) {
	 continue;
       }
#endif

      merlionUtils::TaskTimer ScenarioSim;

      const BoosterScenario& booster_scen = **p_boost;

      // We need to assign the node name before using the 
      // booster injection template (this single Injection is
      // a sparse representation of an identical Injection
      // for every booster station)
      assert(booster_scen.Injections().size() == 1);

      /////////////// Determine a few parameters that will speed up the solves below
      const int chl_start_timestep = booster_scen.Injections().front()->StartTimestep(model);
      const int sim_stop_timestep = booster_scen.sim_stop_timestep;
   
      const int min_row = (options.no_reduce)?(0):(model.N-model.n_nodes*(sim_stop_timestep+1));
   
      float pre_detect_mc_g = 0.0; //mass consumed before detection
      float post_detect_mc_g = 0.0; // mass consumed after detection no matter what
   
      // set the rhs for the toxin injection
      int max_row_tox(0);
      const InjScenList& toxin_scenarios = booster_scen.ToxinScenarios();
      for STL_CONST_ITERATE(InjScenList, p_tox_scen, p_tox_stop, toxin_scenarios) {
         float scaled_mass_injected_g(0.0); //unused
	 int max_row(model.N-1);
         float scale = ((*p_tox_scen)->HasImpact("Normalized Weight"))?((*p_tox_scen)->Impact("Normalized Weight")):(1.0);
         (*p_tox_scen)->SetArrayScaled(scale, model, tox_mass_inj_gpmin, scaled_mass_injected_g, max_row);  
	 if (max_row > max_row_tox) {
	    max_row_tox = max_row;
	 }
      }
   
      if (options.no_reduce) {
         max_row_tox = model.N-1; 
      }
   
      // Solver the linear system
      double tmp_time, backsolve_times(0.0);
      merlionUtils::TaskTimer BackSolve;
      usolve ( tox_model.N,
	       min_row,
	       max_row_tox,
	       tox_model.G->Values(),
	       tox_model.G->iRows(),
	       tox_model.G->pCols(),
	       tox_mass_inj_gpmin );
      if (options.report_scenario_timing) {
         BackSolve.StopAndSave(tmp_time);
         backsolve_times += tmp_time;
      }
   
      // Create a reference variable to make units make more obvious. The linear solver
      // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
      // use this reference when it make sense for units (e.g below the linear solve).
      float*& tox_conc_gpm3 = tox_mass_inj_gpmin; 
   
      // resize the nz_tox_loop vector to zero but keep its memory available
      nz_tox_loop.resize(0);
      // constant values omitted from MIP objective function stored here
      // Also the relative sparse set of indices where the toxin concentration is nonzero
      // is determined and stored in nz_tox_loop. This reduces the search space needed to be
      // analyzed in the booster station loop
      const int u_idx_start = model.N - (sim_stop_timestep+1)*model.n_nodes;
      std::set<int>::const_iterator p_dem_u_idx = positive_demand_u_ids.begin();
      while(*p_dem_u_idx < u_idx_start) {++p_dem_u_idx;}
      std::set<int>::const_iterator p_dem_u_idx_stop = --(positive_demand_u_ids.end());
      while(*p_dem_u_idx_stop > max_row_tox) {--p_dem_u_idx_stop;}
      ++p_dem_u_idx_stop;
      for (; p_dem_u_idx != p_dem_u_idx_stop; ++p_dem_u_idx) {
         double tmp_conc_gpm3 = tox_conc_gpm3[*p_dem_u_idx];
         if (tmp_conc_gpm3 > zero_conc_gpm3){
            int u_idx = *p_dem_u_idx;
            int time = (model.n_steps-1) - (u_idx/model.n_nodes);
            if (time < chl_start_timestep) {
               pre_detect_mc_g += tmp_conc_gpm3*model.demand_ut_m3[u_idx];
            }
            else {
               nz_tox_loop.push_back(u_idx);
            }
         }
      }

      total_pre_detect_mc_g += pre_detect_mc_g;
   
      if (nrhs > 1) {
         // Code optimized for solving booster station sims using set of matrix rhs
         // FASTED METHOD WHERE MEMORY IS LEVERAGED WITH SPEED
         //////////////////////////////////////////////
         if (nrhs > num_booster_stations) {
            nrhs = num_booster_stations;
         }
         const int total_blocks = (num_booster_stations/nrhs) + (((num_booster_stations%nrhs)>0)?1:0);
	 const int odd_count = num_booster_stations%nrhs;
	 int current_block(0);

	 std::vector<int>::const_iterator p_bid = chl_net.BoosterCandidates().begin();
	 std::vector<int>::const_iterator p_bid_stop = chl_net.BoosterCandidates().end();
         std::vector<int>::const_iterator p_bid_block_start;
	 Injection booster_injection = *(booster_scen.Injections().front());
	 
	 std::vector<int> block_booster_ids;
         while (current_block < total_blocks) {
	    block_booster_ids.resize(nrhs);
	    p_bid_block_start = p_bid;
	    // set the rhs matrix
	    int max_row_chl = 0;
	    for (int r = 0; (r < nrhs) && (p_bid != p_bid_stop); ++r, ++p_bid) {
	       float mass_injected_g(0.0);
	       int max_row_index(model.N-1);
	       booster_injection.NodeName() = model.NodeName(*p_bid);
	       booster_injection.SetMultiArray(model,nrhs,r,chl_mass_inj_gpmin,mass_injected_g,max_row_index);
	       block_booster_ids[r] = *p_bid;
	       if (max_row_index > max_row_chl) {
		  max_row_chl = max_row_index;
	       }
            }
            
            // do the backsolve
            merlionUtils::TaskTimer BackSolve;
            usolvem( model.N, 
		     min_row,
		     max_row_chl,
		     nrhs,
		     model.G->Values(),
		     model.G->iRows(),
		     model.G->pCols(),
		     chl_mass_inj_gpmin );
            if (options.report_scenario_timing) {
               BackSolve.StopAndSave(tmp_time);
               backsolve_times += tmp_time;
            }
         
            // Create a reference variable to make units make more obvious. The linear solver
            // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
            // use this reference when it make sense for units (e.g below the linear solve).
            float*& chl_conc_gpm3 = chl_mass_inj_gpmin;

	    const std::vector<int>::const_iterator booster_start = block_booster_ids.begin();
	    const std::vector<int>::const_iterator booster_stop = block_booster_ids.end();
            // analyze the solution
            for STL_ITERATE(std::vector<int>, u_pidx, u_pidx_end, nz_tox_loop) {
               const int u_idx = *u_pidx;
               float *xv_c = chl_conc_gpm3 + u_idx*nrhs;
               for (std::vector<int>::const_iterator b = booster_start; b != booster_stop; ++b) {
                  if (*(xv_c++) > zero_conc_gpm3) {
                     z_rows[u_idx].push_back(*b);
                  }
               }
            }

	    if (options.determine_single_impacts) {
	       for STL_ITERATE(std::vector<int>, u_pidx, u_pidx_end, nz_tox_loop) {
	          const int u_idx = *u_pidx;
		  float *xv_c = chl_conc_gpm3 + u_idx*nrhs;
		  for (std::vector<int>::const_iterator b = booster_start; b != booster_stop; ++b) {
		    if (*(xv_c++) <= zero_conc_gpm3) {
		      single_booster_impacts_g[*b] += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
		    }
		  }
	       }
	    }
         
	    // reset the sparse set of locations where the injections modifed the array,
	    // these may have been outside the region of interest in the linear solver,
	    // in which case, the BlasResetZero code after this loop would not have
	    // zero that region of the array
	    p_bid = p_bid_block_start;
	    for (int r = 0; (r < nrhs) && (p_bid != p_bid_stop); ++r, ++p_bid) {
	       booster_injection.NodeName() = model.NodeName(*p_bid);
	       booster_injection.ClearMultiArray(model, nrhs, r, chl_conc_gpm3);
	    }
            // reset to zero only the values where chl_conc_gpm3
            // was possibly modified by linear solver, this saves time
            const int num_elements = nrhs*(max_row_chl-min_row+1);
            const int p_offset = min_row*nrhs;
            BlasResetZero(num_elements, chl_conc_gpm3+p_offset);

	    ++current_block;
	    // for the last multiple-rhs block, reduce the leading dimension
	    // of the rhs/sol matrix to match the number of rhs left over.
	    // The extra memory that was allocated will just be left unused
	    if (current_block == total_blocks-1) {
	       nrhs = (odd_count)?(odd_count):(nrhs);
	    }
         }
      }
      else {
         // Code optimized for solving booster station sims using all single rhs solves
         // SLOWEST METHOD BUT IN SOME CASES MAY BE OPTIMIZED FOR DATA ACCESS
         //////////////////////////////////////////////
         Injection booster_injection = *(booster_scen.Injections().front());
	 for STL_CONST_ITERATE(std::vector<int>, p_bid, p_bid_end, chl_net.BoosterCandidates()) {
	    
	    int bnode_id = *p_bid;
	    booster_injection.NodeName() = model.NodeName(bnode_id);
	    float mass_injected_g(0.0);
	    int max_row_chl(model.N-1);
	    booster_injection.SetArray(model,chl_mass_inj_gpmin,mass_injected_g,max_row_chl);


            // do the backsolve
            merlionUtils::TaskTimer BackSolve;
            usolve ( model.N,
		     min_row, 
		     max_row_chl, 
		     model.G->Values(), 
		     model.G->iRows(), 
		     model.G->pCols(), 
		     chl_mass_inj_gpmin );
            if (options.report_scenario_timing) {
               BackSolve.StopAndSave(tmp_time);
               backsolve_times += tmp_time;
            }
         
            // Create a reference variable to make units make more obvious. The linear solver
            // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
            // use this reference when it make sense for units (e.g below the linear solve).
            float*& chl_conc_gpm3 = chl_mass_inj_gpmin;
         
            // analyze the solution
            for STL_ITERATE(std::vector<int>, u_pidx, u_pidx_end, nz_tox_loop) {
               if (chl_conc_gpm3[*u_pidx] > zero_conc_gpm3) {
                  z_rows[*u_pidx].push_back(bnode_id);
               }
            }

	    if (options.determine_single_impacts) {
	       for STL_ITERATE(std::vector<int>, u_pidx, u_pidx_end, nz_tox_loop) {
	          if (chl_conc_gpm3[*u_pidx] <= zero_conc_gpm3) {
		    single_booster_impacts_g[bnode_id] += tox_conc_gpm3[*u_pidx]*model.demand_ut_m3[*u_pidx];
		  }
	       }
	    }

	    // reset the sparse set of locations where the injections modifed the array,
	    // these may have been outside the region of interest in the linear solver,
	    // in which case, the BlasResetZero code after this loop would not have
	    // zero that region of the array
	    booster_injection.ClearArray(model, chl_conc_gpm3);
            // reset to zero only the values where chl_conc_gpm3
            // was possibly modified by linear solver, this saves time
            const int num_elements = max_row_chl-min_row+1;
            const int p_offset = min_row;
            BlasResetZero(num_elements, chl_conc_gpm3+p_offset);
         }
      }
   
      // Account for all other constants possibly omitted from objective function
      for STL_CONST_ITERATE(std::vector<int>, u_pidx, u_pidx_end, nz_tox_loop) {
         if (z_rows[*u_pidx].empty()) {
            post_detect_mc_g += tox_conc_gpm3[*u_pidx]*model.demand_ut_m3[*u_pidx];
         }
      }
      (*p_boost)->AddImpact(post_detect_mc_g,"Post-Detect Mass Consumed Grams");
      (*p_boost)->AddImpact(pre_detect_mc_g,"Pre-Detect Mass Consumed Grams");
   
      // add to the reduced version of the model
      // If key does not exist in reduced_z_rows map, it is initialized 
      // to zero as is the default constructor for double in C++.
      // reduced_z_rows maps a unique combination of booster stations to a weight in the 
      // objective function
      int ut_idx = 0;
      for STL_ITERATE(std::vector<std::vector<int> >, cn, cn_end, z_rows) {
         if (!(cn->empty())) {
            rows_orig += 1;
            nnz_orig += cn->size();
            reduced_z_rows[*cn] += tox_conc_gpm3[ut_idx]*model.demand_ut_m3[ut_idx];
         }
         ut_idx += 1;
      }
      rows_reduced = (int) reduced_z_rows.size();
      // Write the next scenario either starting the new file or appending data
      // to the previously started file
      merlionUtils::TaskTimer WriteOut;
      double tmp_io_time(0.0);
      bool is_first_scenario = (p_boost == net.BoosterScenarios().begin())?true:false;
      bool summary_only = !options.disable_reduced_problem;
      writer.WriteScenario(z_rows, tox_conc_gpm3, booster_scen, is_first_scenario, summary_only);
      WriteOut.StopAndSave(tmp_io_time);
   
      // reset the rhs for the toxin backsolves
      for STL_CONST_ITERATE(InjScenList, p_tox_scen, p_tox_stop, toxin_scenarios) {
         (*p_tox_scen)->ClearArray(model,tox_mass_inj_gpmin);
      }
      const int num_elements = max_row_tox-min_row+1;
      const int p_offset = min_row;
      BlasResetZero(num_elements, tox_conc_gpm3+p_offset);
   
      // Clear data associated with this scenario
      for STL_ITERATE(std::vector<std::vector<int> >, cn, cn_end, z_rows) {
         cn->clear();
      }
   
      if (options.report_scenario_timing) {
         ScenarioSim.StopAndSave(tmp_time);
         std::cout << "ORIG ROWS:   " << rows_orig+1 << std::endl;
         std::cout << "REDUCED ROWS: " << rows_reduced+1 << std::endl;
         std::cout << "\tSCENARIO ID:          " << (*p_boost)->Name() << std::endl;
         std::cout << "\t- Backsolves:         " << backsolve_times << std::endl;
         std::cout << "\t- Results Analysis:   " << tmp_time - tmp_io_time - backsolve_times << std::endl;
         std::cout << "\t- Writing Data Files: " << tmp_io_time << std::endl;
         std::cout << "\t- Scenario Total:     " << tmp_time << "\n" << std::endl;
      }
   }

   delete [] chl_mass_inj_gpmin;
   chl_mass_inj_gpmin = NULL;
   delete [] tox_mass_inj_gpmin;
   tox_mass_inj_gpmin = NULL;

   std::cout << "\n@@@ PERFORMING POSTPROCESSING @@@" << std::endl;
   // sort the rows in the problem in order of their weight in the
   // objective function. Reduce the problem size by eliminating the smallest
   // weights in the objective function whose combined effect will have a maximal
   // effect of no more than tol fraction of the entire objective function
   std::multimap<float, std::vector<int> > postp_reduced_z_rows;
   PostprocessRows(reduced_z_rows, postp_reduced_z_rows, options.postprocess_passes, options.postprocess_tol);
   
   // The preprocess problem is not contained in postp_reduced_z_rows
   // so we no longer need this container.
   reduced_z_rows.clear();
   if (!options.disable_reduced_problem) {
      std::cout << "\nWriting Reduced Problem" << std::endl;
      writer.WriteReducedProblem(postp_reduced_z_rows);
   }

   if (options.determine_single_impacts) {
     std::ofstream out;
     out.precision(12);
     out.setf(std::ios::scientific,std::ios::floatfield);
     out.open(std::string(options.output_prefix+"single_impacts.csv").c_str(), std::ios_base::out | std::ios_base::trunc);
     std::map<int, std::string>& node_id_map = writer.NodeMap();
     out << "PreBoosterMassConsumedGrams\n";
     out << total_pre_detect_mc_g << "\n";
     out << "NodeName, PostBoosterMassConsumedGrams\n";
     for (std::map<int,float>::const_iterator pos = single_booster_impacts_g.begin(),
	    pstop = single_booster_impacts_g.end();
	  pos != pstop;
	  ++pos) {
       out << node_id_map[pos->first] << "," << pos->second << "\n";
     }
     out.close();
   }

   // determine the reduced problem size
   typedef std::multimap<float, std::vector<int> > map_float_to_vect;
   for STL_ITERATE(map_float_to_vect, pos, stop, postp_reduced_z_rows) {
      nnz_reduced += pos->second.size();
   }
   rows_reduced = postp_reduced_z_rows.size();

   std::cout << std::endl;
   std::cout << "Optimization Problem Size: " << std::endl;
   std::cout << "- Original" << std::endl;
   std::cout << "\t constraints = " << rows_orig + 1 << std::endl;
   std::cout << "\t variables   = " << rows_orig + net.BoosterCandidates().size() << std::endl;
   std::cout << "\t nonzeros    = " << nnz_orig + rows_orig + net.BoosterCandidates().size() << std::endl;
   std::cout << "- Reduced" << std::endl;
   std::cout << "\t constraints = " << rows_reduced + 1 << std::endl;
   std::cout << "\t variables   = " << rows_reduced + net.BoosterCandidates().size() << std::endl;
   std::cout << "\t nonzeros    = " << nnz_reduced + rows_reduced + net.BoosterCandidates().size() << std::endl;

   //std::map<int, double> greedy_solution;
   //double max_value;
   //GreedySearch(postp_reduced_z_rows, greedy_solution, max_value);
   
   //TODO:
   //send the greedy solution to the data writer

   postp_reduced_z_rows.clear();  
}

void PerformDosageSimulations(const BoosterNetworkSimulator& chl_net, const NetworkSimulator& tox_net, const BoosterSimOptions& options, BoosterDataWriter& writer)
{
   const float zero_conc_gpm3 = std::max(options.zero_conc_tol_gpm3,ZERO_CONC_GPM3);

   const BoosterNetworkSimulator& net = chl_net;
   const MerlionModelContainer& model = chl_net.Model();
   const MerlionModelContainer& tox_model = tox_net.Model();

   int num_booster_stations = net.BoosterCandidates().size();
   std::cout << "\nNetwork Stats:\n";
   std::cout << "\tNumber of Booster Station Candidates - " << num_booster_stations << "\n";
   std::cout << "\tNumber of Junctions                  - " << model.junctions.size() << "\n";
   std::cout << "\tNumber of Nonzero Demand Junctions   - " << model.nzd_junctions.size() << "\n";
   std::cout << "\tNumber of Tanks                      - " << model.tanks.size() << "\n";
   std::cout << "\tNumber of Reservoirs                 - " << model.reservoirs.size() << "\n";
   std::cout << "\tWater Quality Timestep (minutes)     - " << model.qual_step_minutes << "\n";
   std::cout << std::endl;

   // A shortcut for specifying all possible 
   // right hand sides in one call of the multiple
   // right hand side triangular solver
   int nrhs = options.max_rhs;
   if (nrhs <= 0) {
      nrhs = num_booster_stations; 
   }

   // Allocate the right hand side and solution vector for toxin injection sims
   float* tox_mass_inj_gpmin = new float[model.N];
   BlasInitZero(model.N, tox_mass_inj_gpmin);

   std::vector<double> volume_ingested_ut_m3(model.N,0.0);
   std::set<int> positive_volume_ingested_u_ids;
   for (int nt_idx = 0; nt_idx < model.N; ++nt_idx) {
      if (tox_net.VolumeIngested_m3()[nt_idx] > 0.0) {
         volume_ingested_ut_m3[model.perm_nt_to_upper[nt_idx]] = tox_net.VolumeIngested_m3()[nt_idx];
         positive_volume_ingested_u_ids.insert(model.perm_nt_to_upper[nt_idx]);
      }
   }
   
   std::vector<float> node_total_dose_g(model.N, 0.0);
   bool first_solve = true;
   const size_t num_scenarios = chl_net.BoosterScenarios().size();
   size_t current_scenario = 0;
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net.BoosterScenarios()) {

      current_scenario++;
      merlionUtils::TaskTimer ScenarioSim;

      const BoosterScenario& booster_scen = **p_boost;
      assert(booster_scen.ToxinScenarios().size() == 1);
      const InjScenario& scen = *(booster_scen.ToxinScenarios().front());

      /////////////// Determine a few parameters that will speed up the solves below
      const int sim_stop_timestep = booster_scen.sim_stop_timestep;   
      const int min_row = (options.no_reduce)?(0):(model.N-model.n_nodes*(sim_stop_timestep+1));
   
      // set the rhs for the toxin injection
      int max_row_tox(0);
      float mass_injected_g(0.0); //unused
      scen.SetArray(model, tox_mass_inj_gpmin, mass_injected_g, max_row_tox);  
   
      if (options.no_reduce) {
         max_row_tox = model.N-1; 
      }
   
      // Solver the linear system
      double tmp_time, backsolve_times(0.0);
      merlionUtils::TaskTimer BackSolve;
      usolve ( tox_model.N,
	       min_row,
	       max_row_tox,
	       tox_model.G->Values(),
	       tox_model.G->iRows(),
	       tox_model.G->pCols(),
	       tox_mass_inj_gpmin );
      if (options.report_scenario_timing) {
         BackSolve.StopAndSave(tmp_time);
         backsolve_times += tmp_time;
      }
   
      // Create a reference variable to make units make more
      // obvious. The linear solver converts the rhs vector (units
      // gpmin) into the solution vector (gpm3). We will use this
      // reference when it make sense for units (e.g below the linear
      // solve).
      float*& tox_conc_gpm3 = tox_mass_inj_gpmin;

      // calculate total node dosage over the period of interest
      const int u_idx_start = model.N - (sim_stop_timestep+1)*model.n_nodes;
      std::set<int>::const_iterator p_vi_u_idx = positive_volume_ingested_u_ids.begin();
      while(*p_vi_u_idx < u_idx_start) {++p_vi_u_idx;}
      std::set<int>::const_iterator p_vi_u_idx_stop = --(positive_volume_ingested_u_ids.end());
      while(*p_vi_u_idx_stop > max_row_tox) {--p_vi_u_idx_stop;}
      ++p_vi_u_idx_stop;
      for (; p_vi_u_idx != p_vi_u_idx_stop; ++p_vi_u_idx) {
         int u_idx = *p_vi_u_idx;
         int n = model.perm_upper_to_nt[u_idx] / model.n_steps;
         node_total_dose_g[n] += tox_conc_gpm3[u_idx] * volume_ingested_ut_m3[u_idx];
      }

      // Write the next scenario either starting the new file or appending data
      // to the previously started file
      merlionUtils::TaskTimer WriteOut;
      double tmp_io_time(0.0);
      bool leave_open = (current_scenario == num_scenarios)?(false):(true);
      writer.AppendPopulationDosageData(booster_scen.Name(), node_total_dose_g, first_solve, leave_open);
      first_solve = false;
      WriteOut.StopAndSave(tmp_io_time);

      // reset the node dose vector
      BlasResetZero(model.N, &(node_total_dose_g[0]));

      // reset the rhs for the toxin backsolve
      scen.ClearArray(model,tox_mass_inj_gpmin);
      const int num_elements = max_row_tox-min_row+1;
      const int p_offset = min_row;
      BlasResetZero(num_elements, tox_conc_gpm3+p_offset);

      if (options.report_scenario_timing) {
         ScenarioSim.StopAndSave(tmp_time);
         std::cout << "\tSCENARIO ID:          " << scen.Name() << std::endl;
         std::cout << "\t- Backsolves:         " << backsolve_times << std::endl;
         std::cout << "\t- Results Analysis:   " << tmp_time - tmp_io_time - backsolve_times << std::endl;
         std::cout << "\t- Writing Data Files: " << tmp_io_time << std::endl;
         std::cout << "\t- Scenario Total:     " << tmp_time << "\n" << std::endl;
      }
   }

   delete [] tox_mass_inj_gpmin;
   tox_mass_inj_gpmin = NULL;
}

void PostprocessRows(std::map<std::vector<int>, float>& reduced_z_rows, std::multimap<float, std::vector<int> >& postp_reduced_z_rows, int repeat, float tol)
{
   
   typedef std::map<std::vector<int>, float> map_vect_to_float;
   typedef std::multimap<float, std::vector<int> > map_float_to_vect;

   // Copy the reduced_z_rows into a multimap which uses the float vals as keys.
   // This way the new map is odered by the objective weight and not the std::vector<int>.
   // We use a multimap just in case some rows have identical weights.
   double total_sum(0.0);
   int nnz_reduced_orig(0);
   for STL_ITERATE(map_vect_to_float, pos, p_stop, reduced_z_rows) {
      total_sum += pos->second;
      nnz_reduced_orig += pos->first.size();
      postp_reduced_z_rows.insert(std::pair<float, std::vector<int> >(pos->second,pos->first));
   }
   reduced_z_rows.clear();

   double median(0.0);
   if (postp_reduced_z_rows.empty()) {
     return;
   }
   if ((postp_reduced_z_rows.size()%2) == 0) {
      map_float_to_vect::iterator pos = postp_reduced_z_rows.begin();
      int cnt = 0;
      int stop = postp_reduced_z_rows.size()/2;
      while (++cnt != stop) {++pos;}
      median = (pos++)->first;
      median += pos->first;
      median /= 2.0;
   }
   else {
      map_float_to_vect::iterator pos = postp_reduced_z_rows.begin();
      int cnt = 0;
      int stop = (postp_reduced_z_rows.size()+1)/2;
      while (++cnt != stop) {++pos;}
      median = pos->first;
   }
   std::cout << "Before Postprocessing:" << std::endl;
   std::cout << "Smallest Nonzero in Objective: " << postp_reduced_z_rows.begin()->first << std::endl; 
   std::cout << "                       Median: " << median << std::endl;
   std::cout << " Largets Nonzero in Objective: " << (--postp_reduced_z_rows.end())->first << std::endl;
   std::cout << std::endl;

   // Determine the position in the multimap at which the sum of the previous weights
   // is no more the tol fraction of the total sum of all weights. It is safe to assume
   // eliminating all rows before this point will have a combined affect of no more than
   // tol fraction of the objective function on the final solution. This will safely remove
   // variables with weights like 1e-20 in the objective function, while making sure the entire
   // problem is not scaled in this way by the user.
   for (int pass = 0; pass < repeat; ++pass) {
      int postp_reduced_z_rows_orig_size = postp_reduced_z_rows.size();
      double current_sum(0.0);
      int count_rows(0), count_nnz(0);
      map_float_to_vect::iterator stop_del;
      for STL_ITERATE(map_float_to_vect, pos, stop, postp_reduced_z_rows) {
         current_sum += pos->first;
         if ((current_sum)/(total_sum) > tol) {
            stop_del = pos;      
            break;
         }
         count_nnz += pos->second.size();
         count_rows++;
      }
      postp_reduced_z_rows.erase(postp_reduced_z_rows.begin(), stop_del);
   
      median = 0.0;
      if ((postp_reduced_z_rows.size()%2) == 0) {
         map_float_to_vect::iterator pos = postp_reduced_z_rows.begin();
         int cnt = 0;
         int stop = postp_reduced_z_rows.size()/2;
	 while (++cnt != stop) {++pos;}
         median = (pos++)->first;
         median += pos->first;
         median /= 2.0;
      }
      else {
         map_float_to_vect::iterator pos = postp_reduced_z_rows.begin();
         int cnt = 0;
         int stop = (postp_reduced_z_rows.size()+1)/2;
         while (++cnt != stop) {++pos;}
         median = pos->first;
      }
   
      std::cout << "\nPostprocessing the reduced problem eliminated:\n" << 
            " " << count_rows << " variables (" << ((double)count_rows)/((double)postp_reduced_z_rows_orig_size)*100 << " %)\n" <<
            " " << count_rows << " constraints (" << ((double)count_rows)/((double)postp_reduced_z_rows_orig_size)*100 << " %)\n" <<
            " " << count_nnz  << " nonzeros (" << ((double)count_nnz)/((double)nnz_reduced_orig)*100 << " %)\n";
      std::cout << "Smallest Nonzero in Objective: " << postp_reduced_z_rows.begin()->first << std::endl;
      std::cout << "                       Median: " << median << std::endl;
      std::cout << " Largest Nonzero in Objective: " << (--postp_reduced_z_rows.end())->first << std::endl;
      std::cout << std::endl;
   }
}


void GreedySearch(std::multimap<float, std::vector<int> >& submodf, std::map<int, double>& greed_sol, double max_value)
{
   if (submodf.empty()) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Set of candidates can not be empty for the greedy search algorithm.\n";
      std::cerr << std::endl;
      exit(1); 
   }   
   
   // Find the maximum value of the submodular function
   typedef std::multimap<float, std::vector<int> > map_float_to_vect;
   max_value = 0.0;
   for STL_ITERATE(map_float_to_vect, pos, stop, submodf) {
      max_value += pos->first;
   }

   // Determine the delta increase to the objective for 
   // of of the candidate nodes
   typedef std::map<int, double> map_int_to_double;
   std::map<int, double> delta;
   for STL_ITERATE(map_float_to_vect, pos, stop, submodf) {
      double val = pos->first;
      std::vector<int>& v = pos->second;
      for STL_ITERATE(std::vector<int>, pos_v, stop_v, v) {
         delta[*pos_v] += val;
      }
   }

   // The greedy solution
   std::set<int> choices;

   double current_obj = 0.0;
   for (int i = 1; !submodf.empty(); ++i) {
      std::multimap<double, int> ordered_choices;
      for STL_ITERATE(map_int_to_double, pos, stop, delta) {
	 // if the the node is not already in the greedy solution
	 // add it ordered choices set to find the best candidate
         if (choices.count(pos->first) == 0) {
            ordered_choices.insert(std::pair<double,int>(pos->second,pos->first)); 
         }
      }
      std::multimap<double, int>::iterator last = (--ordered_choices.end());
      int node = last->second;
      double val = last->first;
      choices.insert(node);
      current_obj += val;

      greed_sol[node] = val;
      
      delta.clear();
      std::list<map_float_to_vect::iterator> erase_list;
      for STL_ITERATE(map_float_to_vect, pos, stop, submodf) {
         double val = pos->first;
         std::vector<int>& v = pos->second;
         int cntr = 0;
         for STL_ITERATE(std::vector<int>, pos_v, stop_v, v) {
            cntr += choices.count(*pos_v);
	    if (cntr) {
	       erase_list.push_back(pos);
	       break;
	    }
         }
         if (!cntr) {
            for STL_ITERATE(std::vector<int>, pos_v, stop_v, v) {
               delta[*pos_v] += val;
            }
         }
      }
      // remove the elements no longer needed from submodf
      for STL_ITERATE(std::list<map_float_to_vect::iterator>, pos, stop, erase_list) {
	 submodf.erase(*pos);
      }
   }
}



















void PerformBoosterDosageSimulations(const BoosterNetworkSimulator& chl_net, const NetworkSimulator& tox_net, const BoosterSimOptions& options, BoosterDataWriter& writer)
{
   const float zero_conc_gpm3 = std::max(options.zero_conc_tol_gpm3,ZERO_CONC_GPM3);

   const BoosterNetworkSimulator& net = chl_net;
   const MerlionModelContainer& model = chl_net.Model();
   const MerlionModelContainer& tox_model = tox_net.Model();

   int num_booster_stations = net.BoosterCandidates().size();
   std::cout << "\nNetwork Stats:\n";
   std::cout << "\tNumber of Booster Station Candidates - " << num_booster_stations << "\n";
   std::cout << "\tNumber of Junctions                  - " << model.junctions.size() << "\n";
   std::cout << "\tNumber of Nonzero Demand Junctions   - " << model.nzd_junctions.size() << "\n";
   std::cout << "\tNumber of Tanks                      - " << model.tanks.size() << "\n";
   std::cout << "\tNumber of Reservoirs                 - " << model.reservoirs.size() << "\n";
   std::cout << "\tWater Quality Timestep (minutes)     - " << model.qual_step_minutes << "\n";
   std::cout << std::endl;

   // A shortcut for specifying all possible 
   // right hand sides in one call of the multiple
   // right hand side triangular solver
   int nrhs = options.max_rhs;
   if (nrhs <= 0) {
      nrhs = num_booster_stations; 
   }

   // Allocate the right hand side and solution vector for toxin injection sims
   float* tox_mass_inj_gpmin = new float[model.N];
   BlasInitZero(model.N, tox_mass_inj_gpmin);

   // Allocate memory for the rhs and solution vector for the booster sims
   // depending on the number right hand sides specified in options
   float* chl_mass_inj_gpmin(NULL);
   if (nrhs > 1) {
      int TMP_RHS = (nrhs<num_booster_stations)?nrhs:num_booster_stations;
      chl_mass_inj_gpmin = new float[model.N * TMP_RHS];
      BlasInitZero(model.N*TMP_RHS, chl_mass_inj_gpmin);
   }
   else {
      chl_mass_inj_gpmin = new float[model.N];
      BlasInitZero(model.N, chl_mass_inj_gpmin);
   }


   int new_column_counter = 1;
   std::map<std::vector<int>, int> reduced_z_cols;
   std::map<std::vector<int>, int>::const_iterator col_location;
   std::vector<std::vector<int> > z_rows(model.N);
   std::vector<std::map<int,float> > node_objective_terms(model.n_nodes);
   std::vector<std::map<std::vector<int>, int>::const_iterator> new_cols;
   // mass dosed whether or not we use booster stations
   std::vector<float> always_dosed_g(model.n_nodes,0.0);
   // mass dosed if no booster stations are present
   std::vector<float> node_total_dose_g(model.n_nodes, 0.0);

   std::vector<double> volume_ingested_ut_m3(model.N, 0.0);
   std::set<int> positive_volume_ingested_u_ids;
   for (int nt_idx = 0; nt_idx < model.N; ++nt_idx) {
      if (tox_net.VolumeIngested_m3()[nt_idx] > 0.0) {
         volume_ingested_ut_m3[model.perm_nt_to_upper[nt_idx]] = tox_net.VolumeIngested_m3()[nt_idx];
         positive_volume_ingested_u_ids.insert(model.perm_nt_to_upper[nt_idx]);
      }
   }

   // keep track of size of optimization problem
   // defined by the data files being written
   unsigned long long rows_orig(0);	// constraints
   unsigned long long rows_reduced(0);
   unsigned long long nnz_orig(0);     
   unsigned long long nnz_reduced(0);

   // The main solve loop for booster station simulations
   // Outer loop simulates toxin injection scenarios
   // Inner loop simulates booster station injection applied to toxin scenarios
   // Scenario results are written at the end of each iteration of the outer toxin
   // loop. Files are written such that once an iteration end, files can be used in MIP
   // formulations as is. This is to accommodate for long simulations.
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, net.BoosterScenarios()) {

      merlionUtils::TaskTimer ScenarioSim;
      merlionUtils::TaskTimer BackSolve(false);
      merlionUtils::TaskTimer WriteOut(false);

      const BoosterScenario& booster_scen = **p_boost;

      // We need to assign the node name before using the 
      // booster injection template (this single Injection is
      // a sparse representation of an identical Injection
      // for every booster station)
      assert(booster_scen.Injections().size() == 1);

      /////////////// Determine a few parameters that will speed up the solves below
      const int chl_start_timestep = booster_scen.Injections().front()->StartTimestep(model);
      int max_row_chl = model.N-model.n_nodes*chl_start_timestep-1;
      int min_row_no_chl = max_row_chl+1;

      const int sim_stop_timestep = booster_scen.sim_stop_timestep;
      const int min_row = (options.no_reduce)?(0):(model.N-model.n_nodes*(sim_stop_timestep+1));

      // find the start of the earliest toxin scenario injection
      int max_row_tox(0);
      const InjScenList& toxin_scenarios = booster_scen.ToxinScenarios();
      for STL_CONST_ITERATE(InjScenList, p_tox_scen, p_tox_stop, toxin_scenarios) {
         int max_row = (*p_tox_scen)->MaxIndexChanged(model);
	 if (max_row > max_row_tox) {
	    max_row_tox = max_row;
	 }
      }
   
      if (options.no_reduce) {
         max_row_tox = model.N-1; 
      }

      const int u_idx_start = model.N - (sim_stop_timestep+1)*model.n_nodes;
      std::set<int>::const_iterator p_vi_u_idx;
      std::set<int>::const_iterator p_vi_u_idx_start = positive_volume_ingested_u_ids.begin();
      while(*p_vi_u_idx_start < u_idx_start) {++p_vi_u_idx_start;}
      std::set<int>::const_iterator p_vi_u_idx_stop_chl = --(positive_volume_ingested_u_ids.end());
      while(*p_vi_u_idx_stop_chl > max_row_chl) {--p_vi_u_idx_stop_chl;}
      ++p_vi_u_idx_stop_chl;
      std::set<int>::const_iterator p_vi_u_idx_start_prechl = --(positive_volume_ingested_u_ids.end());
      while(*p_vi_u_idx_start_prechl > min_row_no_chl) {--p_vi_u_idx_start_prechl;}

      max_row_chl = 0;
      if (nrhs > 1) {
         // Code optimized for solving booster station sims using set of matrix rhs
         // FASTED METHOD WHERE MEMORY IS LEVERAGED WITH SPEED
         //////////////////////////////////////////////
         if (nrhs > num_booster_stations) {
            nrhs = num_booster_stations;
         }
         const int total_blocks = (num_booster_stations/nrhs) + (((num_booster_stations%nrhs)>0)?1:0);
	 const int odd_count = num_booster_stations%nrhs;
	 int current_block(0);

	 std::vector<int>::const_iterator p_bid = chl_net.BoosterCandidates().begin();
	 std::vector<int>::const_iterator p_bid_stop = chl_net.BoosterCandidates().end();
         std::vector<int>::const_iterator p_bid_block_start;
	 Injection booster_injection = *(booster_scen.Injections().front());
	 
	 std::vector<int> block_booster_ids;
         while (current_block < total_blocks) {
	    block_booster_ids.resize(nrhs);
	    p_bid_block_start = p_bid;
	    // set the rhs matrix
	    int max_row_chl_block = 0;
	    for (int r = 0; (r < nrhs) && (p_bid != p_bid_stop); ++r, ++p_bid) {
	       float mass_injected_g(0.0);
	       int max_row_index(model.N-1);
	       booster_injection.NodeName() = model.NodeName(*p_bid);
	       booster_injection.SetMultiArray(model,nrhs,r,chl_mass_inj_gpmin,mass_injected_g,max_row_index);
	       block_booster_ids[r] = *p_bid;
	       if (max_row_index > max_row_chl_block) {
		  max_row_chl_block = max_row_index;
	       }
            }

            if (max_row_chl_block > max_row_chl) {
               max_row_chl = max_row_chl_block;
            }
            
            // do the backsolve
            BackSolve.Start();
            usolvem( model.N, 
		     min_row,
		     max_row_chl_block,
		     nrhs,
		     model.G->Values(),
		     model.G->iRows(),
		     model.G->pCols(),
		     chl_mass_inj_gpmin );
            if (options.report_scenario_timing) {
               BackSolve.Stop();
            }
         
            // Create a reference variable to make units make more obvious. The linear solver
            // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
            // use this reference when it make sense for units (e.g below the linear solve).
            float*& chl_conc_gpm3 = chl_mass_inj_gpmin;

	    const std::vector<int>::const_iterator booster_start = block_booster_ids.begin();
	    const std::vector<int>::const_iterator booster_stop = block_booster_ids.end();
            // analyze the solution
            for (p_vi_u_idx = p_vi_u_idx_start; p_vi_u_idx != p_vi_u_idx_stop_chl; ++p_vi_u_idx) {
               const int u_idx = *p_vi_u_idx;
               float *xv_c = chl_conc_gpm3 + u_idx*nrhs;
               for (std::vector<int>::const_iterator b = booster_start; b != booster_stop; ++b) {
                  if (*(xv_c++) > zero_conc_gpm3) {
                     z_rows[u_idx].push_back(*b);
                  }
               }
            }

	    // reset the sparse set of locations where the injections modifed the array,
	    // these may have been outside the region of interest in the linear solver,
	    // in which case, the BlasResetZero code after this loop would not have
	    // zero that region of the array
	    p_bid = p_bid_block_start;
	    for (int r = 0; (r < nrhs) && (p_bid != p_bid_stop); ++r, ++p_bid) {
	       booster_injection.NodeName() = model.NodeName(*p_bid);
	       booster_injection.ClearMultiArray(model, nrhs, r, chl_conc_gpm3);
	    }
            // reset to zero only the values where chl_conc_gpm3
            // was possibly modified by linear solver, this saves time
            const int num_elements = nrhs*(max_row_chl_block-min_row+1);
            const int p_offset = min_row*nrhs;
            BlasResetZero(num_elements, chl_conc_gpm3+p_offset);

	    ++current_block;
	    // for the last multiple-rhs block, reduce the leading dimension
	    // of the rhs/sol matrix to match the number of rhs left over.
	    // The extra memory that was allocated will just be left unused
	    if (current_block == total_blocks-1) {
	       nrhs = (odd_count)?(odd_count):(nrhs);
	    }
         }
      }
      else {
         // Code optimized for solving booster station sims using all single rhs solves
         // SLOWEST METHOD BUT IN SOME CASES MAY BE OPTIMIZED FOR DATA ACCESS
         //////////////////////////////////////////////
         Injection booster_injection = *(booster_scen.Injections().front());
	 for STL_CONST_ITERATE(std::vector<int>, p_bid, p_bid_end, chl_net.BoosterCandidates()) {
	    
	    int bnode_id = *p_bid;
	    booster_injection.NodeName() = model.NodeName(bnode_id);
	    float mass_injected_g(0.0);
	    int max_row_chl_index(model.N-1);
	    booster_injection.SetArray(model,chl_mass_inj_gpmin,mass_injected_g,max_row_chl_index);

            if (max_row_chl_index > max_row_chl) {
               max_row_chl = max_row_chl_index;
            }

            // do the backsolve
            BackSolve.Start();
            usolve ( model.N,
		     min_row, 
		     max_row_chl_index, 
		     model.G->Values(), 
		     model.G->iRows(), 
		     model.G->pCols(), 
		     chl_mass_inj_gpmin );
            if (options.report_scenario_timing) {
               BackSolve.Stop();
            }
         
            // Create a reference variable to make units make more obvious. The linear solver
            // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
            // use this reference when it make sense for units (e.g below the linear solve).
            float*& chl_conc_gpm3 = chl_mass_inj_gpmin;
         
            // analyze the solution
            for (p_vi_u_idx = p_vi_u_idx_start; p_vi_u_idx != p_vi_u_idx_stop_chl; ++p_vi_u_idx) {
               if (chl_conc_gpm3[*p_vi_u_idx] > zero_conc_gpm3) {
                  z_rows[*p_vi_u_idx].push_back(bnode_id);
               }
            }

            // reset the sparse set of locations where the injections modifed the array,
	    // these may have been outside the region of interest in the linear solver,
	    // in which case, the BlasResetZero code after this loop would not have
	    // zero that region of the array
	    booster_injection.ClearArray(model, chl_conc_gpm3);
            // reset to zero only the values where chl_conc_gpm3
            // was possibly modified by linear solver, this saves time
            const int num_elements = max_row_chl_index-min_row+1;
            const int p_offset = min_row;
            BlasResetZero(num_elements, chl_conc_gpm3+p_offset);
         }
      }

      bool is_first_scenario = (p_boost == net.BoosterScenarios().begin())?true:false;

      for STL_CONST_ITERATE(InjScenList, p_tox_scen, p_tox_stop, toxin_scenarios) {
            
         const InjScenario& toxin_scen = **p_tox_scen;

         bool do_scaling = false;
         float average_flow_magnitude = 0.0;
         for STL_CONST_ITERATE(InjectionList, p_tox_inj, p_tox_inj_top, toxin_scen.Injections()) {
            average_flow_magnitude += model.node_average_flow_magnitude_m3pmin[model.NodeID((*p_tox_inj)->NodeName())];
            if ((*p_tox_inj)->Type() == InjType_Mass) {
               do_scaling = true;
            }
         }
         if (!toxin_scen.Injections().empty()) {
            average_flow_magnitude /= (float)toxin_scen.Injections().size();
         }
         float scenario_mass_injected_tox_g;
         int scenario_max_row_tox;
         float scenario_scaling = 1.0;
         if ((do_scaling) && (average_flow_magnitude != 0)) {
            scenario_scaling = average_flow_magnitude;
         }
         toxin_scen.SetArrayScaled(scenario_scaling,
                                   model,
                                   tox_mass_inj_gpmin,
                                   scenario_mass_injected_tox_g,
                                   scenario_max_row_tox);

         // Solver the linear system
         BackSolve.Start();
         usolve(tox_model.N,
                min_row,
                scenario_max_row_tox,
                tox_model.G->Values(),
                tox_model.G->iRows(),
                tox_model.G->pCols(),
                tox_mass_inj_gpmin);
         if (options.report_scenario_timing) {
            BackSolve.Stop();
         }

         // Create a reference variable to make units make more obvious. The linear solver
         // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
         // use this reference when it make sense for units (e.g below the linear solve).
         float*& tox_conc_gpm3 = tox_mass_inj_gpmin; 

         // constant values omitted from MIP objective function stored here
         int first_injection_timestep = model.perm_upper_to_nt[scenario_max_row_tox] % model.n_steps;
         if (first_injection_timestep != chl_start_timestep) {
            std::set<int>::const_iterator p_vi_u_idx_stop = --(positive_volume_ingested_u_ids.end());
            while(*p_vi_u_idx_stop > scenario_max_row_tox) {--p_vi_u_idx_stop;}
            ++p_vi_u_idx_stop;
            for (p_vi_u_idx = p_vi_u_idx_start_prechl; p_vi_u_idx != p_vi_u_idx_stop; ++p_vi_u_idx) {
               int u_idx = *p_vi_u_idx;
               float tmp_conc_gpm3 = tox_conc_gpm3[u_idx];
               if (tmp_conc_gpm3 > zero_conc_gpm3){
                  int nt_idx = model.perm_upper_to_nt[u_idx];
                  int node = nt_idx / model.n_steps;
                  always_dosed_g[node] += tmp_conc_gpm3 * volume_ingested_ut_m3[u_idx];
               }
            }
         }

         // add to the reduced version of the model If key does not
         // exist in reduced_z_cols map, it is initialized to zero as
         // is the default constructor for double in C++.
         // reduced_z_cols maps a unique combination of booster
         // stations to a weight in the objective function
         for (p_vi_u_idx = p_vi_u_idx_start; p_vi_u_idx != p_vi_u_idx_stop_chl; ++p_vi_u_idx) {
            if (tox_conc_gpm3[*p_vi_u_idx] > zero_conc_gpm3) {
               int u_idx = *p_vi_u_idx;
               float dose_g = tox_conc_gpm3[u_idx] * volume_ingested_ut_m3[u_idx];
               int node = model.perm_upper_to_nt[u_idx] / model.n_steps;
               if (!(z_rows[u_idx].empty())) {
                  rows_orig += 1;
                  nnz_orig += z_rows[u_idx].size();
                  int& tag_ref = reduced_z_cols[z_rows[u_idx]];
                  col_location = reduced_z_cols.find(z_rows[u_idx]);
                  if (!options.disable_reduced_problem) {
                     if (tag_ref == 0) {
                        // A new col
                        tag_ref = new_column_counter++;
                        new_cols.push_back(col_location);
                     }
                  }
                  else {
                     tag_ref = new_column_counter++;
                     new_cols.push_back(col_location);
                  }
                  node_objective_terms[node][tag_ref] += dose_g;
                  node_total_dose_g[node] += dose_g;
               }
               else {
                  always_dosed_g[node] += dose_g;
               }
            }
         }
         rows_reduced = (int) reduced_z_cols.size();

         cblas_saxpy(model.n_nodes, 1.0f, &(always_dosed_g[0]), 1, &(node_total_dose_g[0]), 1);

         // Write the next scenario either starting the new file or appending data
         // to the previously started file
         WriteOut.Start();
         bool is_first_scenario = p_tox_scen == (net.BoosterScenarios().front()->ToxinScenarios().begin());
         bool is_last_scenario = p_tox_scen == --(net.BoosterScenarios().back()->ToxinScenarios().end());
         writer.WriteDosageScenario(node_objective_terms,
                                    always_dosed_g,
                                    new_cols,
                                    toxin_scen,
                                    scenario_scaling,
                                    is_first_scenario);
         writer.AppendPopulationDosageData(toxin_scen.Name(),
                                           node_total_dose_g,
                                           is_first_scenario,
                                           !is_last_scenario);
         WriteOut.Stop();
   
         // reset the rhs for the toxin backsolve
         toxin_scen.ClearArray(model,tox_mass_inj_gpmin);
         const int num_elements = scenario_max_row_tox-min_row+1;
         const int p_offset = min_row;
         BlasResetZero(num_elements, tox_conc_gpm3+p_offset);
         
         // Clear data associated with this toxin scenario
         new_cols.clear();
         BlasResetZero(model.n_nodes, &(node_total_dose_g[0]));
         BlasResetZero(model.n_nodes, &(always_dosed_g[0]));
         for (int n = 0; n < model.n_nodes; ++n) {
            node_objective_terms[n].clear();
         }
      }

      // Clear data associated with this scenario
      for STL_ITERATE(std::vector<std::vector<int> >, cn, cn_end, z_rows) {
         cn->clear();
      }

      ScenarioSim.Stop();
      if (options.report_scenario_timing) {
         std::cout << "ORIG ROWS:   " << rows_orig+1 << std::endl;
         std::cout << "REDUCED ROWS: " << rows_reduced+1 << std::endl;
         std::cout << "\tSCENARIO ID:          " << (*p_boost)->Name() << std::endl;
         std::cout << "\t- Backsolves:         " << BackSolve.Status() << std::endl;
         std::cout << "\t- Results Analysis:   " << ScenarioSim.Status() - WriteOut.Status() - BackSolve.Status() << std::endl;
         std::cout << "\t- Writing Data Files: " << WriteOut.Status() << std::endl;
         std::cout << "\t- Scenario Total:     " << ScenarioSim.Status() << "\n" << std::endl;
      }
   }

   delete [] chl_mass_inj_gpmin;
   chl_mass_inj_gpmin = NULL;
   delete [] tox_mass_inj_gpmin;
   tox_mass_inj_gpmin = NULL;

   std::cout << std::endl;
   std::cout << "Optimization Problem Size: " << std::endl;
   std::cout << "- Original" << std::endl;
   std::cout << "\t constraints = " << rows_orig + 1 << std::endl;
   std::cout << "\t variables   = " << rows_orig + net.BoosterCandidates().size() << std::endl;
   std::cout << "\t nonzeros    = " << nnz_orig + rows_orig + net.BoosterCandidates().size() << std::endl;
   std::cout << "- Reduced" << std::endl;
   std::cout << "\t constraints = " << rows_reduced + 1 << std::endl;
   std::cout << "\t variables   = " << rows_reduced + net.BoosterCandidates().size() << std::endl;
   std::cout << "\t nonzeros    = " << nnz_reduced + rows_reduced + net.BoosterCandidates().size() << std::endl;

}

