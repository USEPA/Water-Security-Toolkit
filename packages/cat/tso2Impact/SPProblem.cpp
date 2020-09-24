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
 * \file SPProblem.cpp
 *
 * Methods for the SPProblem class.
 */
#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include <sp/SPProblem.h>
#include <sp/impacts.h>
#include <fstream>
#include <algorithm>
#include <utilib/BasicArray.h>
#include <utilib/sort.h>

using namespace std;

// Turn on to print out Superlocations
static bool debugSL = false;
#define DEBUGSL0 \
  if (debugSL) {std::cout << "SL ID, impact range, real location IDs" << std::endl;}
#define DEBUGSL1 \
  if (debugSL) {std::cout << "GOAL " << curr->goal << endl;}
#define DEBUGSL2 \
  if (debugSL) {std::cout << "Event " << eventNum+1 << std::endl;}
#define DEBUGSL3 \
  if (debugSL){ \
   std::cout << ndx << " (" << values[ord[prev_feasible]]; \
   std::cout << " - " << values[ord[last]] << ") "; \
   for (int origID=0; origID < locs.size(); origID++){ \
    std::cout << locs[origID] << " "; \
   } \
  std::cout << std::endl; \
  }

const double SPGoalInfo::OBJECTIVE_GOAL_BOUND = -99999.0;
// TNT: Added flag to optionally load impact data.  It is not always needed
// to be loaded in this format.
void SPProblem::read_config_file(istream& istr,
                                 bool loadImpacts, bool loadWtime, bool loadWtimeMap)
{
   int ngoals;
   string fname;
   istr >> num_nodes >> delay;
   feasible_location.resize(num_nodes);
   feasible_location.set();
   fixed_location.resize(num_nodes);
   fixed_location.reset();
   //
   // Read goals
   //
   {
      istr >> ngoals;
      goals.resize(ngoals);
      int i = 0;
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         //
         // Read config info
         //
         istr >> curr->goal >> curr->impact_file >> curr->threshold >> curr->aggregation_percentage
         >> curr->aggregation_ratio >> curr->distinguish_detection;
         //
         // Error checking
         //
         if (curr->threshold > 0.0 && curr->aggregation_percentage > 0.0)
            // CAP: is there a standard way to issue warnings in teva-sp?
            cerr << "#Warning: both threshold and percentage aggregation set for goal " << curr->goal << ". Using percentage aggregation.\n";
         if (curr->aggregation_percentage > 1.0)
         {
            cerr << "#Warning: aggregation percentage greater than 1.0.  Using 1.0 = 100 percent\n";
            curr->aggregation_percentage = 1.0;
         }
         if (curr->aggregation_percentage < 0.0)
         {
            cerr << "#Warning: aggregation percentage less than 0.  Using 0.\n";
            curr->aggregation_percentage = 1.0;
         }
         if (curr->aggregation_ratio > 1.0)
         {
            cerr << "#Warning: aggregation ratio greater than 1.0.  Using 1.0.\n";
            curr->aggregation_ratio = 1.0;
         }
         if (curr->aggregation_ratio < 0.0)
         {
            cerr << "#Warning: aggregation ratio less than 0.0.  Using 0.0.\n";
            curr->aggregation_ratio = 0.0;
         }
         //
         // Read measures
         //
         int tmp;
         istr >> tmp;
         for (int j = 0; j < tmp; j++)
         {
            string measure;
            istr >> measure;
            curr->measure.push_back(measure);
         }
         //
         // Read objective flag
         //
         char c;
         istr >> c;
         if (c == 'o')
         {
            objectives.push_back(i);
            curr->objective_flag = true;
         }
         else
            curr->objective_flag = false;
         //
         // Read constraint bound information
         //
         for (int j = 0; j < tmp; j++)
         {
            double bound;
            istr >> bound;
            curr->bound.push_back(bound);
         }
         i++;
         curr++;
      }
   }

   //
   // Read fixed placements
   //
   read_array(istr, fixed_placements);
   //
   // Read invalid placements
   //
   // This clears the invalid_placements array, since all solvers are expected to use the
   // 'feasible_location' array and reduced_repn_map to transform the problem representation!
   //
   read_invalid_locations(istr, invalid_placements, feasible_location, reduced_repn_map);
   num_nodes = feasible_location.nbits();
   invalid_placements.resize(0);
   //
   // Transform the locations used for fixed_placements
   //
   for (size_type i = 0; i < fixed_placements.size(); i++)
   {
      int newID = reduced_repn_map[fixed_placements[i]];
      fixed_placements[i] = newID;
      fixed_location.set(newID - 1);  // may only need this for output_impact
   }
   //
   // Read costs
   //
   read_id_costs(istr, costs);

   if (output_impact)
   {
      // If creating new impact files (with superlocations) we need
      // to be able to map superlocations back to an input location ID.

      std::map<int, int>::iterator curr = reduced_repn_map.begin();
      original_node_id.resize(num_nodes + 1);
      while (curr != reduced_repn_map.end())
      {
         original_node_id[curr->second] = curr->first;
         curr++;
      }
   }
   else
   {
      original_node_id.resize(0);
   }

   //
   // Read impact files, using info about invalid placements to
   // prune the impact matrix reads.
   //
   utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
   utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
   while (curr != end)
   {
      if (curr->impact_file != "none")
      {
         ifstream ifstr(curr->impact_file.c_str());
         if (!ifstr)
            EXCEPTION_MNGR(runtime_error, "Missing impact file: " << curr->impact_file);
         if (loadImpacts)
         {
            if (readall)
               curr->events.SetReadAllImpacts();
            else
               curr->events.SetMaximumNumberOfImpacts(readLimit);

            if (loadWtime)
               curr->events.SetCreateWitnessTimeList(true);
            else
               curr->events.SetCreateWitnessTimeList(false);

            if (loadWtimeMap)
               curr->events.SetCreateWitnessTimeMap(true);
            else
               curr->events.SetCreateWitnessTimeMap(false);

            curr->events.SetCreateArrivalSequences(false);
            curr->events.SetImpactFileName(curr->impact_file.c_str());
            curr->events.SetFeasibleLocations(feasible_location);
            curr->events.SetNodeIDMap(reduced_repn_map);
            curr->events.SetDelay(delay);

            // This may only read in the first chunk of the impact file.
            // The rest will be read in when get_impact() requests an event
            // that is not in core yet.
            curr->read_impacts();
         }

         // if there's a file of event probabilities corresponding to this
         // impact file, read it now.
         std::string probfname = curr->impact_file + ".prob";
         ifstream ifprobstr(probfname.c_str());
         if (ifprobstr)
         {
            std::cout << "Using incident weights defined in the file '" << probfname << "'" << std::endl;
            std::map<int, double> probs;
            read_incident_weights(ifprobstr, probs);
            int num_events = curr->num_events();
            curr->probabilities.resize(num_events);
            for (int j = 1; j <= num_events; j++)
            {
               curr->probabilities[j-1] = probs[j];
            }
         }
      }
      curr++;
   }

   //
   // If we've loaded the all impacts, then we don't need this info any more.  In that case, we're generating
   // output for something like PICO.  For that, we need to keep track of the fact that we have eliminated
   // infeasibilities.  (Actually, this is a limitation of the existing code.  We should ignore the fact
   // that we have eliminated the infeasibilities ... but we'll work on that later.)
   //
   // If not, then we'll need this later to read in the impact data.
   //
   if (loadImpacts && (goals[0].num_events_in_core() == goals[0].num_events()))
   {
      feasible_location.resize(num_nodes);
      feasible_location.set();
   }
}



