#include <grab_sample_location/SampleNetworkSimulator.hpp>

#include <merlionUtils/TaskTimer.hpp>
#include <merlionUtils/TSG_Reader.hpp>

#include <merlion/TriSolve.hpp>
#include <merlion/SparseMatrix.hpp>
#include <merlion/Merlion.hpp>
#include <epanet2.h>
//#include <epanetmsx.h>
extern "C" {
 #include <enl.h>
 #include <erd.h>
}
#include <tevautil.h>

#include <algorithm>
#include <cmath>
#include <set>

#include <time.h>

void SampleNetworkSimulator::clear()
{  
   NetworkSimulator::clear();
}

void SampleNetworkSimulator::writeEvents()
{
   int i=0;
   double scale=1000;
   for(merlionUtils::InjScenList::iterator scen_pos=injection_scenarios_.begin(),scen_end=injection_scenarios_.end();scen_pos!=scen_end;scen_pos++)
   {
      i++;
      PEvent newEvent(new Event);
      newEvent->Name()=(*scen_pos)->Name();
      merlionUtils::InjectionList scen_injections=(*scen_pos)->Injections();
      for(merlionUtils::InjectionList::iterator inj_pos=scen_injections.begin(),inj_end=scen_injections.end();inj_pos!=inj_end;inj_pos++)
      {
	 (*inj_pos)->Strength()=(*inj_pos)->Strength()*scale;
         newEvent->Injections().push_back((*inj_pos));
      }
      events_.push_back(newEvent);
   }
}

unsigned long combinatorial(int n, int r)
{
   if(r==0 || n==r) 
   {return 1;}
   else if(r==1) 
   {return n;}
   else
   {return combinatorial(n-1,r)+combinatorial(n-1,r-1);}
}

bool isAnEntry(std::vector<int> vector,int entry)
{
   bool is=false;
   for(int i=0;i<vector.size() && !is;i++)
   {
      if(vector[i]==entry)
      {is=true;}
   }
   return is;
}

void SampleNetworkSimulator::createEpanetLabelMap(PERD db) {
  int numNodes = db->network->numNodes;
  for (int i = 0; i < numNodes; i++) {
    EpanetIdx_[db->nodes[i].id] = i;
  }  
}

std::vector<int> vector_diff( const std::vector<int>& first, const std::vector<int>& second )
{
    std::set<int> s_first( first.begin(), first.end() );
    std::set<int> s_second( second.begin(), second.end() );
    std::vector<int> result;

    std::set_difference( s_first.begin(), s_first.end(), s_second.begin(), s_second.end(),
        std::back_inserter( result ) );

    return result;
}

