#include <math.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#ifdef _WIN32
#include <float.h>
#define isnan(n) _isnan(n)
#define isinf(n) (!_finite(n))
#endif

#include <merlion/WaterQualityModel.hpp>
#include <merlion/DirectedGraph.hpp>

const float time_wt_tol = 0.01;
const float flow_tol_m3pmin = 1e-6;
const float tol = 1e-6;
//const float decay_k_ = 0.0f;

SparseWaterQualityModel::SparseWaterQualityModel(int n_links, int n_nodes, int n_steps)
   :
   n_links_(n_links),
   n_nodes_(n_nodes),
   n_steps_(n_steps),
   col_ci_ci_(n_links*n_steps),
   col_ci_co_(n_links*n_steps),
   col_ci_cn_(n_links*n_steps),
   A_ci_ci_(n_links*n_steps),
   A_ci_co_(n_links*n_steps),
   A_ci_cn_(n_links*n_steps),
   col_co_ci_(n_links*n_steps),
   col_co_co_(n_links*n_steps),
   col_co_cn_(n_links*n_steps),
   A_co_ci_(n_links*n_steps),
   A_co_co_(n_links*n_steps),
   A_co_cn_(n_links*n_steps),
   col_cn_ci_(n_nodes*n_steps),
   col_cn_co_(n_nodes*n_steps),
   //col_cn_cn_(n_nodes*n_steps),
   col_G_(n_nodes*n_steps),
   col_cn_mn_(n_nodes*n_steps),
   A_cn_ci_(n_nodes*n_steps),
   A_cn_co_(n_nodes*n_steps),
   //A_cn_cn_(n_nodes*n_steps),
   G_(n_nodes*n_steps),
   A_cn_mn_(n_nodes*n_steps)
{
}

SparseWaterQualityModel::~SparseWaterQualityModel()
{
}

