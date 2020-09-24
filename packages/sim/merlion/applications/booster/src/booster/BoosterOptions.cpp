#include "BoosterOptions.hpp"

#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>

BoosterOptions::BoosterOptions()
   :
   logging(false),
   output_merlion_labels(false),
   disable_warnings(false),
   ignore_merlion_warnings(false),
   nodemap_filename(""),
   output_prefix(""),
   sim_duration_min(-1),
   qual_step_min(-1)
{
   std::stringstream descr;
   // Add Optional Arguments
   std::string cat_string;

   ////////////////// Other Network Options
   cat_string = "EPANET Input File Options";
   // simulation-duration-minutes  
   descr.str(std::string());
   descr << "Length of simulation period to build Merlion water " << 
      "quality model. When specified, this value will " << 
      "override what is in the EPANET input file."; 
   option_parser_.add("simulation-duration-minutes",sim_duration_min, descr.str());
   option_parser_.categorize("simulation-duration-minutes",cat_string);
   // quality-timestep-minutes  
   descr.str(std::string());
   descr << "Size of water quality timestep used by Merlion to " << 
      "perform water quality simulations. When specified, " << 
      "this value will override what is in the EPANET " << 
      "input file.";
   option_parser_.add("quality-timestep-minutes",qual_step_min, descr.str());
   option_parser_.categorize("quality-timestep-minutes",cat_string);

   ////////////////// Label Options
   cat_string = "Label Options";
   // custom-label-map  
   descr.str(std::string());
   descr << "Simple file mapping EPANET node names to custom " <<
      "labels. All data files will be written using these " << 
      "custom labels.";
   option_parser_.add("custom-label-map",nodemap_filename, descr.str());
   option_parser_.categorize("custom-label-map",cat_string);
   // output-merlion-labels  
   descr.str(std::string());
   descr << "Node names will be translated into integer node IDs " <<
      "to reduce file sizes for large networks. A node map " <<
      "is provided to map node IDs back to node names " <<
      "(MERLION_LABEL_MAP.txt). This option is overridden  " <<
      "by the --custome-label-map flag.";
   option_parser_.add("output-merlion-labels",output_merlion_labels, descr.str());
   option_parser_.categorize("output-merlion-labels",cat_string);

   ////////////////// Data Output Options
   cat_string = "Data Format Options";
   // output-prefix  
   descr.str(std::string());
   descr << "Prepend all output filenames with a string.";
   option_parser_.add("output-prefix",output_prefix, descr.str());
   option_parser_.categorize("output-prefix", cat_string);
   
   ////////////////// Developer Options
   cat_string = "Developer Options (not recommended)";
   // disable-warnings 
   descr.str(std::string());
   descr << "Disables printing of warning statements to stderr.";
   option_parser_.add("disable-warnings",disable_warnings, descr.str());
   option_parser_.categorize("disable-warnings",cat_string);
   // ignore-merlion-warnings 
   descr.str(std::string());
   descr << "Ignore warnings about unsupported features of Merlion.";
   option_parser_.add("ignore-merlion-warnings",ignore_merlion_warnings, descr.str());
   option_parser_.categorize("ignore-merlion-warnings",cat_string);
   // enable-logging
   descr.str(std::string());
   descr << "Generates a logfile with verbose runtime information.";
   option_parser_.add("enable-logging",logging, descr.str());
   option_parser_.categorize("enable-logging",cat_string);

}
