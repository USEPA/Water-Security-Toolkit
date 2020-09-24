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
 * \file impacts.cpp
 * \brief Routines to evaluate the impact of a contamination event, given
 *      sensors.
 */
#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include "sp/impacts.h"
#include <utilib/sort.h>
#include <utilib/comments.h>
#include <iostream>
#include <vector>
#include <list>
#include <iomanip>

using namespace std;

///
/// Read the (node,weight) pairs
///
void read_incident_weights(std::istream& istr, std::map<int, double>& weights)
{
   double total = 0.0;
   int nodeid;
   istr >> nodeid;
   while (istr)
   {
      double weight;
      istr >> weight;
      weights[nodeid] = weight;
      istr >> nodeid;
   }
}


int read_impact_preamble(std::istream& istr, int& nnodes, int delay)
{
   int numDelays;

   istr >> nnodes >> numDelays;
   //
   // Read delays
   //
   int delayIndex = -1;
   for (int i = 0; i < numDelays; i++)
   {
      int tmp;
      istr >> tmp;
      if (tmp == delay) delayIndex = i;
   }
   if (delayIndex == -1)
   {
      EXCEPTION_MNGR(runtime_error, "delay "
                     << delay << " not found in impact file" << endl);
   }
   return delayIndex;
}

int read_next_impact(std::istream& istr, int eventIndex,
                     std::map<int, double>& next_event_impacts,
                     std::vector<std::pair<int, int> > &next_event_sequence,
                     int delayIndex)
{
   // precondition: the event index has been read from the first line of
   //               this event's portion of the impact file

   int numDelays = 1;
   //
   // Variables used to read in the impact file
   // NOTE: event and impact IDs are locations that are 1-based indices.
   //
   int prevEventIndex = -1;    // ID of the previous event
   bool started = false;
   if (istr.eof())
   {
      return eventIndex;
   }
   while (istr)
   {
      if (eventIndex != prevEventIndex)
      {
         if (!started)
         {
            prevEventIndex = eventIndex;
            started = true;
         }
         else
         {
            break;   // done with the event's impacts
         }
      }
      int hitNodeIndex;       // ID of an impacted node "downstream"
      int witnessTime;
      double impact(0.0);
      istr >> hitNodeIndex >> witnessTime;
      for (int i = 0; i < numDelays; i++)
      {
         double tmp;
         istr >> tmp;
         if (i == delayIndex) impact = tmp;
      }
      next_event_impacts[hitNodeIndex] = impact;
      next_event_sequence.push_back(std::make_pair(hitNodeIndex, witnessTime));
      istr >> eventIndex;
   }
   return eventIndex;
}

