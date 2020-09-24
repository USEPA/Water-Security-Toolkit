#include <booster/BoosterDataWriterPYSP.hpp>

#include <merlionUtils/ModelWriter.hpp>

/*

void BoosterDataWriterPYSP::WritePreSimulationFilesPYSP()
{
   pysp_write_scenario_structure();
   
   out_PYSP.open(std::string(options_->output_prefix+"RootNode.dat").c_str(), std::ios_base::out | std::ios_base::trunc);
   out_PYSP << "include " << out_BC_fname << ";\n";
   out_PYSP << "include " << out_CHL_WQM_fname << ";\n";
   out_PYSP << "include " << out_TOX_WQM_fname << ";\n";
   out_PYSP << "include " << out_DEM_fname << ";\n";
   out_PYSP.close();
   const BoosterScenList& scen_list = chl_net_->BoosterScenarios();
   pysp_write_scenario_limit_model(*(scen_list.front()), "ReferenceModel.dat", true);
   for STL_CONST_ITERATE(BoosterScenList, scen, stop, scen_list) {
      pysp_write_scenario_limit_model(**scen, std::string(options_->output_prefix+"SCEN"+(*scen)->Name()+"Node.dat"), false);
   }
}
*/

void BoosterDataWriterPYSP::pysp_write_scenario_limit_model(const BoosterScenario& scen, std::string fname, bool include_root)
{
   int latest_timestep = scen.sim_stop_timestep;
   int earliest_timestep(chl_model_->N-1);
   const InjScenList& toxin_scenarios = scen.ToxinScenarios();
   for STL_CONST_ITERATE(InjScenList, p_tox, p_tox_stop, toxin_scenarios) {
      int t = (*p_tox)->EarliestInjectionTimestep(*chl_model_);
      if (t < earliest_timestep) {
	 earliest_timestep = t;
      }
   }
   
   out_PYSP.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_PYSP << "param P_SCEN_DETECT_TIMESTEP := " << scen.DetectionTimestep(*chl_model_) << ";\n";
   out_PYSP << "param P_SCEN_START_TIMESTEP := " << earliest_timestep << ";\n";
   out_PYSP << "param P_SCEN_END_TIMESTEP := " << latest_timestep << ";\n";
   out_PYSP << "\n";

   if (include_root) {
      out_PYSP << "include "+options_->output_prefix+"RootNode.dat;\n";
   }

   out_PYSP << "\n\n";
   out_PYSP << "# Booster injection strength\n";
   out_PYSP << "# Units are [g/min] for\n";
   out_PYSP << "param P_CHL_STRENGTH_gpmin := " << chl_net_->BoosterInjectionStrength() << ";\n\n";
   out_PYSP << "# This will change depending on whether we are using \n" <<
      "# MASS or FLOWPACED injections.\n";
   if (chl_net_->BoosterInjectionType() == merlionUtils::InjType_Flow) {
      out_PYSP << "# In the current file this represents FLOWPACED type injections.\n";
      out_PYSP << "param P_CHL_INJ_TYPE_MULTIPLIER := \n";
      for STL_CONST_ITERATE(std::vector<int>, p_bid, p_bid_end, chl_net_->BoosterCandidates()) {
         int n = *p_bid;
         assert(chl_model_->merlionModel->NodeIdxTypeMap()[n] != NodeType_Tank);
         for (int t = earliest_timestep; t <= latest_timestep; ++t) {
            out_PYSP << node_map_[n] << " " << t << " " << chl_model_->flow_m3pmin[n*chl_model_->n_steps+t] << "\n";
         }
      }
   }
   else if (chl_net_->BoosterInjectionType() == merlionUtils::InjType_Mass) {
      out_PYSP << "# In the current file this represents MASS type injections.\n";
      out_PYSP << "param P_CHL_INJ_TYPE_MULTIPLIER := \n";
      for STL_CONST_ITERATE(std::vector<int>, p_bid, p_bid_end, chl_net_->BoosterCandidates()) {
         int n = *p_bid;
         for (int t = earliest_timestep; t <= latest_timestep; ++t) {
            out_PYSP << node_map_[n] << " " << t << " 1" << "\n";
         }
      }
   }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Invalid Injection type encountered in BoosterDataWriter" << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   out_PYSP << ";\n\n";


   assert(scen.Injections().size() == 1);
   const merlionUtils::Injection& booster_inj = *(scen.Injections().front());
   int booster_start_timestep = booster_inj.StartTimestep(*chl_model_);
   float timescale_start = booster_inj.TimeScaleStartTimestep(*chl_model_);
   int booster_stop_timestep = booster_inj.StopTimestep(*chl_model_);
   float timescale_stop = booster_inj.TimeScaleStopTimestep(*chl_model_);
   out_PYSP << "param P_TIMESCALE_FACTOR_BOOSTER :=\n";
   if (booster_stop_timestep != booster_start_timestep) {
      out_PYSP << booster_start_timestep << " " << timescale_start << "\n";
   }
   out_PYSP << booster_stop_timestep << " " << timescale_stop << "\n";
   out_PYSP << ";\n\n";
   out_PYSP << "param P_CHL_INJ_START_TIMESTEP := " << booster_start_timestep << ";\n\n";
   out_PYSP << "param P_CHL_INJ_END_TIMESTEP := " << booster_stop_timestep << ";\n\n";

   out_PYSP << "# Toxin mass injection profile [g/min]\n"; 
   out_PYSP << "param P_MN_TOX_gpmin:=\n";
   typedef std::map<int,float> map_int_to_float;
   map_int_to_float toxin_rhs_gpmin;
   for STL_CONST_ITERATE(InjScenList, p_tox, p_tox_stop, toxin_scenarios) {
      (*p_tox)->SetArray(*chl_model_,toxin_rhs_gpmin);
   }
   for STL_ITERATE(map_int_to_float, pos, end, toxin_rhs_gpmin) {
      const int nt_idx = chl_model_->perm_upper_to_nt[pos->first];
      const int node_id = (nt_idx)/chl_model_->n_steps;
      const int time_id = (nt_idx)%chl_model_->n_steps;
      out_PYSP << node_map_[node_id] << " " 
         << time_id << " " 
         << pos->second << "\n";
   }
   
   out_PYSP << ";"; 
   out_PYSP.close();
}


