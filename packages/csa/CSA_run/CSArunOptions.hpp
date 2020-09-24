#ifndef CSARUN_OPTIONS_HPP__
#define CSARUN_OPTIONS_HPP__

#include <utilib/OptionParser.h>

#include <string>

class CSArunOptions 
{
private:
  utilib::OptionParser option_parser_;   
  
public:  
  
  //Constructor 
  CSArunOptions();
  
  // set default values in the class constructor
  bool logging;
  std::string epanet_report_filename;
  std::string inp_filename;
  std::string fixed_sensors_filename;
  std::string measurements_filename;
  std::string output_prefix;
  std::string version_;
  float quality_step_sec; //water quality step in senconds used by BTX 
  float measurements_step_sec; //measurements time step in seconds
  float horizon; // inversion horizon in hours                
  float sim_duration; // simulation duration in hours                
  float meas_threshold;
  int num_sensors; // number of sensors selected from the fixed_sensors_file

  void print_summary(std::ostream& out);
  void parse_inputs(int argc, char** argv);
  
};

#endif
