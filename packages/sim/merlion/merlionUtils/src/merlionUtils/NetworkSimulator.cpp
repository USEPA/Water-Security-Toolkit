#include <merlionUtils/NetworkSimulator.hpp>
#include <merlionUtils/SimTools.hpp>
#include <merlionUtils/TaskTimer.hpp>
#include <merlionUtils/EpanetLinker.hpp>
#include <merlionUtils/TSG_Reader.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <fstream>
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

} // end merlionUtils (local) namespace

void NetworkSimulator::StartLogging(std::string logname) {
   logfile_.close();
   logfile_.open(logname.c_str(),std::ios_base::out | std::ios_base::trunc);
   logfilename_ = logname;      
}

void NetworkSimulator::StopLogging() {
   logfile_.close();
}

void NetworkSimulator::clear()
{  
   disable_warnings_ = false;
   ResetModel();
   ResetSensorData();
   ResetInverse();
   ResetScenarios();
   ResetPopulationUsageData();
}

void NetworkSimulator::ResetModel()
{
   model_.clear();
}

void NetworkSimulator::ResetSensorData()
{
   sensor_node_ids_.clear();
   grab_sample_ids_.clear();
   // Make sure these vectors release the memory that they cache
   std::vector<int>().swap(sensor_node_ids_);
   std::vector<std::pair<int, int> >().swap(grab_sample_ids_);
}
      
void NetworkSimulator::ResetScenarios()
{
   injection_scenarios_.clear();
   ResetScenarioClassifications();
}

void NetworkSimulator::ResetScenarioClassifications()
{
   timestep_to_detected_scenarios_.clear();
   detected_scenarios_.clear();
   non_detected_scenarios_.clear();
   discarded_scenarios_.clear();
}

void NetworkSimulator::ResetInverse()
{
   wqm_inverse_.clear();
}

void NetworkSimulator::ResetPopulationUsageData()
{
   node_population_.clear();
   volume_ingested_m3_.clear();
   // Make sure these vectors release the memory that they cache
   std::vector<int>().swap(node_population_);
   std::vector<double>().swap(volume_ingested_m3_);
}

void NetworkSimulator::SaveInverse(std::ostream&out, bool binary/*=true*/) const
{
   wqm_inverse_.PrintCSRMatrix(out,"wqm_inverse",binary);
   if (binary) {
      std::set<int>::size_type n_witnesses = wqm_inverse_.Witnesses().size();
      out.write(reinterpret_cast<char*>(&n_witnesses), sizeof(n_witnesses));
      for (std::set<int>::const_iterator pos = wqm_inverse_.Witnesses().begin(),
              stop = wqm_inverse_.Witnesses().end(); pos != stop; ++pos) {
         int witness_u_idx = *pos;
         out.write(reinterpret_cast<char*>(&witness_u_idx), sizeof(witness_u_idx));
      }
      for (int event_u_idx = 0; event_u_idx < model_.N; ++event_u_idx) {
         int witness_u_idx = wqm_inverse_.FirstWitness(event_u_idx);
         out.write(reinterpret_cast<char*>(&witness_u_idx), sizeof(witness_u_idx));
      }
   }
   else {
      out << wqm_inverse_.Witnesses().size() << "\n";
      for (std::set<int>::const_iterator pos = wqm_inverse_.Witnesses().begin(),
              stop = wqm_inverse_.Witnesses().end(); pos != stop; ++pos) {
         int witness_u_idx = *pos;
         out << witness_u_idx << "\n";
      }
      for (int event_u_idx = 0; event_u_idx < model_.N; ++event_u_idx) {
         int witness_u_idx = wqm_inverse_.FirstWitness(event_u_idx);
         out << witness_u_idx << "\n";
      }
   }
}

