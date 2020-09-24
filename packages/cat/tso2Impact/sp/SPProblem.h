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

/**
 * \file SPProblem.h
 *
 */

#ifndef tevasp_SPProblem_h
#define tevasp_SPProblem_h

#include <list>
#include <map>
#include <string>
#include <iostream>
#include <utilib/BitArray.h>
#include <utilib/BasicArray.h>
#include <sp/EventImpacts.h>

///
class SPGoalInfo
{
public:

   /// a magic number representing the "null" bound
   /// for an objective goal/measure combination.
   static const double OBJECTIVE_GOAL_BOUND;

   /// Compute the superlocations for this impact.
   /// Note: superlocations are not meaningful for
   /// the "ns" and "cost" goals.
   void compute_superlocations(std::map<utilib::BasicArray<int>, int>& membersets,
                               utilib::BitArray& feasible_locations,
                               utilib::BitArray& fixed_locations,
                               bool output_impact);


   /// Force the EventImpacts object to read in the first events in
   /// the impact file.  It will replace these with subsequent events
   /// when they are required.
   int read_impacts()
   {
      get_impact(0, -1);
      return 0;
   }

   /// Get the impact for the specified event and location
   double get_impact(int event, int loc)
   { return events.get_impact(event, loc); }

   /// Get the witness time for the specified event and location
   /// Requires EventImpacts::SetCreateWitnessTimeMap(true)
   int get_witness_time(int event, int loc)
   { return events.get_witness_time(event, loc); }

   /// Get the list of nodeID/witness times for the specified event
   /// Requires EventImpacts::SetCreateWitnessTimeList(true)
   std::list<std::pair<int, int> >& get_witness_times(int event)
   { return events.get_witness_times(event); }

   /// Get the vector of nodeID/witness times for the specified event
   /// Requires EventImpacts::SetCreateArrivalSequences(true)
   std::vector<std::pair<int, double> >& get_arrival_sequences(int event)
   { return events.get_arrival_sequences(event); }

   /// The number of events in the impact file
   int num_events() const
   {return events.numEvents;}

   /// The number of events currently in memory
   int num_events_in_core() const
   {
      if (events.fromEvent >= 0) return (events.toEvent - events.fromEvent + 1);
      else return 0;
   }

   // The events object calculates the number of impacts in every
   // event when it reads the header.  This number is available, even
   // if not all of the events are in memory.
   int num_impacts(int event) const
   { return events.eventSize[event]; }

   std::vector<std::pair<int, double> > &get_impact_map(int event)
   { return events.get_impact_map(event); }

   /// The name of the impact
   std::string goal;

   /// The file of impacts that are associated with this goal
   std::string impact_file;

   /// The object that reads in all or part of the impact file
   EventImpacts events;

   /// The probabilities of each event
   utilib::BasicArray<double> probabilities;

   /// The superlocations ids
   utilib::BasicArray<std::list<int> > super_locs;

   /// An absolute threshold used to compute super locations.
   double threshold;

   /// The ratio of the best super-location objective value to
   /// the value used by the super-location
   double aggregation_ratio;

   /// The final ratio of the best super-location objective value to
   /// the value used by the super-location observed after aggregation
   double final_aggregation_ratio;

   /// An alternative specification for aggregation (super locations).
   /// This sets the threshold for a specific goal as a percentage of
   /// the maximum impact difference (over all events) between
   /// detection and non-detection.  This takes priority over
   /// thresholds if both are specified and this is a valid percentage > 0.
   double aggregation_percentage;

   /// If we are aggregating (with percentage or absolute threshold),
   /// and this is true, then for an individual event, if the global
   /// threshold is large enough to bring all locations into a single
   /// superlocation, force the dummy into a second location.  This
   /// means that for "inconsequential" events, we still distinguish
   /// between detection at all (with a value equal to the worst for
   /// detection) and no detection.
   bool distinguish_detection;

   /// The measures that are associated with this goal
   std::list<std::string> measure;

