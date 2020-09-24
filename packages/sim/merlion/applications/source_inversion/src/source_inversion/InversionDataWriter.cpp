#include <source_inversion/InversionDataWriter.hpp>
#include <merlionUtils/ModelWriter.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/SparseMatrix.hpp>
#include <set>


InversionDataWriter::InversionDataWriter(InversionNetworkSimulator *Net, InversionSimOptions *Opts)
:
   net_(Net),
   model_(&(Net->Model())),
   options_(Opts)
{
   // set the output precision
   int precision_number = 8;
   out_WQM.setf(std::ios::scientific,std::ios::floatfield);
   out_WQM.precision(precision_number);
   out_CONC.setf(std::ios::scientific,std::ios::floatfield);
   out_CONC.precision(precision_number);
   out_index.setf(std::ios::scientific,std::ios::floatfield);
   out_index.precision(precision_number);
   out_vals.setf(std::ios::scientific,std::ios::floatfield);
   out_vals.precision(precision_number);
   out_UNCERTAIN.setf(std::ios::scientific,std::ios::floatfield);
   out_UNCERTAIN.precision(precision_number);


   // define the data file names
   out_WQM_fname = options_->output_prefix+"WQM.dat";
   out_CONC_fname = options_->output_prefix+"CONC.dat";
   out_INDEX_fname= options_->output_prefix+"INV_ROWS_INDEX.dat";
   out_VALS_fname= options_->output_prefix+"INV_ROWS_VALS.dat";
   out_NMAP_fname = options_->output_prefix+"MERLION_LABEL_MAP.txt";
   out_IMPACTN_fname = options_->output_prefix+"Likely_Nodes.dat";
   out_UNCERTAIN_fname = options_->output_prefix+"Uncertain_nodes.dat";   



   // create the modifed node map taking
   // merlion id -> epanet name -> custom label
   std::map<std::string,std::string> name_map;
   std::map<std::string,std::string> label_map;
   if (options_->nodemap_filename != "") {
      // use custom labels in data files
      std::ifstream in;
      in.open(options_->nodemap_filename.c_str(), std::ios_base::in);
      if (in.is_open()) {
         std::string in_name, in_label;
         std::vector<std::string>::iterator findpos;
         for (int i = 0, stop = model_->n_nodes; i < stop; ++i) {
            in >> in_name;
	    int merlion_node_id = model_->NodeID(in_name);
	    in >> in_label;
	    if (label_map.find(in_label) != label_map.end()) {
	       std::cerr << "Duplicate labels found in node map file: " << in_label << std::endl; 
	    } 
	    else {
	       label_map[in_label] = in_name;
	       name_map[in_name] = in_label;
	    }
         }
      }
      else {
         std::cerr << "Failed to open custom node map file: " << options_->nodemap_filename << std::endl;
         exit(1); 
      }
      in.close();
   }
   else if (options_->output_merlion_labels) {
      // use merlion integer ids in data files
      typedef std::map<std::string, int> map_string_to_int;
      for (int n = 0; n < model_->n_nodes; ++n) {
	 std::ostringstream stream;
         stream << n;
         name_map[model_->NodeName(n)] = stream.str();
      }
   }
   else {
      // use epanet names in data files
      typedef std::map<std::string, int> map_string_to_int;
      for (int n = 0; n < model_->n_nodes; ++n) {
	 name_map[model_->NodeName(n)] = model_->NodeName(n);
      }
   }
   for (int n = 0; n < model_->n_nodes; ++n) {
      node_map_[n] = name_map[model_->NodeName(n)]; 
   }

   //write the map from node names to 
   //merlion labels if this option is requested
   if (options_->output_merlion_labels) {
      out_NMAP.open(out_NMAP_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
      for (int n = 0; n < model_->n_nodes; ++n) {
         out_NMAP << model_->NodeName(n) << " " << n << "\n"; 
      }
      out_NMAP.close();
   }  
}

void InversionDataWriter::clear()
{
   net_ = NULL;
   model_ = NULL;
   options_ = NULL;
   node_map_.clear();
}

InversionDataWriter::~InversionDataWriter()
{
   clear(); 
}

void InversionDataWriter::WriteReduceMAtrixGInverse()
{
   const SparseMatrix& R =net_->WQMInverse();
   //const_cast<SparseMatrix&>(R).TransformToCSCMatrix();
   const_cast<SparseMatrix&>(R).TransformToCSRMatrix();
   std::set<int> impact;
   std::set<int> impact_nodes;
   std::set<int> impact_times;
   //std::ofstream out_index, out_vals;
   out_index.open(out_INDEX_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_vals.open(out_VALS_fname.c_str(), std::ios::out | std::ios::trunc);
   //out_STEP.open(out_STEP_fname.c_str(), std::ios::out | std::ios::trunc);
   out_vals << "param P_INV_CONC_MATRIX_CSR :=\n";
   for (int i = 0, i_stop = R.NRows(); i < i_stop; ++i) {
      int p_start = R.pRows()[i];
      int p_stop = R.pRows()[i+1];
      if (p_start < p_stop) {
         int row_n = model_->perm_upper_to_nt[i]/model_->n_steps;
         int row_t = model_->perm_upper_to_nt[i]%model_->n_steps;
         out_index << "set S_INV_CONC_MATRIX_CSR_INDEX[" << node_map_[row_n] << "," << row_t << "] := ";
         out_vals << "[" << node_map_[row_n] << "," << row_t << ",*,*] ";   
      }
      float scale=R.Values()[p_start];
      for (int p = p_start; p < p_stop; ++p) {
         int ut_idx = R.jCols()[p];
         if (R.Values()[p] > CONC_ZERO_TOL_GPM3) {
            int nt_idx = model_->perm_upper_to_nt[ut_idx];
            impact.insert(nt_idx);
            int node = nt_idx / model_->n_steps;
            impact_nodes.insert(node);
            int time = nt_idx % model_->n_steps;
            impact_times.insert(time);
            out_index << "(" << node_map_[node] << "," << time << ") ";
            out_vals << node_map_[node] << " " << time << " "<<R.Values()[p]/scale<<" ";
         }
      }
      if (p_start < p_stop) {
         out_index << ";\n";
         out_vals << "\n";
      }
   }
   out_vals << ";\n";
   out_vals.close();

   out_index << "\nset S_IMPACT := ";
   for (std::set<int>::iterator pos = impact.begin(), p_end = impact.end(); pos != p_end; ++pos) {
      out_index << "(" << node_map_[(*pos)/model_->n_steps] << "," << (*pos)%model_->n_steps << ") "; 
   }
   out_index << ";\n\n";
   out_index << "set S_IMPACT_NODES := ";
   for (std::set<int>::iterator pos = impact_nodes.begin(), p_end = impact_nodes.end(); pos != p_end; ++pos) {
      out_index << node_map_[*pos] << " "; 
   }
   out_index << ";\n\n";
   for (std::set<int>::iterator pos = impact_nodes.begin(), n_end = impact_nodes.end(); pos != n_end; ++pos) {
      out_index << "set S_IMPACT_TIMES[" <<node_map_[*pos]<<"]:= ";
      for (std::set<int>::iterator i = impact.begin(), p_end = impact.end(); i != p_end; ++i) {
         if((*i)/model_->n_steps == *pos){
            out_index << (*i)%model_->n_steps << " ";
         } 
      }
      out_index <<";\n";
   }
   out_index.close();
   /*for(std::set<int>::iterator it = impact_times.begin(),p_end = impact_times.end();it != p_end;it++)
   {
      out_STEP << "set S_IMPACTED_NODES_AT[" <<(*it)%model_->n_steps<<"] :=";
      for(std::set<int>::iterator pos=impact.begin(),n_end=impact.end();pos!=n_end;++pos)
      {
         if(*it==(*pos)%model_->n_steps)
         {
            out_STEP <<" "<< node_map_[(*pos)/model_->n_steps];
         }
      }
      out_STEP << ";\n";
   }
   out_STEP.close();*/
}

void InversionDataWriter::WriteGMAtrixToAMPL(int start_timestep,int stop_timestep)
{
   out_WQM.open(out_WQM_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMHeadersToAMPLFormat(out_WQM, model_->merlionModel, node_map_,0,model_->n_steps-1);
   merlionUtils::PrintWQMToAMPLFormat(out_WQM, model_->merlionModel, node_map_,0,model_->n_steps-1);
   out_WQM.close();
}

//Write the measurements without forward simulation
void InversionDataWriter::WriteMeasurments(int start_time, int stop_time) 
{
  std::cout<<"start time = " << start_time << "\n stop time = " << stop_time <<std::endl;  
   std::vector<Measure*> measures=net_->MeasureList();
      out_CONC.open(out_CONC_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
      /*out_CONC<<"param: S_MEAS: P_MEASUREMENT_CONC_gpm3 :=\n";
      for(std::vector<Measure*>::iterator pos_s = measures.begin(), s_end = measures.end(); pos_s != s_end; ++pos_s){
            std::string n_measure=(*pos_s)->nodeName();
            int nodeID=const_cast<merlionUtils::MerlionModelContainer*>(model_)->node_name_to_id[n_measure];
            int t_measure=(*pos_s)->Measure_timestep(model_->qual_step_minutes);
            double v_measure=(*pos_s)->value();
            if(t_measure>=start_time && t_measure<=stop_time)
            out_CONC<<node_map_[nodeID]<<" "<<t_measure<<" "<<v_measure<<"\n";
      }*/
      std::vector<std::pair<int,int> > positive_measures;
      std::vector<std::pair<int,int> > negative_measures;
      out_CONC<<"set S_MEAS :=\n";
      for(std::vector<Measure*>::iterator pos_s=measures.begin(), s_end=measures.end(); pos_s != s_end;++pos_s)
      {
         std::string n_measure=(*pos_s)->nodeName();
         int nodeID=model_->NodeID(n_measure);
         int t_measure=(*pos_s)->Measure_timestep(model_->qual_step_minutes);
         double v_measure=(*pos_s)->value();
         if(v_measure>=options_->threshold && t_measure >= start_time && t_measure <= stop_time)
         {
            positive_measures.push_back(std::pair<int,int>(nodeID,t_measure));
         }
         else if (t_measure >= start_time && t_measure <= stop_time) 
         {
            negative_measures.push_back(std::pair<int,int>(nodeID,t_measure));
         }
	 if (t_measure >= start_time && t_measure <= stop_time) {
	   out_CONC <<"("<<node_map_[nodeID]<<","<<t_measure<<") ";
	 }
     }
      out_CONC << ";\n";
      if(positive_measures.size()>0)
      {
         out_CONC << "set S_POS_MEAS :=\n";
         for(std::vector<std::pair<int,int> >::iterator pos_s=positive_measures.begin(),s_end=positive_measures.end();pos_s !=s_end;++pos_s)
         {
            int nodeID=pos_s->first;
            int t_measure=pos_s->second;
            out_CONC <<"("<<node_map_[nodeID]<<","<<t_measure<<") ";
         }
         out_CONC << ";\n";
      }
      if(negative_measures.size()>0)
      {
         out_CONC << "set S_NEG_MEAS :=\n";
         for(std::vector<std::pair<int,int> >::iterator pos_s=negative_measures.begin(),s_end=negative_measures.end();pos_s !=s_end;++pos_s)
         {
            int nodeID=pos_s->first;
            int t_measure=pos_s->second;
            out_CONC <<"("<<node_map_[nodeID]<<","<<t_measure<<") ";
         }
         out_CONC<< ";\n";
      }
      out_CONC<< "param P_MINUTES_PER_TIMESTEP := "<< model_->qual_step_minutes << ";\n";
      out_CONC<< "param P_TIME_STEPS := "<< model_->n_steps-1 << ";\n";
      out_CONC.close();
}

void InversionDataWriter::WriteImpactNodeNames(std::vector<std::string> nodes_v)
{
   out_IMPACTN.open(out_IMPACTN_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   for(std::vector<std::string>::iterator it=nodes_v.begin(),i_end=nodes_v.end();it!=i_end;++it)
   {
      out_IMPACTN<<*it<<"\n";
      //std::cout<<*it<<"\n";
   }
   out_IMPACTN.close();
}

void InversionDataWriter::WriteUncertainNodes()
{
  if(options_->probability && options_->snap_time!=-1){
    out_UNCERTAIN.open(out_UNCERTAIN_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
    EventList events = net_->probableEvents();
    out_UNCERTAIN << events.size() << "\n";
    for(int i = 0; i<model_->n_nodes;++i){
      int accum = 0;
      out_UNCERTAIN << model_->NodeName(i) << " ";
      for(EventList::iterator event_i=events.begin(),event_end=events.end();event_i!=event_end;event_i++){
	if((*event_i)->hasInfectedNodeID(i)){
	  ++accum;
	}
      }
      out_UNCERTAIN << accum <<"\n";
    }
    out_UNCERTAIN.close();
  }
      
}