void NetworkSimulator::ReadInverse(std::string fname, bool binary/*=true*/)
{
   ResetInverse();
   wqm_inverse_.Init(model_);
   
   std::ifstream in;
   if (binary) {in.open(fname.c_str(), std::ios::in | std::ios::binary);}
   else {in.open(fname.c_str(), std::ios::in);}

   if (in.is_open() && in.good()) {
      wqm_inverse_.ReadCSRMatrix(in,binary);
      // ***NOTE*** It is possible that the dimensions below match but the current water quality model (model_)
      //            and the inversre matrix have dothing to do with each other. It begs the question as to whether
      //            the inverse file should contain the water quality model it was build upon.
      if ( (wqm_inverse_.NRows() != model_.N) && 
           (wqm_inverse_.NCols() != model_.N)) {
	 std::cerr << std::endl;
	 std::cerr << "ERROR: Invalid dimensions found in Merlion inverse file." << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      if (binary) {
         std::set<int>::size_type n_witnesses = 0;
         in.read(reinterpret_cast<char*>(&n_witnesses), sizeof(n_witnesses));
         for (std::set<int>::size_type i = 0; i < n_witnesses; ++i) {
            int witness_u_idx = wqm_inverse_.NoWitness;
            in.read(reinterpret_cast<char*>(&witness_u_idx), sizeof(witness_u_idx));
            wqm_inverse_.AddWitness(witness_u_idx);
         }
         for (int event_u_idx = 0; event_u_idx < model_.N; ++event_u_idx) {
            int witness_u_idx = wqm_inverse_.NoWitness;
            in.read(reinterpret_cast<char*>(&witness_u_idx), sizeof(witness_u_idx));
            if (wqm_inverse_.isWitness(witness_u_idx)) {
               wqm_inverse_.WitnessEvent(event_u_idx,witness_u_idx);
            }
         }
      }
      else {
         std::set<int>::size_type n_witnesses = 0;
         in >> n_witnesses;
         for (std::set<int>::size_type i = 0; i < n_witnesses; ++i) {
            int witness_u_idx = wqm_inverse_.NoWitness;
            in >> witness_u_idx;
            wqm_inverse_.AddWitness(witness_u_idx);
         }
         for (int event_u_idx = 0; event_u_idx < model_.N; ++event_u_idx) {
            int witness_u_idx = wqm_inverse_.NoWitness;
            in >> witness_u_idx;
            if (wqm_inverse_.isWitness(witness_u_idx)) {
               wqm_inverse_.WitnessEvent(event_u_idx,witness_u_idx);  
            }
         }
      }
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Unable to open file: " << fname << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   in.close();
}

void NetworkSimulator::GenerateReducedInverse(int sim_start_timestep,
					      int sim_stop_timestep,
                                              float sensor_interval_minutes/*=0*/,
					      int max_rhs/*=1*/,
					      bool map_only/*=false*/,
					      float zero_wqm_inv/*=ZERO_WQM_INV*/)
{
   if (!( (0 <= sim_start_timestep) &&
	  (sim_start_timestep <= sim_stop_timestep) &&
	  (sim_stop_timestep <= model_.n_steps-1) )) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Invalid start or stop timestep supplied to GenerateReducedInverse." << std::endl;
      std::cerr << std::endl;
   }

   // Round the detection interval to the nearest multiple of the water
   // quality timestep
   int sensor_interval_timesteps = 1;
   if (sensor_interval_minutes > model_.qual_step_minutes) {
      sensor_interval_timesteps = RoundToInt(sensor_interval_minutes/model_.qual_step_minutes);
   }

   std::set<int> inverse_row_ids;
   // loop through the sensors and grab samples and add each to the inverse_row_ids set but only
   // if the corresponding timestep is between the sim_start_timestep and sim_stop_timestep
   for (std::vector<int>::const_iterator pos = sensor_node_ids_.begin(), stop = sensor_node_ids_.end(); pos != stop; ++pos) {
      for (int t = sim_start_timestep; t < sim_stop_timestep; t+=sensor_interval_timesteps) {
	 inverse_row_ids.insert(model_.perm_nt_to_upper[(*pos)*model_.n_steps + t]);
      }
   }
   for (std::vector<std::pair<int,int> >::const_iterator pos = grab_sample_ids_.begin(), stop = grab_sample_ids_.end(); pos != stop; ++pos) {
      int n = pos->first;
      int t = merlionUtils::SecondsToNearestTimestepBoundary(pos->second, model_.qual_step_minutes);
      if ((sim_start_timestep <= t) && (t <= sim_stop_timestep)) {
	 inverse_row_ids.insert(model_.perm_nt_to_upper[n*model_.n_steps + t]);
      }
   }
   GenerateReducedInverse(inverse_row_ids, sim_start_timestep, max_rhs, map_only, zero_wqm_inv);
}

void NetworkSimulator::GenerateReducedInverse(const std::set<int>& inverse_row_ids,
					      int sim_start_timestep,
					      int max_rhs/*=1*/,
					      bool map_only/*=false*/,
					      float zero_wqm_inv/*=ZERO_WQM_INV*/)
{

   ResetInverse();
   wqm_inverse_.Init(model_);

   if (!( (0 <= sim_start_timestep) &&
	  (sim_start_timestep <= model_.n_steps-1) )) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Invalid start timestep supplied to GenerateReducedInverse" << std::endl;
      std::cerr << std::endl;
   }
   
   if (inverse_row_ids.empty()) {
      std::cerr << std::endl;
      std::cerr << "ERROR: The list of inverse row indices must not be empty when calling GenerateReducedInverse." << std::endl;
      std::cerr << std::endl;    
      exit(1);
   }
   
   // Add the list of witness ids to the inverse
   for (std::set<int>::const_iterator pos = inverse_row_ids.begin(),
	   stop = inverse_row_ids.end(); pos != stop; ++pos) {
      wqm_inverse_.AddWitness(*pos);
   }

   // Determine the largest column index that needs to be considered when
   // building the inverse. Also remove any rows whose diagonal starts later
   // than this column id
   const int max_u_idx = (model_.N-1) - model_.n_nodes*sim_start_timestep;
  
   // This will contain each of the inverse rows as well as the total nnz, which
   // we will copy into a SparseMatrix class after all rows are collected.
   int nnz = 0;
   std::map<int, std::vector<int> > inverse_cols;
   std::map<int, std::vector<float> > inverse_vals;

   // properly set the number of rhs to use
   int nrhs = max_rhs;
   const int total_solves = inverse_row_ids.size();
   if (nrhs > total_solves) {
      nrhs = total_solves;
   }

   // Allocate memory for the rhs
   float* r(NULL);
   r = new float[model_.N*nrhs];
   BlasInitZero(model_.N*nrhs, r);

   //Create timers but don't start them
   TaskTimer Time_analyze(false);
   TaskTimer Time_zero(false);
   TaskTimer Time_backsolve(false);
   if (nrhs > 1) {
      int total_blocks = total_solves/nrhs;
      const int odd_count = total_solves%nrhs;
      if (odd_count) {++total_blocks;}
      int current_block(0);

      std::set<int>::const_iterator p_row_id = inverse_row_ids.begin();
      std::set<int>::const_iterator p_row_id_end = inverse_row_ids.end();
      std::set<int>::const_iterator p_row_id_block_start;
      std::vector<int> block_inv_row_ids;

      // These will speed up the loop after the 
      // backsolve. We don't have to search all the keys of the
      // inverse_cols/vals std::maps each time we access one of the
      // rows we are dealing with in the multy rhs solve block, so
      // we keep a vector of the rows we are using during each block
      std::list<std::vector<int>* > pv_inv_cols;
      std::list<std::vector<float>* > pv_inv_vals;
      int nnz_prev_row = 0;

      while (current_block < total_blocks) {
         
	 block_inv_row_ids.resize(nrhs);
	 p_row_id_block_start = p_row_id;

	 if(!map_only) {
	    pv_inv_cols.clear();
	    pv_inv_vals.clear();
	 }
	 
	 int min_u_idx(model_.N);
         for (int rhs = 0; rhs < nrhs; ++rhs) {
            const int u_idx = *p_row_id++;
	    block_inv_row_ids[rhs] = u_idx;
            r[u_idx*nrhs + rhs] = 1.0f;
	    if (u_idx < min_u_idx) {min_u_idx = u_idx;}
	    if (!map_only) {
               // As a guess, reserve at least the number of nonzeros as the corresponding row in G, or the amount
               // of nonzeros required for the previous row
	       inverse_vals[u_idx].reserve(std::max(nnz_prev_row,model_.G->pCols()[u_idx+1]-model_.G->pCols()[u_idx])); 
	       inverse_cols[u_idx].reserve(std::max(nnz_prev_row,model_.G->pCols()[u_idx+1]-model_.G->pCols()[u_idx]));
	       pv_inv_cols.push_back(&inverse_cols[u_idx]);
	       pv_inv_vals.push_back(&inverse_vals[u_idx]);
	    }
         }
      
	 Time_backsolve.Stop();
         utsolvem(model_.N, min_u_idx, max_u_idx, nrhs, model_.G->Values(), model_.G->iRows(), model_.G->pCols(), r);
	 Time_backsolve.Start();
      
	 Time_analyze.Start();
	 if (map_only) {
	    for (int u_rhs_idx = min_u_idx*nrhs, u_rhs_stop = (max_u_idx+1)*nrhs - 1; u_rhs_idx <= u_rhs_stop; ++u_rhs_idx) {
	       if (r[u_rhs_idx] > zero_wqm_inv) {
		  wqm_inverse_.WitnessEvent( u_rhs_idx/nrhs, block_inv_row_ids[u_rhs_idx%nrhs] );
	       }
	    }
	 }
	 else {
	    std::list<std::vector<int>* >::iterator pc;
	    std::list<std::vector<float>* >::iterator pv;
	    for (int u_idx = min_u_idx; u_idx <= max_u_idx; ++u_idx) {
	       pc = pv_inv_cols.begin();
	       pv = pv_inv_vals.begin();
	       for (int rhs = 0; rhs < nrhs; ++rhs, ++pc, ++pv) {
		  if (r[u_idx*nrhs+rhs] > zero_wqm_inv) {
		     wqm_inverse_.WitnessEvent( u_idx, block_inv_row_ids[rhs] );
		     (*pv)->push_back(r[u_idx*nrhs+rhs]);
		     (*pc)->push_back(u_idx);
		     ++nnz; 
		  } 
	       }
	    }
            nnz_prev_row = pv_inv_cols.back()->size();
	 }
	 Time_analyze.Stop();
      
	 Time_zero.Start();
	 // don't worry about clearing the array if this was the last solve
	 if (current_block < total_blocks) {
	    // reset to zero only the values where r
	    // was possibly modified by linear solver, this saves time
	    const int num_elements = nrhs*(max_u_idx-min_u_idx+1);
	    const int p_offset = min_u_idx*nrhs;
	    BlasResetZero(num_elements, r+p_offset);
	 }
	 Time_zero.Stop();

	 ++current_block;
	 // for the last multiple-rhs block, reduce the leading dimension
	 // of the rhs/sol matrix to match the number of rhs left over.
	 // The extra memory that was allocated will just be left unused
	 if ((current_block == total_blocks-1) && (odd_count)) {
	    nrhs = odd_count;
	 }
      }
   }
   else if (nrhs == 1) {
      int nnz_prev_row = 0;
      for (std::set<int>::const_iterator p_row_id = inverse_row_ids.begin(), p_row_id_end = inverse_row_ids.end(); p_row_id != p_row_id_end; ++p_row_id) {
         
         const int min_u_idx = *p_row_id;
         r[min_u_idx] = 1.0f;

         Time_backsolve.Start();
         utsolve(model_.N, min_u_idx, max_u_idx, model_.G->Values(), model_.G->iRows(), model_.G->pCols(), r);
	 Time_backsolve.Stop();   
      
         Time_analyze.Start();
	 if (map_only) {
	    for (int u_idx = min_u_idx; u_idx <= max_u_idx; ++u_idx) {
	       if (r[u_idx] > zero_wqm_inv) {
		  wqm_inverse_.WitnessEvent( u_idx, min_u_idx );
	       }
	    }
	 }
	 else {
	    inverse_vals[min_u_idx].reserve(std::max(nnz_prev_row,model_.G->pCols()[min_u_idx+1]-model_.G->pCols()[min_u_idx]));
            inverse_cols[min_u_idx].reserve(std::max(nnz_prev_row,model_.G->pCols()[min_u_idx+1]-model_.G->pCols()[min_u_idx]));
	    std::vector<float>& inv_vals = inverse_vals[min_u_idx];
	    std::vector<int>& inv_cols = inverse_cols[min_u_idx];
	    for (int u_idx = min_u_idx; u_idx <= max_u_idx; ++u_idx) {
	       if (r[u_idx] > zero_wqm_inv) {
		  inv_vals.push_back(r[u_idx]);
		  inv_cols.push_back(u_idx);	  
		  ++nnz;
		  wqm_inverse_.WitnessEvent( u_idx, min_u_idx );
	       }
	    }
            nnz_prev_row = inv_cols.size();
	 }
	 Time_analyze.Stop();
      
         Time_zero.Start();
	 // don't worry about clearing the array if this was the last solve
	 if (std::distance(p_row_id,p_row_id_end) > 1) {
	    // reset to zero only the values where r
	    // was possibly modified by linear solver, this saves time
	    const int num_elements = max_u_idx-min_u_idx+1;
	    const int p_offset = min_u_idx;
	    BlasResetZero(num_elements, r+p_offset);
	 }
	 Time_zero.Stop();
      }
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Invalid number of rhs specified for GenerateReducedInverse" << std::endl;
      std::cerr << std::endl;    
      exit(1);
   }

   delete [] r;
   r = NULL; 

   // Copy the inverse into a SparseMatrix class;
   TaskTimer Time_csr_matrix;
   if (!map_only) {
      wqm_inverse_.AllocateNewCSRMatrix(model_.G->NRows(), model_.G->NCols(), nnz);
      int *R_prows = wqm_inverse_.pRows();
      int *R_jcols = wqm_inverse_.jCols();
      float *R_vals = wqm_inverse_.Values();
      int row_prev = 0;
      R_prows[0] = 0;
      // It is important that we loop through rows from lowest to highest
      // A std::map<int,..> will sort integer keys in this way so this loop is okay.
      for (std::map<int, std::vector<int> >::iterator pos = inverse_cols.begin(), p_stop = inverse_cols.end(); pos != p_stop; ++pos) {
         int row = pos->first;
	 std::vector<int>& v_col = pos->second;
	 std::vector<float>& v_val = inverse_vals[row];
	 
	 while(++row_prev != row+2) {R_prows[row_prev] = R_prows[row_prev-1];}
	 R_prows[row+1] += v_col.size();
	 row_prev = row+1;
	 
	 std::copy(v_col.begin(), v_col.end(), R_jcols);
	 cblas_scopy(v_val.size(),&(v_val[0]),1,R_vals,1);
	 R_jcols += v_col.size();
	 R_vals += v_val.size();
      }
      for (int i = row_prev; i < wqm_inverse_.NRows(); ++i) {
	 R_prows[i+1] = R_prows[i];
      }
   }
   Time_csr_matrix.Stop();
   
   // Log some useful debugging info
   if (logfile_.is_open()) {
      logfile_ << "Summary of GenerateReducedInverse() timing:\n";
      logfile_ << "Solve:   " << Time_backsolve.Status() << "\n";
      logfile_ << "Analyze: " << Time_analyze.Status() << "\n";
      logfile_ << "Zero:    " << Time_zero.Status() << "\n";
      logfile_ << "Copy:    " << Time_csr_matrix.Status() << "\n";
      logfile_ << "TOTAL:   " << Time_backsolve.Status()+Time_analyze.Status()+Time_zero.Status()+Time_csr_matrix.Status() << "\n";
      logfile_ << ";\n"; //mark the end of this section
      logfile_ << std::endl << std::endl; // flush the buffer
   }
}

