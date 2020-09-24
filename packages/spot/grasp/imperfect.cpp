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

#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>

#include <string>
#include <map>
#include <set>

#include <stdlib.h>
#include <float.h>

#include "constructive.h"
#include "solution.h"
#include "instance.h"
#include "impact_instance.h"
#include "instance_factory.h"
#include "search.h"

#include <utilib/seconds.h>
#include <sp/SPProblem.h>

#include "redefs.h"

class NodeTimePair
{
public:

  int Node;
  double ArrivalTime;

  NodeTimePair(int node, double arrivalTime):
    Node(node),
    ArrivalTime(arrivalTime)
  {
  }

  bool operator<(const NodeTimePair &other)const
  {
    if(ArrivalTime<other.ArrivalTime)
      {
	return true;
      }
    else if(ArrivalTime==other.ArrivalTime)
      {
	return Node<other.Node;
      }
    else
      {
	return false;
      }
  }
};

double computeScenarioExpectedImpact(const PMSolution &theSolution,
				     int theScenario,
				     const std::vector<std::vector<std::pair<int,double> > > &arrivalSequence,
				     const std::vector<double> &detectionProbability)
{
  // scan the nodes in terms of arrival sequence to compute the exepected impact

  PMImpactInstance *thisInstance(dynamic_cast<PMImpactInstance*>(theSolution.getInstance()));

  double scenarioExpectedImpact=0.0;
  double scenarioRemainingProbability=1.0;

  // scan the nodes in arrival sequence. this is required to correctly compute the
  // probability of failing to detect at a node (temporally) "upstream". the logic
  // is somewhat complicated due to the fact that multiple sensors might be able to 
  // detect an injection at any given time => the aggregate probability of detection/remaining
  // is a function of all candidate sensor detection probabilities.
  const std::vector<std::pair<int,double> > &thisArrivalSequence=arrivalSequence[theScenario];
  std::vector<std::pair<int,double> >::const_iterator endIter(thisArrivalSequence.end());
  for(std::vector<std::pair<int,double> >::const_iterator iter(thisArrivalSequence.begin());iter!=endIter;)
    {
      double arrivalTime((*iter).second);
      bool detectionPossible(false); // until proven otherwise (used to avoid epsilon comparison issues with doubles)
      double failedDetectionProbability(1.0);
      double cumulativeImpact;
      if((*iter).first==-1)
	{
	  cumulativeImpact=thisInstance->getUndetectedImpact(theScenario);
	}
      else
	{
	  cumulativeImpact=thisInstance->getDist(theScenario,(*iter).first);
	}

      while((iter!=endIter)&&(((*iter).second)==arrivalTime))
	{
	  int thisNode((*iter).first);
	  if(thisNode==-1)
	    {
	      // the dummy node acts as a perfect sensor
	      failedDetectionProbability=0.0;
	      detectionPossible=true;
	    }
	  else if(theSolution.contains(thisNode)==true)
	    {
	      failedDetectionProbability*=(1.0-detectionProbability[thisNode]);
	      detectionPossible=true;
	    }

	  iter++;
	}

      // if the injection could be detected with any non-zero probability at 
      // this node, update the expected impact and remaining probabilities.
      if(detectionPossible==true)
	{
	  double detectionProbability(1.0-failedDetectionProbability);
	  scenarioExpectedImpact+=cumulativeImpact*scenarioRemainingProbability*detectionProbability;
	  scenarioRemainingProbability-=scenarioRemainingProbability*detectionProbability;
	}
    }

  return scenarioExpectedImpact;
}

