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

#include <iostream>
#include "InitialSolution.h"
#include "redefs.h"
using namespace NewSensorPlacement;

InitialSolution::InitialSolution() {
}

InitialSolution::~InitialSolution() {
}


int InitialSolution::getRand(int start, int end) {
  int tem = random() % (end - start);
  return (tem + start);
}

void InitialSolution::permutateList(int* array, int length, int subsize) {
  for (int i=0; i<subsize; i++) {
    int num = getRand(i, length);
    int tem = array[num];
    array[num] = array[i];
    array[i] = tem; 
  }
}

int InitialSolution::getSampleSize(double fraction, int numM, int numSol){
    int sampleSize;
    if ((fraction >=0) && (fraction <=1)) 
      sampleSize = (int)ceil( fraction * (double) numM);
    else
      sampleSize = (int)ceil(log2((double)numM / (double)numSol));
    return std::max(1, sampleSize);
}

void InitialSolution::checkValid(int numSol, int numM) {
  if ((numSol <= 0) || (numSol > numM)) {
    std::cerr << "Invalid input!\n";
    exit(-1);
  }
}

int InitialSolution::getCandList(ProblemSolution *s, int* candList, myInt1 constraints){
  int numM = s->getNumM();
  int numP = s->getNumP();
  int temFac;
  int pos = 0;
  for (int i=s->getNumP()+1; i<= numM; i++){
    temFac = s->getFacility(i);    
    if (constraints[temFac] == 0){
      candList[pos] = temFac;
      pos++;
    }
  }
  return pos;
}

int InitialSolution::findBestFac(ProblemSolution *s, int sampleSize, int numCand, int* candList){
  // If we're considering all the facilities, no need to randomly select what we'll consider
  if (numCand > sampleSize)
    permutateList((candList), numCand, sampleSize);
  int bestCand = 0;
  double bestValue = DBL_MAX; 
  // go over the candList, find the best facility; 
  for (int i = 0; i<sampleSize; i++) {
    int temFac = candList[i];
    double temValue = s->moveEvaluator(temFac, 1.0);
    if (temValue < bestValue) {
      bestValue = temValue;
      bestCand = i;
    }
  }
  return bestCand;
}

void InitialSolution::GreedySampling(ProblemSolution *s, int numSol, double fraction, 
                                     myInt1 constraints) {
  time_t startTime = time(NULL);
  int numM = s->getNumM();
  int numP = s->getNumP();
  int sampleSize;
  if (numSol == 1) 
    // If we are only placing one sensor, consider all possible places
    // and select the best (i.e. no downsizing through sampling)
    sampleSize = numM;
  else sampleSize = getSampleSize(fraction, numM, numSol);
  checkValid(numSol, numM);
  int* candList = new int[numM+1];
  int numCand;
  numCand =  getCandList(s, candList, constraints);
  int temFac;
  while (numP < numSol) {
    int bestCand = findBestFac(s, sampleSize, numCand, candList);
    s->addOneFac(candList[bestCand], true);
    candList[bestCand] = candList[numCand-1];
    numCand--;
    numP++;
  }
  time_t endTime = time(NULL);
  difftime(startTime, endTime);
  delete [] candList;
}

bool InitialSolution::isSmaller(double value1, double value2, int num1, int num2) {
  if (value1 < value2) 
    return true;
  else if ((value1 == value2) && (num1 < num2))
    return true;
  else
    return false;
}

bool InitialSolution::isEqual(double value1, double value2, int num1, int num2) {
  if ((value1 == value2) && (num1 == num2))
    return true;
  else
    return false;
}

void InitialSolution::findBestList(ProblemSolution *s, std::vector<int> &bestFacList,
                                  myInt1 constraints) {
  int numM = s->getNumM();
  int numP = s->getNumP();
  bestFacList.clear();
  double bestValue = DBL_MAX;
  int numBestFac = INT_MAX;
  for (int i=numP+1; i<= numM; i++) {
    int temFac = s->getFacility(i);
    if (constraints[temFac] != 2) {
      int temNumWorst;
      double temValue = s->moveEvaluator(temFac, temNumWorst);
      if (isSmaller(temValue, bestValue, temNumWorst, numBestFac)) {
        bestFacList.clear();
        bestFacList.push_back(temFac);
        bestValue = temValue;
        numBestFac = temNumWorst;
      }
      else if (isEqual(temValue, bestValue, temNumWorst, numBestFac)) 
        bestFacList.push_back(temFac);
    }
  }
}



void InitialSolution::GreedyWorst(ProblemSolution *s, int numSol,
                                  myInt1 constraints) {
  //std::cout << "STARTING GreedyWorst" << std::endl;
  int numM = s->getNumM();
  int numP = s->getNumP();
  checkValid(numSol, numM);
  std::vector<int> bestFacList;
  //
  // Add the first point at random.  This ensures that there is some variability in
  // the solutions being generated.
  // 
  if (numP < numSol) {
        bool validFound(false);
        int newFacility = -1;
        while(validFound==false)
            {
            //newFacility=s->getFacility(BossaRandom::getInteger(p+1,m));
            int tmp = (random() % (numM-numP)) + numP+1;
            newFacility = s->getFacility( tmp );
            //std::cout << "BUG: " << tmp << " " << newFacility << std::endl;
            if (constraints[newFacility] != 2)
                { validFound=true; }
        }
  s->addOneFac(newFacility,true);
  numP++;
  }
  //
  // Add remaining points greedily
  //
  while (numP < numSol) {
    //s->pprint(std::cout);
    findBestList(s, bestFacList, constraints);
    // WEH - THIS IS BIASED
    int indx = random() % bestFacList.size();
    s->addOneFac(bestFacList[indx], true);
    numP++;
  }   
  //s->pprint(std::cout);
  bestFacList.clear();
}