void NetworkSimulator::ClassifyScenarios(bool detection_times_provided,
					 int latest_allowed_detection_timestep/*=-1*/,
					 float detection_tol_gpm3/*=0.0f*/,
                                         float detection_interval_minutes/*=0*/)
{
   /*
     This method uses the water quality model reduced inverse (created from sensors and grab samples) to
     partition the list of injection scenarios into three types:
     
       - detected scenarios - Scenarios that have a detection time earlier than latest_allowed_detection_timestep
       - discarded scenarios - Scenarios that have all injection profiles entirely during stagnant flow    
       - non-detected scenarios - Scenarios which are not detected earlier than latest_allowed_detection_timestep
        
     Additionally, a warning will be printed when at least one scenario has an injection even partially
     during stagnant flow. A warning will also be printed when the discarded scenarios list is ever non-empty.
     
     A detection is flagged when concentration at a sensor node or grab sample node/time reaches the input threshold
     value detection_tol_gpm3.
     
     This method also keeps track of, what are here referred to as, 'nominal' detections. These are automatic detections
     due to injections occurring at a sensor nodes or grab sample node/time (witnesses). Currently, these types of 
     detections do not take into account the resulting concentration of the witness. One edge case where this is important is
     when the start of the injection occurs during stagnant flow. In this case the 'nominal' detection would be immediate
     and the concentration based injection would not occur until non-stagnant flow begins. Another controversial edge case 
     would be that when the nominal injection case would not have resulted in concentration values above the detection
     tolerance (whether or not its a stagnant flow situation).
     
     The final detection time is defined as the minimum of the 'nominal' detection time and the concentration based 
     detection time.

     The detection interval can be used to limit detection times to the nearest rounded time interval,  provided this 
     interval is a multiple of the water quality stepsize (if not the interval will be rounded to the nearest multiple of
     the water quality stepsize.

     The detection tolerance and detection interval options are ignored when the detection times provided flag is 
     set to true.
   */
   ResetScenarioClassifications();

   
   
   // If one specifies a concentration threshold different than 0.0
   // then we need to actually calculate concentration values and
   // we can't just rely on the sparsity patterns of the inverse
   // (which is captured by the first witness logic)
   bool do_simulations = false;
   if (detection_tol_gpm3 > 0.0f) {
      do_simulations = true;
      wqm_inverse_.TransformToCSRMatrix();
   }

   // Round the detection interval to the nearest multiple of the water
   // quality timestep
   int detection_interval_timesteps = 1;
   if (detection_interval_minutes > model_.qual_step_minutes) {
      detection_interval_timesteps = RoundToInt(detection_interval_minutes/model_.qual_step_minutes);
   }
   
   const int final_timestep = model_.n_steps-1;
   // Scenario detected after the latest_allowed_detection_timestep are
   // classifed as undetected. We output a warning to the user when this 
   // happens.
   int late_detection_warning_cnt(0);
   if (latest_allowed_detection_timestep == -1) {latest_allowed_detection_timestep = final_timestep;}

   if (!detection_times_provided) {
      InjScenList::iterator p_scen = injection_scenarios_.begin();
      // reverse iterate through the scenario list
      // if detection does not occur we add the scenario to the non-detected list
      // if the injection is within entirely stagnant flow then add to the discarded list.
      for (; p_scen != injection_scenarios_.end(); ++p_scen) {
	 // will be true when at least one injection point
	 // occurs during non-stagnant flow
         bool is_not_all_stagnant(false);
         int first_nominal_witness = wqm_inverse_.NoWitness;
	 int first_witness = wqm_inverse_.NoWitness;
	 int detection_timestep = model_.n_steps;
	 // Loop through each of the injections for this scenario and find the first witness
	 // **NOTE** This loop is basically computationally free compared to the do_simulations loop
	 //          its all integer operations and it provides us very usefull information
	 InjectionList& injlist = (*p_scen)->Injections();
         for (InjectionList::iterator p_inj = injlist.begin(), inj_end = injlist.end(); p_inj != inj_end; ++p_inj) {
            Injection& inj = **p_inj;
            int n = model_.NodeID(inj.NodeName());
	    int nt_idx_start = n*model_.n_steps + inj.StartTimestep(model_);
            int nt_idx_stop = n*model_.n_steps + inj.StopTimestep(model_);
            for (int nt_idx = nt_idx_start; nt_idx <= nt_idx_stop; ++nt_idx) {
	       int u_idx = model_.perm_nt_to_upper[nt_idx];
               // the nominal witness (i.e a node/time where an injection occurs at a witness) ignores
               // whether or not stagant flow occurs
	       if ((wqm_inverse_.isWitness(u_idx)) && (u_idx > first_nominal_witness)) {
	       	  first_nominal_witness = u_idx;
	       }
               // the "non-nominal" first witness is the one where any nonzero concentration occurs
	       else if ((model_.D[nt_idx]) && (wqm_inverse_.FirstWitness(u_idx) > first_witness)) {
	       	  first_witness = wqm_inverse_.FirstWitness(u_idx);
	       }
	       // make sure at least one injection point was not during stagnant flow
               // otherwise we will flag this scenario as discarded for no particular reason
               // other than making the user aware that this scenario will not result in any
               // nonzero concentration values (which is probably not expected)
	       if ((!is_not_all_stagnant) && (model_.D[nt_idx])) {is_not_all_stagnant = true;}
            }
         }

	 // calculate concentration values at sensor nodes and grab sample node/times (witness locations) in order
	 // to more accurately check when a detection should be flagged
	 // This will be much slower than the above analysis since 
	 // actual concentrations are calculated for each witness
	 // until detection occurs, but this will be more accurate for a relatively
	 // high concentration tolerance for detection
	 if ((is_not_all_stagnant) && (first_witness != wqm_inverse_.NoWitness) && (do_simulations)) {
	    // Since the first_witness determined by the previous loop had only to do with the sparsity 
	    // pattern of the inverse and not a real tolerance, the first_witness calculated by this loop
            // will only ever be smaller (later in time) than that from above.
	    // We can take advantage of this information to reduce the number of floating point operations we need to 
	    // perform in this block
	    int u_idx_max;
	    float mass_injected_g;
            std::map<int,float> tox_mass_inj_gpmin;
	    (*p_scen)->SetArray(model_, tox_mass_inj_gpmin, mass_injected_g, u_idx_max);
	    std::map<int,float>::const_iterator p_tox_rhs;
	    const std::map<int,float>::const_iterator p_tox_rhs_end = tox_mass_inj_gpmin.end();
	    
	    // Reverse iterating through the witness list is equivalent to searching
	    // each sensor/grab sample order of earliest to latest possible time.
	    // This is because this std::set<int> consists of the witness upper triangular
	    // index
	    std::set<int>::const_reverse_iterator rp_witness = wqm_inverse_.Witnesses().rbegin();
	    // no point in checking witnesses before the max index for this scenario's injections
	    // **NOTE** At this point the first witness id tells us the highest id (therefore earliest
	    //          time) where a nonzero in the rhs vector meets a nonzero in any row over that
	    //          column of the inverse, which tells us highest witness id we possibly need to
	    //          check and can save of costly floating point operations for the loop over each
	    //          row in the inverse.
	    assert(first_witness <= u_idx_max);
	    // **** This line results in HUGE speedup to this block of code. It uses information from
	    //      the previous fast loop and in most cases makes this block of code as fast as the
	    //      previous block (but it still requires information from the previous block).
	    while(*rp_witness > first_witness) {++rp_witness;}
	    // now we need to make sure to reset this since the result of this calculation will
	    // override this original value (e.g. the calculated concentration may be below the
	    // detection tolerance, so no detection is flagged)
	    first_witness = wqm_inverse_.NoWitness;
	    std::set<int>::const_reverse_iterator rp_witness_stop = wqm_inverse_.Witnesses().rend();
	    if (mass_injected_g != 0.0f) {
	       for (; rp_witness != rp_witness_stop; ++rp_witness) {
		  float c(0.0f);
		  int p_start = wqm_inverse_.pRows()[*rp_witness];
		  int p_stop = wqm_inverse_.pRows()[(*rp_witness)+1]-1;

		  // iterate over the intersection of the nonzero patterns in the rhs vector and the
		  // current row of the inverse. Despite the added integer operations, this is still
		  // MUCH faster than looping over the entire row and performing the redundant
		  // multiplications of row[i]*0.0. This is because the rhs vector is usually very 
		  // sparse compared to the inverse row.
		  p_tox_rhs = tox_mass_inj_gpmin.begin();
		  while ( (p_start<=p_stop) && (p_tox_rhs!=p_tox_rhs_end) ) {
		     if ( wqm_inverse_.jCols()[p_start] < (p_tox_rhs->first) ) {++p_start;}
		     else if ( (p_tox_rhs->first) < wqm_inverse_.jCols()[p_start] ) {++p_tox_rhs;}
		     else { c += wqm_inverse_.Values()[p_start++]*(p_tox_rhs->second); ++p_tox_rhs; }
		  }

		  if (c > detection_tol_gpm3) {
		     first_witness = *rp_witness;
		     break;
		  }
	       }
	    }
	 }

	 if (first_nominal_witness > first_witness) {first_witness = first_nominal_witness;}

	 if (first_witness != wqm_inverse_.NoWitness) {
	    detection_timestep = model_.perm_upper_to_nt[first_witness] % model_.n_steps;
            if (detection_timestep % detection_interval_timesteps) {
               detection_timestep = RoundToInt(detection_timestep/((float)detection_interval_timesteps))*detection_interval_timesteps;
            }
	 }
         
         if (!is_not_all_stagnant) {
            // copies the scenario pointer into the discarded list
	    // **NOTE**: Although at this point the scenario may have been nominally detected
	    //           (if the injection occurred at a sensor node), I still want to add it to
            //           the discarded scenario list since it will result in COMPLETELY EMPTY
	    //           water quality calculations. Its better to not let things like this go 
	    //           unnoticed 
            discarded_scenarios_.push_back(*p_scen);
         }
         else if (detection_timestep <= latest_allowed_detection_timestep) {
            detected_scenarios_.push_back(*p_scen);
	    (*p_scen)->SetDetectionTimeSeconds(detection_timestep*60*model_.qual_step_minutes);
	    timestep_to_detected_scenarios_[detection_timestep].push_back(*p_scen);
	 }
         else {
            non_detected_scenarios_.push_back(*p_scen);
            // warn about late detections
            (detection_timestep <= final_timestep) && late_detection_warning_cnt++;
         }
      }
      
   }
   else {
      bool has_detection(false); // check if at least one of the scenarios really did have a detection time
      for (InjScenList::iterator p_scen = injection_scenarios_.begin(), scen_stop = injection_scenarios_.end(); p_scen != scen_stop; ++p_scen) {
         int detection_timestep(model_.n_steps);
         bool is_not_all_stagnant(false);
         if ((*p_scen)->isDetected()) {
            detection_timestep = (*p_scen)->DetectionTimestep(model_);
            has_detection = true;
         }
         InjectionList& injlist = (*p_scen)->Injections();
	 // Although detection times have been provided I still want to provide the safeguard against
	 // blindly using scenarios that have injections during entirely stagnant flow, hence the check
	 // for is_not_all_stagnant. We are also checking that the detection time is within the 
	 // latest_allowed_detection_timestep window
         for (InjectionList::iterator p_inj = injlist.begin(), inj_end = injlist.end(); p_inj != inj_end; ++p_inj) {
            Injection& inj = **p_inj;
            int n = model_.NodeID(inj.NodeName());
            int nt_idx_stop = n*model_.n_steps + inj.StopTimestep(model_);
            int nt_idx_start = n*model_.n_steps + inj.StartTimestep(model_);
            for (int nt_idx = nt_idx_start; nt_idx <= nt_idx_stop; ++nt_idx) {
               if (model_.D[nt_idx]) {
                  is_not_all_stagnant = true;
                  break;
               }
            }
         }
         if (!is_not_all_stagnant) {
            // copies the scenario pointer into the discarded list
            discarded_scenarios_.push_back(*p_scen);
         }
         else if (detection_timestep <= latest_allowed_detection_timestep) {
            detected_scenarios_.push_back(*p_scen);
            timestep_to_detected_scenarios_[detection_timestep].push_back(*p_scen);
         }
         else {
            non_detected_scenarios_.push_back(*p_scen);
            // warn about late detections
            (detection_timestep <= final_timestep) && late_detection_warning_cnt++;
         }
      }
      if (!has_detection) {
         std::cerr << std::endl;    
         std::cerr << "WARNING: No scenarios with a detection time were found." << std::endl;
         std::cerr << std::endl; 
      }
   }
   if (late_detection_warning_cnt) {
      if (!disable_warnings_) {
         std::cerr << std::endl;    
         std::cerr << "WARNING: Scenarios were detected late within the simulation period. The detection\n" << 
            "         time was too close to the final simulation time of the water quality model\n" <<
            "         to simulate the allotted time after detection for each scenario. This situation\n" <<
            "         can be avoided by increasing the simulation period for the water quality model\n" <<
            "         in the EPANET input file or by using the '--simulation-duration-minutes' command-\n" <<
            "         line option.\n" <<
            "         Number of late detections: " << late_detection_warning_cnt << "\n";
         std::cerr << std::endl; 
      }
   }
   if (!discarded_scenarios_.empty()) {
      if (!disable_warnings_) {
         std::cerr << std::endl;    
         std::cerr << "WARNING: Some scenarios were discarded from the original scenario list because\n" << 
            "         they consisted entirely of injections during stagnant flow. Merlion does not support\n" <<
            "         injections into nodes during stagnant flow. See " << logfilename_ << "\n" <<
            "         for a detailed report of which scenarios were discarded.\n";
         std::cerr << std::endl;
      }
   }

   std::cout << "\n";
   std::cout << "For " << sensor_node_ids_.size() <<     " sensor node(s)" << std::endl;
   std::cout << "INJECTION SCENARIOS PROVIDED:     " << injection_scenarios_.size() << std::endl;
   std::cout << "DISCARDED DUE TO NO FLOW:         " << discarded_scenarios_.size() << std::endl;
   std::cout << "INJECTIONS SIMULATED:             " << injection_scenarios_.size()-discarded_scenarios_.size() << std::endl;
   std::cout << "INJECTIONS DETECTED:              " << detected_scenarios_.size() << " (" << ((double)(detected_scenarios_.size()))/((double)(injection_scenarios_.size()-discarded_scenarios_.size()))*100.0 << " %)" << std::endl;
   std::cout << "NON-DETECTED COUNT:               " << non_detected_scenarios_.size() << std::endl;
   std::cout << "UNIQUE DETECTION TIMES:           " << timestep_to_detected_scenarios_.size() << std::endl;

   // Log some useful debugging info
   if (logfile_.is_open()) {
      // log discarded scenarios
      logfile_ << "Summary of events discarded from scenario list:\n";
      logfile_ << "Discarded Count: " << discarded_scenarios_.size() << "\n";
      for (InjScenList::iterator p_scen = discarded_scenarios_.begin(), scen_end = discarded_scenarios_.end(); p_scen != scen_end; ++p_scen) {
         (*p_scen)->Print(logfile_,model_);
      }
      logfile_ << ";\n"; //mark the end of this section
      logfile_ << std::endl << std::endl; //flush the buffer

      // log non-detected scenarios
      logfile_ << "Summary of events not-detected:\n";
      logfile_ << "Non-Detected Count: " << non_detected_scenarios_.size() << "\n";
      for (InjScenList::iterator p_scen = non_detected_scenarios_.begin(), scen_end = non_detected_scenarios_.end(); p_scen != scen_end; ++p_scen) {
         (*p_scen)->Print(logfile_,model_);
      }
      logfile_ << ";\n"; //mark the end of this section
      logfile_ << std::endl << std::endl; //flush the buffer
   }
}