void BoosterDataWriterPYSP::WriteScenario(std::map<int,std::vector<int> >& z_rows,
                                          const float* tox_conc_gpm3,
                                          const BoosterScenario& scen,
                                          bool is_first,
                                          bool summary_only)
{
   if (is_first) {
      pysp_write_scenario_neutral_model(z_rows, tox_conc_gpm3, scen, "ReferenceModel.dat");
   }
   pysp_write_scenario_neutral_model(z_rows, tox_conc_gpm3, scen, std::string("SCEN"+scen.Name()+".dat"));
}

void BoosterDataWriterPYSP::pysp_write_scenario_neutral_model(std::map<int,std::vector<int> >& z_rows,
                                                          const float* tox_conc_gpm3,
                                                          const BoosterScenario& scen,
                                                          std::string fname)
{
   out_PYSP.open(fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_PYSP << "param P_PRE_OMITTED_OBJ_g := " << scen.Impact("Pre-Detect Mass Consumed Grams") << ";\n";
   out_PYSP << "param P_POST_OMITTED_OBJ_g := " << scen.Impact("Post-Detect Mass Consumed Grams") << ";\n";
   out_PYSP << "\n";
   out_PYSP << "include " << out_BC_fname << ";\n";
   out_PYSP << "\n\n";

   out_PYSP << "param: S_SPACETIME: P_MASS_CONS_g:=\n";
   typedef std::map<int, std::vector<int> > tmp_map_type;
   for STL_ITERATE(tmp_map_type, pos, pos_end, z_rows) {  
      int u_idx = pos->first;
      int nt_idx = chl_model_->perm_upper_to_nt[u_idx];
      int n = (nt_idx) / chl_model_->n_steps;
      int t = (nt_idx) % chl_model_->n_steps;
      out_PYSP << node_map_[n] << " " << t << " " << 
         tox_conc_gpm3[u_idx]*chl_model_->demand_m3pmin[nt_idx]*chl_model_->qual_step_minutes << "\n";
   }
   out_PYSP << ";\n\n";
   for STL_ITERATE(tmp_map_type, pos, pos_end, z_rows) {  
      int u_idx = pos->first;
      int nt_idx = chl_model_->perm_upper_to_nt[u_idx];
      std::vector<int>& CNDS = pos->second;
      int n = (nt_idx) / chl_model_->n_steps;
      int t = (nt_idx) % chl_model_->n_steps;
      out_PYSP << "set S_CONTROLLER_BOOSTERS[" << node_map_[n] << "," << t << "," << scen.Name() << "] :=";
      for STL_ITERATE(std::vector<int>, pnode, pnode_end, CNDS) {
         out_PYSP << " " << node_map_[*pnode];
      }
      out_PYSP << ";\n";
   }
   out_PYSP << "\n\n";
   out_PYSP.close();
}

void BoosterDataWriterPYSP::pysp_write_scenario_structure()
{
   const BoosterScenList& scen_list = chl_net_->BoosterScenarios();
   out_PYSP.open("ScenarioStructure.dat", std::ios_base::out | std::ios_base::trunc);
   out_PYSP << "set Stages := FirstStage SecondStage ;\n\n";
   out_PYSP << "set Nodes :=\n"+options_->output_prefix+"RootNode\n";
   for STL_CONST_ITERATE(BoosterScenList, pos, stop, scen_list) {
      out_PYSP << options_->output_prefix+"SCEN" << (*pos)->Name() << "Node\n";
   }
   out_PYSP << ";\n\n";
   out_PYSP << "param NodeStage :=\n"+options_->output_prefix+"RootNode\tFirstStage\n";
   for STL_CONST_ITERATE(BoosterScenList, pos, stop, scen_list) {
      out_PYSP << options_->output_prefix+"SCEN" << (*pos)->Name() << "Node\tSecondStage\n";
   }
   out_PYSP << ";\n\n";
   out_PYSP << "set Children["+options_->output_prefix+"RootNode] :=\n";
   for STL_CONST_ITERATE(BoosterScenList, pos, stop, scen_list) {
      out_PYSP << options_->output_prefix+"SCEN" << (*pos)->Name() << "Node\n";
   }
   out_PYSP << ";\n\n";
   out_PYSP << "param ConditionalProbability :=\n"+options_->output_prefix+"RootNode\t1.0\n";
   for STL_CONST_ITERATE(BoosterScenList, pos, stop, scen_list) {
         out_PYSP << options_->output_prefix+"SCEN" << (*pos)->Name() << "Node\t" << (*pos)->Impact("Normalized Weight") << "\n";
   }
   out_PYSP << ";\n\n";
   out_PYSP << "set Scenarios :=\n";
   for STL_CONST_ITERATE(BoosterScenList, pos, stop, scen_list) {
      out_PYSP << options_->output_prefix+"SCEN" << (*pos)->Name() << "\n";
   }
   out_PYSP << ";\n\n";
   out_PYSP << "param ScenarioLeafNode :=\n";
   for STL_CONST_ITERATE(BoosterScenList, pos, stop, scen_list) {
      out_PYSP << options_->output_prefix+"SCEN" << (*pos)->Name() << "\t"+options_->output_prefix+"SCEN" << (*pos)->Name() << "Node\n";
   }
   out_PYSP << ";\n\n";
   out_PYSP << "set StageVariables[FirstStage] := y_booster[*] ;\n\n";
   out_PYSP << "set StageVariables[SecondStage] := cn_tox_gpm3[*,*] cn_chl_gpm3[*,*] mn_chl_gpmin[*,*] tox_r_gpmin[*,*] ;\n\n";
   out_PYSP << "param StageCostVariable :=\n";
   out_PYSP << "FirstStage FirstStageCost\n";
   out_PYSP << "SecondStage SecondStageCost\n;\n\n";
   out_PYSP << "param ScenarioBasedData := False;";
   out_PYSP.close();
}
