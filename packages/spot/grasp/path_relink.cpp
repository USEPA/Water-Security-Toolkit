// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include "path_relink.h"
#include "hybrid_instance.h"
#include "constructive.h"
#include "search.h"

#include <vector>

void PMRelink::fatal (const char *func, const char *msg) {
  fprintf (stderr, "PMRelink::%s: %s.\n", func, msg);
  exit (1);
}


/************************************************************
 *
 * performs path relinking from s1 to s2; result goes to t
 *
 ************************************************************/

double PMRelink::combinePR(PMSolution *t, PMSolution *s1, PMSolution *s2, const std::vector<int> &facilityLocationConstraints) 
{
  PMInstance *inst = t->getInstance();
  PMSearchPathRelinking *sm = new PMSearchPathRelinking (inst);
  PMElite *single_elite = new PMElite (inst, 1);

  sm->setSolutions (s1, s2);  //set both solutions
  single_elite->reset();      //reset set of elite solutions
  t->copyFrom(*s1);
  sm->search(t, facilityLocationConstraints, true, single_elite, false); //perform path-relinking

  double cost;
  if (single_elite->countConsistent() == 0) { //no local minimum was found
    if (BossaRandom::getInteger(1,2) == 1) {
      t->copyFrom(*s1);
      cost = s1->cost();
    } else {
      t->copyFrom(*s2);
      cost = s2->cost();
    }
  } else {
    cost = single_elite->getSolution (1, t); 
  }
		
  //deallocate structures
  delete single_elite;
  delete sm;

  return cost;

}


/********************************************************
 *
 * Combines s1 and s2, creating a new solution t, by
 * solving a subproblem containing only facilities that
 * are different in both cases
 *
 ********************************************************/

/*
  double PMRelink::combineSub (PMSolution *t, PMSolution *s1, PMSolution *s2) {	
  double t_init, t_solve, t_recover;

  BossaTimer timer(true);
  PMHybridInstance *subinst = new PMHybridInstance (s1, s2);
  PMSolution       *subsol  = new PMSolution (subinst);
  PMConstructive constructive;
  t_init = timer.getTime();

  timer.start();
  constructive.setMethod ("rpg:1");
  //PMElite *subelite = new PMElite (subinst, 4);
	
  fprintf (stderr, "WARNING: THIS FEATURE IS NOT PROPERLY IMPLEMENTED!!!\n");
  exit (-1); //the following line should be executed
  return 0.0;

  //grasp (subsol, &constructive, 4, 0, "s", "r:u", false, false);
  t_solve = timer.getTime();
	
  timer.start();
  subinst->restoreSolution (subsol, t);

  //delete subelite;
  delete subsol;
  delete subinst;

  double cost = t->cost();
  t_recover = timer.getTime();

  fprintf (stderr, "[%.2f:%.2f:%.2f] ", t_init, t_solve, t_recover);
  return cost;
  }
*/

double PMRelink::combineSubPR(PMSolution *t, PMSolution *s1, PMSolution *s2, const std::vector<int> &facilityLocationConstraints) 
{
  double t_init, t_initsub, t_build, t_solve, t_recover;

  BossaTimer timer(true);
  bool verbose = false;

	
  //PMInstance *inst = t->getInstance();
  PMHybridInstance *subinst = new PMHybridInstance (s1, s2);
  t_init = timer.start();
	
  PMSolution *sub_s1 = new PMSolution (subinst);
  PMSolution *sub_s2 = new PMSolution (subinst);
  PMSolution *sub_t  = new PMSolution (subinst);
  t_initsub = timer.start();

  subinst->original2sub (s1, sub_s1);
  subinst->original2sub (s2, sub_s2);
  t_build = timer.start();

  combinePR(sub_t, sub_s1, sub_s2,facilityLocationConstraints);
  t_solve = timer.start();
  subinst->restoreSolution (sub_t, t);
  t_recover = timer.getTime();
  delete sub_t;
  delete sub_s2;
  delete sub_s1;
  delete subinst;

  double cost = t->cost();
  if (verbose) fprintf (stderr, "Solution value is %.2f.\n", cost);
  fprintf (stderr, "[%.2f:%.2f:%.2f:%.2f:%.2f] ", t_init, t_initsub, t_build, t_solve, t_recover);
  return cost;
}

