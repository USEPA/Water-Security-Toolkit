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
#include <fstream>
#include <iomanip>

#include <algorithm>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>

#include "constructive.h"
#include "instance.h"
#include "impact_instance.h"
#include "instance_factory.h"
#include "search.h"
#include "bossa_random.h"

#include <sp/SPProblem.h>

#include <utilib/seconds.h>
#include <utilib/BitArray.h>
#include <utilib/BasicArray.h>

#include "redefs.h"

typedef enum {OBJMEAN,OBJVAR,OBJTCE,OBJWORST} ObjectiveStatistic;

double expectedCaseMoveEvaluator(const PMSolution &startSolution,
				 int facilityToOpen,
				 double gamma)
{
  // to avoid the virtual overhead and enable function inlining...
  PMImpactInstance *instance=dynamic_cast<PMImpactInstance*>(startSolution.getInstance());

  int numCustomers(instance->getN());

  double sumDistances(0.0);

  // assess the impact of opening the new facility.
  for(int i=1;i<=numCustomers;++i)
    {
      double distToNewFacility(instance->getDist(i,facilityToOpen));
      if(distToNewFacility<startSolution.getDistClosest(i))
	{
	  sumDistances+=distToNewFacility;
	}
      else
	{
	  sumDistances+=startSolution.getDistClosest(i);
	}
    }
  
  return sumDistances/double(numCustomers);
}

double varMoveEvaluator(const PMSolution &startSolution,
			int facilityToOpen,
			double gamma)
{
  // PMInstance *instance=startSolution.getInstance();
  // to avoid the virtual overhead and enable function inlining...
  PMImpactInstance *instance=dynamic_cast<PMImpactInstance*>(startSolution.getInstance());

  int numCustomers(instance->getN());

  std::vector<double> impacts(numCustomers);

  // assess the impact of opening the new facility.
  for(int i=1;i<=numCustomers;++i)
    {
      double distToNewFacility(instance->getDist(i,facilityToOpen));
      if(distToNewFacility<startSolution.getDistClosest(i))
  	{
  	  impacts[i-1]=distToNewFacility;
  	}
      else
  	{
  	  impacts[i-1]=startSolution.getDistClosest(i);
  	}
    }

  std::sort(impacts.begin(),impacts.end());

  // compute the gamma index under the fixed-probability assumption
  int index((int)(ceil(numCustomers*(1.0-gamma))));

  return impacts[index-1];  
}

double tceMoveEvaluator(const PMSolution &startSolution,
			int facilityToOpen,
			double gamma)
{
  // PMInstance *instance=startSolution.getInstance();
  // to avoid the virtual overhead and enable function inlining...
  PMImpactInstance *instance=dynamic_cast<PMImpactInstance*>(startSolution.getInstance());

  int numCustomers(instance->getN());

  std::vector<double> impacts(numCustomers);

  // assess the impact of opening the new facility.
  for(int i=1;i<=numCustomers;++i)
    {
      double distToNewFacility(instance->getDist(i,facilityToOpen));
      if(distToNewFacility<startSolution.getDistClosest(i))
  	{
  	  impacts[i-1]=distToNewFacility;
  	}
      else
  	{
  	  impacts[i-1]=startSolution.getDistClosest(i);
  	}
    }

  std::sort(impacts.begin(),impacts.end());

  // compute the gamma index under the fixed-probability assumption
  int index((int)(ceil(numCustomers*(1.0-gamma))));
  index--;

  double perAttackProbability=1.0/numCustomers;

  // sum the tail weight and normalize
  double result(0.0);
  double tailProb(0.0);
  for(int i=index;i<numCustomers;++i)
    {
      result+=(impacts[i]*perAttackProbability);
      tailProb+=perAttackProbability;
    }

  return result/tailProb;
}

double worstCaseMoveEvaluator(const PMSolution &startSolution,
			      int facilityToOpen,
			      double gamma)
{
  // PMInstance *instance=startSolution.getInstance();
  // to avoid the virtual overhead and enable function inlining...
  PMImpactInstance *instance=dynamic_cast<PMImpactInstance*>(startSolution.getInstance());

  int numCustomers(instance->getN());

  double worstCaseDistance(0.0);

  // assess the impact of opening the new facility.
  for(int i=1;i<=numCustomers;++i)
    {
      double distToNewFacility(instance->getDist(i,facilityToOpen));
      if(distToNewFacility<startSolution.getDistClosest(i))
	{
	  worstCaseDistance=std::max(worstCaseDistance,distToNewFacility);
	}
      else
	{
	  worstCaseDistance=std::max(worstCaseDistance,startSolution.getDistClosest(i));
	}
    }

  return worstCaseDistance;
}