double computeScenarioExpectedImpact2(const PMSolution &theSolution,
				      int theScenario,
				      const std::vector<NodeTimePair> &sensorArrivalSequence,
				      const std::vector<double> &detectionProbability)
{
  PMImpactInstance *thisInstance(dynamic_cast<PMImpactInstance*>(theSolution.getInstance()));

  // scan the nodes with sensors in order of arrival time. this is required to correctly 
  // compute the probability of failing to detect at a node (temporally) "upstream". the 
  // logic is somewhat complicated due to the fact that multiple sensors might be able to 
  // detect an injection at any given time => the aggregate probability of detection/remaining
  // is a function of all candidate sensor detection probabilities.

  std::vector<NodeTimePair>::const_iterator currentIter;
  std::vector<NodeTimePair>::const_iterator endIter(sensorArrivalSequence.end());

  double scenarioExpectedImpact(0.0);
  double scenarioRemainingProbability(1.0);

  for(currentIter=sensorArrivalSequence.begin();currentIter!=endIter;)
    {
      double thisArrivalTime((*currentIter).ArrivalTime);
      double thisImpact(thisInstance->getDist(theScenario,(*currentIter).Node));

      double failedDetectionProbability(1.0);

      while((currentIter!=endIter)&&(((*currentIter).ArrivalTime)==thisArrivalTime))
	{
	  int thisNode((*currentIter).Node);
	  failedDetectionProbability*=(1.0-detectionProbability[thisNode]);
	  currentIter++;
	}

      double detectionProbability(1.0-failedDetectionProbability);
      scenarioExpectedImpact+=thisImpact*scenarioRemainingProbability*detectionProbability;
      scenarioRemainingProbability-=scenarioRemainingProbability*detectionProbability;
    }

  // handle the dummy node as the final case
  scenarioExpectedImpact+=(thisInstance->getUndetectedImpact(theScenario)*scenarioRemainingProbability);

  return scenarioExpectedImpact;
}

double computeScenarioExpectedImpact3(const PMSolution &theSolution,
				      int theScenario,
				      const std::vector<NodeTimePair> &sensorArrivalSequence, 
				      int newFacility,
				      double newFacilityArrivalTime,
				      const std::vector<double> &detectionProbability)
{
  PMImpactInstance *thisInstance(dynamic_cast<PMImpactInstance*>(theSolution.getInstance()));

  // scan the nodes with sensors in order of arrival time. this is required to correctly 
  // compute the probability of failing to detect at a node (temporally) "upstream". the 
  // logic is somewhat complicated due to the fact that multiple sensors might be able to 
  // detect an injection at any given time => the aggregate probability of detection/remaining
  // is a function of all candidate sensor detection probabilities.

  std::vector<NodeTimePair>::const_iterator currentIter;
  std::vector<NodeTimePair>::const_iterator endIter(sensorArrivalSequence.end());

  double scenarioExpectedImpact(0.0);
  double scenarioRemainingProbability(1.0);

  bool newFacilityHandled(false);

  for(currentIter=sensorArrivalSequence.begin();currentIter!=endIter;)
    {
      double thisArrivalTime((*currentIter).ArrivalTime);
      double thisImpact(thisInstance->getDist(theScenario,(*currentIter).Node));

      double failedDetectionProbability(1.0);

      if(newFacilityHandled==false)
	{
	  if(newFacilityArrivalTime<thisArrivalTime)
	    {
	      // this is the only sensor that can detect during this time.
	      // the while loop will fail.
	      thisArrivalTime=newFacilityArrivalTime;
	      thisImpact=thisInstance->getDist(theScenario,newFacility);
	      failedDetectionProbability*=(1.0-detectionProbability[newFacility]);
	      newFacilityHandled=true;
	    }
	  else if(newFacilityArrivalTime==thisArrivalTime)
	    {
	      // this sensor detects at the same time as at least one other,
	      // which will be caught during the while loop.
	      newFacilityHandled=true;
	      failedDetectionProbability*=(1.0-detectionProbability[newFacility]);
	    }
	}

      while((currentIter!=endIter)&&(((*currentIter).ArrivalTime)==thisArrivalTime))
	{
	  int thisNode((*currentIter).Node);
	  failedDetectionProbability*=(1.0-detectionProbability[thisNode]);
	  currentIter++;
	}

      double detectionProbability(1.0-failedDetectionProbability);
      scenarioExpectedImpact+=thisImpact*scenarioRemainingProbability*detectionProbability;
      scenarioRemainingProbability-=scenarioRemainingProbability*detectionProbability;
    }

  // handle the case in which the new sensor is after all others
  if(newFacilityHandled==false)
    {
      double prob(detectionProbability[newFacility]);
      scenarioExpectedImpact+=thisInstance->getDist(theScenario,newFacility)*scenarioRemainingProbability*prob;
      scenarioRemainingProbability-=scenarioRemainingProbability*prob;      
    }

  // handle the dummy node as the final case
  scenarioExpectedImpact+=(thisInstance->getUndetectedImpact(theScenario)*scenarioRemainingProbability);

  return scenarioExpectedImpact;
}

