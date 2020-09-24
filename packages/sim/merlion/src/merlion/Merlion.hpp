#ifndef MERLION_MERLION_HPP__
#define MERLION_MERLION_HPP__

#include <merlion/MerlionDefines.hpp>
#include <merlion/SparseMatrix.hpp>

#include <string>
#include <vector>
#include <map>

// forward declarations 
class Network;
class WaterQualitySimulator;
class SparseWaterQualityModel;

class Merlion
{
public:
   Merlion();

   ~Merlion();

   // clear net_, sim_, and model_
   void clear();
   // clear all data members
   void reset();

   // Methods to build the network model
   // Methods to add nodes
   void AddJunction(std::string name);
   void AddReservoir(std::string name);
   void AddTank(std::string name, double init_volume_m3);

   // Methods to add links - Note: ALL NODES MUST BE ADDED FIRST PRIOR TO ADDING LINKS
   void AddPipe(std::string name, std::string inlet_node_name, std::string outlet_node_name, double area_m2, double length_m);
   void AddValve(std::string name, std::string inlet_node_name, std::string outlet_node_name, double area_m2);
   void AddPump(std::string name, std::string inlet_node_name, std::string outlet_node_name);

   // Methods to return name/idx mapping for nodes and links
   int NNodes() const { return node_idx_name_map_.size(); }
   const std::vector<std::string>&  NodeIdxNameMap() const { return node_idx_name_map_; }
   const std::vector<NodeType>& NodeIdxTypeMap() const { return node_idx_type_map_; }
   const std::map<std::string, int>& NodeNameIdxMap() const { return node_name_idx_map_; }

   const std::vector<std::string>& LinkIdxNameMap() const { return link_idx_name_map_; }
   const std::vector<LinkType>& LinkIdxTypeMap() const { return link_idx_type_map_; }
   const std::map<std::string, int>& LinkNameIdxMap() const { return link_name_idx_map_; }

   // Methods to return node to link id maps 
   const std::map<int, std::vector<int> >& NodeIdxOutletIdxMap() const { return node_id_to_outlet_ids_; }
   const std::map<int, std::vector<int> >& NodeIdxInletIdxMap() const { return node_id_to_inlet_ids_; }

   // Execute after finishing with node and link additions
   void BuildNetwork();
   
   // Creates maps containing inlet and outlet links for each node
   void CreateNodeToLinkMaps();

   // Call this method before setting hydraulic information
   void InitializeHydraulicMatrices(float duration_min, float stepsize_min);
   const int& NSteps() const { return n_steps_; }
   const float& Stepsize_min() const { return stepsize_min_; }

   void SetVelocity_mpmin(int link_idx, int stepidx, float velocity_mpmin);
   void SetDemand_m3pmin(int node_idx, int stepidx, float demand_m3pmin);
   void AddToVelocity_mpmin(int link_idx, int stepidx, float velocity_mpmin);
   void AddToDemand_m3pmin(int node_idx, int stepidx, float demand_m3pmin);

   // Methods to build water quality model and perform simulations
   bool BuildWQM(float decay_k = -1.0f);

   // perform a single simulation from a single node. The solution vector c_soln must be 
   // of length (number of nodes)*(number of qual timesteps)
   // the order of the solution is as follows:
   //   c_soln_gpm3[i] is the concentration of i = node_idx*n_qual_steps + time_idx
   // return total mass injected in grams
   // Note: This method is inefficient and only for testing purposes
   float PerformSingleInjectionSimulation(int node_index, int start_timestep, int end_timestep, 
      float* c_soln_gpm3, float mass_inj_gpmin=1.0) const;

   // Set and/or add an injection profile in vector X. X must be zeroed and of length 
   //   (number of nodes)*(number of qual timesteps)
   // Returns total mass injected in grams for the injection specified
   float SetInjection(int node_index, int start_timestep, int end_timestep,
      float* X, float mass_inj_gpmin=1.0) const;

