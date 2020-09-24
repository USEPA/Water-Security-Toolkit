#include <merlion/Merlion.hpp>
#include <merlion/Network.hpp>
#include <merlion/WaterQualityModel.hpp>
#include <merlion/SparseMatrix.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/BlasWrapper.hpp>

#include <fstream>
#include <cstdlib>
#include <map>
#include <iostream>

const float conc_tol = 1e-10;

Merlion::Merlion()
   :
   n_steps_(0),
   stepsize_min_(0.0),
   N_(0),
   G_(NULL),
   D_(NULL),
   permute_nt_to_ut_(NULL),
   permute_ut_to_nt_(NULL),
   link_velocities_mpmin_(NULL),
   node_demands_m3pmin_(NULL),
   tank_volumes_m3_(NULL),
   flow_across_nodes_m3pmin_(NULL),
   net_(NULL),
   sim_(NULL),
   model_(NULL)
{
   net_ = new Network("net");
}

Merlion::~Merlion()
{
   reset();
}

void Merlion::reset() {
   n_steps_ = 0;
   stepsize_min_ = 0.0;
   N_ = 0;
   delete G_;
   G_ = NULL;
   delete [] permute_nt_to_ut_;
   permute_nt_to_ut_ = NULL;
   delete [] permute_ut_to_nt_;
   permute_ut_to_nt_ = NULL;
   delete [] D_;
   D_ = NULL;
   delete [] link_velocities_mpmin_;
   link_velocities_mpmin_ =NULL;
   delete [] node_demands_m3pmin_;
   node_demands_m3pmin_ = NULL;
   delete [] tank_volumes_m3_;
   tank_volumes_m3_ = NULL;
   delete [] flow_across_nodes_m3pmin_;
   flow_across_nodes_m3pmin_ = NULL;
   clear(); 
}

void Merlion::clear()
{
   delete model_; // This needs to be investigated
   model_ = NULL;
   delete sim_;
   sim_ = NULL;
   delete net_;
   net_ = NULL;
 
}

void Merlion::AddJunction(std::string name)
{
   net_->AddNode(name, NodeType_Junction);
   node_idx_name_map_.push_back(name);
   node_idx_type_map_.push_back(NodeType_Junction);
   node_name_idx_map_[name] = node_idx_name_map_.size()-1;
}

void Merlion::AddReservoir(std::string name)
{
   net_->AddNode(name, NodeType_Reservoir);
   node_idx_name_map_.push_back(name);
   node_idx_type_map_.push_back(NodeType_Reservoir);
   node_name_idx_map_[name] = node_idx_name_map_.size()-1;
}

void Merlion::AddTank(std::string name, double init_volume_m3)
{
   net_->AddNode(name, NodeType_Tank, init_volume_m3);
   node_idx_name_map_.push_back(name);
   node_idx_type_map_.push_back(NodeType_Tank);
   node_name_idx_map_[name] = node_idx_name_map_.size()-1;
}

void Merlion::AddPipe(std::string name, std::string inlet_node_name, std::string outlet_node_name, double area_m2, double length_m)
{
   net_->AddLink(name, inlet_node_name, outlet_node_name, LinkType_Pipe, area_m2, length_m);
   link_idx_name_map_.push_back(name);
   link_idx_type_map_.push_back(LinkType_Pipe);
   link_name_idx_map_[name] = link_idx_name_map_.size()-1;
}

void Merlion::AddValve(std::string name, std::string inlet_node_name, std::string outlet_node_name, double area_m2)
{
   net_->AddLink(name, inlet_node_name, outlet_node_name, LinkType_Valve, area_m2);
   link_idx_name_map_.push_back(name);
   link_idx_type_map_.push_back(LinkType_Valve);
   link_name_idx_map_[name] = link_idx_name_map_.size()-1;
}