void SampleNetworkSimulator::BuildSets(bool greedy)
{            
      int j,i=0, substract=-1;
      int n_events=events_.size();
      if(n_events!=0)
      {
      	if(!greedy) {	
		 	numberOfPairs= combinatorial(n_events,2);
		 	pairWiseP_=new wisePair*[numberOfPairs];
		 	//merlionUtils::TaskTimer build;
		 	for(EventList::iterator event_pos_i=events_.begin(),event_end=events_.end();event_pos_i!=event_end;event_pos_i++)
		 	{
		    	const Event& first_event = **event_pos_i;
			    const merlionUtils::InjectionList first_list=first_event.Injections();
			    const std::string first_node_name=first_list.front()->NodeName();
			    j=0;
			    for(EventList::iterator event_pos_j=events_.begin();event_pos_j!=event_end;event_pos_j++)
			    {
			       const Event& second_event = **event_pos_j;
			       const merlionUtils::InjectionList second_list=second_event.Injections();
			       const std::string second_node_name=second_list.front()->NodeName();
			       if(j>i)
			       {
				  pairWiseP_[n_events*i+j-1-substract]=new wisePair;   
				  pairWiseP_[n_events*i+j-1-substract]->defineEventIds(model_.NodeID(first_node_name),model_.NodeID(second_node_name));
				  pairWiseP_[n_events*i+j-1-substract]->defineEventNames(first_event.Name(),second_event.Name());
			       }
			       else
			       {substract++;}
			       j++;
			    }
			    i++;
			 }
			 // build.StopAndPrint(std::cout, "\n\t- Build pairwise sets ");     

			 merlionUtils::TaskTimer create_sensor_set;

			 for(i=0;i<numberOfPairs;i++)
			 {
			    std::pair<std::string,std::string> pareja=pairWiseP_[i]->eventNames;
			    
			    std::vector<int> nodes1;
			    std::vector<int> nodes2;
			    bool out1=false, out2=false, out=false;
			 
			    for(EventList::iterator event_pos=events_.begin(),event_end=events_.end();event_pos!=event_end && !out;event_pos++)
			    {
			       const Event& event= **event_pos;
			       if(event.Name()==pareja.first)
			       {
				  for(int j=0;j<event.infectedNodes().size();j++)
				  {
				     nodes1.push_back(event.infectedNodes()[j]);
				  }
				  out1=true;
			       }
			       if(event.Name()==pareja.second)
			       {
				  for(int j=0;j<event.infectedNodes().size();j++)
				  {
				     nodes2.push_back(event.infectedNodes()[j]);
				  }
				  out2=true;
			       }
			       out=out1 && out2;
			    }
			    
			    for(int c=0;c<nodes1.size();c++)
			    {
			       if(!isAnEntry(nodes2,nodes1[c]))
			       {
				  pairWiseP_[i]->addDistinguisableNode(nodes1[c]);
				  // if(greedy) {
				  //   sensorMap_[nodes1[c]].insert(pairWiseP_[i]->pair());
				  // }
			       }
			    }
			    
			    for(int c=0;c<nodes2.size();c++)
			    {
			       if(!isAnEntry(nodes1,nodes2[c]))
			       {
				  pairWiseP_[i]->addDistinguisableNode(nodes2[c]);
				  // if(greedy) {
				  //   sensorMap_[nodes2[c]].insert(pairWiseP_[i]->pair());
				  // }
			       }
			    }

			 }
			 create_sensor_set.StopAndPrint(std::cout, "\n\t- Create sensor sets ");
		 }
		 else {
				for(EventList::iterator event_pos_i=events_.begin(),event_end=events_.end();event_pos_i!=event_end;event_pos_i++)
		 		{	
			    	const Event& first_event = **event_pos_i;
				    const merlionUtils::InjectionList first_list=first_event.Injections();
				    const std::string first_node_name=first_list.front()->NodeName();
				    j=0;
				    for(EventList::iterator event_pos_j=events_.begin();event_pos_j!=event_end;event_pos_j++)
				    {
				       const Event& second_event = **event_pos_j;
				       const merlionUtils::InjectionList second_list=second_event.Injections();
				       const std::string second_node_name=second_list.front()->NodeName();
				       if(j>i)
				       { // get contaminated vectors for both events
				       	 // compare them to get distiguishing nodes
				       	 std::pair<int,int> add_event_pair(model_.NodeID(first_node_name),model_.NodeID(second_node_name));   
				       	 std::vector<int> first_infectedNodes = first_event.infectedNodes();
				       	 std::vector<int> second_infectedNodes = second_event.infectedNodes();
				       	 std::vector<int> diff_vector_1 = vector_diff(first_infectedNodes, second_infectedNodes);
				       	 std::vector<int> diff_vector_2 = vector_diff(second_infectedNodes, first_infectedNodes);
					 std::pair<std::pair<int,int>,int> wij(add_event_pair,diff_vector_1.size()+diff_vector_2.size());
					 weights_greedy.insert(wij);
				       	 for (std::vector<int>::iterator c = diff_vector_1.begin(); c != diff_vector_1.end(); ++c)
				       	 {
				       	 sensorMap_[*c].insert(add_event_pair);
				       	 }
					 for (std::vector<int>::iterator c = diff_vector_2.begin(); c != diff_vector_2.end(); ++c)
				       	 {
				       	 sensorMap_[*c].insert(add_event_pair);	
				       	 }
					  // pairWiseP_[n_events*i+j-1-substract]=new wisePair;   
					  // pairWiseP_[n_events*i+j-1-substract]->defineEventIds(model_.NodeID(first_node_name),model_.NodeID(second_node_name));
					  // pairWiseP_[n_events*i+j-1-substract]->defineEventNames(first_event.Name(),second_event.Name());
				   		}
				       else
				       {substract++;}
				       j++;
				    }
				    i++;
				 }

			 }	     

     }
      /* for(std::map<int,std::set<std::pair<int,int> > >::iterator i=sensorMap_.begin();i!=sensorMap_.end();++i)
      {
	int nodeID = i->first;
	std::cout<<"\n"<<nodeID<< "\t" << sensorMap_[nodeID].size()<<std::endl;
	for(std::set<std::pair<int,int> >::iterator j = sensorMap_[nodeID].begin(); j!=sensorMap_[nodeID].end(); ++j) {
	  std::pair<int,int> pareja= *j;
	  std::cout<<model_.NodeName(pareja.first) <<"\t"<< model_.NodeName(pareja.second) <<"\n";
	  }
	  }*/
      
}