void SPProblem::compute_superlocations()
{
   utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
   utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
   DEBUGSL0
   while (curr != end)
   {
      DEBUGSL1
      curr->compute_superlocations(membersets, feasible_location, fixed_location, output_impact);
      curr++;
   }
   num_superlocations = membersets.size() - 1;  // not including dummy

   if (output_impact)
   {
      // Create a file listing all original locations in each super location.
      std::map<utilib::BasicArray<int>, int>::iterator mcurr = membersets.begin();
      ofstream auxfile("superlocations_all.txt");
      auxfile << "# For each superlocation ID, a list of all original locations in it" << std::endl;
      auxfile << "# First line: number of superlocations" << std::endl;
      auxfile << "# Remaining lines: Superlocation ID followed by original IDs," << std::endl;
      auxfile << "# in no particular order" << std::endl;
      auxfile << num_superlocations << std::endl;
      while (mcurr != membersets.end())
      {
         utilib::BasicArray<int> origIDs = mcurr->first;
         int slID = mcurr->second;
         if (slID != 0)    // dummy is in 0
         {
            auxfile << slID << " ";
            for (int i = 0; i < origIDs.size(); i++)
            {
               int id = original_node_id[origIDs[i]];
               auxfile << id << " ";
            }
            auxfile << std::endl;
         }
         mcurr++;
      }
      auxfile.close();

      // Done with these
      membersets.erase(membersets.begin(), membersets.end());
      feasible_location.resize(0);
      fixed_placements.resize(0);
      invalid_placements.resize(0);
   }
}

