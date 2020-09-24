#include <merlionUtils/SimTools.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/SparseMatrix.hpp>

namespace merlionUtils {

MerlionModelContainer::MerlionModelContainer()
   :
   merlionModel(NULL),
   G(NULL),
   perm_nt_to_upper(NULL),
   perm_upper_to_nt(NULL),
   D(NULL),
   N(0),
   n_nodes(0),
   n_links(0),
   n_steps(0),
   nnz(0),
   demand_m3pmin(NULL),
   demand_ut_m3(NULL),
   flow_m3pmin(NULL),
   tank_volume_m3(NULL),
   qual_step_minutes(0.0f),
   qual_steps_per_hour(0)
{
}

MerlionModelContainer::~MerlionModelContainer()
{
   clear();
}

void MerlionModelContainer::clear()
{
   delete merlionModel;
   merlionModel = NULL;
   // These variables were simply pointing to what
   // was on the merlionModel
   G = NULL;
   perm_nt_to_upper = NULL;
   perm_upper_to_nt = NULL;
   D = NULL;
   demand_m3pmin = NULL;
   flow_m3pmin = NULL;
   node_average_flow_magnitude_m3pmin.clear();
   tank_volume_m3 = NULL;
   //
   delete [] demand_ut_m3;
   demand_ut_m3 = NULL;
   node_name_to_id_.clear();
   node_id_to_name_.clear();
   tanks.clear();
   junctions.clear();
   reservoirs.clear();
   nzd_junctions.clear();
   node_id_to_outlet_ids_.clear();
   node_id_to_inlet_ids_.clear();
   N = 0;
   n_nodes = 0;
   n_links = 0;
   n_steps = 0;
   nnz = 0;
   qual_step_minutes = 0.0;
   qual_steps_per_hour = 0;
}

void MerlionModelContainer::ClassifyNodes()
{
   tanks.clear();
   junctions.clear();
   reservoirs.clear();
   nzd_junctions.clear();
   int tank_n = 0;
   for (int n = 0; n < n_nodes; ++n) {
      NodeType ntype = merlionModel->NodeIdxTypeMap()[n];
      if (ntype == NodeType_Junction) {
         junctions.push_back(n);
         for (int t = 0; t < n_steps; ++t) {
            if (demand_m3pmin[n*n_steps+t] > ZERO_FLOW_M3PMIN) {
               nzd_junctions.push_back(n);
               break;
            }
         }
      }
      else if (ntype == NodeType_Tank) {
         tanks.push_back(std::pair<int,int>(n,tank_n));
         tank_n++;
      }
      else if (ntype == NodeType_Reservoir) {
         reservoirs.push_back(n);
      }
   }
}

void MerlionModelContainer::GetMerlionMembers(Merlion *merModel)
{  
   if (merModel != NULL) {
      merlionModel = merModel;
      N = merlionModel->NSteps()*merlionModel->NNodes();
      n_steps = merlionModel->NSteps();
      n_nodes = merlionModel->NNodes();
      G = const_cast<SparseMatrix*>(merlionModel->GetLHSMatrix());
      nnz = G->NNonzeros();
      D = const_cast<int*>(merlionModel->GetRHSDiagMatrix());
      perm_upper_to_nt = const_cast<int*>(merlionModel->GetPermuteUTtoNT());
      perm_nt_to_upper = const_cast<int*>(merlionModel->GetPermuteNTtoUT());
      demand_m3pmin = const_cast<float*>(merlionModel->GetNodeDemands_m3pmin()); 
      flow_m3pmin = const_cast<float*>(merlionModel->GetFlowAcrossNodes_m3pmin());
      tank_volume_m3 = const_cast<float*>(merlionModel->GetTankVolumes_m3());
      node_name_to_id_ = merlionModel->NodeNameIdxMap(); // this is a copy  
      node_id_to_name_ = merlionModel->NodeIdxNameMap(); // this is a copy
      link_name_to_id_ = merlionModel->LinkNameIdxMap(); // this is a copy  
      link_id_to_name_ = merlionModel->LinkIdxNameMap(); // this is a copy
      qual_step_minutes = merlionModel->Stepsize_min();
      node_id_to_outlet_ids_ = merlionModel->NodeIdxOutletIdxMap(); // this is a copy 
      node_id_to_inlet_ids_ = merlionModel->NodeIdxInletIdxMap(); // this is a copy
      // determine the number of quality steps per hour
      // being careful the cast to int does not eliminate 
      // a time step
      qual_steps_per_hour = RoundToInt(60.0f/qual_step_minutes);
      demand_ut_m3 = new float[N];
      for (int ut_idx = 0; ut_idx < N; ++ut_idx) {
         demand_ut_m3[ut_idx] = demand_m3pmin[perm_upper_to_nt[ut_idx]]*qual_step_minutes; 
      }
      node_average_flow_magnitude_m3pmin.clear();
      node_average_flow_magnitude_m3pmin.resize(n_nodes,0.0);
      for (int n = 0; n < n_nodes; ++n) {
         float average_flow_magnitude_m3pmin = 0.0;
         int cnt = 0;
         for (int t = 0; t < n_steps; ++t) {
            int nt_idx = n*n_steps + t;
            int u_idx = perm_nt_to_upper[nt_idx];
            if (D[nt_idx]) {
               cnt += 1;
               average_flow_magnitude_m3pmin += fabs(flow_m3pmin[nt_idx]);
            }
         }
         if ((cnt == 0) || (average_flow_magnitude_m3pmin == 0)) {
            node_average_flow_magnitude_m3pmin[n] = 0.0;
         }
         else {
            node_average_flow_magnitude_m3pmin[n] = average_flow_magnitude_m3pmin / (float)cnt;
         }         
      }
      ClassifyNodes();
      merlionModel->clear();  
   }
}

} /* end of merlionUtils namespace */