PMSolution *descend(PMSolution &startingPoint,
		    double (*moveEvaluator)(const PMSolution&,int,double),
		    double (PMSolution::*costFunction)(double)const,
		    const std::vector<int> &facilityLocationConstraints,
		    double gamma,
		    double terminationThreshold=0.0,
		    bool enforceSideConstraints=false,
		    const std::vector<std::string> *sideConstraintObjectives=0,
		    const std::vector<ObjectiveStatistic> *sideConstraintStatistics=0,
		    const std::vector<PMInstance*> *sideConstraintInstances=0,
		    const std::vector<double> *sideConstraintBounds=0,
		    const std::vector<double (PMSolution::*)(double)const> *sideConstraintCostFunctions=0,
		    const std::vector<double (*)(const PMSolution&,int,double)> *sideConstraintMoveEvaluators=0)

{
  // this function assumes that all fixed/invalid constraints 
  // are satisfied in the input solution. 

  PMSolution *result=new PMSolution(startingPoint.getInstance());
  result->copyFrom(startingPoint);

  if((startingPoint.*costFunction)(gamma)<=terminationThreshold)
    {
      return result;
    }

  int n(result->getN());
  int m(result->getM());
  int p(result->getP());
  
  bool improvementFound(true); // to trip the loop
  while(improvementFound==true)
    {
      double baselineCost((result->*costFunction)(gamma));
      improvementFound=false; // until proven otherwise

      for(int facilityToClosePosition=1;
	  (facilityToClosePosition<=p)&&(improvementFound==false);
	  facilityToClosePosition++)
	{
	  int facilityToClose(result->getFacility(facilityToClosePosition));

	  if(facilityLocationConstraints[facilityToClose]!=1)
	    {
	      // the "source" facility isn't constrained to be open

	      // remove the facility from both the primary objective and side
	      // constraint solutions, updating the distances accordingly.
	      PMSolution baselineSolution(result->getInstance());
	      baselineSolution.copyFrom(*result);
	      baselineSolution.remove(facilityToClose,true);

	      std::vector<PMSolution*> sideConstraintSolutions;
	      if(enforceSideConstraints==true)
		{
		  for(size_t i=0;i<(*sideConstraintInstances).size();i++)
		    {
		      sideConstraintSolutions.push_back(new PMSolution((*sideConstraintInstances)[i]));
		      sideConstraintSolutions[i]->copyFromNoInstance(*result);
		      sideConstraintSolutions[i]->remove(facilityToClose,true);
		    }
		}
	  
	      // recall that the first p facilities in a solution are all open.
	      // the remaining p+1..m are closed. 
	      for(int facilityToOpenPosition=p+1; // you've closed all but one facility
		  (facilityToOpenPosition<=m)&&(improvementFound==false);
		  facilityToOpenPosition++)
		{
		  int facilityToOpen(result->getFacility(facilityToOpenPosition));

		  if(facilityLocationConstraints[facilityToOpen]!=2)
		    {
		      // the "destination" facility isn't constrained to be closed

		      double costOfMove=(moveEvaluator)(baselineSolution,facilityToOpen,gamma);

		      double delta=costOfMove-baselineCost;

		      if(delta<(-EPSILON))
			{
			  // check satisfaction of all side constraints;
			  bool sideConstraintsSatisfied(true);
			  if(enforceSideConstraints==true)
			    {
			      for(size_t i=0;(i<sideConstraintSolutions.size())&&(sideConstraintsSatisfied==true);i++)
				{
				  double sideConstraintCost=((*sideConstraintMoveEvaluators)[i])(*(sideConstraintSolutions[i]),facilityToOpen,gamma);
				  if(sideConstraintCost>(*sideConstraintBounds)[i])
				    {
				      sideConstraintsSatisfied=false;
				    }
				}
			    }
			  
			  if(sideConstraintsSatisfied==true)
			    {
			      baselineCost=costOfMove;
			      improvementFound=true;
			      result->swap(facilityToClose,facilityToOpen,true);
			      if(baselineCost<=terminationThreshold)
				{
				  return result;
				}
			    }
			}
		    }
		}
	    }
	}
    }

  return result;
}