void SampleNetworkSimulator::BuildSets_teva(PERD db)
{            
  int j,i=0, substract=-1;
  int n_events=events_.size();
  if(n_events!=0)
    {
      numberOfPairs= combinatorial(n_events,2);
      pairWiseP_=new wisePair*[numberOfPairs];
      //merlionUtils::TaskTimer build;
      for(EventList::iterator event_pos_i=events_.begin(),event_end=events_.end();event_pos_i!=event_end;event_pos_i++)
	{
	  const Event& first_event = **event_pos_i;
	  //const merlionUtils::InjectionList first_list=first_event.Injections();
	  const std::string first_node_name=first_event.Name();
	  j=0;
	  for(EventList::iterator event_pos_j=events_.begin();event_pos_j!=event_end;event_pos_j++)
	    {
	      const Event& second_event = **event_pos_j;
	      //const merlionUtils::InjectionList second_list=second_event.Injections();
	      const std::string second_node_name=second_event.Name();
	      if(j>i)
		{
		  pairWiseP_[n_events*i+j-1-substract]=new wisePair;   
		  pairWiseP_[n_events*i+j-1-substract]->defineEventIds(EpanetIdx_[first_node_name],EpanetIdx_[second_node_name]);
		  pairWiseP_[n_events*i+j-1-substract]->defineEventNames(first_event.Name(),second_event.Name());
		}
	      else
		{substract++;}
	      j++;
	    }
	  i++;
	}
      // build.StopAndPrint(std::cout, "\n\t- Build pairwise sets ");     
      
      merlionUtils::TaskTimer create_sensor_set;
      
      for(i=0;i<numberOfPairs;i++)
	{
	  std::pair<std::string,std::string> pareja=pairWiseP_[i]->eventNames;
	  
	  std::vector<int> nodes1;
	  std::vector<int> nodes2;
	  bool out1=false, out2=false, out=false;
	  
	  for(EventList::iterator event_pos=events_.begin(),event_end=events_.end();event_pos!=event_end && !out;event_pos++)
	    {
	      const Event& event= **event_pos;
	      if(event.Name()==pareja.first)
		{
		  for(int j=0;j<event.infectedNodes().size();j++)
		    {
		      nodes1.push_back(event.infectedNodes()[j]);
		    }
		  out1=true;
		}
	      if(event.Name()==pareja.second)
		{
		  for(int j=0;j<event.infectedNodes().size();j++)
		    {
		      nodes2.push_back(event.infectedNodes()[j]);
		    }
		  out2=true;
		}
	      out=out1 && out2;
	    }
	  
	  for(int c=0;c<nodes1.size();c++)
	    {
	      if(!isAnEntry(nodes2,nodes1[c]))
		{
		  pairWiseP_[i]->addDistinguisableNode(nodes1[c]);
		  // if(greedy) {
		  //   sensorMap_[nodes1[c]].insert(pairWiseP_[i]->pair());
		  // }
		}
	    }
	  
	  for(int c=0;c<nodes2.size();c++)
	    {
	      if(!isAnEntry(nodes1,nodes2[c]))
		{
		  pairWiseP_[i]->addDistinguisableNode(nodes2[c]);
		  // if(greedy) {
		  //   sensorMap_[nodes2[c]].insert(pairWiseP_[i]->pair());
		  // }
		}
	    }
	  
	}
      create_sensor_set.StopAndPrint(std::cout, "\n\t- Create sensor sets ");
    }
  
}




