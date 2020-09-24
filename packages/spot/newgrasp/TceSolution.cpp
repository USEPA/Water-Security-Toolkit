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
#include "TceSolution.h"
using namespace NewSensorPlacement;

TceSolution::TceSolution(ProblemInstance *inst):
  ProblemSolution(inst) {
}

TceSolution::~TceSolution(){
}


double TceSolution::moveEvaluator(int facToOpen, int & numWorstCases){
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



double TceSolution::moveEvaluator(int facToOpen, double gamma){
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
  index--;
  double perAttackProbability = 1.0/numN;
  // sum the tail weight and normalize
  double result(0.0);
  double tailProb(0.0);
  for(int i=index;i<numN;++i) {
      result += (impacts[i] * perAttackProbability);
      tailProb += perAttackProbability;
  }
  return result/tailProb;
}


double TceSolution::cost(double gamma)const{
  if(numP==0)
    return MY_MAX; 
  std::vector<double> impacts(numN);
  for(int i=1;i<=numN;i++)
    impacts[i-1]=getDistClosest(i);
  std::sort(impacts.begin(),impacts.end());

  // compute the gamma index under the fixed-probability assumption
  int index((int)(ceil(numN * (1.0-gamma))));
  index--;
  double perAttackProbability = 1.0/numN;
  // sum the tail weight and normalize
  double result(0.0);
  double tailProb(0.0);
  for(int i=index;i<numN;i++)
    {
      result+=(impacts[i]*perAttackProbability);
      tailProb+=perAttackProbability;
    }
  return result/tailProb;
}