void SparseWaterQualityModel::AddEntry(
   BlockID row_block,
   int row_item_idx,
   int row_t_idx,
   BlockID col_block,
   int col_item_idx,
   int col_t_idx,
   double value,
   const Network* net,
   bool second_add/*=false*/)
{
   assert(!isnan(value) && !isinf(value)); 
   assert(row_t_idx >= 0 && row_t_idx < n_steps_);
   assert(col_t_idx >= 0 && col_t_idx < n_steps_);
   assert(row_block != ID_Injection);

   if (row_block == ID_PipeInlet && col_block == ID_PipeInlet) {
      int row_idx = LinkTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = LinkTimeToIdx(col_item_idx, col_t_idx);
      col_ci_ci_[row_idx].push_back(col_idx);
      A_ci_ci_[row_idx].push_back(value);
   }
   else if (row_block == ID_PipeInlet && col_block == ID_PipeOutlet) {
      int row_idx = LinkTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = LinkTimeToIdx(col_item_idx, col_t_idx);
      col_ci_co_[row_idx].push_back(col_idx);
      A_ci_co_[row_idx].push_back(value);
   }
   else if (row_block == ID_PipeInlet && col_block == ID_Node) {
      int row_idx = LinkTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = NodeTimeToIdx(col_item_idx, col_t_idx);
      col_ci_cn_[row_idx].push_back(col_idx);
      A_ci_cn_[row_idx].push_back(value);
   }
   else if (row_block == ID_PipeOutlet && col_block == ID_PipeInlet) {
      int row_idx = LinkTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = LinkTimeToIdx(col_item_idx, col_t_idx);
      col_co_ci_[row_idx].push_back(col_idx);
      A_co_ci_[row_idx].push_back(value);
   }
   else if (row_block == ID_PipeOutlet && col_block == ID_PipeOutlet) {
      int row_idx = LinkTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = LinkTimeToIdx(col_item_idx, col_t_idx);
      col_co_co_[row_idx].push_back(col_idx);
      A_co_co_[row_idx].push_back(value);
   }
   else if (row_block == ID_PipeOutlet && col_block == ID_Node) {
      int row_idx = LinkTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = NodeTimeToIdx(col_item_idx, col_t_idx);
      col_co_cn_[row_idx].push_back(col_idx);
      A_co_cn_[row_idx].push_back(value);
   }
   else if (row_block == ID_Node && col_block == ID_PipeInlet) {
      int row_idx = NodeTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = LinkTimeToIdx(col_item_idx, col_t_idx);
      col_cn_ci_[row_idx].push_back(col_idx);
      A_cn_ci_[row_idx].push_back(value);

      int row_idx_1 = LinkTimeToIdx(col_item_idx, col_t_idx);
      for (int i=0; i<(int)col_ci_ci_[row_idx_1].size(); i++) {
         int row_idx_2 = col_ci_ci_[row_idx_1][i];
         if ((int)col_ci_ci_[row_idx_1].size() == 1 && (int)col_ci_co_[row_idx_1].size() == 0 && (int)col_ci_cn_[row_idx_1].size() == 0) {
            assert(A_ci_ci_[row_idx_1][i] == 1);
            break;
         }
         if (A_ci_ci_[row_idx_1][i] < 0) {
            assert((int)col_ci_ci_[row_idx_2].size() == 1 && (int)col_ci_co_[row_idx_2].size() == 0 && (int)col_ci_cn_[row_idx_2].size() == 1);
            assert(A_ci_cn_[row_idx_2][0] == -1);
            int new_col_item_idx;
            int new_col_t_idx;
            IdxToNodeTime(col_ci_cn_[row_idx_2][0], new_col_item_idx, new_col_t_idx);
            double new_value = value*-A_ci_ci_[row_idx_1][i];
            AddEntry(row_block, row_item_idx, row_t_idx, ID_Node, new_col_item_idx, new_col_t_idx, new_value, net, true);
         }
      }
      for (int i=0; i<(int)col_ci_co_[row_idx_1].size(); i++) {
         int row_idx_2 = col_ci_co_[row_idx_1][i];    
         if (A_ci_co_[row_idx_1][i] < 0) {
            assert((int)col_co_ci_[row_idx_2].size() == 0 && (int)col_co_co_[row_idx_2].size() == 1 && (int)col_co_cn_[row_idx_2].size() == 1);
            assert(A_co_cn_[row_idx_2][0] == -1);
            int new_col_item_idx;
            int new_col_t_idx;
            IdxToNodeTime(col_co_cn_[row_idx_2][0], new_col_item_idx, new_col_t_idx);
            double new_value = value*-A_ci_co_[row_idx_1][i];
            AddEntry(row_block, row_item_idx, row_t_idx, ID_Node, new_col_item_idx, new_col_t_idx, new_value, net, true);
         }
      }
   }
   else if (row_block == ID_Node && col_block == ID_PipeOutlet) {
      int row_idx = NodeTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = LinkTimeToIdx(col_item_idx, col_t_idx);
      col_cn_co_[row_idx].push_back(col_idx);
      A_cn_co_[row_idx].push_back(value);
   
      int row_idx_1 = LinkTimeToIdx(col_item_idx, col_t_idx);
      for (int i=0; i<(int)col_co_ci_[row_idx_1].size(); i++) {
         int row_idx_2 = col_co_ci_[row_idx_1][i];    
         if (A_co_ci_[row_idx_1][i] < 0) {
            assert((int)col_ci_ci_[row_idx_2].size() == 1 && (int)col_ci_co_[row_idx_2].size() == 0 && (int)col_ci_cn_[row_idx_2].size() == 1);
            assert(A_ci_cn_[row_idx_2][0] == -1);
            int new_col_item_idx;
            int new_col_t_idx;
            IdxToNodeTime(col_ci_cn_[row_idx_2][0], new_col_item_idx, new_col_t_idx);
            double new_value = value*-A_co_ci_[row_idx_1][i];
            AddEntry(row_block, row_item_idx, row_t_idx, ID_Node, new_col_item_idx, new_col_t_idx, new_value, net, true);
         }
      }
      for (int i=0; i<(int)col_co_co_[row_idx_1].size(); i++) {
         int row_idx_2 = col_co_co_[row_idx_1][i];
         if ((int)col_co_ci_[row_idx_1].size() == 0 && (int)col_co_co_[row_idx_1].size() == 1 && (int)col_co_cn_[row_idx_1].size() == 0) {
            assert(A_co_co_[row_idx_1][i] == 1);
            break;
         }
         if (A_co_co_[row_idx_1][i] < 0) {
            assert((int)col_co_ci_[row_idx_2].size() == 0 && (int)col_co_co_[row_idx_2].size() == 1 && (int)col_co_cn_[row_idx_2].size() == 1);
            assert(A_co_cn_[row_idx_2][0] == -1);
            int new_col_item_idx;
            int new_col_t_idx;
            IdxToNodeTime(col_co_cn_[row_idx_2][0], new_col_item_idx, new_col_t_idx);
            double new_value = value*-A_co_co_[row_idx_1][i];
            AddEntry(row_block, row_item_idx, row_t_idx, ID_Node, new_col_item_idx, new_col_t_idx, new_value, net, true);
         }
      }
   }
   else if (row_block == ID_Node && col_block == ID_Node) {
      int row_idx = NodeTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = NodeTimeToIdx(col_item_idx, col_t_idx);
      //if (!second_add) {
      //  col_cn_cn_[row_idx].push_back(col_idx);
      //  A_cn_cn_[row_idx].push_back(value);
      //}
   
      bool found = false;
      for (int i=0; i<(int)col_G_[row_idx].size(); i++) {
         if (col_G_[row_idx][i] == col_idx) {
            G_[row_idx][i] = G_[row_idx][i] + value;
            found = true;
            break;
         }
      }
      if (!found) {
         col_G_[row_idx].push_back(col_idx);
         G_[row_idx].push_back(value);
      } 
   }
   else if (row_block == ID_Node && col_block == ID_Injection) {
      int row_idx = NodeTimeToIdx(row_item_idx, row_t_idx);
      int col_idx = NodeTimeToIdx(col_item_idx, col_t_idx);
      col_cn_mn_[row_idx].push_back(col_idx);
      A_cn_mn_[row_idx].push_back(value);
      D_diagonal_.push_back(value);
   }
   else {
      std::cerr << std::endl;
      std::cerr << "WaterQualityModel ERROR: AddEntry failed." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
}

void SparseWaterQualityModel::CreateSparseMatrix(SparseMatrix*& G, int*& perm_nt_to_ut, int*& perm_ut_to_nt, int offset/*=0*/)
{
   int N = n_nodes_*n_steps_;
   // everything is in col_G_ and G_
   // first permute everything to time-node order
   // instead of node-time ordering - build the 
   // big coord matrix with the new ordering
   std::vector<int> permute_nt_to_tn(N,-1);
   std::vector<int> permute_tn_to_lt(n_nodes_*n_steps_,-1);
   int nnz = 0;
   int blocks_not_sorted(0);
   for (int t=0; t<n_steps_; t++) {
      DirectedGraph g(n_nodes_);
      for (int n=0; n<n_nodes_; n++) {
         int nt_row_idx = n*n_steps_ + t;
         int tn_row_idx = t*n_nodes_ + n;
         int nentries = (int)G_[nt_row_idx].size();
         nnz += nentries;
         for (int e=0; e<nentries; e++) {
            int col_nt_idx = col_G_[nt_row_idx][e];
            int col_node = col_nt_idx / n_steps_;
            int col_time = col_nt_idx % n_steps_;
            if (col_time == t && col_nt_idx != nt_row_idx) {
               g.AddEdge(n, col_node);
            }
         }
         permute_nt_to_tn[nt_row_idx] = tn_row_idx;
      }
      if (!g.PerformTopologicalSorting()) {blocks_not_sorted++;}
      for (int n=0; n<n_nodes_; n++) {
         permute_tn_to_lt[g.Permutation()[n]+t*n_nodes_] = t*n_nodes_+n;
      }
   }
   
   if (blocks_not_sorted) {
      std::cerr << std::endl;
      std::cerr << "WaterQualityModel ERROR: Topological sorting Failed." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   G = new SparseMatrix;
   G->AllocateNewCOOMatrix(N, N, nnz);
   perm_nt_to_ut = new int[N];
   perm_ut_to_nt = new int[N];

   int idx = 0;
   for (int t=0; t<n_steps_; t++) {
      for (int n=0; n<n_nodes_; n++) {
         int row_idx = n*n_steps_ + t;
         perm_nt_to_ut[row_idx] = N-1-permute_tn_to_lt[permute_nt_to_tn[row_idx]];
         perm_ut_to_nt[N-1-permute_tn_to_lt[permute_nt_to_tn[row_idx]]] = row_idx;
         int nentries = (int)G_[row_idx].size();
         for (int e=0; e<nentries; e++) {
            int col_idx = col_G_[row_idx][e];
            G->iRows()[idx] = N-1+offset-permute_tn_to_lt[permute_nt_to_tn[row_idx]];
            G->jCols()[idx] = N-1+offset-permute_tn_to_lt[permute_nt_to_tn[col_idx]];
            G->Values()[idx] = G_[row_idx][e];
            idx++;
         }
      }
   }

   // some checks
   //   for (int i=0; i<nnz; i++) {
   //     int cidx = G->jcols_[i];
   //     int ridx = G->irows_[i];
   //     assert(ridx <= cidx);
   //   }
}

WaterQualitySimulator::~WaterQualitySimulator()
{
}

SparseWaterQualityModel* WaterQualitySimulator::BuildPipeByPipeModel(
   const float* velocities_mpmin,
   const float* demands_m3pmin,
   float* tank_volumes_m3,
   float* flow_across_nodes_m3pmin)
{
   // create a sparse matrix for describing the pipe relationships
   const Link_vector& net_links = net_->LinksVector();
   const Node_vector& net_nodes = net_->NodesVector();
   int n_links = net_links.size();
   int n_nodes = net_nodes.size();
   SparseWaterQualityModel* model = new SparseWaterQualityModel(n_links, n_nodes, n_steps_);

   Link_vector::const_iterator link_i;
   for (link_i = net_links.begin(); link_i != net_links.end(); link_i++) {
      // Loop through all the timesteps and track packets as they move through the system
      for (int origin_time_idx=0; origin_time_idx<n_steps_; origin_time_idx++) {
         TrackPacket((*link_i), origin_time_idx, model, velocities_mpmin, demands_m3pmin);
      }
   }

   //
   // Now build the node balance equations
   //
   CalculateTankVolumes_m3(velocities_mpmin, demands_m3pmin, tank_volumes_m3);

   int tank_idx = -1;
   Node_vector::const_iterator node_i;
   for (node_i = net_nodes.begin(); node_i != net_nodes.end(); node_i++) {
      const Node* node = (*node_i);

      if (node->Type() == NodeType_Tank) {
         tank_idx++;
      }
   
      for (int time_idx=0; time_idx<n_steps_; time_idx++) {      
         double flow_out_m3pmin = FlowOut_m3pmin(node, time_idx, velocities_mpmin, demands_m3pmin);
         double flow_in_m3pmin = FlowIn_m3pmin(node, time_idx, velocities_mpmin, demands_m3pmin);
   
         int nodeflowidx = node->UniqueIDX()*n_steps_ + time_idx;
         flow_across_nodes_m3pmin[nodeflowidx] = (flow_out_m3pmin>flow_in_m3pmin ? flow_out_m3pmin : flow_in_m3pmin);
      
         // Add the injection term
         if (fabs(flow_out_m3pmin) >= flow_tol_m3pmin || node->Type() == NodeType_Tank) {
            model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_Injection, node->UniqueIDX(), time_idx, -1.0, net_);
         }
         else {
            model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_Injection, node->UniqueIDX(), time_idx, 0, net_);
         }
      
         Link_vector_const::const_iterator link_i;
         for (link_i = node->Inlets().begin(); link_i != node->Inlets().end(); link_i++) {
            int lidx = (*link_i)->UniqueIDX();
            int velidx = lidx*n_steps_+time_idx;
            double flow_m3pmin = velocities_mpmin[velidx]*(*link_i)->Area_m2();
            if (flow_m3pmin >= flow_tol_m3pmin) {
               model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_PipeOutlet, lidx, time_idx, -flow_m3pmin, net_);
            }
         }
      
         for (link_i = node->Outlets().begin(); link_i != node->Outlets().end(); link_i++) {
            int lidx = (*link_i)->UniqueIDX();
            int velidx = lidx*n_steps_+time_idx;
            double flow_m3pmin = velocities_mpmin[velidx]*(*link_i)->Area_m2();
            if (flow_m3pmin <= -flow_tol_m3pmin) {
               model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_PipeInlet, lidx, time_idx, flow_m3pmin, net_);
            }
         }
      
         if (node->Type() == NodeType_Junction || node->Type() == NodeType_Reservoir) { 
            // Add the C{n,t} term
            if (fabs(flow_out_m3pmin) < flow_tol_m3pmin) {
               model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_Node, node->UniqueIDX(), time_idx, 1.0, net_);
               if (time_idx > 0 && node->Type() == NodeType_Junction) {
                  model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_Node, node->UniqueIDX(), time_idx-1, -1.0, net_);  
               }
            }
            else {
               model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_Node, node->UniqueIDX(), time_idx, flow_out_m3pmin, net_);
            }
         }
      
         else {
            assert(node->Type() == NodeType_Tank);
            double volfac_m3pmin = tank_volumes_m3[tank_idx*n_steps_+time_idx]/qual_stepsize_min_;
            double vol_m3 = tank_volumes_m3[tank_idx*n_steps_+time_idx];
            model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_Node, node->UniqueIDX(), time_idx, volfac_m3pmin + flow_in_m3pmin + vol_m3*decay_k_, net_);
            if (time_idx > 0) {
               model->AddEntry(ID_Node, node->UniqueIDX(), time_idx, ID_Node, node->UniqueIDX(), time_idx-1, 
                  -volfac_m3pmin, net_);
            }
         }
      }
   }

   return model;
}