void Merlion::AddPump(std::string name, std::string inlet_node_name, std::string outlet_node_name)
{
   net_->AddLink(name, inlet_node_name, outlet_node_name, LinkType_Pump);
   link_idx_name_map_.push_back(name);
   link_idx_type_map_.push_back(LinkType_Pump);
   link_name_idx_map_[name] = link_idx_name_map_.size()-1;
}

void Merlion::BuildNetwork()
{
   net_->PushLinkConnectionsToNodes();
}

void Merlion::CreateNodeToLinkMaps()
{
   // Loop throuth all nodes 
   for (Node_vector::const_iterator node_i = net_->NodesVector().begin(); node_i != net_->NodesVector().end(); node_i++) 
   {  
      int nodeIdx = (*node_i)->UniqueIDX(); 
       // Get ids of all outlet links 
      if((*node_i)->Outlets().size() == 0) {
         node_id_to_outlet_ids_[nodeIdx].push_back(-1);
      }
      else{
         for (Link_vector_const::const_iterator link_i = (*node_i)->Outlets().begin(); link_i != (*node_i)->Outlets().end(); link_i++) {
            int lidx = (*link_i)->UniqueIDX();
            node_id_to_outlet_ids_[nodeIdx].push_back(lidx);
         }
      }
      if((*node_i)->Inlets().size() == 0) {
         node_id_to_inlet_ids_[nodeIdx].push_back(-1);
      }
      else{
         for (Link_vector_const::const_iterator link_i = (*node_i)->Inlets().begin(); link_i != (*node_i)->Inlets().end(); link_i++) {
            int lidx = (*link_i)->UniqueIDX();
            node_id_to_inlet_ids_[nodeIdx].push_back(lidx);
         }  
      }
   }
   
}

void Merlion::InitializeHydraulicMatrices(float duration_min, float stepsize_min)
{
   assert(n_steps_ == 0 && N_ == 0);
   n_steps_ = (int)(duration_min/stepsize_min)+1;
   stepsize_min_ = stepsize_min;
   N_ = n_steps_*node_idx_name_map_.size();

   assert(link_velocities_mpmin_ == NULL);
   assert(node_demands_m3pmin_ == NULL);
   assert(tank_volumes_m3_ == NULL);
   assert(flow_across_nodes_m3pmin_ == NULL);

   link_velocities_mpmin_ = new float[link_idx_name_map_.size()*n_steps_];
   BlasInitZero(link_idx_name_map_.size()*n_steps_, link_velocities_mpmin_);

   node_demands_m3pmin_ = new float[N_];
   BlasInitZero(N_, node_demands_m3pmin_);

   int n_tanks = 0;
   for (int i=0; i<(int)node_idx_type_map_.size(); i++) {
      if (node_idx_type_map_[i] == NodeType_Tank) {
         n_tanks++;
      }
   }
   tank_volumes_m3_ = new float[n_tanks*n_steps_];
   BlasInitZero(n_tanks*n_steps_, tank_volumes_m3_);

   flow_across_nodes_m3pmin_ = new float[N_];
   BlasInitZero(N_, flow_across_nodes_m3pmin_);
   
}

void Merlion::SetVelocity_mpmin(int link_idx, int stepidx, float velocity_mpmin)
{
   link_velocities_mpmin_[link_idx*n_steps_+stepidx] = velocity_mpmin;
}

void Merlion::SetDemand_m3pmin(int node_idx, int stepidx, float demand_m3pmin)
{
   node_demands_m3pmin_[node_idx*n_steps_+stepidx] = demand_m3pmin;
}

void Merlion::AddToVelocity_mpmin(int link_idx, int stepidx, float velocity_mpmin)
{
   link_velocities_mpmin_[link_idx*n_steps_+stepidx] += velocity_mpmin;
}

void Merlion::AddToDemand_m3pmin(int node_idx, int stepidx, float demand_m3pmin)
{
   node_demands_m3pmin_[node_idx*n_steps_+stepidx] += demand_m3pmin;
}

