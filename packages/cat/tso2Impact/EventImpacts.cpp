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
  file EventImpacts.h
 *
 */

#ifndef tevasp_EventImpacts_cpp
#define tevasp_EventImpacts_cpp

#include <sp/EventImpacts.h>
#include <cstdio>

/// Read impact file header, count the number of impacts in each event.

int EventImpacts::read_header()
{
   int delayVal, nodeID, timeStep, eventID;
   double impact;
   char buf[256];

   if (fname == "unset")
   {
      FATALERROR("impact file not set")
   }

   std::ifstream ifs(fname.c_str(), std::ios::binary);
   if (!ifs)
   {
      FATALERROR("can't open impact file")
   }
   ifs.getline(buf, 256);
   int nvals = sscanf(buf, "%d %d", &numNodes, &numEvents);
   if (nvals < 1 || nvals > 2)
   {
      FATALERROR("bad impact file")
   }
   else if (nvals == 1)
   {
      numEvents = 0;
   }

   ifs >> numDelays;

   for (int i = 0; i < numDelays; i++)
   {
      ifs >> delayVal;
      if (delayVal == delay)
      {
         delayIndex = i;
      }
      if (delayIndex == -1)
      {
         FATALERROR("delay value not found in impact file")
      }
   }
   // Save position of start of first event
   if (!(ifs >> eventID))
   {
      numEvents = 0;
      nextEventIndex = -1;
      return 0;
   }
   firstEventID = nextEventID = eventID;
   nextEventIndex = 0;

   firstEventPos = nextEventPos = ifs.tellg();

   // Read through whole file line by line, to count number of events
   // (which may not have been in header) and count impacts per event.

   eventSize.clear();
   bool done = false;

   while (!done)
   {
      eventSize.push_back(0);
      int prev = eventID;

      while (eventID == prev)
      {
         getImpactFileLine(ifs, nodeID, timeStep, impact);
         if (feasibleNode(nodeID))
         {
            eventSize.back()++;
         }
         if (!(ifs >> eventID))
         {
            done = true;
            break;
         }
      }
   }

   numEvents = eventSize.size();

   ifs.close();
   return 0;
}

// Replace the events currently in core with a new range - this
// class is optimized for the case where we read contiguous chunks
// of the impact file in order from beginning to end.

int EventImpacts::replace_events(int readFrom, int readTo)
{
   int nodeID, timeStep;
   double impact;

   if (numEvents == 0)
   {
      FATALERROR("no events");
   }
   if ((readFrom < 0) || (readTo >= numEvents) || (readTo < readFrom))
   {
      FATALERROR("bad event numbers");
   }
   if ((readFrom >= fromEvent) && (readTo <= toEvent))
   {
      return 0;   // the requested events are already in memory
   }
   if (readFrom != nextEventIndex)
   {
      setNextEventPos(readFrom);  // expensive
   }

   //std::cout << "Replace events " << readFrom << " through " << readTo << std::endl;

   std::ifstream ifs(fname.c_str());
   if (!ifs)
   {
      FATALERROR("can't open impact file")
   }

   int totalEvents = readTo - readFrom + 1;

   impacts.clear();
   impacts.resize(totalEvents);

   std::vector<std::vector<std::pair<int, double> > >::iterator currImpact = impacts.begin();
   std::map<int, double> scenImpacts;
   // Different users of this class need different structures created
   std::vector<std::list<std::pair<int, int> > >::iterator currTime;
   std::vector<std::map<int, int> >::iterator currTimeMap;
   std::vector<std::vector<std::pair<int, double> > >::iterator currArr;

   if (createWitnessTimeList)
   {
      wtimes.clear();
      wtimes.resize(totalEvents);
      currTime = wtimes.begin();
   }

   if (createWitnessTimeMap)
   {
      wtimesMap.clear();
      wtimesMap.resize(totalEvents);
      currTimeMap = wtimesMap.begin();
   }

   if (createArrivalSequences)
   {
      arrival_sequences.clear();
      arrival_sequences.resize(totalEvents);
      currArr = arrival_sequences.begin();
   }

   std::list<std::pair<int, int > > wtimeList;
   std::map<int, int > wtimeMap;
   std::vector<std::pair<int, double > > wtimeVector;

   int EID = nextEventID;

   ifs.seekg(nextEventPos);

   for (int eventIdx = 0; eventIdx < totalEvents; eventIdx++)
   {
      int prevEID = EID;
      scenImpacts.clear();

      while (EID == prevEID)
      {
         getImpactFileLine(ifs, nodeID, timeStep, impact);
         if (feasibleNode(nodeID))
         {

            if (nodeID != -1)
            {
               nodeID = internalNodeID(nodeID);
            }
            if (createWitnessTimeList)
               (*currTime).push_back(std::make_pair(nodeID, timeStep));

            if (createWitnessTimeMap)
               (*currTimeMap)[nodeID] = timeStep;

            if (createArrivalSequences)
               (*currArr).push_back(std::make_pair(nodeID, timeStep));
            scenImpacts[nodeID] = impact;
         }
         if (!(ifs >> EID))
         {
            break;  // end of file
         }
      }
      currImpact->reserve(eventSize[eventIdx]);
      std::map<int, double>::iterator impIter=scenImpacts.begin();
      while(impIter != scenImpacts.end()) {
         currImpact->push_back(std::make_pair((*impIter).first,(*impIter).second));
         impIter++;
      }
      currImpact++;
      if (createWitnessTimeList) currTime++;
      if (createWitnessTimeMap) currTimeMap++;
      if (createArrivalSequences) currArr++;
   }

   nextEventIndex = readTo + 1;

   if (nextEventIndex == numEvents)
   {
      nextEventIndex = -1;  // flag we're at end of file
   }
   else
   {
      nextEventID = EID;
      nextEventPos = ifs.tellg();
   }

   fromEvent = readFrom;
   toEvent = readTo;

   ifs.close();
}

