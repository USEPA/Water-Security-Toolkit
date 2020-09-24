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
// Build the problem, and solve it.


#ifndef PROBLEMSOLVER_H
#define PROBLEMSOLVER_H

//#define SIDECONSTRAINT
#define IMPERFECT

#include <iostream>
#include <fstream>
#include <iomanip>

#include <algorithm>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>

#include "solution.h"
#include "MeanSolution.h"
#include "WorstSolution.h"
#include "VarSolution.h"
#include "TceSolution.h"
#include "InitialSolution.h"
#include "ProblemInstance.h"
#include "SideConstraintInstance.h"
#include "ImperfectInstance.h"

#include <sp/SPProblem.h>

#include <utilib/seconds.h>
#include <utilib/BitArray.h>
#include <utilib/BasicArray.h>

namespace NewSensorPlacement {
class ProblemSolver { 
  private:
    int inputMode;
    static const int STATUS_INTERVAL=1000;
    int argc;
    char** argv;
    double CPUSeconds_start, runtime_start, WallClockSeconds_start, WallClockIter_start;
    double minCost, sumCosts, maxCost, localOptimumCost, initialCost;
    double gamma, timeLimit;
    int numFacilitiesToOpen, impactFileRepresentation, numSamples, randomSeed;
    std::string impactFileName, objective, statistic;
    utilib::BasicArray<int> fixeds, invalids;
    std::vector<int> locations;
    myInt1 facilityLocationConstraints;
    std::vector<std::string> sideConstraintObjectives;
    std::vector<std::string> sideConstraintImpactFiles;
    std::vector<ProblemSolution*> sideConstraintSolutions;
    std::vector<std::string> sideConstraintStatistics;
    std::vector<double> sideConstraintUpperBounds;

    std::vector<std::map<int, double> > nodeArrivalTime;
    std::vector<std::vector<std::pair<int, double> > > nodeArrivalSequence;
    std::vector<double> detectionProbability;
    std::map<int,double> rawDetectionProbability;
    std::map<int,int> locationCategory;
    std::ofstream jsonOutputFile;

    SideConstraintInstance * mySideIns; 
    ImperfectInstance * myImperfect; 
    ProblemSolution *bestSolution;
    ProblemSolution *initialSolution; 
    SPProblem theProblem;
    ProblemInstance *instance;
    std::vector<ProblemInstance*> sideConstraintInstances; 
    void initialTime(); // initial start time;
    void checkArgc(); // check is the input argc is legal;
    void checkImperfectArgc(); // check is the input argc is legal;
    void readConfig(); // read config file;
    void setParameter(); // set parameters: numFacilities to open, gamma...
    void setParamForSideConst(); // set parameters: numFacilities to open, gamma...
    void validParameter(); // check if the parameter is legal;
    void preLoadImpactFile(); // prepare to load impact file;
    void loadImpactFile(); // read impact;
    void loadSideFile(); // read impact;
    void loadFixs(); // load fixeds;
    void loadInvalids(); // load invalids;
    void loadProbability();
    void loadLocationSensorMap();
    void loadProbabilityComp();
    void getNodeArrival();
    void loadAlgParam(); //set parameters: numSamples, random seed, time limit;
    void setConstraints(); // set facility locaiton constraints;
    void validSideConst();
    void setSolutionType(); // set solution type, either mean, worst, var or tce;
    void loadAll(); // load everything by using all the above helper functions;
    void setSearchStart(); // before search start, set min cost, max cost sum cost ...
    void initializeSolution(); // initially construct some solution to start with;
    void getLocation(ProblemSolution* theSolution); // helper for printResult, and printBestToFile;
    void printResult(int i); // print out the result;
    void updateBestSol(); // update best solution to out file;
    void printBestToFile(); // print best solution to out file;
    void updateCost(); // update cost, sum, max. 
    void printFinalStat(); // print final statistics;
    
  public:
    
    ProblemSolver(int argc, char **argv);
    ProblemSolver(int argc, char **argv, int);
    ~ProblemSolver();
    int runProcess(); // run the whole process of search: load input data file, initial solutions
                       // do the search, print out the best solution, print out statistics.

};
};
#endif