bool Merlion::BuildWQM(float decay_k_ )
{
  sim_ = new WaterQualitySimulator(net_, stepsize_min_, n_steps_, 1, decay_k_);

   model_ = sim_->BuildPipeByPipeModel(link_velocities_mpmin_, node_demands_m3pmin_, tank_volumes_m3_, flow_across_nodes_m3pmin_);

   model_->CreateSparseMatrix(G_, permute_nt_to_ut_, permute_ut_to_nt_);
   assert(N_ == G_->NRows());

   D_ = new int[N_];

   for (int i=0; i<N_; i++) {
      D_[i] = -1.0*model_->Ddiagonal()[i];
   }

   G_->TransformToCSCMatrix();

   return true;
}

float Merlion::PerformSingleInjectionSimulation(int node_index,
						int start_timestep,
						int end_timestep, 
						float* c_soln_gpm3,
						float mass_inj_gpmin/*=1.0*/) const
{
   // allocate the rhs (and soln) vector
   // note: soln is created in-place over top
   // of the inj vector
   float* m_c_soln_permuted = new float[N_];
   BlasInitZero(N_, m_c_soln_permuted);
   BlasResetZero(N_, c_soln_gpm3);
   
   assert(start_timestep >= 0 && start_timestep < n_steps_);
   assert(end_timestep >= start_timestep && end_timestep < n_steps_);
   
   float mass_injected_g = 0.0;
   
   // fill in the m injection vector
   for (int i=start_timestep; i<=end_timestep; i++) {
      int nt_idx = node_index*n_steps_ + i;
      int ut_idx = permute_nt_to_ut_[nt_idx];
      if (D_[nt_idx] != 0) {
         m_c_soln_permuted[ut_idx] = mass_inj_gpmin;
         mass_injected_g += mass_inj_gpmin*stepsize_min_;
      }
   }
   
   // perform the solve
   usolve(N_, 0, N_-1, G_->Values(), G_->iRows(), G_->pCols(), m_c_soln_permuted);  
   
   // fill in the solution vector in nt format
   for (int i=0; i<N_; i++) {
      if (m_c_soln_permuted[i] >= conc_tol) {
         c_soln_gpm3[permute_ut_to_nt_[i]] = m_c_soln_permuted[i];
      }
   }
   
   delete [] m_c_soln_permuted;
   
   return mass_injected_g;
}

float Merlion::SetInjection(int node_index,
			    int start_timestep,
			    int end_timestep,
			    float* X,
			    float mass_inj_gpmin/*=1.0*/) const
{  
   assert(start_timestep >= 0 && start_timestep < n_steps_);
   assert(end_timestep >= start_timestep && end_timestep < n_steps_);
   
   int node_index_times_n_steps = node_index*n_steps_;
   
   float mass_injected_g = 0.0;
   
   // fill in the X injection vector
   for (int i=start_timestep; i<=end_timestep; i++) {
      int nt_idx = node_index_times_n_steps + i;
      if (D_[nt_idx] != 0) {
         int ut_idx = permute_nt_to_ut_[nt_idx];
         X[ut_idx] = mass_inj_gpmin;
         mass_injected_g += mass_inj_gpmin*stepsize_min_;
      }
   }
   
   return mass_injected_g;
}

bool Merlion::PerformSimulation(float* X,
				float* X_soln/*=NULL*/,
				int min_timestep/*=0*/,
				int max_timestep/*=-1*/) const
{
   // TODO: min_timestep and max_timestep not used
   if (max_timestep<0){
      max_timestep = n_steps_-1;
   }
   
   assert(min_timestep >= 0 && min_timestep < n_steps_);
   assert(max_timestep >= min_timestep && max_timestep < n_steps_);
   
   int min_row = N_-node_idx_name_map_.size()*(max_timestep+1);
   int max_row = N_-node_idx_name_map_.size()*min_timestep-1;
   
   if (X_soln == NULL) {
      float* X_temp = new float[N_];
      cblas_scopy(N_, X, 1, X_temp, 1);
      BlasResetZero(N_, X);
      
      // perform the solve
      usolve(N_, min_row, max_row, G_->Values(), G_->iRows(), G_->pCols(), X_temp);  
      
      // fill in the solution vector in nt format
      for (int i=min_row; i<=max_row; i++) {
	 if (X_temp[i] >= conc_tol) {
	    X[permute_ut_to_nt_[i]] = X_temp[i];
	 }
      }
      
      delete [] X_temp;
   }
   else {
      // perform the solve
      usolve(N_, min_row, max_row, G_->Values(), G_->iRows(), G_->pCols(), X);  
      
      // fill in the solution vector in nt format
      for (int i=min_row; i<=max_row; i++) {
	 if (X[i] >= conc_tol) {
	    X_soln[permute_ut_to_nt_[i]] = X[i];
	 }
      }
   }
   
   return true;
}

