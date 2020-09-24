#ifndef MERLION_WATER_QUALITY_MODEL_HPP__
#define MERLION_WATER_QUALITY_MODEL_HPP__

#include <vector>
#include <cassert>

#include <merlion/Network.hpp>
#include <merlion/SparseMatrix.hpp>

enum BlockID
   {
      ID_PipeInlet,
      ID_PipeOutlet,
      ID_Node,
      ID_Injection
   };

class WaterPacket
{
public:
   double xfront;
   double xback;
   BlockID block_id;
};

class SparseWaterQualityModel
{
public:
   SparseWaterQualityModel(int n_links, int n_nodes, int n_steps);

   ~SparseWaterQualityModel();

   void AddEntry(
      BlockID row_block,
      int row_item_idx,
      int row_t_idx,
      BlockID col_block,
      int col_item_idx,
      int col_t_idx,
      double value,
      const Network* net,
      bool second_add=false);

   const std::vector<int>& Ddiagonal() const
   { return D_diagonal_; }

   void CreateSparseMatrix(SparseMatrix*& G, int*& perm_nt_to_ut, int*& perm_ut_to_nt, int offset=0);

   int NLinks() { return n_links_; }
   int NNodes() { return n_nodes_; }
   int NSteps() { return n_steps_; }

private:  
   int n_links_;
   int n_nodes_;
   int n_steps_;

   int NodeTimeToIdx(int node_idx, int time_idx)
   { return node_idx*n_steps_ + time_idx; }

   int LinkTimeToIdx(int link_idx, int time_idx)
   { return link_idx*n_steps_ + time_idx; }

   void IdxToNodeTime(int idx, int& node_idx, int& time_idx)
   { 
      node_idx = (int)idx/(int)n_steps_;
      time_idx = idx % n_steps_;
   }

   void IdxToLinkTime(int idx, int& link_idx, int& time_idx)
   {
      link_idx = (int)idx/(int)n_steps_;
      time_idx = idx % n_steps_;
   }

   // vector of column compressed indices and values
   // for pipe inlet conc. equations
   std::vector< std::vector<int> > col_ci_ci_;
   std::vector< std::vector<int> > col_ci_co_;
   std::vector< std::vector<int> > col_ci_cn_;
   std::vector< std::vector<double> > A_ci_ci_;
   std::vector< std::vector<double> > A_ci_co_;
   std::vector< std::vector<double> > A_ci_cn_;

   // vector of column compressed indices and values
   // for pipe outlet conc. equations
   std::vector< std::vector<int> > col_co_ci_;
   std::vector< std::vector<int> > col_co_co_;
   std::vector< std::vector<int> > col_co_cn_;
   std::vector< std::vector<double> > A_co_ci_;
   std::vector< std::vector<double> > A_co_co_;
   std::vector< std::vector<double> > A_co_cn_;

   // vector of column compressed indices and values
   // for node conc. equations
   std::vector< std::vector<int> > col_cn_ci_;
   std::vector< std::vector<int> > col_cn_co_;
   //std::vector< std::vector<int> > col_cn_cn_;
   std::vector< std::vector<int> > col_G_;
   std::vector< std::vector<int> > col_cn_mn_;
   std::vector< std::vector<double> > A_cn_ci_;
   std::vector< std::vector<double> > A_cn_co_;
   //std::vector< std::vector<double> > A_cn_cn_;
   std::vector< std::vector<double> > G_;
   std::vector< std::vector<double> > A_cn_mn_;

   // vector for storing D matrix diagonal values (Gc+Dm=0, no pipe equations)
   std::vector<int> D_diagonal_;
};

class WaterQualitySimulator
{
public:
  WaterQualitySimulator(Network* net, float qual_stepsize_min, int n_qual_steps, int qsteps_per_hstep, float decay_k = 0.0f)
      :
      net_(net),
      t0_(0.0),
      tF_(qual_stepsize_min_*n_qual_steps),
      n_steps_(n_qual_steps),
      qual_stepsize_min_(qual_stepsize_min),
      qsteps_per_hstep_(qsteps_per_hstep),
      decay_k_(decay_k)
   {
      //assert(t_initial==0);
      assert(qual_stepsize_min>0.0 && n_qual_steps>0.0);
      assert(qsteps_per_hstep>=1);
   }

   ~WaterQualitySimulator();

   SparseWaterQualityModel* BuildPipeByPipeModel(
      const float* velocities_mpmin,
      const float* demands_m3pmin,
      float* tank_volumes_m3,
      float* flow_across_nodes_m3pmin);

   int NQualSteps()
   { return n_steps_; }

   const double& QualStepsizeMin() const
   { return qual_stepsize_min_; }

   const int& QstepsPerHstep() const
   { return qsteps_per_hstep_; }
  
   float& Decay_Const()
   { return decay_k_; }

private:
   void TrackPacket(
      const Link* link,
      int start_idx,
      SparseWaterQualityModel* model,
      const float* velocities_mpmin, 
      const float* demands_m3pmin);

   double FlowIn_m3pmin(
      const Node* node,
      int time_idx, 
      const float* velocities_mpmin, 
      const float* demands_m3pmin) const;

   double FlowOut_m3pmin(
      const Node* node,
      int time_idx,
      const float* velocities_mpmin, 
      const float* demands_m3pmin) const;


   void CalculateTankVolumes_m3(
      const float* velocities_mpmin, 
      const float* demands_m3pmin,
      float* tank_volumes_m3) const;

   const Network* net_;
   const double t0_;
   const double tF_;
   int n_steps_;
   double qual_stepsize_min_;
   int qsteps_per_hstep_;
   float decay_k_;
};

#endif