float SampleNetworkSimulator::GreedySelection(int nSamples)
{
  int largest_distinguishing_node_;
  float obj = 0.0f;
  int n = 1;
  
  //std::cout<<"Sensor map size : " << sensorMap_.size() <<"\n";
  
  // Check to see if sensor map file is not empty . This happens when all events are indistiguishable. 
  if(sensorMap_.size() == 0) {
  	return -1.0;
  }

  while(n<=nSamples) {
    int most_dist_nodes=0;
    for(std::map<int,std::set<std::pair<int,int> > >::iterator i=sensorMap_.begin();i!=sensorMap_.end();++i) {
      
      int nodeID = i->first;
      if(!with_weights){
	if(i->second.size()>most_dist_nodes) {
	  most_dist_nodes = i->second.size();
	  largest_distinguishing_node_ = nodeID;
	}
      }
      else{
	std::set<std::pair<int,int> >::iterator it_pair=i->second.begin(),it_pair_end=i->second.end();
	int accum_weight = 0.0;
	for(;it_pair!=it_pair_end;++it_pair){
	  accum_weight+=weights_greedy[*it_pair];
	}
	if(accum_weight>most_dist_nodes) {
	  most_dist_nodes = accum_weight;
	  largest_distinguishing_node_ = nodeID;
	}
      }
    }
 
    if((nSamples>1)&&(n!=nSamples+1)) {
      for(std::map<int,std::set<std::pair<int,int> > >::iterator k=sensorMap_.begin();k!=sensorMap_.end();++k) {
	int nodeID = k->first;
	if (nodeID != largest_distinguishing_node_) {
	  for(std::set<std::pair<int,int> >::iterator j = sensorMap_[largest_distinguishing_node_].begin(); j!=sensorMap_[largest_distinguishing_node_].end(); ++j) {
	    std::pair<int,int> pair_to_remove= *j;
	    std::set<std::pair<int,int> >::iterator it;
	    it = k->second.find(pair_to_remove);
	    if(it != k->second.end()) {
	      k->second.erase(it);
	    }
	  }
	}
      }    
      sensorMap_.erase(largest_distinguishing_node_);
    }
   
    obj += most_dist_nodes;
    // if the largest_distinguishing_node_ does not belongs to a fixed sensor node then add to the solution 
    // and only then  increment n
    if (std::count(fixed_sensor_ID.begin(), fixed_sensor_ID.end(), largest_distinguishing_node_) == 0){
    std::string nodesName = model_.NodeName(largest_distinguishing_node_);
       greedy_selected_nodes.insert(nodesName);
       n++;
    }
  }
  return obj;
}

void SampleNetworkSimulator::readFixedSensors(std::string fname, bool merlion)
{

  std::string error_msg = "Invalid Fixed Sensor file: ";
  error_msg += fname;
  std::ifstream in;
  in.open(fname.c_str(), std::ios::in);
  std::string tag;

  while(in.good())
    {
      in >> tag;
      if (tag.find(",") != std::string::npos) {
        std::cerr << std::endl << error_msg << std::endl;
        std::cerr << "Fixed sensors cannot be tuples!" << std::endl;
        std::cerr << std::endl;
        exit(1);
      }

      if (tag.length() < 1) continue; // read the next line
      if (merlion){
	if (!model_.isNode(tag)) {
	  std::cerr << std::endl << error_msg << std::endl;
	  std::cerr << "ERROR: Invalid Node Name in Merlion model: " << tag <<std::endl;
	  std::cerr << std::endl;
	  exit(1);		
	}
      }
      else {
	if ( EpanetIdx_.find(tag) == EpanetIdx_.end()) {
	  std::cerr << std::endl << error_msg << std::endl;
	  std::cerr << "ERROR: Invalid Node Name Epanet model: " << tag << std::endl;
	  std::cerr << std::endl;
	  exit(1);		
	}
      }
      int node_id;
      if (merlion) {
	node_id = model_.NodeID(tag);
      }
      else {
	node_id = EpanetIdx_[tag];
      }
      if (std::count(fixed_sensor_ID.begin(), fixed_sensor_ID.end(), node_id) == 0){
        fixed_sensor_ID.push_back(node_id);
      }
 
    }


}