void SPGoalInfo::compute_superlocations(map<utilib::BasicArray<int>, int>& membersets,
                                        utilib::BitArray& feasible_location,
                                        utilib::BitArray& fixed_location,
                                        bool output_impact)
{
   if ((goal == "ns") || (goal == "cost") || (goal == "awd")) return;
   cerr << "# Processing " << goal << "   distinguish " << distinguish_detection << "  threshold " << threshold << endl;
   //
   // The default aggregation ratio is 1.0 (all members of a superlocation have equal values, within a small
   // threshold).  We use this variable to compute the final (instance-specific) worst-case
   // ratio over all supernodes (updated after each superlocation is found/processed).
   //
   final_aggregation_ratio = 1.0;
   //
   // Resize the arrays
   //
   if (output_impact)
   {
      sl_wtime.resize(num_events());
   }
   if (!output_impact)
   {
      super_locs.resize(num_events());
      sl_worst.resize(num_events());
   }
   //
   // Find different super locations for each event
   //
   // Compute the aggregation threshold if necessary.  We're saving space
   // instead of time, so there's some redundant pulling of impact information, etc.
   // If the threshold is also set, we reset using percentages.  We should have
   // already issued a warning in that case.  Recall: a threshold of -1 disables
   // aggregation.

   if (threshold >= 0 && aggregation_percentage > 0 && aggregation_percentage <= 1.0)
   {
      // Find biggest difference between (feasible) detection and no detection

      // If only part of the impact file has been read into memory, then this call
      // reads the entire impact file one line at a time to compute this value.
      double max_difference = events.GetBiggestDetectionDifference();
      threshold = aggregation_percentage * max_difference;
   }

   utilib::BasicArray<list<int> >::iterator scurr = super_locs.begin();
   utilib::BasicArray<map<int, int> >::iterator slcurr = sl_worst.begin();
   utilib::BasicArray<map<int, std::list<slocData> > >::iterator sltimecurr = sl_wtime.begin();

   int eventNum = 0;

   while (eventNum < num_events())
   {
      DEBUGSL2
      double tmp_threshold = threshold;
      //
      // Collect event impacts and locations
      //
      std::vector<std::pair<int, double> > &curr = events.get_impact_map(eventNum);
      utilib::BasicArray<int> nodes(curr.size());
      utilib::BasicArray<double> values(curr.size());
      vector<std::pair<int,double> >::iterator mcurr = curr.begin();
      vector<std::pair<int,double> >::iterator mend  = curr.end();
      std::list<int> fixed_nodes;
      unsigned int nnodes = 0;

      while (mcurr != mend)
      {
         int id = mcurr->first;
         if (!output_impact || (id == -1) || !fixed_location(id - 1))
         {
            nodes[nnodes] = id;
            values[nnodes] = mcurr->second;
            nnodes++;
         }
         else
         {
            // If writing new impact files, each fixed location maps to
            // one new superlocation.  They don't get aggregated.
            fixed_nodes.push_back(id);
         }
         mcurr++;
      }
      if (nnodes < curr.size())
      {
         nodes.resize(nnodes);
         values.resize(nnodes);
      }
      //
      // Order the event values and compute the super locations
      //
      utilib::BasicArray<int> ord;
      utilib::stable_order(ord, values, std::greater<double>());
      utilib::BasicArray<int> locs;
#if 0
      utilib::BasicArray<double> lvalues;
#endif
      double sl_worst, sl_best;
      //
      // Start by adding the 'nonwitnessed' location
      //
      // Many nodes may have the worst case impact; not just the dummy.
      // We now have a sorted order in 'ord', but the index of the dummy node
      // is not guaranteed to be in ord[0].  For example the impact fragment:
      // 3 2 400.5
      // 3 -1 400.5
      // contributes two "400.5"s to the values array.  The utilib stable_order
      // routine will rank these at the top, but it's a toss up whether witness
      // "2" or "-1" gets the zero'th spot.  If "2" got it, then the increment
      // of i below in the line commented "skip dummy" will fail in its purpose.
      // We have to do a final swap to make sure that the dummy node is indeed
      // ranked in ord[0].
      //
      unsigned int i = 0;
      while ((i < ord.size()) && (nodes[ord[i]] != -1))
      {
         i++;
      }
      while (i >= 1)
      {
         int tmp = ord[i];
         ord[i] = ord[i-1];
         ord[i-1] = tmp;
         i--;
      }
      assert(nodes[ord[0]] == -1);

      sl_worst = sl_best = values[ord[0]];
      locs.push_back(nodes[ord[0]]);
#if 0
      lvalues.push_back(values[ord[0]]);
#endif

      // last marks the first element of the superlocation set under construction.
      // This is an index into ord[].  So it marks the rank.
      int last = 0;
      // prev_feasible marks the last element added to the superlocation set
      // under construction (guaranteed within threshold of the first element).
      // Again this is an index into ord[] (hence a rank).
      int prev_feasible = 0;

      //
      // Find next feasible location
      //
      i = 1;        // skip dummy
      while (i <= ord.size())
      {
         //
         // Perform a simple pass through to find super locations.  Add current node to the
         // supernode we're growing as long as it meets the threshold conditions.  If this is
         // the last node, then act as if the threshold is exceeded.
         //
         // If output_impact==TRUE, maintain the dummy for each event,  don't
         // merge other locations in to it.  Superlocation 0 is new dummy.

         if ((i < ord.size()) && (values[ord[last]] < values[ord[i]]) && (nodes[ord[last]] != -1))
            cerr << "ERROR: somehow our items are not sorted! " << last << " " << values[ord[last]] << " " << i << " " << values[ord[i]] << "\n";

         if ((i < ord.size()) &&
               (tmp_threshold >= 0.0) &&
               (!output_impact || (i > 1)) &&
               (!distinguish_detection || (distinguish_detection && ((last > 0) || (values[ord[i]] == values[ord[0]]))))  &&
               ((values[ord[last]] - values[ord[i]]) <= tmp_threshold) &&
               ((values[ord[last]] == 0) || ((values[ord[i]] / values[ord[last]]) >= aggregation_ratio))
            )
         {
            //cerr << "# HERE O" << endl;
            //
            // Add to the current super location set
            //
            /* WEH - TODO if (values[ord[i]] > sl_worst)
               sl_worst = values[ord[i]]; */
            if (values[ord[i]] < sl_best)
               sl_best = values[ord[i]];
            locs.push_back(nodes[ord[i]]);
#if 0
            lvalues.push_back(values[ord[i]]);
#endif

            prev_feasible = i;
         }
         else
         {
            //cerr << "# HERE X" << endl;
            //
            // Look for the super location set in the current set of membersets
            //
#if 0
            cerr << "# SuperLoc IDs:    " << locs << endl;
            cerr << "# SuperLoc Values: " << lvalues << endl;
            cerr << "# Ratio:           ";
            if (max(lvalues) == 0.0) cerr << "1.0" << endl;
            else cerr << min(lvalues) / max(lvalues) << endl;
            cerr << "#" << endl;
#endif
            // Sort the nodes in this superlocation, smallest id to largest.
            // Necessary to see if we already have this set.
            sort(locs.begin(), locs.end());
            map<utilib::BasicArray<int>, int>::iterator lcurr = membersets.find(locs);
            int ndx = 0;

            if (lcurr == membersets.end())
            {
               //
               // Add to the current super location set
               //
               ndx = membersets.size();
               membersets[locs] = ndx;
            }
            else
            {
               ndx = lcurr->second;
            }

            DEBUGSL3

            int repr = nodes[ord[last]];   // highest impact
            if ((repr == -1) && (last < prev_feasible))
            {
               repr = nodes[ord[last+1]];   // but not dummy
            }

            if (!output_impact)
            {
               // need map of SL to node with worst impact
               (*slcurr)[ndx] = repr;
               // need to save list of superlocations for event
               scurr->push_back(ndx);
            }
            else
            {
               // **** Old version used the best impact for a superlocation.
               // We've historically been pessimistic, so changing this to the
               // worst (highest impact).  The earliest witness might have been
               // an artifact of old Lagrangian aggregation code.
               // Ultimately, the user should be able to pick what they want
               // here:  best, worst, median, mean
               // pessimistic witness: worst impact

               int worst_witness = locs[0];
               int worst_witness_time = get_witness_time(eventNum, worst_witness);
               double worst_witness_impact = get_impact(eventNum, worst_witness);

               for (int loc = 1; loc < locs.size(); loc++)
               {
                  int witness = locs[loc];
                  double  witness_impact = get_impact(eventNum, witness);
                  int witness_time = get_witness_time(eventNum, witness);

                  if (witness_impact > worst_witness_impact)
                  {
                     worst_witness = witness;
                     worst_witness_time = witness_time;
                     worst_witness_impact = witness_impact;
                  }
               }
               slocData sl;
               sl.slID = ndx;
               sl.origID = worst_witness;
               sl.impact = worst_witness_impact;
               std::map<int, std::list<slocData > >::iterator timerec =
                  sltimecurr->find(worst_witness_time);
               if (timerec == sltimecurr->end())
               {
                  std::list<slocData > L(1, sl);
                  (*sltimecurr)[worst_witness_time] = L;
               }
               else
               {
                  timerec->second.push_back(sl);
               }
            }

            last = i;
            if ((sl_worst != 0.0) && (final_aggregation_ratio > (sl_best / sl_worst)))
               final_aggregation_ratio = sl_best / sl_worst;
            /// NOTE: if this is uncommented, then only one superlocation is created with a nonzero threshold.
            //tmp_threshold=0.0;
            locs.clear();
#if 0
            lvalues.clear();
#endif
            if (i == ord.size()) break;
            prev_feasible = i;
            sl_worst = sl_best = values[ord[i]];
            locs.push_back(nodes[ord[i]]);
#if 0
            lvalues.push_back(values[ord[i]]);
#endif
         }
         i++;
      }

      if (output_impact)
      {
         // Create a superlocation for each fixed location.
         std::list<int>::iterator ncurr = fixed_nodes.begin();
         int ndx = 0;
         while (ncurr != fixed_nodes.end())
         {
            int fixed_node = *ncurr;
            utilib::BasicArray<int> locs;
            locs.push_back(fixed_node);
            map<utilib::BasicArray<int>, int>::iterator lcurr = membersets.find(locs);
            if (lcurr == membersets.end())
            {
               ndx = membersets.size();
               membersets[locs] = ndx;
            }
            else
            {
               ndx = lcurr->second;
            }
            int witness_time = get_witness_time(eventNum, fixed_node);

            slocData sl;
            sl.slID = ndx;
            sl.origID = fixed_node;
            sl.impact = get_impact(eventNum, fixed_node);
            std::map<int, std::list<slocData > >::iterator timerec =
               sltimecurr->find(witness_time);
            if (timerec == sltimecurr->end())
            {
               std::list<slocData > L(1, sl);
               (*sltimecurr)[witness_time] = L;
            }
            else
            {
               timerec->second.push_back(sl);
            }
            ncurr++;
         }
         sltimecurr++;
      }

      //
      // Update indices
      //
      slcurr++;
      eventNum++;
      if (!output_impact)
      {
         scurr++;
      }
   }

   if (output_impact)
   {
      // Done with these
      events.free_witness_time_maps();
      events.free_impacts();
   }
}

