#include <booster/BoosterNetworkSimulator.hpp>

#include <merlionUtils/TaskTimer.hpp>
#include <merlionUtils/TSG_Reader.hpp>

#include <merlion/SparseMatrix.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/MerlionDefines.hpp>

#include <algorithm>
#include <cmath>
#include <set>

using namespace merlionUtils;

void BoosterNetworkSimulator::clear()
{
   ResetBoosterScenarios();
   /* Base Class clear() */
   NetworkSimulator::clear();
}

void BoosterNetworkSimulator::ResetBoosterScenarios()
{ 
   booster_delay_seconds_ = 0;
   booster_inj_seconds_ = 0;
   sim_after_detect_timesteps_ = 0;
   booster_inj_type_ = InjType_UnDef;
   booster_inj_strength_ = 0.0f;
   booster_candidates_.clear();

   booster_scenarios_.clear();
}

void BoosterNetworkSimulator::ClassifyBoosterScenarios(bool detection_times_provided,
                                                       bool scenario_aggregation/*=true*/,
                                                       float detection_tol_gpm3/*=0.0f*/,
                                                       float detection_interval_minutes/*0.0f*/)
{
   int latest_allowed_detection_timestep = model_.n_steps-1-sim_after_detect_timesteps_;
   ClassifyScenarios(detection_times_provided,
                     latest_allowed_detection_timestep,
                     detection_tol_gpm3,
                     detection_interval_minutes);

   // Make sure each scenario has an assigned "Weight" and they all sum to 1.0
   double total_weight = 0.0;
   double default_weight = (injection_scenarios_.size())?(1.0/((double)injection_scenarios_.size())):(1.0);
   bool found_with = false;
   bool found_without = false;
   for STL_ITERATE(InjScenList, p_scen, scen_end, injection_scenarios_) {
      if ((*p_scen)->HasImpact("Weight")) {
         if ((*p_scen)->Impact("Weight") < 0) {
            std::cerr << std::endl;
            std::cerr << "ERROR: Scenario weights must be nonnegative" << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         found_with = true;
      }
      else {
         (*p_scen)->AddImpact(default_weight,"Weight");
         found_without = true;
      }
      total_weight += (*p_scen)->Impact("Weight");
   }
   if (found_without) {
      if (found_with) {
         std::cerr << std::endl;
         std::cerr << "WARNING: Scenarios weights were not found on all scenario. A default weight "
                   << "of " << default_weight << " has been assigned to the remaining scenarios." << std::endl; 
         std::cerr << std::endl;
      }
      std::cout << "Assigning scenarios a default weight of " << default_weight << std::endl;
   }
   if (abs(total_weight-1.0) > 1e-4) {
         std::cerr << std::endl;
         std::cerr << "WARNING: Scenarios weights do not seem to add up to 1.0 (" << total_weight << ")" << std::endl;
         std::cerr << std::endl;      
   }

   // Create normalized weights for the list of detected scenarios so that
   // the objective function still looks like a weighted average if/when
   // all scenarios don't make it into the detected list
   double total_weight_detected = 0.0;
   for STL_ITERATE(InjScenList, p_scen, scen_end, detected_scenarios_) {
      total_weight_detected += (*p_scen)->Impact("Weight");
   }
   if (total_weight_detected == 0) {
      total_weight_detected = 1.0;
   }
   for STL_ITERATE(InjScenList, p_scen, scen_end, detected_scenarios_) {
      double orig_weight = (*p_scen)->Impact("Weight");
      (*p_scen)->AddImpact(orig_weight/total_weight_detected,"Normalized Weight");
   }

   // Create normalized weights for the list of undetected scenarios
   double total_weight_undetected = 0.0;
   for STL_ITERATE(InjScenList, p_scen, scen_end, non_detected_scenarios_) {
      total_weight_undetected += (*p_scen)->Impact("Weight");
   }
   if (total_weight_undetected == 0) {
      total_weight_undetected = 1.0;
   }
   for STL_ITERATE(InjScenList, p_scen, scen_end, non_detected_scenarios_) {
      double orig_weight = (*p_scen)->Impact("Weight");
      (*p_scen)->AddImpact(orig_weight/total_weight_undetected,"Normalized Weight");
   }

   // Create normalized weights for the list of discarded scenarios
   double total_weight_discarded = 0.0;
   for STL_ITERATE(InjScenList, p_scen, scen_end, discarded_scenarios_) {
      total_weight_discarded += (*p_scen)->Impact("Weight");
   }
   if (total_weight_discarded == 0) {
      total_weight_discarded = 1.0;
   }
   for STL_ITERATE(InjScenList, p_scen, scen_end, discarded_scenarios_) {
      double orig_weight = (*p_scen)->Impact("Weight");
      (*p_scen)->AddImpact(orig_weight/total_weight_discarded,"Normalized Weight");
   }
   
   if (!scenario_aggregation) {
      for STL_ITERATE(InjScenList, p_scen, scen_end, detected_scenarios_) {
         int detection_timestep = (*p_scen)->DetectionTimestep(model_);
         double detection_time_seconds = detection_timestep*60.0*model_.qual_step_minutes;
	 PBoosterScenario ptmp(new BoosterScenario);
	 ptmp->Name() = (*p_scen)->Name();
	 ptmp->SetDetectionTimeSeconds((*p_scen)->DetectionTimeSeconds());
	 // Create a single template Injection (since the injection profile
         // for each booster is identical). This template will need to be
         // assigned a booster node name before setting the rhs in the booster
         // simulations.
	 PInjection booster_inj(new Injection("",
                                              booster_inj_strength_,
                                              booster_inj_type_,
                                              GetBoosterStartTimeSeconds(detection_time_seconds),
                                              GetBoosterStopTimeSeconds(detection_time_seconds)));
	 ptmp->Injections().push_back(booster_inj);
	 ptmp->sim_stop_timestep = GetCalcStopTimestep(detection_timestep);
	 // **NOTE**: This is a list of pointers (PInjScenario) to objects (InjScenario), which also have lists of pointers
         //           (PInjection) to other objects (Injection), the objects themselves are not copied. Therefore when they are modifed
	 //           below, all other lists which contain these pointers will have their InjScenarios and Injections modified
	 //           as well.
         ptmp->ToxinScenarios().push_back(*p_scen);
         ptmp->AddImpact((*p_scen)->Impact("Weight"),"Weight");
         ptmp->AddImpact((*p_scen)->Impact("Normalized Weight"),"Normalized Weight");
         (*p_scen)->AddImpact((*p_scen)->MassInjected(model_),"Original Mass Injected Grams");
         booster_scenarios_.push_back(ptmp);
      }
   }
   else {
      int scenario_cntr = 0;
      typedef std::map<int, InjScenList> int_to_injscenlist;
      for STL_ITERATE(int_to_injscenlist, pos, pos_stop, timestep_to_detected_scenarios_) {
	 int detection_timestep = pos->first;
         double detection_time_seconds = detection_timestep*60.0*model_.qual_step_minutes;
	 InjScenList& scenario_list = pos->second;
         std::stringstream out;
         out << ++scenario_cntr;
         PBoosterScenario ptmp(new BoosterScenario);
         ptmp->Name() = out.str();
         ptmp->SetDetectionTimeSeconds(detection_time_seconds);
         // Create a single template Injection (since the injection profile
         // for each booster is identical). This template will need to be
         // assigned a booster node name before setting the rhs in the booster
         // simulations.
         PInjection booster_inj(new Injection("",
                                              booster_inj_strength_,
                                              booster_inj_type_,
                                              GetBoosterStartTimeSeconds(detection_time_seconds),
                                              GetBoosterStopTimeSeconds(detection_time_seconds)));
         ptmp->Injections().push_back(booster_inj);
	 ptmp->sim_stop_timestep = GetCalcStopTimestep(detection_timestep);
	 // **NOTE**: This is a list of pointers (PInjScenario) to objects (InjScenario), which also have lists of pointers
         //           (PInjection) to other objects (Injection), the objects themselves are not copied. Therefore when they are modifed
	 //           below, all other lists which contain these pointers will have their InjScenarios and Injections modified
	 //           as well.
	 ptmp->ToxinScenarios() = scenario_list; // copy the InjScenList of pointers
         ptmp->AddImpact(0.0,"Weight");
         ptmp->AddImpact(0.0,"Normalized Weight");
	 booster_scenarios_.push_back(ptmp);
      }
   }

   // For every BoosterScenario we need to modify each of the
   // Injection stop times to be no later than the 'no drink order'
   // time which is defined as the sim_stop_time for that BoosterScenario.
   for STL_ITERATE(BoosterScenList, p_boost, p_boost_stop, booster_scenarios_) {
      const double stop_time_seconds = (*p_boost)->sim_stop_timestep*60*model_.qual_step_minutes;
      InjScenList& scenarios = (*p_boost)->ToxinScenarios();
      for STL_ITERATE(InjScenList, p_scen, p_scen_stop, scenarios) {
	 (*p_scen)->AddImpact((*p_scen)->MassInjected(model_),"Original Mass Injected Grams");
	 InjectionList& injlist = (*p_scen)->Injections();
	 for STL_ITERATE(InjectionList, p_tox, p_tox_stop, injlist) {
	    if ((*p_tox)->StopTimeSeconds() > stop_time_seconds) {
               (*p_tox)->StopTimeSeconds() = stop_time_seconds;
            }
	 }
      }
   }
}

void BoosterNetworkSimulator::ReadBoosterFile(std::string fname)
{
   ResetBoosterScenarios();

   std::ifstream in;
   in.open(fname.c_str(), std::ios::in);

   int delay_minutes;
   int booster_inj_minutes;
   int sim_after_detect_minutes;

   std::string error_msg = "Invalid booster file format for file: ";
   error_msg += fname;
   std::string tag;
   in >> tag;
   if (tag != "DELAY_MINUTES:") {
      std::cerr << error_msg << std::endl;
      exit(1);
   }
   in >> delay_minutes;
   in >> tag;
   if (tag != "INJ_DURATION_MINUTES:") {
      std::cerr << error_msg << std::endl;
      exit(1);
   }
   in >> booster_inj_minutes;
   in >> tag;
   if (tag != "SIM_DURATION_MINUTES:") {
      std::cerr << error_msg << std::endl;
      exit(1);
   }
   in >> sim_after_detect_minutes;
   in >> tag;
   if (tag != "INJ_TYPE:") {
      std::cerr << error_msg << std::endl;
      exit(1);
   }
   std::string inj_type_string;
   in >> inj_type_string;
   booster_inj_type_ = StringToInjType(inj_type_string);
   if (booster_inj_type_ == InjType_UnDef) {
      std::cerr << "Booster Injection type specification not recognized: " << inj_type_string << std::endl;
      exit(1);
   }
   in >> tag;
   if (tag != "INJ_STRENGTH:") {
      std::cerr << error_msg << std::endl;
      exit(1);
   }
   in >> booster_inj_strength_;
   // These next lines are optional
   in >> tag;
   std::set<int> feasible_booster_nodes;
   bool do_empty = false;
   if (tag == "FEASIBLE_NODES:") {
      while (in.good()) {
         in >> tag;
         if (tag == "NZD") {
            // set the booster stations to the nzd junctions
            feasible_booster_nodes.insert(model_.nzd_junctions.begin(), model_.nzd_junctions.end());
         }
         else if (tag == "ALL") {
            // set the booster stations to all junctions
            feasible_booster_nodes.insert(model_.junctions.begin(), model_.junctions.end());
         }
         else if (tag == "NONE") {
            do_empty = true;
            break;
         }
         else {
	    feasible_booster_nodes.insert(model_.NodeID(tag));
         }
      }
   }
   //copy the temporary set to the vector
   booster_candidates_.clear();
   booster_candidates_.insert(booster_candidates_.end(), feasible_booster_nodes.begin(), feasible_booster_nodes.end());
   feasible_booster_nodes.clear();
   if (booster_candidates_.empty()) {
      booster_candidates_ = model_.nzd_junctions; //copy 
   }

   if (do_empty) {
      booster_candidates_.clear(); 
   }

   if (sim_after_detect_minutes < (delay_minutes+booster_inj_minutes)) {
      sim_after_detect_minutes = delay_minutes+booster_inj_minutes;
   }
   booster_delay_seconds_ = delay_minutes*60.0;
   booster_inj_seconds_ = booster_inj_minutes*60.0;
   sim_after_detect_timesteps_ = 
      merlionUtils::SecondsToNearestTimestepBoundary(sim_after_detect_minutes*60.0,
                                                     model_.qual_step_minutes);

   in.close();
}
