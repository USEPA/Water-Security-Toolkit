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
// Implementing most of the grasp optimazation structures for speed up
// searching for best local search of p-median problem.


#ifndef MEANSOLUTION_H
#define MEANSOLUTION_H

#include <vector>
#include <fstream>
#include <algorithm>
#include "PreProcess.h"


namespace NewSensorPlacement {
class MeanSolution : public ProblemSolution{
  private:
    PreProcess *myPrePro; // Helper class;
    // find the best pair to swap.
    bool findBestNeigh(int& facToInsert, int& facToRemove, double& profit, myInt1 facilityLocationConstraints);
    // update one customer;
    void updateOne(int user);
    // undo the update for one customer;
    void updateUndo(int user);
    // find the facility with minimal loss to remove;
    double findMinLose(int &facToRemove);
    // find the facility with maximal gain to insert;
    double findMaxSave(int &facToInsert);
  public:	
    myDouble2 extraCost; // extraCost for a pair facilty
    myBool1 affectedList; // list of customers need to update.
    myDouble1 gainValue; // gain by insert each facility;
    myDouble1 lossValue; // loss by remove each facility;

    MeanSolution(ProblemInstance *inst, bool isSort);
    ~MeanSolution(void);
    // reset value;
    void resetValue();
    // update the whole table of loss, gain... at start.
    void updateTable();
    // update once at each step;
    void updateOnce(int mode);
    // update affected customers;
    void updateAffected(int facToInsert, int facToRemove);
    // used for initial solution;
    // also can be used for search, but not optimal way to do it;
    double moveEvaluator(int facToOpen, double gamma);
    // used for initial solution of other cases, not used here;
    double moveEvaluator(int facToOpen, int& numWorstCases);
    // descend until no improvement can be found;
    void descend(double gamma, myInt1 facilityLocationConstraints);
    // cost of current situation;
    double cost(double gamma=0.05)const; 
};
};

#endif
