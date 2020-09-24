#include <merlion/Network.hpp>

#include <cassert>

Network::Network(std::string name)
   :
   name_(name)
{}

Network::~Network()
{
   Link_vector::iterator lcur;
   for (lcur = links_vector_.begin(); lcur != links_vector_.end(); lcur++) 
   {
      Link* link = *lcur;
      delete link;
   }
   links_vector_.clear();
   links_map_.clear();

   Node_vector::iterator ncur;
   for (ncur = nodes_vector_.begin(); ncur != nodes_vector_.end(); ncur++) 
   {
      Node* node = (*ncur);
      delete node;
   }
   nodes_vector_.clear();
   nodes_map_.clear();  
}

const Node* Network::AddNode(std::string& name, NodeType type, double initial_volume_m3 /* = 0.0 */)
{
   // make sure the node does not exist
   assert(nodes_map_.find(name) == nodes_map_.end());
   // make sure the passed in data is reasonable
   assert(initial_volume_m3 >= 0.0);
   int unique_id = (int)nodes_vector_.size();
   Node* newnode = new Node(name, unique_id, type, initial_volume_m3);
   nodes_map_[name] = newnode;
   nodes_vector_.push_back(newnode);
   return newnode;
}

const Link* Network::AddLink(
   std::string& name,
   std::string& inlet_name,
   std::string& outlet_name,
   LinkType type,
   double area_m2 /*= 0.03242927866*/,
   double length_m /*= 0.0*/)
{
   // make sure the nodes exist
   assert(nodes_map_.find(inlet_name) != nodes_map_.end() && nodes_map_.find(outlet_name) != nodes_map_.end());
   // make sure the link does not
   assert(links_map_.find(name) == links_map_.end());
   // make sure the passed in data is reasonable
   assert(length_m >= 0.0 && area_m2 > 0.0);
   // make sure the length is only zero if the pipe is a pump or a valve
   assert( (length_m == 0.0 && (type == LinkType_Pump || type == LinkType_Valve)) || type == LinkType_Pipe );


   const Node* inlet_node = nodes_map_[inlet_name];
   const Node* outlet_node = nodes_map_[outlet_name]; 

   int unique_id = (int)links_vector_.size();
   Link* newlink = new Link(name, unique_id, inlet_node, outlet_node, type, length_m, area_m2);
   links_map_[name] = newlink;
   links_vector_.push_back(newlink);
   return newlink;
}

void Network::PushLinkConnectionsToNodes()
{
   Link_vector::const_iterator link_i;
   for (link_i = links_vector_.begin(); link_i != links_vector_.end(); link_i++) {
      int idx = (*link_i)->Inlet()->UniqueIDX();
      nodes_vector_[idx]->AddOutlet(*link_i);

   idx = (*link_i)->Outlet()->UniqueIDX();
   nodes_vector_[idx]->AddInlet(*link_i);
   }
}
