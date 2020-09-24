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

#include "SideConstraintInstance.h"
using namespace NewSensorPlacement;

SideConstraintInstance::SideConstraintInstance(ProblemInstance *inst,
                       ProblemSolution *sol,
                       std::string stat,
                       myInt1& constraints,
                       double _gamma,
                       std::vector<std::string>& sideStatistics,
                       std::vector<ProblemInstance*>& sideInstances,
                       std::vector<ProblemSolution*>& sideSolutions,
                       std::vector<double>& sideBounds):
  instance(inst),
  mySolution(sol),
  statistics(stat),
  facilityLocationConstraints(constraints),
  gamma(_gamma),
  sideConstraintSolutions(sideSolutions),
  sideConstraintStatistics(sideStatistics),
  sideConstraintInstances(sideInstances),
  sideConstraintBounds(sideBounds) 
{
  weight = 10.1;
  sideCost = std::vector<double>(sideConstraintBounds.size(), 0.0);
  numSideConst = sideConstraintInstances.size();
  setType();
  numN = instance->getN();
  numM = instance->getM();
  numP = instance->getP();
  initialMaxCost();
}

SideConstraintInstance::~SideConstraintInstance(void){

}

void SideConstraintInstance::setType(){
/*  if (statistics == "mean")
    mySolution = new MeanSolution(instance, true);
  if (statistics == "worst")
    mySolution = new WorstSolution(instance);
  if (statistics == "var")
    mySolution = new VarSolution(instance);
  if (statistics == "tce")
    mySolution = new TceSolution(instance);
*/
  for(int i=0;i<numSideConst;i++){
    if ((sideConstraintStatistics)[i] == "mean")
      sideConstraintSolutions.push_back(new MeanSolution((sideConstraintInstances)[i], true));
    if ((sideConstraintStatistics)[i] == "worst")
      sideConstraintSolutions.push_back(new WorstSolution((sideConstraintInstances)[i]));
    if ((sideConstraintStatistics)[i] == "var")
      sideConstraintSolutions.push_back(new VarSolution((sideConstraintInstances)[i]));
    if ((sideConstraintStatistics)[i] == "tce")
      sideConstraintSolutions.push_back(new TceSolution((sideConstraintInstances)[i]));
 } 
}


void SideConstraintInstance::initialMaxCost(){
  resetSolution();
  ObjectMax = mySolution->cost(gamma);
  for(int i=0;i<numSideConst;i++)
    sideMax.push_back(sideConstraintSolutions[i]->cost(gamma));
}


void SideConstraintInstance::resetSolution(){
  mySolution->reset();
  for(int i=0;i<numSideConst;i++)
    sideConstraintSolutions[i]->reset();
}


void SideConstraintInstance::initialSolution(double& initialCost){
  resetSolution();
  //
  // If we have no constraints, then do the initialization the 'old' way...
  //
  if (numSideConst == 0) {
      InitialSolution myIniSol;
      if(statistics=="mean")
        myIniSol.GreedySampling(mySolution, numP, 2, facilityLocationConstraints);
      else
        myIniSol.GreedyWorst(mySolution,numP, facilityLocationConstraints);
  }
  else {
  int temP = mySolution->getNumP();
  int sampleSize = initHelper.getSampleSize(2, numM, numP);
  initHelper.checkValid(numP, numM);
  int* candList = new int[numM+1];
  int numCand;
  numCand =  initHelper.getCandList(mySolution, candList, facilityLocationConstraints);
  int temFac;
  while (temP < numP) {
    int bestCand = findBestFac(sampleSize, numCand, candList);
    mySolution->addOneFac(candList[bestCand], true);
    for(int i=0;i<numSideConst;i++)
      sideConstraintSolutions[i]->addOneFac(candList[bestCand], true);
    candList[bestCand] = candList[numCand-1];
    numCand--;
    temP++;
  }
  delete [] candList;
  }
  for (int i=0; i<numSideConst; i++)
    sideConstraintSolutions[i]->reset();
  initialCost = mySolution->cost(gamma);
  initialValue = initialCost;
  //std::cout<<"initial part is done----\n";
}



