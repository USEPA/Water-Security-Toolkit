// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef ELITE_H
#define ELITE_H

#include "solution.h"
#include "instance.h"

class PMElite
{
 private:
  int cap;          //total number of solutions allowed
  PMSolution **s;   //list of solutions
  double *v;        //list of solution values
  PMInstance *inst; //instance to which the solutions... are solutions
  int consistent;   //number of consistent solutions

  void fatal (const char *func, const char *msg);
  void promote(int i);
  void sort();
  int getMostSimilar (PMSolution *t, int p1, int p2);	
  bool isUnique(PMSolution *t);

 public:
  PMElite (PMInstance *_inst, int _cap);
  int getRandomDifferent(PMSolution *t, int p1, int p2);
  void outputToFiles (const char *prefix);
  void outputDifferences (FILE *file);
  void output (FILE *file, bool complete = false);
  double getSolution (int i, PMSolution *sol);
  bool add(PMSolution *t, double c); //, int pos=0);
  void computeDifferences (int *diff, PMSolution *t);
  int getMostDifferent (PMSolution *t);
  void reset();
  void checkConsistency (int i);
  ~PMElite();

  inline int count () {return cap;}
  inline int countConsistent () {return consistent;}
  inline PMInstance* getInstance() {return inst;}
  inline int getCapacity() {return cap;}
  inline double getSolutionCost (int i) {return v[i];}

};

#endif