// Set nextEventPos and nextEventID to the i'th event in the
// file, where i is eventIndex.

int EventImpacts::setNextEventPos(int eventIndex)
{
   int nodeID, timeStep;
   double impact;

   if ((eventIndex < 0) || (eventIndex >= numEvents))
   {
      FATALERROR("Invalid event index")
   }

   if (eventIndex == nextEventIndex)
      return 0;

   if (eventIndex == 0)
   {
      nextEventPos = firstEventPos;
      nextEventID = firstEventID;
      nextEventIndex = 0;
      return 0;
   }

   std::ifstream ifs(fname.c_str());
   if (!ifs)
   {
      FATALERROR("can't open impact file")
   }

   ifs.seekg(firstEventPos);
   int index = -1;
   int EID = firstEventID;

   while (true)
   {
      int prevEID = EID;
      if (++index == eventIndex)
         break;

      while (EID == prevEID)
      {
         getImpactFileLine(ifs, nodeID, timeStep, impact);
         if (!(ifs >> EID))    // end of file
         {
            FATALERROR("error that shouldn't happen")
         }
      }
   }

   nextEventPos = ifs.tellg();
   nextEventID = EID;
   nextEventIndex = eventIndex;
   ifs.close();

   return 0;
}

int EventImpacts::internalNodeID(int nodeID)
{
   if ((nodeID == -1) || (nodeIdMap == NULL))
      return nodeID;
   else
      return (*nodeIdMap)[nodeID];
}

bool EventImpacts::feasibleNode(int nodeID)
{
   if ((nodeID == -1) || (fl == NULL))
      return true;
   else
      return ((*fl)[nodeID-1] == 1);
}

int EventImpacts::getImpactFileLine(std::ifstream &ifs, int &nodeID,
                                    int &timeStep, double &impact)
{
   double imp;

   if (numDelays == 1)
   {
      ifs >> nodeID >> timeStep >> impact;
   }
   else
   {
      ifs >> nodeID >> timeStep;

      for (int i = 0; i < delayIndex; i++)
      {
         ifs >> imp;
      }
      ifs >> impact;

      for (int i = delayIndex + 1; i < numDelays; i++)
      {
         ifs >> imp;
      }
   }
   return 0;
}

int EventImpacts::getEventIntoCore(int event)
{
   if (readAll)
   {
      replace_events(event, numEvents - 1);
   }
   else
   {
      int impactCount = 0;
      int eventCount = 0;

      for (int ev = event; ev < numEvents; ev++)
      {
         if ((eventSize[ev] + impactCount) <= readLimit)
         {
            eventCount++;
            impactCount += eventSize[ev];
         }
         else
         {
            break;
         }
      }

      if (eventCount == 0)
      {
         // If number of impacts for event exceeds the readLimit, we
         // break the rule and read in all of those impacts.  It doesn't
         // make sense to read in part of an event.
         eventCount = 1;
      }

      replace_events(event, event + eventCount - 1);
   }
   return 0;
}


