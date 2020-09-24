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
  * \file setupIPData.cpp
  *
  * Creates an input file for the GeneralSP.mod IP model.
  */

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include <utilib/OptionParser.h>
#include <sp/ObjectiveAggr.h>
#include <sp/SPProblem.h>
#include "version.h"

using namespace std;

string dirname(string fullpath, char delimiter = '/')
{
   int len = fullpath.length();
   int pos = len - 1;
   const char *str = fullpath.c_str();
   string::reverse_iterator rit(fullpath.rbegin());
   while (rit++ != fullpath.rend() && str[pos] != delimiter)
      pos--;
   pos = pos + 1;
   return fullpath.substr(0, pos);
}

string basename(string fullpath, char delimiter = '/')
{
   int len = fullpath.length();
   int pos = len - 1;
   const char *str = fullpath.c_str();
   string::reverse_iterator rit(fullpath.rbegin());
   while (rit++ != fullpath.rend() && str[pos] != delimiter)
      pos--;
   pos = pos + 1;
   return fullpath.substr(pos, len - pos);
}

void write_aggregated_impact_file(std::string impactInputFileName,
                                  std::string impactOutputFileName,
                                  std::vector<scenario_group>& groups,
                                  std::map<int, int>& group_map)
{
   int eventIndex, nnodes;
   int num_events      = group_map.size();
   int nevents2;
   int num_aggr_events = groups.size();
   std::ifstream in_imp_f(impactInputFileName.c_str());
   int delayIndex = read_impact_preamble(in_imp_f, nnodes, 0);
   in_imp_f >> eventIndex;
   for (int i = 0; i < num_events; i++)
   {
      std::map<int, double> next_event_impacts;
      std::vector<std::pair<int, int> > next_event_sequence;
      int cur_index = eventIndex;
      eventIndex = read_next_impact(in_imp_f, eventIndex,
                                    next_event_impacts,
                                    next_event_sequence, delayIndex);
      int num_impacts = next_event_impacts.size();
      int group_num = group_map[cur_index];
      cout << "groups[" << group_num << "].ml : " <<
           groups[group_num].max_length << endl;
      for (int j = 0; j < groups[group_num].max_length; j++)
      {
         int id = -1;
         if (j < next_event_impacts.size())
         {
            id = next_event_sequence[j].first;
         }
         if (group_num == 2)
         {
         }
         groups[group_num].impacts[j] +=
            next_event_impacts[id];
      }
      if (group_num == 2)
      {
         cout << "candidate longest event " << i << endl;
         cout << "length: " << next_event_impacts.size() << endl;
         cout << "group " << group_num << " max length: " << groups[group_num].max_length
              << endl;
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
   std::string impact_dirname = dirname(impactInputFileName);
   std::string impact_basename = basename(impactInputFileName);
   std::string agImpactOutputFileName;
   if (impactOutputFileName == "")
      agImpactOutputFileName = impact_dirname + "aggr" + impact_basename;
   else
      agImpactOutputFileName = impactOutputFileName;
   std::string agProbOutputFileName = agImpactOutputFileName + ".prob";
   std::ofstream a_imp_f(agImpactOutputFileName.c_str());
   std::ofstream a_prob_f(agProbOutputFileName.c_str());

   a_imp_f << nnodes << endl;
   a_imp_f << "1 0" << endl;  // only support delay of 0 in current
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

/// The main routine for setupIPData
int main(int argc, char **argv)
{
   try
   {

      string impactInputFile;
      string impactOutputFile;
      int numEvents = 0;

      utilib::OptionParser options;
      options.add_usage("scenarioAggr [options...] <impact-file>");
      options.description = "A command to aggregate incidents in an impact file.";
      std::string version = create_version("scenarioAggr", __DATE__, __TIME__);
      options.version(version);

      options.add("numEvents", numEvents, "The number of events before scenario aggregation.");
      options.add("out", impactOutputFile, "The output impact filename.");

      utilib::OptionParser::args_t args = options.parse_args(argc, argv);
      if (options.help_option())
      {
         options.write(std::cout);
         exit(1);
      }
      if (options.version_option())
      {
         options.print_version(std::cout);
         exit(1);
      }
      if (args.size() < 2)
      {
         options.write(std::cerr);
         return 1;
      }

      ifstream in_imp_f(args[1].c_str());
      if (!in_imp_f)
         EXCEPTION_MNGR(runtime_error, "Bad filename " << args[1]);

      SPProblem info;
      VecTrie<int, std::vector<int> > theScenarioAggrTrie;
//
// Read in the problem data
//
      impactInputFile = args[1];

      int eventIndex;
      int nnodes;
      int delayIndex = read_impact_preamble(in_imp_f, nnodes, 0);
      in_imp_f >> eventIndex;

      for (int i = 0; i < numEvents; i++)
      {
         std::map<int, double> next_event_impacts;
         std::vector<std::pair<int, int> > next_event_sequence;
         int cur_event = eventIndex;
         eventIndex = read_next_impact(in_imp_f, eventIndex,
                                       next_event_impacts,
                                       next_event_sequence, delayIndex);
         int num_impacts = next_event_impacts.size();
         vector<int> hit_list;
         for (int i = 0; i < int(next_event_impacts.size() - 1); i++)
         {
            // don't include dummy
            hit_list.push_back(next_event_sequence[i].first);
         }
         VecTrieKey<int> key(hit_list);
         VecTrieNode<int, vector<int> >* n = theScenarioAggrTrie.getnode(key);
         if (!n)
         {
            std::vector<int> scenario_list;
            theScenarioAggrTrie.insert(key, scenario_list);
         }
         else if (!n->hasdata())
         {
            std::vector<int> scenario_list;
            n->setdata(scenario_list);
         }
         std::vector<int>& scenario_list = theScenarioAggrTrie.getdata(key);
         scenario_list.push_back(cur_event);
      }
      in_imp_f.close();

      std::vector<int> key;
      std::vector<int> spine_scenarios;
      std::vector<scenario_group> groups;
      int num_groups = 0;

      std::map<int, int> group_map;
      ScenarioAggrVisitor vis(key, spine_scenarios, groups, group_map);
      theScenarioAggrTrie.dfs(vis);
      write_aggregated_impact_file(impactInputFile, impactOutputFile, groups, group_map);
   }
   STD_CATCH(;)

   return 0;
}
