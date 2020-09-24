#ifndef SAMPLE_LOCATION_OPTIONS_HPP__
#define SAMPLE_LOCATION_OPTIONS_HPP__

#include <utilib/OptionParser.h>

#include <string>

class SampleLocationOptions
{
 private:
   utilib::OptionParser option_parser_;   
   
 public:  
   
   //Constructor 
   SampleLocationOptions();
   
   // set default values in the class constructor
   bool logging;
   bool output_merlion_labels;
   bool output_epanet_labels;
   bool disable_warnings;
   bool ignore_merlion_warnings;
   bool greedy;
   bool weights;
   bool merlion;
   bool RLE;
   std::string epanet_output_filename;
   std::string merlion_save_filename;
   std::string inp_filename;
   std::string wqm_filename;
   std::string scn_filename;
   std::string tsg_filename;
   std::string tsi_filename;
   std::string nodemap_filename;
   std::string fixed_sensors_filename;
   std::string allowedNodes_filename;
   std::string output_prefix;
   std::string version_;
   bool output_impact_matrix;
   int sample_time;
   int nSamples;
   float sim_duration_min;               
   float qual_step_min;
   float threshold;
   float decay_k;
   
   void print_summary(std::ostream& out);
   void parse_inputs(int argc, char** argv);
   
};

#endif