double EventImpacts::GetBiggestDetectionDifference()
{
   double max_difference = 0.0;
   double lowest_impact=0, no_detection_impact;
   bool lowest_impact_set;
   int nodeID, timeStep;
   double impact;

   if (numEvents == 0)
   {
      read_header();
      if (numEvents == 0)
         FATALERROR("no events in impact file");
   }

   if ((fromEvent == 0) && (toEvent == numEvents - 1))
   {
      // all events are in memory
      for (int ev = 0; ev < numEvents; ev++)
      {
         std::vector<std::pair<int, double> >::iterator curr = impacts[ev].begin();
         std::vector<std::pair<int, double> >::iterator currlast = impacts[ev].end();
         lowest_impact_set = false;
         while (curr != currlast)
         {
            nodeID = curr->first;
            impact = curr->second;
            if (feasibleNode(nodeID))
            {
               if (nodeID == -1)
               {
                  no_detection_impact = impact;
               }
               else if ((impact < lowest_impact) || !lowest_impact_set)
               {
                  lowest_impact = impact;
                  lowest_impact_set = true;
               }
            }
            curr++;
         }
         if (lowest_impact_set && ((no_detection_impact - lowest_impact) > max_difference))
            max_difference = no_detection_impact - lowest_impact;
      }
   }
   else
   {
      // read each impact in impact file
      std::ifstream ifs(fname.c_str());

      // position after event ID of first line of first event
      ifs.seekg(firstEventPos);
      int EID = firstEventID;

      for (int eventIdx = 0; eventIdx < numEvents; eventIdx++)
      {
         int prevEID = EID;
         lowest_impact_set = false;

         while (EID == prevEID)
         {
            getImpactFileLine(ifs, nodeID, timeStep, impact);
            if (feasibleNode(nodeID))
            {
               if (nodeID == -1)
               {
                  no_detection_impact = impact;
               }
               else if ((impact < lowest_impact) || !lowest_impact_set)
               {
                  lowest_impact = impact;
                  lowest_impact_set = true;
               }
            }
            if (!(ifs >> EID))
            {
               break;  // end of file
            }
         }
         if (lowest_impact_set && ((no_detection_impact - lowest_impact) > max_difference))
            max_difference = no_detection_impact - lowest_impact;
      }
      ifs.close();
   }

   return max_difference;
}

std::vector<std::pair<int, double> > &EventImpacts::get_arrival_sequences(int event)
{
   if (!createArrivalSequences)
   {
      FATALERROR("arrival sequences not requested")
   }
   if (numEvents == 0)
   {
      read_header();
      if (numEvents == 0)
         FATALERROR("no events in impact file");
   }
   if ((event < 0) || (event >= numEvents))
   {
      FATALERROR("bad event id")
   }
   if ((event < fromEvent) || (event > toEvent))
   {
      getEventIntoCore(event);
   }

   return arrival_sequences[event - fromEvent];
}

std::list<std::pair<int, int> > &EventImpacts::get_witness_times(int event)
{
   if (!createWitnessTimeList)
   {
      FATALERROR("SetCreateWitnessTimeList(true) not called")
   }
   if (numEvents == 0)
   {
      read_header();
      if (numEvents == 0)
         FATALERROR("no events in impact file");
   }
   if ((event < 0) || (event >= numEvents))
   {
      FATALERROR("bad event id")
   }
   if ((event < fromEvent) || (event > toEvent))
   {
      getEventIntoCore(event);
   }

   return wtimes[event - fromEvent];
}

std::vector<std::pair<int, double> > &EventImpacts::get_impact_map(int event)
{
   if (!createImpactMap)
   {
      FATALERROR("SetCreateImpactMap(false) was called")
   }
   if (numEvents == 0)
   {
      read_header();
      if (numEvents == 0)
         FATALERROR("no events in impact file");
   }
   if ((event < 0) || (event >= numEvents))
   {
      FATALERROR("bad event id")
   }
   if ((event < fromEvent) || (event > toEvent))
   {
      getEventIntoCore(event);
   }

   return impacts[event - fromEvent];
}


double EventImpacts::get_impact(int event, int loc)
{
   if (!createImpactMap)
   {
      FATALERROR("SetCreateImpactMap(false) was called")
   }
   if (numEvents == 0)
   {
      read_header();
      if (numEvents == 0)
         FATALERROR("no events in impact file");
   }
   if ((event < 0) || ((numEvents > 0) && (event >= numEvents)))
   {
      FATALERROR("bad event id")
   }
   if ((event < fromEvent) || (event > toEvent))
   {
      // fromEvent changed in call to getEventIntoCore
      getEventIntoCore(event);
   }

   int idx = event - fromEvent;
   double impact = find_impact(idx,loc);
   return impact;
}

double EventImpacts::find_impact(int event, int loc)
{
   std::vector<std::pair<int,double> >& ei=impacts[event];
   int ub,lb;
   ub=ei.size();
   lb=0;
   while(lb < ub)
   {
      int mid=(ub+lb)/2;
      std::pair<int,double>& elem=ei[mid];
      if(elem.first==loc) return elem.second;
      if(loc < elem.first) {
         ub=mid;
      } else {
         lb=mid;
      }
   }
   FATALERROR("bad location");
}

int EventImpacts::get_witness_time(int event, int loc)
{
   if (!createWitnessTimeMap)
   {
      FATALERROR("SetCreateWitnessTimeMap(true) not called")
   }
   if (numEvents == 0)
   {
      read_header();
      if (numEvents == 0)
         FATALERROR("no events in impact file");
   }
   if ((event < 0) || (event >= numEvents))
   {
      FATALERROR("bad event id")
   }
   if ((event < fromEvent) || (event > toEvent))
   {
      // fromEvent changed in call to getEventIntoCore
      getEventIntoCore(event);
   }

   int idx = event - fromEvent;

   std::map<int, int>::iterator curr = wtimesMap[idx].find(loc);
   if (curr == wtimesMap[idx].end())
   {
      FATALERROR("bad location")
   }
   int witnessTime = curr->second;

   return witnessTime;
}
#endif
