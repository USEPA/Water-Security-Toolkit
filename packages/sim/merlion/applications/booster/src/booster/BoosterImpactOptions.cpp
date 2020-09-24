#include "BoosterImpactOptions.hpp"

#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>

BoosterImpactOptions::BoosterImpactOptions()
   :
   BoosterOptions(),
   report_scenario_timing(false),
   json(false),
   yaml(false),
   max_rhs(1),
   tsi_species_id(-1),
   chl_epanet_output_filename("booster-epanet.rpt"),
   tox_epanet_output_filename("tox-epanet.rpt"),
   chl_merlion_save_filename(""),
   tox_merlion_save_filename(""),
   tox_inp_filename(""),
   tox_wqm_filename(""),
   chl_inp_filename(""),
   chl_wqm_filename(""),
   tsg_filename(""),
   tsi_filename(""),
   scn_filename(""),
   dscn_filename(""),
   sensor_filename(""),
   booster_filename(""),
   wqm_inverse_filename(""),
   detection_interval_min(-1),
   detection_tol_gpm3(0.0f),
   zero_conc_tol_gpm3(0.0f),
   chl_decay_k(-1.0f),
   tox_decay_k(-1.0f)
{
   std::stringstream descr;
   descr << "boosterimpact [Options] " << 
      "<Required Network Option> " << 
      "<Required Scenario Option> " << 
      "<Detection Option> " << 
      "<booster-spec-file>";
   option_parser_.add_usage(descr.str());
   descr.str(std::string());
   descr << "A utility for rapid scenario impact analysis " << 
      "given a booster station placement a sensor placement. ";
   option_parser_.description = descr.str();
   option_parser_.version("boosterimpact 1.0");
   option_parser_.alias("version","v");

   // Add Epilog 
   option_parser_.epilog  = "The boosterimpact command is used to generate scenarios impacts by simulating contamination scenarios ";
   option_parser_.epilog += "in water distribution system that has a sensor layout and a booster station layout. This executable uses Merlion ";
   option_parser_.epilog += "to perform water quality simulations on a scenario ensemble. ";

   // Add arguments
   option_parser_.add_argument("booster specifications file","A file defining booster station specifications.");

   // Add Optional Arguments
   std::string cat_string;
   ////////////////// Required Network Option
   cat_string = "Required Network Option (1-tox, 1-booster)";
   // tox-inp
   descr.str(std::string());
   descr << "EPANET network file for toxin.";
   option_parser_.add("tox-inp",tox_inp_filename, descr.str());
   option_parser_.categorize("tox-inp",cat_string);
   // tox-wqm 
   descr.str(std::string());
   descr << "Merlion wqm file for toxin.";
   option_parser_.add("tox-wqm",tox_wqm_filename, descr.str());
   option_parser_.categorize("tox-wqm",cat_string);
   // booster-inp  
   descr.str(std::string());
   descr << "EPANET network file for boosters.";
   option_parser_.add("booster-inp",chl_inp_filename, descr.str());
   option_parser_.categorize("booster-inp",cat_string);
   // booster-wqm 
   descr.str(std::string());
   descr << "Merlion wqm file for boosters.";
   option_parser_.add("booster-wqm",chl_wqm_filename, descr.str());
   option_parser_.categorize("booster-wqm",cat_string);

   ////////////////// Required Scenario Option
   cat_string = "Required Scenario Option";
   // tsg
   descr.str(std::string());
   descr << "TSG file for specifying the injection incidents.";
   option_parser_.add("tsg",tsg_filename, descr.str());
   option_parser_.categorize("tsg",cat_string);
   // tsi
   descr.str(std::string());
   descr << "TSI file for specifying the injection incidents.";
   option_parser_.add("tsi",tsi_filename, descr.str());
   option_parser_.categorize("tsi",cat_string);
   // tsi-species-id
   descr.str(std::string());
   descr << "(*optional) The single TSI species id to use in each scenario by Merlion. "
         << "All other species will be ignored. If this option is not used and multiple "
         << "species ids are detected in the TSI file, an error will be thrown.";
   option_parser_.add("tsi-species-id",tsi_species_id, descr.str());
   option_parser_.categorize("tsi-species-id",cat_string);
   // scn 
   descr.str(std::string());
   descr << "SCN file for specifying the injection incidents.";
   option_parser_.add("scn",scn_filename, descr.str());
   option_parser_.categorize("scn",cat_string);
   // dscn 
   descr.str(std::string());
   descr << "DSCN file for specifying the injection incidents (scn file with detection times).";
   option_parser_.add("dscn",dscn_filename, descr.str());
   option_parser_.categorize("dscn",cat_string);

   ////////////////// Detection Option
   cat_string = "Detection Option (**Required for TSG, TSI, or SCN file)";
   // sensors
   descr.str(std::string());
   descr << "**A file with a list of sensor node names and/or grab sample node-times.";
   option_parser_.add("sensors",sensor_filename, descr.str());
   option_parser_.categorize("sensors",cat_string);
   // wqm-inverse
   descr.str(std::string());
   descr << "**A Merlion water quality model reduced inverse file.";
   option_parser_.add("wqm-inverse",wqm_inverse_filename, descr.str());
   option_parser_.categorize("wqm-inverse",cat_string);
   // detection-tol-gpm3
   descr.str(std::string());
   descr << "Concentration threshold for required for detection. " <<
      "Defaults to using the sparsity tolerance used by the Merlion " <<
      "water quality model (smallest possible value).";
   option_parser_.add("detection-tol-gpm3",detection_tol_gpm3, descr.str());
   option_parser_.categorize("detection-tol-gpm3",cat_string);
   // detection-interval-minutes
   descr.str(std::string());
   descr << "An interval in minutes over which all detection times are " <<
      "rounded to.";
   option_parser_.add("detection-interval-minutes",detection_interval_min, descr.str());
   option_parser_.categorize("detection-interval-minutes",cat_string);

   ////////////////// Timing Options
   cat_string = "Timing Options";
   // report-scenario-timing  
   descr.str(std::string());
   descr << "Reports time spent on individual scenario simulations.";
   option_parser_.add("report-scenario-timing",report_scenario_timing, descr.str());
   option_parser_.categorize("report-scenario-timing",cat_string);

   ////////////////// Speed Options  
   cat_string = "Speed Options";
   // max-rhs  
   descr.str(std::string());
   descr << "Use multiple right-hand sides when simulating " <<
      "booster station injections.\n"
      "\tinteger <= 0 try all rhs for a single scenario " <<
      "\tinteger >= 1 uses min(num,all_rhs) for block size";
   option_parser_.add("max-rhs",max_rhs, descr.str());
   option_parser_.categorize("max-rhs",cat_string);

   ////////////////// Data Output Options
   cat_string = "Data Format Options";
   // yaml  
   descr.str(std::string());
   descr << "Uses yaml file format for output file.";
   option_parser_.add("yaml",yaml, descr.str());
   option_parser_.categorize("yaml", cat_string);
   // json
   descr.str(std::string());
   descr << "Uses json file format for output file.";
   option_parser_.add("json",json, descr.str());
   option_parser_.categorize("json", cat_string);

   ////////////////// EPANET Input File Options
   cat_string = "EPANET Input File Options";
   // booster-epanet-rpt-file  
   descr.str(std::string());
   descr << "Output file generated by EPANET during hydraulic " <<
      "simulations for booster agent model." ;
   option_parser_.add("booster-epanet-rpt-file",chl_epanet_output_filename, descr.str());
   option_parser_.categorize("booster-epanet-rpt-file",cat_string);
   // tox-epanet-rpt-file  
   descr.str(std::string());
   descr << "Output file generated by EPANET during hydraulic " <<
      "simulations for toxin model." ;
   option_parser_.add("tox-epanet-rpt-file",tox_epanet_output_filename, descr.str());
   option_parser_.categorize("tox-epanet-rpt-file",cat_string);
   // booster-merlion-save-file  
   descr.str(std::string());
   descr << "Text file defining the Merlion water quality model for the boosters.";
   option_parser_.add("booster-merlion-save-file",chl_merlion_save_filename, descr.str());
   option_parser_.categorize("booster-merlion-save-file",cat_string);
   // tox-merlion-save-file  
   descr.str(std::string());
   descr << "Text file defining the Merlion water quality model for the toxin.";
   option_parser_.add("tox-merlion-save-file",tox_merlion_save_filename, descr.str());
   option_parser_.categorize("tox-merlion-save-file",cat_string);
   // booster-decay-const
   descr.str(std::string()); 
   descr << "First-order decay coefficient for booster agent(1/min)." << 
            "The default value is equal to bulk reaction k in INP file"; 
   option_parser_.add("booster-decay-const",chl_decay_k, descr.str());
   option_parser_.categorize("booster-decay-const",cat_string);
   // tox-decay-const
   descr.str(std::string()); 
   descr << "First-order decay coefficient for toxin(1/min)." << 
            "The default value is equal to bulk reaction k in INP file"; 
   option_parser_.add("tox-decay-const",tox_decay_k, descr.str());
   option_parser_.categorize("tox-decay-const",cat_string);

   ////////////////// Developer Options
   cat_string = "Developer Options (not recommended)";
   // zero-conc-tol-gpm3
   descr.str(std::string());
   descr << "Zero concentration threshold for toxin or disinfectant. " <<
      "Defaults to using the sparsity tolerance used by the Merlion " <<
      "water quality model (smallest possible value).";
   option_parser_.add("zero-conc-tol-gpm3",zero_conc_tol_gpm3, descr.str());
   option_parser_.categorize("zero-conc-tol-gpm3",cat_string);

   ////////////////// Health Impact Parameters
   cat_string = "Health Impact Parameters";
   // demand-percapita-m3pmin
   descr.str(std::string()); 
   descr << "Per capita usage rate used to determine per-node population"
         << " (DEFAULT: 0)";
   option_parser_.add("demand-percapita-m3pmin",demand_percapita_m3pmin, descr.str());
   option_parser_.categorize("demand-percapita-m3pmin",cat_string);
   // ingestion-rate-m3pmin
   descr.str(std::string()); 
   descr << "Per capita ingestion rate used to determine population dosage impact"
         << " (DEFAULT: 0)";
   option_parser_.add("ingestion-rate-m3pmin",ingestion_rate_m3pmin, descr.str());
   option_parser_.categorize("ingestion-rate-m3pmin",cat_string);
   // population-dosed-threshold-g
   descr.str(std::string()); 
   descr << "Per capita ingestion rate used to determine population dosage impact"
         << " (DEFAULT: 0)";
   option_parser_.add("population-dosed-threshold-g",population_dosed_threshold_g, descr.str());
   option_parser_.categorize("population-dosed-threshold-g",cat_string);

}

void BoosterImpactOptions::PrintSummary(std::ostream& out)
{
   out << "\nSummary of Input Options\n";
   option_parser_.write_values(out);
   out << ";\n";
   out << std::endl;
}

bool BoosterImpactOptions::isDefault(std::string option_name) {
   return !option_parser_.initialized(option_name);
}

void BoosterImpactOptions::ParseInputs(int argc, char** argv)
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
      std::cerr << "ERROR: Missing booster station spec file argument." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   booster_filename = *curr++;
   if (curr != args.end()) {
      std::cerr << std::endl;
      std::cerr << "ERROR:   Extra command line arguments were detected after final argument." << std::endl;
      std::cerr << "         Optional arguments must go before required arguments, otherwise" << std::endl;
      std::cerr << "         they will be ignored." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
}
