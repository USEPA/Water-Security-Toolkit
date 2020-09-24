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


#ifndef WORSTSOLUTION_H
#define WORSTSOLUTION_H

#include <fstream>
#include <vector>

namespace NewSensorPlacement {
class WorstSolution : public ProblemSolution{
  private:
  
  public:	

    WorstSolution(ProblemInstance *inst);
    ~WorstSolution(void);

    double moveEvaluator(int facToOpen, double gamma);
    double moveEvaluator(int facToOpen, int& numWorstCases);
    double cost(double gamma=0.05)const; 
    int numWorstCaseScenarios(double gamma=0.05)const;
};
};

#endif
