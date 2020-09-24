#ifndef EVENTDETECTION_OPTIONS_HPP__
#define EVENTDETECTION_OPTIONS_HPP__

#include <utilib/OptionParser.h>

class EventDetectionOptions
{
public:
   EventDetectionOptions();
   void ParseInputs(int argc, char** argv);
   void PrintSummary(std::ostream& out);
   bool isDefault(std::string option_name);

   bool logging;
   bool yaml;
   bool json;
   bool detected_scn;
   bool disable_warnings;
   bool ignore_merlion_warnings;
   bool save_wqm_inverse;
   bool uniform_random_detection_times;
   bool multiple_detection_times;
   std::string epanet_output_filename;
   std::string merlion_save_filename;
   std::string inp_filename;
   std::string wqm_filename;
   std::string tsg_filename;
   std::string tsi_filename;
   std::string scn_filename;
   std::string sensor_filename;
   std::string output_prefix;
   std::string wqm_inverse_filename;
   int random_seed;
   int tsi_species_id;
   float detection_interval_min;
   float sensor_interval_min;
   float sim_duration_min;
   float qual_step_min;
   float detection_tol_gpm3;
   float decay_k;
   float detection_window_lower_min;
   float detection_window_upper_min;
   float detection_delay_mean;
private:
   utilib::OptionParser option_parser_;
};

#endif