void WaterQualitySimulator::TrackPacket(
   const Link* link,
   int start_idx, SparseWaterQualityModel* model,
   const float* velocities_mpmin, 
   const float* demands_m3pmin)
{
   int l_idx = link->UniqueIDX();
   double pipe_length_m = link->Length_m();

//  set the identity portion of the model
//  the A^I_{PI} matrix and the A^O_{PO} matrix
   model->AddEntry(ID_PipeInlet, l_idx, start_idx, ID_PipeInlet, l_idx, start_idx, 1.0, net_);
   model->AddEntry(ID_PipeOutlet, l_idx, start_idx, ID_PipeOutlet, l_idx, start_idx, 1.0, net_);

   WaterPacket packet;

   int velidx = l_idx*n_steps_ + start_idx;
   double vel_mpmin = velocities_mpmin[velidx];
   double delx_m = vel_mpmin*qual_stepsize_min_;

   // At the origin time, the inlet (or outlet) is equal to the current node value
   // otherwise, start the packet moving...
   if (vel_mpmin==0) {
      // no packet enters at this time
      return;
   }
   else if (vel_mpmin > 0) {
      model->AddEntry(ID_PipeInlet, l_idx, start_idx, ID_Node, link->Inlet()->UniqueIDX(), start_idx, -1.0, net_);

      if (pipe_length_m == 0) {
         model->AddEntry(ID_PipeOutlet, l_idx, start_idx, ID_PipeInlet, l_idx, start_idx, -1.0, net_);
         return;
      }
   
      packet.xfront = delx_m;
      packet.xback = 0;
      packet.block_id = ID_PipeInlet;
   }
   else { // vel < 0
      model->AddEntry(ID_PipeOutlet, l_idx, start_idx, ID_Node, link->Outlet()->UniqueIDX(), start_idx, -1.0, net_);
   
      if (pipe_length_m == 0) {
         model->AddEntry(ID_PipeInlet, l_idx, start_idx, ID_PipeOutlet, l_idx, start_idx, -1.0, net_);
         return;
      }
   
      packet.xfront = pipe_length_m; 
      packet.xback = pipe_length_m + delx_m; // Note: delx_m is negative! 
      packet.block_id = ID_PipeOutlet;
   }

   assert(pipe_length_m != 0);

   // Add the necessary entries induced by the packet flow
   for (int time_idx=start_idx; time_idx<n_steps_; time_idx++) {
      if (time_idx > start_idx) {
         int velidx = l_idx*n_steps_ + time_idx;
         vel_mpmin = velocities_mpmin[velidx];
         delx_m = vel_mpmin*qual_stepsize_min_;
         packet.xfront += delx_m;
         packet.xback += delx_m;
      }

      bool mix_packets = true;
   
      if (mix_packets) {
         // add the necesssary packet entries
         if (packet.xfront > pipe_length_m) {
            assert(vel_mpmin > 0);
            // moved past the outlet node
            double time_before_packet = (pipe_length_m-(packet.xfront-delx_m))/vel_mpmin;
            double time_after_packet = (packet.xback-pipe_length_m)/vel_mpmin;
            double time_wt = (qual_stepsize_min_-time_before_packet-time_after_packet)/qual_stepsize_min_;
            if (time_after_packet < 0) {
               time_wt = (qual_stepsize_min_-time_before_packet)/qual_stepsize_min_;
            }
   
            if ( ! (time_wt > -tol && time_wt <= 1.0+tol) )
            {
               //accounting for float calculation
               std::cerr << "ERROR: time_wt out of valid bounds: " 
                         << time_wt << " not in (" << -tol << ", " 
                         << 1.0+tol << ")" << std::endl;
            }
            if (time_wt < 0) {
               time_wt = 0;
            }
            if (time_wt > 1) {
               time_wt = 1;
            }
         
            packet.xfront = pipe_length_m;
         
            if (time_wt>time_wt_tol) {
	      model->AddEntry(ID_PipeOutlet, l_idx, time_idx, packet.block_id, l_idx, start_idx, -time_wt*exp(-decay_k_*(time_idx-start_idx)*qual_stepsize_min_), net_);
            }
         
            if (time_after_packet >= -tol*qual_stepsize_min_) {
               return;
            }
         }
         else if (packet.xback < 0) {
            assert(vel_mpmin < 0);
            // moved past the inlet node
            double time_before_packet = -(packet.xback-delx_m)/vel_mpmin;
            double time_after_packet = (packet.xfront)/vel_mpmin;
            double time_wt = (qual_stepsize_min_-time_before_packet - time_after_packet)/qual_stepsize_min_;
            if (time_after_packet < 0) {
               time_wt = (qual_stepsize_min_-time_before_packet)/qual_stepsize_min_;
            }
         
            assert(time_wt > -tol && time_wt <= 1.0+tol);//accounting for float calculation
            if (time_wt < 0) {
               time_wt = 0;
            }
            if (time_wt > 1) {
               time_wt = 1;
            }
         
            packet.xback=0.0;
         
            if (time_wt>time_wt_tol) {
               model->AddEntry(ID_PipeInlet, l_idx, time_idx, packet.block_id, l_idx, start_idx, -time_wt*exp(-decay_k_*(time_idx-start_idx)*qual_stepsize_min_), net_);
            }
         
            if (time_after_packet >= -tol*qual_stepsize_min_) {
               return;
            }
         }
         else { 
         // do nothing, packet is in the middle of the pipe and not influencing a node right now
         }
      }
      else {//  don't mix_packets
         if (packet.xfront > pipe_length_m) {
            assert(vel_mpmin > 0);
            // moved past the outlet node
            if (packet.xback <= pipe_length_m) {
               model->AddEntry(ID_PipeOutlet, l_idx, time_idx, packet.block_id, l_idx, start_idx, -1.0*exp(-decay_k_*(time_idx-start_idx)*qual_stepsize_min_), net_);
            }
            packet.xfront = pipe_length_m;
            if (packet.xback >= pipe_length_m) {
               return;
            }
         }
         else if (packet.xback < 0) {
            assert(vel_mpmin < 0);
            // moved past the inlet node
            if (packet.xfront >= 0) {
               model->AddEntry(ID_PipeInlet, l_idx, time_idx, packet.block_id, l_idx, start_idx, -1.0*exp(-decay_k_*(time_idx-start_idx)*qual_stepsize_min_), net_);
            }
            packet.xback=0.0;
            if (packet.xfront <= 0) {
               return;
            }
         }
         else { 
         // do nothing, packet is in the middle of the pipe and not influencing a node right now
         }
      }
   }
}

