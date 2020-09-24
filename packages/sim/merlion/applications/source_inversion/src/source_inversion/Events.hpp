#ifndef MERLION_UTILS_EVENTS_HPP__
#define MERLION_UTILS_EVENTS_HPP__

#include <merlionUtils/Scenarios.hpp>

#include<iostream>
#include <vector>
#include <set>

class Event : public merlionUtils::InjScenario
{
private:
  std::set<int> contaminated_node_IDS;
public:
   double probability; 
   Event():merlionUtils::InjScenario()
   {
     probability = 0.0;
   }
   
   virtual
   ~Event()
   {
      contaminated_node_IDS.clear();
   }
   
   void addContaminatedNodeID(int nodeID) {
       contaminated_node_IDS.insert(nodeID);
   }

  
   
   bool hasInfectedNodeID(int nodeID) const
   {
      std::set<int>::iterator it;
      it=contaminated_node_IDS.find(nodeID);
      bool found = false;
      (it==contaminated_node_IDS.end()) ? found = false : found = true;
      return found;
   }    
   
   const std::set<int>& infectedNodes() const {
      return contaminated_node_IDS;
   }
   
};
#endif
