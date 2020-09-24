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

#include "ImperfectInstance.h"
using namespace NewSensorPlacement;

ImperfectInstance::ImperfectInstance(ProblemSolution &sol, 
                 std::vector<std::vector<std::pair<int, double> > > &arrivalSeq,
                 std::vector<std::map<int, double> > &arrivalTime,
                 myInt1& constraints,
                 std::vector<double> &detectionPro):
  mySolution(sol),
  nodeArrivalSequence(arrivalSeq),
  nodeArrivalTime(arrivalTime),
  facilityLocationConstraints(constraints),
  detectionProbability(detectionPro){
  myInstance = mySolution.getInstance();
  numN = myInstance->getN();
  numM = myInstance->getM();
  numP = myInstance->getP();
  scenarioImpacts = std::vector<double>(numN+1,0.0);
  sensorArrivalTime = std::vector<std::vector<NodeTimePair> >(numN+1);
}

ImperfectInstance::~ImperfectInstance(){
}

double ImperfectInstance::moveEvaluator(int newFacility, int theScenario){
  double newArrivalTime = myInstance->getArrivalTime(theScenario, newFacility);
  std::vector<NodeTimePair>::const_iterator currentIter;
  std::vector<NodeTimePair>::const_iterator endIter(sensorArrivalTime[theScenario].end());
  double scenarioExpectedImpact(0.0);
  double scenarioRemainingProbability(1.0);
  bool newFacilityHandled(false);
  for(currentIter=sensorArrivalTime[theScenario].begin();currentIter!=endIter;){
    double thisArrivalTime((*currentIter).ArrivalTime);
    double thisImpact(myInstance->getDist(theScenario,(*currentIter).Node));
    double failedDetectionProbability(1.0);
    if(newFacilityHandled==false){
      if(newArrivalTime<thisArrivalTime) {
        // this is the only sensor that can detect during this time.
        // the while loop will fail.
        thisArrivalTime=newArrivalTime;
        thisImpact=myInstance->getDist(theScenario,newFacility);
        failedDetectionProbability*=(1.0-detectionProbability[newFacility]);
        newFacilityHandled=true;
      }
      else if(newArrivalTime==thisArrivalTime){
        // this sensor detects at the same time as at least one other,
        // which will be caught during the while loop.
        newFacilityHandled=true;
        failedDetectionProbability*=(1.0-detectionProbability[newFacility]);
      }
    }
    while((currentIter!=endIter)&&(((*currentIter).ArrivalTime)==thisArrivalTime)){
      int thisNode((*currentIter).Node);
      failedDetectionProbability*=(1.0-detectionProbability[thisNode]);
      currentIter++;
    }
    double detectionProbability(1.0-failedDetectionProbability);
    scenarioExpectedImpact+=thisImpact*scenarioRemainingProbability*detectionProbability;
    scenarioRemainingProbability-=scenarioRemainingProbability*detectionProbability;
  }
  // handle the case in which the new sensor is after all others
  if(newFacilityHandled==false){
    double prob(detectionProbability[newFacility]);
    scenarioExpectedImpact+=myInstance->getDist(theScenario,newFacility)*scenarioRemainingProbability*prob;
    scenarioRemainingProbability-=scenarioRemainingProbability*prob;
  }
 // handle the dummy node as the final case
  scenarioExpectedImpact+=(myInstance->getUndetectedImpact(theScenario)*scenarioRemainingProbability);
  return scenarioExpectedImpact;
}


double ImperfectInstance::moveEvaluator(int facToOpen){
  double sumExpectedImpacts(0.0);
  for(int i=1;i<=numN;i++) {
    if(myInstance->getDist(i,facToOpen)==myInstance->getUndetectedImpact(i))
      sumExpectedImpacts+=scenarioImpacts[i];
      // the new sensor can't detect this scenario,
      // so just use the pre-computed/cached value.
    else{
      double val(moveEvaluator(facToOpen, i));
      sumExpectedImpacts+=val;
    }
  }
  return sumExpectedImpacts;
}


double ImperfectInstance::cost(int theScenario){
  // scan the nodes with sensors in order of arrival time. this is required to correctly 
  // compute the probability of failing to detect at a node (temporally) "upstream". the 
  // logic is somewhat complicated due to the fact that multiple sensors might be able to 
  // detect an injection at any given time => the aggregate probability of detection/remaining
  // is a function of all candidate sensor detection probabilities.
  std::vector<NodeTimePair>::const_iterator currentIter;
  std::vector<NodeTimePair>::const_iterator endIter(sensorArrivalTime[theScenario].end());
  double scenarioExpectedImpact(0.0);
  double scenarioRemainingProbability(1.0);
  for(currentIter=sensorArrivalTime[theScenario].begin();currentIter!=endIter;){
    double thisArrivalTime((*currentIter).ArrivalTime);
    double thisImpact(myInstance->getDist(theScenario,(*currentIter).Node));
    double failedDetectionProbability(1.0);
    while((currentIter!=endIter)&&(((*currentIter).ArrivalTime)==thisArrivalTime)){
      int thisNode((*currentIter).Node);
      failedDetectionProbability*=(1.0-detectionProbability[thisNode]);
      currentIter++;
    }
    double detectionProbability(1.0-failedDetectionProbability);
    scenarioExpectedImpact+=thisImpact*scenarioRemainingProbability*detectionProbability;
    scenarioRemainingProbability-=scenarioRemainingProbability*detectionProbability;
  }
  // handle the dummy node as the final case
  scenarioExpectedImpact+=(myInstance->getUndetectedImpact(theScenario)*scenarioRemainingProbability);
  return scenarioExpectedImpact;
}