double WaterQualitySimulator::FlowIn_m3pmin(
   const Node* node,
   int time_idx, 
   const float* velocities_mpmin, 
   const float* demands_m3pmin) const
{
   double total_inlet_flow_m3pmin = 0.0;
   Link_vector_const::const_iterator link_i;
   for (link_i = node->Inlets().begin(); link_i != node->Inlets().end(); link_i++) {
      int lidx = (*link_i)->UniqueIDX();
      int velidx = lidx*n_steps_+time_idx;
      double flow_m3pmin = velocities_mpmin[velidx]*(*link_i)->Area_m2();
      if (flow_m3pmin > 0) {
         // flow is into the node - therefore, add to total_inlet_flow_m3pmin
         total_inlet_flow_m3pmin += flow_m3pmin;
      }
   }

   for (link_i = node->Outlets().begin(); link_i != node->Outlets().end(); link_i++) {
      int lidx = (*link_i)->UniqueIDX();
      int velidx = lidx*n_steps_+time_idx;
      double flow_m3pmin = velocities_mpmin[velidx]*(*link_i)->Area_m2();
      if (flow_m3pmin < 0) {
         // flow is into the node - therefore, add to total_inlet_flow_m3pmin
         total_inlet_flow_m3pmin -= flow_m3pmin;
      }
   }

   int demidx = node->UniqueIDX()*n_steps_ + time_idx;
   double demand_m3pmin = demands_m3pmin[demidx];
   if (demand_m3pmin < 0) {
      total_inlet_flow_m3pmin -= demand_m3pmin;
   }

   return total_inlet_flow_m3pmin;
}

