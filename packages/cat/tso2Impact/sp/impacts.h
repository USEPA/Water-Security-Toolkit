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

/*!
 * \file impacts.h
 * \brief Routines to evaluate the impact of a contamination event, given
 *      sensors.
 */

#ifndef tevasp_impacts_h
#define tevasp_impacts_h

#include <iostream>
#include <map>
#include <string>
#include <list>
#include <vector>
#include <utilib/BasicArray.h>
#include <utilib/_math.h>
#include <utilib/BitArray.h>
#include <sp/scenario.h>

#include "VecTrie.h"

// required for the health impacts objective
#ifdef __cplusplus
extern "C" {
#endif
#include "teva-assess.h"
#include "ta_input.h"
#include "tai.h"
#include "sss.h"
#include "ta_utils.h"
#ifdef __cplusplus
}
#endif

//#define OLD_IMPACT_FORMAT

template <class T>
void read_array(std::istream& istr, utilib::BasicArray<T>& array)
{
   int tmp;
   istr >> tmp;
   array.resize(tmp);
   for (int i = 0; i < tmp; i++)
      istr >> array[i];
}


/**
 * Read a file with invalid locations, and setup a BitArray with
 * the feasibility flags.
 */
inline void read_invalid_locations(std::istream& istr, utilib::BasicArray<int>& invalid_loc, utilib::BitArray& feasible_locations, std::map<int, int>& reduced_repn_map)
{
   read_array(istr, invalid_loc);
   feasible_locations.set();
   for (size_type i = 0; i < invalid_loc.size(); i++)
      if ((invalid_loc[i] >= 1) && ((size_type)invalid_loc[i] <= feasible_locations.size()))
         feasible_locations.reset(invalid_loc[i] - 1);

   int j = 1;
   for (size_type i = 0; i < feasible_locations.size(); i++)
      if (feasible_locations(i))
         reduced_repn_map[i+1] = j++;
}


/**
 * Read a node map file.
 * Format
 *    <node-id> <string>
 */
inline void read_nodemap(std::istream& istr, std::map<int, std::string>& nodemap)
{
   int id;
   istr >> id;
   while (istr)
   {
      std::string tmp;
      istr >> tmp;
      nodemap[id] = tmp;
      istr >> id;
   }
}



/**
 * Read a file containing weights of different contamination incidents.
 * Format:
 *    <node-id> <probability>
 */
void read_incident_weights(std::istream& istr, std::map<int, double>& weights);

inline void read_incident_weights(std::istream& istr, utilib::BasicArray<double>& weights)
{
   std::map<int, double> vals;
   read_incident_weights(istr, vals);
   for (unsigned int i = 0; i < weights.size(); i++)
      weights[i] = 0.0;
   std::map<int, double>::iterator curr = vals.begin();
   std::map<int, double>::iterator end  = vals.end();
   while (curr != end)
   {
      weights[curr->first] = curr->second;
      curr++;
   }
}


inline void read_costs(std::istream& istr, std::map<std::string, double>& costs)
{
   std::string id;
   istr >> id;
   while (istr)
   {
      double cost;
      istr >> cost;
      costs[id] = cost;
      istr >> id;
   }
}


inline void read_id_costs(std::istream& istr, std::map<int, double>& costs)
{
   int len = 0;
   istr >> len;
   for (int i = 0; i < len; i++)
   {
      int index;
      double val;
      istr >> index >> val;
      costs[index] = val;
   }
}


/**
 * Read an sc probabilities file.
 * Format
 *    <sc> <probability>
 */
inline void read_sc_probabilities(std::istream& istr, std::map<int, double>& sc_probabilities)
{
   int sc;
   istr >> sc;
   while (istr)
   {
      double probability;
      istr >> probability;
      sc_probabilities[sc] = probability;
      istr >> sc;
   }
}


/**
 * Read an scs file.
 * Format
 *    <node-id> <sc>
 */
inline void read_scs(std::istream& istr, std::map<int, int>& scs)
{
   int id;
   istr >> id;
   while (istr)
   {
      int sc;
      istr >> sc;
      scs[id] = sc;
      istr >> id;
   }
}


/**
 * Read an impact file.
 *
 * The arrival_sequences parameter is passed as a pointer. If this is NULL,
 * then it is ignored.
 */