   // The zeroed vector X_soln of the same size as X is an optional argument. Otherwise, solution
   // is created in-place over top of the inj vector X. The order of X is as follows:
   //   X[i] is the concentration of i = node_idx*n_qual_steps + time_idx
   bool PerformSimulation(float* X, float* X_soln=NULL, int min_timestep=0, int max_timestep=-1) const;

   /*
   //indexes start at zero, mass_rate_profile_mgpmin is size n_steps, node_indexes are determined based on the order nodes are added
   void SetInjection(float* X, int n_scenario, int scenario_index, int node_index, int timestep_index, float mass_rate_mgpmin);
   void SetInjection(float* X, int n_scenario, int scenario_index, std::string node_name, int timestep_index, float mass_rate_mgpmin);
   void SetNodeInjectionProfile(float* X, int n_scenario, int scenario_index, int node_index, float* mass_rate_profile_mgpmin);
   void SetNodeInjectionProfile(float* X, int n_scenario, int scenario_index, std::string node_name, float* mass_rate_profile_mgpmin);
   void SetInjectionProfile(float* X, int n_scenario, int n_scenarios_added, int* scenario_indexes, int* node_indexes, float* mass_rate_profile_mgpmin);

   void Simulate(float* X, int n_scenarios, int min_timestep=0, int max_timestep=-1); //optional arguments
   */

   // Methods to access some internal data
   // indexed as i_link*n_steps_ + t
   const float* GetVelocities_mpmin() const { return link_velocities_mpmin_; }
   // indexed as i_node*n_steps_ + t
   const float* GetNodeDemands_m3pmin() const { return node_demands_m3pmin_; }
   // indexed as i_tank*n_steps_ + t
   const float* GetTankVolumes_m3() const { return tank_volumes_m3_; }
   // indexed as i_node*n_steps_ + t
   const float* GetFlowAcrossNodes_m3pmin() const { return flow_across_nodes_m3pmin_; }

   //Functions to access Merlion objects
   const SparseMatrix* GetLHSMatrix() const {return G_;}
   const int* GetRHSDiagMatrix() const { return D_; }
   const int* GetPermuteNTtoUT() const {return permute_nt_to_ut_;}
   const int* GetPermuteUTtoNT() const {return permute_ut_to_nt_;}

   // Methods to write the merlion class to a text file and read it back in
   void PrintWQM(std::ostream& out, bool binary=true);
   void PrintWQM(std::string fname, bool binary=true);
   void ReadWQM(std::istream& in, bool binary=true);
   void ReadWQM(std::string fname, bool binary=true);

   //Functions to access network object
   const Network* GetNetwork() const {return net_;}

private:
   // maps to convert back and forth from names
   // to idxs, etc.
   std::vector<std::string> node_idx_name_map_;
   std::vector<NodeType> node_idx_type_map_;
   std::map<std::string, int> node_name_idx_map_;

   std::vector<std::string> link_idx_name_map_;
   std::vector<LinkType> link_idx_type_map_;
   std::map<std::string, int> link_name_idx_map_;

   // maps to get inlet and outlet link ids for all nodes
   std::map<int, std::vector<int> > node_id_to_outlet_ids_;
   std::map<int, std::vector<int> > node_id_to_inlet_ids_;

   // number of timesteps
   int n_steps_;

   // size of timestep
   float stepsize_min_;

   // N_ = n_nodes_ * n_steps_
   // this is provided as convenience since it is used so often
   int N_;

   // matrices that form the water quality model
   SparseMatrix* G_;
   int* D_;
   // permutation vectors that go with the sparse matrix G
   int *permute_nt_to_ut_;
   int *permute_ut_to_nt_;

   // variables to store the hydraulic information
   float* link_velocities_mpmin_;       // indexed as i_link*n_steps_ + t
   float* node_demands_m3pmin_;         // indexed as i_node*n_steps_ + t
   float* tank_volumes_m3_;             // indexed as i_tank*n_steps_ + t
   float* flow_across_nodes_m3pmin_;    // indexed as i_node*n_steps_ + t

   Network* net_;
   WaterQualitySimulator* sim_;
   SparseWaterQualityModel* model_;
};

#endif