double ImperfectInstance::cost(){
  double sumExpectedImpacts(0.0);
  for(int i=1;i<=numN;i++) {
    double scenarioExpectedImpact(0.0);
    double scenarioRemainingProbability(1.0);
    // scan the nodes in arrival sequence. this is required to correctly compute the
    // probability of failing to detect at a node (temporally) "upstream". the logic
    // is somewhat complicated due to the fact that multiple sensors might be able to 
    // detect an injection at any given time => the aggregate probability of detection/remaining
    // is a function of all candidate sensor detection probabilities.
    const std::vector<std::pair<int,double> > &thisArrivalSequence=nodeArrivalSequence[i];
    for(size_t j=0;j<thisArrivalSequence.size();){
      double arrivalTime(thisArrivalSequence[j].second);
      bool detectionPossible(false); // until proven otherwise (used to avoid epsilon comparison issues with doubles)
      double failedDetectionProbability(1.0);
      double cumulativeImpact;
      if(thisArrivalSequence[j].first==-1)
        cumulativeImpact=myInstance->getUndetectedImpact(i);
      else
        cumulativeImpact=myInstance->getDist(i,thisArrivalSequence[j].first);
      while((j<thisArrivalSequence.size())&&(thisArrivalSequence[j].second==arrivalTime)){
        int thisNode(thisArrivalSequence[j].first);
        if(thisNode==-1) {
          // the dummy node acts as a perfect sensor
          failedDetectionProbability=0.0;
          detectionPossible=true;
        }
        else if(mySolution.isOpenFac(thisNode)==true) {
          failedDetectionProbability*=(1.0-detectionProbability[thisNode]);
          detectionPossible=true;
        }
        j++;
      }
      // if the injection could be detected with any non-zero probability at
 // this node, update the expected impact and remaining probabilities.
      if(detectionPossible==true)
      {
        scenarioExpectedImpact+=cumulativeImpact*scenarioRemainingProbability*(1.0-failedDetectionProbability);
        scenarioRemainingProbability-=scenarioRemainingProbability*(1.0-failedDetectionProbability);
      }
    }
    sumExpectedImpacts+=scenarioExpectedImpact;
  }
  return sumExpectedImpacts;
}       


void ImperfectInstance::computeSensorArrivalTime(){
  for(int i=1;i<=numN;i++){
    const std::map<int,double> &theseArrivalTime=nodeArrivalTime[i];
    sensorArrivalTime[i].clear();
    for(int j=1;j<=numP-1;j++){
      int thisNode(mySolution.getFacility(j));
      std::map<int,double>::const_iterator iter=theseArrivalTime.find(thisNode);
      if(iter!=theseArrivalTime.end())
        sensorArrivalTime[i].push_back(NodeTimePair(thisNode,(*iter).second));
    }
    std::sort(sensorArrivalTime[i].begin(),sensorArrivalTime[i].end());
  }
}


void ImperfectInstance::computeImpact(){
  for(int i=1;i<=numN;i++)
    scenarioImpacts[i] = cost(i);
}

void ImperfectInstance::descend(double& startCost, double& finalCost){
  startCost = cost();
  startCost = double(startCost)/double(numN);
  int numDescentIterations(0);
  bool improvementFound(true); // to trip the loop
  while(improvementFound==true){
    numDescentIterations++;
    //std::cout << "New iteration=" << numDescentIterations << std::endl;
    double baselineCost(cost());
    //std::cout << "Baseline cost=" << double(baselineCost)/double(numN) << std::endl;
    improvementFound=false; // until proven otherwise
    double bestMoveCost(baselineCost);
    int bestMoveClose(-1); // dummy initialize to prevent compiler warning
    int bestMoveOpen(-1);  // ditto
    for(int facilityToClosePosition=1;facilityToClosePosition<=numP;facilityToClosePosition++){
      int facilityToClose(mySolution.getFacility(facilityToClosePosition));
      if(facilityLocationConstraints[facilityToClose]!=1){
        // the "source" facility isn't constrained to be open
        // remove the facility from the solution and update the distances accordingly
        mySolution.removeOneFac(facilityToClose, false);
        // at this point, there are only p-1 open facilities in the solution
        computeSensorArrivalTime();              
        // compute the per-scenario expected impacts.
        computeImpact();       
        // recall that the first p facilities in a solution are all open.
        // the remaining p+1..m are closed. 
        for(int facilityToOpenPosition=numP+1;(facilityToOpenPosition<=numM);facilityToOpenPosition++){
          int facilityToOpen(mySolution.getFacility(facilityToOpenPosition));
          if(facilityLocationConstraints[facilityToOpen]!=2){
            // the "destination" facility isn't constrained to be closed
            double costOfMove(moveEvaluator(facilityToOpen));
            if(costOfMove<bestMoveCost) {
              improvementFound=true;
              //std::cout << "Improvement is best-so-far - cost=" << costOfMove/double(numN) << std::endl;
              bestMoveCost=costOfMove;
              bestMoveClose=facilityToClose;
              bestMoveOpen=facilityToOpen;
            }
          }
        }    
        mySolution.addOneFac(facilityToClose, false); 
      }
    }
    if(improvementFound==true)
      mySolution.swapFac(bestMoveClose,bestMoveOpen, false);
  }
  std::cout << "Total number of iterations=" << numDescentIterations << std::endl;
  finalCost = cost();
  finalCost = double(finalCost)/double(numN);
}

void ImperfectInstance::runSearch(double& startCost, double& finalCost){
  mySolution.reset();
  InitialSolution myIniSol;
  myIniSol.GreedySampling(&mySolution, numP, 2, facilityLocationConstraints);
  mySolution.descend(0, facilityLocationConstraints);
  descend(startCost, finalCost);
}