void SPProblem::write_impact_file(ostream& os, int goal_index)
{
   SPGoalInfo& goal = goals[goal_index];
   os << num_nodes << endl;
   os << "1 " << delay << endl;
   int numEvents = goal.num_events();
   int i = 0;
   while (i < numEvents)
   {
      std::list<std::pair<int, int> > &wtime = goal.events.get_witness_times(i);

      std::list<std::pair<int, int> >::const_iterator mcurr = wtime.begin();
      std::list<std::pair<int, int> >::const_iterator mend  = wtime.end();

      while (mcurr != mend)
      {
         double impact = goal.events.find_impact(i,mcurr->first);
         os << (i+1) << " " << mcurr->first << " " << mcurr->second << " " << impact << std::endl;
         mcurr++;
      }
      i++;
   }
}

void SPProblem::write_aggregated_impacts(std::ostream& osIm, int goal_index)
{
   SPGoalInfo& goal = goals[goal_index];

   osIm << num_superlocations << std::endl;
   osIm << "1 " << delay << std::endl;

   int numevents = goal.sl_wtime.size();

   for (int event = 0; event < numevents; event++)
   {

      std::map<int, std::list<slocData > > timemap = goal.sl_wtime[event];

      std::map<int, std::list<slocData > >::iterator tcurr = timemap.begin();
      std::map<int, std::list<slocData > >::iterator tend = timemap.end();

      std::list<int> wtimes;
      while (tcurr != tend)
      {
         wtimes.push_back(tcurr->first);
         tcurr++;
      }
      wtimes.sort();   // order by increasing witness time

      std::list<int>::iterator curr = wtimes.begin();

      while (curr != wtimes.end())
      {
         int wtime = *curr;

         std::map<int, std::list<slocData > >::iterator currlist = timemap.find(wtime);

         std::list<slocData > sllist = currlist->second;
         std::list<slocData >::iterator slcurr = sllist.begin();
         while (slcurr != sllist.end())
         {
            slocData sl = *slcurr;
            int superloc = sl.slID;
            int origloc = sl.origID;   // earliest witness
            double impact = sl.impact;

            if (superloc == 0)
            {
               osIm << (event + 1) << " -1 ";  // dummy is in ID 0
            }
            else
            {
               osIm << (event + 1) << " " << superloc << " ";
            }

            osIm << wtime << " " << impact << std::endl;

            slcurr++;
         } // next super location
         curr++;
      } // next time
   } // next event
}

