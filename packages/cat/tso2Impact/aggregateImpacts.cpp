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
  * \file aggregateImpacts.cpp
  *
  * Read a configuration file and impact file(s).  Aggregate
  * locations into superlocations.  Write a new impact file
  * with superlocations instead of original locations.  Write
  * a new configuration file reflecting the superlocations.
  * Add to the end of the configuration file a mapping from
  * each superlocation to representative real locations.
  *
  * aggregateImpacts {configuration-file-name}
  *
  * Optional argument that follows configuration file name is
  * the maximum number of impacts to read at one time from an
  * impact file.  The default is to read all impacts in.
  *
  * aggregateImpacts test1_mc.impact 10000
  *
  * will read no more than 10000 impacts at a time (unless there
  * is an event with more than 10000 impacts, in which case it
  * reads all impacts in the event).
  */

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include <utilib/OptionParser.h>
#include <utilib/string_ops.h>
#include <sp/SPProblem.h>
#include "version.h"
#include <string>

using namespace std;


/// The main routine for setupIPData
int main(int argc, char **argv)
{
   try
   {
      int impactReadLimit = 0;

      utilib::OptionParser options;
      options.add_usage("aggregateImpacts [options...] <config-file>");
      options.description = "This command aggregates locations into superlocations.  It creates a new config file and new impact files that combines locations into superlocations, to reduce problem size. The new config file is: tmpname_agg.config .";
      std::string version = create_version("aggregateImpacts", __DATE__, __TIME__);
      options.version(version);

      options.add("read_limit", impactReadLimit, "Limit the number of impacts that are read in at a time.  If there is an event with more than this many impacts, then read in all impacts for that event.  By default, all impacts are read.");

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



      const char *configFile = args[1].c_str();
      ifstream ifstr(configFile);
      if (!ifstr)
         EXCEPTION_MNGR(runtime_error, "Bad filename " << configFile);

      SPProblem info;
      info.output_impact = true;

      if (impactReadLimit > 0)
      {
         info.SetMaximumNumberOfImpacts(impactReadLimit);
         info.SetReadAllImpacts(false);
      }
      else
      {
         info.SetReadAllImpacts(true);
      }

      info.read_config_file(ifstr, true, false, true);

      ifstr.close();

      info.compute_superlocations();

      // Write new impact files.     prob_goal.impact -> prob_goal_agg.impact

      std::vector<std::string> newImpacts(info.goals.size(), "");

      for (int i = 0; i < info.goals.size(); i++)
      {
         std::string newname = info.goals[i].impact_file;
         if (newname != "none")
         {
            int loc = newname.find_last_of(".");
            if (loc >= 0)
            {
               newname.insert(loc, "_agg");
            }
            else
            {
               newname.append("_agg");
            }

            ofstream ofImpact(newname.c_str());
            info.write_aggregated_impacts(ofImpact, i);
            ofImpact.close();
            std::cout << "New impact file: " << newname << std::endl;
         }
         newImpacts[i] = newname;
      }

      // Write a corresponding configuration file  prob.config -> prob_agg.config

      std::string newname(configFile);   // new config file
      std::string earliestname(configFile); // maps sl ID to earliest impact orig ID
      std::string mapname(configFile);   // maps sl ID to all orig IDs in sl

      int loc = newname.find_last_of(".");
      if (loc >= 0)
      {
         newname.insert(loc, "_agg");
         earliestname.insert(loc, "_earliest");
         mapname.insert(loc, "_map");
      }
      else
      {
         newname.append("_agg");
         earliestname.append("_earliest");
         mapname.append("_map");
      }

      ofstream of(newname.c_str());
      info.write_aggregated_config_file(of, newImpacts);
      of.close();

      // compute_superlocations wrote out a file with a full list of
      // real locations that are in each superlocation, and a file
      // with "earliest" ids for each event for each superlocation.

      rename("superlocations_first.txt", earliestname.c_str());
      rename("superlocations_all.txt", mapname.c_str());

      std::cout << "New configuration file: " << newname << std::endl;
      std::cout << "Worst impact original location IDs: " << earliestname << std::endl;
      std::cout << "Map SL IDs to original location IDs: " << mapname << std::endl;
   }
   STD_CATCH(;)

   return 0;
}
