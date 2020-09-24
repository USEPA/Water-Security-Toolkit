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
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "solution.h"
using namespace NewSensorPlacement;

// swap 2 facilities and update the position index;
void ProblemSolution::swappos(int i, int j){
  int prev_i = myFacilityList[i];
  int prev_j = myFacilityList[j];
  myFacilityList[i]=prev_j;
  myFacilityList[j]=prev_i;
  myFacilityPosition[myFacilityList[i]]=i;
  myFacilityPosition[myFacilityList[j]]=j;
}

// initialize members;
ProblemSolution::ProblemSolution(ProblemInstance *inst):
  myInstance(inst),
  numN(myInstance->getN()),
  numM(myInstance->getM())
{
  numP = 0;
  #ifdef OPTIMER 
  myFacilityList = new int[numM+1];
  myFacilityPosition = new int[numM+1];
  myClosestFacility = new int[numN+1];
  mySecondClosestFacility = new int[numN+1];
  myClosestDist = new double[numN+1];
  mySecondClosestDist = new double[numN+1];
  for(int i=0;i<=numN;i++){
    myClosestFacility[i] = 0;
    mySecondClosestFacility[i] =0;
    myClosestDist[i] = MY_MAX;
    mySecondClosestDist[i] = MY_MAX;
  }
  #else
    #if 1
    myFacilityList = std::vector<int>(numM+1, 0);
    myFacilityPosition = std::vector<int>(numM+1, 0);
    myClosestFacility = std::vector<int>(numN+1, 0);
    mySecondClosestFacility = std::vector<int>(numN+1, 0);
    myClosestDist = std::vector<double>(numN+1, MY_MAX);
    mySecondClosestDist = std::vector<double>(numN+1, MY_MAX);
    #else
    myFacilityList = utilib::BasicArray<int>(numM+1, 0);
    myFacilityPosition = utilib::BasicArray<int>(numM+1, 0);
    myClosestFacility = utilib::BasicArray<int>(numN+1, 0);
    mySecondClosestFacility = utilib::BasicArray<int>(numN+1, 0);
    myClosestDist = utilib::BasicArray<double>(numN+1, MY_MAX);
    mySecondClosestDist = utilib::BasicArray<double>(numN+1, MY_MAX);
    #endif
  #endif
  for(int i=0;i<=numM;i++){
    myFacilityList[i]=  i;
    myFacilityPosition[i]= i;
  }
}

// copy solution from s and update closest list; assume closest information
// is correst in s;
void ProblemSolution::setSolution(ProblemSolution &s){
  reset();
  numP = s.getNumP();
  for(int i=1;i<=numM;i++){
    myFacilityList[i] = s.getFacility(i);
    myFacilityPosition[i] = s.getPosition(i);
  }
  updateClosest();
  /*for(int i=1;i<=numN;i++){
    myClosestFacility[i] = s.getClosest(i);
    myClosestDist[i] = s.getDistClosest(i);
    mySecondClosestFacility[i] = s.getSecondClosest(i);
    mySecondClosestDist[i] = s.getDistSecondClosest(i);
  }*/

}

// free up the space;
ProblemSolution::~ProblemSolution(void) {
  #ifdef OPTIMER
  delete [] myFacilityList;
  delete [] myFacilityPosition;
  delete [] myClosestFacility;
  delete [] mySecondClosestFacility;
  delete [] myClosestDist;
  delete [] mySecondClosestDist;
  #endif
}


void ProblemSolution::readSolution(const char *fileName) {
  std::cout << "Reading solution..." << std::endl;
  const int LINESIZE = 256;
  int line = 0;
  char buffer [LINESIZE + 1];
  FILE *file = fopen (fileName, "r");
  if (file == 0)  {
    std::cerr << "Error opening input file=" << fileName << std::endl;
    exit (-1);
  }
  reset();
  while(fgets (buffer, LINESIZE, file)) {
    line ++;
    int f;
    switch (buffer[0]) {
    case 'f': //line describing problem size
      if (sscanf (buffer, "f %d", &f)==1) {
	if (f<1 || f>numM) {
	  std::cerr << "Error reading file " << fileName 
                    << ": facility label in line " 
                    << line << " is out of range (" << f << ")." 
                    << std::endl;
	} 
	else 
	  addOneFac(f, false);
      }
    default:
      continue;
    }
  }
  fclose (file);
  updateClosest();
}
		



