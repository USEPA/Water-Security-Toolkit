#ifndef MERLION_UTILS_EPANET_LINKER_HPP__
#define MERLION_UTILS_EPANET_LINKER_HPP__

#include <vector>
#include <string>

/* forward declarations */
class Merlion;

namespace merlionUtils {
// Use this method to create a Merlion object from an EPANET inp file
Merlion* CreateMerlionFromEpanet(
   char* inp_file,
   char* txt_rpt_file,
   char* bin_rpt_file,
   float duration_hr=-1.0,
   float timestep_hr=-1.0,
   float scale=-1.0,
   int seed=-1,
   bool merlion_ignore_warnings=false,
   float decay_k_ = -1.0f);

// Use this method to create a Merlion object from an EPANET inp file
// Note: In this function, the EPANET object is LEFT OPEN.
//       You must call ENclose()
Merlion* OpenEpanetAndCreateMerlion(
   char* inp_file,
   char* txt_rpt_file,
   char* bin_rpt_file,
   float duration_hr=-1.0,
   float timestep_hr=-1.0,
   float scale=-1.0,
   int seed=-1,
   bool merlion_ignore_warnings=false,
   float decay_k_ = -1.0f);

} /* end of merlionUtils namespace */

#endif
