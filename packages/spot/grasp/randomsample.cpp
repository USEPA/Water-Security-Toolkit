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

double worstCaseMoveEvaluator(const PMSolution &startSolution,
                              int facilityToOpen,
                              double gamma)
{
    // PMInstance *instance=startSolution.getInstance();
    // to avoid the virtual overhead and enable function inlining...
    PMImpactInstance *instance = dynamic_cast<PMImpactInstance*>(startSolution.getInstance());

    int numCustomers(instance->getN());

    double worstCaseDistance(0.0);

    // assess the impact of opening the new facility.
    for (int i = 1; i <= numCustomers; ++i)
    {
        double distToNewFacility(instance->getDist(i, facilityToOpen));
        if (distToNewFacility < startSolution.getDistClosest(i))
        {
            worstCaseDistance = std::max(worstCaseDistance, distToNewFacility);
        }
        else
        {
            worstCaseDistance = std::max(worstCaseDistance, startSolution.getDistClosest(i));
        }
    }

    return worstCaseDistance;
}

double varMoveEvaluator(const PMSolution &startSolution,
                        int facilityToOpen,
                        double gamma)
{
    // PMInstance *instance=startSolution.getInstance();
    // to avoid the virtual overhead and enable function inlining...
    PMImpactInstance *instance = dynamic_cast<PMImpactInstance*>(startSolution.getInstance());

    int numCustomers(instance->getN());

    std::vector<double> impacts(numCustomers);

    // assess the impact of opening the new facility.
    for (int i = 1; i <= numCustomers; ++i)
    {
        double distToNewFacility(instance->getDist(i, facilityToOpen));
        if (distToNewFacility < startSolution.getDistClosest(i))
        {
            impacts[i - 1] = distToNewFacility;
        }
        else
        {
            impacts[i - 1] = startSolution.getDistClosest(i);
        }
    }

    std::sort(impacts.begin(), impacts.end());

    // compute the gamma index under the fixed-probability assumption
    int index((int)(ceil(numCustomers * (1.0 - gamma))));

    return impacts[index - 1];
}

double tceMoveEvaluator(const PMSolution &startSolution,
                        int facilityToOpen,
                        double gamma)
{
    // PMInstance *instance=startSolution.getInstance();
    // to avoid the virtual overhead and enable function inlining...
    PMImpactInstance *instance = dynamic_cast<PMImpactInstance*>(startSolution.getInstance());

    int numCustomers(instance->getN());

    std::vector<double> impacts(numCustomers);

    // assess the impact of opening the new facility.
    for (int i = 1; i <= numCustomers; ++i)
    {
        double distToNewFacility(instance->getDist(i, facilityToOpen));
        if (distToNewFacility < startSolution.getDistClosest(i))
        {
            impacts[i - 1] = distToNewFacility;
        }
        else
        {
            impacts[i - 1] = startSolution.getDistClosest(i);
        }
    }

    std::sort(impacts.begin(), impacts.end());

    // compute the gamma index under the fixed-probability assumption
    int index((int)(ceil(numCustomers * (1.0 - gamma))));
    index--;

    double perAttackProbability = 1.0 / numCustomers;

    // sum the tail weight and normalize
    double result(0.0);
    double tailProb(0.0);
    for (int i = index; i < numCustomers; ++i)
    {
        result += (impacts[i] * perAttackProbability);
        tailProb += perAttackProbability;
    }

    return result / tailProb;
}

