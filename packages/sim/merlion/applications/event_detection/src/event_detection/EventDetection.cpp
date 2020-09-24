#include <event_detection/EventDetectionOptions.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/MerlionDefines.hpp>
#include <merlion/SparseMatrix.hpp>

#include <merlionUtils/TaskTimer.hpp>
#include <merlionUtils/NetworkSimulator.hpp>

#include <set>
#include <exception>
#include <algorithm>
#include <boost/random/uniform_real.hpp>
//#include <boost/random/gamma_distribution.hpp>
#include <boost/random/linear_congruential.hpp>
#include <ctime>

int main(int argc, char** argv)
{  
   // Handle the optional inputs to this executable
   EventDetectionOptions options;
   options.ParseInputs(argc, argv);   
   
   // Helpful interface for setting up the simulations
   merlionUtils::NetworkSimulator net(options.disable_warnings);
   const merlionUtils::MerlionModelContainer& model = net.Model();

   if (options.logging) {
      net.StartLogging(options.output_prefix+"eventDetection.log");
      options.PrintSummary(net.Log());
   }

   std::cout << "\n@@@ PARSING INPUT FILES @@@" << std::endl;
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
                      -1,
                      -1,
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


   if (options.tsg_filename != "") {
      merlionUtils::TaskTimer Read_tsg;
      // Read in the injection scenarios define by the *.tsg file
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
   


   bool detection_times_provided = false;
   // Read in the sensor node list or inverse
   if (options.wqm_inverse_filename != "") {
      merlionUtils::TaskTimer Read_Inverse;
      net.ReadInverse(options.wqm_inverse_filename);
      Read_Inverse.StopAndPrint(std::cout, "\n\t- Read Merlion Inverse File ");
   }
   else if (options.sensor_filename != "") {
      merlionUtils::TaskTimer Read_Sensors;
      // Read the file defining the set of sensors
      net.ReadSensorFile(options.sensor_filename);
      Read_Sensors.StopAndPrint(std::cout,"\n\t- Read sensor file ");
   }
   else if (options.uniform_random_detection_times == true) {
      detection_times_provided = true;
      if ((options.detection_window_upper_min < 0.0) || (options.detection_window_lower_min < 0.0)) {
         std::cerr << std::endl;
         std::cerr << "ERROR: Nonnegative detection window limits are required" << std::endl;
         std::cerr << std::endl;
         return 1;
      }

      // Round the detection interval to the nearest multiple of the water
      // quality timestep
      int detection_interval_timesteps = 
         merlionUtils::SecondsToNearestTimestepBoundary(options.detection_interval_min*60.0,
                                                        net.Model().qual_step_minutes);
      if (detection_interval_timesteps == 0) {
         detection_interval_timesteps = 1;
      }

      int seed = (options.isDefault("random-seed"))?(time(NULL)):(options.random_seed);
      boost::minstd_rand generator(seed);
      boost::uniform_real<double> distribution(options.detection_window_lower_min*60.0,options.detection_window_upper_min*60.0);

      for (merlionUtils::InjScenList::const_iterator p_scen = net.InjectionScenarios().begin(), p_stop = net.InjectionScenarios().end(); p_scen != p_stop; ++p_scen) {
         double scen_start = (*p_scen)->EarliestInjectionTimeSeconds();
         double detection_time_seconds = scen_start+distribution(generator);
         int detection_timestep = 
            merlionUtils::SecondsToNearestTimestepBoundary(detection_time_seconds,
                                                           net.Model().qual_step_minutes);
         if (detection_timestep % detection_interval_timesteps) {
            // Round to closest interval of the requested detection time interval
            detection_timestep = \
               merlionUtils::RoundToInt(detection_timestep/((float)detection_interval_timesteps))*detection_interval_timesteps;
         }
         detection_time_seconds = detection_timestep*net.Model().qual_step_minutes*60.0;
         if (detection_timestep >= model.n_steps) {
            std::cerr << std::endl;
            std::cerr << "ERROR: A random detection was generated outside of the simulation period. To prevent this " 
                      << "from happening increase the simulation period or reduce the random detection window." <<std::endl;
            std::cerr << std::endl;
            return 1;
         }
         (*p_scen)->SetDetectionTimeSeconds(detection_time_seconds);
      }
   }
   /*   else if (options.gamma_random_detection_times == true) {
      detection_times_provided = true;
      if (options.detection_delay_mean < 0.0) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A nonnegative detection delay mean is required" << std::endl;
         std::cerr << std::endl;
         return 1;
      }

      // Round the detection interval to the nearest multiple of the water
      // quality timestep
      int detection_interval_timesteps = 1;
      if (options.detection_interval_min > net.Model().qual_step_minutes) {
         detection_interval_timesteps = merlionUtils::RoundToInt(options.detection_interval_min/net.Model().qual_step_minutes);
      }

      double mean = (double)options.detection_delay_mean*60.0;
      double alpha = sqrt(mean);
      double beta = alpha;

      int seed = (options.isDefault("random-seed"))?(time(NULL)):(options.random_seed);
      boost::minstd_rand generator(seed);
      boost::gamma_distribution<double> distribution(alpha, beta);
  
      for (merlionUtils::InjScenList::const_iterator p_scen = net.InjectionScenarios().begin(), p_stop = net.InjectionScenarios().end(); p_scen != p_stop; ++p_scen) {
         double scen_start = (*p_scen)->EarliestInjectionTimeSeconds();
         double detection_time_seconds = scen_start+distribution(generator);
         int detection_timestep = merlionUtils::RoundToInt(detection_time_seconds/60.0/net.Model().qual_step_minutes);
         if (detection_timestep % detection_interval_timesteps) {
            detection_timestep = merlionUtils::RoundToInt(detection_timestep/((float)detection_interval_timesteps))*detection_interval_timesteps;
         }
         detection_time_seconds = detection_timestep*net.Model().qual_step_minutes*60.0;
         if (detection_timestep >= model.n_steps) {
            std::cerr << std::endl;
            std::cerr << "ERROR: A random detection was generated outside of the simulation period. To prevent this " 
                      << "from happening increase the simulation period or reduce the random detection delay mean." <<std::endl;
            std::cerr << std::endl;
            return 1;
         }
         (*p_scen)->SetDetectionTimeSeconds(detection_time_seconds);
      }
   
      }*/

   else if (options.multiple_detection_times) { // Generate multiple copies of each scenario with a range of detection times 
      detection_times_provided = true;
      if ((options.detection_window_upper_min < 0.0) || (options.detection_window_lower_min < 0.0)) {
         std::cerr << std::endl;
         std::cerr << "ERROR: Nonnegative detection window limits are required" << std::endl;
         std::cerr << std::endl;
         return 1;
      }

      // Round the detection interval to the nearest multiple of the water
      // quality timestep
      int detection_interval_timesteps = 
         merlionUtils::SecondsToNearestTimestepBoundary(options.detection_interval_min*60.0,
                                                        net.Model().qual_step_minutes);
      if (detection_interval_timesteps == 0) {
         detection_interval_timesteps = 1;
      }

      // Copy Injection Scenario List 
      merlionUtils::InjScenList Inj_scen_copy = net.InjectionScenarios();
      
      // Add multiple injections  
      for (merlionUtils::InjScenList::const_iterator p_scen = Inj_scen_copy.begin(), 
              p_stop = Inj_scen_copy.end(); p_scen != p_stop; ++p_scen) {
         // Set start time as detection time for original scenario
         double scen_start = (*p_scen)->EarliestInjectionTimeSeconds();
         int detection_timestep = 
            merlionUtils::SecondsToNearestTimestepBoundary(scen_start,
                                                           net.Model().qual_step_minutes);
         int start_timestep = detection_timestep + options.detection_window_lower_min/net.Model().qual_step_minutes;
         if (detection_timestep % detection_interval_timesteps) {
            // Round to closest interval of the requested detection time interval
            detection_timestep =
               merlionUtils::RoundToInt(detection_timestep/((float)detection_interval_timesteps))*detection_interval_timesteps;
         }
         double detection_time_seconds = detection_timestep*net.Model().qual_step_minutes*60.0;
         if (detection_timestep >= model.n_steps) {
            std::cerr << std::endl;
            std::cerr << "ERROR: A detection was generated outside of the simulation period. To prevent this " 
                      << "from happening increase the simulation period or reduce the random detection window." <<std::endl;
            std::cerr << std::endl;
            return 1;
         }
         (*p_scen)->SetDetectionTimeSeconds(detection_time_seconds);

         int stop_timestep = 
            merlionUtils::SecondsToNearestTimestepBoundary(scen_start+options.detection_window_upper_min*60.0,
                                                           net.Model().qual_step_minutes);
         // Add new scenarios with a range of detection times
         for (int t = start_timestep+detection_interval_timesteps; t < stop_timestep; t += detection_interval_timesteps) {

            if (t >= model.n_steps) {
               std::cerr << std::endl;
               std::cerr << "ERROR: A detection was generated outside of the simulation period. To prevent this " 
                         << "from happening increase the simulation period or reduce the random detection window." <<std::endl;
               std::cerr << std::endl;
               return 1;
            }
            detection_time_seconds = t*net.Model().qual_step_minutes*60.0;
            
            std::cout<< detection_time_seconds << std::endl;
            
            merlionUtils::PInjScenario new_scen(new merlionUtils::InjScenario);
            new_scen->DeepCopy(**p_scen);
            new_scen->SetDetectionTimeSeconds(detection_time_seconds);
            net.InjectionScenarios().push_back(new_scen);
         }
      }
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing sensor or inverse file which is required when using a TSG or SCN file." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   if (!((options.sensor_filename != "") ^ (options.wqm_inverse_filename != "") ^ (options.uniform_random_detection_times == true) ^ (options.multiple_detection_times == true))) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Too many required detection options specified." << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   std::cout << "\n@@@ DETERMINING SCENARIO DETECT TIMES @@@" << std::endl;
   if (options.wqm_inverse_filename != "") {
      merlionUtils::TaskTimer Read_Inverse;
      net.ReadInverse(options.wqm_inverse_filename);
      Read_Inverse.StopAndPrint(std::cout, "\n\t- Read Merlion Inverse File ");
   }
   else if (options.sensor_filename != "") {
      merlionUtils::TaskTimer Build_Inverse;
      
      // Build reduced inverse so sensor node detections can be rapidly
      // determined for each injection
      int min_inj_timestep = model.n_steps;
      for (merlionUtils::InjScenList::const_iterator p_scen = net.InjectionScenarios().begin(), p_stop = net.InjectionScenarios().end(); p_scen != p_stop; ++p_scen) {
         int scen_start = (*p_scen)->EarliestInjectionTimestep(model);
         if (scen_start < min_inj_timestep) {
            min_inj_timestep = scen_start;
         }
      }
      if (options.save_wqm_inverse) {min_inj_timestep = 0;}
      
      // Build a list of node/time tuple where rows of the inverse are needed.
      if (net.SensorNodeIDS().empty() && net.GrabSampleIDS().empty()) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A non-empty list of sensor nodes or grab samples is required when "
	           << "using eventDetection" << std::endl;
         std::cerr << std::endl;
         return 1;
      }

      bool map_only(true);
      if (options.detection_tol_gpm3 > 0) {map_only = false;}
      if (options.save_wqm_inverse) {map_only = false;}

      net.GenerateReducedInverse(min_inj_timestep,model.n_steps-1,options.sensor_interval_min,1,map_only);
      
      if (options.save_wqm_inverse) {
	 std::ofstream out_inv;
	 out_inv.open(std::string(options.output_prefix+"MerlionInverse.bin").c_str(), std::ios::out | std::ios::trunc | std::ios::binary); 
	 net.SaveInverse(out_inv);
	 out_inv.close();
      }

      Build_Inverse.StopAndPrint(std::cout, "\n\t- Inverse Built ");
   }

   merlionUtils::TaskTimer ApplySensor;
   // Apply the sensor layout and generate the new scenario data
   // based on detection times
   net.ClassifyScenarios(detection_times_provided,
                         model.n_steps-1,
                         options.detection_tol_gpm3,
                         options.detection_interval_min);
   ApplySensor.StopAndPrint(std::cout, "\n\t- Determined Detection Times");

   // Free the memory for the reduced system inverse
   net.ResetInverse();
   // Free the memory for the list of sensors
   net.ResetSensorData();

   std::cout << "Processed " << net.InjectionScenarios().size() << " injection events." << std::endl;
   std::ofstream out;
   out.precision(12);
   if (options.yaml) {
      std::string fname = options.output_prefix+"EventAnalysis.yml";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      merlionUtils::PrintScenarioListYAML(net.InjectionScenarios(), out);
      out.close();
   }
   if (options.json) {
      std::string fname = options.output_prefix+"EventAnalysis.json";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      merlionUtils::PrintScenarioListJSON(net.InjectionScenarios(), out);
      out.close();
   }
   if (options.detected_scn) {
      std::string fname = options.output_prefix+"EventAnalysis.dscn";
      out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      net.WriteDetectedSCNFile(out);
      out.close();
   }
   //if (!options.yaml && !options.json && !options.detected_scn) {
   std::string fname = options.output_prefix+"EventAnalysis.txt";
   out.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   for (merlionUtils::InjScenList::const_iterator p_scen = net.InjectionScenarios().begin(), p_stop = net.InjectionScenarios().end(); p_scen != p_stop; ++p_scen) {
      if ((*p_scen)->isDetected()) {
         out << (*p_scen)->DetectionTimeSeconds() << "\n";
      }
      else {
         out << "None\n";
      }
   }
   out.close();
   //}

   net.StopLogging();
   net.clear();

   return 0;
}
