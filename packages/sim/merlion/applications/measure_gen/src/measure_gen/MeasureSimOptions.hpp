#ifndef MEASURE_SIMOPTIONS_HPP__
#define MEASURE_SIMOPTIONS_HPP__

#include <string>
#include <vector>


#include <utilib/OptionParser.h>

class MeasureGenOptions
{
   int required_args;
   std::string version_;
public:

   MeasureGenOptions();   
   ~MeasureGenOptions();

   // set default values in the class constructor
   bool logging;
   bool disable_warnings;
   bool output_merlion_labels;
   bool print_concentrations;
   bool ignore_merlion_warnings;
   std::string epanet_output_filename;
   std::string merlion_save_filename;
   std::string inp_filename;
   std::string wqm_filename;
   std::string tsg_filename;
   std::string tsi_filename;
   std::string scn_filename;
   std::string sensor_filename;
   std::string nodemap_filename;
   std::string output_prefix;
   int tsi_species_id;
   int start_time_sensing;
  //int stop_time_sensing;
  std::string stop_time_sensing;
   int measurements_per_hour; 
   float sim_duration_min;
   float qual_step_min;
   float sigma;
   float threshold;
   int seed;
   float FPR;
   float FNR;
   float decay_k;

   std::vector<std::string> registered_options;

   void print_summary(std::ostream& out);
   bool isDefault(std::string option_name);
   void parse_inputs(int argc, char** argv);
   
   utilib::OptionParser option_parser_;
};

#endif