void SampleNetworkSimulator::readAllowedNodes(std::string fname, bool merlion)
{

  std::string error_msg = "Invalid Allowed Nodes file: ";
  error_msg += fname;
  
  std::ifstream in;
  in.open(fname.c_str(), std::ios::in);
  std::string tag;

  while(in.good())
    {
      in >> tag;
      if (tag.find(",") != std::string::npos) {
        std::cerr << std::endl << error_msg << std::endl;
        std::cerr << "Allowed Node cannot be tuples!" << std::endl;
        std::cerr << std::endl;
        exit(1);
      }

      if (tag.length() < 1) continue; // read the next line
      if (merlion) {
	if (!model_.isNode(tag)) {
	  std::cerr << std::endl << error_msg << std::endl;
	  std::cerr << "ERROR: Invalid Node Name in Merlion Model: " <<  tag <<std::endl;
	  std::cerr << std::endl;
	  exit(1);		
	}
      }
      else {
	if ( EpanetIdx_.find(tag) == EpanetIdx_.end()) {
	  std::cerr << std::endl << error_msg << std::endl;
	  std::cerr << "ERROR: Invalid Node Name in Epanet Model: " << tag << std::endl;
	  std::cerr << std::endl;
	  exit(1);		
	}
      }
      int node_id;
      if (merlion) {
	node_id = model_.NodeID(tag);
      }
      else {
	node_id = EpanetIdx_[tag] ;
      }
      //std::cout << "Allowed Nodes :"; 
      if (std::count(allowed_node_ID.begin(), allowed_node_ID.end(), node_id) == 0){
        allowed_node_ID.push_back(node_id);
	//std::cout << node_id << std::endl;
      }
 
    }

}
/*
int SampleNetworkSimulator::NumberOfUncertainNodes()
{
  int n_uncertain = 0;
  for(int i = 0; i<model_.n_nodes;++i){
    //std::cout<< i << "  " << model_.NodeName(i) << std::endl;
    const Event& first_event = *events_.front();
    bool state = first_event.hasInfectedNodeID(i);
    bool state2 = state;

    for(EventList::iterator event_pos_i=events_.begin(),event_end=events_.end();event_pos_i!=event_end && state==state2;event_pos_i++)
    {
      	const Event& event = **event_pos_i;
        std::vector<int> infectedNodes = event.infectedNodes();
	if(i==0)
	{
	  std::cout<< "Senario "<<event.Name() << " ";
	  for(std::vector<int>::iterator it_node = infectedNodes.begin(),it_node_end = infectedNodes.end();it_node!=it_node_end;++it_node)
	    {
	      std::cout<< model_.NodeName(*it_node) << " ";
	    }
	  std::cout <<"\n";
	}
    }

    for(EventList::iterator event_pos_i=events_.begin(),event_end=events_.end();event_pos_i!=event_end && state==state2;event_pos_i++)
    {
	const Event& event = **event_pos_i;
	state2 = event.hasInfectedNodeID(i);
    }
    if(state!=state2)
    {
      std::cout<<"Uncertain node "<< model_.NodeName(i) <<"\n";
      n_uncertain+=1;
    }
  }
  std::cout<<"N_uncertain "<<n_uncertain<<"\n";
}
*/
