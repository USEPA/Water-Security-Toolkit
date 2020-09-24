#include <merlionUtils/EpanetLinker.hpp>

#include <merlion/Merlion.hpp>

#include <epanet2.h>

#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <cmath>
#include <fstream>
#include <set>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <vector>


// const float flow_tol_gpm = 1e-3;
// This is the old tolerance (1e-3 gallons per minute)
// converted to meters cubed per minute
const float flow_tol_m3pmin = 3.78541178e-6; 

const int n_samples = 500;

#define BOUNDARY
//#define AVERAGE

// forward declaration
class EPANETUnitsConverter;
bool SetHydInfo(Merlion* model, const EPANETUnitsConverter& units, int step_idx, float time_weight);

class EPANETUnitsConverter
{
public:

   EPANETUnitsConverter(int unitscode) 
   {

      if ((unitscode == EN_GPM) ||
          (unitscode == EN_CFS) ||
          (unitscode == EN_MGD) ||
          (unitscode == EN_IMGD) ||
          (unitscode == EN_AFD)) {
         // ft3 to m3
         volume_conversion = 0.0283168466;
         // ft to m
         length_conversion = 0.3048;
         // in to m
         pipe_diameter_conversion = 0.0254;
      }
      else if ((unitscode == EN_LPS) ||
               (unitscode == EN_LPM) ||
               (unitscode == EN_MLD) ||
               (unitscode == EN_CMH) ||
               (unitscode == EN_CMD)) {
         // m3 to m3
         volume_conversion = 1.0;
         // m to m
         length_conversion = 1.0;
         // mm to m
         pipe_diameter_conversion = 1e-3;
      }

      if (unitscode == EN_GPM) {
         // USgal/min to m3/min
         flow_conversion = 0.00378541178;
      }
      else if (unitscode == EN_CFS) {
         // ft3/s to m3/min
         flow_conversion = 1.6990108;
      }
      else if (unitscode == EN_MGD) {
         // MillionUSgal/day to m3/min
         flow_conversion = 2.62875818;
      }
      else if (unitscode == EN_IMGD) {
         // MillionImperialgal/min to m3/min
         flow_conversion = 3.15700825;
      }
      else if (unitscode == EN_AFD) {
         // acre-ft/day to m3/min
         flow_conversion = 0.856584609;         
      }
      else if (unitscode == EN_LPS) {
         // liters/s to m3/min
         flow_conversion = 0.06;
      }
      else if (unitscode == EN_LPM) {
         // liters/min to m3/min
         flow_conversion = 0.001;
      }
      else if (unitscode == EN_MLD) {
         // Megaliters/day to m3/min
         flow_conversion = 0.6944444444444444444444;
      }
      else if (unitscode == EN_CMH) {
         // m3/hour to m3/min
         flow_conversion = 0.01666666666666666666667;
      }
      else if (unitscode == EN_CMD) {
         // m3/day to m3/min
         flow_conversion = 0.00069444444444444444444;
      }
      else {
         std::cerr << std::endl;
         std::cerr << "***ERROR***: Unrecognized unitscode (" << unitscode << ") encountered " << 
                      "Unable to convert to Merlion standard units set." << std::endl;
         std::cerr << std::endl;
         exit(1);
      }
   }

   ~EPANETUnitsConverter() {};

   double volume_conversion;
   double length_conversion;
   double pipe_diameter_conversion;
   double flow_conversion;
   
};

void PrintENErrorMsg(int x) {
   char emsg[512];
   ENgeterror(x, emsg, 512); 
   std::cerr << emsg << std::endl; 
}

