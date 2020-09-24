#ifndef BOOSTER_BOOSTERSIMOPTIONS_HPP__
#define BOOSTER_BOOSTERSIMOPTIONS_HPP__

#include "BoosterOptions.hpp"

class BoosterSimOptions : public BoosterOptions
{
public:

   BoosterSimOptions();
   virtual ~BoosterSimOptions() {}
   virtual void ParseInputs(int argc, char** argv);
   virtual void PrintSummary(std::ostream& out);
   virtual bool isDefault(std::string option_name);

   bool report_scenario_timing;
   bool no_reduce;
   bool disable_scenario_aggregation;
   bool output_PYOMO;
   bool output_PYSP;
   bool detections_only;
   bool disable_reduced_problem;
   bool determine_single_impacts;
   std::string scenario_weights_filename;
   std::string chl_epanet_output_filename;
   std::string tox_epanet_output_filename;
   std::string chl_merlion_save_filename;
   std::string tox_merlion_save_filename;
   std::string tox_inp_filename;
   std::string tox_wqm_filename;
   std::string chl_inp_filename;
   std::string chl_wqm_filename;
   std::string tsg_filename;
   std::string tsi_filename;
   std::string scn_filename;
   std::string dscn_filename;
   std::string sensor_filename;
   std::string booster_filename;
   std::string wqm_inverse_filename;
   int tsi_species_id;
   int max_rhs;
   int postprocess_passes;
   float detection_interval_min;
   float postprocess_tol;
   float detection_tol_gpm3;
   float zero_conc_tol_gpm3;
   float tox_decay_k;
   float chl_decay_k;
   double demand_percapita_m3pmin;
   double ingestion_rate_m3pmin;
};

#endif
