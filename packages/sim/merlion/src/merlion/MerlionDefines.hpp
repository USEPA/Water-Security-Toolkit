#ifndef MERLION_MERLION_DEFINES_HPP__
#define MERLION_MERLION_DEFINES_HPP__

#include <string>

enum LinkType
   {
      LinkType_Valve,
      LinkType_Pump,
      LinkType_Pipe
   };

enum NodeType 
   {
      NodeType_Junction,
      NodeType_Tank,
      NodeType_Reservoir
   };

// GAH: Unable to add these without getting duplicate symbol errors.
/*
std::string LinkTypeToString(LinkType link)
{
   if (link == LinkType_Valve) {return "LinkType_Valve";}
   if (link == LinkType_Pump) {return "LinkType_Pump";}
   if (link == LinkType_Pipe) {return "LinkType_Pipe";}
   return "";
}

std::string NodeTypeToString(NodeType node)
{
   if (node == NodeType_Junction) {return "NodeType_Junction";}
   if (node == NodeType_Tank) {return "NodeType_Tank";}
   if (node == NodeType_Reservoir) {return "NodeType_Reservoir";}
   return "";
}
*/

#endif