void read_impacts(std::istream& istr,
                  std::vector<std::map<int, double> >& impacts,
                  std::vector<std::vector<std::pair<int, double> > > *arrival_sequences,
                  int delay,
                  std::vector<std::list<std::pair<int, int> > >* wtimes,
                  utilib::BitArray& feasible_location,
                  std::map<int, int>& reduced_repn_map)
{
   int nnodes, numDelays;
   char buf[256];
   int numEvents;
   // TNT: Changed so that it can understand the first line having one or two values
   // The second value (if present) represents the number of events contained in the file.
   istr.getline(buf, 256);
   int nvals = sscanf(buf, "%d %d", &nnodes, &numEvents);
   if (nvals < 1 || nvals > 2)
   {
      EXCEPTION_MNGR(runtime_error, "improperly formatted impact file" << endl);
   }
   else if (nvals == 1)
   {
      numEvents = -1;
   }

   // CAP:  If we did not get a nodemap file in evalsensors (for example), the reduced representation
   // map will still be empty, with resulting bad behavior.  Set it up now

   if (reduced_repn_map.size() == 0)
      for (size_type i = 0; i < nnodes; i++)
         reduced_repn_map[i+1] = i + 1;


   impacts.reserve(numEvents != -1 ? numEvents : nnodes);
   if (wtimes)
      wtimes->reserve(numEvents != -1 ? numEvents : nnodes);
   // TNT: Only process the arrival sequences if necessary
   if (arrival_sequences != NULL)
   {
      arrival_sequences->reserve(numEvents != -1 ? numEvents : nnodes);
   }
#if 0
   utilib::IntVector delays(numDelays);
   for (int i = 0; i < numDelays; i++)
   {
      istr >> delays[i];
      if (i > 0)  assert(delays[i] >= delays[i-1]);  // force tso2Impact to
      // put these in order
   }
   // now, search the delays to find the index of the delay specified by the user
   int delayIndex = -1;
   for (int i = 0; i < numDelays; i++)
   {
      if (delays[i] == delay)
         delayIndex = i;
   }
#endif
   istr.getline(buf, 256);
   char *tok = strtok(buf, "\t ");
   sscanf(tok, "%d", &numDelays);
   //
   // Read delays
   //
   int delayIndex = -1;
   for (int i = 0; i < numDelays; i++)
   {
      int tmp;
      tok = strtok(NULL, "\t ");
      tmp = atoi(tok);
      if (tmp == delay) delayIndex = i;
   }
   if (delayIndex == -1)
   {
      EXCEPTION_MNGR(runtime_error, "delay "
                     << delay << " not found in impact file" << endl);
   }
   //
   // Variables used to read in the impact file
   // NOTE: event and impact IDs are locations that are 1-based indices.
   //
   int prevScenarioIndex = -1;    // ID of the previous event
   int scenarioIndex;       // event ID
   //
   // Read in the impact file.
   //
   istr.getline(buf, 256);
   while (istr)
   {
      tok = strtok(buf, "\t ");
      scenarioIndex = atoi(tok);
      //
      // If we have a new scenario, then extend the impacts and arrival
      // sequences arrays to include new entries for the new scenario.
      //
      if (scenarioIndex != prevScenarioIndex)
      {
         impacts.push_back(std::map<int, double>());
         if (wtimes)
            wtimes->push_back(std::list<std::pair<int, int> >());
         if (arrival_sequences != NULL)
         {
            arrival_sequences->push_back(std::vector<std::pair<int, double> >());
         }
         prevScenarioIndex = scenarioIndex;
      }
      //
      // Add the new impact that we have just read to the hash table for the
      // latest event
      //
      int hitNodeIndex;       // ID of an impacted node "downstream"
      int witnessTime;
      double impact(0.0);
      tok = strtok(NULL, "\t "); hitNodeIndex = atoi(tok);
      tok = strtok(NULL, "\t "); witnessTime  = atoi(tok);
      //
      // If feasible_location is empty, then all locations are
      // feasible
      //
      if ((hitNodeIndex == -1) || (feasible_location.size() == 0) || feasible_location(hitNodeIndex - 1))
      {
         if (hitNodeIndex != -1)
            hitNodeIndex = reduced_repn_map[hitNodeIndex];
         if (wtimes)
         {
            std::pair<int, int> tmp(hitNodeIndex, witnessTime);
            wtimes->back().push_back(tmp);
         }

         // delay getting impact to avoid unneeded string to double conversions...
         for (int i = 0; i < numDelays; i++)
         {
            tok = strtok(NULL, "\t ");
            if (i == delayIndex) impact = atof(tok);
         }

         impacts.back()[hitNodeIndex] = impact;
         if (arrival_sequences != NULL)
         {
            arrival_sequences->back().push_back(std::make_pair(hitNodeIndex, witnessTime));
         }
      }
#if 0
      int detectionTime;      // true detection, not considering delay
      int impactTime;         // time at which delayed impact is
      // measured (detect + delay)
      double impact = 0.0;
      istr >> hitNodeIndex >> detectionTime;
      impactTime = detectionTime + delay;
      for (int i = 0; i < numDelays; i++)
      {
         double nextImpact;
         istr >> nextImpact;
         if (i == delayIndex)
         {
            impact = nextImpact;
         }
      }
      impacts.back()[hitNodeIndex] = impact;
#endif
      //
      //cerr << "DEBUG " << scenarioIndex << " " << hitNodeIndex << " " << impact << " " << delay << endl;
      //
      // Read in the next impact
      //
      istr.getline(buf, 256);
   }
}

#if 0
//WEH - depricated because it's not being used

///
class SPCompare
{
public:

   ///
   bool operator()(const pair<double, double>& a,
                   const pair<double, double>& b) const
   {
      if (a.first < b.first) return true;
      if (a.first > b.first) return false;
      return (a.second < b.second);
   }

};