PMSolution *descend(PMSolution &startingPoint,
                    double(*moveEvaluator)(const PMSolution&, int, double),
                    double(PMSolution::*costFunction)(double)const,
                    const std::vector<int> &facilityLocationConstraints,
                    double gamma)
{
    // this function assumes that all fixed/invalid constraints
    // are satisfied in the input solution.

    PMSolution *result = new PMSolution(startingPoint.getInstance());
    result->copyFrom(startingPoint);

    int p(result->getP());
    int m(result->getM());

    bool improvementFound(true); // to trip the loop
    while (improvementFound == true)
    {
        double baselineCost((result->*costFunction)(gamma));
        improvementFound = false; // until proven otherwise

        for (int facilityToClosePosition = 1;
                (facilityToClosePosition <= p) && (improvementFound == false);
                facilityToClosePosition++)
        {
            int facilityToClose(result->getFacility(facilityToClosePosition));

            if (facilityLocationConstraints[facilityToClose] != 1)
            {
                // the "source" facility isn't constrained to be open

                // remove the facility from the solution and update the distances accordingly
                PMSolution baselineSolution(result->getInstance());
                baselineSolution.copyFrom(*result);

                baselineSolution.remove(facilityToClose, true);

                // recall that the first p facilities in a solution are all open.
                // the remaining p+1..m are closed.
                for (int facilityToOpenPosition = p + 1; // you've closed all but one facility
                        (facilityToOpenPosition <= m) && (improvementFound == false);
                        facilityToOpenPosition++)
                {
                    int facilityToOpen(result->getFacility(facilityToOpenPosition));

                    if (facilityLocationConstraints[facilityToOpen] != 2)
                    {
                        // the "destination" facility isn't constrained to be closed

                        double costOfMove = (moveEvaluator)(baselineSolution, facilityToOpen, gamma);

                        //std::cout << " Looking for improvement: " << facilityToClose << " " << facilityToOpen << " : " << costOfMove << " " << baselineCost << " : " << (costOfMove < baselineCost) << " " << improvementFound << std::endl;
                        if (costOfMove < baselineCost)
                        {
                            baselineCost = costOfMove;
                            improvementFound = true;
                            result->swap(facilityToClose, facilityToOpen, true);
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
    if ((argc != 6) &&
            (argc != 7) &&
            (argc != 8)
       )
    {
        std::cout << "***Usage: randomsample <configuration-file> <num-samples> <random-seed> <impact-file-representation> <time-limit> [solution-file] [json-file]" << std::endl;
        return 1;
    }

    std::cout << "Executable: randomsample" << std::endl;

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
    if (!configFile)
    {
        std::cout << "***Failed to open configuration file=" << argv[1] << std::endl;
        return 1;
    }
    // TNT: Added the flag to tell the SPProblem class to not
    // load the impact data (it will be loaded later.
    theProblem.read_config_file(configFile, false);
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

    for (size_t i = 0; i < theProblem.goals.size(); ++i)
    {
        SPGoalInfo &thisGoal = theProblem.goals[i];
        // only take the first goal
        if ((thisGoal.objective_flag == true) && (objective == ""))
        {
            objective = thisGoal.goal;
            impactFileName = thisGoal.impact_file;
            statistic = thisGoal.measure.front();
        }
        else
        {
            if ((thisGoal.goal == "ns") && (numFacilitiesToOpen == -1))
            {
                numFacilitiesToOpen = int(rint(thisGoal.bound.front()));
            }
        }
    }

    /////////////////////////////////////////////////////////
    // validate the problem-specific parameters. shouldn't //
    // be necessary, but you never know...                 //
    /////////////////////////////////////////////////////////

    std::cout << "Max number of facilities to open=" << numFacilitiesToOpen << std::endl;

    if (numFacilitiesToOpen <= 0)
    {
        std::cout << "***The value of p must be >= 1" << std::endl;
        return 1;
    }
    std::cout << "P=" << numFacilitiesToOpen << std::endl;

    if (objective == "")
    {
        std::cout << "***No objective was specified" << std::endl;
        return 1;
    }

    if (statistic == "")
    {
        std::cout << "No statistic was specified" << std::endl;
        return 1;
    }

    std::cout << "Objective=" << objective << std::endl;

    ////////////////////////////////////////////////////////////////////
    // determine whether a sparse matrix (0) or dense matrix (1)      //
    // representation will be used to store the impact file coontents //
    ////////////////////////////////////////////////////////////////////
    // TNT: Added representation types 2 and 3
    int impactFileRepresentation(atoi(argv[4]));
    if ((impactFileRepresentation != 0) &&
            (impactFileRepresentation != 1) &&
            (impactFileRepresentation != 2) &&
            (impactFileRepresentation != 3))
    {
        std::cout << "***Unknown impact file representation=" << impactFileRepresentation << std::endl;
        return 1;
    }
    std::cout << "Impact file representation=" << impactFileRepresentation << std::endl;

    //////////////////////////
    // load the impact file //
    //////////////////////////

    std::cout << "Instance filename=" << impactFileName << std::endl;
    PMImpactInstance *instance = new PMImpactInstance;

    FILE *inputImpactFile = fopen(impactFileName.c_str(), "r");
    if (!inputImpactFile)
    {
        std::cout << "***Failed to read problem instance=" << impactFileName << std::endl;
    }

    if (impactFileRepresentation == 0)
    {
        // employ sparse storage scheme
        instance->enableSparseRepresentation();
    }
    else if (impactFileRepresentation == 1)
    {
        // employ dense storage scheme
        instance->enableDenseRepresentation();
    }
    // TNT
    else if (impactFileRepresentation == 2)
    {
        // employ dense storage scheme
        instance->enableFileBasedRepresentation();
    }
    // TNT
    else if (impactFileRepresentation == 3)
    {
        // employ dense storage scheme
        instance->enableCachedRepresentation();
    }
    instance->readImpact(inputImpactFile, numFacilitiesToOpen, theProblem.feasible_location, theProblem.reduced_repn_map, false);
//  instance->setDumpIndices();
    fclose(inputImpactFile);

    // post-impact validations...
    if (numFacilitiesToOpen > instance->getM())
    {
        numFacilitiesToOpen = instance->getM();
        //std::cout << "***The value of p must be <= the number of potential facilities" << std::endl;
    }

    ////////////////////////////////////////////////////////////////
    // load and validate all of the facility location constraints //
    ////////////////////////////////////////////////////////////////

    const utilib::BasicArray<int> fixeds = theProblem.fixed_placements;

    // validate the fixed placements - ideally, SPProblem should
    // do this, but the combination of it and the sp script
    // aren't ensuring this as of yet.
    for (size_t i = 0; i < fixeds.size(); ++i)
    {
        if ((fixeds[i] <= 0) || (fixeds[i] > instance->getM()))
        {
            std::cout << "***Illegal fixed node index specified, value=" << fixeds[i] << std::endl;
            return 1;
        }
    }

    std::cout << "Fixed placements(" << fixeds.size() << ")=";
    for (size_t i = 0; i < fixeds.size(); ++i)
    {
        std::cout << fixeds[i] << " ";
    }
    std::cout << std::endl;

    const utilib::BasicArray<int> invalids = theProblem.invalid_placements;

    // validate the invalid placements - ideally, SPProblem should
    // do this, but the combination of it and the sp script aren't
    // ensuring this as of yet.
    for (size_t i = 0; i < invalids.size(); ++i)
    {
        if ((invalids[i] <= 0) || (invalids[i] > instance->getM()))
        {
            std::cout << "***Illegal invalid node index specified, value=" << invalids[i] << std::endl;
            return 1;
        }
    }

    std::cout << "Invalid placements(" << invalids.size() << ")=";
    for (size_t i = 0; i < invalids.size(); ++i)
    {
        std::cout << invalids[i] << " ";
    }
    std::cout << std::endl;

    int effectiveP(numFacilitiesToOpen - fixeds.size());
    if (effectiveP < 0)
    {
        std::cout << "***The number of fixed sensor locations must be <= p" << std::endl;
        return 1;
    }
    std::cout << "Effective P=" << effectiveP << std::endl;

    /////////////////////////////////////////////////////////
    // load and validate any algorithm-specific parameters //
    /////////////////////////////////////////////////////////

    int numSamples(atoi(argv[2]));
    if (numSamples <= 0)
    {
        std::cout << "***The number of samples must be >= 1" << std::endl;
        return 1;
    }
    // We could probably use effectiveP == 1 for this test, but it may depend on
    // user options about always taking the best moves in the local search.  So
    // until we have time to make sure, we'll just avoid unnecessary iterations for
    // the case we know.
    if (numFacilitiesToOpen == 1)
        // We'll just compute the optimal answer, and only need to do that once.
    {
        numSamples = 1;
    }
    std::cout << "Number of requested samples=" << numSamples << std::endl;

    ////////////////////////////////////////////
    // initialize the random number generator //
    ////////////////////////////////////////////

    int randomSeed(atoi(argv[3]));
    if (randomSeed <= 0)
    {
        std::cout << "***The random number seed must be >= 1" << std::endl;
        return 1;
    }
    BossaRandom::randomize(randomSeed);
    std::cout << std::endl << "Random number seed=" << randomSeed << std::endl << std::endl;

    ////////////////////////////////////
    // set up the runtime limit //
    ////////////////////////////////////

    double timeLimit(atof(argv[5]));
    if (timeLimit < 0.0)
    {
        std::cout << "***The time limit " << argv[5] << " must be >= 0.0" << std::endl;
        return 1;
    }
    std::cout << std::endl << "WallClock Time Limit=" << timeLimit << std::endl << std::endl;


    ////////////////////////////////////
    // set up the move/cost functions //
    ////////////////////////////////////

    double(PMSolution::*costFunction)(double)const;
    double(*moveEvaluator)(const PMSolution&, int, double);

    if (statistic == "worst")
    {
        costFunction = &PMSolution::worstCaseCost;
        moveEvaluator = &worstCaseMoveEvaluator;
    }
    else if (statistic == "var")
    {
        costFunction = &PMSolution::varCost;
        moveEvaluator = &varMoveEvaluator;
    }
    else if (statistic == "tce")
    {
        costFunction = &PMSolution::tceCost;
        moveEvaluator = &tceMoveEvaluator;
    }
    else if (statistic == "mean")
    {
        costFunction = 0;
        moveEvaluator = 0;
    }
    else
    {
        std::cout << "***Unknown performance statistic=" << statistic << std::endl;
        return 1;
    }

    std::cout << "Performance statistic=" << statistic << std::endl;

    ////////////////////////////////////////////////////////
    // construct the facility location constraint vector. //
    // the array is 1-based, like the facility indices    //
    // 0=no constraints, free to place facility           //
    // 1=fixed facility location                          //
    // 2=invalid facility location                        //
    ////////////////////////////////////////////////////////

    std::vector<int> facilityLocationConstraints(theProblem.num_nodes + 1, 0);
    for (size_t i = 0; i < fixeds.size(); ++i)
    {
        facilityLocationConstraints[fixeds[i]] = 1;
    }
    for (size_t i = 0; i < invalids.size(); ++i)
    {
        if (facilityLocationConstraints[invalids[i]] != 0)
        {
            std::cout << "***WARNING - Over-riding fixed facility location index=" << invalids[i] << " with an invalid constraint" << std::endl;
        }
        facilityLocationConstraints[invalids[i]] = 2;
    }

    //////////////////////////////////////////////////
    // initialize the constructive algorithm, which //
    // currently isn't anything special.            //
    //////////////////////////////////////////////////

    PMConstructive constructiveAlgorithm;
    if (statistic == "mean")
    {
        // If there's only one facility, the optimal solution is to pick the single best location.
        // This is actually true if effectiveP == 1, but the constructive algorithm doesn't now
        // about effectiveP.  The regular algorithms should do the right thing if k > 1 even if effeciveP = 1.
        // We have to consider 1 facility as a special case because the constructive algorithm doesn't
        // work properly (I leaves the second-best value as infinite, which messes up the local descent computations.
        if (numFacilitiesToOpen == 1)
            // The second part of this is a parameter we won't use.  But we have to set it this way or
            // this call will fail.
        {
            constructiveAlgorithm.setMethod("pgreedy:1.0");
        }
        // default in the grasp executable
        else { constructiveAlgorithm.setMethod("sample:2"); }
    }
    else
    {
        // p-center heuristic plateau-wandering heuristic
        constructiveAlgorithm.setMethod("pcenter");
    }

    // Create the JSON output file before starting optimization
    std::ofstream jsonOutputFile;
    if (argc == 8)
        jsonOutputFile.open(argv[7]);
    jsonOutputFile << "{" << std::endl;
    jsonOutputFile << "\"solutions\": [" << std::endl;
    bool first_json_solution=true;

    //////////////////////////////////////////////
    // start the main routine, which is nothing //
    // but iterated hill-climbing.              //
    //////////////////////////////////////////////

    std::cout << "Starting iterated descent" << std::endl;

    double runtime_start = CPUSeconds();
    double minCost(DBL_MAX);
    double sumCosts(0.0);
    double maxCost(0.0);

    PMSolution *bestSolution = new PMSolution(instance);

    std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);
    std::cout.precision(2);

    const int STATUS_INTERVAL = 1000;
    for (int i = 1; i <= numSamples; ++i)
    {
        double WallClockIter_start = WallClockSeconds();
        if ((i % STATUS_INTERVAL) == 0)
        {
            std::cout << "Generated " << i << " samples so far" << std::endl;
        }

        PMSolution *initialPoint = new PMSolution(instance);
        initialPoint->reset();
        // TNT: set impact cahce (if used) to process in node-major order
        if (instance->getImpactCache() != NULL) { instance->getImpactCache()->setNodeMajor(); }
        constructiveAlgorithm.run(initialPoint, numFacilitiesToOpen, facilityLocationConstraints);

        double initialCost(0.0);
        if (statistic == "mean")
        {
            initialCost = double(initialPoint->cost()) / double(instance->getN());
        }
        else
        {
            initialCost = (initialPoint->*costFunction)(gamma);
        }

        PMSolution *localOptimum(0);

        if (statistic == "mean")
        {
            localOptimum = new PMSolution(initialPoint->getInstance());
            // TNT: set impact cahce (if used) to process in node-major order
            if (instance->getImpactCache() != NULL) { instance->getImpactCache()->setNodeMajor(); }
            localOptimum->copyFrom(*initialPoint);
            // "f" is for interchange (slower)
            // "s" is for sparse matrix (fastest)
            // TNT: set impact cahce (if used) to process in event-major order
            if (instance->getImpactCache() != NULL) { instance->getImpactCache()->setEventMajor(); }
            if (numFacilitiesToOpen > 1)
            {
                PMSearch::runLocalSearch(localOptimum, "s", facilityLocationConstraints);
            }
        }
        else
        {
            localOptimum = descend(*initialPoint, moveEvaluator, costFunction, facilityLocationConstraints, gamma);
        }

        double localOptimumCost(0.0);
        if (statistic == "mean")
        {
            localOptimumCost = double(localOptimum->cost()) / double(instance->getN());
        }
        else
        {
            localOptimumCost = (localOptimum->*costFunction)(gamma);
        }

        int numFacs = localOptimum->getP();
        std::vector<int> locations;
        locations.reserve(numFacs);
        for (int j = 1; j <= numFacs; j++)
        {
            locations.push_back(localOptimum->getFacility(j));
        }
        std::sort(locations.begin(), locations.end());

        std::cout << std::setw(4) << i << " [" << initialCost << "] -> [" << localOptimumCost << "]  Solution=[ ";
        for (size_t j = 0; j < locations.size(); j++)
        {
            std::cout << locations[j] << " ";
        }
        std::cout << "]";

        if (statistic == "worst")
        {
            std::cout << " #-wc-scenarios=" << localOptimum->numWorstCaseScenarios();
        }

        if (localOptimumCost < minCost)
        {
            minCost = localOptimumCost;
            bestSolution->copyFrom(*localOptimum);
            std::cout << "   <<<"; // denote the new best-so-far

            // output facility locations to file, if requested
            if (argc == 7)
            {
                int numFacs = bestSolution->getP();
                std::vector<int> locations;
                locations.reserve(numFacs);
                for (int i = 1; i <= numFacs; ++i)
                {
                    locations.push_back(bestSolution->getFacility(i));
                }
                std::sort(locations.begin(), locations.end());

                std::ofstream facilityOutputFile(argv[6]);
                if (!facilityOutputFile)
                {
                    std::cerr << "***Failed to open facility output file=" << argv[6] << std::endl;
                    return 1;
                }

                facilityOutputFile << "#" << std::endl;
                facilityOutputFile << "# Generated by GRASP solver interface" << std::endl;
                facilityOutputFile << "#" << std::endl;

                facilityOutputFile << rand() << " "; // the ID
                facilityOutputFile << numFacs << " "; // the number of sensors
                for (size_t i = 0; i < locations.size(); ++i)
                {
                    facilityOutputFile << locations[i] << " ";
                }
                facilityOutputFile << std::endl;

                facilityOutputFile.close();

                std::cout << "Best solution written to file=" << argv[6] << std::endl;
            }
            if (argc == 8) {
                if (!first_json_solution) jsonOutputFile << "," << std::endl;
                first_json_solution = false;
                jsonOutputFile << "  {" << std::endl;
                jsonOutputFile << "  \"value\": " << minCost << "," << std::endl;
                jsonOutputFile << "  \"ids\": [ ";
                bool first=true;
                for (size_t i = 0; i < locations.size(); ++i)
                {
                    if (!first)
                        jsonOutputFile << ", ";
                    jsonOutputFile << locations[i];
                    first = false;
                }
                jsonOutputFile << " ]" << std::endl;
                jsonOutputFile << "  }";
            }
        }

        sumCosts += localOptimumCost;

        if (localOptimumCost > maxCost)
        {
            maxCost = localOptimumCost;
        }

        std::cout << std::endl;

        delete initialPoint;
        delete localOptimum;

        if (timeLimit > 0.0)
        {
            // TNT: Added to display the time it took for a single iteration.
            std::cout << "Iteration #" << i << " " << (WallClockSeconds() - WallClockIter_start) << std::endl;;
            if ((WallClockSeconds() - WallClockSeconds_start) > timeLimit) { break; }
        }
    }

    if (argc == 8) {
        jsonOutputFile << std::endl << "]" << std::endl;
        jsonOutputFile << "}" << std::endl;
        jsonOutputFile.close();
    }

    std::cout << std::endl;
    double now_time = CPUSeconds();
    std::cout << "CPU RunTime=         " << (now_time - runtime_start) << std::endl;
    std::cout << "CPU TotalTime=       " << (now_time - CPUSeconds_start) << std::endl;
    std::cout << "WallClock TotalTime= " << (WallClockSeconds() - WallClockSeconds_start) << std::endl;

    std::cout << std::endl;
    std::cout << "Summary statistics" << std::endl;
    std::cout << "Minimum cost= " << minCost << std::endl;
    std::cout << "Average cost= " << sumCosts / double(numSamples) << std::endl;
    std::cout << "Maximum cost= " << maxCost << std::endl;
    std::cout << std::endl;

    delete bestSolution;
    delete instance;
    return 0;
}
