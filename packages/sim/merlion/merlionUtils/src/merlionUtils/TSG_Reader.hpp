#ifndef MERLION_UTILS_TSG_READER_HPP__
#define MERLION_UTILS_TSG_READER_HPP__

#include <merlionUtils/Scenarios.hpp>

#include <vector>
#include <string>

/* forward declarations */
class Merlion;

namespace merlionUtils {

void ReadTSG(std::string tsg_filename,
             Merlion* model,
             InjScenList& injection_scenarios);

void ReadTSI(std::string tsi_filename,
             Merlion* model,
             InjScenList& injection_scenarios,
             int species_id=-1);

} /* end of merlionUtils namespace */


#endif
