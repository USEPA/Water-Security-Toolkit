// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include "elite.h"
//#include "basics.h"
#include "bossa_random.h"

#include <iostream>
#include <fstream>
#include <iomanip>

/******************
 *
 * error message
 *
 ******************/

void PMElite::fatal (const char *func, const char *msg) {
  fprintf (stderr, "PMElite::%s: %s.\n", func, msg);
  exit (-1);
}


/***********************************************************
 *
 * promote an instance from the i-th position to its proper
 * position (tries only positions below i)
 *
 ***********************************************************/

void PMElite::promote(int i) {
  while (i>1) {
    if (v[i-1] <= v[i]) break;
    PMSolution *temps;
    double tempv;
    temps = s[i]; tempv = v[i];
    s[i] = s[i-1]; v[i] = v[i-1];
    s[i-1] = temps; v[i-1] = tempv;
    i--;
  }			
}

/*******************************************
 *
 * Sort list of elite solutions by solution
 * value. The algorithm used is insertion
 * sort.
 *
 *******************************************/

void PMElite::sort() {
  for (int i=2; i<=consistent; i++) promote(i);
}


/************************************
 *
 * Copy into 'sol' the i-th solution
 *
 ************************************/

double PMElite::getSolution (int i, PMSolution *sol) {
  sol->copyFrom(*(s[i])); 
  return v[i];
}


/**********************************************
 *
 * returns the number of the solution that 
 * is most similar to t; returns 0 if there is
 * a solution that is too similar
 *
 **********************************************/

int PMElite::getMostSimilar (PMSolution *t, int p1, int p2) {
  int diff, mindiff;
  int sub;
  mindiff = 2 * t->getInstance()->getM() + 1; //a big number
  sub = 0;

  if (p1>p2) fatal ("getMostSimilar", "invalid range");

  for (int i=p1; i<=p2; i++) {
    diff = t->computeDifference(*(s[i]));
    //if (diff<=2) return 0; //there's not point in adding solution is too 
    if (diff==0) fatal ("getMostSimilar", "solution is not unique");
    if (diff<mindiff) {
      mindiff = diff;
      sub = i;
    }
  }
  return sub;
}


/************************************************
 *
 * checks if t is unique: returns true if no
 * other solution is equal to t, false otherwise
 *
 ************************************************/
	
bool PMElite::isUnique(PMSolution *t) 
{
  int c = countConsistent();
  bool unique = true;
  for (int i=1; i<=c; i++) {
    if (t->equals(*(s[i]))) {unique=false; break;}
  }
  return unique;		
}


/*----------------------------------------------------------------
  | Select an element in the pool at random to be combined with s. 
  | The probability of a given element to be selected is 
  | proportional to how different it is from s; function returns 0 
  | if a near-identical solution is found.
  *---------------------------------------------------------------*/

int PMElite::getRandomDifferent(PMSolution *t, int p1, int p2) {
  int diff, sumdiff;
  diff = sumdiff = 0;
  int sub = 0;
			
  for (int i=p1; i<=p2; i++) {
    //if very similar to an existing solution, there's a problem
    diff = t->computeDifference(*(s[i]));
    if (diff == 0) return 0;

    //among those that cost more, select the most similar
    sumdiff += diff;
    if (BossaRandom::getInteger(1,sumdiff)<=diff) sub = i;
  }
  return sub;
}


/*************************************
 *
 * output elite solutions to files
 *
 *************************************/

void PMElite::outputToFiles(const char *prefix) 
{
  for(int i=1;i<=cap;i++) 
    {
      char fileName[256];
      sprintf(fileName,"%s%02d.out",prefix,i);
      std::cout << "Writing file=" << fileName << std::endl;
      std::ofstream outputFile(fileName);
      if(!outputFile)
	{
	  std::cerr << "ERROR - Failed to open output file=" << fileName << std::endl;
	  exit(-1);
	}
      s[i]->printSolution(outputFile);
      outputFile.close();
      std::cout << "Done." << std::endl;
    }
}


/*****************************************************************
 *
 * for every pair of instances, output their symmetric difference
 *
 *****************************************************************/

void PMElite::outputDifferences (FILE *file) {
  for (int i=1; i<=cap; i++) {
    for (int j=1; j<=cap; j++) {
      fprintf (file, "(%d:%f x %d:%f): %5d\n", i, v[i], j, v[j], s[i]->computeDifference(*(s[j])));
    }
  }
}



