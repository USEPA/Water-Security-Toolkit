#ifndef SAMPLE_NETWORK_SIMULATOR_HPP__
#define SAMPLE_NETWORK_SIMULATOR_HPP__

#include <grab_sample_location/Events.hpp>

#include <merlionUtils/NetworkSimulator.hpp>
#include <epanet2.h>
//#include <epanetmsx.h>
extern "C" {
 #include <enl.h>
 #include <erd.h>
}
#include <tevautil.h>

#include <iostream>
#include <fstream>
#include <list>
#include <stack>
#include <sstream>
#include <vector>


struct wisePair
{
   std::pair<int,int> eventIds;
   std::vector<int> distinguishableNodeIds;
   std::pair<std::string,std::string> eventNames;
   
   void defineEventIds(int id1,int id2)
   {
      eventIds.first=id1;
      eventIds.second=id2;
   }
   
   void defineEventNames(std::string name1,std::string name2)
   {
      eventNames.first=name1;
      eventNames.second=name2;
   }
   
   inline std::pair<int,int>& pair()
   {return eventIds;}
   
   void addDistinguisableNode(int nodeId)
   {distinguishableNodeIds.push_back(nodeId);}
   
   /*inline std::vector<int>& DistinguishableNodeIds() {return distinguishableNodeIds;}*/
};

const float CONC_ZERO_TOL_GPM3     = 1e-30f;
typedef boost::shared_ptr<Event> PEvent;
typedef std::list<PEvent> EventList;

class SampleNetworkSimulator : public merlionUtils::NetworkSimulator
{

private:
  long numberOfPairs;
  wisePair** pairWiseP_; 
  std::map<int,std::set<std::pair<int,int> > > sensorMap_;
  std::map<std::pair<int,int>, int> weights_greedy;
public:
   EventList events_;
   bool with_weights;
  
   SampleNetworkSimulator(bool disable_warnings=false)
     :
     NetworkSimulator(disable_warnings)
   {
     with_weights = false;
   }

   virtual ~SampleNetworkSimulator()
   {
      clear();
   }

  std::map<std::string, int> EpanetIdx_;
  std::vector<int> fixed_sensor_ID;   
  std::vector<int> allowed_node_ID;
  //std::map<int,std::vector<int> > event_node_map_;
  std::set<std::string> greedy_selected_nodes; 
  void createEpanetLabelMap(PERD db);
  void readFixedSensors(std::string fixed_filename, bool merlion);
  void readAllowedNodes(std::string fname, bool merlion);
  void writeEvents();
  void BuildSets(bool);
  void BuildSets_teva(PERD db);
  float GreedySelection(int);
  int NumberOfEvents(){return events_.size();}
  //int NumberOfUncertainNodes();
  const EventList& Events() const {return events_;}
  const int getEpanetIdx(std::string name){return EpanetIdx_[name];}
  EventList& Events(){return events_;}
  wisePair** Pairs() const {return pairWiseP_;}
  wisePair** Pairs(){return pairWiseP_;}
  const long NumberOfPairs() const {return numberOfPairs;}
  void clear(); //hides the base class clear()   

};

#endif