void SPProblem::write_aggregated_config_file(std::ostream& ostr, std::vector<std::string> &impactFiles)
{
   ostr << num_superlocations << " " << delay << std::endl;
   ostr << goals.size() << std::endl;
   //
   // Write back goals with new impact file names
   //
   utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
   utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
   int i = 0;
   float threshold = -1;   // flag: don't aggregate next time
   while (curr != end)
   {
      ostr << curr->goal << " " << impactFiles[i] << " ";
      ostr << threshold << " " << curr->aggregation_percentage << " ";
      ostr << curr->aggregation_ratio << " " << curr->distinguish_detection << " ";
      ostr << curr->measure.size() << " ";
      std::list<std::string>::iterator mcurr = curr->measure.begin();
      while (mcurr != curr->measure.end())
      {
         ostr << *mcurr << " ";
         mcurr++;
      }
      if (curr->objective_flag)
      {
         ostr << "o ";
      }
      else
      {
         ostr << "c ";
      }
      std::list<double>::iterator bcurr = curr->bound.begin();

      while (bcurr != curr->bound.end())
      {
         ostr << *bcurr << " ";
         bcurr++;
      }

      ostr << std::endl;
      i++;
      curr++;
   }

   //
   // Find fixed superlocations.
   //
   // For superlocations that are not fixed, save a list of the original
   // location in that superlocation with earliest impact for each goal/event
   // that superlocation appears in.  (Use worst impact for tie breaker.)
   //
   // We should include costs somehow - TODO

   int nfixed = fixed_location.nbits();
   int countFixed = 0;
   std::list<int> initList(1, -2);

   // slMap[slID] = fixedID, goal, event, firstWitness, goal, event, firstWitness, ...
   //   Either fixedID >= -1 and that's entire list, or
   //   fixedID == -2 and goal/event/firstWitnesses follow
   // IDs are the original IDs from the input impact files.

   std::vector<std::list<int> > slMap(num_superlocations + 1, initList);

   curr = goals.begin();
   end = goals.end();
   i = 0;

   while (curr != end)
   {
      utilib::BasicArray<std::map<int, std::list<slocData> > > timemaps = curr->sl_wtime;

      utilib::BasicArray<std::map<int, std::list<slocData> > >::iterator
      evcurr = timemaps.begin();
      utilib::BasicArray<std::map<int, std::list<slocData> > >::iterator
      evend = timemaps.end();

      int event = 1;
      while (evcurr != evend)
      {
         std::map<int, std::list<slocData> >::iterator timecurr = (*evcurr).begin();
         std::map<int, std::list<slocData> >::iterator timeend = (*evcurr).end();

         while (timecurr != timeend)
         {
            std::list<slocData> sllist = timecurr->second;
            std::list<slocData>::iterator slcurr = sllist.begin();
            std::list<slocData>::iterator slend = sllist.end();
            while (slcurr != slend)
            {
               slocData sl = *slcurr;
               int slID = sl.slID;
               if (slMap[slID].front() == -2)
               {
                  int redReprID = sl.origID;
                  int fixed = 0;
                  int origID = -1;
                  if (redReprID > 0)
                  {
                     fixed = fixed_location(redReprID - 1);
                     origID = original_node_id[redReprID];
                  }
                  if (fixed)
                  {
                     slMap[slID].pop_front();
                     slMap[slID].push_back(origID);
                     countFixed++;
                  }
                  else
                  {
                     slMap[slID].push_back(i);
                     slMap[slID].push_back(event);
                     slMap[slID].push_back(origID);
                  }
               }
               slcurr++;
            } // next superlocation in list for this witness time
            timecurr++;
         } // next witness time
         evcurr++;
         event++;
      } // next event
      curr++;
      i++;
   } // next goal

   // Write fixed placements.
   //
   // These are superlocation ID numbers.

   ostr << countFixed << " ";

   if (nfixed > 0)
   {
      for (int slID = 0; slID <= num_superlocations; slID++)
      {
         if (slMap[slID].front() > -2)
         {
            if (slID == 0)
            {
               std::cout << "Error in write_aggregated_config_file" << std::endl;
               std::cout << "Dummy is a fixed location." << std::endl;
               exit(1);
            }
            ostr << slID << " ";
         }
      }
   }
   ostr << std::endl;

   // Write invalid locations.
   //
   // There are no invalid superlocations.  Invalid locations were
   // already filtered out.

   ostr << "0" << std::endl;


   // Write location costs.
   //
   // We are not handling costs yet.  What should the cost of a
   // superlocation be?  If it's a fixed location, make it
   // the cost of the fixed location.  But if superlocation appears
   // in more than one goal/event record, it's worst impact ID
   // may differ, and then what cost should we use?

   ostr << "0" << std::endl;

   // Write mapping from superlocations to earliest impact original IDs.

   ofstream of("superlocations_first.txt");
   of << "# List of locations that had earliest witness time in super location" << std::endl;
   of << "# First line: number of superlocations" << std::endl;
   of << "# Following lines (in order by superlocation ID):" << std::endl;
   of << "# SL-ID goal event original-ID goal event original-ID ..." << std::endl;
   of << "# (If location is fixed, SL-ID is followed by real location ID only)" << std::endl;
   of << num_superlocations << std::endl;

   for (int slID = 0; slID <= num_superlocations; slID++)
   {
      if (slID == 0)
         continue;  // the dummy's in 0

      of << slID << " ";

      std::list<int> slInfo = slMap[slID];
      if (slInfo.size() == 1)        // original ID of fixed location
      {
         of << slInfo.front() << std::endl;
      }
      else    // goal, event, original ID of location with worst impact
      {
         slInfo.pop_front();
         std::list<int>::iterator curr = slInfo.begin();
         int i = 0;
         while (curr != slInfo.end())
         {
            if (i % 3 == 0)
            {
               of << goals[*curr].goal << " ";
            }
            else
            {
               of << *curr << " ";
            }
            i++;
            curr++;
         }
         of << std::endl;
      }
   }
   of.close();
}


