#include <source_inversion/InversionSimOptions.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>

InversionSimOptions::~InversionSimOptions() 
{
 
   registered_options.clear();

}


InversionSimOptions::InversionSimOptions()
   :
   required_args(2),
   logging(false),
   output_merlion_labels(false),
   epanet_output_filename("epanet.rpt"),
   merlion_save_filename(""),
   inp_filename(""),
   wqm_filename(""),
   nodemap_filename(""),
   output_prefix(""),
   measurement_filename(""),
   allowedNodes_filename(""),
   reduceSystem(false),
   output_impact_nodeNames(false),
   start_time(-1),
   snap_time(-1),
   probability(false),
   optimization(true),
   meas_failure(0.05),
   confidence(0.95),
   threshold(0.95),
   meas_threshold(0.0),
   sim_duration_min(-1),
   inversion_horizon(-1),
   qual_step_min(-1),
   wqm_zero_tol(0.0),
   decay_k(-1.0f),
   ignore_merlion_warnings(false),
   merlion(false)
{
   std::stringstream descr;
   descr << "inversionsim [options] " <<
            "<Required Network Option> " <<
            "<Measurement List file> ";
   option_parser_.add_usage(descr.str());
   descr.str(std::string());
   descr << "A utility for source inversion data generation " << 
            "used to determine potential injection nodes " <<
            "in water distribution systems.";
   option_parser_.description = descr.str();
   option_parser_.version("inversionsim 1.0");
   option_parser_.alias("version","v");

   // Add Epilog 
   option_parser_.epilog  = "The inversionsim command is used to finding potential injection nodes ";
   option_parser_.epilog += "in water distribution systems. This executable uses Merlion to perform water quality simulations ";
   option_parser_.epilog += "and generates the required data files to solve an optimization problem. ";
   
   // Add arguments
   option_parser_.add_argument("measurement list file", "A file with the grab samples");

   // Add Optional Arguments
   std::string cat_string;
   ////////////////// Required Network Option
   cat_string = "Required Network Option";
   // inp  
   descr.str(std::string());
   descr << "EPANET network file.";
   option_parser_.add("inp",inp_filename, descr.str());
   option_parser_.categorize("inp",cat_string);
   // wqm 
   descr.str(std::string());
   descr << "Merlion wqm file.";
   option_parser_.add("wqm",wqm_filename, descr.str());
   option_parser_.categorize("wqm",cat_string);

   ////////////////// Save File Options
   cat_string = "Save Options";
   // epanet-rpt-file  
   descr.str(std::string());
   descr << "Output file generated by EPANET during hydraulic " <<
            "simulations." ;
   option_parser_.add("epanet-rpt-file",epanet_output_filename, descr.str());
   option_parser_.categorize("epanet-rpt-file",cat_string);
   // merlion-save-file  
   descr.str(std::string());
   descr << "Text file defining the Merlion water quality model.";
   option_parser_.add("merlion-save-file",merlion_save_filename, descr.str());
   option_parser_.categorize("merlion-save-file",cat_string);

   ////////////////// EPANET Input File Options
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

   // horizon in minutes
   cat_string = "Time options";
   descr.str(std::string());
   descr << "Horizon time (min) at which the source inversion take place ";
   option_parser_.add("horizon-minutes",inversion_horizon, descr.str());
   option_parser_.categorize("horizon-minutes",cat_string);
   // Start inversion time
   descr.str(std::string());
   descr << "Start time (min) at which the source inversion take place ";
   option_parser_.add("start-inversion",start_time, descr.str());
   option_parser_.categorize("start-inversion",cat_string);
   // Time to take a snap of contaminated nodes
   descr.str(std::string());
   descr << "Time (min) to check number of uncertain nodes to be in the contamination plume ";
   option_parser_.add("snap-time",snap_time, descr.str());
   option_parser_.categorize("snap-time",cat_string);
   
   // probability options
   cat_string = "Probability options";
   descr.str(std::string());
   descr << "Measurement faliure rate in fraction(default:0.05)";
   option_parser_.add("meas-failure",meas_failure, descr.str());
   option_parser_.categorize("meas-failure",cat_string);
   descr.str(std::string());
   descr << "Confidence limit on probability(default:0.95)";
   option_parser_.add("prob-confidence",confidence, descr.str());
   option_parser_.categorize("prob-confidence",cat_string);
   
   
   ////////////////// Data Output Options
   cat_string = "Data Format Options";
   // output-prefix  
   descr.str(std::string());
   descr << "Prepend all output filenames with a string.";
   option_parser_.add("output-prefix",output_prefix, descr.str());
   option_parser_.categorize("output-prefix", cat_string);
   
   ////////////////// Threshold Option
   cat_string = "Threshold Option";
   // meas-threshold  
   descr.str(std::string());
   descr << "Value to determine if a measure is positive or negative. " << 
            "The default value is 0.1"; 
   option_parser_.add("meas-threshold",meas_threshold, descr.str());
   option_parser_.categorize("meas-threshold",cat_string);
   // threshold  
   descr.str(std::string());
   descr << "For optimization algorithm, this decides the objective cut-off for cndidate events." << 
            "The default value is 0.95"; 
   option_parser_.add("threshold",threshold, descr.str());
   option_parser_.categorize("threshold",cat_string);

   //Reaction decay constant
   descr.str(std::string()); 
   descr << "First-order decay coefficient(1/min)." << 
            "The default value is taken from INP file."; 
   option_parser_.add("decay-const",decay_k, descr.str());
   option_parser_.categorize("decay-const",cat_string);
   
   ////////////////// Impact Nodes Options
   cat_string = "Impact Nodes Options";
      // output-merlion-labels  
   descr.str(std::string());
   descr << "Merlion impact node names will be printed (Likely_Nodes.dat)";
   option_parser_.add("output-impact-nodes",output_impact_nodeNames, descr.str());
   option_parser_.categorize("output-impact-nodes",cat_string);
   descr.str(std::string());
   descr << "Allowed impact inversion node names";
   option_parser_.add("allowed-impacts",allowedNodes_filename, descr.str());
   option_parser_.categorize("allowed-impacts",cat_string);
 
  ////////////////// Algorithm type options
   cat_string = "Algorithm type options";
      // output-merlion-labels  
   descr.str(std::string());
   descr << "An optimization based algorithm is used. Prints data files for PYOMO and AMPL.";
   option_parser_.add("optimization",optimization, descr.str());
   option_parser_.categorize("optimization",cat_string);
   descr.str(std::string());
   descr << "A probability based algorithm is used to give a solution.(default)";
   option_parser_.add("probability",probability, descr.str());
   option_parser_.categorize("probability",cat_string);


  ////////////////// Simulation option
   cat_string = "Simulation option for probability method.";
      // merlion  
   descr.str(std::string());
   descr << "Select merlion for simulating candidate events." <<
     "Note that without this option merlion is still used to" <<
     "identify candidate events and EPANET is used to simulate them";
   option_parser_.add("merlion",merlion, descr.str());
   option_parser_.categorize("merlion",cat_string);
   
   ////////////////// Developer Options
   
   cat_string = "Developer Options (not recommended)";
   // ignore-merlion-warnings
   descr.str(std::string());
   descr << "Ignore Merlion Warnings ";
   option_parser_.add("ignore-merlion-warnings",ignore_merlion_warnings, descr.str());
   option_parser_.categorize("ignore-merlion-warnings",cat_string);
   // enable-logging
   descr.str(std::string());
   descr << "Generates a logfile with verbose runtime information.";
   option_parser_.add("enable-logging",logging, descr.str());
   option_parser_.categorize("enable-logging",cat_string);
   // wqm-zero
   descr.str(std::string());
   descr << "Zero tolerance for the reduced water quality model(default:0.0)";
   option_parser_.add("wqm-zero",wqm_zero_tol, descr.str());
   option_parser_.categorize("wqm-zero",cat_string);
   // disable-warnings 
   descr.str(std::string());
   descr << "Disables printing of warning statements to stderr.";
   option_parser_.add("disable-warnings",disable_warnings, descr.str());
   option_parser_.categorize("disable-warnings",cat_string);
   // output-full-system
   descr.str(std::string());
   descr << "Disables writing the reduced optimization problem ";
   option_parser_.add("disable-reduced-problem",reduceSystem, descr.str());   
   option_parser_.categorize("disable-reduced-problem", cat_string);
}

