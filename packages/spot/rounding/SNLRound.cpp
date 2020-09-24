/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the LGPL License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top software directory.
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

#define PREC 1024

using namespace std;

double probAll(double *x, int n)
{
	double result = 1.0;
	for (int i=0; i<n; i++)
		result *= x[i];
	return result;
}

double probNone(double *x, int n)
{
	double result = 1.0;
	for (int i=0; i<n; i++)
		result *= (1-x[i]);
	return result;
}

// *******************************************************************
// ** void probSelectKofLastN(double **table, int k, int n, double *x)
// **
// ** find the probability of selecting k elements from 0..n, given
// ** that the probability of selecting i is x[i].
// *******************************************************************
void probSelectKofLastN(double **table, int k, int n, double *x)
{
        double prod=1.0;
	long long int one=1;
	double mindenormalized = *(double*)&one;
                                                                                
        for (int i=1; i<=k; i++) {
                prod *= x[i-1];
                table[i][i-1] = prod;
        }
        prod = 1.0;
        for (int i=0; i<n; i++) {
                prod *= (1-x[i]);
                table[0][i] = prod;
        }
        for (int i=1; i<=k; i++) {
                for (int j=i; j<n; j++) {
                        table[i][j] = x[j]     * table[i-1][j-1] +
                                      (1-x[j]) * table[i][j-1];
			if (table[i][j]!=0.0 && fabs(table[i][j])<=
						2*mindenormalized)
				cout  << "INCUMBENT ERROR: UNDERFLOW" 
				      << " entry (" << i << "," << j 
				      << "): " << table[i][j] <<  endl;
                }
        }
}

void selectElements(double f, double **table, double *x, int ind, 
		    int leftToPick, vector<int>& result)
{
	if (leftToPick == 0) {
		return;
	}
	if (leftToPick == ind+1) {
		for (int i=0; i<=ind; i++)
			result.push_back(i);
		return;
	}
	//if (ind == 0) {
	//	if (f < table[leftToPick][ind])
	//		result.push_back(ind);
	//	return;
	//}

	double probPickKofThese = table[leftToPick][ind];
	double condProbPickThisOne = x[ind]*table[leftToPick-1][ind-1]
				     /probPickKofThese;
	double condProbDontPickThisOne=(1-x[ind])*table[leftToPick][ind-1]
				     /probPickKofThese;
	if (f < condProbPickThisOne) { // case 1 : pick element 'ind'
		result.push_back(ind);
		double next_f = f / condProbPickThisOne;
		if (ind > 0)
		   selectElements(next_f, table, x, ind-1, leftToPick-1, result);
	} else {	// case 2: done pick element 'ind'
		double next_f = (f - condProbPickThisOne)
				/ condProbDontPickThisOne;
		if (ind > 0)
		   selectElements(next_f, table, x, ind-1, leftToPick, result);
	}
}

double ** condProbTable(double *x, int n, int k)
{
	double **table = new double*[n];
	for (int i=0; i<n; i++) {
		table[i] = new double[n];
		for (int j=0; j<n; j++) {
			table[i][j] = -1;
		}
	}

	probSelectKofLastN((double**)table,k,n,x);
	return table;
}
		
vector<int> kRandomizedRounding(double **table, double *x, int n, int k)
{
	vector<int> result;
	double f = 0.5;   /*drand48();*/
	selectElements(f, table, x, n-1, k, result);
	return result;
}

double * readSensorVars(int numSvars)
{
	int index;
	double value;
	double *values = new double[numSvars];
	for (int i=0; i<numSvars; i++) {
		cin >> index >> value;
		values[index-1] = value;
	}
	return values;
}

vector<int>  placeSensors(double **table,
			int numSvars,
			int numsensors,
			double *x, double& rtime)
{
	// int vars are symmetric;  s_ij == s_ji
	vector<int> roundingresult=kRandomizedRounding(table, x, numSvars, 
						       numsensors);
	vector<int> result;
	for (unsigned int i=0; i<roundingresult.size(); i++) {
		int xind = roundingresult[i];
		result.push_back(xind);
	}
	return result;
}

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
  double *x = readSensorVars(numSvars);
  double **table = 0;
  table = condProbTable(x, numSvars, numSensors);
  double rtime = 0.0;
  for (int i=0; i < numIters; i++)
    {
      sfile << i << " " << numSensors << " ";
      // do the k-randomized rounding
      vector<int> sensors =  placeSensors(table, numSvars, numSensors,
				          x, rtime);

      for (int i=0; i<numSensors; i++) {
      		sfile << sensors[i]+1 << " ";
      }
      sfile << endl;
   }
   sfile.close();

   delete [] x;
   for (int i=0; i<numSvars; i++) 
	delete [] table[i];
   delete [] table;
   exit(0);
}
