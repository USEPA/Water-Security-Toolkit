#ifndef INVERSION_SIMOPTIONS_HPP__
#define INVERSION_SIMOPTIONS_HPP__

#include <string>
#include <vector>


#include <utilib/OptionParser.h>

class InversionSimOptions
{
   int required_args;
   std::string version_;
public:
   
   InversionSimOptions();   
   ~InversionSimOptions();
   
   // set default values in the class constructor
   bool logging;
   bool output_merlion_labels;
   bool disable_warnings;
   bool ignore_merlion_warnings;;
   std::string epanet_output_filename;
   std::string merlion_save_filename;
   std::string inp_filename;
   std::string wqm_filename;
   std::string nodemap_filename;
   std::string output_prefix;
   std::string measurement_filename;
   bool reduceSystem;
   int inversion_horizon;           
   int start_time;
   int snap_time;
   float meas_threshold;
   float threshold;
   float sim_duration_min;               
   float qual_step_min;
   std::string allowedNodes_filename;
   bool output_impact_nodeNames;
   bool optimization;
   bool probability;
   bool merlion;
   float meas_failure;
   float confidence;
   float wqm_zero_tol;
   float decay_k;
   std::vector<std::string> registered_options;
   bool isDefault(std::string option_name);
   void print_summary(std::ostream& out);
   void parse_inputs(int argc, char** argv);
   
   
private:
   utilib::OptionParser option_parser_;    
};

#endif
