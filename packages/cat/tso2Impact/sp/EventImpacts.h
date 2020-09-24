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
 * \file EventImpacts.h
 *
 */

#ifndef tevasp_EventImpacts_h
#define tevasp_EventImpacts_h

#include <list>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <utilib/BitArray.h>

#define FATALERROR(s) {std::cout << __FILE__ << ":" << __LINE__ << " " << s << std::endl; exit(1);}

typedef struct _slocData
{
   int slID;            // super location ID
   int origID;          // ID of earliest witness in super location
   double impact;       // impact at earliest witness
} slocData;

/// Some impact files are too large to read into memory all at
/// once.  This object can hold just some of the scenarios in the
/// file, and replace them with the following scenarios when they
/// are needed.  It's efficient if the code using it reads the
/// impact file in order from the first scenario (event) to
/// the last.  It's not recommended for random access.

class EventImpacts
{
public:
   /// constructor

   EventImpacts() :
         fname("unset"), delay(0),
         fl(NULL), nodeIdMap(NULL),
         readAll(false), readLimit(10000),
         createImpactMap(true),
         createWitnessTimeMap(false),
         createWitnessTimeList(false), createArrivalSequences(false),
         numEvents(0), fromEvent(-1), toEvent(-1), nextEventIndex(0) {}

   /// global information about impact file
   int numEvents;
   int numNodes;
   int numDelays;
   int delayIndex;
   std::vector<int> eventSize;   // number of impacts for event

   /// which events are in core
   volatile int fromEvent;
   volatile int toEvent;

   /// Set the maximum number of impacts to be read into memory at
   /// one time.  The default is 10,000.  If an event has more than
   /// the maximum number, the entire event will be read in to memory.
   void SetMaximumNumberOfImpacts(int num)
   { readLimit = num; readAll = false;}

   /// Call this to read all impacts into memory rather than reading the
   /// impact file in in chunks (the default).
   void SetReadAllImpacts()
   {readAll = true;}

   /// Create or do not create the impact map when the impact file is read.
   /// Default is to create the impact map.
   void SetCreateImpactMap(bool calc)
   {createImpactMap = calc;}

   /// Create or do not create the nodeID/witness time list when the
   /// impact file is read.   Default is not to create the list.
   void SetCreateWitnessTimeList(bool calc)
   {createWitnessTimeList = calc;}

   /// Create or do not create the nodeID/witness time map when the
   /// impact file is read.   Default is not to create the map.
   void SetCreateWitnessTimeMap(bool calc)
   {createWitnessTimeMap = calc;}

   /// Create or do not create the nodeID/witness time vector when the
   /// impact file is read.   Default is not to create the vector.
   void SetCreateArrivalSequences(bool calc)
   {createArrivalSequences = calc;}

   /// Set the name of the impact file
   void SetImpactFileName(std::string nm)
   {fname = nm;}

   /// Set the array of feasible locations.
   void SetFeasibleLocations(utilib::BitArray &f)
   {fl = &f;}

   /// Set the map from file IDs to internal IDs
   void SetNodeIDMap(std::map<int, int> &idMap)
   {nodeIdMap = &idMap;}

   /// Set the delay value
   void SetDelay(int d)
   {delay = d;}

   /// Over all scenarios, the largest difference between the undetected
   /// impact and the other impacts (at feasible locations)
   double GetBiggestDetectionDifference();

   /// Get the impact for the specified event (events range from 0 to
   /// numEvents-1), and location (using internal location IDs)
   double get_impact(int event, int loc) ;

   /// find the impact for the given event and location in the list
   /// of pairs 
   double find_impact(int event, int loc);

   /// Get the map of node IDs to impacts for the specified event
   std::vector<std::pair<int, double> > &get_impact_map(int event) ;

   /// Get the list of nodeID / witness time pairs for an event
   std::list<std::pair<int, int> > &get_witness_times(int event) ;

   /// Get the witness time for an event and location (requires witness time map)
   int get_witness_time(int event, int loc) ;

