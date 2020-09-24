#include "CSArunOptions.hpp"

#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>

CSArunOptions::CSArunOptions()
   :
   logging(false),
   epanet_report_filename("epanet.rpt"),
   inp_filename(""),	     
   fixed_sensors_filename(""),
   measurements_filename(""), 
   output_prefix(""),
   quality_step_sec(3600.0),
   measurements_step_sec(3600.0),
   horizon(24.0),
   sim_duration(-1.0),
   meas_threshold(0.0),
   num_sensors(0),
   version_("1.0")              
{
  std::stringstream descr;
  descr << "csarun [options] " <<
    "<Required Network Option> " <<
    "<Events Sensor Option> " <<
    "<Required Measurements Option>";
  option_parser_.add_usage(descr.str());
  descr.str(std::string());
  descr << "A utility to run the Contaminant Source Algorithm " << 
    "used to determine candidate contamination events " <<
    "in water distribution systems.";
  option_parser_.description = descr.str();
  option_parser_.version("csarun 1.0");
  option_parser_.alias("version","v");
  
  // Add Epilog 
  option_parser_.epilog  = "The csarun command is used to finding potential injection events ";
  option_parser_.epilog += "in water distribution systems. This executable uses the CSA Toolkit witten by ";
  option_parser_.epilog += "Annamaria E. De Sanctis and generates the source status matrix file. ";
   
  // Add arguments
  //option_parser_.add_argument("Sample time", "Desired time [min] to take the grab samples");
  
  // Add Optional Arguments
  std::string cat_string;
  ////////////////// Required Network Option
  cat_string = "Required Network Option";
  // inp  
  descr.str(std::string());
  descr << "EPANET network file.";
  option_parser_.add("inp",inp_filename, descr.str());
  option_parser_.categorize("inp",cat_string);
  
  ////////////////// Required Sensor Option
  cat_string = "Required Sensor Option";
  // sensors 
  descr.str(std::string());
  descr << "File containing list of all sensors.";
  option_parser_.add("sensors",fixed_sensors_filename, descr.str());
  option_parser_.categorize("sensors",cat_string); 

  ////////////////// Required Measurement Option
  cat_string = "Required Measurements Option";
  // meas 
  descr.str(std::string());
  descr << "File containing historical measurements data. ";
  option_parser_.add("meas",measurements_filename, descr.str());
  option_parser_.categorize("meas",cat_string); 

  ////////////////// Time Options
  cat_string = "Time Options";
  // qual-step-sec  
   descr.str(std::string());
   descr << "Water quality step used to build the EPANET-BTX " << 
            "model. Units: Seconds, Default: 3600.0 "; 
   option_parser_.add("qual-step-sec",quality_step_sec, descr.str());
   option_parser_.categorize("qual-step-sec",cat_string);
   // meas-step-sec  
   descr.str(std::string());
   descr << "Size of measurement timestep used in the measurements file. " << 
     "Units: Seconds, Default: 3600.0"; 
   option_parser_.add("meas-step-sec",measurements_step_sec, descr.str());
   option_parser_.categorize("meas-step-sec",cat_string);
   //horizon
   descr.str(std::string()); 
   descr << "Horizon of measurement data to consider for inversion. " << 
   "Units: Hours, Default: 24.0"; 
   option_parser_.add("horizon",horizon, descr.str());
   option_parser_.categorize("horizon",cat_string);
   //sim_duration
   descr.str(std::string()); 
   descr << "Epanet simulation duration consider for inversion. " << 
   "Units: Hours, Default: From INP File"; 
   option_parser_.add("sim-duration",sim_duration, descr.str());
   option_parser_.categorize("sim-duration",cat_string);
   
   ////////////////// Other Options
   cat_string = "Other Options";
   // output-prefix  
   descr.str(std::string());
   descr << "Prepend all output filenames with a string.";
   option_parser_.add("output-prefix",output_prefix, descr.str());
   option_parser_.categorize("output-prefix", cat_string);
   // meas-threshold 
   descr.str(std::string());
   descr << "Concentration threshold above which a measurement " <<
     "is considered positive. Units: gm/L, Default: 0.0";
   option_parser_.add("meas-threshold",meas_threshold, descr.str());
   option_parser_.categorize("meas-threshold",cat_string); 
   // num-sensors 
   descr.str(std::string());
   descr << "Number of sensors provided in the measurements file. " <<
     "These are the top n sensors in the sensors file. ";
   option_parser_.add("num-sensors",num_sensors, descr.str());
   option_parser_.categorize("num-sensors",cat_string); 
   
  ////////////////// Developer Options
}
void CSArunOptions::print_summary(std::ostream& out)
{
  out << "\nSummary of Input Options\n";
  option_parser_.write_values(out);
  out << ";\n";
  out << std::endl; 
}
   
void CSArunOptions::parse_inputs(int argc, char** argv)
{
  // Parse the arguments
  utilib::OptionParser::args_t args = option_parser_.parse_args(argc,argv);
  std::string temp(""); 
  if (option_parser_.help_option()) {
    option_parser_.write(std::cout);
    exit(0);
  }
  if (option_parser_.version_option()) {
    option_parser_.print_version(std::cout);
    exit(0);
  }
  //      utilib::OptionParser::args_t::iterator curr = args.begin();
  //      ++curr;
  //      if (curr == args.end()) {
  //         option_parser_.write(std::cout);
  //         std::cerr << std::endl;
  //         std::cerr << "ERROR: Missing sample time." << std::endl;
  //         std::cerr << std::endl;
  //         exit(1);
  //      }
  //      temp=*curr++;      
  //      sample_time=atoi(temp.c_str());
  //      if (curr != args.end()) {
  //         std::cerr << std::endl;
  //         std::cerr << "WARNING: Extra command line arguments were detected after final argument." << std::endl;
  //         std::cerr << "         Optional arguments must go before required arguments, otherwise" << std::endl;
  //         std::cerr << "         they will be ignored." << std::endl;
  //         std::cerr << std::endl;
  //     
}

