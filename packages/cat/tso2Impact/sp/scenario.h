/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top SPOT directory.
 *  _________________________________________________________________________
 */

#ifndef tevasp_scenario_h
#define tevasp_scenario_h

#include <utilib/exception_mngr.h>
#include <sp/ObjectiveBase.h>

extern "C"
{

}

extern const std::string NODE_MAP_FILE_SUFFIX;
extern const std::string SCENARIO_MAP_FILE_SUFFIX;

// useful helper stuff for dealing with the EPANET toolkit.
// lifted from TEVA code.
#define STRMAX 512
extern char emsg[];
#define ENCHECK(x) if ((x) > 0) { ENgeterror(x, emsg, STRMAX); EXCEPTION_MNGR(std::runtime_error, std::cerr << emsg << std::endl;);}


/// Process a scenario to compute impact values.
void processScenario(std::vector<ObjectiveBase*> &theObjectives,
                     int thisScenarioIndex,
                     double detectionDelay,
                     int detectionConfidence,
                     double minimumQuality,
                     PNetInfo net, PNodeInfo nodes, PLinkInfo links,
                     PSourceData, void *aggrInput, int speciesIndex);

#endif