   /// Get the vector of nodeID / witness time pairs for an event
   std::vector<std::pair<int, double> > &get_arrival_sequences(int event) ;

   /// To manage memory better, a method to free impacts
   void free_impacts()
   {impacts.clear();}

   /// To manage memory better, a method to free witness time lists
   void free_witness_time_lists()
   {wtimes.clear();}

   /// To manage memory better, a method to free witness time maps
   void free_witness_time_maps()
   {wtimesMap.clear();}

   /// To manage memory better, a method to free arrival sequences
   void free_arrival_sequences()
   {arrival_sequences.clear();}

   /// Test methods
   int testReadEvents(int from, int to);
   int testDisplayEventsInCore();
   void showArrivalSequences(int i);
   void showWtimes(int i);
   void showImpacts(int i);

private:

   /// Name of impact file
   std::string fname;

   /// delay to read (0 is the first delay)
   int delay;

   /// feasible locations
   utilib::BitArray *fl;

   /// map from impact file node IDs to internal node IDs
   std::map<int, int> *nodeIdMap;

   /// Read all impacts into memory, or limit the number of impacts
   /// read in.  If "readAll" is false, "readLimit" is the maximum
   /// number of impacts to read.
   /// read_impacts() will read in entire scenarios, up to that maximum.
   /// If there is a scenario that exceeds that number of impacts, it
   /// will still read the entire scenario.
   bool readAll;
   int readLimit;

   /// Include creation of map from nodeIDs to impact values
   bool createImpactMap;

   /// Include creation of witness time list for each event
   bool createWitnessTimeList;

   /// Include creation of witness time map for lookup by event/nodeID
   bool createWitnessTimeMap;

   /// Include creation of arrival_sequences (wtimes in a vector instead
   /// of a list, time is double instead of int, used only by evalsensor
   /// when sensor categories are provided)
   bool createArrivalSequences;

   /// Location after event ID in first line of first event
   std::streampos firstEventPos;

   /// ID of first event in impact file
   int firstEventID;

   /// Location after event ID in first line of next event to be read.
   std::streampos nextEventPos;

   /// ID of next event in impact file, if it's -1, there are no more
   int nextEventID;

   /// Index of next event in impact file, if it's -1, there are no more
   int nextEventIndex;

   /// The impact values.
   /// impacts[i] is a vector of pairs of the impacts for event 
   /// "fromEvent + i"
   std::vector<std::vector<std::pair<int,double> > > impacts;

   /// The witness time values.
   /// wtimes[i] is a list of node/time pairs for event "fromEvent + i"
   std::vector<std::list<std::pair<int, int> > > wtimes;

   /// The witness time values.
   /// arrival_sequences[i] is a vector of node/time pairs for event "fromEvent + i"
   std::vector<std::vector<std::pair<int, double> > > arrival_sequences;

   /// The witness time values in a map.
   /// wtimesMap[i] is a map from nodeID to witness time for event "fromEvent + i"
   std::vector<std::map<int, int> > wtimesMap;

   /// Read a line in an impact file
   int getImpactFileLine(std::ifstream &ifs, int &nodeID, int &timeStep, double &impact);

   /// return whether nodeID found in impact file is a feasible node
   bool feasibleNode(int nodeID);

   /// return internal index corresponding to nodeID in impact file
   int internalNodeID(int nodeID);

   /// Set the nextEventPos and nextEventID fields to the given event
   int setNextEventPos(int eventIndex);

   /// Replace the impacts in core with the impacts for the range of
   /// scenarios.  This assumes we're reading scenarios in order from
   /// beginning to end, otherwise it's not efficient.
   int replace_events(int readFrom, int readTo);

   /// Read the impact file header and set some values, including the
   /// stream position to the start of the first event.
   int read_header();

   /// A subset of events are in memory.  This call replaces them
   /// with the specified event, plus events following it in the
   /// impact file, until the "readLimit" is reached.  It will always
   /// read in at least one event, event if that event has more
   /// than "readLimit" impacts.
   int getEventIntoCore(int event);
};
#endif