double PMRelink::combinePartial(PMSolution *t, 
				const std::vector<int> &facilityLocationConstraints,
				PMSolution *s1, 
				PMSolution *s2) 
{
  PMInstance *inst  = t->getInstance();
  PMSolution *start = new PMSolution (inst);
  PMSolution *best  = new PMSolution (inst);
  double best_value;
  int p = inst->getP();
  //int n = inst->getN();


  int i;
  //----------------------------------------
  // build list of candidates for insertion
  //----------------------------------------
  int *cand = new int [p+1];
  cand[0] = 0;
  for (i=1; i<=s2->getP(); i++) {
    int f = s2->getFacility(i);
    if (s1->contains(f)) continue;
    cand[0]++;
    cand[cand[0]] = f;
  }

  best->copyFrom(*s1);
  best_value = POPSTAR_INFINITY; //	best->cost();
	
  int maxdiff = 3;
  if (maxdiff > cand[0]/2) maxdiff = cand[0]/2;
  int min = 0;

  for (int it=1; it<=5; it++) {
    int x = BossaRandom::getInteger (min, maxdiff);
    if (x==0) min = 1;
    start->copyFrom(*s1);
		
    partialShuffle (cand, 1, cand[0], x);
    for (int i=1; i<=x; i++) {
      int in = cand[i];
      int out;
      double profit;
      PMSearch::move(start,facilityLocationConstraints,in,out,profit);
      start->swap(out, in, true);
      //fprintf (stderr, "(%d->%d) ", out, in);
    }
    double v = combinePR (t, start, s2, facilityLocationConstraints);
    fprintf (stderr, "[%.2f]", v);
    if (v < best_value) {
      best_value = v;
      best->copyFrom(*t);
    }
  }

  t->copyFrom(*best);
  delete [] cand;
  delete start;
  delete best;
  return t->cost();

}


/******************************************
 *
 * Combine two solutions to find a new one
 *
 ******************************************/

double PMRelink::combine_r (PMSolution *t, PMSolution *s1, PMSolution *s2, const char* method, const std::vector<int> &facilityLocationConstraints) 
{
  if (method[0] != 'r') fatal ("combine_r", "invalid method");
  if (method[1] != ':') fatal ("combine_r", "wrong format");

  if (method[2] == 'b') {
    double v1 = combinePR (t, s1, s2, facilityLocationConstraints);
    PMSolution *tcopy = new PMSolution (t->getInstance());
    tcopy->copyFrom(*t);
    double v2 = combinePR(t, s2, s1, facilityLocationConstraints);
    if (v2 > v1) t->copyFrom(*tcopy);
    delete tcopy;
    return t->cost();
  } else {
    bool direct = true; //one to two?
    switch (method[2]) {
    case 'u': direct = (s1->cost()<=s2->cost()); break;
    case 'd': direct = (s1->cost()>s2->cost()); break;
    case 'r': direct = (BossaRandom::getInteger(0,1) == 1); break;
    case 's': direct = true; break;
    case 'i': direct = false; break;
    default: fatal ("combine_r", "invalid parameter for method 'r'");
    }
    if (direct) return combinePR (t, s1, s2, facilityLocationConstraints);
    else return combinePR (t, s2, s1, facilityLocationConstraints);
  }
}



double PMRelink::combine(PMSolution *t, 
			 const std::vector<int> &facilityLocationConstraints,
			 PMSolution *s1, 
			 PMSolution *s2, 
			 const char* method) 
{
  if (s1->computeDifference(*s2) < 2) 
    {
      t->copyFrom(*s1);
      return t->cost();
    }
  if (method==0) fatal ("combine", "invalid method");
  double result = 0;

  switch (method[0]) 
    {
      //case 's': result=combineSub(t, s1, s2); break;
    case 'p': result=combinePartial(t, facilityLocationConstraints, s1, s2); break;
    case 'r': result=combine_r(t, s1, s2, method, facilityLocationConstraints); break;
    case 'X': result=combineSubPR(t, s1, s2, facilityLocationConstraints); break;
    case 'n': t->copyFrom(*s1); result=t->cost(); break;
    default: fatal ("combine", "unrecognized method"); 
    }
  return result;
}
	


/**************************************************
 *
 * combine: merges two solutions (s1, s2) creating
 *          a third one.
 *
 **************************************************/

/*
  double PMRelink::combine2 (PMSolution *t, PMSolution *s1, PMSolution *s2) {
  fatal ("combine2", "deprecated function");
  return 0.0;

  PMInstance *inst = t->getInstance();
  int ins;
  int i;
  int p = inst->getP();
  int *candidates = new int [p+1];

  fprintf (stderr, "Combining solutions...\n");

  //--------------------------------------------
  // add p/2 facilities from the first solution
  //--------------------------------------------
  for (i=1; i<=p; i++) candidates[i] = s1->getFacility(i);
  RFW::shuffle (candidates, p); //we'll select the facilities at random

  t->reset();
  ins = 0;
  while (ins < p/2) {
  ins++;
  t->add(candidates[ins], true);
  }

  //---------------------------------------------
  // add p/2 facilities from the second solution
  //---------------------------------------------
  for (i=1; i<=p; i++) candidates[i] = s2->getFacility(i);
  RFW::shuffle (candidates, p);

  i = 0;
  while (ins < p) {
  i++;
  int c = candidates[i];
  if (t->contains(c)) continue;
  ins++;
  t->add(c, true);
  }

  delete [] candidates;
 
  //localSearch (t, true, 0);

  fprintf (stderr, "done combining.\n");

  return t->cost();
  }
*/