void read_impacts(std::istream& istr,
                  std::vector<std::map<int, double> >& impacts,
                  std::vector<std::vector<std::pair<int, double> > > *arrival_sequences,
                  int delay,
                  std::vector<std::list<std::pair<int, int> > >* wtime,
                  utilib::BitArray& feasible_location,
                  std::map<int, int>& reduced_repn_map);

int read_impact_preamble(std::istream& istr, int& nnodes, int delay);

int read_next_impact(std::istream& istr, int eventIndex,
                     std::map<int, double>& next_event_impacts,
                     std::vector<std::pair<int, int> > &next_event_sequence,
                     int delayIndex);

#if 0
//WEH - depricated because it's not being used
/**
 * Take raw impact information and a delay, and compute new
 * impact values that take into account these delays.
 */
void compute_delayed_impacts(utilib::BasicArray<std::map<int, std::pair<double, double> > >& raw_impacts, double delay, utilib::BasicArray<std::map<int, double> >& impacts);
#endif

/**
 * Read sensor locations from a file.
 */
void read_sensor_placements(std::istream& istr, std::map<int, utilib::BasicArray<int> >& sensors);

/**
 * Read network ids from a file.
 * Format:
 *    <node-id>
 */
void read_network_ids(std::istream& istr, utilib::BasicArray<int>& sensors);


/// A class that contains summary statistics about the performance of a sensor placement.
class ImpactStats
{
public:

   ImpactStats()
         : goal(""), minImpact(-DBL_MAX), maxImpact(DBL_MIN), medianImpact(0.0),
         firstQImpact(0.0), thirdQImpact(0.0), meanImpact(0.0), VaR(0.0), TCE(0.0),
         nscenarios(0)
   {}

   ///
   std::string goal;

   ///
   double minImpact;

   ///
   double maxImpact;

   ///
   double medianImpact;

   ///
   double firstQImpact;

   ///
   double thirdQImpact;

   ///
   double meanImpact;

   ///
   double VaR;

   ///
   double TCE;

   ///
   int nscenarios;

   ///
   utilib::BasicArray<int> witness;  // indexed by event number

   ///
   utilib::BasicArray<double> impact;// indexed by event number
};

/// Output sensors in sorted order by decreasing impact mitigated

void compute_greedy_order(const utilib::BasicArray<int>& sensors,
                          std::vector<std::map<int, double> >& impacts,
                          std::vector<std::vector<std::pair<int, double> > > &arrival_sequences,
                          std::map<int, int>& scs,
                          std::map<int, double>& sc_probabilities,
                          ImpactStats& stats,
                          std::map<int, double>& probabilities,
                          utilib::BasicArray<int>& work,
                          std::list<std::list<std::pair<int, double> > >&
                          resultsOfPartialSPs,
                          double gamma);

/// Compute impact statistics
void compute_statistics(const utilib::BasicArray<int>& sensors,
                        std::vector<std::map<int, double> >& impacts,
                        std::vector<std::vector<std::pair<int, double> > > &arrival_sequences,
                        std::map<int, int>& scs, std::map<int, double>& sc_probabilities,
                        ImpactStats& stats,
                        std::map<int, double>& probabilities,
                        utilib::BasicArray<int>& work,
                        double alpha);


/// Write impact statistics

/// The "impacts" are stored as follows:
///    -  the top level vector is indexed by the id of the sensor placement
///    -  the nested vector is indexed by the impact type (mc, vc, nfd, etc)
///    -  the map maps event numbers to impact values
void write_impact_statistics(int id, const utilib::BasicArray<int>& sensors,
                             utilib::BasicArray<std::map<int, ImpactStats> >& stats,
                             utilib::BasicArray<std::vector<std::map<int, double> > >& impacts,
                             const std::list<std::string>& files, const std::string& format,
                             bool first_flag, bool debug, std::map<int, std::string>& nodemap,
                             std::map<std::string, double>& costs, double gamma);

void write_impact_statistics(std::map<int, utilib::BasicArray<int> >& sensors,
                             utilib::BasicArray<std::map<int, ImpactStats> >& stats,
                             utilib::BasicArray<std::vector<std::map<int, double> > >& impacts,
                             const std::list<std::string>& files, const std::string& format,
                             bool first_flag, bool debug, std::map<int, std::string>& nodemap,
                             std::map<std::string, double>& costs, double gamma);
