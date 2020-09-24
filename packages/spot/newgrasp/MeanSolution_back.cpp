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

#include <stdlib.h>
#include "solution.h"
#include "MeanSolution.h"

MeanSolution::MeanSolution(ProblemInstance *inst):
  ProblemSolution(inst) {
  extraCost = std::vector<std::vector<double> >(numM+1,std::vector<double>(numM+1,0.0));  
  gainValue.assign(numM+1,0.0);
  lossValue.assign(numM+1,0.0);
  affectedList.assign(numN+1,false);
}

MeanSolution::~MeanSolution(){
  gainValue.clear();
  lossValue.clear();
}

double MeanSolution::moveEvaluator(int facToOpen, double gamma) {
  double sumDistance(0.0);
  // assess the impact of opening the new facility.
  for(int i=1;i<=numN;++i){
    double distToNewFacility(myInstance->getDist(i,facToOpen));
    sumDistance= sumDistance + std::min(getDistClosest(i),distToNewFacility);
  }
  return (sumDistance / ((double) numN));
}


void MeanSolution::resetValue() {
  for (int i=1; i<= numM; i++){
    gainValue[i] = 0.0;
    lossValue[i] = 0.0;
    for (int j=1; j<=numM; j++)
      extraCost[i][j] = 0.0;
  }
}

void MeanSolution::updateTable() {
  for (int i=1; i<=numN; i++)
    updateOne(i);
}

void MeanSolution::updateOne(int user) {
  int facToRemove = getClosest(user);
  lossValue[facToRemove] += getDistSecondClosest(user) - getDistClosest(user);
  for (int i=numP+1; i<=numM; i++) {
    int facToInsert = myFacilityList[i];
    double temDist = myInstance->getDist(user, facToInsert);
    if (temDist < getDistSecondClosest(user)) {
      gainValue[facToInsert] += std::max( 0.0, (getDistClosest(user) - temDist));
      extraCost[facToInsert][facToRemove] += getDistSecondClosest(user) - std::max(temDist, getDistClosest(user));
    }
  }  
}

void MeanSolution::updateUndo(int user) {
  int facToRemove = getClosest(user);
  lossValue[facToRemove] -= getDistSecondClosest(user) - getDistClosest(user);
  for (int i=numP+1; i<=numM; i++) {
    int facToInsert = myFacilityList[i];
    double temDist = myInstance->getDist(user, facToInsert);
    if (temDist < getDistSecondClosest(user)) {
      gainValue[facToInsert] -= std::max( 0.0, (getDistClosest(user) - temDist));
      extraCost[facToInsert][facToRemove] -= getDistSecondClosest(user) - std::max(temDist, getDistClosest(user));
    }
  }  
}

void MeanSolution::updateAffected(int facToInsert, int facToRemove) {
  for (int i=1; i<=numN; i++){
    if ( (getClosest(i) == facToRemove) ||
         (getSecondClosest(i) == facToRemove) ||
         (getDistSecondClosest(i) > myInstance->getDist(i, facToInsert)))
      affectedList[i] = true;
    else
      affectedList[i] = false;
  }
}

void MeanSolution::updateOnce(int mode) {
  if (mode >0) {
    for (int i=1; i<=numN; i++) 
      if (affectedList[i])
        updateOne(i);
  }
  else { 
    for (int i=1; i<=numN; i++) 
      if (affectedList[i])
        updateUndo(i);
  }
}



void MeanSolution::descend(double gamma, const std::vector<int> &facilityLocationConstraints) {
  resetValue();
  updateTable();
  int facToInsert, facToRemove;
  double profit;
  while(findBestNeigh(facToInsert, facToRemove, profit, facilityLocationConstraints)){
    //std::cout <<"Profit is ---------------" << profit<<std::endl;
    updateAffected(facToInsert, facToRemove);
    updateOnce(-1);
    swapFac(facToRemove, facToInsert, true);
    updateOnce(1);
  } 
}



bool MeanSolution::findBestNeigh(int& facToInsert, int& facToRemove, double& profit,
                                 const std::vector<int> &facilityLocationConstraints){
  profit = 0.0;
  double temValue;
  int temFacRem, temFacIns;
  for (int i=1; i<=numP; i++)
    for (int j=numP+1; j<=numM; j++) {
      temFacRem = myFacilityList[i];
      temFacIns = myFacilityList[j];
      if ((facilityLocationConstraints[temFacRem] !=1) &&
          (facilityLocationConstraints[temFacIns] !=2)){
        temValue = gainValue[temFacIns] - lossValue[temFacRem] + extraCost[temFacIns][temFacRem];
        if (temValue > profit){
          facToInsert = temFacIns;
          facToRemove = temFacRem;
          profit = temValue;
        }
      }
    }
  if (profit > 0) return true;   
  else return false; 
}


double MeanSolution::cost(double gamma)const {
  if(numP==0)
    return MY_MAX; 
  double sum = 0.0;
  for(int i=1; i<= numN; i++)
    sum = sum + myClosestDist[i];
  return (sum / (double)numN);
}	


double MeanSolution::costForInitial(double gamma)const {
  return cost(gamma);
}

int MeanSolution::numWorstCaseScenarios(double gamma)const {
  return 1;
}

