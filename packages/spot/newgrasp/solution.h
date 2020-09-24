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

// Author: Haixia Jia
// Created on May 28th 2008.
// The base class.
//

#ifndef SOLUTION_H
#define SOLUTION_H

#include <teva_config.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "ProblemInstance.h"
#define MY_MAX 10e20
#define OPTIMER

#ifdef OPTIMER
  typedef double* myDouble1;
  typedef double** myDouble2;
  typedef int* myInt1;
  typedef int** myInt2;
  typedef bool* myBool1;
#else 
#if 0
  typedef std::vector<double> myDouble1;
  typedef std::vector<std::vector<double> > myDouble2;
  typedef std::vector<bool> myBool1;
  typedef std::vector<int> myInt1;
  typedef std::vector<std::vector<int> > myInt2;
#else
  typedef utilib::BasicArray<double> myDouble1;
  typedef utilib::BasicArray<utilib::BasicArray<double> > myDouble2;
  typedef utilib::BasicArray<bool> myBool1;
  typedef utilib::BasicArray<int> myInt1;
  typedef utilib::BasicArray<utilib::BasicArray<int> > myInt2;
#endif
#endif


namespace NewSensorPlacement {
class ProblemSolution {
  protected:
    ProblemInstance *myInstance;
    int numP; // number of facilities in the solution 
    int numN; // number of customers
    int numM; // number of potential facilities
    myInt1 myFacilityList;
    myInt1 myFacilityPosition;
    myInt1 myClosestFacility;
    myInt1 mySecondClosestFacility;
    myDouble1 myClosestDist;
    myDouble1 mySecondClosestDist;
    
    void updateAddOneFac(int numF, int numC);
    void updateAddOneFac(int numF);
    void updateRemoveOneFac(int numF, int numC);
    void updateRemoveOneFac(int numF);
    // Most generic descend function, without use moveEvaluator, but slower;
    bool descendOnce(double gamma, myInt1 facilityLocationConstraints);
    // Generic descend function, use moveEvaluator, find the best swap pair and swap it;
    bool descendBest(double gamma, myInt1 facilityLocationConstraints);
    // Generic descend function, use moveEvaluator, find the first pair which will give some
    // cost improvemet and swap it; I don't know if this is any better or worst than descendBest;
    bool descendHelper(double gamma, myInt1 facilityLocationConstraints);
  public:	

    ProblemSolution(ProblemInstance *inst);
    ~ProblemSolution(void);

    // swap two facility position in the facility list;
    void swappos(int i, int j);
    // copy a solution from s;
    void setSolution(ProblemSolution &s);
    // read a solution from the input file
    void readSolution(const char *filename);  
    inline ProblemInstance *getInstance(void)const{
      return myInstance;
    } 
    // number of facilities in the solution
    inline int getNumP(void)const {
      return numP;
    } 
    // number of customers
    inline int getNumN(void)const {
      return numN;
    } 
    // number of facilities to open
    inline int getNumM(void)const {
      return numM;
    } 
    // get label of the i-th facility
    inline int getFacility(int i)const {
      return myFacilityList[i];
    } 
    // get position of the i-th facility
    inline int getPosition(int i)const {
      return myFacilityPosition[i];
    } 
    inline bool isOpenFac(int i)const
    {
      return (myFacilityPosition[i]<= numP);
    }
    inline int getClosest(int i) const {
      return myClosestFacility[i];}
    inline int getSecondClosest(int i) const {
      return mySecondClosestFacility[i];}
    inline double getDistClosest(int i) const {
      return myClosestDist[i];}
    inline double getDistSecondClosest(int i) const {
      return mySecondClosestDist[i];}
    void updateClosest(int i);
    // update closest and second closest list;
    void updateClosest();

    // given a facToOpen, return the cost;
    virtual double moveEvaluator(int facToOpen, double gamma) = 0;
    virtual double moveEvaluator(int facToOpen, int& numWorstCases) = 0;
    // descend until no improvement is possible;
    virtual void descend(double gamma, myInt1 facilityLocationConstraints);
    // return the cost of current open facility;
    virtual double cost(double gamma=0.05)const = 0; 
    // This cost function will only be used for initial solution;
    void reset(void);                         // remove all facilities
    void addOneFac(int f, bool update);             // add f to the solution
    void swapFac(int rem, int ins, bool update); // swap facilities rem and int
    void removeOneFac(int f, bool update);          // remove f from solution

    void pprint(std::ostream& os);

};
};

#endif