int SideConstraintInstance::findBestFac(int sampleSize, int numCand, int* candList){
  initHelper.permutateList((candList), numCand, sampleSize);
  int bestCand = 0;
  double bestValue = DBL_MAX;
  // go over the candList, find the best facility; 
  for (int i = 0; i<sampleSize; i++) {
    int temFac = candList[i];
    double temValue = getInitCost(temFac);
    if (temValue < bestValue) {
      bestValue = temValue;
      bestCand = i;
    }
  }
  return bestCand;
}


double SideConstraintInstance::getInitCost(int facToOpen){
  double temCost = 0;
  if (numSideConst == 0)
    temCost = mySolution->moveEvaluator(facToOpen, gamma);
  else
    for (int i=0; i<numSideConst; i++)
      temCost += normalizeCost(i,sideConstraintSolutions[i]->moveEvaluator(facToOpen, gamma));

  return 0.0;
} 

/*
void SideConstraintInstance::initialSolution(double& initialCost){
  resetSolution();
  InitialSolution myIniSol;
  if (numSideConst == 0){
    if(statistics=="mean")
      myIniSol.GreedySampling(mySolution, numP, 2, facilityLocationConstraints);
    else
      myIniSol.GreedyWorst(mySolution,numP, facilityLocationConstraints);
  }
  else if (numSideConst == 1){
    bool feasibleInitialPointFound(false);
    const int MAX_ATTEMPTS_AT_FEASIBILITY=10;
    for(int j=1;(j<=MAX_ATTEMPTS_AT_FEASIBILITY)&&(feasibleInitialPointFound==false);
        j++){
      if ((sideConstraintStatistics)[0] == "mean")
        myIniSol.GreedySampling(sideConstraintSolutions[0], numP, 2, facilityLocationConstraints);
      else
        myIniSol.GreedyWorst(sideConstraintSolutions[0],numP, facilityLocationConstraints);
      sideConstraintSolutions[0]->descend(gamma, facilityLocationConstraints);
      if(sideConstraintSolutions[0]->cost(gamma)<=(sideConstraintBounds)[0]){
        mySolution->setSolution(*sideConstraintSolutions[0]);
        feasibleInitialPointFound=true;
        sideConstraintSolutions[0]->reset();
        initialCost = mySolution->cost(gamma);
        initialValue = initialCost;
      }
    }
    if(feasibleInitialPointFound==false){
      std::cout << "***An upper bound of " << (sideConstraintBounds)[0] ;
      std::cout << " could not be achieved after " << MAX_ATTEMPTS_AT_FEASIBILITY << " iterations of sampling!\n";
      exit(1);
    }
  }
  else 
    exit(1);
}
*/


double SideConstraintInstance::getCost(){
  double temCost = mySolution->cost(gamma);
  for(int i=0; i<numSideConst; i++){
    double temSide = sideConstraintSolutions[i]->cost(gamma);
    temCost += normalizeCost(i, temSide);
  }
  return temCost;
}



void SideConstraintInstance::closeOneFac(int facilityToClose){
  for(int i=0; i<numSideConst; i++){  
    if (sideConstraintStatistics[i] != "mean") 
      sideConstraintSolutions[i]->removeOneFac(facilityToClose,true);
  }
  if (statistics != "mean") 
    mySolution->removeOneFac(facilityToClose,true);
}
          
void SideConstraintInstance::openOneFac(int facilityToClose){
  for(int i=0; i<numSideConst; i++){  
    if (sideConstraintStatistics[i] != "mean") 
      sideConstraintSolutions[i]->addOneFac(facilityToClose,true);
  }
  if (statistics != "mean") 
    mySolution->addOneFac(facilityToClose,true);
}


double SideConstraintInstance::getOneCost(int facilityToClose, int facilityToOpen,
                                          ProblemSolution* theSolution, 
                                          std::string theStatistics,
                                          double currentCost){
  if (theStatistics != "mean")
    return theSolution->moveEvaluator(facilityToOpen, gamma);

  double temCost = currentCost
     - ((MeanSolution*) theSolution)->gainValue[facilityToOpen] 
     + ((MeanSolution*) theSolution)->lossValue[facilityToClose]
     - ((MeanSolution*) theSolution)->extraCost[facilityToOpen][facilityToClose];
  //std::cout << "cost: " << temCost << " == " << currentCost 
  //           << " - " << ((MeanSolution*) theSolution)->gainValue[facilityToOpen] 
  //           << " + " << ((MeanSolution*) theSolution)->lossValue[facilityToClose]
  //           << " - " << ((MeanSolution*) theSolution)->extraCost[facilityToOpen][facilityToClose]
  //           << std::endl;
  return temCost;
}