void compute_delayed_impacts(utilib::BasicArray<std::map<int, std::pair<double, double> > >& raw_impacts,
                             double delay,
                             utilib::BasicArray<std::map<int, double> >& impacts)
{
   impacts.resize(raw_impacts.size());
   for (unsigned int i = 0; i < raw_impacts.size(); i++)
   {
      utilib::BasicArray<pair<double, double> > values(raw_impacts[i].size());
      utilib::BasicArray<int> node(raw_impacts[i].size());
      utilib::BasicArray<int> ord;
      {
         std::map<int, std::pair<double, double> >::iterator curr = raw_impacts[i].begin();
         std::map<int, std::pair<double, double> >::iterator end  = raw_impacts[i].end();
         int k = 0;
         while (curr != end)
         {
            values[k] = curr->second;
            node[k] = curr->first;
            k++;
            curr++;
         }
         order(ord, values, SPCompare());
      }
      //for (unsigned int j=0; j<ord.size(); j++)
      //cerr << j << " " << node[ord[j]] << " " << values[ord[j]].first << " " << values[ord[j]].second << endl;

      if (ord.size() == 0)
         EXCEPTION_MNGR(runtime_error, "Null impacts!?!?!");
      double last_impact;
      unsigned int ii = 0, jj = 1;
      last_impact = values[ord[jj-1]].first;
      while ((jj < ord.size()) &&
             (values[ord[jj]].second <= (delay + 1e-7 + (values[ord[ii]].second))))
      {
         last_impact = values[ord[jj-1]].first;
         cerr << "jj: " << jj << " " << "last impact reset to: " << last_impact << endl;
         jj++;
      }
      last_impact = values[ord[jj-1]].first;
      while (ii < ord.size())
      {
         impacts[i][node[ord[ii]]] = last_impact;
         ii++;
         while ((jj < ord.size()) &&
                (values[ord[jj]].second <= (delay + 1e-7 + values[ord[ii]].second)))
         {
            last_impact = values[ord[jj-1]].first;
            jj++;
         }
         last_impact = values[ord[jj-1]].first;
      }
      //for (unsigned int j=0; j<ord.size(); j++)
      //cerr << j << " " << node[ord[j]] << " " << impacts[i][node[ord[j]]] << endl;
   }
}
#endif


void read_sensor_placements(std::istream& istr, std::map<int, utilib::BasicArray<int> >& sensors)
{
   istr >> utilib::comment_lines;
//
// Read in the sensors one at a time
//
   while (istr)
   {
      int designID;
      istr >> designID;
      if (!istr) break;
      int len;
      istr >> len;
      if (!istr) break;
      utilib::BasicArray<int> array(len);
      for (int i = 0; i < len; i++)
         istr >> array[i];
      sensors[designID] = array;
      istr >> utilib::comment_lines;
   }
}


void read_network_ids(std::istream& istr, utilib::BasicArray<int>& sensors)
{
   //
   // Read in the sensors one at a time
   //
   int newLocation;
   istr >> newLocation;
   while (!(!istr))
   {
      sensors.push_back(newLocation);
      istr >> newLocation;
   }
}