void Merlion::PrintWQM(std::string fname, bool binary/*=true*/)
{
   std::ofstream out;
   if (binary) {out.open(fname.c_str(),std::ios::out | std::ios::trunc | std::ios::binary);}
   else {out.open(fname.c_str(),std::ios::out | std::ios::trunc);}
   out.precision(10);
   out.setf(std::ios::scientific,std::ios::floatfield);
   PrintWQM(out,binary);
}

void Merlion::PrintWQM(std::ostream& out, bool binary/*=true*/)
{
   if (binary) {
      // save the number of nodes
      std::vector<NodeType>::size_type n_nodes = node_idx_type_map_.size();
      out.write(reinterpret_cast<char*>(&n_nodes), sizeof(n_nodes));
      // save the list of node names
      int node_id = 0;
      int n_tanks = 0;
      for (std::vector<std::string>::iterator p = node_idx_name_map_.begin(), p_stop = node_idx_name_map_.end(); p != p_stop; ++p) {
	     std::string::size_type name_size = p->size();
	     out.write(reinterpret_cast<char*>(&name_size), sizeof(name_size));
	     out.write(const_cast<char*>(p->c_str()), sizeof(char)*name_size);
	     if (node_idx_type_map_[node_id] == NodeType_Tank) {n_tanks++;}
	     ++node_id;
      }
      // save the list of node types
      out.write(reinterpret_cast<char*>(&(node_idx_type_map_[0])),sizeof(NodeType)*node_idx_type_map_.size());
      // save the number of links
      std::vector<NodeType>::size_type n_links = link_idx_type_map_.size();
      out.write(reinterpret_cast<char*>(&n_links), sizeof(n_links));
      // save the list of link names
      for (std::vector<std::string>::iterator p = link_idx_name_map_.begin(), p_stop = link_idx_name_map_.end(); p != p_stop; ++p) {
	     std::string::size_type name_size = p->size();
	     out.write(reinterpret_cast<char*>(&name_size), sizeof(name_size));
	     out.write(const_cast<char*>(p->c_str()), sizeof(char)*name_size);
      }
      // save the list of link types
      out.write(reinterpret_cast<char*>(&(link_idx_type_map_[0])),sizeof(LinkType)*link_idx_type_map_.size());
      // save the number of timesteps
      out.write(reinterpret_cast<char*>(&n_steps_), sizeof(n_steps_));
      // save the number minutes per timestep
      out.write(reinterpret_cast<char*>(&stepsize_min_), sizeof(stepsize_min_));
      // save the merlion water quality matrix
      G_->PrintCSCMatrix(out,"Merlion",true);
      // save the permutation vectors
      out.write(reinterpret_cast<char*>(permute_nt_to_ut_), sizeof(permute_nt_to_ut_[0])*N_);
      out.write(reinterpret_cast<char*>(permute_ut_to_nt_), sizeof(permute_ut_to_nt_[0])*N_);
      // save the rhs diagonal matrix
      out.write(reinterpret_cast<char*>(D_), sizeof(D_[0])*N_);
      // save the list of link velocities
      out.write(reinterpret_cast<char*>(link_velocities_mpmin_), sizeof(link_velocities_mpmin_[0])*n_links*n_steps_);
      // save the list of node demands
      out.write(reinterpret_cast<char*>(node_demands_m3pmin_), sizeof(node_demands_m3pmin_[0])*n_nodes*n_steps_);
      // save the list of node flows
      out.write(reinterpret_cast<char*>(flow_across_nodes_m3pmin_), sizeof(flow_across_nodes_m3pmin_[0])*n_nodes*n_steps_);
      // save the list of tank volumes
      out.write(reinterpret_cast<char*>(tank_volumes_m3_), sizeof(tank_volumes_m3_[0])*n_tanks*n_steps_);
      //loop through node ids to outlet map
      for (std::map<int, std::vector<int> >::iterator i = node_id_to_outlet_ids_.begin(), i_stop = node_id_to_outlet_ids_.end(); i != i_stop; ++i) {
         int node_idx = i->first;  
         //save node id
         out.write(reinterpret_cast<char*>(&node_idx), sizeof(node_idx));
         // save the number of outlet links
         std::vector<int>::size_type n_outlets = node_id_to_outlet_ids_[node_idx].size();
         out.write(reinterpret_cast<char*>(&n_outlets), sizeof(n_outlets));
         // save the list of outlet link names
         for (std::vector<int>::iterator p = node_id_to_outlet_ids_[node_idx].begin(), p_stop = node_id_to_outlet_ids_[node_idx].end(); p != p_stop; ++p) {
            int link_id = *p;
            out.write(reinterpret_cast<char*>(&link_id), sizeof(link_id));
         }
      }
      // loop through node ids to inlet map
      for (std::map<int, std::vector<int> >::iterator i = node_id_to_outlet_ids_.begin(), i_stop = node_id_to_outlet_ids_.end(); i != i_stop; ++i) {
         int node_idx = i->first;  
         //save node id
         out.write(reinterpret_cast<char*>(&node_idx), sizeof(node_idx));
         // save the number of inlet links
         std::vector<int>::size_type n_inlets = node_id_to_inlet_ids_[node_idx].size();
         out.write(reinterpret_cast<char*>(&n_inlets), sizeof(n_inlets));
         // save the list of inlet link names
         for (std::vector<int>::iterator p = node_id_to_inlet_ids_[node_idx].begin(), p_stop = node_id_to_inlet_ids_[node_idx].end(); p != p_stop; ++p) {
            int link_id = *p;
            out.write(reinterpret_cast<char*>(&link_id), sizeof(link_id));
         }
      }


   }
   else {
      std::map<int, std::string> idx_node_enum_map;
      idx_node_enum_map[0] = "NodeType_Junction";
      idx_node_enum_map[1] = "NodeType_Tank";
      idx_node_enum_map[2] = "NodeType_Reservoir";
      
      std::map<int, std::string> idx_link_enum_map;
      idx_link_enum_map[0] = "LinkType_Valve";
      idx_link_enum_map[1] = "LinkType_Pump";
      idx_link_enum_map[2] = "LinkType_Pipe";
      
      out << "NETWORK_INFORMATION" << std::endl;
      
      int n_tanks = 0;
      out << node_idx_name_map_.size() << std::endl;
      for (int i=0; i<(int)node_idx_name_map_.size(); i++) {
	 out << node_idx_name_map_[i] << " " << idx_node_enum_map[node_idx_type_map_[i]] << std::endl;
	 if (node_idx_type_map_[i] == NodeType_Tank) {
	    n_tanks++;
	 }
      }
      
      out << link_idx_name_map_.size() << std::endl;
      for (int i=0; i<(int)link_idx_name_map_.size(); i++) {
	 out << link_idx_name_map_[i] << " " << idx_link_enum_map[link_idx_type_map_[i]] << std::endl;
      }
      
      out << "LINEAR_SYSTEM_INFORMATION" << std::endl;
      
      out << n_steps_ << std::endl;
      out << stepsize_min_ << std::endl;
      out << N_ << std::endl;
      
      G_->TransformToCOOMatrix();
      
      out << G_->NNonzeros() << std::endl;
      for (int i=0; i<G_->NNonzeros(); i++) {
	 out << G_->iRows()[i] << " " << G_->jCols()[i] << " " << G_->Values()[i] << std::endl;
      }
      
      G_->TransformToCSCMatrix();
      
      for (int i=0; i<N_; i++) {
	 out << permute_nt_to_ut_[i] << " " << permute_ut_to_nt_[i] << " " << D_[i] << std::endl;
      }
      
      out << "HYDRAULIC_INFORMATION" << std::endl;
      
      out << link_idx_name_map_.size()*n_steps_ << std::endl;
      for (int i=0; i<(int)link_idx_name_map_.size()*n_steps_; i++) {
	 out << link_velocities_mpmin_[i] << std::endl;
      }
      
      out << node_idx_name_map_.size()*n_steps_ << std::endl;
      for (int i=0; i<(int)node_idx_name_map_.size()*n_steps_; i++) {
	 out << node_demands_m3pmin_[i] << " " << flow_across_nodes_m3pmin_[i] << std::endl;
      }
      
      out << n_tanks*n_steps_ << std::endl;
      for (int i=0; i<n_tanks*n_steps_; i++) {
	 out << tank_volumes_m3_[i] << std::endl;
      }
   }
}

