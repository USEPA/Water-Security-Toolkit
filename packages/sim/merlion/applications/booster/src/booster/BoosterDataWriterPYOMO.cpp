#include <booster/BoosterDataWriterPYOMO.hpp>

#include <merlionUtils/ModelWriter.hpp>


void BoosterDataWriterPYOMO::WriteWQHeadersFile(int earliest_timestep, int latest_timestep)
{
   out_HEAD.open(out_HEAD_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMHeadersToPythonFormat(out_HEAD, model_->merlionModel, node_map_, earliest_timestep, latest_timestep);
   out_HEAD.close();
}

void BoosterDataWriterPYOMO::WriteWQNodeTypesFile()
{
   out_TYPES.open(out_TYPES_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMNodeTypesToPythonFormat(out_TYPES, model_->merlionModel, node_map_);
   out_TYPES.close();
}


void BoosterDataWriterPYOMO::WriteWQModelFile(int earliest_timestep, int latest_timestep)
{
   out_WQM.open(out_WQM_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMToPythonFormat(out_WQM, model_->merlionModel, node_map_, earliest_timestep, latest_timestep);
   out_WQM.close();
}


void BoosterDataWriterPYOMO::WriteWQDemandsFile(int earliest_timestep, int latest_timestep)
{
   out_DEM.open(out_DEM_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintDemandToPythonFormat(out_DEM, model_->merlionModel, node_map_, earliest_timestep, latest_timestep);
   out_DEM.close();
}


void BoosterDataWriterPYOMO::WriteWQTankVolumesFile(int earliest_timestep, int latest_timestep)
{
   out_TANK.open(out_TANK_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintTankVolumesToPythonFormat(out_TANK, model_->merlionModel, node_map_, earliest_timestep, latest_timestep);
   out_TANK.close();
}


void BoosterDataWriterPYOMO::WriteBoosterWQHeadersFile(int earliest_timestep, int latest_timestep)
{
   std::string fname = options_->output_prefix+"BOOSTER_"+out_HEAD_basename;
   out_HEAD.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMHeadersToPythonFormat(out_HEAD, chl_model_->merlionModel, node_map_, earliest_timestep, latest_timestep, "_BOOSTER");
   out_HEAD.close();
}


void BoosterDataWriterPYOMO::WriteBoosterWQNodeTypesFile()
{
   std::string fname = options_->output_prefix+"BOOSTER_"+out_TYPES_basename;
   out_TYPES.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMNodeTypesToPythonFormat(out_TYPES, chl_model_->merlionModel, node_map_, "_BOOSTER");
   out_TYPES.close();
}


void BoosterDataWriterPYOMO::WriteBoosterWQModelFile(int earliest_timestep, int latest_timestep)
{
   std::string fname = options_->output_prefix+"BOOSTER_"+out_WQM_basename;
   out_WQM.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMToPythonFormat(out_WQM, chl_model_->merlionModel, node_map_, earliest_timestep, latest_timestep, "_BOOSTER");
   out_WQM.close();
}


void BoosterDataWriterPYOMO::WriteBoosterWQDemandsFile(int earliest_timestep, int latest_timestep)
{
   std::string fname = options_->output_prefix+"BOOSTER_"+out_DEM_basename;
   out_DEM.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintDemandToPythonFormat(out_DEM, chl_model_->merlionModel, node_map_, earliest_timestep, latest_timestep, "_BOOSTER");
   out_DEM.close();
}


void BoosterDataWriterPYOMO::WriteBoosterWQTankVolumesFile(int earliest_timestep, int latest_timestep)
{
   std::string fname = options_->output_prefix+"BOOSTER_"+out_TANK_basename;
   out_TANK.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintTankVolumesToPythonFormat(out_TANK, chl_model_->merlionModel, node_map_, earliest_timestep, latest_timestep, "_BOOSTER");
   out_TANK.close();
}


void BoosterDataWriterPYOMO::WriteBoosterCandidatesFile(int earliest_timestep, int latest_timestep)
{
   // write the list of booster candidate nodes being used
   out_BC.open(out_BC_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_BC << "S_BOOSTER_CANDIDATES = [";
   int BOOSTER_CANDIDATES_SIZE = chl_net_->BoosterCandidates().size();
   for (int i=0; i<BOOSTER_CANDIDATES_SIZE; i++) {
      out_BC << node_map_[chl_net_->BoosterCandidates()[i]] << ",";
   }
   out_BC << "]\n";
   out_BC.close();
}


void BoosterDataWriterPYOMO::WriteBoosterInjectionsFile(int earliest_timestep, int latest_timestep)
{
   out_CHL.open(out_CHL_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_CHL << "# Booster injection strength\n";
   out_CHL << "# Units are [g/min] for\n";
   out_CHL << "P_INJ_STRENGTH_BOOSTER_gpmin = " << chl_net_->BoosterInjectionStrength() << "\n\n";
   out_CHL << "# This will change depending on whether we are using \n" <<
               "# MASS or FLOWPACED injections.\n";
   if (chl_net_->BoosterInjectionType() == merlionUtils::InjType_Flow) {
      out_CHL << "# In the current file this represents FLOWPACED type injections.\n";
      out_CHL << "P_INJ_TYPE_MULTIPLIER_BOOSTER = {}\n";
      for STL_CONST_ITERATE(std::vector<int>, p_bid, p_bid_end, chl_net_->BoosterCandidates()) {
         int n = *p_bid;
         assert(chl_model_->merlionModel->NodeIdxTypeMap()[n] != NodeType_Tank);
         for (int t = earliest_timestep; t <= latest_timestep; ++t) {
            out_CHL << "P_INJ_TYPE_MULTIPLIER_BOOSTER[" << node_map_[n] << "," << t << "] = " << chl_model_->flow_m3pmin[n*chl_model_->n_steps+t] << "\n";
         }
      }
   }
   else if (chl_net_->BoosterInjectionType() == merlionUtils::InjType_Mass) {
      out_CHL << "# In the current file this represents MASS type injections.\n";
      out_CHL << "P_INJ_TYPE_MULTIPLIER_BOOSTER = \n";
      for STL_CONST_ITERATE(std::vector<int>, p_bid, p_bid_end, chl_net_->BoosterCandidates()) {
         int n = *p_bid;
         for (int t = earliest_timestep; t <= latest_timestep; ++t) {
            out_CHL << "P_INJ_TYPE_MULTIPLIER_BOOSTER[" << node_map_[n] << "," << t << "] = 1" << "\n";
         }
      }
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Invalid Injection type encountered in BoosterDataWriter" << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   out_CHL << "\n\n";


   if (chl_net_->BoosterScenarios().size()) {
      out_CHL << "P_TIMESCALE_FACTOR_BOOSTER = {}\n";
      for STL_CONST_ITERATE(BoosterScenList, pboost, boost_stop, chl_net_->BoosterScenarios()) {
         assert((*pboost)->Injections().size() == 1);
         const merlionUtils::Injection& booster_inj = *((*pboost)->Injections().front());
         int start_timestep = booster_inj.StartTimestep(*chl_model_);
         float timescale_start = booster_inj.TimeScaleStartTimestep(*chl_model_);
         int stop_timestep = booster_inj.StopTimestep(*chl_model_);
         float timescale_stop = booster_inj.TimeScaleStopTimestep(*chl_model_);
         if (stop_timestep != start_timestep) {
            out_CHL << "P_TIMESCALE_FACTOR_BOOSTER[" << start_timestep << ",\"" << (*pboost)->Name() << "\"] = " << timescale_start << "\n";
         }
         out_CHL << "P_TIMESCALE_FACTOR_BOOSTER[" << stop_timestep << ",\"" << (*pboost)->Name() << "\"] = " << timescale_stop << "\n";
      }
      out_CHL << "\n\n";
      out_CHL << "P_INJ_START_TIMESTEP_BOOSTER = {}\n";
      for STL_CONST_ITERATE(BoosterScenList, pboost, boost_stop, chl_net_->BoosterScenarios()) {
         assert((*pboost)->Injections().size() == 1);
         out_CHL << "P_INJ_START_TIMESTEP_BOOSTER[\"" << (*pboost)->Name() << "\"] = " <<
            (*pboost)->Injections().front()->StartTimestep(*chl_model_) << "\n";
      }
      out_CHL << "\n\n";
      out_CHL << "P_INJ_END_TIMESTEP_BOOSTER = {}\n";
      for STL_CONST_ITERATE(BoosterScenList, pboost, boost_stop, chl_net_->BoosterScenarios()) {
         assert((*pboost)->Injections().size() == 1);
         out_CHL << "P_INJ_END_TIMESTEP_BOOSTER[\"" << (*pboost)->Name() << "\"] = " <<
            (*pboost)->Injections().front()->StopTimestep(*chl_model_) << "\n";
      }
      out_CHL << "\n\n";
   }
   out_CHL.close();
}


void BoosterDataWriterPYOMO::WriteToxinWQHeadersFile(int earliest_timestep, int latest_timestep)
{
   std::string fname = options_->output_prefix+"TOXIN_"+out_HEAD_basename;
   out_HEAD.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMHeadersToPythonFormat(out_HEAD, tox_model_->merlionModel, node_map_, earliest_timestep, latest_timestep, "_TOXIN");
   out_HEAD.close();
}


void BoosterDataWriterPYOMO::WriteToxinWQNodeTypesFile()
{
   std::string fname = options_->output_prefix+"TOXIN_"+out_TYPES_basename;
   out_TYPES.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMNodeTypesToPythonFormat(out_TYPES, tox_model_->merlionModel, node_map_, "_TOXIN");
   out_TYPES.close();
}


void BoosterDataWriterPYOMO::WriteToxinWQModelFile(int earliest_timestep, int latest_timestep)
{
   std::string fname = options_->output_prefix+"TOXIN_"+out_WQM_basename;
   out_WQM.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMToPythonFormat(out_WQM, tox_model_->merlionModel, node_map_, earliest_timestep, latest_timestep, "_TOXIN");
   out_WQM.close();
}


void BoosterDataWriterPYOMO::WriteToxinWQDemandsFile(int earliest_timestep, int latest_timestep)
{
   std::string fname = options_->output_prefix+"TOXIN_"+out_DEM_basename;
   out_DEM.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintDemandToPythonFormat(out_DEM, tox_model_->merlionModel, node_map_, earliest_timestep, latest_timestep, "_TOXIN");
   out_DEM.close();
}


void BoosterDataWriterPYOMO::WriteToxinWQTankVolumesFile(int earliest_timestep, int latest_timestep)
{
   std::string fname = options_->output_prefix+"TOXIN_"+out_TANK_basename;
   out_TANK.open(fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintTankVolumesToPythonFormat(out_TANK, tox_model_->merlionModel, node_map_, earliest_timestep, latest_timestep, "_TOXIN");
   out_TANK.close();
}


void BoosterDataWriterPYOMO::WriteToxinInjectionsFile()
{
   // **NOTE: We use chl_net_ and chl_model_ below because it defines
   //         the booster scenarios holding the potentially shortened
   //         versions of the toxin injections

   out_TOX.open(out_TOX_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_TOX << "# Toxin mass injection profile [g/min]\n"; 
   out_TOX << "P_MN_TOXIN_gpmin = {}\n";
   typedef std::map<int,float> map_int_to_float;
   std::vector<map_int_to_float> toxin_rhs_gpmin;
   toxin_rhs_gpmin.resize(chl_net_->BoosterScenarios().size());
   std::vector<map_int_to_float >::iterator p_tox = toxin_rhs_gpmin.begin();
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net_->BoosterScenarios()) {
      const InjScenList& toxin_scenarios = (*p_boost)->ToxinScenarios();
      map_int_to_float& toxin_rhs = *p_tox++;
      for STL_CONST_ITERATE(InjScenList, p_tox, p_tox_stop, toxin_scenarios) {
         (*p_tox)->SetArray(*chl_model_,toxin_rhs);
      }
      for STL_CONST_ITERATE(map_int_to_float, pos, end, toxin_rhs) {
         const int nt_idx = chl_model_->perm_upper_to_nt[pos->first];
         const int node_id = (nt_idx)/chl_model_->n_steps;
         const int time_id = (nt_idx)%chl_model_->n_steps;
         out_TOX << "P_MN_TOXIN_gpmin[" << node_map_[node_id] << "," 
            << time_id << "," 
            << "\"" << (*p_boost)->Name() << "\"] = " 
            << pos->second << "\n";
      }
   }
   out_TOX << "\n";
   out_TOX.close();
}


void BoosterDataWriterPYOMO::WriteScenarioSummaryFile()
{
   out_SUM.open(out_SUM_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   out_SUM << "P_NUM_DETECTED_SCENARIOS = " << chl_net_->DetectedScenarios().size() << "\n";
   out_SUM << "S_SCENARIOS = [";
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net_->BoosterScenarios()) {
      out_SUM << "\"" << (*p_boost)->Name() << "\",";
   }
   out_SUM << "]\n\n";
   out_SUM << "P_SCEN_DETECT_TIMESTEP = {";
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net_->BoosterScenarios()) {
      out_SUM << "\"" << (*p_boost)->Name() << "\":" << (*p_boost)->DetectionTimestep(*chl_model_) << ","; 
   }
   out_SUM << "}\n\n";
   out_SUM << "P_SCEN_START_TIMESTEP = {";
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net_->BoosterScenarios()) {
      int first_injection_timestep(chl_model_->N-1);
      const InjScenList& toxin_scenarios = (*p_boost)->ToxinScenarios();
      for STL_CONST_ITERATE(InjScenList, p_tox, p_tox_stop, toxin_scenarios) {
	 int t = (*p_tox)->EarliestInjectionTimestep(*chl_model_);
	 if (t < first_injection_timestep) {
	    first_injection_timestep = t;
	 }
      }
      out_SUM << "\"" << (*p_boost)->Name() << "\":" << first_injection_timestep << ",";
   }
   out_SUM << "}\n\n";
   out_SUM << "P_SCEN_END_TIMESTEP = {";
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net_->BoosterScenarios()) {
      out_SUM << "\"" << (*p_boost)->Name() << "\":" << (*p_boost)->sim_stop_timestep << ",";
   }
   out_SUM << "}\n\n";
   out_SUM << "P_WEIGHT = {";
   for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net_->BoosterScenarios()) {
      out_SUM << "\"" << (*p_boost)->Name() << "\":" << (*p_boost)->Impact("Normalized Weight") << ",";
   }
   out_SUM << "}\n\n";
   out_SUM.close();
}


void BoosterDataWriterPYOMO::WriteScenario(const std::vector<std::vector<int> >& z_rows,
                                           const float* tox_conc_gpm3,
                                           const BoosterScenario& scen,
                                           bool is_first,
                                           bool summary_only)
{
   if (!summary_only) {
      out_SPCT.open(out_SPCT_fname.c_str(), std::ios_base::out | std::ios_base::app);
      out_CONT.open(out_CONT_fname.c_str(), std::ios_base::out | std::ios_base::app);
   }
   out_SUM.open(out_SUM_fname.c_str(), std::ios_base::out | std::ios_base::app);

   if (is_first) {
      out_SUM << "P_NUM_DETECTED_SCENARIOS = " << chl_net_->DetectedScenarios().size() << "\n";
      out_SUM << "P_PRE_OMITTED_OBJ_g = {}\n";
      out_SUM << "P_POST_OMITTED_OBJ_g = {}\n";
      
      if (!summary_only) {
         out_SPCT << "# mass consumed (grams)\n";
         out_SPCT << "P_IMPACT = {}\n";
         out_CONT << "S_CONTROLLER_BOOSTERS = {}\n";
      }
   }

   if (!summary_only) {
      for (int u_idx = 0; u_idx < chl_model_->N; ++u_idx) {
         if (z_rows[u_idx].empty()) {
            continue;
         }
         const std::vector<int>& CNDS = z_rows[u_idx];
         int nt_idx = chl_model_->perm_upper_to_nt[u_idx];
         int n = (nt_idx) / chl_model_->n_steps;
         int t = (nt_idx) % chl_model_->n_steps;
         out_SPCT << "P_IMPACT[" << node_map_[n] << "," << t << ",\"" << scen.Name() << "\"] = " << 
                     tox_conc_gpm3[u_idx]*chl_model_->demand_m3pmin[nt_idx]*chl_model_->qual_step_minutes << "\n";
         out_CONT << "S_CONTROLLER_BOOSTERS[" << node_map_[n] << "," << t << ",\"" << scen.Name() << "\"] = [";
         for STL_CONST_ITERATE(std::vector<int>, pnode, pnode_end, CNDS) {
            out_CONT << node_map_[*pnode] << ",";
         }
         out_CONT << "]\n";
      }
      out_SPCT.close();
      out_CONT.close();
   }

   out_SUM << "P_PRE_OMITTED_OBJ_g[\"" << scen.Name() << "\"] = " << scen.Impact("Pre-Detect Mass Consumed Grams") << "\n"; 
   out_SUM << "P_POST_OMITTED_OBJ_g[\"" << scen.Name() << "\"] = " << scen.Impact("Post-Detect Mass Consumed Grams") << "\n";
   out_SUM.close();
}


void BoosterDataWriterPYOMO::WriteReducedProblem(std::multimap<float, std::vector<int> >& reduced_z_rows)
{
   out_SPCT.open(out_SPCT_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_SPCT << "# mass consumed (grams)\n";
   out_SPCT << "P_IMPACT = {}\n";
   out_CONT.open(out_CONT_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_CONT << "S_CONTROLLER_BOOSTERS = {}\n";
   int cntr = 0;
   typedef std::multimap<float, std::vector<int> > tmp_map_type;
   for STL_CONST_ITERATE(tmp_map_type, pos, pos_end, reduced_z_rows) {
      const float& mc_g = pos->first;
      const std::vector<int>& CNDS = pos->second;
      out_SPCT << "P_IMPACT[" << cntr << "," << cntr << "," << cntr << "] = " << mc_g << "\n";
      out_CONT << "S_CONTROLLER_BOOSTERS[" << cntr << "," << cntr << "," << cntr << "] = [";
      for STL_CONST_ITERATE(std::vector<int>, pnode, pnode_end, CNDS) {
         out_CONT << node_map_[*pnode] << ",";
      }
      out_CONT << "]\n";
      ++cntr;
   }
   out_SPCT.close();
   out_CONT.close();
}

void BoosterDataWriterPYOMO::WritePopulationDosageFile(int earliest_timestep, int latest_timestep)
{
   out_POP.open(out_POP_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   std::vector<int> nnz_pop_nodes;
   if (!(tox_net_->NodePopulation().empty())) {
      for (int n = 0; n < tox_model_->n_nodes; ++n) {
         if (tox_net_->NodePopulation()[n] > 0) {
            nnz_pop_nodes.push_back(n);
         }
      }
   }
   out_POP << "S_NZP_NODES = [\n";
   for (size_t i = 0, i_stop = nnz_pop_nodes.size(); i < i_stop; ++i) {
      int n = nnz_pop_nodes[i];
      out_POP << node_map_[n] << ",";
   }
   out_POP << "]\n\n";
   out_POP << "P_NODE_POPULATION = {}\n";
   if (!nnz_pop_nodes.empty()) {
      for (size_t i = 0, i_stop = nnz_pop_nodes.size(); i < i_stop; ++i) {
         int n = nnz_pop_nodes[i];
         int population = tox_net_->NodePopulation()[n];
         out_POP << "P_NODE_POPULATION[" << node_map_[n] << "] = " << population << "\n";
      }
      out_POP << "\n\n";
   }
   else {
      out_POP << "# No nonzero node populations\n\n";
   }

   std::vector<int> nnz_vi_ids;
   if (!(tox_net_->VolumeIngested_m3().empty())) {
      for (int nt_idx = 0; nt_idx < tox_model_->N; ++nt_idx) {
         if (tox_net_->VolumeIngested_m3()[nt_idx] > 0.0) {
            int t = nt_idx % tox_model_->n_steps;
            if ((t >= earliest_timestep) && (t <= latest_timestep)) {
               nnz_vi_ids.push_back(nt_idx);
            }
         }
      }
   }
   out_POP << "P_VOLUME_INGESTED_m3 = {}\n";
   if (!nnz_vi_ids.empty()) {
      for (size_t i = 0, i_stop = nnz_vi_ids.size(); i < i_stop; ++i) {
         int nt_idx = nnz_vi_ids[i];
         int n = nt_idx / tox_model_->n_steps;
         int t = nt_idx % tox_model_->n_steps;
         out_POP << "P_VOLUME_INGESTED_m3[" << node_map_[n] << "," << t << "] = " << tox_net_->VolumeIngested_m3()[nt_idx] << "\n";
      }
      out_POP << "\n\n";
   }
   else {
      out_POP << "# No nonzero ingested volumes\n\n";
   }
   out_POP.close();
}

void BoosterDataWriterPYOMO::AppendPopulationDosageData(std::string scenario_name,
                                                        const std::vector<float>& node_total_dose_g,
                                                        bool first_time,
                                                        bool leave_open)
{
   if (first_time) {
      out_POP.open(out_POP_fname.c_str(),std::ios_base::out | std::ios_base::app);
      out_POP << "P_MAX_DOSE_g = {}\n";
   }
   for (int n = 0; n < model_->n_nodes; ++n) {
      if (node_total_dose_g[n] > 0) {
         out_POP << "P_MAX_DOSE_g[" << node_map_[n] << ",\"" << scenario_name << "\"] = " << node_total_dose_g[n] << "\n";
      }
   }
   if (!leave_open) {
      out_POP << "\n\n";
      out_POP.close();
   }
}

void BoosterDataWriterPYOMO::WriteDosageScenario(const std::vector<std::map<int,float> >& node_objective_terms,
                                                 const std::vector<float>& always_dosed_g,
                                                 const std::vector<std::map<std::vector<int>, int>::const_iterator>& new_cols,
                                                 const InjScenario& toxin_scen,
                                                 float scaling,
                                                 bool is_first)
{
   if (is_first) {
      out_SUM.open(out_SUM_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
      out_SUM << "P_NUM_DETECTED_SCENARIOS = " << chl_net_->DetectedScenarios().size() << "\n";
      out_SUM << "P_WEIGHT = {}\n";
      for STL_CONST_ITERATE(BoosterScenList, p_boost, p_boost_stop, chl_net_->BoosterScenarios()) {
         for STL_CONST_ITERATE(InjScenList, p_tox_scen, p_tox_stop, ((*p_boost)->ToxinScenarios())) {
            out_SUM << "P_WEIGHT[\"" << (*p_tox_scen)->Name() << "\"] = " << (*p_tox_scen)->Impact("Normalized Weight") << "\n";
         }
      }
      out_SUM << "\n";
      out_SUM << "P_ALWAYS_DOSED_g = {}\n";

      out_SCL.open(out_SCL_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
      out_SCL << "P_INJECTION_SCALING = {}\n";

      out_SPCT.open(out_SPCT_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      out_SPCT << "# mass dosed (grams)\n";
      out_SPCT << "P_IMPACT = {}\n";
      out_CONT.open(out_CONT_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      out_CONT << "S_CONTROLLER_BOOSTERS = {}\n";
      out_REDM.open(out_REDM_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      out_REDM << "S_REDUCED_MAP = {}\n";
   }
   else {
      out_SUM.open(out_SUM_fname.c_str(), std::ios_base::out | std::ios_base::in);
      out_SUM.seekp(-1, std::ios::end);

      out_SCL.open(out_SCL_fname.c_str(), std::ios_base::out | std::ios_base::in);
      out_SCL.seekp(0, std::ios::end);
      
      out_SPCT.open(out_SPCT_fname.c_str(), std::ios_base::out | std::ios_base::in);
      out_SPCT.seekp(-1, std::ios::end);
      
      out_CONT.open(out_CONT_fname.c_str(), std::ios_base::out | std::ios_base::in);
      out_CONT.seekp(0, std::ios::end);

      out_REDM.open(out_REDM_fname.c_str(), std::ios_base::out | std::ios_base::in);
      out_REDM.seekp(0, std::ios::end);
   }


   typedef std::map<int,float> tmp_map_type;
   for (int n = 0; n < chl_model_->n_nodes; ++n) {
      const std::map<int,float>& terms = node_objective_terms[n];
      if (!terms.empty()) {
         out_REDM << "S_REDUCED_MAP[" << node_map_[n] << ",\"" << toxin_scen.Name() << "\"] = [";
         for STL_CONST_ITERATE(tmp_map_type, pos, pos_end, terms) {
               out_SPCT << "P_IMPACT[" << node_map_[n] << ",\"" << toxin_scen.Name() << "\"," << pos->first << "] = "
                     << pos->second << "\n";
            out_REDM << pos->first << ",";
         }
         out_REDM << "]\n";
      }
   }
   out_SPCT << "\n";
   out_SPCT.close();
   out_REDM.close();

   int new_max_index = 0;
   typedef std::vector<std::map<std::vector<int>, int>::const_iterator> tmp_type;
   for STL_CONST_ITERATE(tmp_type, pos, pos_end, new_cols) {
      int i = (*pos)->second;
      const std::vector<int>& controller_nodes = (*pos)->first;
      if (i > new_max_index) {
         new_max_index = i;
      }
      out_CONT << "S_CONTROLLER_BOOSTERS[" << i << "] = [";
      for STL_CONST_ITERATE(std::vector<int>, pnode, pnode_end, controller_nodes) {
         out_CONT << node_map_[*pnode] << ",";
      }
      out_CONT << "]\n";
   }
   if (new_max_index > 0) {
      out_DIND.open(out_DIND_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      out_DIND << "S_DELTA_INDEX = range(1," << new_max_index+1 << ")\n";
      out_DIND.close();
   }
   out_CONT.close();

   out_SCL << "P_INJECTION_SCALING[\"" << toxin_scen.Name() << "\"] = "
           << scaling << "\n";
   out_SCL.close();

   for (int n = 0; n < chl_model_->n_nodes; ++n) {
      if (always_dosed_g[n] > 0) {
         out_SUM << "P_ALWAYS_DOSED_g[" << node_map_[n] << ",\""
                 << toxin_scen.Name() << "\"] = "
                 << always_dosed_g[n] << "\n";
      }
   }
   out_SUM << "\n";
   out_SUM.close();
}
