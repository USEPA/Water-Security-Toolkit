#ifndef MERLION_UTILS_EVENTS_HPP__
#define MERLION_UTILS_EVENTS_HPP__

#include <merlionUtils/Scenarios.hpp>

#include<iostream>
#include <vector>
#include <set>

class Event : public merlionUtils::InjScenario
{
private:
   std::vector<int> contaminated_node_IDS;
   std::set<int> setContaminated_node_IDS;
public:
   Event():merlionUtils::InjScenario()
   {
   }
   
   virtual
   ~Event()
   {
      contaminated_node_IDS.clear();
   }
   
   void addContaminatedNodeID(int nodeID) {
      contaminated_node_IDS.push_back(nodeID);
      //setContaminated_node_IDS.
   }
   
   bool hasInfectedNodeID(int nodeID) const
   {
      bool found=false;
      for(int i=0;i<contaminated_node_IDS.size() && !found;i++) {
	 if(nodeID==contaminated_node_IDS[i]){found=true;}
      }
      return found;
   }    
   
   const std::vector<int>& infectedNodes() const {
      return contaminated_node_IDS;
   }
   
};
#endif