void compute_greedy_order(const utilib::BasicArray<int>& sensors,
                          std::vector<std::map<int, double> >& impacts,
                          std::vector<std::vector<std::pair<int, double> > > &arrival_sequences,
                          std::map<int, int>& scs,
                          std::map<int, double>& sc_probabilities,
                          ImpactStats& stats,
                          std::map<int, double>& probabilities,
                          utilib::BasicArray<int>& work,
                          std::list<std::list<std::pair<int, double> > >& resultsOfPartialSPs,
                          double gamma)
{
   std::list<std::pair<int, double> > resultsOfPartialSP;
   utilib::BasicArray<bool> in_use(sensors.size());

   // compute impact with no sensors...
   utilib::BasicArray<int> no_sensors(0);
   ImpactStats stats_with_partial_sp;
   compute_statistics(no_sensors, impacts,
                      arrival_sequences, scs,
                      sc_probabilities, stats_with_partial_sp,
                      probabilities, work, gamma);
   resultsOfPartialSP.push_back(std::pair<int, double>(-1, stats_with_partial_sp.meanImpact));

   for (size_t i = 0; i < sensors.size(); i++)
   {
      in_use[i] = false;
   }
   for (size_t numsens = 0; numsens < sensors.size(); numsens++)
   {
      int index_of_best_mean = 0;
      double best_mean = DBL_MAX;
      //
      // A temporary sensor placement that is tried out this iteration
      //
      utilib::BasicArray<int> current_sp(numsens + 1);
      int next = 0;
      for (size_t i = 0; i < sensors.size(); i++)
      {
         if (in_use[i])
            current_sp[next++] = sensors[i];
      }
      //
      // Try out each of the sensors that are not in use
      //
      for (size_t i = 0; i < sensors.size(); i++)
      {
         if (in_use[i]) continue;
         current_sp[numsens] = sensors[i];
         //
         // Compute the mean impact for the 'current_sp' sensor placement'
         //
         ImpactStats stats_with_partial_sp;
         compute_statistics(current_sp, impacts,
                            arrival_sequences, scs,
                            sc_probabilities, stats_with_partial_sp,
                            probabilities, work, gamma);
         //
         // Update index of the best mean seen so far
         //
         if (stats_with_partial_sp.meanImpact < best_mean)
         {
            index_of_best_mean = i;
            best_mean = stats_with_partial_sp.meanImpact;
         }
      }
      //
      // Augment our greedy order with the best next sensor placement
      //
      resultsOfPartialSP.push_back(std::pair<int, double>(sensors[index_of_best_mean], best_mean));
      in_use[index_of_best_mean] = true;
   }
   //
   // Augment this list, for the greedy results for this sensor placement
   // This function is called multiple times, augmenting this list each time.
   //
   resultsOfPartialSPs.push_back(resultsOfPartialSP);
}

