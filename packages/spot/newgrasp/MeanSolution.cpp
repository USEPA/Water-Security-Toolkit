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
using namespace NewSensorPlacement;

MeanSolution::MeanSolution(ProblemInstance *inst, bool isSort):
  ProblemSolution(inst) {
  #ifdef OPTIMER
    extraCost = new double*[numM+1];
    for (int i=0; i<=numM; i++)
      extraCost[i] = new double[numM+1];
    gainValue = new double[numM+1];
    lossValue = new double[numM+1];
    affectedList = new bool[numN+1];
  #else
    extraCost = std::vector<std::vector<double> >(numM+1, std::vector<double>(numM+1, 0.0));
    gainValue = std::vector<double>(numM+1, 0.0);
    lossValue = std::vector<double>(numM+1, 0.0);
    affectedList = std::vector<bool>(numN+1, false);
  #endif
  myPrePro = new PreProcess(inst, isSort);
}





MeanSolution::~MeanSolution(){
  #ifdef OPTIMER
  delete [] affectedList;
  delete [] gainValue;
  delete [] lossValue;
  for (int i=0; i<=numM; i++)
    delete [] extraCost[i];
  delete [] extraCost;
  #endif
  delete myPrePro;
}

double MeanSolution::moveEvaluator(int facToOpen, int & numWorstCases) {
  return 0.0;
}

// evaluate the cost for insert a new facility;
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


/*

// old way to do update, maybe slower than the current one;
void MeanSolution::updateOne(int user) {
  int facToRemove = getClosest(user);
  double dist1 = getDistClosest(user);
  double dist2 = getDistSecondClosest(user);
  lossValue[facToRemove] += dist2 - dist1;
  int* closeList = myPrePro->getPreFac(user);
  bool isContinue =  true;
  int i = 1;
  while (isContinue && (i <=numN)) {
    int facToInsert = closeList[i];
    if (!isOpenFac(facToInsert)) {
      double temDist = myInstance->getDist(user, facToInsert);
      if (temDist>=dist2) {
        isContinue = false;
        break;
      }
      else {
      gainValue[facToInsert] += std::max( 0.0, (dist1 - temDist));
      extraCost[facToInsert][facToRemove] += dist2 - std::max(temDist, dist1);
    }}
    i++;
  }
}

void MeanSolution::updateUndo(int user) {
  int facToRemove = getClosest(user);
  double dist1 = getDistClosest(user);
  double dist2 = getDistSecondClosest(user);
  lossValue[facToRemove] -= dist2 - dist1;
  int* closeList = myPrePro->getPreFac(user);
  bool isContinue =  true;
  int i = 1;
  while (isContinue && (i <=numN)) {
    int facToInsert = closeList[i];
    if (!isOpenFac(facToInsert)) {
      double temDist = myInstance->getDist(user, facToInsert);
      if (temDist>=dist2) {
        isContinue = false;
        break;
      }
      else {
      gainValue[facToInsert] -= std::max( 0.0, (dist1 - temDist));
      extraCost[facToInsert][facToRemove] -= dist2 - std::max(temDist, dist1);
    }}
    i++;
  }
}


*/
void MeanSolution::updateOne(int user) {
  int facToRemove = getClosest(user);
  double dist1 = getDistClosest(user);
  double dist2 = getDistSecondClosest(user);
  lossValue[facToRemove] += dist2 - dist1;
  // get all the facilities whose closer than dist2;
  //std::pair<int, double>* closeList = myPrePro->getPreFac(user, dist2);
  myPair1 closeList = myPrePro->getNewPreFac(user, dist2);
  for (int i=1; i<=closeList[0].num; i++) {
    int facToInsert = closeList[i].num;
    double temDist = closeList[i].dist;
    if (!isOpenFac(facToInsert)) {
      gainValue[facToInsert] += std::max( 0.0, (dist1 - temDist));
      // extraCost is an interaction between two facilities; if we are
      // only placing a single facility, there can be no interaction.
      if ( numP != 1 )
         extraCost[facToInsert][facToRemove] += dist2 - std::max(temDist, dist1);
    }
  }  
}

void MeanSolution::updateUndo(int user) {
  int facToRemove = getClosest(user);
  double dist1 = getDistClosest(user);
  double dist2 = getDistSecondClosest(user);
  lossValue[facToRemove] -= dist2 - dist1;
  //std::pair<int, double>* closeList = myPrePro->getPreFac(user, dist2);
  myPair1 closeList = myPrePro->getNewPreFac(user, dist2);
  for (int i=1; i<=closeList[0].num; i++) {
    int facToInsert = closeList[i].num;
    double temDist = closeList[i].dist;
    if (!isOpenFac(facToInsert)) {
      gainValue[facToInsert] -= std::max( 0.0, (dist1 - temDist));
      // extraCost is an interaction between two facilities; if we are
      // only placing a single facility, there can be no interaction.
      if ( numP != 1 )
        extraCost[facToInsert][facToRemove] -= dist2 - std::max(temDist, dist1);
    }
  }  
}

/*
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
*/


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



void MeanSolution::descend(double gamma, myInt1 facilityLocationConstraints){
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

double MeanSolution::findMinLose(int &facToRemove) {
  double profit = DBL_MAX;
  double temValue;
  int temFacRem;
  for (int i=1; i<=numP; i++){
    temFacRem = myFacilityList[i];
    temValue = lossValue[temFacRem];
    if (temValue < profit){
      facToRemove = temFacRem;
      profit = temValue;
    }
  }
  return profit;
}

double MeanSolution::findMaxSave(int &facToInsert) {
  double profit = 0.0;
  double temValue;
  int temFacIns;
  for (int j=numP+1; j<=numM; j++) {
    temFacIns = myFacilityList[j];
    temValue = gainValue[temFacIns];
    if (temValue > profit){
      facToInsert = temFacIns;
      profit = temValue;
    }
  }
  return profit;
}

bool MeanSolution::findBestNeigh(int& facToInsert, int& facToRemove, double& profit,
                                 myInt1 facilityLocationConstraints){
  profit = 0.0;
  double temValue;
  int temFacRem, temFacIns;
  profit = std::max(0.0, (findMaxSave(facToInsert) - findMinLose(facToRemove)));
  for (int i=1; i<=numP; i++)
    for (int j=numP+1; j<=numM; j++) {
      temFacRem = myFacilityList[i];
      temFacIns = myFacilityList[j];
      if ((extraCost[temFacIns][temFacRem] > 0) &&
          (facilityLocationConstraints[temFacRem] !=1) &&
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




