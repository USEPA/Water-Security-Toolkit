#include <merlionUtils/ModelWriter.hpp>

#include <merlion/Merlion.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>

namespace merlionUtils {

void PrintWQMHeadersToPythonFormat(std::ostream& out, const Merlion* model, std::map<int,std::string>& label_map, int start_timestep/*=0*/, int stop_timestep/*=-1*/, std::string post_tag/*=""*/)
{
   /*
   This function will write out the indexing sets for the linear system defining the Merlion water
   quality model in Python format.
   */
   
   const int n_nodes = model->NNodes();
   const int n_steps = model->NSteps();
   const int N = n_nodes*n_steps;
   
   if (stop_timestep == -1) {
      stop_timestep = n_steps-1;  
   }

   if ((start_timestep < 0) ||
      (start_timestep > stop_timestep) || 
      (start_timestep >= n_steps) ||
      (stop_timestep < 0) ||
      (stop_timestep < start_timestep) ||
      (stop_timestep >= n_steps)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: start_timestep (" << start_timestep << ") "
                << "or stop_timestep (" << stop_timestep << ") "
                << "is not within allowed range [0," << n_steps << ") "
                << "or does not make sense." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   out << "# The number of minutes in each timestep\n";
   out << "P_MINUTES_PER_TIMESTEP" << post_tag << " = " << model->Stepsize_min() << "\n\n";

   out << "# The set of nodes in the network\n";
   out << "S_NODES" << post_tag << " = [";
   for (int n = 0; n < n_nodes; ++n) {
      out << label_map[n] << ",";
   }
   out << "]\n\n" << std::endl;

   out << "# The set of timesteps in the network\n";
   out << "S_TIMES" << post_tag << " = [";
   for (int t = 0; t < n_steps; ++t) {
      if ((t >= start_timestep) && (t <= stop_timestep)) {
         out << t << ",";
      }
   }
   out << "]\n\n" << std::endl;
}

void PrintWQMHeadersToAMPLFormat(std::ostream& out, const Merlion* model, std::map<int,std::string>& label_map, int start_timestep/*=0*/, int stop_timestep/*=-1*/, std::string post_tag/*=""*/)
{
   /*
   This function will write out the indexing sets for the linear system defining the Merlion water
   quality model in AMPL format.
   */
   
   const int n_nodes = model->NNodes();
   const int n_steps = model->NSteps();
   const int N = n_nodes*n_steps;
   
   if (stop_timestep == -1) {
      stop_timestep = n_steps-1;  
   }   

   if ((start_timestep < 0) ||
      (start_timestep > stop_timestep) ||
      (start_timestep >= n_steps) ||
      (stop_timestep < 0) ||
      (stop_timestep < start_timestep) ||
      (stop_timestep >= n_steps)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: start_timestep (" << start_timestep << ") "
                << "or stop_timestep (" << stop_timestep << ") "
                << "is not within allowed range [0," << n_steps << ") "
                << "or does not make sense." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   out << "# The number of minutes in each timestep\n";
   out << "param P_MINUTES_PER_TIMESTEP" << post_tag << " := " << model->Stepsize_min() << ";\n\n";

   out << "# The set of nodes in the network\n";
   out << "set S_NODES" << post_tag << " := ";
   for (int n = 0; n < n_nodes; ++n) {
      out << label_map[n] << " ";
   }
   out << ";\n\n" << std::endl;

   out << "# The set of timesteps in the network\n";
   out << "set S_TIMES" << post_tag << " := ";
   for (int t = 0; t < n_steps; ++t) {
      if ((t >= start_timestep) && (t <= stop_timestep)) {
         out << t << " ";
      }
   }
   out << ";\n\n" << std::endl;
}

void PrintWQMNodeTypesToPythonFormat(std::ostream& out, const Merlion* model, std::map<int,std::string>& label_map, std::string post_tag/*=""*/)
{
   /*
   This function will write out the sets of junctions, tanks, and reservoirs in the Merlion water
   quality model in Python format.
   */
   const int n_nodes = model->NNodes();
   
   out << "# The number of minutes in each timestep\n";
   out << "P_MINUTES_PER_TIMESTEP" << post_tag << " = " << model->Stepsize_min() << "\n\n";

   out << "# The set of junctions in the network\n";
   out << "S_JUNCTIONS" << post_tag << " = [";
   for (int n = 0; n < n_nodes; ++n) {
      if (model->NodeIdxTypeMap()[n] == NodeType_Junction) {
         out << label_map[n] << ",";
      }
   }
   out << "]\n\n" << std::endl;

   out << "# The set of tanks in the network\n";
   out << "S_TANKS" << post_tag << " = [";
   for (int n = 0; n < n_nodes; ++n) {
      if (model->NodeIdxTypeMap()[n] == NodeType_Tank) {
         out << label_map[n] << ",";
      }
   }
   out << "]\n\n" << std::endl;

   out << "# The set of reservoirs in the network\n";
   out << "S_RESERVOIRS" << post_tag << " = [";
   for (int n = 0; n < n_nodes; ++n) {
      if (model->NodeIdxTypeMap()[n] == NodeType_Reservoir) {
         out << label_map[n] << ",";
      }
   }
   out << "]\n\n" << std::endl;
}

void PrintWQMNodeTypesToAMPLFormat(std::ostream& out, const Merlion* model, std::map<int,std::string>& label_map, std::string post_tag/*=""*/)
{
   /*
   This function will write out the sets of junctions, tanks, and reservoirs in the Merlion water
   quality model in AMPL format.
   */
   
   const int n_nodes = model->NNodes();

   out << "# The set of junctions in the network\n";
   out << "set S_JUNCTIONS" << post_tag << " := ";
   for (int n = 0; n < n_nodes; ++n) {
      if (model->NodeIdxTypeMap()[n] == NodeType_Junction) {
         out << label_map[n] << " ";
      }
   }
   out << ";\n\n" << std::endl;

   out << "# The set of tanks in the network\n";
   out << "set S_TANKS" << post_tag << " := ";
   for (int n = 0; n < n_nodes; ++n) {
      if (model->NodeIdxTypeMap()[n] == NodeType_Tank) {
         out << label_map[n] << " ";
      }
   }
   out << ";\n\n" << std::endl;

   out << "# The set of reservoirs in the network\n";
   out << "set S_RESERVOIRS" << post_tag << " := ";
   for (int n = 0; n < n_nodes; ++n) {
      if (model->NodeIdxTypeMap()[n] == NodeType_Reservoir) {
         out << label_map[n] << " ";
      }
   }
   out << ";\n\n" << std::endl;
}

void PrintWQMToPythonFormat(std::ostream& out, const Merlion* model, std::map<int,std::string>& label_map, int start_timestep/*=0*/, int stop_timestep/*=-1*/, std::string post_tag/*=""*/)
{
   /*
   This function will write out the linear system defining the Merlion water
   quality model in Python format (Gc + Dm = 0). Currently, the system is scaled by dividing
   each row by the element on its diagonal.
   */
   // This is placed here for convenience so that we may
   // make this optional at some point in the future.
   bool do_scaling = true;   
   
   const int n_nodes = model->NNodes();
   const int n_steps = model->NSteps();
   const int N = n_nodes*n_steps;
   const SparseMatrix* G = model->GetLHSMatrix();
   const int *D = model->GetRHSDiagMatrix();
   const int *perm_upper_to_nt = model->GetPermuteUTtoNT();
   const int *perm_nt_to_upper = model->GetPermuteNTtoUT();

   
   if (stop_timestep == -1) {
      stop_timestep = n_steps-1;  
   }

   if ((start_timestep < 0) ||
      (start_timestep > stop_timestep) || 
      (start_timestep >= n_steps) ||
      (stop_timestep < 0) ||
      (stop_timestep < start_timestep) ||
      (stop_timestep >= n_steps)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: start_timestep (" << start_timestep << ") "
                << "or stop_timestep (" << stop_timestep << ") "
                << "is not within allowed range [0," << n_steps << ") "
                << "or does not make sense." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   const_cast<SparseMatrix*>(G)->TransformToCSRMatrix(); // fills in pRows(), jCols(), and Values() to CSR format

   out << "# The sparse indexing set of P_CONC_MATRIX_CSR" << post_tag << "\n";
   out << "S_CONC_MATRIX_CSR_INDEX" << post_tag << " = {}\n";
   for (int row_UT_idx = 0; row_UT_idx < N; ++row_UT_idx) {
      int row_NT_idx = perm_upper_to_nt[row_UT_idx]; 
      int n = row_NT_idx / n_steps;
      int t = row_NT_idx % n_steps;
      if ((t >= start_timestep) && (t <= stop_timestep)) {
         out << "S_CONC_MATRIX_CSR_INDEX" << post_tag << "[" << label_map[n] << "," << t << "] = [";

         int p_start = G->pRows()[row_UT_idx];
         int p_stop = G->pRows()[row_UT_idx+1];
         for (int p = p_start; p < p_stop; ++p) {
            int col_NT_idx = perm_upper_to_nt[G->jCols()[p]];
            int nn = col_NT_idx / n_steps;
            int tt = col_NT_idx % n_steps;
            if ((tt >= start_timestep) && (tt <= stop_timestep)) {
               out << "(" << label_map[nn] << "," << tt << "),";
            }  
         }
         out << "]\n";
      }
   }
   out << "\n\n" << std::endl;

   out << "# The concentration matrix (G) in CSR format\n";
   out << "P_CONC_MATRIX_CSR" << post_tag << " = {}\n";
   for (int row_UT_idx = 0; row_UT_idx < N; ++row_UT_idx) {
      int row_NT_idx = perm_upper_to_nt[row_UT_idx]; 
      int n = row_NT_idx / n_steps;
      int t = row_NT_idx % n_steps;
      if ((t >= start_timestep) && (t <= stop_timestep)) {
         std::string name_out = label_map[n];

         int p_start = G->pRows()[row_UT_idx];
         int p_stop = G->pRows()[row_UT_idx+1];
         // scale the linear system by the diagonal element of each row
         float diag_element = (do_scaling)?(G->Values()[p_start]):1.0f;
         for (int p = p_start; p < p_stop; ++p) {
            int col_NT_idx = perm_upper_to_nt[G->jCols()[p]];
            int nn = col_NT_idx / n_steps;
            int tt = col_NT_idx % n_steps;
            if ((tt >= start_timestep) && (tt <= stop_timestep)) {
               out << "P_CONC_MATRIX_CSR" << post_tag << "[" << name_out << "," << t << "," << label_map[nn] << "," << tt << "] = " << G->Values()[p]/diag_element << "\n"; 
            }      
         }
         out << "\n";
      }
   }
   out << "\n\n" << std::endl;

   out << "# The injection matrix (D) which is diagonal\n";
   out << "P_INJ_MATRIX_DIAG" << post_tag << " = {}\n";
   for (int nt_idx = 0; nt_idx < N; ++nt_idx) {
      int n = nt_idx / n_steps;
      int t = nt_idx % n_steps;
      // scale the linear system by the diagonal element of each row
      int ut_idx = perm_nt_to_upper[nt_idx];
      float diag_element = (do_scaling)?(G->Values()[G->pRows()[ut_idx]]):1.0f;
      if ((t >= start_timestep) && (t <= stop_timestep)) {
         out << "P_INJ_MATRIX_DIAG" << post_tag << "[" << label_map[n] << "," << t << "] = " << D[nt_idx]*(-1.0f)/diag_element << "\n";
      }  
   }
   out << "\n";

   const_cast<SparseMatrix*>(G)->TransformToCSCMatrix(); // fills in iRows(), pCols(), and Values() to CSC format
   
}

void PrintWQMToAMPLFormat(std::ostream& out, const Merlion* model, std::map<int,std::string>& label_map, int start_timestep/*=0*/, int stop_timestep/*=-1*/, std::string post_tag/*=""*/)
{
   /*
   This function will write out the linear system defining the Merlion water
   quality model in AMPL format (Gc + Dm = 0). Currently, the system is scaled by dividing
   each row by the element on its diagonal.
   */
   // This is placed here for convenience so that we may
   // make this optional at some point in the future.
   bool do_scaling = true;      
   
   const int n_nodes = model->NNodes();
   const int n_steps = model->NSteps();
   const int N = n_nodes*n_steps;
   const SparseMatrix* G = model->GetLHSMatrix();
   const int *D = model->GetRHSDiagMatrix();
   const int *perm_upper_to_nt = model->GetPermuteUTtoNT();  
   const int *perm_nt_to_upper = model->GetPermuteNTtoUT();
   
   if (stop_timestep == -1) {
      stop_timestep = n_steps-1;  
   }   

   if ((start_timestep < 0) ||
      (start_timestep > stop_timestep) ||
      (start_timestep >= n_steps) ||
      (stop_timestep < 0) ||
      (stop_timestep < start_timestep) ||
      (stop_timestep >= n_steps)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: start_timestep (" << start_timestep << ") "
                << "or stop_timestep (" << stop_timestep << ") "
                << "is not within allowed range [0," << n_steps << ") "
                << "or does not make sense." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   
   const_cast<SparseMatrix*>(G)->TransformToCSRMatrix(); // fills in pRows(), jCols(), and Values() to CSR format

   out << "# The sparse indexing set of P_CONC_MATRIX_CSR" << post_tag << "\n";
   for (int row_UT_idx = 0; row_UT_idx < N; ++row_UT_idx) {
      int row_NT_idx = perm_upper_to_nt[row_UT_idx]; 
      int n = row_NT_idx / n_steps;
      int t = row_NT_idx % n_steps;
      if ((t >= start_timestep) && (t <= stop_timestep)) {
         out << "set S_CONC_MATRIX_CSR_INDEX" << post_tag << "[" << label_map[n] << "," << t << "] := ";

         int p_start = G->pRows()[row_UT_idx];
         int p_stop = G->pRows()[row_UT_idx+1];
         for (int p = p_start; p < p_stop; ++p) {
            int col_NT_idx = perm_upper_to_nt[G->jCols()[p]];
            int nn = col_NT_idx / n_steps;
            int tt = col_NT_idx % n_steps;
            if ((tt >= start_timestep) && (tt <= stop_timestep)) {
               out << "(" << label_map[nn] << "," << tt << ") ";
            }
         }
         out << ";\n";
      }
   }
   out << "\n\n" << std::endl;

   out << "# The concentration matrix (G) in CSR format\n";
   out << "param P_CONC_MATRIX_CSR" << post_tag << " := \n";
   for (int row_UT_idx = 0; row_UT_idx < N; ++row_UT_idx) {
      int row_NT_idx = perm_upper_to_nt[row_UT_idx]; 
      int n = row_NT_idx / n_steps;
      int t = row_NT_idx % n_steps;
      if ((t >= start_timestep) && (t <= stop_timestep)) {
         out << " [" << label_map[n] << "," << t << ",*,*] ";

         int p_start = G->pRows()[row_UT_idx];
         int p_stop = G->pRows()[row_UT_idx+1];
         // scale the linear system by the diagonal element of each row
         float diag_element = (do_scaling)?(G->Values()[p_start]):1.0f;
         for (int p = p_start; p < p_stop; ++p) {
            int col_NT_idx = perm_upper_to_nt[G->jCols()[p]];
            int nn = col_NT_idx / n_steps;
            int tt = col_NT_idx % n_steps;
            if ((tt >= start_timestep) && (tt <= stop_timestep)) {
               out << label_map[nn] << " " << tt << " " << G->Values()[p]/diag_element << " ";
            }
         }
         out << "\n";
      }
   }
   out << ";\n\n" << std::endl;

   out << "# The injection matrix (D) which is diagonal\n";
   out << "param P_INJ_MATRIX_DIAG" << post_tag << " := \n";
   for (int nt_idx = 0; nt_idx < N; ++nt_idx) {
      int n = nt_idx / n_steps;
      int t = nt_idx % n_steps;
      // scale the linear system by the diagonal element of each row
      int ut_idx = perm_nt_to_upper[nt_idx];
      float diag_element = (do_scaling)?(G->Values()[G->pRows()[ut_idx]]):1.0f;
      if ((t >= start_timestep) && (t <= stop_timestep)) {
         out << label_map[n] << " "
            << t << " "
            << D[nt_idx]*(-1.0f)/diag_element << "\n";
      }  
   }
   out << ";\n";

   const_cast<SparseMatrix*>(G)->TransformToCSCMatrix(); // fills in iRows(), pCols(), and Values() to CSC format

}

void PrintTankVolumesToPythonFormat(std::ostream& out, const Merlion *model, std::map<int,std::string>& label_map, int start_timestep/*=0*/, int stop_timestep/*=-1*/, std::string post_tag/*=""*/)
{
   const int n_nodes = model->NNodes();
   const int n_steps = model->NSteps();
   const float *tank_volumes = model->GetTankVolumes_m3();   
   
   if (stop_timestep == -1) {
      stop_timestep = n_steps-1;  
   }
   
   if ((start_timestep < 0) ||
      (start_timestep > stop_timestep) ||
      (start_timestep >= n_steps) ||
      (stop_timestep < 0) ||
      (stop_timestep < start_timestep) ||
      (stop_timestep >= n_steps)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: start_timestep (" << start_timestep << ") "
                << "or stop_timestep (" << stop_timestep << ") "
                << "is not within allowed range [0," << n_steps << ") "
                << "or does not make sense." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   
   out << "# The set of Tank Node IDs\n";
   out << "S_TANKS" << post_tag << " = [";
   int tank_n = 0;
   std::vector<std::pair<int,int> > tanks;
   for (int n = 0; n < n_nodes; ++n) {
      if (model->NodeIdxTypeMap()[n] == NodeType_Tank) {
         tanks.push_back(std::pair<int,int>(n,tank_n));
         tank_n++;
         out << label_map[n] << ",";
      }
   }
   if (!tanks.empty()) {
      out.seekp(-1,std::ios::end);
   }
   out << "]\n\n";

   out << "# Tank Volumes [m^3]\n";
   out << "P_TANK_VOLUMES" << post_tag << "_m3 = {}\n";
   for (std::vector<std::pair<int,int> >::const_iterator pos = tanks.begin(), stop = tanks.end(); pos != stop; ++pos) {
      int n = pos->first;
      int tank_n = pos->second;
      for (int t = start_timestep; t <= stop_timestep; ++t) {
         out << "P_TANK_VOLUMES_m3[" << label_map[n] << "," << t << "] = " << tank_volumes[tank_n*n_steps+t] << "\n";
      }
   }
   out << "\n";
}

void PrintTankVolumesToAMPLFormat(std::ostream& out, const Merlion *model, std::map<int,std::string>& label_map, int start_timestep/*=0*/, int stop_timestep/*=-1*/, std::string post_tag/*=""*/)
{
   const int n_nodes = model->NNodes();
   const int n_steps = model->NSteps();
   const float *tank_volumes = model->GetTankVolumes_m3();   
   
   if (stop_timestep == -1) {
      stop_timestep = n_steps-1;  
   }
   
   if ((start_timestep < 0) ||
      (start_timestep > stop_timestep) ||
      (start_timestep >= n_steps) ||
      (stop_timestep < 0) ||
      (stop_timestep < start_timestep) ||
      (stop_timestep >= n_steps)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: start_timestep (" << start_timestep << ") "
                << "or stop_timestep (" << stop_timestep << ") "
                << "is not within allowed range [0," << n_steps << ") "
                << "or does not make sense." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   
   out << "# The set of Tank Node IDs\n";
   out << "set S_TANKS" << post_tag << " := ";
   int tank_n = 0;
   std::vector<std::pair<int,int> > tanks;
   for (int n = 0; n < n_nodes; ++n) {
      if (model->NodeIdxTypeMap()[n] == NodeType_Tank) {
         tanks.push_back(std::pair<int,int>(n,tank_n));
         tank_n++;
         out << label_map[n] << " ";
      }
   }
   out << ";\n\n";

   out << "# Tank Volumes [m^3]\n";
   out << "param P_TANK_VOLUMES" << post_tag << "_m3 := \n";
   for (std::vector<std::pair<int,int> >::const_iterator pos = tanks.begin(), stop = tanks.end(); pos != stop; ++pos) {
      int n = pos->first;
      int tank_n = pos->second;
      for (int t = start_timestep; t <= stop_timestep; ++t) {
         out << label_map[n] << " "
             << t << " " 
             << tank_volumes[tank_n*n_steps+t] << "\n";
      }
   }
   out << ";\n";
}

void PrintDemandToPythonFormat(std::ostream& out, const Merlion *model, std::map<int,std::string>& label_map, int start_timestep/*=0*/, int stop_timestep/*=-1*/, std::string post_tag/*=""*/)
{
   const int n_nodes = model->NNodes();
   const int n_steps = model->NSteps();
   const float *demand = model->GetNodeDemands_m3pmin();   
   
   if (stop_timestep == -1) {
      stop_timestep = n_steps-1;  
   }
   
   if ((start_timestep < 0) ||
      (start_timestep > stop_timestep) ||
      (start_timestep >= n_steps) ||
      (stop_timestep < 0) ||
      (stop_timestep < start_timestep) ||
      (stop_timestep >= n_steps)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: start_timestep (" << start_timestep << ") "
                << "or stop_timestep (" << stop_timestep << ") "
                << "is not within allowed range [0," << n_steps << ") "
                << "or does not make sense." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   out << "# Node Demands [m^3/min]\n";
   out << "P_DEMANDS" << post_tag << "_m3pmin = {}\n";
   for (int n = 0; n < n_nodes; ++n) {
      for (int t = start_timestep; t <= stop_timestep; ++t) {
         out << "P_DEMANDS" << post_tag << "_m3pmin[" << label_map[n] << "," << t << "] = " << demand[n*n_steps+t] << "\n";
      }
   }
   out << "\n";
}

void PrintDemandToAMPLFormat(std::ostream& out, const Merlion *model, std::map<int,std::string>& label_map, int start_timestep/*=0*/, int stop_timestep/*=-1*/, std::string post_tag/*=""*/)
{
   const int n_nodes = model->NNodes();
   const int n_steps = model->NSteps();
   const float *demand = model->GetNodeDemands_m3pmin();   

   if (stop_timestep == -1) {
      stop_timestep = n_steps-1;  
   }

   if ((start_timestep < 0) ||
      (start_timestep > stop_timestep) ||
      (start_timestep >= n_steps) ||
      (stop_timestep < 0) ||
      (stop_timestep < start_timestep) ||
      (stop_timestep >= n_steps)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: start_timestep (" << start_timestep << ") "
                << "or stop_timestep (" << stop_timestep << ") "
                << "is not within allowed range [0," << n_steps << ") "
                << "or does not make sense." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   out << "# Node Demands [m^3/min]\n";
   out << "param P_DEMANDS" << post_tag << "_m3pmin := \n";
   for (int n = 0; n < n_nodes; ++n) {
      for (int t = start_timestep; t <= stop_timestep; ++t) {
         out << label_map[n] << " "
             << t << " "
             << demand[n*n_steps+t] << "\n";
      }
   }
   out << ";\n";
}

} /* end of merlionUtils namespace */