void SPProblem::write_ampl_data(ostream& os, bool imperfect)
{
   os << "#" << endl;
   os << "# GeneralSP AMPL file" << endl;
   os << "#" << endl;

   //
   // For now, we only consider a single objective
   //
   os << endl;
   os << "param objectiveGoal := " << goals[*(objectives.begin())].goal << " ;" << endl;
   os << "param objectiveMeasure := " << *(goals[*(objectives.begin())].measure.begin()) << " ;" << endl;
   os << endl;
   //
   // Value used for CVaR and TCE calculations
   //
   os << "param gamma := " << gamma <<  " ;" << endl;
   //
   // Goal constraint information
   //
   os << "param: goalConstraints: GoalBound :=" << endl;
   {
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         list<double>::iterator bcurr = curr->bound.begin();
         list<string>::iterator mcurr = curr->measure.begin();
         list<string>::iterator mend  = curr->measure.end();
         while (mcurr != mend)
         {
            double goalval = *bcurr;
            if (goalval != SPGoalInfo::OBJECTIVE_GOAL_BOUND)
               os << curr->goal << " " << *mcurr << " " << *bcurr << endl;
            bcurr++;
            mcurr++;
         }
         curr++;
      }
      os << ";" << endl;
   }
   //
   // Superlocation aggregation threshold information
   //
   os << endl;
   {
      os << "param slThreshold := " << endl;
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         if (curr->num_events() > 0)
            os << curr->goal << " " << curr->threshold << endl;
         curr++;
      }
      os << ";" << endl;
   }
   //
   // Superlocation aggregation ratio information
   //
   os << endl;
   {
      os << "param slAggregationRatio := " << endl;
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         if (curr->num_events() > 0)
            os << curr->goal << " " << curr->final_aggregation_ratio << endl;
         curr++;
      }
      os << ";" << endl;
   }

   ////
   //// Network location information
   ////
   os << endl;
   os << "param numNetworkLocations := " << num_nodes << " ;" << endl;
   //
   // Fixed locations
   //
   if (fixed_placements.size() > 0)
   {
      os << "set FixedSensorLocations :=" << endl;
      utilib::BasicArray<int>::iterator curr = fixed_placements.begin();
      utilib::BasicArray<int>::iterator end  = fixed_placements.end();
      while (curr != end)
      {
         os << *curr << endl;
         curr++;
      }
      os << ";" << endl;
   }
#if 0
   // WEH - Invalid locations are now preprocessed when reading the input, and thus
   //     they do not need to be explicitly include in the AMPL *.dat file.
   //
   // Invalid locations
   //
   if (invalid_placements.size() > 0)
   {
      os << "set InfeasibleLocations :=" << endl;
      utilib::BasicArray<int>::iterator curr = invalid_placements.begin();
      utilib::BasicArray<int>::iterator end  = invalid_placements.end();
      while (curr != end)
      {
         os << *curr << endl;
         curr++;
      }
      os << ";" << endl;
   }
