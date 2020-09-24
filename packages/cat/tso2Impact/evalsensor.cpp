/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: A sensor placement utility for water systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the LGPL License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top SPOT directory.
 *  _________________________________________________________________________
 */

/*!
 * \file evalsensor.cpp
 * \brief Evaluate a sensor placement using impacts defined by a TSO file.
 */

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include "version.h"
#include <fstream>
#include <sp/impacts.h>
#include <utilib/OptionParser.h>
#include <utilib/BasicArray.h>
#include <vector>
#include <list>

void print_usage(utilib::OptionParser& options)
{ options.write(std::cerr); }



/// The main routine for the evalsensor executable.
int main(int argc, char **argv)
{
   try
   {
      //
      // Global data
      //
      bool debug = false;
      int delay = 0;
      double gamma = 0.05;
      std::string format = "cout";
      std::string weightsFilename;
      std::string nodemap_file;
      std::string costs_file;
      std::string sc_probabilities_file;
      std::string scs_file;
      bool all_locs_feasible = false;
      bool compute_greedy_ranking = false;
      //
      // Setup command-line options
      //
      utilib::OptionParser options;
      options.add_usage("evalsensor [options...] <sensor-file> <impact-file1> [<impact-file2>...]");
      options.add_usage("evalsensor [options...] none <impact-file1> [<impact-file2>...]");
      options.description = "A command to read in one or more sensor placements and summarize their performance according to various metrics.";

      options.add("debug", debug, "A boolean flag that adds output information about each incident.");
      options.add('r', "responseTime", delay, "This parameter indicates the number of minutes that are needed to respond to the detection of a continant.  As the response time increases, the impact increases because the contaminant affects the network for a greater length of time. Unit: minutes.");
      options.add("gamma", gamma, "The fraction of the tail distribution used to compute the VaR and TCE performance measures. (The default value is 0.05).");
      options.add("format", format, "The type of output that the evaluation will generate:\n\
 cout  Generates output that is easily read.\n\
        (default)\n\
 xls   Generates output that is easily\n\
        imported into a MS Excel spreadsheet.\n\
 xml   Generates an XML-formated output that\n\
        is used to communicate with the TEVA\n\
        GUI.  (Not currently supported.)");
      options.add("incident-weights", weightsFilename, "A file with the weights of the different contamination incidents.");
      options.add("nodemap", nodemap_file, "A file with the node map information, for translating node IDs into junction labels.");
      options.add("costs", costs_file, "A file with the cost information for each location id.");
      options.add("sc-probabilities", sc_probabilities_file, "A file with the probability of detection for each sensor category.");
      options.add("scs", scs_file, "A file with the sensor category information for each location id.");
      options.add("all-locs-feasible", all_locs_feasible, "A boolean flag that indicates that all locations are treated as feasible.");
      options.add("compute-greedy-ranking", compute_greedy_ranking, "A boolean flag. If true, also compute and print a greedy solution to the sensor placement problem.");

      options.add_argument("sensor-file", "A sensor placement file, which contains one or more sensor placements that will be evaluated.  If 'none' is specified, then evalsensor will evaluate impacts with no sensors.");
      options.add_argument("impact-file", "A file that contains the impact data concerning contamination incidents.  If one or more impact files are specified, then evaluations are performed for each impact separately.");

      options.epilog = "Note that options like 'responseTime' can be specified with the syntax '--responseTime 10.0' or '--responseTime=10.0'.";

      std::string version = create_version("evalsensor", __DATE__, __TIME__);
      options.version(version);
      //
      // Process command-line options
      //
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

      //
      // Check for errors in the command-line arguments
      //
      if (args.size() < 2)
      {
         options.write(std::cerr);
         return 1;
      }

      //
      // Read the incident weights file
      //
      std::map<int, double> weights;
      if (weightsFilename != "")
      {
         std::ifstream weightsInputFile(weightsFilename.c_str());
         read_incident_weights(weightsInputFile, weights);
      }
      //
      // Read the nodemap file
      //
      std::map<int, std::string> nodemap;
      if (nodemap_file != "")
      {
         std::ifstream ifstr(nodemap_file.c_str());
         read_nodemap(ifstr, nodemap);
      }
      //
      // Read the costs file
      //
      std::map<std::string, double> costs;
      if (costs_file != "")
      {
         if (nodemap.size() == 0)
            EXCEPTION_MNGR(std::runtime_error, "Cannot specify costs without a nodemap file");
         std::ifstream ifstr(costs_file.c_str());
         read_costs(ifstr, costs);
      }
      //
      // Read the sc probabilities
      //
      std::map<int, double> sc_probabilities;
      if (sc_probabilities_file != "")
      {
         std::ifstream ifstr(sc_probabilities_file.c_str());
         read_sc_probabilities(ifstr, sc_probabilities);
      }
      //
      // Read the scs
      //
      std::map<int, int> scs;
      if (scs_file != "")
      {
         if (sc_probabilities.size() == 0)
            EXCEPTION_MNGR(std::runtime_error,
                           "Cannot specify scs without an sc-probabilities file");
         std::ifstream ifstr(scs_file.c_str());
         read_scs(ifstr, scs);
      }

      //
      // Read sensor placement file
      //
      std::map<int, utilib::BasicArray<int> > sensors;
      utilib::OptionParser::args_t::iterator args_curr = args.begin();
      utilib::OptionParser::args_t::iterator args_end  = args.end();
      args_curr++;
      if (*args_curr == "none")
      {
         utilib::BasicArray<int> tmp;
         sensors[0] = tmp;
      }
      else
      {
         std::ifstream sensorInputFile(args_curr->c_str());
         if (!sensorInputFile)
         {
            std::cerr << "***Failed to open sensor file=" << *args_curr << std::endl;
            return 0;
         }
         read_sensor_placements(sensorInputFile, sensors);
         sensorInputFile.close();
      }
      args_curr++;
      //
      // Get impact filenames
      //
      std::list<std::string> files;
      while (args_curr != args_end)
      {
         files.push_back(*args_curr);
         args_curr++;
      }
      //
      // All points are feasible w.r.t this analysis
      //
      utilib::BitArray feasible_location(nodemap.size());
      if (nodemap.size() == 0)
         all_locs_feasible = true;
      if (!all_locs_feasible)
      {
         //if (nodemap.size() == 0)
         //  EXCEPTION_MNGR(std::runtime_error, "Cannot specify feasible locations without a nodemap file");
         std::map<int, utilib::BasicArray<int> >::iterator mcurr = sensors.begin();
         std::map<int, utilib::BasicArray<int> >::iterator mend  = sensors.end();
         while (mcurr != mend)
         {
            utilib::BasicArray<int> sids = mcurr->second;
            for (size_t si = 0; si < sids.size(); si++)
            {
               feasible_location.set(sids[si] - 1);
            }
            mcurr++;
         }
      }
      else
      {
         feasible_location.set();
      }
      std::map<int, int> reduced_repn_map;
      for (size_type i = 0; i < nodemap.size(); i++)
         reduced_repn_map[i+1] = i + 1;
      //
      // Read impact files and compute impact information
      //
      utilib::BasicArray<std::map<int, ImpactStats> > statistics(files.size());
      utilib::BasicArray<std::vector<std::map<int, double> > > delayed_impacts(files.size());
      std::list<std::list<std::pair<int, double> > > greedySensorOrders;
      //
      // stores <witness-index,detection-time> for each incident, sorted in order
      // of detection time (values are original detection time, not accounting for any
      // user-specified delay). only currently used in the computation of impacts under
      // the imperfect sensor model.
      //
      utilib::BasicArray<std::vector<std::vector<std::pair<int, double> > > > arrival_sequences(files.size());
      utilib::BasicArray<std::map<int, ImpactStats> >::iterator scurr = statistics.begin();
      std::list<std::string>::iterator fcurr = files.begin();
      std::list<std::string>::iterator fend = files.end();
      utilib::BasicArray<int> work;
      utilib::BasicArray<std::vector<std::map<int, double> > >::iterator dicurr = delayed_impacts.begin();
      utilib::BasicArray<std::vector<std::vector<std::pair<int, double> > > >::iterator ascurr = arrival_sequences.begin();
      std::map<int, double> impactMitigated;
      for (; fcurr != fend; fcurr++, scurr++, dicurr++, ascurr++)
      {
         //
         // Read the data from the impact file
         //        impacts[i] is a hash table of (node-id,impact) pairs
         //
         std::ifstream impactInputFile(fcurr->c_str());
         if (!impactInputFile)
         {
            std::cerr << "***Failed to open impact file=" << *fcurr << std::endl;
            return 0;
         }
         // TNT: Only load arrival_sequence data if we need it.
         if (scs.size() > 0)
         {
            read_impacts(impactInputFile, *dicurr, &(*ascurr), delay, NULL, feasible_location, reduced_repn_map);
         }
         else
         {
            read_impacts(impactInputFile, *dicurr, NULL, delay, NULL, feasible_location, reduced_repn_map);
         }
         impactInputFile.close();

         // Through a quirk in the design of sp, each impact file may have
         // an associated file of incident weights.  These are typically
         // redundant across different impacts (mc, vc, etc.).
         std::map<int, double> weights;
         std::string probFilename = *fcurr + ".prob";
         std::ifstream weightsInputFile(probFilename.c_str());
         if (weightsInputFile)
         {
            std::cout << "# Using weights file: " << *fcurr + ".prob" << std::endl;
            read_incident_weights(weightsInputFile, weights);
         }
         else
         {
            // WEH - disabling this warning, since this is unlikely.
            //std::cout << "# No weights file: " << *fcurr + ".prob" << std::endl;
         }
         //
         // Compute impact statistics
         //
         std::map<int, utilib::BasicArray<int> >::iterator mcurr = sensors.begin();
         std::map<int, utilib::BasicArray<int> >::iterator mend  = sensors.end();
         while (mcurr != mend)
         {
            ImpactStats tmp;
            compute_statistics(mcurr->second, *dicurr, *ascurr,
                               scs, sc_probabilities, tmp, weights, work, gamma);
            if (sensors.size() == 1 && compute_greedy_ranking)
            {
               compute_greedy_order(mcurr->second, *dicurr, *ascurr,
                                    scs, sc_probabilities, tmp, weights, work,
                                    greedySensorOrders, gamma);
            }
            (*scurr)[mcurr->first] = tmp;
            mcurr++;
         }
      }
      //
      // Write out impact statistics
      //
      if (format == "cout")
      {
         std::map<int, utilib::BasicArray<int> >::iterator mcurr = sensors.begin();
         std::map<int, utilib::BasicArray<int> >::iterator mend  = sensors.end();
         bool flag = true;
         while (mcurr != mend)
         {
            write_impact_statistics(mcurr->first, mcurr->second,
                                    statistics, delayed_impacts,
                                    files, format, flag, debug, nodemap, costs, gamma);
            if (sensors.size() == 1 && compute_greedy_ranking)
            {
               write_greedy_order(greedySensorOrders, files, format);
            }
            flag = false;
            mcurr++;
         }
      }

      else if (format == "xls")
      {
         std::map<int, utilib::BasicArray<int> >::iterator mcurr;
         std::map<int, utilib::BasicArray<int> >::iterator mend;
         bool flag;

         mcurr = sensors.begin();
         mend  = sensors.end();
         flag = true;
         while (mcurr != mend)
         {
            write_impact_statistics(mcurr->first, mcurr->second,
                                    statistics, delayed_impacts,
                                    files, "xls-sensors", flag, debug, nodemap, costs, gamma);
            flag = false;
            mcurr++;
         }

         mcurr = sensors.begin();
         mend  = sensors.end();
         flag = true;
         while (mcurr != mend)
         {
            write_impact_statistics(mcurr->first, mcurr->second,
                                    statistics, delayed_impacts,
                                    files, "xls-summary", flag, debug, nodemap, costs, gamma);
            flag = false;
            mcurr++;
         }

         write_impact_statistics(sensors, statistics, delayed_impacts,
                                 files, "xls-impact-distributions", flag, debug, nodemap, costs, gamma);
         if (sensors.size() == 1)
         {
            write_greedy_order(greedySensorOrders, files, format);
         }
      }
   }
   STD_CATCH(;)

   return 0;
}

