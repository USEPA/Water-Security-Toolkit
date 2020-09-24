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

#include "solution.h"
#include "VarSolution.h"
using namespace NewSensorPlacement;

VarSolution::VarSolution(ProblemInstance *inst):
  ProblemSolution(inst) {
}

VarSolution::~VarSolution(){
}

//
// TODO: review this code.  This moveEvaluator compute the worst-case
// impact, which I think is a bug.  But the real issue is that gamma
// is not available here...
//
double VarSolution::moveEvaluator(int facToOpen, int & numWorstCases){
  double worstCaseDistance(0.0);
  // assess the impact of opening the new facility.
  std::vector<double> temDist(numN+1, 0.0);
  for(int i=1;i<=numN;++i){
    temDist[i] = std::min(myInstance->getDist(i, facToOpen), myClosestDist[i]);
    worstCaseDistance=std::max(worstCaseDistance, temDist[i]);
  }
  numWorstCases = 0;
  for(int i=1; i<=numN; i++)
    if(worstCaseDistance == temDist[i])
      numWorstCases++;
  return worstCaseDistance;
}



double VarSolution::moveEvaluator(int facToOpen, double gamma){
  std::vector<double> impacts(numN);
  // assess the impact of opening the new facility.
  for(int i=1;i<=numN;++i){
    double distToNewFacility(myInstance->getDist(i,facToOpen));
    if (distToNewFacility<getDistClosest(i))
      impacts[i-1] = distToNewFacility;
    else
      impacts[i-1] = getDistClosest(i);
  }
  std::sort(impacts.begin(),impacts.end());
  // compute the gamma index under the fixed-probability assumption
  int index((int)(ceil(numN*(1.0-gamma))));
  return impacts[index-1];
}


double VarSolution::cost(double gamma)const {
  if(numP==0) 
    return MY_MAX; 
  std::vector<double> impacts(numN);
  for(int i=1;i<=numN;i++)
      impacts[i-1]=getDistClosest(i);
  std::sort(impacts.begin(),impacts.end());

  // compute the gamma index under the fixed-probability assumption
  int index((int)(ceil(numN*(1.0-gamma))));
  return impacts[index-1];
}