   /// If true, then this goal is in the objective
   /// Note: only the first measure is currently used in the
   /// objective.
   bool objective_flag;

   /// The bounds on the goal measures. if the measure is
   /// the objective, the goal
   std::list<double> bound;

   /// The worst node in each superlocation set
   /// Note if output_impact is "true", we overload this array to
   /// to hold not the worst node, but the node with earliest
   /// impact time - so it's really "sl_first".
   utilib::BasicArray<std::map<int, int> > sl_worst;

   /// Each superlocation has an original location with earliest
   /// witness time for this event.  This map maps witness
   /// times to lists of structures containing a superlocation ID,
   /// earliest witness ID, and earliest witness impact.
   ///
   /// Only needed if we're writing an impact file of super locations
   /// (output_impact == true).
   utilib::BasicArray<std::map<int, std::list<slocData> > > sl_wtime;
};


///
class SPProblem
{
public:

   ///
   SPProblem()
         : num_nodes(0), delay(0), num_superlocations(0), readLimit(10000), readall(true),
         conserve_memory(true), gamma(0.05), output_impact(false) {}

   /// Read the SPProblem configuration file
   // TNT: Added flag to optionally load impact data.  It is not always needed
   // to be loaded in this format.
   void read_config_file(std::istream& istr, bool loadImpacts = true, bool loadWtime = false, bool loadWtimeMap = false);

   /// Write an AMPL *.dat file used with GeneralSP.mod
   void write_ampl_data(std::ostream& os, bool imperfect);

   /// Write a data file used with GeneralSP.mod and ufl Lagrangian solver
   void write_lagrangian_data(bool imperfect, std::string& fname);

   /// Write an impact file for a specific goal.
   void write_impact_file(std::ostream& os, int goal_index);

   /// Write an impact file with superlocations replacing original locations
   void write_aggregated_impacts(std::ostream& osIm, int goal_index);

   /// Write a new configuration file reflecting location aggregation
   void write_aggregated_config_file(std::ostream& ostr, std::vector<std::string> &impactFiles);

   /// Compute the super locations
   void compute_superlocations();

   /// Set a limit on the number of impacts in memory at one time.
   void SetMaximumNumberOfImpacts(int max)
   { readLimit = max; }

   /// If set to true, read entire impact file into memory, if set
   /// to false, read impacts in chunks.
   void SetReadAllImpacts(bool val = true)
   { readall = val; }

   /// The goal data
   utilib::BasicArray<SPGoalInfo> goals;

   /// The locations where sensors have been predeteremined
   utilib::BasicArray<int> fixed_placements;

   /// The locations where sensors cannot be placed
   utilib::BasicArray<int> invalid_placements;

   /// Feasible locations
   utilib::BitArray feasible_location;

   /// Fixed locations
   utilib::BitArray fixed_location;

   /// The costs of placement
   std::map<int, double> costs;

   /// The goals that are included in the objective.
   std::list<int> objectives;

   /// The superlocation sets used in this model
   std::map<utilib::BasicArray<int>, int> membersets;

   /// Maps from the 'internal' representation used by impact files to
   /// the reduced representation after invalid locations are
   /// eliminated.
   std::map<int, int> reduced_repn_map;

   /// The original node ID from the impact file, indexed by our
   /// contiguous node IDs after invalid locations are eliminated.
   std::vector<int> original_node_id;

   /// The number of nodes in the network
   int num_nodes;

   /// The maximum number of impacts to read into memory at one time.
   int readLimit;

   /// If true, then read entire file of impacts.  Otherwise, read at most
   /// readLimit at a time.
   bool readall;

   /// The number of superlocations in the network, not including
   /// the dummy location.  We use superlocation ID 0 for the dummy.
   int num_superlocations;

   ///
   int delay;

   ///
   bool conserve_memory;

   ///
   double gamma;

   /// Output an impact file containing superlocations
   bool output_impact;
};

#endif