void PMElite::output (FILE *file, bool complete) {
  fprintf (file, "(%d/%d) ", consistent, cap);
  for (int i=1; i<=cap; i++) {
    if (complete) {
      fprintf (file, "%d: %f ->", i, v[i]);
      PMSolution *t = s[i];
      for (int f=1; f<=t->getP(); f++) {
	fprintf (file, " %d", t->getFacility(f));
      }
      fprintf (file, "\n");
    } else {
      fprintf (file, "%f ", v[i]);
    }
  }
  if (!complete) fprintf (file, "\n");
}		


/**************************************************
 *
 * compute difference from every valid solution in
 * the pool to t; returns the results in diff
 *
 **************************************************/

void PMElite::computeDifferences(int *diff, PMSolution *t) 
{
  int i;
  for (i=1; i<=consistent; i++) {diff[i] = t->computeDifference(*(s[i]));}
  for (i=consistent+1; i<=cap; i++) {diff[i] = 2*t->getM()+1;}
}



/********************************************************
 *
 * Tries to add a solution to the pool:
 * - must be different enough from all cheaper solutions
 * - replaces most similar more expensive solutions
 *
 *********************************************************/

bool PMElite::add (PMSolution *t, double c) { 
  bool debug = false;

  //Minimum distance for two solutions to be considered "different"
  //It is always four "moves"; however, since only swaps are allowed
  //in the p-median problem, this corresponds to 8 units. For facility
  //location, individual insertions and removals are allowed, so
  //four is the correct number.
  int THRESHOLD = (t->getInstance()->fixedP()) ? 8 : 4;
	
  if (c>v[cap]) return false; //not cheap enough

  int bestpos  = 0;
  int bestdiff = 2*t->getM() + THRESHOLD + 1; //a big number

  //Find the correct position to insert the new element, if any
  for (int i=1; i<=cap; i++) {
    if (i>consistent) { //not full yet
      if (bestdiff>=THRESHOLD) bestpos=i;  //add in last position if there's nobody very similar
      break;
    } 
    int diff = t->computeDifference(*(s[i]));
    if (v[i]<c) { //s[i] is cheaper --- will not be replaced
      if (diff<THRESHOLD) {bestpos=0; break;} //s[i] is also too similar; won't add t
    } else { //s[i] is at least as expensive as t
      if (diff<bestdiff) { //if it is similar, will be a good candidate
	bestpos=i;
	bestdiff=diff;
      }
    }
  }

  //insert...
  if (bestpos>0) {
    if (v[bestpos]<c) fatal ("add", "invalid position for insertion");
    s[bestpos]->copyFrom(*t);
    if (bestpos>consistent) consistent = bestpos;
    v[bestpos] = c;
    if (debug) checkConsistency (bestpos);
    sort(); //this is insertion sort, so it takes O(cap) time to update
  } 

  return (bestpos>0); //true if inserted, false otherwise
}


/********************************************
 *
 * get solution that differs the most from t
 *
 ********************************************/

int PMElite::getMostDifferent (PMSolution *t) {
  int c = countConsistent();
  if (c < 1) fatal ("getMostDifferent", "pool is empty");
			
  int besti = 1;
  int bestdiff = t->computeDifference(*(s[1]));
  for (int i=2; i<=c; i++) {
    int diff = t->computeDifference(*(s[i]));
    if (diff > bestdiff) {
      besti = i;
      bestdiff = diff;
    }
  }
  return besti;
}


/**************
 *
 * Constructor
 *
 **************/

PMElite::PMElite (PMInstance *_inst, int _cap) {
  cap = _cap;
  inst = _inst;

  s = new PMSolution* [cap+1];
  v = new double [cap+1];

  for (int i=1; i<=cap; i++) {
    s[i] = new PMSolution (inst);
  }
  reset();
}


/**************************
 *
 * reset list of solutions
 *
 **************************/

void PMElite::reset() {
  consistent = 0;
  for (int i=1; i<=cap; i++) {
    s[i]->reset();
    v[i] = s[i]->cost(); //this should be a very big number
  }
}


/***********************
 *
 * destructor
 *
 ***********************/

PMElite::~PMElite() {
  for (int i=1; i<=cap; i++) delete s[i];
  delete [] s;
  delete [] v;
}


/************************************
 *
 * check the consistency of the node
 *
 ************************************/

void PMElite::checkConsistency (int i) {
  double diff = v[i] - s[i]->cost();
  if (diff>EPSILON || diff<-EPSILON) {
    fprintf (stderr, "(saved:%f calculated:%f)\n", v[i], s[i]->cost());
    fatal ("checkConsistency", "inconsistent solution");
  }
}