double SideConstraintInstance::getAllCost(int facilityToClose, int facilityToOpen, std::vector<double>& currentCosts){
  double totalCost = getOneCost(facilityToClose, facilityToOpen, mySolution, statistics, currentCosts[0]); 
  double temSideCost;
  for(int i=0; i<numSideConst; i++){
    temSideCost = getOneCost(facilityToClose, facilityToOpen, sideConstraintSolutions[i],
                             sideConstraintStatistics[i], currentCosts[i+1]);
    totalCost += normalizeCost(i, temSideCost); 
  }
  return totalCost;
}

bool SideConstraintInstance::findBestPair(int& bestInsert, int& bestRemove, bool steepest){
  //std:: cout << " Starting findBestPair" << std::endl;
  double baselineCost = getCost();
  double initialCost = baselineCost;
  //std::cerr <<" baseline cost is: "<< baselineCost<<std::endl;
  double temCost;
  bool isImprovement = false;

  static std::vector<double> currentCosts;
  if ( currentCosts.size() != numSideConst + 1 )
     currentCosts.resize(numSideConst + 1, 0.0);

  //
  // Iterate over all facilities that we can close
  //
  for(int facilityToClosePosition=1; facilityToClosePosition<=numP && (!isImprovement || steepest); facilityToClosePosition++){
    int facilityToClose = mySolution->getFacility(facilityToClosePosition);
    //
    // If the facility can be closed, then try closing it and see if there is an improvement that can be found.
    //
    if (facilityLocationConstraints[facilityToClose] != 1) {
       closeOneFac(facilityToClose);
       // Since the "system's" total cost is constant throughout the following 
       // exhaustive search for the best swap, pass the current total cost of the 
       // "system" (systems if there are side constraints) to getAllCost() so that 
       // the same value is NOT computed needlessly and endlessly.
       currentCosts[0] = mySolution->cost(gamma);
       for (int i=0; i<numSideConst; i++) 
          currentCosts[i+1] = sideConstraintSolutions[i]->cost(gamma);
       //
       // Iterate over all positions that could be opened.
       //
       for(int facilityToOpenPosition=numP+1; facilityToOpenPosition<=numM && (!isImprovement || steepest); facilityToOpenPosition++){
        int facilityToOpen = mySolution->getFacility(facilityToOpenPosition);
        if(facilityLocationConstraints[facilityToOpen]!=2) {
          temCost = getAllCost(facilityToClose, facilityToOpen, currentCosts); 
          //std::cerr << " Looking for improvement: " << facilityToClose << " " << facilityToOpen << " : " << temCost << " " << baselineCost << " : " << (temCost < baselineCost) << " " << isImprovement << std::endl;
          if (temCost<baselineCost){
            //std::cerr << "  improvement: " << facilityToClose << " " << facilityToOpen << " : " << temCost << " " << baselineCost << " : " << (temCost < baselineCost) << " " << isImprovement << std::endl;
            baselineCost = temCost;
            bestInsert = facilityToOpen;
            bestRemove = facilityToClose;
            isImprovement = true;
          }
        }
      }
      openOneFac(facilityToClose);
    }
  }
  //std::cout<<" best cost is: "<< baselineCost<<std::endl;
  //std::cout<<" one find---------------\n";
  return (isImprovement && (fabs(initialCost-baselineCost) > 1e-8));
}

void SideConstraintInstance::resetValue(){
  resetValue(mySolution, statistics);
  for(int i=0; i<numSideConst; i++)
    resetValue(sideConstraintSolutions[i], sideConstraintStatistics[i]);
} 

void SideConstraintInstance::resetValue(ProblemSolution* theSolution, 
                                        std::string theStatistics){
  if (theStatistics == "mean"){
    ((MeanSolution*) theSolution)->resetValue();
    ((MeanSolution*) theSolution)->updateTable();
  }
}


void SideConstraintInstance::updateMeanTable(int facToInsert, int facToRemove,
                                             ProblemSolution* theSolution,
                                             std::string theStatistics) {
  if (theStatistics == "mean"){
    ((MeanSolution*) theSolution)->updateAffected(facToInsert, facToRemove);
    ((MeanSolution*) theSolution)->updateOnce(-1);
  }
}

