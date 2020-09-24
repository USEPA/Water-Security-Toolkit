#include <booster/BoosterDataWriter.hpp>

BoosterDataWriter::BoosterDataWriter(NetworkSimulator *Net, BoosterNetworkSimulator *Chl_Net, NetworkSimulator *Tox_Net, BoosterOptions *Opts)
:
   net_(Net),
   model_((Net==NULL)?(NULL):(&(Net->Model()))),
   chl_net_(Chl_Net),
   chl_model_((Chl_Net==NULL)?(NULL):(&(Chl_Net->Model()))),
   tox_net_(Tox_Net),
   tox_model_((Tox_Net==NULL)?(NULL):(&(Chl_Net->Model()))),
   options_(Opts)
{

   // set the output precision
   int precision_number = 8;
   out_SUM.setf(std::ios::scientific,std::ios::floatfield);
   out_SUM.precision(precision_number);
   out_SCL.setf(std::ios::scientific,std::ios::floatfield);
   out_SCL.precision(precision_number);
   out_SPCT.setf(std::ios::scientific,std::ios::floatfield);
   out_SPCT.precision(precision_number);
   out_CONT.setf(std::ios::scientific,std::ios::floatfield);
   out_CONT.precision(precision_number);
   out_REDM.setf(std::ios::scientific,std::ios::floatfield);
   out_REDM.precision(precision_number);
   out_DIND.setf(std::ios::scientific,std::ios::floatfield);
   out_DIND.precision(precision_number);
   out_BC.setf(std::ios::scientific,std::ios::floatfield);
   out_BC.precision(precision_number);
   out_TOX.setf(std::ios::scientific,std::ios::floatfield);
   out_TOX.precision(precision_number);
   out_CHL.setf(std::ios::scientific,std::ios::floatfield);
   out_CHL.precision(precision_number);
   out_DEM.setf(std::ios::scientific,std::ios::floatfield);
   out_DEM.precision(precision_number);
   out_TANK.setf(std::ios::scientific,std::ios::floatfield);
   out_TANK.precision(precision_number);
   out_WQM.setf(std::ios::scientific,std::ios::floatfield);
   out_WQM.precision(precision_number);
   out_HEAD.setf(std::ios::scientific,std::ios::floatfield);
   out_HEAD.precision(precision_number);
   out_TYPES.setf(std::ios::scientific,std::ios::floatfield);
   out_TYPES.precision(precision_number);
   out_PYSP.setf(std::ios::scientific,std::ios::floatfield);
   out_PYSP.precision(precision_number);
   out_POP.setf(std::ios::scientific,std::ios::floatfield);
   out_POP.precision(precision_number);

   // define the data file names
   out_SUM_fname = options_->output_prefix+"SCENARIO_SUMMARY.dat";
   out_SCL_fname = options_->output_prefix+"SCENARIO_SCALING.dat";
   out_SPCT_fname = options_->output_prefix+"IMPACTS.dat";
   out_CONT_fname = options_->output_prefix+"CONTROLLER_NODES.dat";
   out_REDM_fname = options_->output_prefix+"OBJECTIVE_TERMS.dat";
   out_DIND_fname = options_->output_prefix+"DELTA_INDEX.dat";
   out_NMAP_fname = options_->output_prefix+"MERLION_LABEL_MAP.txt";
   out_BC_fname = options_->output_prefix+"BOOSTER_CANDIDATES.dat";
   out_TOX_fname = options_->output_prefix+"TOXIN_INJECTION.dat";
   out_CHL_fname = options_->output_prefix+"BOOSTER_INJECTION.dat";
   out_POP_fname = options_->output_prefix+"POPULATION_DOSAGE.dat";
   
   out_DEM_basename = "DEMANDS.dat";
   out_DEM_fname = options_->output_prefix+out_DEM_basename;
   out_TANK_basename = "TANK_VOLUMES.dat";
   out_TANK_fname = options_->output_prefix+out_TANK_basename;
   out_WQM_basename = "WQM.dat";
   out_WQM_fname = options_->output_prefix+out_WQM_basename;
   out_HEAD_basename = "WQM_HEADERS.dat";
   out_HEAD_fname = options_->output_prefix+out_HEAD_basename;
   out_TYPES_basename = "WQM_TYPES.dat";
   out_TYPES_fname = options_->output_prefix+out_TYPES_basename;
   
   //out_PYSP_fname = options_->output_prefix+"ScenarioStructure.dat";

   // create the modifed node map taking
   // merlion id -> epanet name -> custom label
   std::map<std::string,std::string> name_map;
   std::map<std::string,std::string> label_map;
   if (options_->nodemap_filename != "") {
      // use custom labels in data files
      std::ifstream in;
      in.open(options_->nodemap_filename.c_str(), std::ios_base::in);
      if (in.is_open()) {
         std::string in_name, in_label;
         std::vector<std::string>::iterator findpos;
         for (int i = 0, stop = model_->n_nodes; i < stop; ++i) {
            in >> in_name;
	    int merlion_node_id = model_->NodeID(in_name);
	    in >> in_label;
	    if (label_map.find(in_label) != label_map.end()) {
	       std::cerr << "Duplicate labels found in node map file: " << in_label << std::endl; 
	    } 
	    else {
	       label_map[in_label] = in_name;
	       name_map[in_name] = in_label;
	    }
         }
      }
      else {
         std::cerr << "Failed to open custom node map file: " << options_->nodemap_filename << std::endl;
         exit(1); 
      }
      in.close();
   }
   else if (options_->output_merlion_labels) {
      // use merlion integer ids in data files
      typedef std::map<std::string, int> map_string_to_int;
      for (int n = 0; n < model_->n_nodes; ++n) {
	 std::ostringstream stream;
         stream << n;
         name_map[model_->NodeName(n)] = stream.str();
      }
   }
   else {
      // use epanet names in data files
      typedef std::map<std::string, int> map_string_to_int;
      for (int n = 0; n < model_->n_nodes; ++n) {
	 name_map[model_->NodeName(n)] = model_->NodeName(n);
      }
   }
   for (int n = 0; n < model_->n_nodes; ++n) {
      node_map_[n] = name_map[model_->NodeName(n)]; 
   }

   //write the map from node names to merlion labels
   out_NMAP.open(out_NMAP_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   for (int n = 0; n < model_->n_nodes; ++n) {
      out_NMAP << model_->NodeName(n) << " " << name_map[model_->NodeName(n)] << "\n"; 
   }
   out_NMAP.close();

}

void BoosterDataWriter::clear()
{
   net_ = NULL;
   model_ = NULL;
   chl_net_ = NULL;
   chl_model_ = NULL;
   tox_net_ = NULL;
   tox_model_ = NULL;
   options_ = NULL;
   node_map_.clear();
}

BoosterDataWriter::~BoosterDataWriter()
{
   clear(); 
}
