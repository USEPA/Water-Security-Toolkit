/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the LGPL License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top software directory.
 *  _________________________________________________________________________
 */

// Author: Haixia Jia
//
// Created on June 1 2008.
// A derivative class from solution class.
// Construct the initial solution.

#ifndef INITIALSOLUTION_H
#define INITIALSOLUTION_H

#include<stdlib.h>
#include<stdio.h>
#include<iostream>
#include<vector>

#include "solution.h"

namespace NewSensorPlacement {
class InitialSolution{
  public:
    int getRand(int start, int end); // return a random number between start and end;
    // permutate the array only the first subsize elements;
    void permutateList(int* array, int length, int subsize);
    // calculate the sample size needed;
    int getSampleSize(double fraction, int numM, int numSol);
    void checkValid(int numSol, int numM);
    // find all the eligible candidates;
    int getCandList(ProblemSolution *s, int* candList, myInt1 constraints);
    // find the best facility for GreedySampling function;
  private:
    int findBestFac(ProblemSolution *s, int sampleSize, int numCand, int* candList);
    bool isSmaller(double value1, double value2, int num1, int num2);
    bool isEqual(double value1, double value2, int num1, int num2);
    // find the best facility for GreedyWorst function;
    void findBestList(ProblemSolution *s, std::vector<int> &bestFacList, 
                        myInt1 constraints);
  public:
    InitialSolution();
    ~InitialSolution();
    // greedy sampling for MeanSolution;
    void GreedySampling(ProblemSolution *s, int numSol, double fraction,  
                        myInt1 constraints);
    // greedy Sampling for other cases: worst, var, tce;
    void GreedyWorst(ProblemSolution *s, int numSol,   
                        myInt1 constraints);

};
};
#endif