// reset the number of open facility to 0 and update closest info;
void ProblemSolution::reset(void) {
  numP=0;
  for (int i=1; i<=numN; i++) {
    myClosestFacility[i] = 0;
    myClosestDist[i] = MY_MAX;
    mySecondClosestFacility[i] = 0;
    mySecondClosestDist[i] = MY_MAX;
  }
}


// add one facility to open;
void ProblemSolution::addOneFac(int f, bool update) {
  swappos(myFacilityPosition[f],++numP); // increase solution size, and put f in the last (p-th) position
  if(update==true)
    updateAddOneFac(f);
}

// swap a pair of facilities for open and close;		
void ProblemSolution::swapFac(int facilityToClose, int facilityToOpen, bool update) {
  addOneFac(facilityToOpen,update); 
  removeOneFac(facilityToClose, update);
}

// close one facility;		
void ProblemSolution::removeOneFac(int f, bool update) {
  swappos(myFacilityPosition[f], numP); // place the p-th element where i was
  numP--; // decrease solution size
  if(update==true) 
    updateRemoveOneFac(f);
}			

// update closest info after opening facility numF;
void ProblemSolution::updateAddOneFac(int numF) {
  for (int i=1; i<=numN; i++)
    updateAddOneFac(numF, i);
}

// helper function for above function;
void ProblemSolution::updateAddOneFac(int numF, int numC) {
  double dist = myInstance->getDist(numC, numF);
  if (dist < mySecondClosestDist[numC]){
    if (dist < myClosestDist[numC]) {
      mySecondClosestFacility[numC] = myClosestFacility[numC];
      mySecondClosestDist[numC] = myClosestDist[numC];
      myClosestFacility[numC] = numF;
      myClosestDist[numC] = dist;
    }
    else {
      mySecondClosestFacility[numC] = numF;
      mySecondClosestDist[numC] = dist;
    }
  }
}

// helper function for below function;
void ProblemSolution::updateRemoveOneFac(int numF, int numC) {
  if ( (mySecondClosestFacility[numC] == numF)
     ||(myClosestFacility[numC] == numF))
    updateClosest(numC);
}
    
// update closest info after closing facility numF;
void ProblemSolution::updateRemoveOneFac(int numF) {
  for (int i=1; i<=numN; i++)
    updateRemoveOneFac(numF, i);
}

// update closest info from scrub, should be slow;
void ProblemSolution::updateClosest() {
  for (int i=1; i<= numN; i++)
    updateClosest(i);
}

// update closest info for customer i;
void ProblemSolution::updateClosest(int i) {
  int num1 = 0;
  int num2 = 0;
  double dist1 = MY_MAX;
  double dist2 = MY_MAX;
  int temNum;
  double temDist;
  // finding the closest and second closest facilties;
  for(int j=1; j<=numP; j++) {
    temNum = myFacilityList[j];
    temDist = myInstance->getDist(i, temNum);
    if (temDist < dist2) {
      if (temDist < dist1) {
        dist2 = dist1;
        dist1 = temDist;
        num2 = num1;
        num1 = temNum;
      }
      else {
        dist2 = temDist;
        num2 = temNum;
      }
    }
  }
  myClosestFacility[i] = num1;
  myClosestDist[i] = dist1;
  mySecondClosestFacility[i] = num2;
  mySecondClosestDist[i] = dist2;
}

