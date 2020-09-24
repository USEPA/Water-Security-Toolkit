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

#include <utilib/ParameterSet.h>
#include <utilib/ParameterList.h>
#include <sp/SPProblem.h>

using namespace std;

/// The main routine for setupIPData
int main(int argc, char **argv)
{
   try
   {

      string dataOutputFile;
      bool lagrangianOutput = true;
      double gamma = 0.05;
      utilib::ParameterSet params;
      bool imperfect = false; // will be a parameter

      params.create_parameter("gamma", gamma, "<double>", "0.05",
                              "The fraction of the tail used to compute VaR and TCE statistics");
      params.create_parameter("output", dataOutputFile, "<string>","<null>",
         "The output filename.  By default, output goes to stdout.");
      params.create_parameter("lagrangian",lagrangianOutput, "<bool>", "false",
         "Option to produce output for Lagrangian solver instead of for AMPL.");
      /*
      params.create_parameter("costs",costFile, "<string>","<null>",
         "The cost file.");
      params.create_parameter("lock-sensors",fixedSensorFile, "<string>","<null>",
         "A file with the locations of pre-fixed sensor locations.");
      params.create_parameter("infeasible",infeasibleLocationsFile, "<string>","<null>",
         "A file with the locations that are infeasible for sensors.");
      params.create_parameter("",infeasibleLocationsFile, "<string>","<null>",
         "A file with the locations that are infeasible for sensors.");
      */

      utilib::ParameterList plist;
      plist.register_parameters(params);
      plist.process_parameters(argc, argv, 1);
      params.set_parameters(plist, false);

      if (argc == 1)
         EXCEPTION_MNGR(runtime_error, "Missing sensor placement filename!");
      ifstream ifstr(argv[1]);
      if (!ifstr)
         EXCEPTION_MNGR(runtime_error, "Bad filename " << argv[1]);

      SPProblem info;
      info.gamma = gamma;

// This says read entire impact file into memory (the default).  The
// EventImpacts object of the SPGoalInfo object is capable of reading
// the impact files in from disk in chunks, each chunk replacing the
// previous one.  But we can't use that capability here because the
// output functions below require all of the impacts in core.
// info.SetReadAllImpacts(false).)

      info.SetReadAllImpacts(true);

//
// Read in the problem data
//
      info.read_config_file(ifstr);

// For now, lagrangian output is always done to files.  The cout version
// is harder to support when we might have goal constraints.
      if (lagrangianOutput)
      {
         info.write_lagrangian_data(imperfect, dataOutputFile);
      }
      else
      {
         //
         // Compute super location information
         //
         info.compute_superlocations();
         //
         // Write the AMPL *.dat file
         //
         if (dataOutputFile != "")
         {
            ofstream ofstr(dataOutputFile.c_str());
            info.write_ampl_data(ofstr, imperfect);
         }
         else
            info.write_ampl_data(cout, imperfect);
      }
   }
   STD_CATCH(;)

   return 0;
}
