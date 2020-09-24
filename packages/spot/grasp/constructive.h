// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/**************************
 *
 * Constructive algorithms
 *
 **************************/

#ifndef CONSTRUCTIVE_H
#define CONSTRUCTIVE_H

#include "solution.h"

#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <vector>

class PMConstructive 
{
 private:

  enum {RPG, RGREEDY, PGREEDY, PWORST, PREVERSE, MST, SAMPLE, MIX, RANDOM, PCENTER, NMETHODS}; // method types
  char method_name[256];  //name of the current method
  double param[NMETHODS]; //default parameters for all methdos
  int method;              //current method
  static void fatal(const char *func, const char *msg);
  bool tryMethod(char *m, const char *name, int code, int np);

  static void checkFixed (PMSolution *s, const char *method);
  static void checkNotFixed (PMSolution *s, const char *method);

 public:

  PMConstructive(void) {setMethod ("rpg:1");} //constructor; default is random solution

  /*--------------------------------------------------
    | routines dealing the 'current' or default method
    *-------------------------------------------------*/
  void setMethod(char *m);              // define current method
  void printMethod(std::ostream &ostr); // print name of current method
  void run(PMSolution *s,
	   int k,
	   const std::vector<int> &facilityLocationConstraints);        // run current method

  /*-------------------------------------
    | constructive algorithms for fixed p
    *------------------------------------*/
  static void addRandom (PMSolution *s, int k, const std::vector<int> &facilityLocationConstraints); // k is the actual number to *add*
  static void pworst    (PMSolution *s, int k, int *candlist=0, double exponent=1, bool stats=false);
  static void greedy    (PMSolution *s, int k, bool proportional, double alpha,int *candlist,const std::vector<int> &facilityLocationConstraints);
  static void rpg       (PMSolution *s, int k, double rfrac, bool stats,const std::vector<int> &facilityLocationConstraints);
  static void preverse  (PMSolution *s, int k, bool stats=false);
  static void sample    (PMSolution *s, int k, double frac, bool stats, const std::vector<int> &facilityLocationConstraints);
  static void pcenter   (PMSolution *s, int k, const std::vector<int> &facilityLocationConstraints);

  /*----------------------------------------
    | constructive algorithms for variable p
    *---------------------------------------*/
  static void random(PMSolution *s, double prob=0.5, bool stats=false); //facility
};

#endif
