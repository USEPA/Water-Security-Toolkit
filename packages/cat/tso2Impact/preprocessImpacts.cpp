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
  * \file preprocessImpact.cpp
  *
  * Processes an impact file and generates a smaller one.
  */

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include <utilib/ParameterSet.h>
#include <utilib/ParameterList.h>
#include <utilib/string_ops.h>
#include <sp/SPProblem.h>

using namespace std;


/// The main routine for setupIPData
int main(int argc, char **argv)
{
   try
   {

      int num_impacts = -1;
      int num_events = -1;

      utilib::ParameterSet params;
      params.create_parameter("num_impacts", num_impacts, "<int>", "-1",
                              "An upper bound on the number of impact values");
      params.create_parameter("num_events", num_events, "<int>", "-1",
                              "An upper bound on the number of events");

      utilib::ParameterList plist;
      plist.register_parameters(params);
      plist.process_parameters(argc, argv, 1);
      params.set_parameters(plist, false);

      if ((argc == 1) || params.get_parameter<bool>("help"))
      {
         ucout << "preprocessImpact [options] <config-file>" << endl;
         plist.write_registered_parameters(cout);
         utilib::CommonIO::end();
         return -1;
      }

      if (argc == 1)
         EXCEPTION_MNGR(runtime_error, "Missing sensor placement filename!");
      ifstream ifstr(argv[1]);
      if (!ifstr)
         EXCEPTION_MNGR(runtime_error, "Bad filename " << argv[1]);

//
// Read in the problem data
//
      SPProblem info;
      info.read_config_file(ifstr, true, true, false);
      ifstr.close();
//
// Write out an impact file
//
      for (int i = 0; i < info.goals.size(); i++)
      {
         if (info.goals[i].impact_file != "none")
         {
            utilib::BasicArray<utilib::CharString> tokens = utilib::split(info.goals[i].impact_file.c_str(), '.');
            int foo = tokens.size();
            utilib::BasicArray<utilib::CharString> basename_tok = utilib::split(tokens[foo-2], '_');
            basename_tok[basename_tok.size()-2] += "_tmp";
            tokens[foo-2] = utilib::join(basename_tok, '_');
            utilib::CharString newname = utilib::join(tokens, '.');
            ofstream ofstr(newname.data());
            info.write_impact_file(ofstr, i);
            ofstr.close();
         }
      }
   }
   STD_CATCH(;)

   return 0;
}
