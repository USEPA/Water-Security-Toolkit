// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/*******
 *
 * class PMGraphInstance: 
 * 
 *******/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "hybrid_instance.h"
#include "graph_instance.h"
#include "matrix_instance.h"
#include "basics.h"
#include "bossa_timer.h"
#include "solution.h"

/***********************************
 *
 * error message
 *
 ***********************************/

void PMHybridInstance::fatal (const char *func, const char *msg) {
  fprintf (stderr, "PMHybridInstance::%s: %s.\n", func, msg);
  exit(1);
}

/************************************
 *
 * default constructor; not used for
 * hybrid instances
 *
 ************************************/

PMHybridInstance::PMHybridInstance() {
  fatal ("PMHybridInstance", "default constructor cannot be called for hybrid instances");
}



void PMHybridInstance::original2sub (PMSolution *solorig, PMSolution *solsub) {
  if (solorig->getInstance() != original) fatal ("orig2sub", "original solution refers to unknown instance");
  if (solsub->getInstance() != this) fatal ("orig2sub", "subproblem solution refers to unknown instance");

  bool verbose = false;
  if (verbose) fprintf (stderr, "Building solution to subproblem. Original value is %.2f.\n", solorig->cost());

  solsub->reset();

  int f0, fs;
	
  for (f0=1; f0<=solorig->getM(); f0++) {
    if (solorig->contains(f0) && fixed_original[f0]) {
      solsub->add(1, true);
    }
  }

  for (fs=2; fs<=m; fs++) { //don't consider the zero-th facility
    f0 = flabel[fs];
    if (solorig->contains(f0)) {
      solsub->add(fs, true);
    }
  }

  /*
    solorig->ensureConsistency();
    int c0, cs;
    for (cs=2; cs<=solsub->getN(); cs++) {
    c0 = clabel[cs];
    }
  */
  solsub->ensureConsistency();

  if (verbose) fprintf (stderr, "Solution build. Value is %.2f.\n", solsub->cost() + getBaseCost());
}


/*****************************************************************	
 *
 * restoreSolution: transforms a solution to this hybrid instance
 * into a solution to the original instance on which it is based
 *
 *****************************************************************/

void PMHybridInstance::restoreSolution (PMSolution *solcur, PMSolution *solorg) {
  if (solcur->getInstance() != this) fatal ("restoreSolution", "can only deal with own solutions");
  if (solorg->getInstance() != original) fatal ("restoreSolution", "can only transform into solution of original base instance");

  solorg->reset();
  if (solcur->contains(1)) {
    for (int i=1; i<=original->getN(); i++) {
      if (fixed_original[i]) solorg->add(i, false);
    }
  }

  for (int f=2; f<=m; f++) {
    if (solcur->contains(f)) {solorg->add(flabel[f], false);}
  }

  //--------------------------------------------------
  // linear time computation of closeness information
  //--------------------------------------------------	
  int i;

  for (i=1; i<=original_n; i++) {
    solorg->setClosest (i, 0);
    solorg->setClosest2 (i, 0);
  }

  for (i=2; i<=n; i++) { //for every 'free' user
    int f = solcur->getClosest(i);
    int oi = clabel[i]; //label of this user in the original solution
    int of = flabel[f];
    solorg->setClosest (oi, of);
		
    f = solcur->getClosest2(f);
    of = flabel[f];
    solorg->setClosest2 (oi, of);
  }

  for (i=1; i<=original_n; i++) { 
    double d = solorg->getInstance()->getDist(i,original_closest[i]);
    solorg->upgrade (i, original_closest[i],d);
    d = solorg->getInstance()->getDist(i,original_closest_2[i]);
    solorg->upgrade (i, original_closest_2[i], d);
  }

  for (i=1; i<=original_n; i++) solorg->forceConsistent(i);

  double orgcost = solorg->cost();	
  double curcost = solcur->cost()+getBaseCost();

  if ((orgcost-curcost>EPSILON) || (orgcost-curcost<-EPSILON))
    fatal ("restoreSolution", "invalid reconstruction");

  int verbose = 0;
  if (verbose) fprintf (stderr, "Restored solution with %d facilites and value %.2f.\n", solorg->getP(), solorg->cost());
}



/*************************************************
 *
 * constructor
 *
 *************************************************/