double computeImperfectCost(const PMSolution &theSolution,
			    const std::vector<std::vector<std::pair<int,double> > > &nodeArrivalSequences,
			    const std::vector<double> &detectionProbabilities)
{
  // for each injection scenario, scan the nodes in terms of arrival sequence to
  // compute a per-scenario exepected impact, which is then summed across alll scenarios.

  PMImpactInstance *thisInstance(dynamic_cast<PMImpactInstance*>(theSolution.getInstance()));

  double sumExpectedImpacts(0.0);

  int n(thisInstance->getN());

  for(int i=1;i<=n;i++)
    {
      double scenarioExpectedImpact(0.0);
      double scenarioRemainingProbability(1.0);

      // scan the nodes in arrival sequence. this is required to correctly compute the
      // probability of failing to detect at a node (temporally) "upstream". the logic
      // is somewhat complicated due to the fact that multiple sensors might be able to 
      // detect an injection at any given time => the aggregate probability of detection/remaining
      // is a function of all candidate sensor detection probabilities.
      const std::vector<std::pair<int,double> > &thisArrivalSequence=nodeArrivalSequences[i];
      for(size_t j=0;j<thisArrivalSequence.size();)
	{
	  double arrivalTime(thisArrivalSequence[j].second);
	  bool detectionPossible(false); // until proven otherwise (used to avoid epsilon comparison issues with doubles)
	  double failedDetectionProbability(1.0);
	  double cumulativeImpact;
	  if(thisArrivalSequence[j].first==-1)
	    {
	      cumulativeImpact=thisInstance->getUndetectedImpact(i);
	    }
	  else
	    {
	      cumulativeImpact=thisInstance->getDist(i,thisArrivalSequence[j].first);
	    }

	  while((j<thisArrivalSequence.size())&&(thisArrivalSequence[j].second==arrivalTime))
	    {
	      int thisNode(thisArrivalSequence[j].first);		  
	      if(thisNode==-1)
		{
		  // the dummy node acts as a perfect sensor
		  failedDetectionProbability=0.0;
		  detectionPossible=true;
		}
	      else if(theSolution.contains(thisNode)==true)
		{
		  failedDetectionProbability*=(1.0-detectionProbabilities[thisNode]);
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

double computeImperfectCost2(const PMSolution &theSolution,
			     int facilityClosed,
			     int facilityOpened,
			     const std::vector<double> &scenarioImpacts,
			     std::vector<std::vector<NodeTimePair> > &sensorArrivalTimes,
			     const std::vector<double> &detectionProbabilities)
{
  PMImpactInstance *thisInstance(dynamic_cast<PMImpactInstance*>(theSolution.getInstance()));

  double sumExpectedImpacts(0.0);

  int n(thisInstance->getN());

  for(int i=1;i<=n;i++)
    {
      if(thisInstance->getDist(i,facilityOpened)==thisInstance->getUndetectedImpact(i))
	{
	  // the new sensor can't detect this scenario,
	  // so just use the pre-computed/cached value.
	  sumExpectedImpacts+=scenarioImpacts[i];
	}
      else
	{
	  double val(computeScenarioExpectedImpact3(theSolution,
						    i,
						    sensorArrivalTimes[i],
						    facilityOpened,
						    thisInstance->getArrivalTime(i,facilityOpened),
						    detectionProbabilities));
	  sumExpectedImpacts+=val;


	}
    }

  return sumExpectedImpacts;
}

PMSolution *descend(PMSolution &startingPoint,
		    std::vector<std::map<int,double> > &nodeArrivalTimes,
		    const std::vector<std::vector<std::pair<int,double> > > &nodeArrivalSequences,
		    const std::vector<double> &detectionProbabilities,
		    const std::vector<int> &facilityLocationConstraints)
{
  //  std::cout << "Starting descent" << std::endl;

  // this function assumes that all fixed/invalid constraints 
  // are satisfied in the input solution. 

  PMSolution *result=new PMSolution(startingPoint.getInstance());
  result->copyFrom(startingPoint);

  int p(result->getP());
  int n(result->getN()); // customers/scenarios
  int m(result->getM()); // facilities/junctions

  std::vector<double> scenarioImpacts(n+1,0.0); // the 0th entry is unused.
  std::vector<std::vector<NodeTimePair> > sensorArrivalTimes(n+1);

  int numDescentIterations(0);

  bool improvementFound(true); // to trip the loop
  while(improvementFound==true)
    {
      numDescentIterations++;
      std::cout << "New iteration=" << numDescentIterations << std::endl;

      double baselineCost(computeImperfectCost(*result,
					       nodeArrivalSequences,
					       detectionProbabilities));

      std::cout << "Baseline cost=" << double(baselineCost)/double(n) << std::endl;

      improvementFound=false; // until proven otherwise

      double bestMoveCost(baselineCost);
      int bestMoveClose(-1); // dummy initialize to prevent compiler warning
      int bestMoveOpen(-1);  // ditto

      for(int facilityToClosePosition=1;
	  facilityToClosePosition<=p;
	  facilityToClosePosition++)
	{
	  int facilityToClose(result->getFacility(facilityToClosePosition));

	  if(facilityLocationConstraints[facilityToClose]!=1)
	    {
	      // the "source" facility isn't constrained to be open

	      // remove the facility from the solution and update the distances accordingly
	      PMSolution baselineSolution(result->getInstance());
	      baselineSolution.copyFrom(*result);

	      baselineSolution.remove(facilityToClose,false); 

	      // at this point, there are only p-1 open facilities in the solution

	      // compute node/arrival time sets for each scenario, considering 
	      // only those nodes at which sensors are placed. this must be 
	      // done before the per-scenario impacts are computed.
	      for(int i=1;i<=n;i++)
		{
		  const std::map<int,double> &theseArrivalTimes=nodeArrivalTimes[i];
		  sensorArrivalTimes[i].clear();
		  for(int j=1;j<=p-1;j++)
		    {
		      int thisNode(baselineSolution.getFacility(j)); 
		      std::map<int,double>::const_iterator iter=theseArrivalTimes.find(thisNode);
		      if(iter!=theseArrivalTimes.end())
			{
			  sensorArrivalTimes[i].push_back(NodeTimePair(thisNode,(*iter).second));
			}
		    }
		  std::sort(sensorArrivalTimes[i].begin(),sensorArrivalTimes[i].end());
		}

	      // compute the per-scenario expected impacts.
	      for(int i=1;i<=n;i++)
		{
		  scenarioImpacts[i]=computeScenarioExpectedImpact2(baselineSolution,
								    i,
								    sensorArrivalTimes[i],
								    detectionProbabilities);
		}

	      // recall that the first p facilities in a solution are all open.
	      // the remaining p+1..m are closed. 
	      for(int facilityToOpenPosition=p+1; // you've closed all but one facility (TBD - add explanation of p-th is the one you just closed!
		  (facilityToOpenPosition<=m);
		  facilityToOpenPosition++)
		{
		  int facilityToOpen(result->getFacility(facilityToOpenPosition));

		  if(facilityLocationConstraints[facilityToOpen]!=2)
		    {		  
		      // the "destination" facility isn't constrained to be closed

		      double costOfMove(computeImperfectCost2(baselineSolution,
							      facilityToClose,
							      facilityToOpen,
							      scenarioImpacts,
							      sensorArrivalTimes,
							      detectionProbabilities));
		      if(costOfMove<bestMoveCost)
			{
			  improvementFound=true;
			  std::cout << "Improvement is best-so-far - cost=" << costOfMove/double(n) << std::endl;
			  bestMoveCost=costOfMove;
			  bestMoveClose=facilityToClose;
			  bestMoveOpen=facilityToOpen;
			}
		    }
		}
	    }
	}

      if(improvementFound==true)
	{
	  result->swap(bestMoveClose,bestMoveOpen,false);
	}
    }

  std::cout << "Total number of iterations=" << numDescentIterations << std::endl;

  return result;
}

int main(int argc, char **argv)
{
  if((argc!=8)&&
     (argc!=9))
    {
      std::cout << "***Incorrect invocation - Usage is: imperfect <configuration-file> <category-definition-input-file> <location-category-input-file> <num-samples> <random-seed> <impact-file-representation> <time-limit> [solution-file]" << std::endl;
      return 0;
    }

  std::cout << "Executable: imperfect" << std::endl;

  InitializeTiming();
  double CPUSeconds_start = CPUSeconds();
  double WallClockSeconds_start = WallClockSeconds();

  //////////////////////////////////////////////////////////////
  // all problem-specific (as opposed to algorithm-specific)  //
  // parameters are read from the SPProblem class, which is   //
  // an interface to the configuration text file generated by //
  // the 'sp' solver script. the sole current exceptions are  //
  // the sensor category and location definition files, which //
  // should eventually migrate to the SPProblem class.        //
  //////////////////////////////////////////////////////////////

  SPProblem theProblem;
  std::ifstream configFile(argv[1]);
  if(!configFile)
    {
      std::cout << "***Failed to open configuration file=" << argv[1] << std::endl;
      return 0;
    }
  theProblem.read_config_file(configFile);
  configFile.close();

  std::cout << "Configuration file=" << argv[1] << std::endl;

  ///////////////////////////////////////////////////////
  // determine the objective, statistic, and number of //
  // facilities to open (i.e., sensors to place) from  //
  // the overall problem definition.                   //
  ///////////////////////////////////////////////////////

  double gamma(theProblem.gamma);

  std::string impactFileName;
  std::string objective;
  std::string statistic;
  int numFacilitiesToOpen(-1);  

  for(size_t i=0;i<theProblem.goals.size();++i)
    {
      SPGoalInfo &thisGoal=theProblem.goals[i];
      // only take the first goal
      if((thisGoal.objective_flag==true)&&(objective==""))
	{
	  objective=thisGoal.goal;
	  impactFileName=thisGoal.impact_file;
	  statistic=thisGoal.measure.front();
	}
      else 
	{
	  if((thisGoal.goal=="ns")&&(numFacilitiesToOpen==-1))
	    {
	      numFacilitiesToOpen=int(rint(thisGoal.bound.front()));
	    }
	}
    }

  /////////////////////////////////////////////////////////
  // validate the problem-specific parameters. shouldn't //
  // be necessary, but you never know...                 //
  /////////////////////////////////////////////////////////
  
  std::cout << "Max number of facilities to open=" << numFacilitiesToOpen << std::endl;
  
  if(numFacilitiesToOpen<=0)
    {
      std::cout << "***The value of p must be >= 1" << std::endl;
      return 0;
    }
  std::cout << "P=" << numFacilitiesToOpen << std::endl;

  if(objective=="")
    {
      std::cout << "***No objective was specified" << std::endl;
      return 0;
    }

  if(statistic=="")
    {
      std::cout << "No statistic was specified" << std::endl;
      return 0;
    }

  std::cout << "Objective=" << objective << std::endl;

  ////////////////////////////////////////////////////////////////////
  // determine whether a sparse matrix (0) or dense matrix (1)      //
  // representation will be used to store the impact file coontents //
  ////////////////////////////////////////////////////////////////////

  int impactFileRepresentation(atoi(argv[6]));
  if((impactFileRepresentation!=0)&&
     (impactFileRepresentation!=1))
    {
      std::cout << "***Unknown impact file representation=" << impactFileRepresentation << std::endl;
      return 0;
    }
  std::cout << "Impact file representation=" << impactFileRepresentation << std::endl;

  //////////////////////////
  // load the impact file //
  //////////////////////////

  std::cout << "Instance filename=" << impactFileName << std::endl;
  PMImpactInstance *instance = new PMImpactInstance;

  FILE *inputImpactFile=fopen(impactFileName.c_str(),"r");
  if(!inputImpactFile)
    {
      std::cout << "***Failed to read problem instance=" << inputImpactFile << std::endl;
    }

  if(impactFileRepresentation==0)
    {
      // employ sparse storage scheme
      instance->enableSparseRepresentation();
    }
  else
    {
      // employ dense storage scheme
      instance->enableDenseRepresentation();
    }

  // always needed for imperfect sensors
  instance->enableArrivalSequenceStorage();

  instance->readImpact(inputImpactFile,numFacilitiesToOpen,theProblem.feasible_location,theProblem.reduced_repn_map);
  fclose(inputImpactFile);

  // post-impact validations...
  if(numFacilitiesToOpen > instance->getM())
    {
      numFacilitiesToOpen = instance->getM();
      //std::cout << "***The value of p must be <= the number of potential facilities" << std::endl;
      //return 0;
    }

  ////////////////////////////////////////////////////////////////
  // load and validate all of the facility location constraints //
  ////////////////////////////////////////////////////////////////

  const utilib::BasicArray<int> fixeds=theProblem.fixed_placements;

  // validate the fixed placements - ideally, SPProblem should
  // do this, but the combination of it and the sp script 
  // aren't ensuring this as of yet.
  for(size_t i=0;i<fixeds.size();++i)
    {
      if((fixeds[i]<=0)||(fixeds[i]>instance->getM()))
	{
	  std::cout << "***Illegal fixed node index specified, value=" << fixeds[i] << std::endl;
	  return 0;
	}
    }

  std::cout << "Fixed placements(" << fixeds.size() << ")=";
  for(size_t i=0;i<fixeds.size();++i)
    {
      std::cout << fixeds[i] << " ";
    }
  std::cout << std::endl;

  const utilib::BasicArray<int> invalids=theProblem.invalid_placements;

  // validate the invalid placements - ideally, SPProblem should
  // do this, but the combination of it and the sp script aren't 
  // ensuring this as of yet.
  for(size_t i=0;i<invalids.size();++i)
    {
      if((invalids[i]<=0)||(invalids[i]>instance->getM()))
	{
	  std::cout << "***Illegal invalid node index specified, value=" << invalids[i] << std::endl;
	  return 0;
	}
    }

  std::cout << "Invalid placements(" << invalids.size() << ")=";
  for(size_t i=0;i<invalids.size();++i)
    {
      std::cout << invalids[i] << " ";
    }
  std::cout << std::endl;

  int effectiveP(numFacilitiesToOpen-fixeds.size());
  if(effectiveP<0)
    {
      std::cout << "***The number of fixed sensor locations must be <= p" << std::endl;
      return 0;
    }
  std::cout << "Effective P=" << effectiveP << std::endl;

  ////////////////////////////////////////////////////////////
  // load the category definition file, which specifies the //
  // probability of detection for a hypothetical sensor     //
  // placed at a node of a given category                   //
  ////////////////////////////////////////////////////////////

  std::map<int,double> detectionProbabilities;
  std::ifstream detectionProbabilitiesInputFile(argv[2]);
  if(!detectionProbabilitiesInputFile)
    {
      std::cout << "***Failed to open category definition input file=" << argv[2] << std::endl;
      return 0;
    }

  int category;
  double probability;
  detectionProbabilitiesInputFile >> category >> probability;
  while(detectionProbabilitiesInputFile)
    {
      if(detectionProbabilities.find(category)!=detectionProbabilities.end())
	{
	  std::cout << "Multiple entries for category=" << category << " were defined - all but the first are ignored" << std::endl;
	}
      else
	{
	  detectionProbabilities[category]=probability;
	}
      detectionProbabilitiesInputFile >> category >> probability;
    }

  detectionProbabilitiesInputFile.close();

  std::cout << "Number of detection categories=" << detectionProbabilities.size() << std::endl;

  /////////////////////////////////////////////////////////////
  // load the location->sensor map, which assigns a category //
  // to each potential sensor location                       //
  /////////////////////////////////////////////////////////////

  std::map<int,int> locationCategory;
  std::ifstream locationCategoryInputFile(argv[3]);
  if(!locationCategoryInputFile)
    {
      std::cout << "***Failed to open location category input file=" << argv[3]  << std::endl;
      return 0;
    }

  int location;
  locationCategoryInputFile >> location >> category;
  while(locationCategoryInputFile)
    {
      if(locationCategory.find(location)!=locationCategory.end())
	{
	  std::cout << "Multiple categories for location=" << location << " were defined - all but the first are ignored" << std::endl;
	}
      else if(detectionProbabilities.find(category)==detectionProbabilities.end())
	{
	  std::cout << "Category=" << category << " (location=" << location << ") was not defined in the category definition input file" << std::endl;
	}
      else
	{
	  locationCategory[location]=category;
	}
      locationCategoryInputFile >> location >> category;      
    }

  locationCategoryInputFile.close();

  std::cout << "Number of location-to-category entries=" << locationCategory.size() << std::endl;

  if(int(locationCategory.size())!=instance->getM())
    {
      std::cout << "A category must be assigned to each location" << std::endl;
      return 0;
    }

  // create a compiled version of the sensor location probability of detection
  std::vector<double> detectionProbabilitiesCompiled(instance->getM()+1,0.0);
  for(int i=1;i<=instance->getM();i++)
    {
      detectionProbabilitiesCompiled[i]=detectionProbabilities[locationCategory[i]];
    }

  /////////////////////////////////////////////////////////
  // load and validate any algorithm-specific parameters //
  /////////////////////////////////////////////////////////

  int numSamples(atoi(argv[4]));
  if(numSamples<=0)
    {
      std::cout << "***The number of samples must be >= 1" << std::endl;
      return 0;
    }
  std::cout << "Number of requested samples=" << numSamples << std::endl;

  ////////////////////////////////////////////
  // initialize the random number generator //
  ////////////////////////////////////////////

  int randomSeed(atoi(argv[5]));
  if(randomSeed<=0)
    {
      std::cout << "***The random number seed must be >= 1" << std::endl;
      return 0;
    }
  BossaRandom::randomize(randomSeed);
  std::cout << std::endl << "Random number seed=" << randomSeed << std::endl << std::endl;  

  ////////////////////////////////////
  // set up the runtime limit //
  ////////////////////////////////////

  double timeLimit(atof(argv[7]));
  if(timeLimit<0.0)
    {
      std::cout << "***The time limit " << argv[7] << " must be >= 0.0" << std::endl;
      return 0;
    }
  std::cout << std::endl << "WallClock Time Limit=" << timeLimit << std::endl << std::endl;

  ////////////////////////////////////////////////////////
  // construct the facility location constraint vector. //
  // the array is 1-based, like the facility indices    //
  // 0=no constraints, free to place facility           //
  // 1=fixed facility location                          //
  // 2=invalid facility location                        //
  ////////////////////////////////////////////////////////

  std::vector<int> facilityLocationConstraints(theProblem.num_nodes+1,0);
  for(size_t i=0;i<fixeds.size();++i)
    {
      facilityLocationConstraints[fixeds[i]]=1;
    }
  for(size_t i=0;i<invalids.size();++i)
    {
      if(facilityLocationConstraints[invalids[i]]!=0)
	{
	  std::cout << "***WARNING - Over-riding fixed facility location index=" << invalids[i] << " with an invalid constraint" << std::endl;
	}
      facilityLocationConstraints[invalids[i]]=2;
    }

  //////////////////////////////////////////////////
  // initialize the constructive algorithm, which //
  // currently isn't anything special.            //
  //////////////////////////////////////////////////

  PMConstructive constructiveAlgorithm; 
  constructiveAlgorithm.setMethod("pgreedy:0.75"); 

  /////////////////////////////////////////////////////////////////////
  // compile the node arrival sequences into a map-based lookup form //
  /////////////////////////////////////////////////////////////////////

  const std::vector<std::vector<std::pair<int,double> > > &nodeArrivalSequences((static_cast<PMImpactInstance*>(instance))->arrivalSequences());
  std::vector<std::map<int,double> > nodeArrivalTimes(instance->getN()+1);
  for(int i=1;i<=instance->getN();i++)
    {
      const std::vector<std::pair<int,double> > &thisSequence(nodeArrivalSequences[i]);
      for(size_t j=0;j<thisSequence.size();j++)
	{
	  nodeArrivalTimes[i][thisSequence[j].first]=thisSequence[j].second;
	}
    }

  //////////////////////////////////////////////
  // start the main routine, which is nothing //
  // but iterated hill-climbing.              //
  //////////////////////////////////////////////

  std::cout << std::endl;
  std::cout << "Starting iterated descent" << std::endl;
  std::cout << std::endl;

  double runtime_start = CPUSeconds();
  double minCost(DBL_MAX);
  double sumCosts(0.0);
  double maxCost(0.0);

  PMSolution *bestSolution=new PMSolution(instance);

  const int STATUS_INTERVAL=1000;
  for(int i=1;i<=numSamples;++i)
    {
      if((i%STATUS_INTERVAL)==0)
	{
	  std::cout << "Generated " << i << " samples so far" << std::endl;
	}

      PMSolution *initialPoint=new PMSolution(instance);
      initialPoint->reset();
      if(i==1)
	{
	  // in practice (at least for the EPA variant of the problem),
	  // initialization from the mean solution tends to yield an
	  // optimal imperfect sensor solution after descent.
	  constructiveAlgorithm.run(initialPoint,numFacilitiesToOpen,facilityLocationConstraints);
	  // "f" is for interchange (slower)
	  // "s" is for sparse matrix (fastest)
	  PMSearch::runLocalSearch(initialPoint,"s",facilityLocationConstraints);
	}
      else
	{
	  // for any remaining samples, start from a non-local 
	  // optimum generated via the constructive heuristic.
	  constructiveAlgorithm.run(initialPoint,numFacilitiesToOpen,facilityLocationConstraints);
	}
      double initialCost(computeImperfectCost(*initialPoint,
					      nodeArrivalSequences,
					      detectionProbabilitiesCompiled));

      PMSolution *localOptimum(descend(*initialPoint,
				       nodeArrivalTimes,
				       nodeArrivalSequences,
				       detectionProbabilitiesCompiled,
				       facilityLocationConstraints));
      
      double localOptimumCost(computeImperfectCost(*localOptimum,
						   nodeArrivalSequences,
						   detectionProbabilitiesCompiled));

      int numFacs=initialPoint->getP();
      std::vector<int> locations;
      locations.reserve(numFacs);
      for(int j=1;j<=numFacs;j++)
	{
	  locations.push_back(localOptimum->getFacility(j));
	}
      std::sort(locations.begin(),locations.end());

      std::cout << std::setw(4) << i << " [" << double(initialCost)/double(instance->getN()) << "] -> [" << double(localOptimumCost)/double(instance->getN()) << "]  Solution=[ ";
      for(size_t j=0;j<locations.size();j++)
	{
	  std::cout << locations[j] << " ";	  
	}
      std::cout << "]";

      if(localOptimumCost<minCost)
	{
	  minCost=localOptimumCost;
	  bestSolution->copyFrom(*localOptimum);
	  std::cout << "   <<<"; // denote the new best-so-far

          // output facility locations to file, if requested
          if(argc==9)
            {
              int numFacs=bestSolution->getP();
              std::vector<int> locations;
              locations.reserve(numFacs);
              for(int i=1;i<=numFacs;++i)
	        {
	          locations.push_back(bestSolution->getFacility(i));
	        }
              std::sort(locations.begin(),locations.end());

              std::ofstream facilityOutputFile(argv[8]);
              if(!facilityOutputFile)
	        {
	          std::cerr << "***Failed to open facility output file=" << argv[8] << std::endl;
	          return 0;
	        }

              facilityOutputFile << "#" << std::endl;
              facilityOutputFile << "# Generated by GRASP solver interface" << std::endl;
              facilityOutputFile << "#" << std::endl;
        
              facilityOutputFile << rand() << " "; // the ID
              facilityOutputFile << numFacs << " "; // the number of sensors
              for(size_t i=0;i<locations.size();++i)
	        {
	          facilityOutputFile << locations[i] << " ";	  
	        }
              facilityOutputFile << std::endl;
        
              facilityOutputFile.close();

              std::cout << "Best solution written to file=" << argv[8] << std::endl;
            }
	}

      sumCosts+=localOptimumCost;

      if(initialCost>maxCost)
	{
	  maxCost=localOptimumCost;
	}

      std::cout << std::endl;

      if ((timeLimit > 0.0) && ((WallClockSeconds() - WallClockSeconds_start) > timeLimit)) break;
    }

  std::cout << std::endl;
  double now_time = CPUSeconds();
  std::cout << "CPU RunTime=         " << (now_time - runtime_start) << std::endl;
  std::cout << "CPU TotalTime=       " << (now_time - CPUSeconds_start) << std::endl;
  std::cout << "WallClock TotalTime= " << (WallClockSeconds() - WallClockSeconds_start) << std::endl;

  std::cout << std::endl;
  std::cout << "Summary statistics (all normalized)" << std::endl;
  std::cout << "Minimum cost= " << minCost/double(instance->getN()) << std::endl;
  std::cout << "Average cost= " << sumCosts/double(numSamples)/double(instance->getN()) << std::endl;
  std::cout << "Maximum cost= " << maxCost/double(instance->getN()) << std::endl;
  std::cout << std::endl;

  delete bestSolution;

  return 1;
}
