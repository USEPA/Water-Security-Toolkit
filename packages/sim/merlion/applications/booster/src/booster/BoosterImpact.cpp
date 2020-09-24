#include <booster/BoosterNetworkSimulator.hpp>
#include <booster/BoosterImpactOptions.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/MerlionDefines.hpp>
#include <merlion/SparseMatrix.hpp>

#include <merlionUtils/TaskTimer.hpp>

#include <set>
#include <exception>
#include <algorithm>


using namespace merlionUtils;

//forward declarations
void PerformBoosterSimulations(const BoosterNetworkSimulator& chl_net,
                               const NetworkSimulator& tox_net,
			       const BoosterImpactOptions& options,
                               bool pd_impacts=false);

int main(int argc, char** argv)
{  
   // Handle the optional inputs to this executable
   BoosterImpactOptions options;
   options.ParseInputs(argc, argv);

   // Helpful interface for setting up the booster station simulations
   BoosterNetworkSimulator chl_net(options.disable_warnings);
   NetworkSimulator tox_net(true);
   
   if (options.logging) {
      chl_net.StartLogging(options.output_prefix+"boosterimpact.log");  
      tox_net.StartLogging(options.output_prefix+"boosterimpact.log");  
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


   // **A weak way of checking the the models match (except for, of course, their decay coefficients)
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
   bool scenario_aggregation = true;
   tox_net.ClassifyScenarios(detection_times_provided,
                             tox_net.Model().n_steps-1,
                             options.detection_tol_gpm3,
                             options.detection_interval_min);
   chl_net.CopyScenarioList(tox_net);
   chl_net.ClassifyBoosterScenarios(true,
                                    scenario_aggregation, 
                                    options.detection_tol_gpm3,
                                    options.detection_interval_min);
   Classify.StopAndPrint(std::cout, "\n\t- Classified Scenarios ");

   bool pd_impacts = false;
   if (options.demand_percapita_m3pmin ||
       options.ingestion_rate_m3pmin) {
      pd_impacts = true;
      merlionUtils::TaskTimer GeneratePopUsageData;
      tox_net.GeneratePopulationUsageData(options.demand_percapita_m3pmin,
                                          options.ingestion_rate_m3pmin);
      GeneratePopUsageData.StopAndPrint(std::cout, "\n\t- Generated Population Usage Data");
   }

   // Free the memory for the reduced system inverse
   tox_net.ResetInverse();
   // Free the memory for the list of sensors
   tox_net.ResetSensorData();

   double booster_seconds = 0.0;
   std::cout << "\n@@@ SIMULATING BOOSTER STATIONS @@@" << std::endl;
   merlionUtils::TaskTimer SimAndSave;
   PerformBoosterSimulations(chl_net,tox_net,options,pd_impacts); 
   SimAndSave.StopAndSave(booster_seconds);

   double sims_per_second = (chl_net.BoosterCandidates().size()*chl_net.BoosterScenarios().size() + chl_net.DetectedScenarios().size())/booster_seconds;
   std::cout << "\n\t- Finished Booster Station Simulations (time in seconds): " << booster_seconds << std::endl;
   std::cout << "\n\t- Approximate Simulations Per Second: " << sims_per_second << std::endl;

   chl_net.StopLogging();
   tox_net.StopLogging();

   // Free all other memory being used
   chl_net.clear();
   tox_net.clear();

   return 0;
}


//forward declarations
void PerformBoosterSimulations(const BoosterNetworkSimulator& chl_net,
                               const NetworkSimulator& tox_net,
			       const BoosterImpactOptions& options,
                               bool pd_impacts/*=false*/)
{
   // Create a timer but don't start it
   TaskTimer timer(false);

   const BoosterNetworkSimulator& net = chl_net;
   const MerlionModelContainer& model = chl_net.Model();
   const MerlionModelContainer& tox_model = tox_net.Model();

   const float zero_conc_gpm3 = std::max(options.zero_conc_tol_gpm3,ZERO_CONC_GPM3);

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

   std::set<int> positive_demand_u_ids;
   for (int u_idx = 0; u_idx < model.N; ++u_idx) {
      if (model.demand_ut_m3[u_idx] > 0.0f) {
         positive_demand_u_ids.insert(u_idx);
      }
   }

   std::vector<double> volume_ingested_ut_m3;
   std::vector<double> node_total_dose_g;
   std::vector<double> node_total_pre_dose_g;
   if (pd_impacts) {
      volume_ingested_ut_m3.clear();
      volume_ingested_ut_m3.resize(model.N,0.0);
      node_total_dose_g.clear();
      node_total_dose_g.resize(model.n_nodes, 0.0);
      node_total_pre_dose_g.clear();
      node_total_pre_dose_g.resize(model.n_nodes, 0.0);
      for (int nt_idx = 0; nt_idx < model.N; ++nt_idx) {
         if (tox_net.VolumeIngested_m3()[nt_idx] > 0.0) {
            volume_ingested_ut_m3[model.perm_nt_to_upper[nt_idx]] = tox_net.VolumeIngested_m3()[nt_idx];
         }
      }
   }
   
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

   timer.Start();
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, net.BoosterScenarios()) {
      timer.Start();
      const BoosterScenario& booster_scen = **p_boost;
      
      const int sim_stop_timestep = booster_scen.sim_stop_timestep;
      const int min_row = model.N-model.n_nodes*(sim_stop_timestep+1);

      // the set of node,time ids in upper trianglular orientation where 
      // chlorine is present (0 indicates no chlorine)
      std::vector<int> disinf_u_ids(model.N,0);
      
      if (nrhs > 1 && !net.BoosterCandidates().empty()) {
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
            usolvem( model.N, 
		     min_row,
		     max_row_chl,
		     nrhs,
		     model.G->Values(),
		     model.G->iRows(),
		     model.G->pCols(),
		     chl_mass_inj_gpmin );
	    
            // Create a reference variable to make units more obvious. The linear solver
            // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
            // use this reference when it make sense for units (e.g below the linear solve).
            float*& chl_conc_gpm3 = chl_mass_inj_gpmin;
	    
            // analyze the solution
	    // keep track of the nodes/times where chlorine is present
            for (int i = min_row*nrhs, i_stop = (max_row_chl+1)*nrhs - 1; i <= i_stop; ++i) {
	       if (chl_conc_gpm3[i] > zero_conc_gpm3) {
		  disinf_u_ids[i/nrhs] += 1;
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
      else if (!net.BoosterCandidates().empty()) {
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
            usolve ( model.N,
		     min_row, 
		     max_row_chl, 
		     model.G->Values(), 
		     model.G->iRows(), 
		     model.G->pCols(), 
		     chl_mass_inj_gpmin );
         
            // Create a reference variable to make units make more obvious. The linear solver
            // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
            // use this reference when it make sense for units (e.g below the linear solve).
            float*& chl_conc_gpm3 = chl_mass_inj_gpmin;
         
            // analyze the solution
            for (int u_idx = min_row; u_idx <= max_row_chl; ++u_idx) {
               if (chl_conc_gpm3[u_idx] > zero_conc_gpm3) {
                  disinf_u_ids[u_idx] += 1;
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
      
      timer.Stop();
      if (options.report_scenario_timing) {
         std::cout << "\tBOOSTER SCENARIO ID:    " << (*p_boost)->Name() << std::endl;
         std::cout << "\t- Booster Simulations:  " << timer.LastLap() << std::endl;
      }
      timer.Start();
   
      // this is the maximum row after which chlorine injections have not begun
      const int chl_start_timestep = booster_scen.Injections().front()->StartTimestep(model);
      const int max_row_chl = (model.N-1)-model.n_nodes*chl_start_timestep;
      const InjScenList& toxin_scen_list = booster_scen.ToxinScenarios();
      for STL_CONST_ITERATE(InjScenList, ptox_scen_pos, ptox_scen_pos_stop, toxin_scen_list) {
         const PInjScenario ptox_scen = *ptox_scen_pos;
         // set the rhs for the toxin injection
         float scenario_mass_injected_g(0.0);
         int max_row_tox(model.N-1);
         ptox_scen->SetArray(model, tox_mass_inj_gpmin, scenario_mass_injected_g, max_row_tox);
         ptox_scen->AddImpact(scenario_mass_injected_g, "Response-Window Mass Injected Grams");

         // Solver the linear system
         usolve ( model.N,
		  min_row,
		  max_row_tox,
		  tox_model.G->Values(),
		  tox_model.G->iRows(),
		  tox_model.G->pCols(),
		  tox_mass_inj_gpmin );
      
         // Create a reference variable to make units make more obvious. The linear solver
         // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
         // use this reference when it make sense for units (e.g below the linear solve).
         float*& tox_conc_gpm3 = tox_mass_inj_gpmin;

         const int u_idx_start = model.N-model.n_nodes*(sim_stop_timestep+1);
         std::set<int>::const_iterator p_dem_u_idx = positive_demand_u_ids.begin();
         while(*p_dem_u_idx < u_idx_start) {++p_dem_u_idx;}
         std::set<int>::const_iterator p_dem_u_idx_stop = --(positive_demand_u_ids.end());
         while(*p_dem_u_idx_stop > max_row_tox) {--p_dem_u_idx_stop;}
         ++p_dem_u_idx_stop;
         // determine full scenario impact
         float scenario_mass_consumed_g(0.0);
         // Check the mass balance
         float mass_check(0.0);
         if (!pd_impacts) {
            for (; p_dem_u_idx != p_dem_u_idx_stop; ++p_dem_u_idx) {
               int u_idx = *p_dem_u_idx;
               if (tox_conc_gpm3[u_idx] > zero_conc_gpm3) {
                  mass_check += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
                  if (!disinf_u_ids[u_idx]) {
                     // mc
                     scenario_mass_consumed_g += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
                  }
               }
            }
         }
         else { // Includes the code above, just minimizes checking if pd_impacts
            for (; p_dem_u_idx != p_dem_u_idx_stop; ++p_dem_u_idx) {
               int u_idx = *p_dem_u_idx;
               if (tox_conc_gpm3[u_idx] > zero_conc_gpm3) {
                  mass_check += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
                  if (!disinf_u_ids[u_idx]) {
                     // mc
                     scenario_mass_consumed_g += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
                     // pd
                     int n = model.perm_upper_to_nt[u_idx] / model.n_steps;
                     node_total_dose_g[n] += tox_conc_gpm3[u_idx] * volume_ingested_ut_m3[u_idx];
                  }
               }
            }
         }

         // determine impact before booster stations activate
         p_dem_u_idx = p_dem_u_idx_stop;
         --p_dem_u_idx;
         while(*p_dem_u_idx >= (max_row_chl+1)) {--p_dem_u_idx;}
         ++p_dem_u_idx;
         float scenario_pre_booster_mass_consumed_g(0.0);
         if (!pd_impacts) {
            for (; p_dem_u_idx != p_dem_u_idx_stop; ++p_dem_u_idx) {
               int u_idx = *p_dem_u_idx;
               if (tox_conc_gpm3[u_idx] > zero_conc_gpm3) {
                  scenario_pre_booster_mass_consumed_g += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
               }
            }
         }
         else { // Includes the code above, just minimizes checking if pd_impacts
            for (; p_dem_u_idx != p_dem_u_idx_stop; ++p_dem_u_idx) {
               int u_idx = *p_dem_u_idx;
               if (tox_conc_gpm3[u_idx] > zero_conc_gpm3) {
                  // pre mc
                  scenario_pre_booster_mass_consumed_g += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
                  // pre pd
                  int n = model.perm_upper_to_nt[u_idx] / model.n_steps;
                  node_total_pre_dose_g[n] += tox_conc_gpm3[u_idx] * volume_ingested_ut_m3[u_idx];
               }
            }
         }

         // Include mass remaining in tanks in mass balance check
         float tank_mass(0.0);
         for (std::vector<std::pair<int,int> >::const_iterator pos = model.tanks.begin(), stop = model.tanks.end(); pos != stop; ++pos) {
            int n = pos->first;
            int tank_n = pos->second;
            int t = sim_stop_timestep;
            int nt_tank_idx = tank_n*model.n_steps + t;
            int nt_idx = n*model.n_steps + t;
            int u_idx = model.perm_nt_to_upper[nt_idx];
            if (((tox_conc_gpm3[u_idx] > zero_conc_gpm3) && (!disinf_u_ids[u_idx]))) {
               tank_mass += tox_conc_gpm3[u_idx]*model.tank_volume_m3[nt_tank_idx];
            }
            mass_check += tox_conc_gpm3[u_idx]*model.tank_volume_m3[nt_tank_idx];
         }
         ptox_scen->AddImpact(scenario_mass_consumed_g, "Mass Consumed Grams");
         ptox_scen->AddImpact(scenario_pre_booster_mass_consumed_g, "Pre-Booster Mass Consumed Grams");
         ptox_scen->AddImpact(tank_mass, "Final Mass In Tanks Grams");
         ptox_scen->AddImpact(mass_check/scenario_mass_injected_g*100.0f, "Simulation Mass Balance");

         if (pd_impacts) {
            double population_dosed = 0.0;
            for (int n = 0; n < model.n_nodes; ++n) {
               if (node_total_dose_g[n] > options.population_dosed_threshold_g) {
                  population_dosed += tox_net.NodePopulation()[n];
               }
            }
            ptox_scen->AddImpact(population_dosed, "Population Dosed");
            double pre_population_dosed = 0.0;
            for (int n = 0; n < model.n_nodes; ++n) {
               if (node_total_pre_dose_g[n] > options.population_dosed_threshold_g) {
                  pre_population_dosed += tox_net.NodePopulation()[n];
               }
            }
            ptox_scen->AddImpact(pre_population_dosed, "Pre-Population Dosed");
            // reset the node dose vectors
            BlasResetZero(model.n_nodes, &(node_total_dose_g[0]));
            BlasResetZero(model.n_nodes, &(node_total_pre_dose_g[0]));
         }

	 // reset the sparse set of locations where the injections modifed the array,
	 // these may have been outside the region of interest in the linear solver,
	 // in which case, the BlasResetZero code after this loop would not have
	 // zero that region of the array
	 ptox_scen->ClearArray(model, tox_mass_inj_gpmin);
         // reset the rhs for the toxin backsolves
         const int num_elements = max_row_tox-min_row+1;
         const int p_offset = min_row;
         BlasResetZero(num_elements, tox_conc_gpm3+p_offset);
      }
      timer.Stop();
      if (options.report_scenario_timing) {
         std::cout << "\t- Toxin Simulations:  " << timer.LastLap() << std::endl;
         std::cout << std::endl;
      }
   }

   timer.Start();
   for STL_CONST_ITERATE(InjScenList, ptox_scen_pos, ptox_scen_pos_stop, chl_net.NonDetectedScenarios()) {
      const PInjScenario ptox_scen = *ptox_scen_pos;

      // We simulate until the end of the time horizon
      const int sim_stop_timestep = model.n_steps-1;
      const int min_row = 0;

      // Set the mass injected label for the non-detected scenarios
      ptox_scen->AddImpact(ptox_scen->MassInjected(chl_net.Model()),"Mass Injected Grams");
         
      // set the rhs for the toxin injection
      float scenario_mass_injected_g(0.0);
      int max_row_tox(model.N-1);
      ptox_scen->SetArray(model, tox_mass_inj_gpmin, scenario_mass_injected_g, max_row_tox);

      // Solver the linear system
      usolve (model.N,
              min_row,
              max_row_tox,
              tox_model.G->Values(),
              tox_model.G->iRows(),
              tox_model.G->pCols(),
              tox_mass_inj_gpmin );
      
      // Create a reference variable to make units make more obvious. The linear solver
      // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
      // use this reference when it make sense for units (e.g below the linear solve).
      float*& tox_conc_gpm3 = tox_mass_inj_gpmin;

      std::set<int>::const_iterator p_dem_u_idx = positive_demand_u_ids.begin();
      while(*p_dem_u_idx < min_row) {++p_dem_u_idx;}
      std::set<int>::const_iterator p_dem_u_idx_stop = --(positive_demand_u_ids.end());
      while(*p_dem_u_idx_stop > max_row_tox) {--p_dem_u_idx_stop;}
      ++p_dem_u_idx_stop;
      // determine full scenario impact
      float scenario_mass_consumed_g(0.0);
      // Check the mass balance
      float mass_check(0.0);
      for (; p_dem_u_idx != p_dem_u_idx_stop; ++p_dem_u_idx) {
         int u_idx = *p_dem_u_idx;
         if (tox_conc_gpm3[u_idx] > zero_conc_gpm3) {
            mass_check += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
            // mc
            scenario_mass_consumed_g += tox_conc_gpm3[u_idx]*model.demand_ut_m3[u_idx];
            // pd
            if (pd_impacts) {
               int n = model.perm_upper_to_nt[u_idx] / model.n_steps;
               node_total_dose_g[n] += tox_conc_gpm3[u_idx] * volume_ingested_ut_m3[u_idx];
            }
         }
      }

      // Include mass remaining in tanks in mass balance check
      float tank_mass(0.0);
      for (std::vector<std::pair<int,int> >::const_iterator pos = model.tanks.begin(), stop = model.tanks.end(); pos != stop; ++pos) {
         int n = pos->first;
         int tank_n = pos->second;
         int t = sim_stop_timestep;
         int nt_tank_idx = tank_n*model.n_steps + t;
         int nt_idx = n*model.n_steps + t;
         int u_idx = model.perm_nt_to_upper[nt_idx];
         if (tox_conc_gpm3[u_idx] > zero_conc_gpm3) {
            tank_mass += tox_conc_gpm3[u_idx]*model.tank_volume_m3[nt_tank_idx];
         }
         mass_check += tox_conc_gpm3[u_idx]*model.tank_volume_m3[nt_tank_idx];
      }
      ptox_scen->AddImpact(scenario_mass_consumed_g, "Mass Consumed Grams");
      ptox_scen->AddImpact(tank_mass, "Final Mass In Tanks Grams");
      ptox_scen->AddImpact(mass_check/scenario_mass_injected_g*100.0f, "Simulation Mass Balance");

      if (pd_impacts) {
         double population_dosed = 0.0;
         for (int n = 0; n < model.n_nodes; ++n) {
            if (node_total_dose_g[n] > options.population_dosed_threshold_g) {
               population_dosed += tox_net.NodePopulation()[n];
            }
         }
         ptox_scen->AddImpact(population_dosed, "Population Dosed");
      }

      // reset the node dose vector
      BlasResetZero(model.n_nodes, &(node_total_dose_g[0]));

      // reset the sparse set of locations where the injections modifed the array,
      // these may have been outside the region of interest in the linear solver,
      // in which case, the BlasResetZero code after this loop would not have
      // zero that region of the array
      ptox_scen->ClearArray(model, tox_mass_inj_gpmin);
      // reset the rhs for the toxin backsolves
      const int num_elements = max_row_tox-min_row+1;
      const int p_offset = min_row;
      BlasResetZero(num_elements, tox_conc_gpm3+p_offset);
   }
   timer.Stop();
   if (options.report_scenario_timing) {
      std::cout << "\t- Non-Detected Toxin Simulations:  " << timer.LastLap() << std::endl;
      std::cout << std::endl;
   }

   //print scenario impact
   std::ofstream out;
   out.precision(12);
   out.setf(std::ios::scientific,std::ios::floatfield);
   if (options.yaml) {
      std::string fname = options.output_prefix+"DetectedScenarioImpacts.yml";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      merlionUtils::PrintScenarioListYAML(net.DetectedScenarios(), out);
      out.close();
      fname = options.output_prefix+"NonDetectedScenarioImpacts.yml";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      merlionUtils::PrintScenarioListYAML(net.NonDetectedScenarios(), out);
      out.close();
      fname = options.output_prefix+"DiscardedScenarioImpacts.yml";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      merlionUtils::PrintScenarioListYAML(net.DiscardedScenarios(), out);
      out.close();
   }
   if (options.json) {
      std::string fname = options.output_prefix+"DetectedScenarioImpacts.json";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      merlionUtils::PrintScenarioListJSON(net.DetectedScenarios(), out);
      out.close();
      fname = options.output_prefix+"NonDetectedScenarioImpacts.json";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      merlionUtils::PrintScenarioListJSON(net.NonDetectedScenarios(), out);
      out.close();
      fname = options.output_prefix+"DiscardedScenarioImpacts.json";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      merlionUtils::PrintScenarioListJSON(net.DiscardedScenarios(), out);
      out.close();
   }
   if (!options.yaml && !options.json) {

      std::string fname = options.output_prefix+"ScenarioImpacts.txt";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      out << "Detected " << net.DetectedScenarios().size() << "\n";
      out << "label   weight   normalized-weight   orig-mass-injected   window-mass-injected   mass-in-tanks   percent-balance\n";
      for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.DetectedScenarios()) {
         out << (*p_scen)->Name() << " "
             << (*p_scen)->Impact("Weight") << " "
             << (*p_scen)->Impact("Normalized Weight") << " "
             << (*p_scen)->Impact("Original Mass Injected Grams") << " "
             << (*p_scen)->Impact("Response-Window Mass Injected Grams") << " "
             << (*p_scen)->Impact("Final Mass In Tanks Grams") << " "
             << (*p_scen)->Impact("Simulation Mass Balance") << "\n";
      }
      out << "\n";
      out << "UnDetected " << net.NonDetectedScenarios().size() << std::endl;
      out << "label   weight   normalized-weight   mass-injected   mass-in-tanks   percent-balance\n";
      for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.NonDetectedScenarios()) {
         out << (*p_scen)->Name() << " "
             << (*p_scen)->Impact("Weight") << " "
             << (*p_scen)->Impact("Normalized Weight") << " "
             << (*p_scen)->Impact("Mass Injected Grams") << " "
             << (*p_scen)->Impact("Final Mass In Tanks Grams") << " "
             << (*p_scen)->Impact("Simulation Mass Balance") << "\n";
      }
      out << "\n";
      out << "Discarded " << net.DiscardedScenarios().size() << std::endl;
      out << "label   weight   normalized-weight   percent-balance\n";
      for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.DiscardedScenarios()) {
         out << (*p_scen)->Name() << " "
             << (*p_scen)->Impact("Weight") << " "
             << (*p_scen)->Impact("Normalized Weight") << " "
             << 0.0 << "\n";
      }
      out.close();


      fname = options.output_prefix+"ScenarioImpacts_MC.txt";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      out << "Detected " << net.DetectedScenarios().size() << std::endl;
      out << "label   mass-consumed  pre-booster-mass-consumed\n";
      for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.DetectedScenarios()) {
         out << (*p_scen)->Name() << " "
             << (*p_scen)->Impact("Mass Consumed Grams") << " "
             << (*p_scen)->Impact("Pre-Booster Mass Consumed Grams") << "\n";
      }
      out << "\n";
      out << "UnDetected " << net.NonDetectedScenarios().size() << std::endl;
      out << "label   mass-consumed\n";
      for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.NonDetectedScenarios()) {
         out << (*p_scen)->Name() << " "
             << (*p_scen)->Impact("Mass Consumed Grams") << "\n";
      }
      out << "\n";
      out << "Discarded " << net.DiscardedScenarios().size() << std::endl;
      out << "label\n";
      for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.DiscardedScenarios()) {
         out << (*p_scen)->Name() << "\n";
      }
      out.close();

      if (pd_impacts) {
         fname = options.output_prefix+"ScenarioImpacts_PD.txt";
         out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
         out << "Detected " << net.DetectedScenarios().size() << std::endl;
         out << "label   population-dosed  pre-booster-population-dosed\n";
         for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.DetectedScenarios()) {
            out << (*p_scen)->Name() << " "
                << (*p_scen)->Impact("Population Dosed") << " "
                << (*p_scen)->Impact("Pre-Population Dosed") << "\n";
         }
         out << "\n";
         out << "UnDetected " << net.NonDetectedScenarios().size() << std::endl;
         out << "label   population-dosed\n";
         for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.NonDetectedScenarios()) {
            out << (*p_scen)->Name() << " "
                << (*p_scen)->Impact("Population Dosed") << "\n";
         }
         out << "\n";
         out << "Discarded " << net.DiscardedScenarios().size() << std::endl;
         out << "label\n";
         for STL_CONST_ITERATE(InjScenList, p_scen, p_stop, net.DiscardedScenarios()) {
            out << (*p_scen)->Name() << "\n";
         }
      }
   }
}
