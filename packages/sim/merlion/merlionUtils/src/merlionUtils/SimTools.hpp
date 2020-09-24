#ifndef MERLION_UTILS_SIM_TOOLS_HPP__
#define MERLION_UTILS_SIM_TOOLS_HPP__

#include <merlion/Merlion.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/Network.hpp>
#include <merlion/BlasWrapper.hpp>

#include <math.h>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <cmath>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

namespace merlionUtils {

//// Zero Tolerance constants
const float ZERO_CONC_GPM3 = 1e-30;
const float ZERO_FLOW_M3PMIN = 1e-30;
const float ZERO_VOLUME_M3 = 1e-30;

/* 
  Ideally, these two would be defined 
  using the above tolerances or with
  those tolerances used to run the origin
  tracking algorithm
*/
// defines what is zero for an entry in 
// the LHS water quality matrix
const float ZERO_WQM = 1e-30;
// defines what is zero for an entry in 
// the inverse of the LHS water quality matrix
  const float ZERO_WQM_INV = 1e-30; // -AS: Now being passed as a parameter to the GenerateReducedInverse function


//// Helper Functions    
inline int RoundToInt(float x)
{
   return static_cast<int>((x < 0.0f)?(x-0.5f):(x+0.5f));
}
inline int RoundToInt(double x) 
{
   return static_cast<int>((x < 0.0)?(x-0.5):(x+0.5));  
}
inline int SecondsToNearestTimestepBoundary(double seconds, double stepsize_minutes)
{
   return (seconds < 0)?(-1):(RoundToInt(seconds/60.0/stepsize_minutes));
}
inline int SecondsToOwningTimestep(double seconds, double stepsize_minutes)
{
   return (seconds < 0)?(-1):(floor(seconds/60.0/stepsize_minutes));
}

////

//// An external interface to the Merlion interface to protect the code if/when a redesign happens
/*
   Makes the interface to the merlion class a little cleaner.
   Eliminates function calls to access data members because
   speed is so critical. If anything, this should give a good
   idea on a better interface to merlion if/when a re-write is done.
*/
class MerlionModelContainer
{
public:

   MerlionModelContainer();
   ~MerlionModelContainer();

   void clear();
   void GetMerlionMembers(Merlion *merlionModel);

   Merlion *merlionModel;
   SparseMatrix *G;
   int *perm_nt_to_upper;
   int *perm_upper_to_nt;
   int *D;
   int N;
   int n_nodes;
   int n_links;
   int n_steps;
   int nnz;
   float *demand_m3pmin;
   // Optimization since this pointer is accessed
   // so often and we always need to permute to upper
   // triangular orientation
   float *demand_ut_m3;
   float *flow_m3pmin;
   std::vector<float> node_average_flow_magnitude_m3pmin;
   float *tank_volume_m3;
   float qual_step_minutes;
   int qual_steps_per_hour;
   std::vector<std::pair<int,int> > tanks;
   std::vector<int> junctions;
   std::vector<int> reservoirs;
   std::vector<int> nzd_junctions;
   
   int NodeID(std::string name) const {
      std::map<std::string,int>::const_iterator iter_node_id = node_name_to_id_.find(name);
      if (iter_node_id == node_name_to_id_.end()) {
	 std::cerr << std::endl;
	 std::cerr << "Merlion Error: Node name not found: \"" << name << "\"" << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      return iter_node_id->second;
   }

   bool isNode(std::string name) const {
      if (node_name_to_id_.find(name) == node_name_to_id_.end()) {
	 return false;
      }
      return true;
   }

   std::string NodeName(int id) const {
      return node_id_to_name_[id];
   }
   
   int LinkID(std::string name) const {
      std::map<std::string,int>::const_iterator iter_link_id = link_name_to_id_.find(name);
      if (iter_link_id == link_name_to_id_.end()) {
    std::cerr << std::endl;
    std::cerr << "Merlion Error: Link name not found: \"" << name << "\"" << std::endl;
    std::cerr << std::endl;
    exit(1);
      }
      return iter_link_id->second;
   }

   bool isLink(std::string name) const {
      if (link_name_to_id_.find(name) == link_name_to_id_.end()) {
    return false;
      }
      return true;
   }

   std::string LinkName(int id) const {
      return link_id_to_name_[id];
   }

   std::vector<int> NodeOutlets(int NodeID) const {
      std::map<int, std::vector<int> >::const_iterator iter = node_id_to_outlet_ids_.find(NodeID);
      if (iter == node_id_to_outlet_ids_.end()) {
      std::cerr << std::endl;
      std::cerr << "Merlion Error: Node id not found in node to outlet map: \"" << NodeID << "\"" << std::endl;
      std::cerr << std::endl;
      exit(1);
      }
      return iter->second;
   }
   std::vector<int> NodeInlets(int NodeID) const {
      std::map<int, std::vector<int> >::const_iterator iter = node_id_to_inlet_ids_.find(NodeID);
      if (iter == node_id_to_inlet_ids_.end()) {
      std::cerr << std::endl;
      std::cerr << "Merlion Error: Node id not found in node to outlet map: \"" << NodeID << "\"" << std::endl;
      std::cerr << std::endl;
      exit(1);
      }
      return iter->second;
   }

private:
   // providing const acces to a std::map is a real problem, so we provide a 
   // const safe function call in order to obtain node ids from node names
   std::map<std::string, int> node_name_to_id_;
   // and just to make things more formal we do the same for node name acces
   std::vector<std::string> node_id_to_name_;
   std::map<std::string, int> link_name_to_id_;
   std::vector<std::string> link_id_to_name_;
   std::map<int, std::vector<int> > node_id_to_outlet_ids_;
   std::map<int, std::vector<int> > node_id_to_inlet_ids_;

   void ClassifyNodes();
};
////

} /* end of merlionUtils namespace */


#endif