#endif

   //
   // Event information
   //
   os << endl;
   {
      os << "param numEvents := " << endl;
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         if (curr->num_events() > 0)
            os << curr->goal << " " << curr->num_events() << endl;
         curr++;
      }
      os << ";" << endl;
   }
   {
      int tmp = 0;
      for (unsigned int i = 0; i < goals.size(); i++)
         tmp += goals[i].probabilities.size();
      if (tmp > 0)
      {
         //
         // At least one of the goals has specified probabilities
         //
         os << "param EventWeight :=" << endl;
         utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
         utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
         while (curr != end)
         {
            for (unsigned int i = 0; i < curr->probabilities.size(); i++)
               os << curr->goal << " " << (i + 1) << " " << curr->probabilities[i] << endl;
            curr++;
         }
         os << ";" << endl;
      }
   }

   //
   // Points where events can have an impact
   //
   {
      os << endl;
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         for (unsigned int i = 0; i < curr->num_events(); i++)
         {
            std::vector<std::pair<int, double> > &impactMap = curr->events.get_impact_map(i);
            os << "set EventTouchedLocations[" << curr->goal << "," << (i + 1) << "] := ";
            std::vector<std::pair<int,double> >::iterator mcurr = impactMap.begin();
            std::vector<std::pair<int,double> > ::iterator mend  = impactMap.end();
            while (mcurr != mend)
            {
               os << " " << mcurr->first;
               mcurr++;
            }
            os << ";" << endl;
         }
         curr++;
      }
   }

   //
   // SuperLocations
   //
   os << endl;
   {
      os << "set SuperLocations :=";
      map<utilib::BasicArray<int>, int>::iterator curr = membersets.begin();
      map<utilib::BasicArray<int>, int>::iterator end  = membersets.end();
      while (curr != end)
      {
         os << " " << curr->second;
         curr++;
      }
      os << ";" << endl;
   }

   //
   // SuperLocation Members
   //
   os << endl;
   {
      map<utilib::BasicArray<int>, int>::iterator curr = membersets.begin();
      map<utilib::BasicArray<int>, int>::iterator end  = membersets.end();
      while (curr != end)
      {
         os << "set SuperLocationMembers[" << curr->second << "] :=";
         utilib::BasicArray<int>::const_iterator lcurr = curr->first.begin();
         utilib::BasicArray<int>::const_iterator lend  = curr->first.end();
         while (lcurr != lend)
         {
            os << " " << *lcurr;
            lcurr++;
         }
         os << ";" << endl;
         curr++;
      }
   }

   //
   // The EventSuperLocation sets
   //
   {
      os << endl;
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         for (unsigned int i = 0; i < curr->super_locs.size(); i++)
         {
            os << "set EventSuperLocations[" << curr->goal << "," << (i + 1) << "] := ";
            list<int>::iterator lcurr = curr->super_locs[i].begin();
            list<int>::iterator lend  = curr->super_locs[i].end();
            while (lcurr != lend)
            {
               os << " " << *lcurr;
               lcurr++;
            }
            os << ";" << endl;
         }
         curr++;
      }
   }


   //
   // Impact measuresments for each goal
   //
   {
      os << endl;
      os << "param GoalMeasurement :=" << endl;
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         for (unsigned int i = 0; i < curr->num_events(); i++)
         {
            vector<pair<int, double> > &impactMap = curr->events.get_impact_map(i);
            vector<pair<int,double> >::iterator mcurr = impactMap.begin();
            vector<pair<int,double> >::iterator mend  = impactMap.end();
            while (mcurr != mend)
            {
               os << curr->goal << " " << (i + 1) << " " << mcurr->first << " " << mcurr->second << endl;
               mcurr++;
            }
         }
         curr++;
      }
      os << ";" << endl;
   }

   //
   // Placement costs
   //
   if (costs.size() > 0)
   {
      os << "param PlacementCost :=" << endl;
      double dval;
      map<int, double>::iterator tmp = costs.find(-1);
      if (tmp == costs.end())
         dval = 0.0;
      else
         dval = tmp->second;
      for (unsigned int i = 0; i < feasible_location.size(); i++)
         if (feasible_location(i))
         {
            map<int, double>::iterator tmp = costs.find(i + 1);
            if (tmp == costs.end())
               os << (i + 1) << " " << dval << endl;
            else
               os << (i + 1) << " " << costs[i+1] << endl;
         }
      os << ";" << endl;
   }

   //
   // The ID of the worst super location member for a given goal and event
   //
   {
      os << endl;
      os << "param WorstSuperLocationMember :=" << endl;
      utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      while (curr != end)
      {
         for (unsigned int i = 0; i < curr->sl_worst.size(); i++)
         {
            map<int, int>::iterator mcurr = curr->sl_worst[i].begin();
            map<int, int>::iterator mend  = curr->sl_worst[i].end();
            while (mcurr != mend)
            {
               os << curr->goal << " " << (i + 1) << " " << mcurr->first << " " << mcurr->second << endl;
               mcurr++;
            }
         }
         curr++;
      }
      os << ";" << endl;
   }

   if (imperfect)
   {
      os << "param numEnvironmentTypes := " << 3 << " ;" << endl;

      os << "param EnvironmentTypes := " << endl;
      for (int i = 1; i < num_nodes / 3; i++)
      {
         os << i << " 1" << endl;
      }
      for (int i = num_nodes / 3; i < 2*num_nodes / 3; i++)
      {
         os << i << " 2" << endl;
      }
      for (int i = 2 * num_nodes / 3; i < num_nodes; i++)
      {
         os << i << " 3" << endl;
      }
      os << ";" << endl << endl;

      os << "param FalsePositiveRate := " << endl;
      os << "1 0.00001" << endl;
      os << "2 0.00001" << endl;
      os << "3 0.00001" << endl;
      os << ";" << endl << endl;

      os << "param FalseNegativeProbability := " << endl;
      os << "1 0.125" << endl;
      os << "2 0.25" << endl;
      os << "3 0.5" << endl;
      os << ";" << endl;

   }

   os << endl;
   os << "end;" << endl;
}