void compute_statistics(const utilib::BasicArray<int>& sensors,
                        std::vector<std::map<int, double> >& impacts,
                        std::vector<std::vector<std::pair<int, double> > > &arrival_sequences,
                        std::map<int, int>& scs,
                        std::map<int, double>& sc_probabilities,
                        ImpactStats& stats,
                        std::map<int, double>& probabilities,
                        utilib::BasicArray<int>& work,
                        double gamma)
{
   //cerr << "Computing Statistics" << endl;
   // STL-ize the sensor collection so you can use STL algorithms
   std::vector<int> sensorsSTL(sensors.size());
   for (size_t i = 0; i < sensors.size(); i++)
   {
      sensorsSTL[i] = sensors[i];
   }

   //
   // Collect the impact values for each event
   //
   utilib::BasicArray<double> values(impacts.size());
   stats.witness.resize(impacts.size());
   stats.impact.resize(impacts.size());
   for (size_t i = 0; i < impacts.size(); i++)
   {
      std::map<int, double> &scenarioImpacts = impacts[i];
      if (scs.size())
      {
         double scenarioExpectedImpact = 0.0;
         double scenarioRemainingProbability = 1.0;

         // scan the nodes in arrival sequence. this is required to correctly compute the
         // probability of failing to detect at a node (temporally) "upstream". the logic
         // is somewhat complicated due to the fact that multiple sensors might be able to
         // detect an event at any given time => the aggregate probability of detection/remaining
         // is a function of all candidate sensor detection probabilities.
         std::vector<std::pair<int, double> > &thisArrivalSequence = arrival_sequences[i];
         for (size_t j = 0; j < thisArrivalSequence.size();)
         {
            double arrivalTime(thisArrivalSequence[j].second);
            bool detectionPossible(false); // until proven otherwise (used to avoid epsilon comparison issues with doubles)
            double failedDetectionProbability(1.0);
            double cumulativeImpact(scenarioImpacts[thisArrivalSequence[j].first]);

            while ((j < thisArrivalSequence.size()) && (thisArrivalSequence[j].second == arrivalTime))
            {
               int thisNode(thisArrivalSequence[j].first);
               if (std::find(sensorsSTL.begin(), sensorsSTL.end(), thisNode) != sensorsSTL.end())
               {
                  failedDetectionProbability *= (1.0 - sc_probabilities[scs[thisNode]]);
                  detectionPossible = true;
               }
               else if (thisNode == -1)
               {
                  // the dummy node acts as a perfect sensor
                  failedDetectionProbability = 0.0;
                  detectionPossible = true;
               }
               j++;
            }

            // if the event could be detected with any non-zero probability at
            // this node, update the expected impact and remaining probabilities.
            if (detectionPossible == true)
            {
               scenarioExpectedImpact += cumulativeImpact * scenarioRemainingProbability * (1.0 - failedDetectionProbability);
               scenarioRemainingProbability -= scenarioRemainingProbability * (1.0 - failedDetectionProbability);
            }
         }
         stats.impact[i] = values[i] = scenarioExpectedImpact;
      }
      else
      {
         int witnessOfMin = -1;
         double minImpact = scenarioImpacts[witnessOfMin];
         if (minImpact == 0.0) minImpact = -1.0;

         for (size_t j = 0; j < sensors.size(); j++)
         {
            if (scenarioImpacts.find(sensors[j]) != scenarioImpacts.end())
            {
               if ((minImpact < 0.0) || (minImpact > scenarioImpacts[sensors[j]]))
               {
                  minImpact = scenarioImpacts[sensors[j]];
                  witnessOfMin = sensors[j];
               }
            }
         }
         if (minImpact < 0.0) minImpact = 0.0;
         stats.witness[i] = witnessOfMin;
         stats.impact[i] = values[i] = minImpact;
      }
      //cerr << "HERE " << stats.witness[i] << " " << stats.impact[i] << std::endl;
   }

   //
   // Setup the probability array
   //
   utilib::BasicArray<double> probs(values.size());
   if (probabilities.size() > 0)
   {
      probs << 0.0;
      double total = 0.0;
      std::map<int, double>::iterator curr = probabilities.begin();
      std::map<int, double>::iterator end = probabilities.end();
      while (curr != end)
      {
         probs[curr->first - 1] = curr->second; // assume 1 indexed maps
         total += curr->second;  // assume 1 indexed maps
         curr++;
      }
      for (unsigned int i = 0; i < probs.size(); i++)
         probs[i] /= total;
   }
   else
   {
      probs << 1.0 / values.size();
   }
   //
   // Setup data structures for weighted quantiles
   //
   // NOTE: this is probably not the most efficient way to do this
   //
   utilib::order(work, values);
   utilib::BasicArray<double> cumval(values.size());
   cumval[0] = probs[work[0]];
   for (unsigned int i = 1; i < values.size(); i++) {
      cumval[i] = cumval[i-1] + probs[work[i]];
      //std::cerr << "X " << values[work[i]] << " " << work[i] << " " << cumval[i] << " " << probs[work[i]] << std::endl;
      }
   //
   // Compute values for the statistics object
   //
   // nscenarios
   stats.nscenarios = impacts.size();
   // MIN
   stats.minImpact = min(values);
   // MAX
   stats.maxImpact = max(values);
   // MEAN
   stats.meanImpact = 0.0;
   for (unsigned int i = 0; i < values.size(); i++)
      stats.meanImpact += probs[i] * values[i];
   // Quartiles
   stats.firstQImpact = -1.0;
   stats.medianImpact = -1.0;
   stats.thirdQImpact = -1.0;
   stats.VaR = -1.0;
   double alpha = 1.0 - gamma;
   int ndx = -1;
   for (unsigned int i = 0; i < values.size(); i++)
   {
      if ((stats.firstQImpact == -1.0) && (cumval[i] >= 0.25 - 1e-7))
      {
         stats.firstQImpact = values[work[i]];
      }
      else if ((stats.medianImpact == -1.0) && (cumval[i] >= 0.5 - 1e-7))
      {
         stats.medianImpact = values[work[i]];
      }
      else if ((stats.thirdQImpact == -1.0) && (cumval[i] >= 0.75 - 1e-7))
      {
         stats.thirdQImpact = values[work[i]];
      }
      if ((stats.VaR == -1.0) && (cumval[i] >= alpha - 1e-7))
      {
         ndx = i;
         stats.VaR = values[work[i]];
      }
   }
   // TCE
   stats.TCE = 0.0;
   double tail_prob = 0.0;
   for (unsigned int i = ndx; i < values.size(); i++)
   {
      tail_prob += probs[work[i]];
      stats.TCE += values[work[i]] * probs[work[i]];
   }
   stats.TCE /= tail_prob;
}

