#include <merlionUtils/TSG_Reader.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/MerlionDefines.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

namespace merlionUtils {

namespace {

std::vector<std::string> tokenize(const char* str, const char* delim)
{
   std::vector<std::string> tokens;
   char* token = strtok (const_cast<char*>(str), const_cast<char*>(delim));
   while (token != NULL) {
      tokens.push_back(token);
      token = strtok (NULL, delim);
   }
   return tokens;
}

int get_node_idx(std::string source,const std::map<std::string,int>& node_name_idx_map)
{
   int inj_node_idx;
   std::map<std::string,int>::const_iterator iter;
   iter=node_name_idx_map.find(source);
   if(iter == node_name_idx_map.end()) {
      throw new std::out_of_range("element not found");
   }
   inj_node_idx=iter->second;
   return inj_node_idx;
}

struct SetTracker {
   std::vector<int>::const_iterator begin;
   std::vector<int>::const_iterator end;
   std::vector<int>::const_iterator current;
};

template<typename T>
void cartesian_product(const std::vector<std::vector<int> >& set_list, std::vector<T>& product_set)
{
   std::vector<SetTracker> status;

   // Start all of the n-tuple positions at the beginning of their respective sets
   for(std::vector<std::vector<int> >::const_iterator pos = set_list.begin(), pos_end = set_list.end(); pos != pos_end; ++pos) {
      SetTracker st = {pos->begin(), pos->end(), pos->begin()};
      status.push_back(st);
   }

   while(1) {

      // build the n-tuple with the current member that each n-tuple position is pointing to from its respective set
      T result;
      for(std::vector<SetTracker>::const_iterator pos = status.begin(), pos_end = status.end(); pos != pos_end; ++pos) {
	 // we use insert because this method is common to both vectors and sets and the type of T may be one of these.
	 result.insert(result.end(),*(pos->current));
      }
      product_set.push_back(result);
      
      // Increment the leftmost current element of the n-tuple.
      // If this is not the end of that tuple position's set, go to the beginning of the while loop and write the next n-tuple.
      // If this is the end of that tuple position's set, reset that tuple position to its set's beginning 
      // and increment the neighboring tuple position in its set.
      // If you reach the end of a tuple position's set and there is no neighboring tuple to increment, the cartesian product
      // is complete.
      for(std::vector<SetTracker>::iterator pos = status.begin(), pos_end = status.end(); ; ) {
	 ++(pos->current);
	 if(pos->current == pos->end) {
	    if(pos + 1 == pos_end) {return;}
	    else {
	       pos->current = pos->begin;
	       ++pos;
	    }
	 }
	 else {
	    break;
	 }
      }
   }
}

} // end of merlionUtils (local) namespace

void ReadTSG(std::string tsg_filename, Merlion* model, InjScenList& injection_scenarios)
{
   std::ifstream tsg;
   tsg.open(tsg_filename.c_str(), std::ios_base::in);
   if (!tsg.is_open()) {
      std::cerr << std::endl;
      std::cerr << "TSG ERROR: Unable to open file" << std::endl;
      std::cerr << std::endl;
      throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
   }

   std::vector<int> junctions_idx;
   std::vector<int> nzd_junctions_idx;  
   
   for (int i=0; i<model->NNodes(); i++) {
      if (model->NodeIdxTypeMap()[i] == NodeType_Junction) {
         junctions_idx.push_back(i);
         for (int j=0; j<model->NSteps(); j++) {
            int idx = i*model->NSteps()+j;
            if (model->GetNodeDemands_m3pmin()[idx]>0.0) {
               nzd_junctions_idx.push_back(i);
               break;
            }
         }
      }
   }
   std::string buffer;
   int scen_name_cntr = 0; // name the scenarios with a counter
   while (tsg.good()) {
      buffer = "";
      std::getline(tsg,buffer);
      std::vector<std::string> tokens = tokenize(buffer.c_str(), " \t\r\n");
      
      int n_tokens = tokens.size();
      
      if (n_tokens == 0) {
	 continue;
      }
      
      std::string temp = tokens[0];
      
      if (temp.compare(0,1,";") == 0) {
	 continue;
      }
      
      if (n_tokens < 5) {
	 std::cerr << std::endl;
	 std::cerr << "TSG ERROR: Invalid line format, not enough tokens" << std::endl;
	 std::cerr << std::endl;
	 throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
      }
      if (n_tokens > 6) {
	 std::cerr << std::endl;
	 std::cerr << "TSG ERROR: Invalid line format, too many tokens," << std::endl;
	 std::cerr << "           Merlion only supports single species injections" << std::endl;
	 std::cerr << std::endl;
	 throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
      }
      
      temp = tokens[n_tokens -3];
      float inj_strength = atof(temp.c_str());
      if (inj_strength <= 0) {
	 std::cerr << std::endl;
	 std::cerr << "TSG ERROR: Injection strength must be a positive number" << std::endl;
	 std::cerr << std::endl;
	 throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
      }
      
      temp = tokens[n_tokens -2];
      float inj_start_seconds = atof(temp.c_str());
      int inj_start_timesteps = Injection::SecondsToTimestep(inj_start_seconds, model->Stepsize_min());
      
      temp = tokens[n_tokens -1];
      float inj_end_seconds = atof(temp.c_str());
      int inj_end_timesteps = Injection::SecondsToTimestep(inj_end_seconds, model->Stepsize_min());
      
      // check that the injection takes place within the simulation period
      if (!(inj_end_timesteps >= 0 && inj_end_timesteps < model->NSteps())) {
	 std::cerr << std::endl;
	 std::cerr << "TSG ERROR: Injection does not take place within simulation period" << std::endl;
	 std::cerr << std::endl;
	 throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
      }
      if (!(inj_start_timesteps >= 0 && inj_start_timesteps < model->NSteps())) {
	 std::cerr << std::endl;
	 std::cerr << "TSG ERROR: Injection does not take place within simulation period" << std::endl;
	 std::cerr << std::endl;
	 throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
      }
      if (!(inj_end_timesteps >= inj_start_timesteps)) {
	 std::cerr << std::endl;
	 std::cerr << "TSG ERROR: Injection stop time occurs before injection start time" << std::endl;
	 std::cerr << std::endl;
	 throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
      }
      
      std::vector<std::vector<int> > sources;
      std::string inj_type;
      for (int i=0; i<n_tokens-3; i++) {
	 temp = tokens[i];
	 if (temp != "CONCEN" && temp != "MASS" && temp != "FLOWPACED" && temp != "SETPOINT" &&
	     temp != "concen" && temp != "mass" && temp != "flowpaced" && temp != "setpoint") {
	    if (temp == "NZD") {sources.push_back(nzd_junctions_idx);}
	    else if (temp == "ALL") {sources.push_back(junctions_idx);}
	    else {
	       int inj_node_idx=get_node_idx(temp,model->NodeNameIdxMap());
	       sources.push_back(std::vector<int>(1,inj_node_idx));
	    }
	 }
	 else {
	    if ((temp == "MASS") || (temp == "mass")) {
	       inj_type = temp;
	    }
	    else if ((temp == "FLOWPACED") || (temp == "flowpaced")) {
	       inj_type = temp;
	    }
	    else {
	       std::cerr << std::endl;
	       std::cerr << "TSG ERROR: Merlion only supports MASS or FLOWPACED type injections" << std::endl;
	       std::cerr << std::endl;
	       throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
	    }
	    break;
	 }
      }
      
      // Find the cartesian product of the sources for this line in the tsg file.
      // This is mainly for cases where NZD or ALL is specified along with one or more node names
      
      // This definition of nTuple implies that duplicate injections _WILL_ _NOT_ occur when the 
      // cartesian product results in repeated node ids for the list of sources
      typedef std::set<int> nTuple;
      // This definition of nTuple implies that duplicate injections _WILL_ occur when the 
      // cartesian product results in repeated node ids for the list of sources
      //typedef std::vector<int> nTuple;
      
      std::vector<nTuple> expanded_sources;
      cartesian_product(sources,expanded_sources);
      sources.clear();
      
      // Fill in the list InjScenario pointers
      for (std::vector<nTuple>::iterator pos_v = expanded_sources.begin(), pos_v_stop = expanded_sources.end(); pos_v != pos_v_stop; ++pos_v) {
	 nTuple& source_node_ids = *pos_v;
	 PInjScenario tmp_scenario(new InjScenario);
	 std::stringstream scen_name;
	 scen_name << scen_name_cntr++;
	 tmp_scenario->Name() = scen_name.str();
	 for (nTuple::iterator pos_s = source_node_ids.begin(), pos_s_stop = source_node_ids.end(); pos_s != pos_s_stop; ++pos_s) {
	    PInjection tmp(new Injection);
	    tmp->NodeName() = model->NodeIdxNameMap()[*pos_s];
	    tmp->Type() = StringToInjType(inj_type);
	    tmp->Strength() = inj_strength;
	    if (tmp->Type() == InjType_Mass) {
	       tmp->Strength() *= .001; //mg to g
	    }
	    if (tmp->Type() == InjType_UnDef) {
	       std::cerr << std::endl;
	       std::cerr << "TSG ERROR: Toxin injection type not recognized: " << inj_type << std::endl;
	       std::cerr << std::endl;
	       throw std::runtime_error("Failed to read tsg file: "+tsg_filename);
	    }
	    tmp->StartTimeSeconds() = inj_start_seconds;
	    tmp->StopTimeSeconds() = inj_end_seconds;
	    tmp_scenario->Injections().push_back(tmp);
	 }
	 injection_scenarios.push_back(tmp_scenario);
      }
      expanded_sources.clear();
   }
}

void ReadTSI(std::string tsi_filename, 
             Merlion* model, 
             InjScenList& injection_scenarios,
             int injection_species_id/* = -1 */)
{
   std::ifstream tsi;
   tsi.open(tsi_filename.c_str(), std::ios_base::in);
   if (!tsi.is_open()) {
      std::cerr << std::endl;
      std::cerr << "TSI ERROR: Unable to open file" << std::endl;
      std::cerr << std::endl;
      throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
   }

   std::string buffer;
   std::set<int> species_ids;
   int scen_name_cntr = 0; // name the scenarios with a counter
   while (tsi.good()) {
      buffer = "";
      std::getline(tsi,buffer);
      std::vector<std::string> tokens = tokenize(buffer.c_str(), " \t\r\n");
      
      int n_tokens = tokens.size();
      
      if (n_tokens == 0) {
	 continue;
      }
      
      if (tokens[0].compare(0,1,";") == 0) {
	 continue;
      }
      
      if ( (n_tokens%6) != 0 ) {
	 std::cerr << std::endl;
	 std::cerr << "TSI ERROR: Invalid line format, wrong number of tokens (found " << n_tokens << ")" << std::endl;
	 std::cerr << std::endl;
	 throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
      }
      int n_injections = n_tokens/6;

      PInjScenario tmp_scenario(new InjScenario);
      std::stringstream scen_name;
      scen_name << scen_name_cntr++;
      tmp_scenario->Name() = scen_name.str();
      for (int inj=0; inj < n_injections; ++inj) {

         std::string node_name = tokens[6*inj+0];

         int inj_node_idx=get_node_idx(node_name, model->NodeNameIdxMap());
 
         int type_id = atoi(tokens[6*inj+1].c_str());
         int species_id = atoi(tokens[6*inj+2].c_str());
         if ((injection_species_id != -1) && (injection_species_id != species_id)) {
            continue;
         }
         species_ids.insert(species_id);
         float inj_strength = atof(tokens[6*inj+3].c_str());

         if (inj_strength <= 0) {
            std::cerr << std::endl;
            std::cerr << "TSI ERROR: Injection strength must be a positive number" << std::endl;
            std::cerr << std::endl;
            throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
         }

         float inj_start_seconds = atof(tokens[6*inj+4].c_str());
         int inj_start_timesteps = Injection::SecondsToTimestep(inj_start_seconds, model->Stepsize_min());
         
         float inj_end_seconds = atof(tokens[6*inj+5].c_str());
         int inj_end_timesteps = Injection::SecondsToTimestep(inj_end_seconds, model->Stepsize_min());
         
         // check that the injection takes place within the simulation period
         if (!(inj_end_timesteps >= 0 && inj_end_timesteps < model->NSteps())) {
            std::cerr << std::endl;
            std::cerr << "TSI ERROR: Injection does not take place within simulation period" << std::endl;
            std::cerr << std::endl;
            throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
         }
         if (!(inj_start_timesteps >= 0 && inj_start_timesteps < model->NSteps())) {
            std::cerr << std::endl;
            std::cerr << "TSI ERROR: Injection does not take place within simulation period" << std::endl;
            std::cerr << std::endl;
            throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
         }
         if (!(inj_end_timesteps >= inj_start_timesteps)) {
            std::cerr << std::endl;
            std::cerr << "TSI ERROR: Injection stop time occurs before injection start time" << std::endl;
            std::cerr << std::endl;
            throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
         }

         PInjection tmp(new Injection);
         tmp->NodeName() = node_name;
         if (type_id == 0) { // EN_CONCEN
            std::cerr << std::endl;
            std::cerr << "TSI ERROR: Merlion does not support CONCEN type injections" << std::endl;
            std::cerr << std::endl;
            throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
         }
         else if (type_id == 1) { // EN_MASS
            tmp->Type() = InjType_Mass;
            tmp->Strength() = inj_strength*.001; //mg to g
         }
         else if (type_id == 2) { // EN_SETPOINT
               std::cerr << std::endl;
               std::cerr << "TSI ERROR: Merlion does not support SETPOINT type injections" << std::endl;
               std::cerr << std::endl;
               throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
         }
         else if (type_id == 3) { // EN_FLOWPACED
            tmp->Type() = InjType_Flow;
            tmp->Strength() = inj_strength;
         }
         else {
            std::cerr << std::endl;
            std::cerr << "TSI ERROR: Toxin injection type not recognized: " << type_id << std::endl;
            std::cerr << std::endl;
            throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
         }
         tmp->StartTimeSeconds() = inj_start_seconds;
         tmp->StopTimeSeconds() = inj_end_seconds;
         tmp_scenario->Injections().push_back(tmp);
      }
      injection_scenarios.push_back(tmp_scenario);
   }
   if ((injection_species_id == -1) && (species_ids.size() > 1)) {
      std::cerr << std::endl;
      std::cerr << "TSI ERROR: Multiple species ids were detected but Merlion only supports single species simulations" << std::endl;
      std::cerr << std::endl;
      throw std::runtime_error("Failed to read tsi file: "+tsi_filename);
   }
}
   
} /* end of merlionUtils namespace */

