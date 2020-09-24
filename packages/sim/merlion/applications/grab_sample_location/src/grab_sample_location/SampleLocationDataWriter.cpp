#include <grab_sample_location/SampleLocationDataWriter.hpp>

#include <merlionUtils/ModelWriter.hpp>

#include <epanet2.h>
extern "C" {
 #include <enl.h>
 #include <erd.h>
}



SampleLocationDataWriterMerlion::SampleLocationDataWriterMerlion(SampleNetworkSimulator *Net, SampleLocationOptions *Opts)
:
   net_(Net),
   model_(&(Net->Model())),
   options_(Opts)
{
   // set the output precision
   int precision_number = 8;
   out_data.setf(std::ios::scientific,std::ios::floatfield);
   out_data.precision(precision_number);

   // define the data file names
   out_fname = options_->output_prefix+"GSP.dat";
   out_NMAP_fname = options_->output_prefix+"MERLION_LABEL_MAP.txt";
   out_MATRIX_fname = options_->output_prefix+"ImpactMatrix.dat";
   
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
	 //int merlion_node_id = model_->NodeID(in_name);
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
      
   if(options_->output_impact_matrix) {
     out_MATRIX.open(out_MATRIX_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
     out_MATRIX << "Event | Contaminated_Sample_Locations ";
     /*for (int i = 0; i < model_->n_nodes; ++i)
       {
       out_MATRIX <<node_map_[i]<<" "; 
       }*/
     out_MATRIX << "\n";
     EventList events_=net_->Events();
     for(EventList::iterator it=events_.begin(),end=events_.end();it!=end;it++)
       {
	 PEvent& event=*it;
	 out_MATRIX << event->Name() << "\t";
	 for(int nodeID=0;nodeID<model_->n_nodes;nodeID++)
	   {
	     if(event->hasInfectedNodeID(nodeID))
	       {out_MATRIX << node_map_[nodeID] << "  ";}
	     //else
	     //{out_MATRIX << "0 ";}
	   }
	 out_MATRIX << "\n\n";
       }
     out_MATRIX.close();
   }
   
}

void SampleLocationDataWriterMerlion::clear()
{
   net_ = NULL;
   model_ = NULL;
   options_ = NULL;
   node_map_.clear();
}

SampleLocationDataWriterMerlion::~SampleLocationDataWriterMerlion()
{
   clear(); 
}

void SampleLocationDataWriterMerlion::WriteNodesUncertainty()
{
  
}

void SampleLocationDataWriterMerlion::WriteProblem()
{
   out_data.open(out_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   EventList events_=net_->Events();
   //write the list of events
   out_data<<"set S_EVENTS := ";
   for(EventList::iterator it=events_.begin(),end=events_.end();it!=end;it++)
   {
      PEvent& event=*it;
      const merlionUtils::InjectionList injections=event->Injections();
      std::string injected_node = (*injections.begin())->NodeName();
      out_data<<event->Name()<<" ";
      //model_->node_name_to_id[injected_node];
      //out_data<<injected_node<<" ";
   }
   out_data<<";\n\n";
   //write the list of possible grab locations ALL NODES
   out_data<< "set S_LOCATIONS := ";   
   for (int n = 0; n < model_->n_nodes; ++n){
      out_data<<node_map_[n]<<" "; 
   }
   out_data<<";\n\n";
   //write pair wise set of all candidate events
   wisePair** pairs_=net_->Pairs();
   out_data<<"set S_PAIR_WISE := ";
   for(int i=0;i<net_->NumberOfPairs();i++)
   {
      std::pair<std::string,std::string> single_pair=pairs_[i]->eventNames;
      out_data<<"("<<single_pair.first<<","<<single_pair.second<<") ";
   }
   out_data<<";\n\n";
   //write the set of sample that distinguish i and j
   for(int i=0;i<net_->NumberOfPairs();i++)
   {
      std::pair<std::string,std::string> single_pair=pairs_[i]->eventNames;
      out_data<<"set S_DISTINGUISH_LOCATIONS["<<single_pair.first<<","<<single_pair.second<<"] :=";
      for(int j=0;j<pairs_[i]->distinguishableNodeIds.size();j++)
      {
         out_data<<" "<<node_map_[pairs_[i]->distinguishableNodeIds[j]];
      }
      out_data<<";\n";
   }
   out_data<<"\n";


   //write the set of fixed sensors
   out_data<<"set S_FIXED_LOCATIONS := ";
   for (int i=0; i< net_->fixed_sensor_ID.size();i++){
     out_data<<net_->fixed_sensor_ID[i]<<" ";
   }
   out_data<<";\n\n";

   //write the number of samples to be taken
   out_data<<"param P_MAX_SAMPLES := "<<options_->nSamples+net_->fixed_sensor_ID.size()<<";\n";
   
   if(options_->weights)
   {
     //Write weight
     out_data<<"\nparam P_WEIGHT_FACTORS := \n";
     for(int i=0;i<net_->NumberOfPairs();i++)
     {
	 std::pair<std::string,std::string> single_pair=pairs_[i]->eventNames;
	 out_data << single_pair.first << " " << single_pair.second << " " << pairs_[i]->distinguishableNodeIds.size()<<"\n";
     }
     out_data<<";\n";
   }
   out_data.close();
}


SampleLocationDataWriterEpanet::SampleLocationDataWriterEpanet(SampleNetworkSimulator *Net, SampleLocationOptions *Opts, PERD db)
:
   net_(Net),
   db_(db),
   options_(Opts)
{
   // set the output precision
   int precision_number = 8;
   out_data.setf(std::ios::scientific,std::ios::floatfield);
   out_data.precision(precision_number);

   // define the data file names
   out_fname = options_->output_prefix+"GSP.dat";
   out_NMAP_fname = options_->output_prefix+"EPANET_LABEL_MAP.txt";
   out_MATRIX_fname = options_->output_prefix+"ImpactMatrix.dat";
   
   // create the modifed node map taking
   // merlion id -> epanet name -> custom label
   int numNodes = db_->network->numNodes;
   std::map<std::string,std::string> name_map;
   std::map<std::string,std::string> label_map;
   if (options_->nodemap_filename != "") {
      // use custom labels in data files
     std::ifstream in;
     in.open(options_->nodemap_filename.c_str(), std::ios_base::in);
     if (in.is_open()) {
       std::string in_name, in_label;
       std::vector<std::string>::iterator findpos;
       for (int i = 0, stop = numNodes; i < stop; ++i) {
	 in >> in_name;
	 //int merlion_node_id = net_->getEpanetIdx(in_name);
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
   else if (options_->output_epanet_labels) {
     // use epanet integer ids in data files
     typedef std::map<std::string, int> map_string_to_int;
     for (int n = 0; n < numNodes; ++n) {
       std::ostringstream stream;
       stream << n;
       name_map[db_->nodes[n].id] = stream.str();
     }
   }
   else {
     // use epanet names in data files
     typedef std::map<std::string, int> map_string_to_int;
     for (int n = 0; n < numNodes; ++n) {
       name_map[db_->nodes[n].id] = db_->nodes[n].id;
     }
   }
   for (int n = 0; n < numNodes; ++n) {
     node_map_[n] = name_map[db_->nodes[n].id]; 
   }
   
   //write the map from node names to 
   //epanet labels if this option is requested
   if (options_->output_epanet_labels) {
     std::cout << "Printing EPANET labels ." << std::endl;
     out_NMAP.open(out_NMAP_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
     for (int n = 0; n < numNodes; ++n) {
       out_NMAP << db_->nodes[n].id << " " << n << "\n"; 
     }
     out_NMAP.close();
   }  
      
   if(options_->output_impact_matrix) {
     out_MATRIX.open(out_MATRIX_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
     out_MATRIX << "Event | Contaminated_Sample_Locations ";
     /*for (int i = 0; i < model_->n_nodes; ++i)
       {
       out_MATRIX <<node_map_[i]<<" "; 
       }*/
     out_MATRIX << "\n";
     EventList events_=net_->Events();
     for(EventList::iterator it=events_.begin(),end=events_.end();it!=end;it++)
       {
	 PEvent& event=*it;
	 out_MATRIX << event->Name() << "\t";
	 for(int nodeID=0;nodeID<numNodes;nodeID++)
	   {
	     if(event->hasInfectedNodeID(nodeID))
	       {out_MATRIX << node_map_[nodeID] << "  ";}
	     //else
	     //{out_MATRIX << "0 ";}
	   }
	 out_MATRIX << "\n\n";
       }
     out_MATRIX.close();
   }
   
}

void SampleLocationDataWriterEpanet::clear()
{
   net_ = NULL;
   options_ = NULL;
   node_map_.clear();
}

SampleLocationDataWriterEpanet::~SampleLocationDataWriterEpanet()
{
   clear(); 
}

void SampleLocationDataWriterEpanet::WriteProblem()
{
   out_data.open(out_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   EventList events_=net_->Events();
   //write the list of events
   out_data<<"set S_EVENTS := ";
   for(EventList::iterator it=events_.begin(),end=events_.end();it!=end;it++)
   {
      PEvent& event=*it;
      //const merlionUtils::InjectionList injections=event->Injections();
      //std::string injected_node = (*injections.begin())->NodeName();
      out_data<<event->Name()<<" ";
      //model_->node_name_to_id[injected_node];
      //out_data<<injected_node<<" ";
   }
   out_data<<";\n\n";
   //write the list of possible grab locations ALL NODES
   out_data<< "set S_LOCATIONS := ";   
   for (int n = 0; n < db_->network->numNodes; ++n){
      out_data<<node_map_[n]<<" "; 
   }
   out_data<<";\n\n";
   //write pair wise set of all candidate events
   wisePair** pairs_=net_->Pairs();
   out_data<<"set S_PAIR_WISE := ";
   for(int i=0;i<net_->NumberOfPairs();i++)
   {
      std::pair<std::string,std::string> single_pair=pairs_[i]->eventNames;
      out_data<<"("<<single_pair.first<<","<<single_pair.second<<") ";
   }
   out_data<<";\n\n";
   //write the set of sample that distinguish i and j
   for(int i=0;i<net_->NumberOfPairs();i++)
   {
      std::pair<std::string,std::string> single_pair=pairs_[i]->eventNames;
      out_data<<"set S_DISTINGUISH_LOCATIONS["<<single_pair.first<<","<<single_pair.second<<"] :=";
      for(int j=0;j<pairs_[i]->distinguishableNodeIds.size();j++)
      {
         out_data<<" "<<node_map_[pairs_[i]->distinguishableNodeIds[j]];
      }
      out_data<<";\n";
   }
   out_data<<"\n";

   //write the set of fixed sensors
   out_data<<"set S_FIXED_LOCATIONS := ";
   for (int i=0; i< net_->fixed_sensor_ID.size();i++){
     out_data<<net_->fixed_sensor_ID[i]<<" ";
   }
   out_data<<";\n\n";

   //write the number of samples to be taken
   out_data<<"param P_MAX_SAMPLES := "<<options_->nSamples+net_->fixed_sensor_ID.size()<<";\n\n";
   
   //Write weight
   if(options_->weights)
   {
     out_data<<"param P_WEIGHT_FACTORS := \n";
     for(int i=0;i<net_->NumberOfPairs();i++)
       {
	 std::pair<std::string,std::string> single_pair=pairs_[i]->eventNames;
	 out_data << single_pair.first << " " << single_pair.second << " " << pairs_[i]->distinguishableNodeIds.size()<<"\n";
       }
     out_data<<";\n";
   }
   out_data.close();
}