double compute_cost(const utilib::BasicArray<int>& sensors, std::map<std::string, double>& costs, std::map<int, std::string>& nodemap)
{
   double total = 0.0;
//
// Get the default cost
//
   double dval = 0.0;
   std::map<std::string, double>::iterator dcost = costs.find("__default__");
   if (dcost != costs.end())
      dval = dcost->second;
//
// Loop through the sensors and use the cost value
//
   utilib::BasicArray<int>::const_iterator scurr = sensors.begin();
   utilib::BasicArray<int>::const_iterator send  = sensors.end();
   while (scurr != send)
   {
      std::map<std::string, double>::iterator cost = costs.find(nodemap[*scurr]);
      if (cost != costs.end())
         total += cost->second;
      else
         total += dval;
      scurr++;
   }
   return total;
}


/// Comparator for sorting sensors by decreasing impact mitigated
int paircmp(const std::pair<int, double>& p1, const std::pair<int, double>&p2)
{
   if (p2.second < p1.second)  return -1;
   else if (p2.second == p1.second) return 0;
   else    return 1;
}

void write_greedy_order(
   std::list<std::list<std::pair<int, double> > >& resultsOfPartialSPs,
   const std::list<std::string>& files, const std::string& format)
{
   if (format == "cout")
   {
      std::list<std::list<std::pair<int, double> > >::iterator lit;
      std::list<std::string>::const_iterator fcurr = files.begin();
      for (lit = resultsOfPartialSPs.begin(); lit != resultsOfPartialSPs.end(); lit++)
      {
         std::cout << "------------------------------------------------------------------------------" << std::endl;
         std::cout << "Greedy ordering of sensors: " << *fcurr << std::endl;
         fcurr++;
         std::cout << "------------------------------------------------------------------------------" << std::endl;
         std::list<std::pair<int, double> >& sensors = *lit;
         std::list<std::pair<int, double> >::iterator it;
         for (it = sensors.begin(); it != sensors.end(); it++)
         {
            pair<int, double> p = *it;
            std::cout << p.first << "\t" << fixed << setprecision(4)
                      << p.second << std::endl;
         }
      }
   }
   else if (format == "xls")
   {
      std::list<std::list<std::pair<int, double> > >::iterator lit;
      std::list<std::string>::const_iterator fcurr = files.begin();
      for (lit = resultsOfPartialSPs.begin(); lit != resultsOfPartialSPs.end(); lit++)
      {
         std::cout << "Greedy ordering of sensors: " << std::endl;
         fcurr++;
         std::list<std::pair<int, double> >& sensors = *lit;
         std::list<std::pair<int, double> >::iterator it;
         for (it = sensors.begin(); it != sensors.end(); it++)
         {
            pair<int, double> p = *it;
            std::cout << p.first << "\t" << fixed << setprecision(4)
                      << p.second << std::endl;
         }
      }
   }
}

