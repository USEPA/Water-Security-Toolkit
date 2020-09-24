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
// Created on June 30 2008.
// A class is used for imperfect sensor problems.


#ifndef IMPERFECTINSTANCE_H
#define IMPERFECTINSTANCE_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <stdlib.h>
#include <vector>
#include "solution.h"
#include "InitialSolution.h"
#include "MeanSolution.h"
#include "WorstSolution.h"
#include "VarSolution.h"
#include "TceSolution.h"
#include "InitialSolution.h"
#include "ProblemInstance.h"


class NodeTimePair {
  public:

    int Node;
    double ArrivalTime;

    NodeTimePair(int node, double arrivalTime):
      Node(node),
      ArrivalTime(arrivalTime)
    {
    }

    bool operator<(const NodeTimePair &other)const{
      if(ArrivalTime<other.ArrivalTime)
        return true;
      else if(ArrivalTime==other.ArrivalTime)
        return Node<other.Node;
      else
        return false;
    }
};



namespace NewSensorPlacement {
class ImperfectInstance {
  private:
    int numN, numP, numM;
    ProblemSolution& mySolution;
    ProblemInstance* myInstance;
    std::vector<std::vector<NodeTimePair> > sensorArrivalTime;
    std::vector<std::map<int, double> > &nodeArrivalTime;
    std::vector<std::vector<std::pair<int, double> > > &nodeArrivalSequence;
    std::vector<double> scenarioImpacts;
    std::vector<double> &detectionProbability; 
    myInt1& facilityLocationConstraints; 
    double moveEvaluator(int newFacility, int theScenario);
    double cost(int theScenario);
    void computeSensorArrivalTime(); 
    void computeImpact(); 
  public:	

    ImperfectInstance(ProblemSolution &sol, 
                std::vector<std::vector<std::pair<int, double> > > &arrivalSeq,
                std::vector<std::map<int, double> > &arrivalTime,
                myInt1& constraints,
                std::vector<double> &detectionPro);
    ~ImperfectInstance(void);

    double moveEvaluator(int facToOpen);
    double cost(); 
    void descend(double& startCost, double &finalCost); 
    void runSearch(double& startCost, double &finalCost); 
};
};

#endif
