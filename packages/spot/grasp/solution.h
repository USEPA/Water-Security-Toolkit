// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef SOLUTION_H
#define SOLUTION_H

#include "instance.h"

#include <fstream>

class PMSolution 
{
 private:

  PMInstance *myInstance;
  int myP; // number of facilities in the solution (may differ from instance)
  int myN; // number of customers
  int myM; // number of potential facilities
  int *myFacilityList;     // list of all facilities, first p are the solution
  int *myFacilityPosition; // myFacilityPosition[i] = j  -> facility i is in myFacilityList[j]		
  int *myClosestFacility;       // myClosestFacility[i] = j -> facility j (which must be in the solution) is the closest to i
  int *mySecondClosestFacility; // mySecondClosestFacility[i] = j -> facility j is the second closest to i
  double *myDistanceToClosestFacility;       
  double *myDistanceToSecondClosestFacility; 
  bool *myConsistent; 
  int myInconsistencies;

  inline bool isConsistent(void)const {return (myInconsistencies==0);}

  void fatal(const char *func, const char *msg)const; // standard error message
  void swappos(int i, int j); // swap facilities in positions i and j

 private:

  // permamently disable default construction (a solution requires an instance).
  PMSolution(void);

  // temporarily disable copy construction and assignment.
  PMSolution(const PMSolution&);
  PMSolution &operator=(const PMSolution&);

 public:	

  //////////////////////////
  // non-domain C++ stuff //
  //////////////////////////

  PMSolution(PMInstance *inst);
  void copyFrom(PMSolution &s);
  void copyFromNoInstance(PMSolution &s);
  ~PMSolution(void);

  /////////
  // i/o //
  /////////

  void readSolution(const char *filename);  // read a solution from the input file
  void printSolution(std::ostream &ostr, const char *name=0)const; // print solution in DIMACS-like format
  void printSolution(const char *filename, const char *name=0)const; //same thing

  /////////////////////////////
  // basic attribute queries //
  /////////////////////////////

  inline PMInstance *getInstance(void)const
    {
      return myInstance;
    } 

  // label of the i-th facility
  inline int getFacility(int i)const 
    {
      return myFacilityList[i];
    } 

  // number of facilities in the solution
  inline int getP(void)const 
    {
      return myP;
    } 

  // number of customers
  inline int getN(void)const 
    {
      return myN;
    } 

  // number of facilities to open
  inline int getM(void)const 
    {
      return myM;
    } 
  
  // position of the i-th facility
  inline int getPosition(int f)const 
    {
      return myFacilityPosition[f];
    } 

  inline bool contains(int i)const
    {
      return (myFacilityPosition[i]<=myP);
    }

  //-----------------------
  // closeness information
  //-----------------------
  inline int    getClosest      (int i) const {return myClosestFacility[i];}
  inline int    getClosest2     (int i) const {return mySecondClosestFacility[i];}
  inline double getDistClosest  (int i) const {return myDistanceToClosestFacility[i];}
  inline double getDistClosest2 (int i) const {return myDistanceToSecondClosestFacility[i];}
  inline void   setClosest      (int i, int f) {setClosest (i,f,myInstance->getDist(i,f));}		
  inline void   setClosest2     (int i, int f) {setClosest2(i,f,myInstance->getDist(i,f));}
  inline void   setClosest      (int i, int f, double v) {myClosestFacility[i] = f; myDistanceToClosestFacility[i] = v;}
  inline void   setClosest2     (int i, int f, double v) {mySecondClosestFacility[i] = f; myDistanceToSecondClosestFacility[i] = v;}
  void ensureConsistency(void); //ensure solution is consistent
  void updateClosest(int i);

  //------------------------------
  // dealing with other solutions
  //------------------------------

  int computeDifference(const PMSolution &s)const; // symmetric diff. between this and another solution
  bool equals(const PMSolution &s)const;           // check if this is identical to s

  //---------------------
  // solution management
  //---------------------

  double cost(double gamma=0.0)const; 
  double normalizedCost(double gamma=0.0)const;
  double worstCaseCost(double gamma=0.0)const; 
  int numWorstCaseScenarios(double gamma=0.0)const;
  double varCost(double gamma=0.05)const;
  double tceCost(double gamma=0.05)const;
  void reset(void);                         // remove all facilities
  void add(int f, bool update);             // add f to the solution
  void swap(int rem, int ins, bool update); // swap facilities rem and int
  void remove(int f, bool update);          // remove f from solution

  //------------------------------------------------
  // check if f is close enough so as to become the
  // closest or second closest --- if it is, update
  //------------------------------------------------
  inline void upgrade (int u, int f, double duf) 
    {
      // CAP, changing to require strict inequality to win either first place or second
    if (duf < getDistClosest2(u)) {       //only interested if closer than 2nd closest
      double d1 = getDistClosest(u);
      if (duf < d1) {                   //if also closer than first closest...
	setClosest2 (u,myClosestFacility[u],d1); //old closest becomes 2nd closetst
	setClosest  (u, f, duf);       //f is the new closest
      } else {
	setClosest2 (u, f, duf);       //f becomes new second closest
      }
    }
  }

  //--------------------------------
  // Marks vertex i as inconsistent
  //--------------------------------
  inline void makeInconsistent(int i) 
    {
      if(myConsistent[i]) 
	{
	  myConsistent[i]=false;
	  myInconsistencies++;
	}
    }

  //--------------------------------------------
  // marks vertex i as consistent --- call this 
  // only if you know what you're doing
  //--------------------------------------------
  void forceConsistent(int i) 
    {
      if(myConsistent[i]==false) 
	{
	  myConsistent[i] = true;
	  myInconsistencies--;
	}
    }
};

#endif