void write_impact_statistics(int id, const utilib::BasicArray<int>& sensors,
                             utilib::BasicArray<std::map<int, ImpactStats> >& stats,
                             utilib::BasicArray<std::vector<std::map<int, double> > >& impacts,
                             const std::list<std::string>& files,
                             const std::string& format, bool flag, bool debug,
                             std::map<int, std::string>& nodemap,
                             std::map<std::string, double>& costs,
                             double gamma)
{
   if (format == "cout")
   {
      if (flag)
         std::cout << "------------------------------------------------------------------------------" << endl;
      std::cout << "Sensor placement id:        " << id << std::endl;
      std::cout << "Number of sensors:          " << sensors.size() << std::endl;
      std::cout << "Total cost:                 " << compute_cost(sensors, costs, nodemap) << std::endl;
      std::cout << "Sensor node IDs:            ";
      utilib::BasicArray<int>::const_iterator scurr = sensors.begin();
      utilib::BasicArray<int>::const_iterator send  = sensors.end();
      while (scurr != send)
      {
         std::cout << *scurr << " ";
         scurr++;
      }
      std::cout << std::endl;
      if (nodemap.size() > 0)
      {
         std::cout << "Sensor junctions:           ";
         utilib::BasicArray<int>::const_iterator scurr = sensors.begin();
         utilib::BasicArray<int>::const_iterator send  = sensors.end();
         while (scurr != send)
         {
            std::cout << nodemap[*scurr] << " ";
            scurr++;
         }
         std::cout << std::endl;
      }

      utilib::BasicArray<std::vector<std::map<int, double> > >::iterator icurr = impacts.begin();
      std::list<std::string>::const_iterator fcurr = files.begin();
      std::list<std::string>::const_iterator fend = files.end();
      utilib::BasicArray<std::map<int, ImpactStats> >::iterator curr = stats.begin();
      int percent = static_cast<int>(floor(gamma * 100));
      for (; fcurr != fend; fcurr++, curr++, icurr++)
      {
         ImpactStats& stat = (*curr)[id];
         std::cout << std::endl;
         std::cout << "Impact File:                " << *fcurr << std::endl;
         std::cout << "Number of events:           " << stat.nscenarios << std::endl;

         std::cout << "Min impact:                 " << fixed << setprecision(4) << stat.minImpact << std::endl;
         std::cout << "Mean impact:                " << fixed << setprecision(4) << stat.meanImpact << std::endl;
         std::cout << "Lower quartile impact:      " << fixed << setprecision(4) << stat.firstQImpact << std::endl;
         std::cout << "Median impact:              " << fixed << setprecision(4) << stat.medianImpact << std::endl;
         std::cout << "Upper quartile impact:      " << fixed << setprecision(4) << stat.thirdQImpact << std::endl;
         std::cout << "Value at Risk (VaR) (" << setw(3) << percent << "%): " << fixed << setprecision(4) << stat.VaR << std::endl;
         std::cout << "TCE                 (" << setw(3) << percent << "%): " << fixed << setprecision(4) << stat.TCE << std::endl;
         std::cout << "Max impact:                 " << fixed << setprecision(4) << stat.maxImpact << std::endl;

         if (debug)
         {
            std::cout << "Debugging Information" << std::endl;
            std::cout << "i witness[i] impact[i][witness[i]]" << std::endl;
            for (unsigned int i = 0; i < stat.witness.size(); i++)
            {
               std::cout << i << " " << stat.witness[i] << " " <<
                         (*icurr)[i][stat.witness[i]] << std::endl;
            }
         }
      }
      std::cout << "------------------------------------------------------------------------------" << endl;
   }

   else if (format == "xls-sensors")
   {
      if (nodemap.size() > 0)
      {
         if (flag)
         {
            std::cout << "Sensor ID\tNum Sensors\tSensor Placements" << std::endl;
         }
         std::cout << id << "\t";
         std::cout << sensors.size() << "\t";
         utilib::BasicArray<int>::const_iterator scurr = sensors.begin();
         utilib::BasicArray<int>::const_iterator send  = sensors.end();
         while (scurr != send)
         {
            std::cout << nodemap[*scurr] << "\t";
            scurr++;
         }
         std::cout << std::endl;
      }
   }

   else if (format == "xls-summary")
   {
      if (flag)
      {
         std::cout << "Sensor ID\tNum Sensors\tCost\tImpactFile\tNum Events\tMin\tMean\tLower Quartile\tMedian\tUpper Quartile\tVaR\tTCE\tMax" << endl;
      }
      std::list<std::string>::const_iterator fcurr = files.begin();
      std::list<std::string>::const_iterator fend = files.end();
      utilib::BasicArray<std::map<int, ImpactStats> >::iterator curr = stats.begin();
      for (; fcurr != fend; fcurr++, curr++)
      {
         ImpactStats& stat = (*curr)[id];
         std::cout << id << "\t" << sensors.size() << "\t" <<
                   compute_cost(sensors, costs, nodemap) << "\t" <<
                   *fcurr << "\t" <<
                   stat.nscenarios << "\t" <<
                   stat.minImpact << "\t" <<
                   stat.meanImpact << "\t" <<
                   stat.firstQImpact << "\t" <<
                   stat.medianImpact << "\t" <<
                   stat.thirdQImpact << "\t" <<
                   stat.VaR << "\t" <<
                   stat.TCE << "\t" <<
                   stat.maxImpact << std::endl;
      }
   }

}

