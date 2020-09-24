/*  _________________________________________________________________________
 *
 *  Acro: A Common Repository for Optimizers
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top Acro directory.
 *  _________________________________________________________________________
 */

#include <map>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <assert.h>
#include <string.h>
#include <gmp.h>
#include <strstream>
#include <sundry/SNLRound.h>

#define PREC 1024

using namespace std;

int main(int argc, char *argv[])
{
  if (argc < 4) {
	cerr << "usage: snlRound <numLocs> <numSensors> "
	     << "<numTrials>   <  <fract. sol. file>"
	     << endl << endl;
	cerr << "where the fractional solution file has ampl display format: " 
	     << endl;
	cerr << "<index> <frac> <index> <frac> ... <index> <frac>" << endl;
	cerr << "<index> <frac> <index> <frac> ... <index> <frac>" << endl;
	cerr << "..." << endl;
	cerr << "<index> <frac> <index> <frac> ... <index> <frac>" << endl;
	cerr << endl;
	cerr << "-------------------------------------------------" << endl;
	cerr << "The result is stored in snlround.<numLoc>."
                "<numSensors>.<numTrials>.sensors" << endl;
	exit(1);
  }
	
  int numSvars   = atoi(argv[1]);
  int numSensors = atoi(argv[2]);
  int numHeuristicTrials = atoi(argv[3]);
  vector<int> sensors;

  int numIters = numHeuristicTrials;
  /*srand48(3490);*/

  ofstream sfile, tfile;
  char fname[256], tfname[256];
  sprintf(fname,"snlround.%d.%d.%d.sensors", numSvars, numSensors, numIters);
  sfile.open(fname);
  double *x = readSensorVars<double>(numSvars);
  double **table = 0;


  vector<vector<int> > result = 
	multipleRandomizedSensorPlacements(x,numSvars,numSensors, 
					   numIters);
  for (int i=0; i < numIters; i++)
    {
      sfile << i << " " << numSensors << " ";
      // do the k-randomized rounding
      vector<int> sensors =  result[i];

      for (int i=0; i<numSensors; i++) {
      		sfile << sensors[i]+1 << " ";
      }
      sfile << endl;
   }
   sfile.close();

   delete [] x;
   for (int i=0; i<=numSensors; i++) 
	delete [] table[i];
   delete [] table;
   exit(0);
}
