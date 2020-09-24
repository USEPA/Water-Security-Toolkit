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


#ifndef VARSOLUTION_H
#define VARSOLUTION_H

#include <fstream>
#include <vector>

namespace NewSensorPlacement {
class VarSolution : public ProblemSolution{
  private:

  public:	

    VarSolution(ProblemInstance *inst);
    ~VarSolution(void);
    
    double moveEvaluator(int facToOpen, int& numWorstCases);
    double moveEvaluator(int facToOpen, double gamma);
    double cost(double gamma=0.05)const; 
    

};
};

#endif