void write_impact_statistics(map<int, utilib::BasicArray<int> >& sensors,
                             utilib::BasicArray<std::map<int, ImpactStats> >& stats,
                             utilib::BasicArray<std::vector<std::map<int, double> > >& impacts,
                             const std::list<std::string>& files,
                             const std::string& format, bool flag, bool debug,
                             std::map<int, std::string>& nodemap,
                             std::map<std::string, double>& costs,
                             double gamma)
{
   if (format == "xls-impact-distributions")
   {

      unsigned int ndx = 0;
      unsigned int done = 0;

      while (done < sensors.size()*files.size())
      {
         switch (ndx)
         {
         case 0:
         {
            map<int, utilib::BasicArray<int> >::iterator curr = sensors.begin();
            map<int, utilib::BasicArray<int> >::iterator end  = sensors.end();
            while (curr != end)
            {
               std::list<std::string>::const_iterator fcurr = files.begin();
               std::list<std::string>::const_iterator fend = files.end();
               while (fcurr != fend)
               {
                  cout << curr->first << "\t";
                  cout << curr->first << "\t";
                  cout << curr->first << "\t";
                  cout << curr->first << "\t";
                  cout << "\t";
                  fcurr++;
               }
               curr++;
            }
         }
         break;

         case 1:
         {
            map<int, utilib::BasicArray<int> >::iterator curr = sensors.begin();
            map<int, utilib::BasicArray<int> >::iterator end  = sensors.end();
            while (curr != end)
            {
               std::list<std::string>::const_iterator fcurr = files.begin();
               std::list<std::string>::const_iterator fend = files.end();
               while (fcurr != fend)
               {
                  cout << *fcurr << "\t";
                  cout << *fcurr << "\t";
                  cout << *fcurr << "\t";
                  cout << *fcurr << "\t";
                  cout << "\t";
                  fcurr++;
               }
               curr++;
            }
         }
         break;

         case 2:
         {
            map<int, utilib::BasicArray<int> >::iterator curr = sensors.begin();
            map<int, utilib::BasicArray<int> >::iterator end  = sensors.end();
            while (curr != end)
            {
               utilib::BasicArray<std::map<int, ImpactStats> >::iterator scurr = stats.begin();
               std::list<std::string>::const_iterator fcurr = files.begin();
               std::list<std::string>::const_iterator fend = files.end();
               while (fcurr != fend)
               {
                  cout << (*scurr)[curr->first].nscenarios << "\t";
                  cout << (*scurr)[curr->first].nscenarios << "\t";
                  cout << (*scurr)[curr->first].nscenarios << "\t";
                  cout << (*scurr)[curr->first].nscenarios << "\t";
                  cout << "\t";
                  scurr++;
                  fcurr++;
               }
               curr++;
            }
         }
         break;

         case 3:
         {
            map<int, utilib::BasicArray<int> >::iterator curr = sensors.begin();
            map<int, utilib::BasicArray<int> >::iterator end  = sensors.end();
            while (curr != end)
            {
               utilib::BasicArray<std::map<int, ImpactStats> >::iterator scurr = stats.begin();
               std::list<std::string>::const_iterator fcurr = files.begin();
               std::list<std::string>::const_iterator fend = files.end();
               while (fcurr != fend)
               {
                  cout << "Event ID\t";
                  cout << "Witness Location Index\t";
                  cout << "Witness Location ID\t";
                  cout << "Impact\t";
                  cout << "\t";
                  scurr++;
                  fcurr++;
               }
               curr++;
            }
         }
         break;

         default:
         {
            map<int, utilib::BasicArray<int> >::iterator curr = sensors.begin();
            map<int, utilib::BasicArray<int> >::iterator end  = sensors.end();
            while (curr != end)
            {
               utilib::BasicArray<std::map<int, ImpactStats> >::iterator scurr = stats.begin();
               utilib::BasicArray<std::map<int, ImpactStats> >::iterator send  = stats.end();
               while (scurr != send)
               {
                  if ((ndx - 4) < (*scurr)[curr->first].witness.size())
                  {
                     cout << (ndx - 3) << "\t";
                     cout << (*scurr)[curr->first].witness[ndx-4] << "\t";
                     cout << nodemap[(*scurr)[curr->first].witness[ndx-4]] << "\t";
                     cout << (*scurr)[curr->first].impact[ndx-4] << "\t";
                     cout << "\t";
                  }
                  else
                  {
                     cout << "\t";
                     cout << "\t";
                     cout << "\t";
                     cout << "\t";
                     cout << "\t";
                     if ((ndx - 4) == (*scurr)[curr->first].witness.size()) done++;
                  }
                  scurr++;
               }
               curr++;
            }
         }

         }
         cout << endl;
         ndx++;
      }
   }
}