void NetworkSimulator::ReadINPFile(std::string inp_file, 
                                   float duration_min, 
                                   float qstep_minutes,
                                   std::string rpt_file, 
                                   std::string merlion_save_file,
                                   float scale/*=-1.0*/,
                                   int seed/*=-1*/, 
                                   bool ignore_merlion_warnings/*=false*/,
                                   float decay_k/*=0.0f*/)
{
   ResetModel();
   Merlion *merlionModel = merlionUtils::CreateMerlionFromEpanet(const_cast<char*>(inp_file.c_str()), 
                                                                 const_cast<char*>(rpt_file.c_str()), 
                                                                 (char*)"", 
                                                                 duration_min, 
                                                                 qstep_minutes,
                                                                 scale,
                                                                 seed,
                                                                 ignore_merlion_warnings, 
                                                                 decay_k);
   if (!merlionModel) {
      std::cerr << std::endl;
      std::cerr << "Error creating the Merlion object from the EPANET .inp file." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   model_.GetMerlionMembers(merlionModel);

   if (merlion_save_file != "") {
      std::ofstream out(merlion_save_file.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios::binary);
      out.setf(std::ios::scientific,std::ios::floatfield);
      out.precision(12);
      merlionModel->PrintWQM(out,true);
      out.close();
   }
   // model_.clear() will call delete on merlionModel
}

void NetworkSimulator::ReadWQMFile(std::string fname, bool binary/*=true*/)
{
   ResetModel();
   std::ifstream in;
   Merlion *merlionModel(new Merlion);
   if (binary) {
      in.open(fname.c_str(), std::ios::in|std::ios::binary);
   }
   else {
      in.open(fname.c_str(), std::ios::in);
   }
   merlionModel->ReadWQM(in,binary);
   in.close();
   model_.GetMerlionMembers(merlionModel);
   // model_.clear() will call delete on merlionModel
}

void NetworkSimulator::ReadTSGFile(std::string fname, bool append/*=false*/)
{
   if (append == false) {
      ResetScenarios();
   }

   ReadTSG(fname, model_.merlionModel, injection_scenarios_);

   // Log some useful debugging info
   if (logfile_.is_open()) {
      logfile_ << "TSG Reader Stats:\n";
      logfile_ << "Number of scenarios defined = " << injection_scenarios_.size() << "\n";
      for (InjScenList::iterator p_scen = injection_scenarios_.begin(), scen_end = injection_scenarios_.end(); p_scen != scen_end; ++p_scen) {
         (*p_scen)->Print(logfile_,model_);
      }
      logfile_ << ";\n"; //mark the end of this section
      logfile_ << std::endl << std::endl; //flush the buffer
   }

   ValidateScenarios();
}

void NetworkSimulator::ReadTSIFile(std::string fname, bool append/*=false*/, int species_id/*=-1*/)
{
   if (append == false) {
      ResetScenarios();
   }

   ReadTSI(fname, model_.merlionModel, injection_scenarios_, species_id);

   // Log some useful debugging info
   if (logfile_.is_open()) {
      logfile_ << "TSI Reader Stats:\n";
      logfile_ << "Number of scenarios defined = " << injection_scenarios_.size() << "\n";
      for (InjScenList::iterator p_scen = injection_scenarios_.begin(), scen_end = injection_scenarios_.end(); p_scen != scen_end; ++p_scen) {
         (*p_scen)->Print(logfile_,model_);
      }
      logfile_ << ";\n"; //mark the end of this section
      logfile_ << std::endl << std::endl; //flush the buffer
   }

   ValidateScenarios();
}

void NetworkSimulator::ReadSensorFile(std::string fname)
{
   assert(model_.merlionModel != NULL);
   ResetSensorData();

   std::string error_msg = "Invalid sensor file format for file: ";
   error_msg += fname;

   std::ifstream in;
   in.open(fname.c_str(), std::ios::in);

   std::string tag;
   while (in.good()) {
      in >> tag;
      // check for a tuple defining a grab sample and not a sensor
      if (tag.find("(") != std::string::npos) {
         // check that the closing bracket exists in the current string
         // meaning no white space can be present in a tuple
         if (tag.find(")") == std::string::npos) {
            std::cerr << std::endl << error_msg << std::endl;
            std::cerr << "Invalid tuple representation in sensor file. A \"(\" character was found" << std::endl;
            std::cerr << "but no \")\" character found before white space was encountered." << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         const int time = atoi(tag.substr(tag.find(",")+1,tag.find(")")-tag.find(",")-1).c_str());
         const int timestep = 
            merlionUtils::SecondsToNearestTimestepBoundary(time*60.0, model_.qual_step_minutes);
         if ((timestep <= -1) || (timestep >= model_.n_steps)) {
            std::cerr << std::endl << error_msg << std::endl;
            std::cerr << "Timestep: " << timestep << "(" << time << " s) is not within the model simulation horizon." << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         std::string name = tag.substr(tag.find("(")+1,tag.find(",")-tag.find("(")-1);
         // check that the name is in the list of node names for the model
	 if (!model_.isNode(name)) {
	    std::cerr << std::endl << error_msg << std::endl;
            std::cerr << "Sensor File ERROR: Invalid node name: " << name << std::endl;
            std::cerr << std::endl;
            exit(1);
	 }
	 int node_id = model_.NodeID(name);
         bool in_vector=false;
         for(std::vector<std::pair<int, int> >::iterator pos=grab_sample_ids_.begin(),end=grab_sample_ids_.end();pos!=end && !in_vector;pos++)
         {
            int pos_name=(*pos).first;
            int pos_time=(*pos).second;
            if(pos_name==node_id && pos_time==timestep){in_vector=true;}
         }
         if(!in_vector){grab_sample_ids_.push_back(std::pair<int,int>(node_id,timestep));}
      }
      else {
         // check that the name is in the list of node names for the model
	 if (!model_.isNode(tag)) {
	    std::cerr << std::endl << error_msg << std::endl;
            std::cerr << "Sensor File ERROR: Invalid node name: " << tag << std::endl;
            std::cerr << std::endl;
            exit(1);
	 }
	 int node_id = model_.NodeID(tag);
         if (std::count(sensor_node_ids_.begin(), sensor_node_ids_.end(), node_id) == 0) {
            sensor_node_ids_.push_back(node_id);
         }
      }
   }

   // Log some useful debugging info
   if (logfile_.is_open()) {
      logfile_ << "Sensor File Reader Stats:\n";
      logfile_ << "Number of sensors given = " << sensor_node_ids_.size() << "\n";
      for (int i = 0; i < (int)sensor_node_ids_.size(); ++i) {
         logfile_ << model_.NodeName(sensor_node_ids_[i]) << std::endl; 
      }
      logfile_ << "Number of grab sample (node/timestep) tuples given = " << grab_sample_ids_.size() << "\n";
      for (int i = 0; i < (int)grab_sample_ids_.size(); ++i) {
         logfile_ << model_.NodeName(grab_sample_ids_[i].first) << " " << grab_sample_ids_[i].second << std::endl;
      }
      logfile_ << ";\n"; //mark the end of this section
      logfile_ << std::endl << std::endl; //flush the buffer
   }
   in.close();
}

void NetworkSimulator::WriteDetectedSCNFile(std::ostream& out) const
{
   int orig_precision = out.precision();
   out.precision(8);
   for (InjScenList::const_iterator pos = injection_scenarios_.begin(), stop = injection_scenarios_.end(); pos != stop; ++pos) {
      const InjScenario& scen = **pos;       
      out << scen.Name() << " ";
      out << scen.Injections().size() << " ";
      if (scen.isDetected()) {
	 out << scen.DetectionTimeSeconds();
      }
      else {
         out << "n ";
      }
      std::map<std::string,float> impacts;
      scen.CopyImpacts(impacts);
      for (std::map<std::string,float>::const_iterator pos = impacts.begin(), stop = impacts.end(); pos != stop; ++pos) {
         out << pos->first << "=" << pos->second << " ";
      }
      out << "\n";
      for (InjectionList::const_iterator pos_inj = scen.Injections().begin(), stop_inj = scen.Injections().end(); pos_inj != stop_inj; ++pos_inj) {
         const Injection& inj = **pos_inj;
         out << inj.NodeName() << " ";
         out << InjTypeToString(inj.Type()) << " "; 
         out << inj.Strength() << " ";
         out << inj.StartTimeSeconds() << " ";
         out << inj.StopTimeSeconds() << "\n";
      }
      out << "\n";
   }
   out.precision(orig_precision);
}

void NetworkSimulator::CopyScenarioList(const NetworkSimulator& net, bool append/*=false*/)
{
   if (append == false) {
      ResetScenarios();
   }

   const InjScenList& new_list = net.InjectionScenarios();
   for (InjScenList::const_iterator p_scen = new_list.begin(), stop=new_list.end(); p_scen != stop; ++p_scen) {
      const InjScenario& s_orig = **p_scen;
      PInjScenario s_copy(new InjScenario);
      // By calling DeepCopy we are copying the InjScenario instance as well
      // as each of the Injection instances in its injection list. Simply doing
      // as shallow copy of the injection list (which consists of smart pointers)
      // would result in both InjScenarios reference the same Injection objects
      // in their injection lists.
      s_copy->DeepCopy(s_orig);
      injection_scenarios_.push_back(s_copy);
   }

   ValidateScenarios();
}

void NetworkSimulator::ReadScenarioWeightsFile(std::string fname)
{
   if (injection_scenarios_.empty()) {
      std::cerr << std::endl;
      std::cerr << "ReadScenarioWeightsFile ERROR: Unable to load weights with an empty list of scenarios" << std::endl;
      std::cerr << std::endl;
      exit(1);
   }  

   std::ifstream in (fname.c_str(), std::ios::in);   
   if(!(in.is_open() && in.good()))
   {
      std::cerr << std::endl;    
      std::cerr << "ReadScenarioWeightsFile ERROR: Failed to open file: " << fname << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   std::string tmp;
   double weight(0.0);
   InjScenList::iterator pinj = injection_scenarios_.begin();   
   while(in >> tmp) {
      weight = atof(tmp.c_str());
      if (weight < 0.0) {
         std::cerr << std::endl;    
         std::cerr << "ReadScenarioWeightsFile ERROR: Weights must be nonnegative but found " << weight << std::endl;
         std::cerr << std::endl;
         exit(1);         
      }
      (*pinj)->AddImpact(weight,"Weight");
      ++pinj;
   }
  
   if (pinj != injection_scenarios_.end()) {
      std::cerr << std::endl;    
      std::cerr << "ReadScenarioWeightsFile ERROR: The number of weights does not match the number of scenarios" << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

}

void NetworkSimulator::ReadSCNFile(std::string fname, bool append/*=false*/)
{
   if (append == false) {
      ResetScenarios();
   }

   std::ifstream file (fname.c_str(), std::ios::in);   
   if(!(file.is_open() && file.good()))
   {
      std::cerr << std::endl;    
      std::cerr << "ReadSCNFile ERROR: Failed to open file: " << fname << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   
   const int max_chars_per_line = 512;
   char *buffer(new char[max_chars_per_line]);
   std::vector<std::string> tokens;
   int n_tokens;

   while(file.good()) {
      buffer[0] = '\0';
      file.getline(buffer, max_chars_per_line);
      tokens = tokenize(buffer, " \t\r\n");
      n_tokens = tokens.size();

      //allow for empty lines
      if (n_tokens == 0) {
	 continue;
      } // and allow for comments
      else if (tokens[0][0] == ';') {
	 continue;
      }

      PInjScenario ptmp_scen(new InjScenario);
      int injection_count = 0;
      if (n_tokens >= 2) {
	 ptmp_scen->Name()=tokens[0];
	 injection_count=atoi(tokens[1].c_str());
      }
      if (n_tokens >= 3) {
	 // this scenario has been tagged with a detection time
         if (tokens[2] != "n") {
            if ((!isdigit(tokens[2][0])) || (tokens[2][0] == '.')) {
               std::cerr << std::endl;      
               std::cerr << "ReadSCNFile ERROR: Bad scenario detection time:" << tokens[2] << std::endl;
               std::cerr << "                   Token must me numeric or \'n\'" << std::endl;
               std::cerr << std::endl;
               exit(1);
            }
            double detect_time_seconds = atof(tokens[2].c_str());
            int detect_timestep = 
               merlionUtils::SecondsToNearestTimestepBoundary(detect_time_seconds, model_.qual_step_minutes);         
            if(detect_timestep < 0 || detect_timestep > model_.n_steps) {
               std::cerr << std::endl;      
               std::cerr << "ReadSCNFile ERROR: Bad scenario detection time: " << tokens[2] << std::endl;
               std::cerr << std::endl;
               exit(1);
            }
            ptmp_scen->SetDetectionTimeSeconds(detect_time_seconds);
         }
      }
      if (n_tokens >= 4) {
	 // this scenario has been tagged with impacts
         for (int token_id = 3; token_id < n_tokens; ++token_id) {
            const std::string token = tokens[token_id];
            if (token.find('=') != std::string::npos) {
               std::string name = token.substr(0,token.find('='));
               double impact = atof(token.c_str());
               ptmp_scen->AddImpact(impact, name);
            }
            else {
               std::cerr << std::endl;      
               std::cerr << "ReadSCNFile ERROR: Bad scenario tag: " << token << std::endl;
               std::cerr << "                   Format is <tagname>=<number>" << std::endl;
               std::cerr << std::endl;
               exit(1);
            }
         }
      }
      if (n_tokens < 2){
	 std::cerr << std::endl;
	 std::cerr << "ReadSCNFile ERROR: Invalid format for file " << fname << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }

      for(int j = 0; j < injection_count; ) {
	 buffer[0] = '\0';
         file.getline(buffer, max_chars_per_line);
         tokens = tokenize(buffer, " \t\r\n");
         n_tokens = tokens.size();
	 
	 //allow for empty lines
	 if (n_tokens == 0) {
	    continue;
	 } // and allow for comments
	 else if (tokens[0][0] == ';') {
	    continue;
	 }
	 ++j;

         if (n_tokens != 5) {
	    std::cerr << std::endl;
            std::cerr << "ReadSCNFile ERROR: Bad Injection specification for file " << fname <<", in scenario: " << ptmp_scen->Name() << std::endl;
	    std::cerr << std::endl;
            exit(1);
         }

         std::string tok=tokens[1];
         
         if (StringToInjType(tok) == InjType_UnDef) {            
	    std::cerr << std::endl;
	    std::cerr << "ReadSCNFile ERROR: Bad injection type: " << tok << std::endl;
	    std::cerr << std::endl;
            exit(1);
         }
         
         tok = tokens[n_tokens-2];
         
         double start_time_seconds = atof(tok.c_str());
         int start_timestep = 
            merlionUtils::Injection::SecondsToTimestep(start_time_seconds, model_.qual_step_minutes);         
         if(start_timestep < 0 || start_timestep > model_.n_steps) {
            std::cerr << std::endl;      
            std::cerr << "ReadSCNFile ERROR: Bad injection start time: " << tok << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         
         tok = tokens[n_tokens-1];
         double end_time_seconds = atof(tok.c_str());
         int end_timestep = 
            merlionUtils::Injection::SecondsToTimestep(end_time_seconds, model_.qual_step_minutes);         
         if(end_timestep < 0 || end_timestep > model_.n_steps) {
            std::cerr << std::endl;
            std::cerr << "ReadSCNFile ERROR: Bad injection stop time: " << tok << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         
         PInjection tmp_tox(new Injection);
         tmp_tox->NodeName() = std::string(tokens[0]);
         tmp_tox->Type() = StringToInjType(std::string(tokens[1]));
         tmp_tox->Strength() = atof(std::string(tokens[2]).c_str()); // g or g/m^3
         tmp_tox->StartTimeSeconds() = start_time_seconds;
         tmp_tox->StopTimeSeconds() = end_time_seconds;
         ptmp_scen->Injections().push_back(tmp_tox);
      }
      injection_scenarios_.push_back(ptmp_scen);
   }
   
   // Log some useful debugging info
   if (logfile_.is_open()) {
      logfile_ << "SCN Reader Stats:\n";
      logfile_ << "Number of scenarios defined = " << injection_scenarios_.size() << "\n";
      for (InjScenList::iterator p_scen = injection_scenarios_.begin(), scen_end = injection_scenarios_.end(); p_scen != scen_end; ++p_scen) {
         (*p_scen)->Print(logfile_,model_);
      }
      logfile_ << ";\n"; //mark the end of this section
      logfile_ << std::endl << std::endl; //flush the buffer
   }
   
   file.close();

   delete [] buffer;
   buffer = NULL;
   
   ValidateScenarios();   
}

void NetworkSimulator::ValidateScenarios()
{
   typedef std::map<PInjScenario,std::list<int> > scen_map;
   scen_map modified_scenarios;
   for (InjScenList::iterator p_scen = injection_scenarios_.begin(), scen_stop = injection_scenarios_.end(); p_scen != scen_stop; ++p_scen) {
      InjectionList& injlist = (*p_scen)->Injections();
      if ((*p_scen)->isDetected()) {
         int detection_timestep = (*p_scen)->DetectionTimestep(model_);
         if (detection_timestep < 0 || detection_timestep >= model_.n_steps) {
            std::cerr << std::endl;
            std::cerr << "ERROR: Scenario detection timestep is not within simulation period.\n";
            std::cerr << std::endl;
            exit(1);
         }
      }
      if ((*p_scen)->Injections().empty()) {
         std::cerr << std::endl;
         std::cerr << "ERROR: Scenario with empty injection list was found." << std::endl;
         (*p_scen)->Print(std::cerr);
         std::cerr << std::endl;
         exit(1);
      }
      for (InjectionList::iterator p_inj = injlist.begin(), inj_stop = injlist.end(); p_inj != inj_stop; ++p_inj) {
         if (!(*p_inj)->isValid(model_)) {
            std::cerr << std::endl;
            std::cerr << "ERROR: Invalid injection detected for scenario." << std::endl;
            (*p_scen)->Print(std::cerr);
            std::cerr << std::endl;
            exit(1);
         }
         int n = model_.NodeID((*p_inj)->NodeName());
         int start_timestep = (*p_inj)->StartTimestep(model_);
         int stop_timestep = (*p_inj)->StopTimestep(model_);
         int m_end_idx = n*model_.n_steps + stop_timestep;
         int m_start_idx = n*model_.n_steps + start_timestep;
         for (int nt_idx=m_start_idx; nt_idx<=m_end_idx; ++nt_idx) {
            if (!model_.D[nt_idx]) {
               modified_scenarios[*p_scen].push_back(nt_idx);
            }
         }
      }
   }

   if (modified_scenarios.size() > 0) {
      if (!disable_warnings_) {
         std::cerr << std::endl;    
         std::cerr << "WARNING: Some scenarios will be modified because of injections occurring at nodes\n" << 
            "         during stagnant flow. Merlion does not support injections into\n" <<
            "         nodes during stagnant flow. Injections during these timesteps will be\n" <<
            "         omitted from the injection profile for that scenario. See " << logfilename_ << "\n" <<
            "         for a detailed report of which scenarios were modified.\n";
         std::cerr << std::endl;
      }
   }
   
   // Log some useful debugging info
   if (logfile_.is_open()) {
      // log discarded scenarios
      logfile_ << "Summary of scenarios modifed due to stagnant flow:\n";
      logfile_ << "Total Scenarios Modified: " << modified_scenarios.size() << "\n";
      for (scen_map::iterator p_scen = modified_scenarios.begin(), scen_end = modified_scenarios.end(); p_scen != scen_end; ++p_scen) {
         logfile_ << "Scenario Name: " << (p_scen->first)->Name() << "\n";
         std::list<int>& inj_ids = p_scen->second;
         for (std::list<int>::iterator p_idx = inj_ids.begin(), p_idx_end = inj_ids.end(); p_idx != p_idx_end; ++p_idx) {
            int node_id = (*p_idx) / model_.n_steps;
            int time = (*p_idx) % model_.n_steps;
            logfile_ << "Node: " << model_.NodeName(node_id) << ", Timestep: " << time << "\n";
         }
      }
      logfile_ << ";\n"; //mark the end of this section
      logfile_ << std::endl << std::endl; //flush the buffer
   }

   modified_scenarios.clear();
}

void NetworkSimulator::GeneratePopulationUsageData(double demand_percapita_m3pmin, double ingestion_rate_m3pmin) 
{

  //std::ofstream vol_log;
  //vol_log.open("Rho_Volume.csv");

   double total_simtime_min = model_.qual_step_minutes*model_.n_steps;
   double total_usage_percapita_m3 = 
      demand_percapita_m3pmin*total_simtime_min;

   double *node_demand_m3(NULL);
   node_demand_m3 = new double[model_.n_nodes];
   BlasInitZero(model_.n_nodes, node_demand_m3);
   for (size_t i = 0, size = model_.junctions.size(); i < size; ++i) {
      int n = model_.junctions[i];
      double total_demand_m3pmin = 0.0;
      for (int t = 0; t < model_.n_steps; ++t) {
         total_demand_m3pmin += model_.demand_m3pmin[n*model_.n_steps + t];
      }
      node_demand_m3[n] = total_demand_m3pmin*model_.qual_step_minutes;
      std::cout << n << " " << node_demand_m3[n] << std::endl;
   }
   
   node_population_.clear();
   node_population_.resize(model_.n_nodes, 0);
   for (size_t i = 0, size = model_.junctions.size(); i < size; ++i) {
      int n = model_.junctions[i];
      node_population_[n] = 
         merlionUtils::RoundToInt(node_demand_m3[n]/total_usage_percapita_m3);
   }

   volume_ingested_m3_.clear();
   volume_ingested_m3_.resize(model_.N, 0.0);
   for (size_t i = 0, size = model_.junctions.size(); i < size; ++i) {
      int n = model_.junctions[i];
      for (int t = 0; t < model_.n_steps; ++t) {
	if (node_demand_m3[n]) {
	  double consumption_ratio =
            model_.demand_m3pmin[n*model_.n_steps + t] *
            total_simtime_min /
            node_demand_m3[n];
	  volume_ingested_m3_[n*model_.n_steps+t] =
            consumption_ratio *
            ingestion_rate_m3pmin *
            model_.qual_step_minutes;
	  //vol_log<< model_.NodeName(n) << "\t" << t << "\t" << consumption_ratio << "\n"; 
	} else {
	  //vol_log<< model_.NodeName(n) << "\t" << t << "\t" << 0 << "\n"; 
	}

      }
   }
   //vol_log.close();
   delete [] node_demand_m3;
   node_demand_m3 = NULL;
}


} /* end of merlionUtils namespace */
