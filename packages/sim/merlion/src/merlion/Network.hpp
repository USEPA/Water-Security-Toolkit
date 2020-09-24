#ifndef MERLION_NETWORK_HPP__
#define MERLION_NETWORK_HPP__

#include <merlion/MerlionDefines.hpp>

#include <string>
#include <vector>
#include <map>
#include <cassert>

// forward declarations
class Link;
class Node;

// structure for comparisons in a map of std::strings
struct ltstdstr
{
   bool operator()(const std::string& s1, const std::string& s2) const
   {
      return s1 < s2;
   }
};

// typedefs
typedef std::map<const std::string, Link*, ltstdstr> Link_map;
typedef std::map<const std::string, Node*, ltstdstr> Node_map;
typedef std::vector<Link*> Link_vector;
typedef std::vector<Node*> Node_vector;
typedef std::vector<const Link*> Link_vector_const;
typedef std::vector<const Node*> Node_vector_const;

class Node
{
public:
   Node(std::string name, int unique_idx, NodeType type, double initial_volume_m3 = 0.0)
      : 
      name_(name),
      unique_idx_(unique_idx),
      type_(type),
      initial_volume_m3_(initial_volume_m3)
   {
   }

   ~Node() 
   {
   };

   const std::string& Name() const
   { return name_; }

   const int& UniqueIDX() const
   { return unique_idx_; }

   const NodeType& Type() const
   { return type_; }

   const double& InitialVolume_m3() const
   {
      assert(type_ == NodeType_Tank);
      return initial_volume_m3_;
   }

   void AddInlet(const Link* inlet)
   { 
      assert(inlet != NULL);
      inlets_.push_back(inlet); 
   }

   void AddOutlet(const Link* outlet)
   { 
      assert(outlet != NULL);
      outlets_.push_back(outlet);
   }

   const Link_vector_const& Inlets() const
   { return inlets_; }

   const Link_vector_const& Outlets() const
   { return outlets_; }

private:
   const std::string name_;
   const int unique_idx_;
   const NodeType type_;
   const double initial_volume_m3_;
   Link_vector_const inlets_;
   Link_vector_const outlets_;
};

class Link
{
public:
   Link(
      std::string name,
      int unique_idx,
      const Node* inlet,
      const Node* outlet,
      LinkType type, 
      double length_m,
      double area_m2)
   :
   name_(name),
   unique_idx_(unique_idx),
   inlet_(inlet),
   outlet_(outlet),
   type_(type),
   length_m_(length_m),
   area_m2_(area_m2)
   {
   }

   ~Link()
   {
   }

   const std::string& Name() const
   { return name_; }

   const int& UniqueIDX() const
   { return unique_idx_; }

   const LinkType& Type() const
   { return type_; }

   const double& Length_m() const
   { return length_m_; }

   const double& Area_m2() const
   { return area_m2_; }

   const Node* Inlet() const
   { return inlet_; }

   const Node* Outlet() const
   { return outlet_; }

private:
   const std::string name_;
   const int unique_idx_;
   const Node* const inlet_;
   const Node* const outlet_;
   const LinkType type_;
   const double length_m_;
   const double area_m2_;
};

class Network
{
public:
   Network(std::string name);

   ~Network();

   const Node* AddNode(std::string& name, NodeType type, double initial_volume_m3 = 0.0);

   // Add link to the network (Note: default values assume that all pumps are 8in in diameter with zero length)
   const Link* AddLink(
         std::string& name,
         std::string& inlet_name,
         std::string& outlet_name,
         LinkType type, 
         double area_m2 = 0.03242927866,
         double length_m = 0.0);

   const Link_map& LinksMap() const
   { return links_map_; }

   const Node_map& NodesMap() const
   { return nodes_map_; }

   const Link_vector& LinksVector() const
   { return links_vector_; }

   const Node_vector& NodesVector() const
   { return nodes_vector_; }
   std::string Name()
   { return name_;}

   void PushLinkConnectionsToNodes();

private:
   const std::string name_;
   Link_map links_map_;
   Node_map nodes_map_;
   Link_vector links_vector_;
   Node_vector nodes_vector_;
};

#endif
