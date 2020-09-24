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
// Created on June 24 2008.
// Used to handle sideconstraint problems. 


#ifndef SIDECONSTRAINTINSTANCE_H
#define SIDECONSTRAINTINSTANCE_H

#include <vector>
#include "solution.h"
#include "MeanSolution.h"
#include "WorstSolution.h"
#include "VarSolution.h"
#include "TceSolution.h"
#include "InitialSolution.h"
#include "ProblemInstance.h"

namespace NewSensorPlacement {
class SideConstraintInstance {
  private:
    ProblemInstance *instance;  // main object ProblemInstance.
    InitialSolution initHelper; // Use some of its function for initial solutions.
    std::string statistics; // Main objective statistics: mean or worst...
    myInt1& facilityLocationConstraints; 
    double gamma;
    // side constraint problem statistics.
    std::vector<std::string>& sideConstraintStatistics; 
    // side constraint problem instances.
    std::vector<ProblemInstance*>& sideConstraintInstances; 
    // side constraint bounds.
    std::vector<double>& sideConstraintBounds;
    // solution to generate for main object.
    ProblemSolution* mySolution;
    // solutions for side constraint instances.
    std::vector<ProblemSolution*>& sideConstraintSolutions;
    // side constraint cost;
    std::vector<double> sideCost;
    // side constraint maximal cost.
    std::vector<double> sideMax;
    // object problem maximal cost.
    double ObjectMax;
    // initial cost.
    double initialValue;
    // weight for side constraints.
    double weight;
    int numN;
    int numM;
    int numP;
    // number of side constraints.
    int numSideConst;
    // reset all solutions to empty;
    void resetSolution();
    // get max cost initially.
    void initialMaxCost();
    // used for initialize solutions.
    int findBestFac(int sampleSize, int numCand, int* candList);
    // get cost for open one facility for initializing solutions;
    // not add object cost, only sideconstraints cost.
    double getInitCost(int facToOpen);
    // get total cost for main object and all the side constraints.
    double getCost();
    // get normalized cost for side constraints num.
    double normalizeCost(int num);
    // get normalized cost for side constraints num, with temValue as real cost.
    double normalizeCost(int num, double temValue);
    // close one fac, update main object and all side constraints:
    // if it's mean, don't do any thing, otherwise, close it for true.
    void closeOneFac(int facilityToClose);
    void openOneFac(int facilityToClose);
    // given a problem instance, get its cost with one facility open and 
    // one facility close.
    double getOneCost(int facilityToClose, int facilityToOpen,
                      ProblemSolution* theSolution,
                      std::string theStatistics,
                      double currentCost);
    // Get total cost of main object and side constraints with a pair of
    // facilities swapped.
   double getAllCost(int facilityToClose, int facilityToOpen,
                     std::vector<double> &currentCosts);
    // Find the best pair of facilities to switch.
    bool findBestPair(int& bestInsert, int& bestRemove, bool steepest);
    // reset its preProcess table before search start, only needed for "mean".
    void resetValue(ProblemSolution* theSolution, 
                    std::string theStatistics);
    void resetValue();
    // update PreProcess table for "mean" after a pair swap.
    void updateMeanTable(int facToInsert, int facToRemove,
                         ProblemSolution* theSolution,
                         std::string theStatistics);
    void updateMeanTable(int facToInsert, int facToRemove);
    // swap a pair of facilities for main object and side constraints.
    void swapAll(int facToInsert, int facToRemove);
    // update table 2.
    void updateMeanTable2(int facToInsert, int facToRemove);
    
  public:	

    SideConstraintInstance(
                           ProblemInstance *inst,
                           ProblemSolution *sol,
                           std::string stat,
                           myInt1& constraints,
                           double _gamma,
                           std::vector<std::string>& sideStatistics,
                           std::vector<ProblemInstance*>& sideInstances,
                           std::vector<ProblemSolution*>& sideSolutions,
                           std::vector<double>& sideBounds);
    ~SideConstraintInstance(void);
    // according to its statistics, create each side constraint solutions.
    void setType();
    // initialize solution by using greedy sampling with only side constraint cost.
    void initialSolution(double& initialCost);
    // descend until no improvement.
    void descend();
    // run the whole search process: initialize solution, then descend.
    void runSearch(double& initialCost);
    // check if open a new facility still satisfy all the side constraints.
    // used for only version of descend.
    bool isSideOk(int facilityToOpen);
    // old version of descend.
    bool descendOnce();
};
};

#endif
