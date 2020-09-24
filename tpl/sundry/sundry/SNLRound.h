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
//#include <gmp.h>
//#include <strstream>

#define PREC 1024

using namespace std;

template <typename value>
value probAll(value *x, int n)
{
	value result = 1.0;
	for (int i=0; i<n; i++)
		result *= x[i];
	return result;
}

template <typename value>
value probNone(value *x, int n)
{
	value result = 1.0;
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
template <typename value_type>
void probSelectKofLastN(double **table, int k, int n, value_type* x)
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
				cerr  << "INCUMBENT ERROR: UNDERFLOW" 
				      << " entry (" << i << "," << j 
				      << "): " << table[i][j] <<  endl;
                }
        }
}

template <typename value_type>
void selectElements(double f, double **table, value_type*x, int ind, 
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

template <typename value_type>
double ** condProbTable(value_type* x, int n, int k)
{
	double **table = new double*[k+1];
	for (int i=0; i<=k; i++) {
		table[i] = new double[n];
		for (int j=0; j<n; j++) {
			table[i][j] = -1;
		}
	}

	probSelectKofLastN((double**)table,k,n,x);
	return table;
}
		
template <typename value_type>
vector<int> kRandomizedRounding(double **table, value_type* x, int n, int k)
{
	vector<int> result;
        double f=0.0;
#ifdef _WIN32
        f = (double)rand() / (double)RAND_MAX;
#else
	f = drand48();
#endif
	selectElements(f, table, x, n-1, k, result);
	return result;
}

template <typename value>
value * readSensorVars(int numSvars)
{
	int index;
	value _value;
	value *values = new value[numSvars];
	for (int i=0; i<numSvars; i++) {
		cin >> index >> _value;
		values[index-1] = _value;
	}
	return values;
}

template <typename value_type>
vector<int>  placeSensors(double **table,
			int numSvars,
			int numsensors,
			value_type *x)
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

template <typename value_type>
vector<vector<int> > multipleRandomizedSensorPlacements(value_type*x,int size, 
						 int num_sensors, 
						 int result_size)
{
	int num_to_place = num_sensors;
	vector<vector<int> > result(result_size);
  	list<int> indicesOfFractional;
	map<int,int> fractionalIndexToOriginalIndex;
	int numones=0;
	for (int i=0; i<result_size; i++) {
		result[i].resize(num_sensors);
		for (int j=0; j<num_sensors; j++) {
			result[i][j] = 0;
		}
	}
	int numfrac=0;
  	for (int i=0; i<size; i++) {
		// should deal with tolerances consistently
		if ((x[i] > 0.0+1e-7) && (x[i] < 1.0-1e-7)) {
			indicesOfFractional.push_back(i);
			fractionalIndexToOriginalIndex[numfrac++] = i;
		} else if (x[i] >= 1.0-1e-7) {
			for (int j=0; j<result_size; j++)
				result[j][num_to_place-1] = i;
			num_to_place--;
		}
  	}
  	value_type * fractional_vals = new value_type[numfrac];
  	int next=0;
  	for (list<int>::iterator it=indicesOfFractional.begin(); 
				it!=indicesOfFractional.end(); it++,next++) {
		fractional_vals[next] = x[*it];
  	}

	double **table = condProbTable(fractional_vals, numfrac, num_to_place);
	for (int i=0; i<result_size; i++) {
		vector<int> tmp_result;
		tmp_result = placeSensors(table, numfrac, num_to_place, 
					 fractional_vals);
  		list<int>::iterator it = indicesOfFractional.begin();
		int next=0;
		for(int j=0; j<tmp_result.size(); j++) {
			result[i][j] = 
			  fractionalIndexToOriginalIndex[tmp_result[j]];
		}
	}
	for (int i=0; i<=num_to_place; i++) {
		delete[] table[i];
	}
	delete[] table;
	delete[] fractional_vals;
	return result;
}

/*
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
  //srand48(3490);

  ofstream sfile, tfile;
  char fname[256], tfname[256];
  sprintf(fname,"snlround.%d.%d.%d.sensors", numSvars, numSensors, numIters);
  sfile.open(fname);
  double *x = readSensorVars(numSvars);
  double **table = 0;
  table = condProbTable(x, numSvars, numSensors);
  for (int i=0; i < numIters; i++)
    {
      sfile << i << " " << numSensors << " ";
      // do the k-randomized rounding
      vector<int> sensors =  placeSensors(table, numSvars, numSensors,
				          x);

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
*/