/***************************************************************
 *
 * pathRelinking: execute multiple-generation path-relinking, 
 *     considering all pairs of solutions in 'elite'. The best 
 *     solution found in the process is returned in best_s. The 
 *     contents of 'elite' may change.
 *
 ***************************************************************/

void PMRelink::pathRelinking(PMSolution *best_s, 
			     PMElite *elite, 
			     const char *method, 
			     const std::vector<int> &facilityLocationConstraints,
			     bool stats) 
{
  if (elite == 0) fatal ("pathRelinking", "list of elite solutions cannot be 0");

  BossaTimer timer;
  timer.start();
	
  bool advance = true;
  bool debug = false;
	
  PMElite *local_elite;
  PMSolution *s, *source, *target;
  PMInstance *inst = elite->getInstance();

  elite->getSolution (1, best_s);
  int capacity = elite->getCapacity();

  local_elite  = new PMElite (inst, capacity); //will hold the next generation
  s      = new PMSolution (inst); //termporary solution
  source = new PMSolution (inst); //first solution in the path
  target = new PMSolution (inst); //last solution in the path

  double vsource, vtarget; //, profit;
  double best = elite->getSolution (1, source);
  double oldbest = best + 1;

  int generation = 0;

  //save original best solution
  elite->getSolution(1, best_s);

  while (advance) {
    advance = false;
    generation ++;
    int count = elite->countConsistent();
    fprintf (stderr, "GENERATION %d\n", generation);
    oldbest = best;
    local_elite->reset();
    //for (int i=1; i<count; i++) {	
		
    for (int i=count; i>1; i--) {
      //get source
      vsource = elite->getSolution (i, source);
      if (debug) elite->checkConsistency (i);

      for (int j=i-1; j>=1; j--) {
	//for (int j=i+1; j<=count; j++) {
	//if (i==j) continue;

	if (debug) {
	  elite->checkConsistency(j);
	  fprintf (stderr, "Combining solutions (%d:%d); there are %d consistent solutions..\n", i, j, count);
	}
	//------------
	// get target
	//------------
	vtarget = elite->getSolution (j, target);
	PMSolution *r;
	double rvalue1, rvalue2; //before and after local search

	int diff = source->computeDifference(*target);

	//--------------------------------------------------------------------------
	// perform path-relinking: best solution will be in r, its value in rvalue2
	//--------------------------------------------------------------------------
	rvalue1 = rvalue2 = combine(s, facilityLocationConstraints, source, target, method);
	r = s;
	if (r->computeDifference(*source)>1 && r->computeDifference(*target)>1) {
	  PMSearch::localSearch(r, true, false, facilityLocationConstraints);  //try to improve solution with local search
	  rvalue2 = r->cost();      //get the value
	}

	//-------------------
	// update statistics
	//-------------------
	//current = vsource - profit;
	if (rvalue2 < best - EPSILON) {
	  best = rvalue2;
	  fprintf (stderr, "> ");
	} else {
	  fprintf (stderr, "  ");
	}
	fprintf (stderr, "(%02d:%.2f) x (%02d:%.2f) [%d]: %.2f", i, vsource, j, vtarget, diff, rvalue1);
	if (rvalue2 < rvalue1 - EPSILON) fprintf (stderr, " -> %.2f", rvalue2);
	fprintf (stderr, "\n");

	//------------------------------------
	// store solution for next generation
	//------------------------------------
	local_elite->add (r, rvalue2);
      }	
    }
		
    //---------------------------------------------------
    // if there is improvement, prepare a new generation
    //---------------------------------------------------
    if (best < oldbest-EPSILON) {
      advance = true;	                         //there is a new generation
      local_elite->getSolution (1, best_s);    //best_s holds the best solution now
      elite->reset();                          //remove old elite solutions
      count = local_elite->countConsistent();  
      for (int i=1; i<=count; i++) {           //add new set of elite solutions
	vsource = local_elite->getSolution (i, source);
	elite->add (source, vsource);
      }
    }
  }
			

  //------------------
  // print statistics
  //------------------
  if(stats) 
    {
      std::cout << "generations " << generation << std::endl;
      std::cout << "prmethod " << method << std::endl;
      std::cout << "prelite " << elite->getCapacity() << std::endl;
      std::cout << "prtime " << std::setprecision(3) << timer.getTime() << std::endl;
      std::cout << "pr " << std::setprecision(2) << best << std::endl;
    }

  //----------
  // clean up 
  //----------
  delete local_elite;
  delete s;
  delete source;
  delete target;
}