void Merlion::ReadWQM(std::string fname, bool binary/*=true*/)
{
   std::ifstream in;
   if (binary) {in.open(fname.c_str(),std::ios::in | std::ios::binary);}
   else {in.open(fname.c_str(),std::ios::in);}
   ReadWQM(in,binary);
}

void Merlion::ReadWQM(std::istream& in, bool binary/*=false*/)
{
   reset();
   if (binary) {
      // read the number of nodes 
      std::vector<NodeType>::size_type n_nodes = 0;
      in.read(reinterpret_cast<char*>(&n_nodes), sizeof(n_nodes));
      node_idx_name_map_.resize(n_nodes);
      // read the list of node names
      int node_id = 0;
      for (std::vector<std::string>::iterator p = node_idx_name_map_.begin(), p_stop = node_idx_name_map_.end(); p != p_stop; ++p) {
	     std::string::size_type name_size = p->size();
	     in.read(reinterpret_cast<char*>(&name_size), sizeof(name_size));
	     p->resize(name_size);
	     in.read(const_cast<char*>(p->c_str()), sizeof(char)*name_size);
	     node_name_idx_map_[*p] = node_id++;
      }
      // read the list of node types
      node_idx_type_map_.resize(n_nodes);
      in.read(reinterpret_cast<char*>(&(node_idx_type_map_[0])),sizeof(NodeType)*node_idx_type_map_.size());
      int n_tanks = 0;
      for (std::vector<NodeType>::iterator p = node_idx_type_map_.begin(), p_stop = node_idx_type_map_.end(); p != p_stop; ++p) {
	     if ((*p) == NodeType_Tank) {n_tanks++;}
      }
      // read the number of links
      std::vector<NodeType>::size_type n_links = link_idx_type_map_.size();
      in.read(reinterpret_cast<char*>(&n_links), sizeof(n_links));
      link_idx_name_map_.resize(n_links);
      // read the list of link names
      for (std::vector<std::string>::iterator p = link_idx_name_map_.begin(), p_stop = link_idx_name_map_.end(); p != p_stop; ++p) {
	     std::string::size_type name_size = p->size();
	     in.read(reinterpret_cast<char*>(&name_size), sizeof(name_size));
	     p->resize(name_size);
	     in.read(const_cast<char*>(p->c_str()), sizeof(char)*name_size);
      }
      // read the list of link types
      link_idx_type_map_.resize(n_links);
      in.read(reinterpret_cast<char*>(&(link_idx_type_map_[0])),sizeof(LinkType)*link_idx_type_map_.size());
      // read the number of timesteps
      in.read(reinterpret_cast<char*>(&n_steps_), sizeof(n_steps_));
      // read the number of minutes per timesteps
      in.read(reinterpret_cast<char*>(&stepsize_min_), sizeof(stepsize_min_));
      // read the merlion water quality matrix
      G_ = new SparseMatrix;
      G_->ReadCSCMatrix(in,true);
      N_ = n_nodes*n_steps_;
      assert(N_ == G_->NRows());
      assert(N_ == G_->NCols());
            
      // read the permutation vectors
      permute_nt_to_ut_ = new int[N_];
      permute_ut_to_nt_ = new int[N_];
      in.read(reinterpret_cast<char*>(permute_nt_to_ut_), sizeof(permute_nt_to_ut_[0])*N_);
      in.read(reinterpret_cast<char*>(permute_ut_to_nt_), sizeof(permute_ut_to_nt_[0])*N_);
      // read the rhs diagonal matrix
      D_ = new int[N_];
      in.read(reinterpret_cast<char*>(D_), sizeof(D_[0])*N_);
      // read the list of link velocities
      link_velocities_mpmin_ = new float[link_idx_name_map_.size()*n_steps_];
      in.read(reinterpret_cast<char*>(link_velocities_mpmin_), sizeof(link_velocities_mpmin_[0])*n_links*n_steps_);
      // read the list of node demands
      node_demands_m3pmin_ = new float[node_idx_name_map_.size()*n_steps_];
      in.read(reinterpret_cast<char*>(node_demands_m3pmin_), sizeof(node_demands_m3pmin_[0])*n_nodes*n_steps_);
      // read the list of node flows
      flow_across_nodes_m3pmin_ = new float[node_idx_name_map_.size()*n_steps_];
      in.read(reinterpret_cast<char*>(flow_across_nodes_m3pmin_), sizeof(flow_across_nodes_m3pmin_[0])*n_nodes*n_steps_);
      // read the list of tank volumes
      tank_volumes_m3_ = new float[n_tanks*n_steps_];
      in.read(reinterpret_cast<char*>(tank_volumes_m3_), sizeof(tank_volumes_m3_[0])*n_tanks*n_steps_);
       
       //loop through node ids to outlet map
      for (int i=0; i<n_nodes; ++i) {
         int node_idx = 0;  
         //read node id
         in.read(reinterpret_cast<char*>(&node_idx), sizeof(node_idx));
         // read the number of outlet links
         std::vector<int>::size_type n_outlets = 0;
         in.read(reinterpret_cast<char*>(&n_outlets), sizeof(n_outlets));
         node_id_to_outlet_ids_[node_idx].resize(n_outlets);
         // save the list of outlet link names
         for (std::vector<int>::iterator p = node_id_to_outlet_ids_[node_idx].begin(), p_stop = node_id_to_outlet_ids_[node_idx].end(); p != p_stop; ++p) {
            int link_id = 0;
            in.read(reinterpret_cast<char*>(&link_id), sizeof(link_id));
            *p = link_id;
         }
      }

       //loop through node ids to inlet map
      for (int i=0; i<n_nodes; ++i) {
         int node_idx = 0;  
         //read node id
         in.read(reinterpret_cast<char*>(&node_idx), sizeof(node_idx));
         // read the number of inlet links
         std::vector<int>::size_type n_inlets = 0;
         in.read(reinterpret_cast<char*>(&n_inlets), sizeof(n_inlets));
         node_id_to_inlet_ids_[node_idx].resize(n_inlets);
         // save the list of inlet link names
         for (std::vector<int>::iterator p = node_id_to_inlet_ids_[node_idx].begin(), p_stop = node_id_to_inlet_ids_[node_idx].end(); p != p_stop; ++p) {
            int link_id = 0;
            in.read(reinterpret_cast<char*>(&link_id), sizeof(link_id));
            *p = link_id;
         }
      }
               


   }
   else {
      std::map<std::string, NodeType> node_type_enum_map;
      node_type_enum_map["NodeType_Junction"] = NodeType_Junction;
      node_type_enum_map["NodeType_Tank"] = NodeType_Tank;
      node_type_enum_map["NodeType_Reservoir"] = NodeType_Reservoir;
      
      std::map<std::string, LinkType> link_type_enum_map;
      link_type_enum_map["LinkType_Valve"] = LinkType_Valve;
      link_type_enum_map["LinkType_Pump"] = LinkType_Pump;
      link_type_enum_map["LinkType_Pipe"] = LinkType_Pipe;
      
      std::string temp_str;
      in >> temp_str;
      if (temp_str != "NETWORK_INFORMATION") {
         std::cerr << std::endl;
	 std::cerr << "Merlion ERROR: Failed to read Merlion water quality model file." << std::endl;
         std::cerr << std::endl;
         exit(1);
      }  
      
      int n_nodes;
      in >> n_nodes;
      for (int i=0; i<n_nodes; i++) {
	 std::string node_name;
	 std::string node_type;
	 in >> node_name;
	 in >> node_type;
	 node_name_idx_map_[node_name] = i;
	 node_idx_name_map_.push_back(node_name);
	 node_idx_type_map_.push_back(node_type_enum_map[node_type]);
      }
      
      int n_links;
      in >> n_links;
      for (int i=0; i<n_links; i++) {
	 std::string link_name;
	 std::string link_type;
	 in >> link_name;
	 in >> link_type;
	 link_name_idx_map_[link_name] = i;
	 link_idx_name_map_.push_back(link_name);
	 link_idx_type_map_.push_back(link_type_enum_map[link_type]);
      }
      
      in >> temp_str;
      if (temp_str != "LINEAR_SYSTEM_INFORMATION") {
         std::cerr << std::endl;
	 std::cerr << "Merlion ERROR: Failed to read Merlion water quality model file." << std::endl;
         std::cerr << std::endl;
         exit(1);
      }  
      
      in >> n_steps_;
      in >> stepsize_min_;
      in >> N_;
      assert(N_ == n_steps_*n_nodes);
      
      int nnz;
      in >> nnz;
      
      G_ = new SparseMatrix;
      G_->AllocateNewCOOMatrix(N_, N_, nnz);
      D_ = new int[N_];
      
      for (int i=0; i<nnz; i++) {
	 in >> G_->iRows()[i];
	 in >> G_->jCols()[i];
	 in >> G_->Values()[i];
      }
      
      permute_nt_to_ut_ = new int[N_];
      permute_ut_to_nt_ = new int[N_];
      for (int i=0; i<N_; i++) {
	 in >> permute_nt_to_ut_[i];
	 in >> permute_ut_to_nt_[i];
	 in >> D_[i];
      }
      
      G_->TransformToCSCMatrix();
      
      in >> temp_str;
      if (temp_str != "HYDRAULIC_INFORMATION") {
         std::cerr << std::endl;
	 std::cerr << "Merlion ERROR: Failed to read Merlion water quality model file." << std::endl;
         std::cerr << std::endl;
         exit(1);
      }  
      
      int temp_int;
      in >> temp_int;
      assert(temp_int = n_links*n_steps_);
      
      link_velocities_mpmin_ = new float[link_idx_name_map_.size()*n_steps_];
      for (int i=0; i<(int)link_idx_name_map_.size()*n_steps_; i++) {
	 in >> link_velocities_mpmin_[i];
      }
      
      in >> temp_int;
      assert(temp_int = n_nodes*n_steps_);
      
      node_demands_m3pmin_ = new float[node_idx_name_map_.size()*n_steps_];
      flow_across_nodes_m3pmin_ = new float[node_idx_name_map_.size()*n_steps_];
      for (int i=0; i<(int)node_idx_name_map_.size()*n_steps_; i++) {
	 in >> node_demands_m3pmin_[i];
	 in >> flow_across_nodes_m3pmin_[i];
      }
      
      in >> temp_int;
      tank_volumes_m3_ = new float[temp_int];
      for (int i=0; i<temp_int; i++) {
	 in >> tank_volumes_m3_[i];
      }
   }
}
