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

#ifndef tevasp_ObjectiveAggr_h
#define tevasp_ObjectiveAggr_h

#ifdef _MSC_VER
#include <algorithm>
#endif
#include <fstream>

#include <sp/impacts.h>
#include <sp/ObjectiveVisitor.h>

class scenario_group
{
public:
   // no need to store event indices explicitly;
   // these objects will be stored in a vector, and the index will be
   // the group id.  Events will map to group id's.

   // we need to know the maximum witness list length for any event
   // in this group
   scenario_group() : max_length(0), num_events(0), event_prob(0.0),
         hit_sequence(), hit_times(), impacts() {}
   scenario_group(const scenario_group& sg) :
         max_length(sg.max_length),
         num_events(sg.num_events),
         event_prob(sg.event_prob),
         hit_sequence(sg.hit_sequence),
         hit_times(sg.hit_times),
         impacts(sg.impacts) {}
   int max_length;
   int num_events;
   double event_prob;
   std::vector<int> hit_sequence;
   std::vector<int> hit_times;   // correspond to those of a max length
   // event in this aggregated event
   std::vector<double> impacts;
};

class ScenarioAggrVisitor
{
public:
   ScenarioAggrVisitor(vector<int>& k, vector<int>& sp,
                       std::vector<scenario_group>& grps,
                       std::map<int, int>& group_mp)
         : spine_scenarios(sp), groups(grps),
         group_map(group_mp) {}

   void discover(VecTrieNode<int, vector<int> >* n, int nextchar)
   {
      key.push_back(nextchar);
      if (n->hasdata())
      {
         vector<int>& v = n->getdata();
         for (int i = 0; i < v.size(); i++)
         {
            spine_scenarios.push_back(v[i]);
         }
      }
   }
   void finish(VecTrieNode<int, vector<int> >* n)
   {
      if (n->hasdata() && (spine_scenarios.size() > 0))
      {
         outputGroup();
      }
      key.pop_back();
   }

   void outputGroup()
   {
      scenario_group sg;
      int group_index = groups.size();
      sg.max_length = key.size() + 1;
      cout << "outputGroup: setting ml[" << group_index << "] to " << sg.max_length << endl;
      sg.num_events = spine_scenarios.size();
      sg.hit_sequence.reserve(sg.max_length);
      sg.hit_times.reserve(sg.max_length);
      sg.impacts.reserve(sg.max_length);
      for (int i = 0; i < sg.max_length; i++)
      {
         sg.hit_sequence.push_back(0);
         sg.hit_times.push_back(0);
         sg.impacts.push_back(0.0);
      }
      std::cout << "TrieNode: ";
      for (int i = 0; i < key.size(); i++)
      {
         std::cout << key[i] << " ";
      }
      std::cout << std::endl;
      std::cout << "Aggregated Scenario Indices: "
                << std::endl;
      for (int j = 0; j < spine_scenarios.size(); j++)
      {
         int event_ind = spine_scenarios[j];
         group_map[event_ind] = group_index;
         std::cout << spine_scenarios[j] << " ";
      }
      std::cout << std::endl;
      groups.push_back(sg);
      spine_scenarios.clear();
   }

private:
   vector<int> key;
   vector<int>& spine_scenarios;
   std::vector<scenario_group>& groups;
   std::map<int, int>& group_map;
};

class DepthFindingScenarioAggrVisitor
{
public:
   DepthFindingScenarioAggrVisitor(
      std::map<VecTrieNode<int, vector<int> >*, int>& m)
         : next(NULL), depthmap(m), maxDepthToLeaf(0) {}
   DepthFindingScenarioAggrVisitor(const DepthFindingScenarioAggrVisitor&v)
         : depthmap(v.depthmap),
         next((DepthFindingScenarioAggrVisitor*) &v),
         maxDepthToLeaf(0) { }

   void discover(VecTrieNode<int, vector<int> >* n, int nextchar)
   {
      if (n->numlinks() == 0)
      {
         DepthFindingScenarioAggrVisitor *tmp = next;
         int curdepth = 1;
         while (tmp && curdepth > tmp->maxDepthToLeaf)
         {
            tmp->maxDepthToLeaf = curdepth;
            curdepth++;
            tmp = tmp->next;
         }
      }
   }
   void finish(VecTrieNode<int, vector<int> >* n)
   {
      depthmap[n] = maxDepthToLeaf;
   }
private:
   int maxDepthToLeaf;
   DepthFindingScenarioAggrVisitor *next;
   std::map<VecTrieNode<int, vector<int> >*, int>& depthmap;
};