double WaterQualitySimulator::FlowOut_m3pmin(
   const Node* node,
   int time_idx,
   const float* velocities_mpmin, 
   const float* demands_m3pmin) const
{
   double total_outlet_flow_m3pmin = 0.0;
   Link_vector_const::const_iterator link_i;
   for (link_i = node->Inlets().begin(); link_i != node->Inlets().end(); link_i++) {
      int lidx = (*link_i)->UniqueIDX();
      int velidx = lidx*n_steps_+time_idx;
      double flow_m3pmin = velocities_mpmin[velidx]*(*link_i)->Area_m2();
      if (flow_m3pmin < 0) {
         // flow is going out the node - therefore, add to total_outlet_flow_m3pmin
         total_outlet_flow_m3pmin -= flow_m3pmin;
      }
   }

   for (link_i = node->Outlets().begin(); link_i != node->Outlets().end(); link_i++) {
      int lidx = (*link_i)->UniqueIDX();
      int velidx = lidx*n_steps_+time_idx;
      double flow_m3pmin = velocities_mpmin[velidx]*(*link_i)->Area_m2();
      if (flow_m3pmin > 0) {
         // flow is going out the node - therefore, add to total_outlet_flow_m3pmin
         total_outlet_flow_m3pmin += flow_m3pmin;
      }
   }

   int demidx = node->UniqueIDX()*n_steps_ + time_idx;
   double demand_m3pmin = demands_m3pmin[demidx];
   if (demand_m3pmin > 0) {
      total_outlet_flow_m3pmin += demand_m3pmin;
   }

   return total_outlet_flow_m3pmin;
}

void WaterQualitySimulator::CalculateTankVolumes_m3(
   const float* velocities_mpmin, 
   const float* demands_m3pmin,
   float* tank_volumes_m3) const
{
   const Node_vector& net_nodes = net_->NodesVector();
   int tank_idx = 0;
   for (int n=0; n<(int)net_nodes.size(); n++) {
      if (net_nodes[n]->Type() == NodeType_Tank) {
         tank_volumes_m3[tank_idx*n_steps_] = net_nodes[n]->InitialVolume_m3();
         for (int time_idx=1; time_idx<n_steps_; time_idx++) {
            double delvol_m3 = (FlowIn_m3pmin(net_nodes[n], time_idx, velocities_mpmin, demands_m3pmin)
               - FlowOut_m3pmin(net_nodes[n], time_idx, velocities_mpmin, demands_m3pmin))*(qual_stepsize_min_);
            int tank_time_idx = tank_idx*n_steps_ + time_idx -1;
            tank_volumes_m3[tank_time_idx+1] = tank_volumes_m3[tank_time_idx] + delvol_m3;        
         }
         tank_idx++;
      }
   }
}