void SideConstraintInstance::updateMeanTable(int facToInsert, int facToRemove) {
  updateMeanTable(facToInsert, facToRemove, mySolution, statistics);
  for(int i=0; i<numSideConst; i++)
    updateMeanTable(facToInsert, facToRemove, 
                    sideConstraintSolutions[i], sideConstraintStatistics[i]);
  
}


void SideConstraintInstance::swapAll(int facToInsert, int facToRemove) {
  mySolution->swapFac(facToRemove, facToInsert, true);
  for(int i=0; i<numSideConst; i++)
    sideConstraintSolutions[i]->swapFac(facToRemove, facToInsert, true);
}

void SideConstraintInstance::updateMeanTable2(int facToInsert, int facToRemove) {
  if (statistics == "mean")
    ((MeanSolution*) mySolution)->updateOnce(1);
  for(int i=0; i<numSideConst; i++){
    if (sideConstraintStatistics[i] == "mean")
    ((MeanSolution*) sideConstraintSolutions[i])->updateOnce(1);
  }
}



void SideConstraintInstance::descend(){
  for(int i=0; i<numSideConst; i++)
    sideConstraintSolutions[i]->setSolution(*mySolution);
  resetValue(); // reset PreProcess table for compute pair costs.
  #if 0
  while(descendOnce());
  #else
  int facToInsert, facToRemove;
  bool steepest = statistics == "mean";
  while(findBestPair(facToInsert, facToRemove, steepest)){
    //std::cerr << "HERE" << facToInsert << " " << facToRemove << std::endl;
    updateMeanTable(facToInsert, facToRemove);
    swapAll(facToInsert, facToRemove);
    updateMeanTable2(facToInsert, facToRemove);
    //std::cout<<"swaped ----------"<<facToInsert<<" -----  "<<facToRemove<<std::endl;
  }
  #endif
}

double SideConstraintInstance::normalizeCost(int num, double temValue){
  if (temValue <= sideConstraintBounds[num]) 
    return 0.0;
  else 
    return ((temValue - sideConstraintBounds[num]) * weight * ObjectMax 
      / sideMax[num]);
}

double SideConstraintInstance::normalizeCost(int num){
  double temValue = sideConstraintSolutions[num]->cost(gamma);
  if (temValue <= sideConstraintBounds[num]) 
    return 0.0;
  else
    return ((temValue - sideConstraintBounds[num]) * weight * ObjectMax 
     / sideMax[num]);
}

bool SideConstraintInstance::descendOnce(){
  double baselineCost = mySolution->cost(gamma);
  for(int facilityToClosePosition=1; facilityToClosePosition<=numP; facilityToClosePosition++){
    int facilityToClose(mySolution->getFacility(facilityToClosePosition));
    if((facilityLocationConstraints)[facilityToClose]!=1) {
      mySolution->removeOneFac(facilityToClose,true);
      for(int facilityToOpenPosition=numP+1;facilityToOpenPosition<=numM;facilityToOpenPosition++){
        int facilityToOpen(mySolution->getFacility(facilityToOpenPosition));
        if((facilityLocationConstraints)[facilityToOpen]!=2) {
          double costOfMove = mySolution->moveEvaluator(facilityToOpen, gamma);
          if ((costOfMove<baselineCost)&&(isSideOk(facilityToOpen))){ 
          // see if add this facility give a better cost;
            baselineCost = costOfMove;
            mySolution->addOneFac(facilityToOpen,true); // if so, add it and return true;
            return true;
          }
        }
      }
      mySolution->addOneFac(facilityToClose, true);
    }
  }
  return false;
}

bool SideConstraintInstance::isSideOk(int facilityToOpen){
  bool isSatisfied = true;
  for(int i=0; i<numSideConst; i++){  
    sideConstraintSolutions[i]->setSolution(*mySolution);
    sideCost[i] = sideConstraintSolutions[i]->moveEvaluator(facilityToOpen, gamma);
    if (sideCost[i] > (sideConstraintBounds)[i])
      isSatisfied = false;
  }
  return isSatisfied;
}

void SideConstraintInstance::runSearch(double& initialCost){
  initialSolution(initialCost);
  //weight = 10000.0;
  //descend();
  //weight = 10.1;
  descend();
}

/* Old and slow version of descend, use descendOnce as helper.
void SideConstraintInstance::descend(){
  while (descendOnce())
    ;
}
*/

