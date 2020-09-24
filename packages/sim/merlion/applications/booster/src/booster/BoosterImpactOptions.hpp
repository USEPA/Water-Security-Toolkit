#ifndef BOOSTER_BOOSTERIMPACTOPTIONS_HPP__
#define BOOSTER_BOOSTERIMPACTOPTIONS_HPP__

#include "BoosterOptions.hpp"

class BoosterImpactOptions : public BoosterOptions
{
public:
   BoosterImpactOptions();
   virtual ~BoosterImpactOptions() {}
   virtual void ParseInputs(int argc, char** argv);
   virtual void PrintSummary(std::ostream& out);
   virtual bool isDefault(std::string option_name);

   bool report_scenario_timing;
   bool json;
   bool yaml;
   int max_rhs;
   int tsi_species_id;
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
   float detection_interval_min;
   float detection_tol_gpm3;
   float zero_conc_tol_gpm3;
   float tox_decay_k;
   float chl_decay_k;
   double demand_percapita_m3pmin;
   double ingestion_rate_m3pmin;
   double population_dosed_threshold_g;
};

#endif