// go over all the possible facility to close, and open, find the first pair which can
// improve the cost and return true, otherwise return false;
bool ProblemSolution::descendOnce(double gamma, myInt1 facilityLocationConstraints) {
  double baselineCost(cost(gamma));
  for(int facilityToClosePosition=1;facilityToClosePosition<=numP;facilityToClosePosition++){
    int facilityToClose(getFacility(facilityToClosePosition));
    if(facilityLocationConstraints[facilityToClose]!=1) {
      removeOneFac(facilityToClose,true);
      for(int facilityToOpenPosition=numP+1;facilityToOpenPosition<=numM;facilityToOpenPosition++){
        int facilityToOpen(getFacility(facilityToOpenPosition));
        if(facilityLocationConstraints[facilityToOpen]!=2) {
          addOneFac(facilityToOpen,true);
          if(cost(gamma)<baselineCost){
            return true;
          }
          removeOneFac(facilityToOpen, true);
        }
      }
      addOneFac(facilityToClose, true);
    }
  }
  return false;
}

bool ProblemSolution::descendHelper(double gamma, myInt1 facilityLocationConstraints){ 
  double baselineCost(cost(gamma));
  for(int facilityToClosePosition=1;facilityToClosePosition<=numP;facilityToClosePosition++){
    int facilityToClose(getFacility(facilityToClosePosition));
    if(facilityLocationConstraints[facilityToClose]!=1) {
      removeOneFac(facilityToClose,true); // try to remove one facility;
      for(int facilityToOpenPosition=numP+1;facilityToOpenPosition<=numM;facilityToOpenPosition++){
        int facilityToOpen(getFacility(facilityToOpenPosition));
        if(facilityLocationConstraints[facilityToOpen]!=2) {
          double costOfMove = moveEvaluator(facilityToOpen, gamma);
          if(costOfMove<baselineCost){ // see if add this facility give a better cost;
            baselineCost = costOfMove; 
            addOneFac(facilityToOpen,true); // if so, add it and return true;
            return true;
          }
        }
      }
      addOneFac(facilityToClose, true); // re-open this facility for next iteration try;
    }
  }
  return false; // Improvement is not found;
}
  

// go over all the possible facility to close and open, find the best pair and swap them;
bool ProblemSolution::descendBest(double gamma, myInt1 facilityLocationConstraints){ 
  int bestRev, bestIns;
  bool isFound = false;
  double baselineCost(cost(gamma));
  for(int facilityToClosePosition=1;facilityToClosePosition<=numP;facilityToClosePosition++){
    int facilityToClose(getFacility(facilityToClosePosition));
    if(facilityLocationConstraints[facilityToClose]!=1) {
      removeOneFac(facilityToClose,true);
      for(int facilityToOpenPosition=numP+1;facilityToOpenPosition<=numM;facilityToOpenPosition++){
        int facilityToOpen(getFacility(facilityToOpenPosition));
        if(facilityLocationConstraints[facilityToOpen]!=2) {
          double costOfMove = moveEvaluator(facilityToOpen, gamma);
          if(costOfMove<baselineCost){ // Find a better pair, update swap info;
            baselineCost = costOfMove;
            bestRev = facilityToClose;
            bestIns = facilityToOpen;
            isFound  = true;
          }
        }
      }
      addOneFac(facilityToClose, true); // reopen closed facility for next iteration;
    }
  }
  if (isFound) 
    swapFac(bestRev, bestIns, true); // swap the best pair for improvement;
  return isFound;
}

void ProblemSolution::descend(double gamma, myInt1 facilityLocationConstraints){
  while (descendHelper(gamma, facilityLocationConstraints))
    ;
}

void ProblemSolution::pprint(std::ostream& os)
{
//
// Print current solution
//
os << "Current Solution: ";
for (int i=1; i<=numP; i++)
  os << myFacilityList[i] << " ";
os << std::endl;
//
// Print distance matrix
//
os << "Nearest Facility for Each Customer: ";
for (int i=1; i<=numN; i++)
  os << myClosestFacility[i] << " ";
os << std::endl;
os << "Distance to Nearest Facility for Each Customer: ";
for (int i=1; i<=numN; i++)
  os << myClosestDist[i] << " ";
os << std::endl;
}