void InversionSimOptions::print_summary(std::ostream& out)
{
   out << "\nSummary of Input Options\n";
   option_parser_.write_values(out);
   out << ";\n";
   out << std::endl;  
}

bool InversionSimOptions::isDefault(std::string option_name) {
   return !option_parser_.initialized(option_name);
}

void InversionSimOptions::parse_inputs(int argc, char** argv)
{
   // Parse the arguments
   utilib::OptionParser::args_t args = option_parser_.parse_args(argc,argv);

   // check for help or version request and exit  
   if (option_parser_.help_option()) {
      option_parser_.write(std::cout);
      exit(0);
   }
   if (option_parser_.version_option()) {
      option_parser_.print_version(std::cout);
      exit(0);
   }
   utilib::OptionParser::args_t::iterator curr = args.begin();
   ++curr;
   if (curr == args.end()) {
      option_parser_.write(std::cout);
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing measurements file argument." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   measurement_filename=*curr++;
   if (curr != args.end()) {
      std::cerr << std::endl;
      std::cerr << "WARNING: Extra command line arguments were detected after final argument." << std::endl;
      std::cerr << "         Optional arguments must go before required arguments, otherwise" << std::endl;
      std::cerr << "         they will be ignored." << std::endl;
      std::cerr << std::endl;
   }
}