PMHybridInstance::PMHybridInstance (PMSolution *s, PMSolution *t) {
  int verbose = 0;
  oracle = 0;
  clabel = 0;
  flabel = 0;
  cud = 0;
			
  //----------------------------------
  // get data about original instance
  //----------------------------------
  original = s->getInstance();
  s->ensureConsistency();
  t->ensureConsistency();
  original_n = s->getN();
  original_m = s->getM();

  //--------------------------
  // allocate data structures
  //--------------------------
  clabel = new int [original_n+2]; //customer labels
  flabel = new int [original_m+2]; //facility labels
  cud = new double [original_n+2]; //distances for customers to universal facilites
  fixed_original = new bool [original_m+1];
  original_closest = new int [original_n+1];
  original_closest_2 = new int [original_n+1];

  clabel[0] = flabel[0] = 0; //zero maps zero
  cud[0] = POPSTAR_INFINITY;         //distance from zero to universal facility
  n = m = 0;                 //we start with zero of everything

  base_cost = 0;      //fixed cost; cost of all customers not inserted
  int base_count = 0; //number of elements in the facility
  int i, f;

  //----------------------------------------
  // find set of fixed facilities --- those
  // that are in both solutions
  //----------------------------------------
  p = original->getP() + 1; //to start with 
  for (f=1; f<=s->getM(); f++) {
    fixed_original[f] = (s->contains(f) && t->contains(f));
    if (fixed_original[f]) p--;
  }

  //-----------
  // set users
  //-----------
  if (verbose) fprintf (stderr, "Setting users...\n");
  n = 1; //first user is a fake one
  clabel[1] = 0;

  for (i=1; i<=s->getN(); i++) {
    cud[i] = POPSTAR_INFINITY;
    int fs = s->getClosest(i); //closest facility in s
    int ft = t->getClosest(i); //closest facility in t

    original_closest[i] = original_closest_2[i] = 0;


    //if both facilities in both solutions, i is considered covered
    if (fixed_original[fs] && fixed_original[ft]) {
      base_count ++;
      base_cost += s->getDistClosest(i);
			
      if (s->getDistClosest(i) <= t->getDistClosest(i)) {
	original_closest  [i] = fs;
	original_closest_2[i] = ft;
      } else {
	original_closest  [i] = ft;
	original_closest_2[i] = fs;
      }
    } else { //at least one facility is not in both solutions
      n++;
      clabel[n] = i;
    }
  }

  //--------------------------------------------------------------------------------
  // distance from user 1 (the fake user) to facility 1 (the fake facility)
  // is zero; the distance from this user to everbody else is infinity (very large)
  //--------------------------------------------------------------------------------
  cud[1] = 0;

  if (verbose) fprintf (stderr, "Instance has %d customers.\n", n);
  if (verbose) fprintf (stderr, "Base cost: %.2f (corresponding to %d users).\n", base_cost, base_count);

  //----------------
  // set facilities
  //----------------
  if (verbose) fprintf (stderr, "Setting facilities (%d)...\n", s->getM());
  m = 1; //1 will be the universal facility	
  flabel[1] = 0;

  for (f=1; f<=s->getM(); f++) { //loop on the original facilities
    int c = 0;
    c += s->contains(f) ? 1 : 0;
    c += t->contains(f) ? 1 : 0;

    if (c==1) { //facility in one solution: put it here
      m++; 
      flabel[m] = f; 
    } else if (c==2) { //facility in two solutions: add to the universal thing
      updateOriginalCloseness (f);
      /*
	for (int i=1; i<=n; i++) {
	int oi = clabel[i];
	double d = original->getDist(oi,f);
	if (d<cud[i]) cud[i]=d;
	}*/
    }
  }
  if (verbose) fprintf (stderr, "Instance has %d potential facilities.\n", m);
  if (verbose) fprintf (stderr, "Our goal is to find a subset of %d facilities.\n", p);

  initOracle();
}

/*****************************************************************
 * 
 * updateOriginalCloseness: takes a FIXED facility f as input and,
 * for all NON-FIXED vertices, determine if f is a better closest
 * or second closest fixed facility. Also updates cud, the 
 * distance from users to fixed facilities.
 *
 *****************************************************************/

void PMHybridInstance::updateOriginalCloseness (int f) {
  for (int i=2; i<=n; i++) { //user 1 is not a real user!
    int oi = clabel[i];                 //original label of i
    //fprintf (stderr, "(Original label of %d is %d.)", i, oi);
    double d = original->getDist(oi,f); //distance to the facility
    double d2 = original->getDist(oi,original_closest_2[oi]);

    if (d2 <= d) continue; //not close enough
    double d1 = cud[i]; //which is the same as original->getDist(oi,original_closest[oi]);

    if (d < d1) {
      original_closest_2[oi] = original_closest[oi];
      original_closest[oi] = f;
      cud[i] = d;
    } else {
      original_closest_2[oi] = f;
    }
  }
}


/*********************************
 *
 * destructor
 *
 *********************************/

PMHybridInstance::~PMHybridInstance() {
  delete oracle;
  if (cud) delete [] cud;
  if (clabel) delete [] clabel;
  if (flabel) delete [] flabel;
  if (fixed_original) delete [] fixed_original;
  if (original_closest) delete [] original_closest;
  if (original_closest_2) delete [] original_closest_2;
}


/***********************************************
 *
 * distance from one facility to another;
 * doesn't make sense for this kind of instance
 *
 ***********************************************/

double PMHybridInstance::getFacDist (int f, int g) {
  fatal ("getFacDistance", "function cannot be called for hybrid instances");
  return 0.0;
}


/***************************************
 * 
 * distance from user u to facility f
 *
 **************************************/

double PMHybridInstance::getDist (int u, int f) {
  if (u<0 || u>n) fatal ("getDist", "customer number out of range");
  if (f<0 || f>m) fatal ("getDist", "facility number out of range");
	
  if (u==1) { //fake user
    if (f==1) return 0; //distance zero to the fake facility
    return POPSTAR_INFINITY;    //infinity to all other facilities
  }

  if (f==1) return cud[u]; //fake facility		
  if (f==0) return POPSTAR_INFINITY;
			
  return original->getDist(clabel[u], flabel[f]);
}