class AggrObjectiveVisitor : public ObjectiveVisitor
{
public:

   VecTrie<int, std::vector<int> >* theScenarioAggrTrie;

   void setScenarioAggrTrie(VecTrie<int, std::vector<int> >* theTrie)
   { theScenarioAggrTrie = theTrie; }

   void outputScenarioImpactData(std::list<Impact>& myImpacts, int scenarioIndex)
   {
      vector<int> hit_list;
      list<Impact>::iterator curr = myImpacts.begin();
      list<Impact>::iterator end  = myImpacts.end();
      while (curr != end)
      {
         hit_list.push_back(curr->nodeID);
         curr++;
      }
      VecTrieKey<int> key(hit_list);
      VecTrieNode<int, vector<int> >* n = theScenarioAggrTrie->getnode(key);
      if (!n)
      {
         std::vector<int> scenario_list;
         theScenarioAggrTrie->insert(key, scenario_list);
      }
      else if (!n->hasdata())
      {
         std::vector<int> scenario_list;
         n->setdata(scenario_list);
      }
      std::vector<int>& scenario_list = theScenarioAggrTrie->getdata(key);
      scenario_list.push_back(scenarioIndex);
   }

   void finalize()
   {
      vector<int> key;
      vector<int> spine_scenarios;
      int num_groups = 0;
      std::vector<scenario_group> groups;
      std::map<int, int> group_map;
      ScenarioAggrVisitor vis(key, spine_scenarios, groups, group_map);
      theScenarioAggrTrie->dfs(vis);
      write_aggregated_impact_file(groups, group_map);
   }

   void write_aggregated_impact_file(std::vector<scenario_group>& groups,
                                     std::map<int, int>& group_map)
   {
      std::ifstream imp_f(impactOutputFileName.c_str());

      int eventIndex;
      int nnodes;
      int num_events      = group_map.size();
      int num_aggr_events = groups.size();
      int delayIndex = read_impact_preamble(imp_f, nnodes, 0);
      imp_f >> eventIndex;
      for (int i = 0; i < num_events; i++)
      {
         std::map<int, double> next_event_impacts;
         std::vector<std::pair<int, int> > next_event_sequence;
         int cur_event = eventIndex;
         eventIndex = read_next_impact(imp_f, eventIndex,
                                       next_event_impacts,
                                       next_event_sequence, delayIndex);
         int num_impacts = next_event_impacts.size();
         int group_num = group_map[cur_event];
         for (int j = 0; j < groups[group_num].max_length; j++)
         {
            int id = -1;
            if (j < next_event_impacts.size())
            {
               id = next_event_sequence[j].first;
            }
            groups[group_num].impacts[j] +=
               next_event_impacts[id];
         }
         if (next_event_impacts.size() == groups[group_num].max_length)
         {
            for (int j = 0; j < groups[group_num].max_length; j++)
            {
               groups[group_num].hit_sequence[j] =
                  next_event_sequence[j].first;
               groups[group_num].hit_times[j] =
                  next_event_sequence[j].second;
            }
         }
      }
      std::string agImpactOutputFileName = "aggr" + impactOutputFileName;
      std::ofstream a_imp_f(agImpactOutputFileName.c_str());
      std::string agProbOutputFileName = impactOutputFileName + ".prob";
      std::ofstream a_prob_f(agProbOutputFileName.c_str());

      a_imp_f << nnodes << endl;
      a_imp_f << "1 0" << endl;   // only support delay of 0 in current
      // impact format; delays implemented in
      // setupIPData, randomsample
      for (int i = 0; i < num_aggr_events; i++)
      {
         // assuming uniform original event probabilities for now
         groups[i].event_prob = groups[i].num_events / (double)num_events;
         for (int j = 0; j < groups[i].max_length; j++)
         {
            groups[i].impacts[j] /= groups[i].num_events;
         }
         for (int j = 0; j < groups[i].max_length; j++)
         {
            int hitnode = groups[i].hit_sequence[j];
            int hittime = groups[i].hit_times[j];
            a_imp_f << i + 1 << " " << hitnode
            << " " << hittime
            << " " << groups[i].impacts[j] << std::endl;
         }
         a_prob_f << i + 1 << " " << groups[i].event_prob << endl;
      }
      a_imp_f.close();
      a_prob_f.close();
   }

};

#endif
