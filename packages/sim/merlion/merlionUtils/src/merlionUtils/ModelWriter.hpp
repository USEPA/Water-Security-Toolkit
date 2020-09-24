#ifndef MERLION_UTILS_MODELWRITER_HPP__
#define MERLION_UTILS_MODELWRITER_HPP__

#include <map>
#include <fstream>

/* forward declarations */
class Merlion;

namespace merlionUtils {

void PrintWQMHeadersToAMPLFormat(
   std::ostream& out,                     // The output file
   const Merlion* model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   int start_timestep=0,                  // By default prints out every timestep
   int stop_timestep=-1,
   std::string post_tag="");

void PrintWQMHeadersToPythonFormat(
   std::ostream& out,                     // The output file
   const Merlion* model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   int start_timestep=0,                  // By default prints out every timestep
   int stop_timestep=-1,
   std::string post_tag="");


void PrintWQMNodeTypesToAMPLFormat(
   std::ostream& out,                     // The output file
   const Merlion* model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   std::string post_tag="");

void PrintWQMNodeTypesToPythonFormat(
   std::ostream& out,                     // The output file
   const Merlion* model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   std::string post_tag="");


void PrintWQMToAMPLFormat(
   std::ostream& out,                     // The output file
   const Merlion* model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   int start_timestep=0,                  // By default prints out every timestep
   int stop_timestep=-1,
   std::string post_tag="");

void PrintWQMToPythonFormat(
   std::ostream& out,                     // The output file
   const Merlion* model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   int start_timestep=0,                  // By default prints out every timestep
   int stop_timestep=-1,
   std::string post_tag="");


void PrintDemandToAMPLFormat(
   std::ostream& out,                     // The output file
   const Merlion* model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   int start_timestep=0,                  // By default prints out every timestep
   int stop_timestep=-1,
   std::string post_tag="");

void PrintDemandToPythonFormat(
   std::ostream& out,                     // The output file
   const Merlion *model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   int start_timestep=0,                  // By default prints out every timestep
   int stop_timestep=-1,
   std::string post_tag="");
   
   
void PrintTankVolumesToAMPLFormat(
   std::ostream& out,                     // The output file
   const Merlion* model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   int start_timestep=0,                  // By default prints out every timestep
   int stop_timestep=-1,
   std::string post_tag="");
   
void PrintTankVolumesToPythonFormat(
   std::ostream& out,                     // The output file
   const Merlion *model,                  // The merlion model
   std::map<int,std::string>& label_map,  // The map taking merlion node ids to labels
   int start_timestep=0,                  // By default prints out every timestep
   int stop_timestep=-1,
   std::string post_tag="");

} /* end of merlionUtils namespace */

#endif