int main(int argc, char **argv)
{
  if((argc!=6)&&
     (argc!=7))
    {
      std::cout << "***Usage: sideconstraints <configuration-file> <num-samples> <random-seed> <impact-file-representation> <time-limit> [solution-file]" << std::endl;
      return 0;
    }

  std::cout << "Executable: sideconstraints" << std::endl;

  InitializeTiming();
  double CPUSeconds_start = CPUSeconds();
  double WallClockSeconds_start = WallClockSeconds();

  //////////////////////////////////////////////////////////////
  // all problem-specific (as opposed to algorithm-specific)  //
  // parameters are read from the SPProblem class, which is   //
  // an interface to the configuration text file generated by //
  // the 'sp' solver script.                                  //
  //////////////////////////////////////////////////////////////

  SPProblem theProblem;
  std::ifstream configFile(argv[1]);
  if(!configFile)
    {
      std::cout << "***Failed to open configuration file=" << argv[1] << std::endl;
      return 0;
    }
  theProblem.read_config_file(configFile, false);
  configFile.close();

  std::cout << "Configuration file=" << argv[1] << std::endl;

  ////////////////////////////////////////////////////////////////////
  // determine whether a sparse matrix (0) or dense matrix (1)      //
  // representation will be used to store the impact file coontents //
  ////////////////////////////////////////////////////////////////////

  int impactFileRepresentation(atoi(argv[4]));
  if((impactFileRepresentation!=0)&&
     (impactFileRepresentation!=1))
    {
      std::cout << "***Unknown impact file representation=" << impactFileRepresentation << std::endl;
      return 0;
    }
  std::cout << "Impact file representation=" << impactFileRepresentation << std::endl;

  ////////////////////////////////////////////////////////
  // determine the objective, statistic, and number of  //
  // facilities to open (i.e., sensors to place) from   //
  // the overall problem definition. also load the side //
  // constraints, including bounds and impact files.    //
  ////////////////////////////////////////////////////////

  double gamma(theProblem.gamma);

  std::string objective;
  std::string statisticName;
  ObjectiveStatistic statistic;
  std::string objectiveImpactFileName;
  int numFacilitiesToOpen(-1);  

  std::vector<std::string> sideConstraintObjectives; 
  std::vector<std::string> sideConstraintImpactFiles;
  std::vector<ObjectiveStatistic> sideConstraintStatistics;
  std::vector<std::string> sideConstraintStatisticNames;
  std::vector<double> sideConstraintUpperBounds;
  std::vector<double (PMSolution::*)(double)const> sideConstraintCostFunctions; 
  std::vector<double (*)(const PMSolution&,int,double)> sideConstraintMoveEvaluators;

  for(size_t i=0;i<theProblem.goals.size();++i)
    {
      SPGoalInfo &thisGoal=theProblem.goals[i];
      // only take the first goal that is flagged as an objective -
      // the rest will be treated as side-constraints (the error
      // should have been picked up in the sp driver script)
      if((thisGoal.objective_flag==true)&&(objective==""))
	{
	  if(thisGoal.bound.size()>2)
	    {
	      std::cout << "***More than one side constraint bound was specified for the objective=" << objective << std::endl;
	      return 1;
	    }

	  objective=thisGoal.goal;
	  objectiveImpactFileName=thisGoal.impact_file;

	  std::list<std::string>::const_iterator measureCurrIter=thisGoal.measure.begin();
	  std::list<std::string>::const_iterator measureEndIter=thisGoal.measure.end();
	  std::list<double>::const_iterator boundCurrIter=thisGoal.bound.begin();
	  std::list<double>::const_iterator boundEndIter=thisGoal.bound.end();
	  
	  for(;measureCurrIter!=measureEndIter;++measureCurrIter,++boundCurrIter)
	    {
	      std::string thisMeasure=(*measureCurrIter);
	      double thisBound=(*boundCurrIter);
	      ObjectiveStatistic thisStatistic;
	      double (PMSolution::*thisCostFunction)(double)const;
	      double (*thisMoveEvaluator)(const PMSolution&,int,double);

	      if(thisMeasure=="mean")
		{
		  thisStatistic=OBJMEAN;
		  thisCostFunction=&PMSolution::normalizedCost;
		  thisMoveEvaluator=&expectedCaseMoveEvaluator;
		}
	      else if(thisMeasure=="var")
		{
		  thisStatistic=OBJVAR;
		  thisCostFunction=&PMSolution::varCost;
		  thisMoveEvaluator=&varMoveEvaluator;
		}
	      else if(thisMeasure=="tce")
		{
		  thisStatistic=OBJTCE;
		  thisCostFunction=&PMSolution::tceCost;
		  thisMoveEvaluator=&tceMoveEvaluator;
		}
	      else if(thisMeasure=="worst")
		{
		  thisStatistic=OBJWORST;
		  thisCostFunction=&PMSolution::worstCaseCost;
		  thisMoveEvaluator=&worstCaseMoveEvaluator;
		}
	      else
		{
		  std::cout << "***Unknown statistic specified: " << thisMeasure << std::endl;
		  return 0;
		}

	      if(thisBound==SPGoalInfo::OBJECTIVE_GOAL_BOUND)
		{
		  statisticName=thisMeasure;
		  statistic=thisStatistic;
		}
	      else
		{
		  sideConstraintObjectives.push_back(thisGoal.goal);
		  sideConstraintImpactFiles.push_back(thisGoal.impact_file);
		  sideConstraintUpperBounds.push_back(thisBound);
		  sideConstraintStatisticNames.push_back(thisMeasure);
		  sideConstraintStatistics.push_back(thisStatistic);
		  sideConstraintCostFunctions.push_back(thisCostFunction);
		  sideConstraintMoveEvaluators.push_back(thisMoveEvaluator);
		}
	    }
	}
      else 
	{
	  if((thisGoal.goal=="ns")&&(numFacilitiesToOpen==-1))
	    {
	      // the number of sensors is a hard-wired primary constraint, 
	      // as opposed to a secondary side constraint
	      numFacilitiesToOpen=int(rint(thisGoal.bound.front()));
	    }
	  else if((thisGoal.goal=="ns")&&(numFacilitiesToOpen!=-1))
	    {
	      std::cout << "***The number of sensors is muliply specified" << std::endl;
	      return 0;
	    }
	  else 
	    {
	      if(thisGoal.bound.size()>1)
		{
		  std::cout << "***Multiple bounds were specified for constraint=" << thisGoal.goal << std::endl;
		  return 1;
		}
	      if(thisGoal.measure.size()>1)
		{
		  std::cout << "***Multiple statistics were specified for constraint=" << thisGoal.goal << std::endl;
		  return 1;
		}

	      sideConstraintObjectives.push_back(thisGoal.goal);
	      sideConstraintImpactFiles.push_back(thisGoal.impact_file);
	      sideConstraintUpperBounds.push_back(thisGoal.bound.front());

	      std::string thisStatistic(thisGoal.measure.front());
	      sideConstraintStatisticNames.push_back(thisStatistic);
	      if(thisStatistic=="mean")
		{
		  sideConstraintStatistics.push_back(OBJMEAN);
		  sideConstraintCostFunctions.push_back(&PMSolution::normalizedCost);
		  sideConstraintMoveEvaluators.push_back(&expectedCaseMoveEvaluator);
		}
	      else if(thisStatistic=="var")
		{
		  sideConstraintStatistics.push_back(OBJVAR);
		  sideConstraintCostFunctions.push_back(&PMSolution::varCost);
		  sideConstraintMoveEvaluators.push_back(&varMoveEvaluator);
		}
	      else if(thisStatistic=="tce")
		{
		  sideConstraintStatistics.push_back(OBJTCE);
		  sideConstraintCostFunctions.push_back(&PMSolution::tceCost);
		  sideConstraintMoveEvaluators.push_back(&tceMoveEvaluator);
		}
	      else if(thisStatistic=="worst")
		{
		  sideConstraintStatistics.push_back(OBJWORST);
		  sideConstraintCostFunctions.push_back(&PMSolution::worstCaseCost);
		  sideConstraintMoveEvaluators.push_back(&worstCaseMoveEvaluator);
		}
	      else
		{
		  std::cout << "***The heuristic does not support the objective statistic=" << thisStatistic << std::endl;
		  return 0;
		}
	    }
	}
    }

  /////////////////////////////////////////////////////////
  // validate the problem-specific parameters. shouldn't //
  // be necessary, but you never know...                 //
  /////////////////////////////////////////////////////////
  
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

  if(statisticName=="")
    {
      std::cout << "No statistic was specified" << std::endl;
      return 0;
    }

  std::cout << "Optimization objective=" << objective << " (impact file=" << objectiveImpactFileName << ")" << std::endl;
  std::cout << "Optimization statistic=" << statisticName << std::endl;
  std::cout << std::endl;

  int numSideConstraints(sideConstraintObjectives.size());

  if(numSideConstraints>1)
    {
      std::cout << "***The heuristic currently only supports specification of a single side constraint - " << numSideConstraints << " were specified" << std::endl;
      return 0;
    }

  for(size_t i=0;i<sideConstraintObjectives.size();i++)
    {
      std::cout << "Side constraint=" << sideConstraintObjectives[i] << ", statistic=";
      switch(sideConstraintStatistics[i])
	{
	case OBJMEAN:
	  std::cout << "mean";
	  break;
	case OBJVAR:
	  std::cout << "var";
	  break;
	case OBJTCE:
	  std::cout << "tce";
	  break;
	case OBJWORST:
	  std::cout << "worst";
	  break;
	}
      std::cout << ", upper bound=" << sideConstraintUpperBounds[i] << " (impact file=" << sideConstraintImpactFiles[i] << ")" << std::endl;
    }
  std::cout << std::endl;


  /////////////////////////////////////////////////
  // load the optimization objective impact file //
  /////////////////////////////////////////////////

  PMImpactInstance *optimizationInstance = new PMImpactInstance;

  FILE *inputOptimizationImpactFile=fopen(objectiveImpactFileName.c_str(),"r");
  if(!inputOptimizationImpactFile)
    {
      std::cout << "***Failed to read problem instance=" << objectiveImpactFileName << std::endl;
    }

  if(impactFileRepresentation==0)
    {
      // employ sparse storage scheme
      optimizationInstance->enableSparseRepresentation();
    }
  else
    {
      // employ dense storage scheme
      optimizationInstance->enableDenseRepresentation();
    }
  optimizationInstance->readImpact(inputOptimizationImpactFile,numFacilitiesToOpen,theProblem.feasible_location,theProblem.reduced_repn_map, false);
  fclose(inputOptimizationImpactFile);  

  // post-impact validations...
  if(numFacilitiesToOpen>optimizationInstance->getM())
    {
    numFacilitiesToOpen = optimizationInstance->getM();
      //std::cout << "***The value of p must be <= the number of potential facilities" << std::endl;
      //return 0;
    }

  ///////////////////////////////////////////
  // load the side constraint impact files //
  ///////////////////////////////////////////

  std::vector<PMInstance*> sideConstraintInstances;
  for(size_t i=0;i<sideConstraintImpactFiles.size();i++)
    {
      PMImpactInstance *newInstance = new PMImpactInstance;

      FILE *inputSideConstraintImpactFile=fopen(sideConstraintImpactFiles[i].c_str(),"r");
      if(!inputSideConstraintImpactFile)
	{
	  std::cout << "***Failed to read problem instance=" << sideConstraintImpactFiles[i] << std::endl;
	}

      if(impactFileRepresentation==0)
	{
	  // employ sparse storage scheme
	  newInstance->enableSparseRepresentation();
	}
      else
	{
	  // employ dense storage scheme
	  newInstance->enableDenseRepresentation();
	}
      //newInstance->readImpact(inputSideConstraintImpactFile,numFacilitiesToOpen);
        newInstance->readImpact(inputSideConstraintImpactFile,numFacilitiesToOpen,theProblem.feasible_location,theProblem.reduced_repn_map, false);
      fclose(inputSideConstraintImpactFile);  

      sideConstraintInstances.push_back(newInstance);
    }

  //////////////////////////////////////////////////////////////
  // verify that all of the side constraint impact files have //
  // the same dimensions as the primary objective impact file //
  //////////////////////////////////////////////////////////////
  
  for(size_t i=0;i<sideConstraintInstances.size();i++)
    {
      if(sideConstraintInstances[i]->getM()!=optimizationInstance->getM())
	{
	  std::cout << "***Mistmatch in number of facilities between optimization impact file and side-constraint impact file=" << sideConstraintImpactFiles[i] << std::endl;
	  return 0;
	}
      if(sideConstraintInstances[i]->getN()!=optimizationInstance->getN())
	{
	  std::cout << "***Mistmatch in number of customers between optimization impact file and side-constraint impact file=" << sideConstraintImpactFiles[i] << std::endl;
	  return 0;
	}
    }

  // only deal with one side constraint for now (it's hard enough to solve efficiently)...
  if(sideConstraintInstances.size()>1)
    {
      std::cout << "***The heuristic presently supports specification of a single side constraints" << std::endl;
      return 0;
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
      if((fixeds[i]<=0)||(fixeds[i]>optimizationInstance->getM()))
	{
	  std::cout << "***Illegal fixed node index specified, value=" << fixeds[i] << std::endl;
	  return 0;
	}
    }

  std::cout << "Fixed placements (" << fixeds.size() << " total)=";
  for(size_t i=0;i<fixeds.size();++i)
    {
      std::cout << fixeds[i] << " ";
    }
  std::cout << std::endl;

  const utilib::BasicArray<int> invalids=theProblem.invalid_placements;

  //////////////////////////////////////////////////////////////////
  // validate the invalid placements - ideally, SPProblem should  //
  // do this, but the combination of it and the sp script aren't  //
  // ensuring this as of yet.                                     //
  //////////////////////////////////////////////////////////////////

  for(size_t i=0;i<invalids.size();++i)
    {
      if((invalids[i]<=0)||(invalids[i]>optimizationInstance->getM()))
	{
	  std::cout << "***Illegal invalid node index specified, value=" << invalids[i] << std::endl;
	  return 0;
	}
    }

  std::cout << "Invalid placements (" << invalids.size() << " total)=";
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

  /////////////////////////////////////////////////////////
  // load and validate any algorithm-specific parameters //
  /////////////////////////////////////////////////////////

  int numSamples(atoi(argv[2]));
  if(numSamples<=0)
    {
      std::cout << "***The number of samples must be >= 1" << std::endl;
      return 0;
    }
  std::cout << "Number of requested samples=" << numSamples << std::endl;

  ////////////////////////////////////////////
  // initialize the random number generator //
  ////////////////////////////////////////////

  int randomSeed(atoi(argv[3]));
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

  double timeLimit(atof(argv[5]));
  if(timeLimit<0.0)
    {
      std::cout << "***The time limit " << argv[5] << " must be >= 0.0" << std::endl;
      return 0;
    }
  std::cout << std::endl << "WallClock Time Limit=" << timeLimit << std::endl << std::endl;

  ////////////////////////////////////
  // set up the move/cost functions //
  ////////////////////////////////////
  
  double (PMSolution::*costFunction)(double)const;
  double (*moveEvaluator)(const PMSolution&,int,double);

  if(statistic==OBJMEAN)
    {
      costFunction=&PMSolution::normalizedCost;
      moveEvaluator=&expectedCaseMoveEvaluator;
    }
  else if(statistic==OBJVAR)
    {
      costFunction=&PMSolution::varCost;      
      moveEvaluator=&varMoveEvaluator;
    }
  else if(statistic==OBJTCE)
    {
      costFunction=&PMSolution::tceCost;      
      moveEvaluator=&tceMoveEvaluator;
    }
  else if(statistic==OBJWORST)
    {
      costFunction=&PMSolution::worstCaseCost;
      moveEvaluator=&worstCaseMoveEvaluator;
    }
  else
    {
      std::cout << "***Unknown performance statistic=" << statistic << std::endl;
      return 0;
    }

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

  //////////////////////////////////////////////
  // start the main routine, which is nothing //
  // but iterated hill-climbing.              //
  //////////////////////////////////////////////

  std::cout << "Starting iterated descent" << std::endl;

  double runtime_start = CPUSeconds();
  double minCost(DBL_MAX);
  double sumCosts(0.0);
  double maxCost(0.0);

  PMSolution *bestSolution=new PMSolution(optimizationInstance);

  std::cout.setf(std::ios_base::fixed,std::ios_base::floatfield);
  std::cout.precision(3);

  const int STATUS_INTERVAL=1000;
  for(int i=1;i<=numSamples;++i)
    {
      if((i%STATUS_INTERVAL)==0)
	{
	  std::cout << "Generated " << i << " samples so far" << std::endl;
	}

      ////////////////////////////////////
      // construct the initial solution //
      ////////////////////////////////////

      PMSolution *initialPoint=new PMSolution(optimizationInstance);
      initialPoint->reset();

      if(numSideConstraints==1)
	{
	  // with a single side constraint, the strategy for creating an initial point
	  // that is feasible w.r.t. the side-constraint is simple: optimize the 
	  // side-constraint objective and see if you can obtain the upper bound.

	  bool feasibleInitialPointFound(false);
	  double bestEvaluation(DBL_MAX);
	  const int MAX_ATTEMPTS_AT_FEASIBILITY=10;
	  for(int j=1;
	      (j<=MAX_ATTEMPTS_AT_FEASIBILITY)&&(feasibleInitialPointFound==false);
	      j++)
	    {
	      PMSolution *sideConstraintInitialPoint=new PMSolution(sideConstraintInstances[0]);
	      sideConstraintInitialPoint->reset();

	      if(sideConstraintStatistics[0]==OBJMEAN)
		{
		  // default in the grasp executable
		  constructiveAlgorithm.setMethod("sample:2"); 
		}
	      else
		{
		  // p-center heuristic plateau-wandering heuristic
		  constructiveAlgorithm.setMethod("pcenter");
		}
	      constructiveAlgorithm.run(sideConstraintInitialPoint,numFacilitiesToOpen,facilityLocationConstraints);
	      double sideConstraintInitialPointCost=(sideConstraintInitialPoint->*(sideConstraintCostFunctions[0]))(gamma);

	      PMSolution *sideConstraintLocalOptimum=new PMSolution(sideConstraintInitialPoint->getInstance());
	      sideConstraintLocalOptimum->copyFrom(*sideConstraintInitialPoint);
	      double sideConstraintLocalOptimumCost(DBL_MAX);

	      if(sideConstraintStatistics[0]==OBJMEAN)
		{
		  // "f" is for interchange (slower)
		  // "s" is for sparse matrix (fastest)
		  // NOTE: with running through the GRASP code base, the numbers aren't normalized by the scenario count...
		  PMSearch::runLocalSearch(sideConstraintLocalOptimum,"s",facilityLocationConstraints,false,0,sideConstraintUpperBounds[0]*sideConstraintInstances[0]->getN());
		}
	      else
		{
		  sideConstraintLocalOptimum=descend(*sideConstraintInitialPoint,sideConstraintMoveEvaluators[0],sideConstraintCostFunctions[0],facilityLocationConstraints,gamma,sideConstraintUpperBounds[0],false);
		}
	      sideConstraintLocalOptimumCost=(sideConstraintLocalOptimum->*(sideConstraintCostFunctions[0]))(gamma);

	      bestEvaluation=std::min(bestEvaluation,sideConstraintLocalOptimumCost);

	      if(sideConstraintLocalOptimumCost<=sideConstraintUpperBounds[0])
		{
		  initialPoint->copyFromNoInstance(*sideConstraintLocalOptimum);

		  feasibleInitialPointFound=true;

		  delete sideConstraintInitialPoint;
		  delete sideConstraintLocalOptimum;
		}
	    }

	  if(feasibleInitialPointFound==false)
	    {
	      std::cout << "***An upper bound of " << sideConstraintUpperBounds[0] << " for side constraint objective=" << sideConstraintObjectives[0];
	      std::cout << " could not be achieved - minimal value obtained via " << MAX_ATTEMPTS_AT_FEASIBILITY << " iterations of sampling was ";
	      std::cout << bestEvaluation << std::endl;
	      return 0;
	    }
	}
      else
	{
	  // no side constraints were specified - proceed as usual.
	  if(statistic==OBJMEAN)
	    {
	      // default in the grasp executable
	      constructiveAlgorithm.setMethod("sample:2"); 
	    }
	  else
	    {
	      // p-center heuristic plateau-wandering heuristic
	      constructiveAlgorithm.setMethod("pcenter");
	    }

	  constructiveAlgorithm.run(initialPoint,numFacilitiesToOpen,facilityLocationConstraints);
	}

      double initialCost=(initialPoint->*costFunction)(gamma);

      PMSolution *localOptimum=0;

      localOptimum=descend(*initialPoint,
			   moveEvaluator,
			   costFunction,
			   facilityLocationConstraints,
			   gamma,
			   0.0,
			   true,
			   &sideConstraintObjectives,
			   &sideConstraintStatistics,
			   &sideConstraintInstances,
			   &sideConstraintUpperBounds,
			   &sideConstraintCostFunctions,
			   &sideConstraintMoveEvaluators);

      double localOptimumCost=(localOptimum->*costFunction)(gamma);

      int numFacs=localOptimum->getP();
      std::vector<int> locations;
      locations.reserve(numFacs);
      for(int j=1;j<=numFacs;j++)
	{
	  locations.push_back(localOptimum->getFacility(j));
	}
      std::sort(locations.begin(),locations.end());

      std::cout << std::setw(4) << i << " [" << initialCost << "] -> [" << localOptimumCost << "]  Solution=[ ";
      for(size_t j=0;j<locations.size();j++)
	{
	  std::cout << locations[j] << " ";	  
	}
      std::cout << "] ";

      if(statistic==OBJWORST)
	{
	  std::cout << " #-wc-scenarios=" << localOptimum->numWorstCaseScenarios() << " ";
	}

      for(size_t j=0;j<sideConstraintObjectives.size();j++)
	{
	  PMSolution *sideConstraintSolution=new PMSolution(sideConstraintInstances[j]);
	  sideConstraintSolution->copyFromNoInstance(*localOptimum);
	  double sideConstraintEvaluation=(sideConstraintSolution->*(sideConstraintCostFunctions[j]))(gamma);
	  std::cout << sideConstraintObjectives[j] << "_" << sideConstraintStatisticNames[j] << "=" << sideConstraintEvaluation << " (ub=";
	  std::cout << sideConstraintUpperBounds[j] << ") ";
	}

      if(localOptimumCost<minCost)
	{
	  minCost=localOptimumCost;
	  bestSolution->copyFrom(*localOptimum);
	  std::cout << "   <<<"; // denote the new best-so-far

          // output facility locations to file, if requested
          if(argc==7)
            {
              int numFacs=bestSolution->getP();
              std::vector<int> locations;
              locations.reserve(numFacs);
              for(int i=1;i<=numFacs;++i)
	        {
	          locations.push_back(bestSolution->getFacility(i));
	        }
              std::sort(locations.begin(),locations.end());
        
              std::ofstream facilityOutputFile(argv[6]);
              if(!facilityOutputFile)
	        {
	          std::cerr << "***Failed to open facility output file=" << argv[6] << std::endl;
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
        
              std::cout << "Best solution written to file=" << argv[6] << std::endl;
            }
        
	}

      sumCosts+=localOptimumCost;

      if(localOptimumCost>maxCost)
	{
	  maxCost=localOptimumCost;
	}

      std::cout << std::endl;

      delete initialPoint;
      delete localOptimum;

      if ((timeLimit > 0.0) && ((WallClockSeconds() - WallClockSeconds_start) > timeLimit)) break;
    }

  std::cout << std::endl;
  double now_time = CPUSeconds();
  std::cout << "CPU RunTime=         " << (now_time - runtime_start) << std::endl;
  std::cout << "CPU TotalTime=       " << (now_time - CPUSeconds_start) << std::endl;
  std::cout << "WallClock TotalTime= " << (WallClockSeconds() - WallClockSeconds_start) << std::endl;

  std::cout << std::endl;
  std::cout << "Summary statistics" << std::endl;
  std::cout << "Minimum cost= " << minCost << std::endl;
  std::cout << "Average cost= " << sumCosts/double(numSamples) << std::endl;
  std::cout << "Maximum cost= " << maxCost << std::endl;
  std::cout << std::endl;

  delete bestSolution;

  return 1;
}