///
/// Show the effect of adding sensors in a greedy order
///
void write_greedy_order(
   std::list<std::list<std::pair<int, double> > >& resultsOfPartialSPs,
   const std::list<std::string>& files, const std::string& format);

void process_arguments(int argc, char **argv, PNodeInfo&nodes, PLinkInfo&links,
                       PNetInfo&net, PTSO&tso, PSource&sources,
                       std::string&outputFilePrefix,
                       std::string&tsoFileOrDirectoryName,
                       int&detectionDelay,
                       double&minQuality,
                       bool&mcSelected,
                       bool&vcSelected,
                       bool&nfdSelected,
                       bool&tdSelected,
                       bool&ecSelected,
                       std::string&epanetInputFilename,
                       bool&pkSelected,
                       bool&pdSelected,
                       bool&peSelected,
                       std::string&taiInputFilename,
                       std::string&tsoPattern,
                       std::string&sensorInputFilename,
                       std::list<int>&sensorLocations);

void readTsoInfo(int& numNodes,
                 PNodeInfo&nodes, PLinkInfo&links,
                 PNetInfo&net, PTSO&tso, PSource&sources,
                 std::string&outputFilePrefix,
                 std::string&tsoFileOrDirectoryName,
                 std::string&tsoPattern,
                 std::string&nodeMapFileName,
                 std::string&scenarioMapFileName,
                 std::ofstream&nodeMapFile,
                 std::ofstream&scenarioMapFile,
                 std::vector<std::string>&nodeIndexToIDMap,
                 std::map<std::string, int>&nodeIDToIndexMap);

void initStandaloneTso2Impact(
   int& numNodes, clock_t&start, clock_t& stop,
   PNodeInfo&nodes, PLinkInfo&links,
   PNetInfo&net, PTSO&tso, PSource&sources,
   std::string&outputFilePrefix,
   std::string&tsoFileOrDirectoryName,
   int&detectionDelay,
   double&minQuality,
   bool&mcSelected,
   bool&vcSelected,
   bool&nfdSelected,
   bool&tdSelected,
   bool&ecSelected,
   std::string&epanetInputFilename,
   bool&pkSelected,
   bool&pdSelected,
   bool&peSelected,
   std::string&taiInputFilename,
   PMem& assessMem,
   std::string&tsoPattern,
   std::string&sensorInputFilename,
   std::list<int>&sensorLocations,
   std::string&nodeMapFileName,
   std::string&scenarioMapFileName,
   std::ofstream&nodeMapFile,
   std::ofstream&scenarioMapFile,
   std::vector<ObjectiveBase*>&theObjectives,
   std::vector<std::string>&nodeIndexToIDMap,
   std::map<std::string, int>&nodeIDToIndexMap,
   VecTrie<int, vector<int> >& scenarioAggrTrie);

void initTso2ImpactAfterTsoRead(
   int& numNodes, clock_t&start, clock_t& stop,
   PNodeInfo&nodes, PLinkInfo&links,
   PNetInfo&net, PTSO&tso, PSource&sources,
   std::string&outputFilePrefix,
   std::string&tsoFileOrDirectoryName,
   int&detectionDelay,
   double&minQuality,
   bool&mcSelected,
   bool&vcSelected,
   bool&nfdSelected,
   bool&tdSelected,
   bool&ecSelected,
   std::string&epanetInputFilename,
   bool&pkSelected,
   bool&pdSelected,
   bool&peSelected,
   std::string&taiInputFilename,
   PMem& assessMem,
   std::string&tsoPattern,
   std::string&sensorInputFilename,
   std::list<int>&sensorLocations,
   std::string&nodeMapFileName,
   std::string&scenarioMapFileName,
   std::ofstream&nodeMapFile,
   std::ofstream&scenarioMapFile,
   std::vector<ObjectiveBase*>&theObjectives,
   std::vector<std::string>&nodeIndexToIDMap,
   std::map<std::string, int>&nodeIDToIndexMap,
   VecTrie<int, vector<int> >& scenarioAggrTrie);


void finalizeTheObjectives(std::vector<ObjectiveBase*>&theObjectives);

#endif