namespace merlionUtils {

Merlion* CreateMerlionFromEpanet(
   char* inp_file,
   char* txt_rpt_file,
   char* bin_rpt_file,
   float duration_min/*=-1.0*/, 
   float stepsize_min/*=-1.0*/,
   float scale/*=-1.0*/,
   int seed/*=-1*/,
   bool merlion_ignore_warnings/*=false*/,
   float decay_k_)
{
  Merlion* merlion = OpenEpanetAndCreateMerlion(inp_file, txt_rpt_file, bin_rpt_file, duration_min, stepsize_min, scale, seed, merlion_ignore_warnings,decay_k_);
   ENclose();
   return merlion;
}

Merlion* OpenEpanetAndCreateMerlion(
   char* inp_file,
   char* txt_rpt_file,
   char* bin_rpt_file, 
   float duration_min/*=-1.0*/,
   float stepsize_min/*=-1.0*/,
   float scale/*=-1.0*/,
   int seed/*=-1*/,
   bool merlion_ignore_warnings/*=false*/,
   float decay_k_)
{

   int errcode;
   // Initialize EPANET toolkit and read the input file
   if (errcode = ENopen(inp_file, txt_rpt_file, bin_rpt_file)) {
      std::cerr << "***ERROR***: ENopen failed with input file: "<< inp_file << std::endl;
      std::cerr << "                  and txt report file: "<< txt_rpt_file << std::endl;
      std::cerr << "                  and bin report file: "<< bin_rpt_file << std::endl;
      PrintENErrorMsg(errcode);
      return NULL;
   }

   // Create the Merlion object
   Merlion* model = new Merlion();

   //***
   // Read EPANET network information (and populate the Merlion object)
   //***

   //check the input file for situations not supported by Merlion
   bool warnings = false;

   // check for the type of hydraulic controls used
   int n_controls = -1;
   if ((errcode = ENgetcount(EN_CONTROLCOUNT, &n_controls)) > 0) {
      PrintENErrorMsg(errcode);
      return NULL;
   }

   // retrieve info for all the controls
   for (int i=0; i<n_controls; i++) {
      int ctype = -1;
      int lindex = -1;
      float setting = -1;
      int nindex = -1;
      float level = -1;
      
      if ((errcode = ENgetcontrol(i+1, &ctype, &lindex, &setting, &nindex, &level)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL; 
      }
      
      if (ctype != EN_TIMER && ctype != EN_TIMEOFDAY) {
	//std::cerr << "***WARNING***: Merlion only supports time controls. Please change level controls to time controls in the input file." << std::endl;
	// Suppressing warnings from showing up in the WST output - AS
	std::cout << "***WARNING***: Merlion only supports time controls. Please change level controls to time controls in the input file." << std::endl;
	warnings = true;
	break;
      }
   }

   // check for type of water quality analysis called for
   int qualcode = -1;
   int tracenode = -1;
   if ((errcode = ENgetqualtype(&qualcode, &tracenode)) > 0) {
      PrintENErrorMsg(errcode);
      return NULL;
   }

   if (qualcode != EN_NONE && qualcode != EN_CHEM) {
      std::cerr << "***ERROR***: Merlion does not support age or trace water quality analysis. Please change quality option to NONE or CHEMICAL in the input file." << std::endl;
      return NULL;
   }

   // check post-processing options
   long statcode = -1;
   if ((errcode = ENgettimeparam(EN_STATISTIC, &statcode)) > 0) {
      PrintENErrorMsg(errcode);
      return NULL;
   }

   if (statcode != EN_NONE) {
      std::cerr << "***ERROR***: Merlion does not support any statistical post-processing to the time series results. Please change statistic in the input file to NONE." << std::endl;
      return NULL;
   }

   // check for report start time
   long reportstart_sec = -1;
   if ((errcode = ENgettimeparam(EN_REPORTSTART, &reportstart_sec)) > 0) {
      PrintENErrorMsg(errcode);
      return NULL;
   }

   if (reportstart_sec != 0) {
      std::cerr << "***ERROR***: Merlion only supports report start time of 0:00. Please change Report Start Time in the input file to 0:00." << std::endl;
      return NULL;
   }

   // check the units
   int unitscode = -1;
   if ((errcode = ENgetflowunits(&unitscode)) > 0) {
      PrintENErrorMsg(errcode);
      return NULL;
   }

   EPANETUnitsConverter units(unitscode);  

   // check for quality and report timestep
   if (stepsize_min<0) {
      long reportstep_sec = -1;
      if ((errcode = ENgettimeparam(EN_REPORTSTEP, &reportstep_sec)) > 0) {
	 PrintENErrorMsg(errcode);
	 return NULL;
      }

      long qualstep_sec = -1;
      if ((errcode = ENgettimeparam(EN_QUALSTEP, &qualstep_sec)) > 0) {
	 PrintENErrorMsg(errcode);
	 return NULL;
      }

      if (reportstep_sec != qualstep_sec) {
	 std::cerr << "***ERROR***: Merlion only supports same report and quality time step. Please change them in the input file." << std::endl;
	 std::cerr << "Reporting timestep (seconds): " << reportstep_sec << std::endl;
	 std::cerr << "Quality timestep (seconds): " << qualstep_sec << std::endl;
	 return NULL; 
      }
   }

   // get the number of nodes from EPANET
   int n_nodes = -1;
   if ((errcode = ENgetcount(EN_NODECOUNT, &n_nodes)) > 0) {
      PrintENErrorMsg(errcode);
      return NULL; 
   }

   bool warn_init_qual = false;
   // retrieve info for all the nodes
   for (int i=0; i<n_nodes; i++) {
      char name[64];
      int node_type;

      // get the name and type of the node from EPANET
      if ((errcode = ENgetnodeid(i+1, name)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL; 
      }
   
      if ((errcode = ENgetnodetype(i+1, &node_type)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
   
      float initqual = -1;
      if ((errcode = ENgetnodevalue(i+1, EN_INITQUAL, &initqual)) > 0) {
	 PrintENErrorMsg(errcode);
	 return NULL;
      }

      if ((initqual > 0.0) && (warn_init_qual=false)){
         warn_init_qual = warnings = true;
	 std::cerr << "***WARNING***: Merlion does not use initial quality information. Please remove initial quality information from the input file." << std::endl;
      }

      if (node_type == EN_JUNCTION) {
         model->AddJunction(name);
      }
      else if (node_type == EN_RESERVOIR) {
         model->AddReservoir(name);
      }
      else if (node_type == EN_TANK) {
	 float mixmodel = -1;
         if ((errcode = ENgetnodevalue(i+1, EN_MIXMODEL, &mixmodel)) > 0) {
            PrintENErrorMsg(errcode);
            return NULL;
         }

	 if (mixmodel != EN_MIX1) {
	    std::cerr << "***ERROR***: Merlion only supports MIXED models for storage tanks. Please change each tank's mixing model to MIXED in the input file." << std::endl;
	    return NULL;
	 }

         float volume_ENUNIT = 0.0f;
         if ((errcode = ENgetnodevalue(i+1, EN_INITVOLUME, &volume_ENUNIT)) > 0) {
            PrintENErrorMsg(errcode);
            return NULL;
         }
         double volume_m3 = volume_ENUNIT*units.volume_conversion;

         model->AddTank(name, volume_m3);
      }
      else {
         std::cerr << "***ERROR***: Invalid node typecode: " << node_type << " returned for node: " << name << std::endl;
         return NULL;
      }
   }

   if (warnings && !merlion_ignore_warnings) {
      return NULL;
   }
   else if (warnings && merlion_ignore_warnings) {
     //std::cerr << "***Ignoring above Warnings***" << std::endl;
     // Suppressing warning ignored notification -AS 
     std::cout << "***Ignoring above Warnings***" << std::endl;
   }

   // Retrieve info for all the links
   int n_links = -1;
   if ((errcode = ENgetcount(EN_LINKCOUNT, &n_links)) > 0) {
      PrintENErrorMsg(errcode);
      return NULL;
   } 
   if (decay_k_ < 0.0) { // if no override by the user  
      float kbulk = -1;
      if ((errcode = ENgetlinkvalue(1, EN_KBULK, &kbulk)) > 0) { // Reaction decay_constant for the first link
	 PrintENErrorMsg(errcode);
	 return NULL;
      }
      decay_k_ = -kbulk/(24*60); //Units conversion from 1/days to 1/min 
   } 

   if (decay_k_ < 0.0) {
     std::cerr << "***ERROR***: Merlion only supports decay(not growth) in bulk reactions. Please change the bulk reaction constant in the input file." << std::endl;
     return NULL;
   }


   for (int i=0; i<n_links; i++) {
      char name[64];
      int link_type, from_node_idx, to_node_idx;
      std::string str_from_node, str_to_node;

      // get the index of the nodes connected to this pipe
      if ((errcode = ENgetlinknodes(i+1, &from_node_idx, &to_node_idx)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
   
      // get the id (name or label) corresponding to the index
      if ((errcode = ENgetnodeid(from_node_idx, name)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
      str_from_node = name;
   
      if ((errcode = ENgetnodeid(to_node_idx, name)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
      str_to_node = name;
   
      // get link info
      if ((errcode = ENgetlinkid(i+1, name)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
   
      if ((errcode = ENgetlinktype(i+1, &link_type)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
   

      /*if (kbulk != 0.0) {
	 std::cerr << "***ERROR***: Merlion does not support bulk reactions. Please set bulk reaction coefficient to 0 in the input file." << std::endl;
	 return NULL;
      }
      */

      float kwall = -1;
      if ((errcode = ENgetlinkvalue(i+1, EN_KWALL, &kwall)) > 0) {
	 PrintENErrorMsg(errcode);
	 return NULL;
      }

      if (kwall != 0.0) {
	 std::cerr << "***ERROR***: Merlion does not support wall reactions. Please set wall reaction coefficient to 0 in the input file." << std::endl;
	 return NULL;
      }

      if (link_type == EN_CVPIPE || link_type == EN_PIPE) {
         // link is a pipe or a pipe with a valve
   
         // get the diameter
         float diameter_ENUNIT = 0;
         if ((errcode = ENgetlinkvalue(i+1, EN_DIAMETER, &diameter_ENUNIT)) > 0) {
            PrintENErrorMsg(errcode);
            return NULL;
         }
         double diameter_m = diameter_ENUNIT*units.pipe_diameter_conversion;
         double area_m2 = 3.14159265*(diameter_m*diameter_m/4.0);
         //float area_m2 = 3.14159265*(diameter_ENUNIT*diameter_ENUNIT/4.0)*units.pipe_diameter_conversion;
      
         float length_ENUNIT = 0;
         if ((errcode = ENgetlinkvalue(i+1, EN_LENGTH, &length_ENUNIT)) > 0) {
            PrintENErrorMsg(errcode);
            return NULL;
         }
         float length_m = length_ENUNIT*units.length_conversion;

         model->AddPipe(name, str_from_node, str_to_node, area_m2, length_m);
      }
      else if (link_type == EN_PUMP) {
         // link is a pump
         model->AddPump(name, str_from_node, str_to_node);
      }
      else if (link_type == EN_PRV ||
               link_type == EN_PSV ||
               link_type == EN_PBV ||
               link_type == EN_FCV ||
               link_type == EN_TCV ||
               link_type == EN_GPV) {
         // link is a valve
         float diameter_ENUNIT = 0;
         if ((errcode = ENgetlinkvalue(i+1, EN_DIAMETER, &diameter_ENUNIT)) > 0){
            PrintENErrorMsg(errcode);
            return NULL;
         }
         double diameter_m = diameter_ENUNIT*units.pipe_diameter_conversion;
         double area_m2 = 3.14159265*(diameter_m*diameter_m/4.0);

         model->AddValve(name, str_from_node, str_to_node, area_m2);
      }
      else {
         std::cerr << "***ERROR***: Invalid link typecode: " << link_type << " returned for link: " << name << std::endl;
         return NULL;
      }
   }

   model->BuildNetwork();

   model->CreateNodeToLinkMaps();
   // Set simulation options/specs
   if (duration_min<0) {
      long duration_sec = -1;
      if ((errcode = ENgettimeparam(EN_DURATION, &duration_sec)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
      duration_min = duration_sec/60.0; // simulation duration
   }
   else {
      if ((errcode = ENsettimeparam(EN_DURATION, duration_min*60.0)) > 0) { // simulation duration
         PrintENErrorMsg(errcode);
         return NULL;
      }
   }

   if (stepsize_min<0) {
      long qual_stepsize_sec = -1;
      if ((errcode = ENgettimeparam(EN_QUALSTEP, &qual_stepsize_sec)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
      stepsize_min = qual_stepsize_sec/60.0;
   }

   // Get hydraulic data
   model->InitializeHydraulicMatrices(duration_min, stepsize_min);
   int n_steps = model->NSteps();

   // Add noise to base demand ***OPTIONAL***
   if (scale > 0) {
      if (seed<0) {
         srand(time(NULL));
      }
      else {
         srand(seed);
      }

      for (int i=0; i<n_nodes; i++) {
         float demand_ENUNIT = -1;
         if ((errcode = ENgetnodevalue(i+1, EN_BASEDEMAND, &demand_ENUNIT)) > 0) {
            PrintENErrorMsg(errcode);
            return NULL;
         }
         double demand_m3pmin = demand_ENUNIT*units.flow_conversion;

         if (demand_m3pmin >= flow_tol_m3pmin) {
            float noise = 0.0f;
            for (int j=0; j<n_samples; j++) {
               noise = noise + rand()/double(RAND_MAX);
            }
            noise = scale*sqrt(12.0/n_samples)*(noise-n_samples/2.0);
            demand_m3pmin += noise*demand_m3pmin;
            if (demand_m3pmin < flow_tol_m3pmin) {
               demand_m3pmin = 0.0f;
            }
            if ((errcode = ENsetnodevalue(i+1, EN_BASEDEMAND, demand_m3pmin/units.flow_conversion)) > 0) {
               PrintENErrorMsg(errcode);
               return NULL;
            }
         }
      }
   }

   // open the hydraulics analysis system
   if ((errcode = ENopenH()) > 0) {
      PrintENErrorMsg(errcode);
      return NULL;
   }

   // initialize the hydraulic engine (10 means re-initialize flows, but do not save a results file)
   if ((errcode = ENinitH(10)) > 0) {
      PrintENErrorMsg(errcode);
      return NULL;
   }

   int qtimestep_sec = stepsize_min*60.0+0.1;
   int qstep_sec_advanced = 0;
   int step_idx = 0;
   long step_sec = qtimestep_sec; // step length in seconds
   long current_t_sec = 0;

   do {
      if ((errcode = ENrunH(&current_t_sec)) > 0) {
         PrintENErrorMsg(errcode);
         //return NULL;
      }
      int time_sec_advanced = 0;
      float time_weight = 0.0;

   #ifdef AVERAGE
      if (step_sec <= qtimestep_sec) {
         if (qstep_sec_advanced+step_sec >= qtimestep_sec) {
            time_sec_advanced = qtimestep_sec-qstep_sec_advanced;
            time_weight = (float)time_sec_advanced/(float)qtimestep_sec;
            if (!SetHydInfo(model, units, step_idx, time_weight)) {
               std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
               return NULL;
            }
            step_idx++;
            qstep_sec_advanced = step_sec-time_sec_advanced;
            time_sec_advanced = qstep_sec_advanced;
            time_weight = (float)time_sec_advanced/(float)qtimestep_sec;
            if (!SetHydInfo(model, units, step_idx, time_weight)) {
               std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
               return NULL;
            }
         }
         else { // does not need to update step_idx
            qstep_sec_advanced = qstep_sec_advanced+step_sec;
            time_sec_advanced = step_sec;
            time_weight = (float)time_sec_advanced/(float)qtimestep_sec;
            if (!SetHydInfo(model, units, step_idx, time_weight)) {
               std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
               return NULL;
            }
         }
      } 
      else if (step_sec > qtimestep_sec) {
         long step_sec_left = step_sec;
         while (step_sec_left >= qtimestep_sec) {
            time_sec_advanced = qtimestep_sec-qstep_sec_advanced;
            step_sec_left = step_sec_left-time_sec_advanced;
            time_weight = (float)time_sec_advanced/(float)qtimestep_sec;
            if (!SetHydInfo(model, units, step_idx, time_weight)) {
               std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
               return NULL;
            }
            step_idx++;
            qstep_sec_advanced = 0;
         }
         qstep_sec_advanced = step_sec_left;
         time_sec_advanced = qstep_sec_advanced;
         time_weight = (float)time_sec_advanced/(float)qtimestep_sec;
         if (!SetHydInfo(model, units, step_idx, time_weight)) {
            std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
            return NULL;
         }
      } 
   #endif
   
   #ifdef BOUNDARY
      if (step_sec <= qtimestep_sec) {
         if (qstep_sec_advanced+step_sec >= qtimestep_sec) {
            time_sec_advanced = qtimestep_sec-qstep_sec_advanced;
            time_weight = 1.0;
            if (!SetHydInfo(model, units, step_idx, time_weight)) {
               std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
               return NULL;
            }
            step_idx++;
            qstep_sec_advanced = step_sec-time_sec_advanced;
            time_sec_advanced = qstep_sec_advanced;
            time_weight = 1.0;
            if (qstep_sec_advanced != 0) {
               if (!SetHydInfo(model, units, step_idx, time_weight)) {
                  std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
                  return NULL;
               }
            }
         }
         else { // does not need to update step_idx
            qstep_sec_advanced = qstep_sec_advanced+step_sec;
            time_sec_advanced = step_sec;
            time_weight = 1.0;
            if (!SetHydInfo(model, units, step_idx, time_weight)) {
               std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
               return NULL;
            }
         }
      } 
      else if (step_sec > qtimestep_sec) {
         long step_sec_left = step_sec;
         while (step_sec_left >= qtimestep_sec) {
            time_sec_advanced = qtimestep_sec-qstep_sec_advanced;
            step_sec_left = step_sec_left-time_sec_advanced;
            time_weight = 1.0;
            if (!SetHydInfo(model, units, step_idx, time_weight)) {
               std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
               return NULL;
            }
            step_idx++;
            qstep_sec_advanced = 0;
         }
         qstep_sec_advanced = step_sec_left;
         time_sec_advanced = qstep_sec_advanced;
         time_weight = 1.0;
         if (qstep_sec_advanced != 0) {
            if (!SetHydInfo(model, units, step_idx, time_weight)) {
               std::cerr << "***ERROR***: Failed to set hydraulic information." << std::endl;
               return NULL;
            }
         }
      } 
   #endif
   
      if ((errcode = ENnextH(&step_sec)) > 0) {
         PrintENErrorMsg(errcode);
         return NULL;
      }
   } while (step_sec > 0);

   assert(step_idx == n_steps);
   ENcloseH();
   std::cout << "Hydraulic simulation ran for a duration of: " << current_t_sec/3600.0 << " hr" << std::endl;

   if (!model-> BuildWQM(decay_k_)) {
      std::cerr << "***ERROR***: Failed to build water quality model. Internal Error." << std::endl;
      return NULL;
   }
   
   return model;
}

} /* end of merlionUtils namespace */

bool _set_hyd_info(Merlion* model, const EPANETUnitsConverter& units, int step_idx, float time_weight)
{
   int n_nodes = model->NNodes();
   int n_links = (int)model->LinkIdxNameMap().size();
   int errcode;
   
   // get demand info - note: tanks have strange behavior
   for (int i=0; i<n_nodes; i++) {
      float demand_ENUNIT = -1;
      if ((errcode = ENgetnodevalue(i+1, EN_DEMAND, &demand_ENUNIT)) > 0) {
         PrintENErrorMsg(errcode);
         return false;
      }
      double demand_m3pmin = demand_ENUNIT*units.flow_conversion;
      
      int node_type = 0;
      if ((errcode = ENgetnodetype(i+1, &node_type)) > 0) {
         PrintENErrorMsg(errcode);
         return false;
      }
      
      if (node_type == EN_TANK) {
         // this node is a tank. EPANET returns the sum of all exiting
         // flows as "demand" for a tank, even though these flows go
         // back to the network. This can cause miscalculation since
         // we assume that all demand flows to residential or
         // industrial users. Therefore, here we set the demand to
         // zero.
#ifdef AVERAGE
         model->AddToDemand_m3pmin(i, step_idx, 0.0);
#endif
#ifdef BOUNDARY
         model->SetDemand_m3pmin(i, step_idx, 0.0);
#endif
      }
      else {
#ifdef AVERAGE
         model->AddToDemand_m3pmin(i, step_idx, time_weight*demand_m3pmin);
#endif
#ifdef BOUNDARY
         model->SetDemand_m3pmin(i, step_idx, time_weight*demand_m3pmin);
#endif
      }
   }
   
   // get velocity info
   for (int i=0; i<n_links; i++) {
      float flow_ENUNIT = -1;
      if ((errcode = ENgetlinkvalue(i+1, EN_FLOW, &flow_ENUNIT)) > 0) {
         PrintENErrorMsg(errcode);
         return false;
      }
      double flow_m3pmin = flow_ENUNIT*units.flow_conversion;
      
      if (fabs(flow_m3pmin) < flow_tol_m3pmin && flow_m3pmin != 0.0) { // Small flow value, setting to zero.
         flow_m3pmin = 0.0f;
      }
      
      float diameter_ENUNIT = 8.0;
      int link_type;
      if ((errcode = ENgetlinktype(i+1, &link_type)) > 0) {
         PrintENErrorMsg(errcode);
         return false;
      }
      
      if (link_type != EN_PUMP) {
         if ((errcode = ENgetlinkvalue(i+1, EN_DIAMETER, &diameter_ENUNIT)) > 0) {
            PrintENErrorMsg(errcode);
            return false;
         }
      }
      double diameter_m = diameter_ENUNIT*units.pipe_diameter_conversion;
      double area_m2 = 3.14159265*(diameter_m*diameter_m/4.0);
      
      double velocity_mpmin = flow_m3pmin/area_m2;
#ifdef AVERAGE 
      model->AddToVelocity_mpmin(i, step_idx, time_weight*velocity_mpmin);
#endif
#ifdef BOUNDARY
      model->SetVelocity_mpmin(i, step_idx, time_weight*velocity_mpmin);
#endif
   }
   
   return true;
}

bool SetHydInfo(Merlion* model, const EPANETUnitsConverter& units, int step_idx, float time_weight)
{
   bool ret = true;
   if (step_idx == 0) {
      ret = _set_hyd_info(model, units, step_idx, time_weight);
   }
   if (ret && (step_idx < model->NSteps()-1)) {
      step_idx++;
      ret = _set_hyd_info(model, units, step_idx, time_weight);
   }
   return ret;
}
