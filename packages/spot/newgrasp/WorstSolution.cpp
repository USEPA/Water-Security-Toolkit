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
#include "WorstSolution.h"
using namespace NewSensorPlacement;

WorstSolution::WorstSolution(ProblemInstance *inst):
  ProblemSolution(inst) {
}

WorstSolution::~WorstSolution(){
}

double WorstSolution::moveEvaluator(int facToOpen, int & numWorstCases){
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


double WorstSolution::moveEvaluator(int facToOpen, double gamma){
  double worstCaseDistance(0.0);
  // assess the impact of opening the new facility.
  for(int i=1;i<=numN;++i){
    double distToNewFacility(myInstance->getDist(i,facToOpen));
    if (distToNewFacility<getDistClosest(i)) 
      worstCaseDistance=std::max(worstCaseDistance,distToNewFacility);
    else
      worstCaseDistance=std::max(worstCaseDistance,getDistClosest(i));
  }
  return worstCaseDistance;
}

double WorstSolution::cost(double gamma)const{
  if(numP==0)  
    return MY_MAX; 
  double sum = 0.0;
  for(int i=1; i <= numN; i++)
    sum = std::max(sum, myClosestDist[i]);
  return sum;
}       



int WorstSolution::numWorstCaseScenarios(double gamma)const {
  if(numP==0) 
    return -1;
  double myCost = cost(gamma);
  int result = 0;
  for(int i=1; i<=numN; i++) 
    if(myCost == myClosestDist[i])
      result++; 
  return result;
}       