void SPProblem::write_lagrangian_data(bool imperfect, std::string& outputfile_name)
{
   //
   // Write the input file for the lagrangian relaxation code.
   //

   ofstream *os;
   int last_slash;
   utilib::BasicArray<SPGoalInfo>::iterator curr = goals.begin();
   utilib::BasicArray<SPGoalInfo>::iterator end = goals.end();
   while (curr != end)
   {
      // Check the correctness of the number of sensors goal, but otherwise ignore it.
      // This information is passed to the Lagrangian solver on the command line.
      if (curr->goal == "ns")
      {
         // Number of sensors goal, should have precisely one measure that it should be total
         if (curr->measure.size() != 1)
            EXCEPTION_MNGR(runtime_error, "Number-of-sensors goal can only have one measure");
         if (*(curr->measure.begin()) != "total")
            EXCEPTION_MNGR(runtime_error, "Number-of-sensors goal can only have the total measure");
         curr++;
         continue;
      }
      // If we weren't given probabilities, use the uniform distribution
      if (curr->probabilities.size() == 0)
      {
         int num_customers = curr->num_events();
         double uniform_prob = 1.0 / num_customers;
         curr->probabilities.resize(num_customers);
         for (int i = 0; i < num_customers; i++)
         {
            curr->probabilities[i] = uniform_prob;
         }
      }

      //
      // Impact measuresments for each goal
      //

      /* CAP:  pretty much all deprecated stuff.  Anything relevant, we pull
      out above
      double total_of_max_impacts = 0;
      int num_events=0;
      int num_impacts=0;
      //    std::list<int> theLocations;
      curr = goals.begin();
      utilib::BasicArray<SPGoalInfo>::iterator end  = goals.end();
      int goalCount=0;
      while (curr != end && goalCount < 1) {
      goalCount++;
      for (unsigned int i=0; i<curr->num_events(); i++) {
        std::map<int, double> &impactMap = curr->events.get_impact_map(i);
        map<int,double>::iterator mcurr = impactMap.begin();
        map<int,double>::iterator mend  = impactMap.end();
        double max_impact = 0;
        while (mcurr != mend) {
          if (mcurr->second > max_impact) {
                max_impact = mcurr->second;
          }
      num_impacts++;
      //          theLocations.push_back(mcurr->first);
          mcurr++;
        }
        total_of_max_impacts += max_impact;
      //        theLocations.sort();    // is there a better way to get the
      //        theLocations.unique();  // node id's?
      }
      curr++;
      }
      */
      /*  CAP: we now pass the number of sensors in the command line from the sp script,
          so this goes away.  I'm leaving for a little while in case the code can be reused
          during these modifications.

      int num_sensors=0;
      if (curr != end) {
        list<double>::iterator bcurr = curr->bound.begin();
        list<string>::iterator mcurr = curr->measure.begin();
        list<string>::iterator mend  = curr->measure.end();
        while (mcurr != mend) {
          double goalval = *bcurr;
          if (curr->goal == "ns" && *mcurr == "total")
                  num_sensors = (int) *bcurr;
          bcurr++;
          mcurr++;
        }
      }
      */
      /*  This was for uflp (now deprecated)

          int num_nodes_hit = theLocations.size();
          double cost_of_one_facility = total_of_max_impacts/num_nodes_hit;
          assert(num_nodes_hit <= num_nodes);
          int *water2lr_map = new int[num_nodes];
          int *lr2water_map = new int[num_nodes_hit];
          int next_id=1;
          list<int>::iterator lit = theLocations.begin();
          while (lit != theLocations.end()) {
              int loc = *lit;
              if (loc == -1) loc = num_nodes;
              os << cost_of_one_facility << std::endl;
              water2lr_map[loc] = next_id;
              lr2water_map[next_id] = loc;
              lit++;
              next_id++;
          }
      */
      int total_impacts = 0;
      for (unsigned int i = 0; i < curr->num_events(); i++)
      {
         total_impacts += curr->num_impacts(i);
      }

      // If the impact file name for a goal is foo.impact, we will create the file foo.lag for
      // the Lagranian code.
      if (outputfile_name == "") {
            outputfile_name = curr->impact_file.c_str();
      // If the impact file was not in this directory, there will be a long path in this name.
      // We want to create the .lag file in the working directory.  So remove the path prefix.
      // This might be done more nicely with find_last_of (looking for the slash), but that method
      // doesn't seem to be working, so we'll go with this.

      last_slash = -1;
      for (int i = outputfile_name.length() - 1; i >= 0; i--)
         if (outputfile_name[i] == '/')
         {
            last_slash = i;
            break;
         }

      if (last_slash >= 0)
         outputfile_name = outputfile_name.substr(last_slash + 1, outputfile_name.length() - 1);

      // replace the "impact" extension
      outputfile_name.replace(outputfile_name.length() - 6, 6, "lag", 3);
        }

      os = new ofstream(outputfile_name.c_str());
      *os << num_nodes << " " << curr->num_events() << " " << total_impacts << " " << endl;
      for (unsigned int i = 0; i < curr->num_events(); i++)
      {
         vector<pair<int,double> > &impactMap = curr->events.get_impact_map(i);
         vector<pair<int,double> >::iterator mcurr = impactMap.begin();
         vector<pair<int,double> >::iterator mend  = impactMap.end();
         double prob = goals[0].probabilities[i];
         while (mcurr != mend)
         {
            int loc = mcurr->first;
            if (loc == -1)
            {
               loc = num_nodes;
            }
            else
            {
               loc = loc - 1;
            }
            *os << loc << "\t" << i + 1 << "\t" << prob * mcurr->second << std::endl;
            mcurr++;
         }
      }
      os->close();
      delete os;
      curr++;
   }
   /*  This was for uflp (now deprecated)
     delete [] lr2water_map;
     delete [] water2lr_map;
   */
}

