// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include "constructive.h"
#include "bossa_stack.h"
#include "bossa_timer.h"
#include "bossa_random.h"
#include "rfw_stats.h"
#include "bossa_heap.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>

#include <iostream>
#include <iomanip>
#include <vector>

void PMConstructive::fatal(const char *func, const char *msg) 
{
  std::cerr << "PMConstructive::" << func << ": " << msg << std::endl;
  exit(-1);
}

void PMConstructive::printMethod(std::ostream &ostr)
{
  ostr << method_name;
}

/*-----------------------------------------------------------------------
  | Parser for string that specifies a method; returns true if the 
  | string is valid (in which case, relevant local variables are updated)
  *----------------------------------------------------------------------*/

bool PMConstructive::tryMethod(char *m, const char *name, int code, int np) 
{
  if(np==0) 
    {
      if(strcmp(m,name) == 0) 
	{
	  sprintf(method_name,m);
	  method=code;
	  return true;		
	}
    } 
  else if(np == 1) 
    {
      char format[256], buffer[256];
      sprintf(format,"%s:%%s", name);
      if(sscanf(m,format,buffer)==1) 
	{
	  sprintf(method_name,m);
	  method=code;
	  param[method]=atof(buffer);
	  return true;
	}
    }
  return false;
}

void PMConstructive::setMethod(char *m) 
{
  if(tryMethod(m,"rgreedy",RGREEDY,1)) return;
  if(tryMethod(m,"random",RANDOM,1)) return;
  if(tryMethod(m,"rpg",RPG,1)) return;
  if(tryMethod(m,"pgreedy",PGREEDY, 1)) return;
  if(tryMethod(m,"pworst",PWORST, 1)) return;
  if(tryMethod(m,"preverse",PREVERSE, 0)) return;
  if(tryMethod(m,"mix",MIX,0)) return;
  if(tryMethod(m,"sample",SAMPLE,1)) return;
  if(tryMethod(m,"pcenter",PCENTER,0)) return;

  std::cerr << "WARNING: '" << m << "' is not a valid constructive heuristic." << std::endl;
}

/*-------------------------------------------
  | run 'active' method, defined by setMethod
  *------------------------------------------*/

void PMConstructive::run(PMSolution *s, 
			 int k,
			 const std::vector<int> &facilityLocationConstraints)
{
  int m;
  if(method!=MIX) 
    {
      m=method;
    }
  else 
    {
      m=BossaRandom::getInteger(0,MIX-1);
    }

  double v=param[m];

  if(k<=0) 
    {
      if(s->getInstance()->fixedP())
	{ 
	  k=s->getInstance()->getP();
	}
      else
	{
	  k=(s->getInstance()->getM()+1)/2;
	}
    }

  // verify that all facility locations that are constrained to be
  // open actually are open. assumes all checking relating to the
  // number of facilities to open and the number of fixed facilities
  // have been performed up-front.
  for(size_t i=1;i<facilityLocationConstraints.size();i++)
    {
      if(facilityLocationConstraints[i]==1)
	{
	  if(s->contains(i)==false)
	    {
	      s->add(i,true);
	    }
	}
    }

  switch(m) 
    {
    case RPG: 
      rpg(s,k,v,false,facilityLocationConstraints); 
      break;

    case RGREEDY: 
      greedy(s,k,false,v,0,facilityLocationConstraints); 
      break;
      
    case PGREEDY: 
      greedy(s,k,true,v,0,facilityLocationConstraints); 
      break;
      
    case PWORST: 
      std::cout << "WARNING - METHOD DOES NOT ENFORCE FACILITY LOCATION CONSTRAINTS" << std::endl;
      pworst(s,k,0,v,false); 
      break;

    case PREVERSE: 
      std::cout << "WARNING - METHOD DOES NOT ENFORCE FACILITY LOCATION CONSTRAINTS" << std::endl;
      preverse(s,k,false); 
      break;

    case SAMPLE: 
      sample(s,k,v,false,facilityLocationConstraints); 
      break;

    case RANDOM: 
      std::cout << "WARNING - METHOD DOES NOT ENFORCE FACILITY LOCATION CONSTRAINTS" << std::endl;
      random(s,v,false); 
      break;

    case PCENTER: 
      pcenter(s,k,facilityLocationConstraints); 
      break;

    default: 
      fatal("run","invalid constructive heuristic");
    }
}

void PMConstructive::addRandom(PMSolution *s, 
			       int k,
			       const std::vector<int> &facilityLocationConstraints) 
{
  int p=s->getP(); // number of facilities currently in the solution
  int m=s->getM(); // number of potential facilities
  int count = m-p;   // number of candidates
	
  if(k>count) 
    {
      std::cout << "(p=" << p << ", m=" << m << ", k=" << k << std::endl;
      fatal("addRandom","too many facilities");
    }

  std::vector<int> v;
  v.reserve(m+1);

  for(int i=1;i<=count;i++)
    {
      int candidateFacility=s->getFacility(i+p);
      if(facilityLocationConstraints[candidateFacility]!=2)
	{
	  v.push_back(candidateFacility);
	}
    }

  partialShuffle(&(v[0]), 1, count, k);                   // pick the first k at random
	
  for (int i=1;i<=k;i++)
    {
      s->add(v[i],true); // add them to the solution
    }
}


/*------------------------------------------------------
  | Builds a completely random solution: each facility
  | has probability 'prob' of being open. At least one
  | facility will be opened. For facility location only.
  *-----------------------------------------------------*/

void PMConstructive::random(PMSolution *s, double prob, bool stats) 
{
  checkNotFixed(s,"random"); // for facility location only

  BossaTimer timer (true);
  int m = s->getM();
  s->reset(); //ensure that the solution is empty

  // if probability not within range, guess something
  if (prob<0 || prob>1) {prob = BossaRandom::getDouble();}

  // open all facilities
  for(int u=m; u>0; u--) 
    {
      if (BossaRandom::getDouble()<=prob) {s->add(u, false);}
    }

  // make sure at least one facility is opened...
  if (s->getP()==0) {s->add(BossaRandom::getInteger(1,m), false);}
	
  //compute closest, second closest
  s->ensureConsistency();

  // print statistics, if necessary
  if (stats) 
    {
      double result = s->cost();
      std::cout << "random " << std::setprecision(2) << result << std::endl;
      std::cout << "randomtime " << std::setprecision(3) << timer.getTime() << std::endl;
      std::cout << "randomprop " << std::setprecision(3) << prob << std::endl;
    }
}

void PMConstructive::rpg(PMSolution *s,
			 int k,
			 double fractionRandom,
			 bool stats,
			 const std::vector<int> &facilityLocationConstraints) 
{
  checkFixed(s,"rpg"); // for p-median only
	
  if((k<=0)||(k>s->getM())) 
    {
      fatal("rpg","Number of facilities to insert out of range");
    }

  int effectiveK=k-s->getP();
  if(effectiveK<=0)
    {
      // we're already done
      return;
    }

  if((fractionRandom<0.0)||(fractionRandom>1.0))
    {
      fatal("rpg","Parameter rfrac must be on the unit interval");
    }
  
  BossaTimer timer(true);

  // determine the number of random facilities
  int numRandom((int)(ceil(fractionRandom*double(effectiveK))));

  // construct the solution
  addRandom(s,numRandom,facilityLocationConstraints); // add numRandom elements at random
  greedy(s,k,false,0,0,facilityLocationConstraints);  // complete greedily

  // print statistics if necessary
  if(stats) 
    {
      std::cout << "rpg " << std::setprecision(2) << s->cost() << std::endl;
      std::cout << "rpgtime " << std::setprecision(2) << timer.getTime() << std::endl;
    }
}

/*------------------------------------------------------------------
  | Greedy: adds facilities to s one at a time, until p facilities
  |   are inserted. The running time is O(nk), where k is the 
  |   number of times cities 'change hands' during the execution of
  |   the algorithm. One can find trivial upper and lower bounds for
  |   k: n <= k <= np (n is a lower bound if we start from an empty 
  |   solution; if we start with something else we may have k<n).
  |
  | The input solution need not be empty; all facilities already
  | there will be preserved.
  *-----------------------------------------------------------------*/

void PMConstructive::greedy(PMSolution *s,
			    int k,
			    bool proportional,
			    double alpha,
			    int *initlist,
			    const std::vector<int> &facilityLocationConstraints) 
{
  checkFixed(s,"greedy");
  int i, f;
  int verbose(0);
  std::vector<int> candlist; // for backward compatability purposes, the 0th element is unused
  candlist.push_back(0);
  double *savelist=0;

  if((k<=0)||(k>s->getM()))
    { 
      fatal("greedy","invalid number of facilities to insert");
    }

  // if alpha is not within range, we will use a random value
  bool random_alpha=(alpha<0 || alpha>1);

  PMInstance *instance(s->getInstance()); 
  int n(instance->getN()); // number of customers
  int m(instance->getM()); // number of potential facilities

  if(s->getP()>=k)
    {
      // maybe we're done already
      return;   
    }

  //---
  // check the list of candidates
  //---

  bool list_provided=(initlist!=0);
  if(!list_provided) 
    {
      candlist.reserve(m);
      for(i=s->getP()+1;i<=m;i++) 
	{
	  if(facilityLocationConstraints[i]!=2)
	    {
	      candlist.push_back(s->getFacility(i));
	    }
	}
    }
  else 
    {
      candlist.reserve(initlist[0]);
      for(i=1;i<=initlist[0];i++)
	{
	  candlist.push_back(initlist[i]);
	}
    }

  int ncand = int(candlist.size()-1);

      // If we're only selecting one facility, deterministically pick the best one.
      // That is the optimal solution.  There should be nothing selected or we
  // would have returned in the test above (if (s->getP() >= k) return)
  if (k == 1)
    {
      // keep 1-based
      double *Cost = new double[ncand + 1];
      for (int j = 1; j <= ncand; j++)
	Cost[j] = 0;
      for (int i = 1; i <= n; i++)
	for (int j=1; j <= ncand; j++)
	  Cost[j] += instance->getDist(i,candlist[j]);

      double lowestCost = Cost[1];
      int bestf = candlist[1];
      for (int j = 2; j <= ncand; j++)
	{
	  if (Cost[j] < lowestCost)
	    {
	      lowestCost = Cost[j];
	      bestf = candlist[j];
	    }
	}
      s->add(bestf, true);
      if(verbose==1)
	{
	  std::cerr << "Greedy algorithm executed successfully." << std::endl << std::endl;
	}
      delete [] Cost;
      return;
    }

  //---
  // determine closeness information
  //---
  double *save = new double [m+1]; //save[i] = how much we save if we add i to the solution
  savelist = new double [m+1];

  save[0] = -EPSILON; //doing nothing saves us zero (essentially)
  for (i=m; i>0; i--) save[i] = 0; //WARNING: assumes cost is zero
  s->ensureConsistency(); //make sure closeness information makes sense

  //---
  // calculate 'save' for all facilities [O(nm) time]
  //---
  for (i=n; i>0; i--) { //for every customer i
    double d = s->getDistClosest(i); //distance to the closest facility
    for (int j=s->getP()+1; j<=m; j++) { //consider all facilities outside the solution
      f = s->getFacility(j);
      double profit = d - instance->getDist(i,f);
      if (profit>0) save[f] += profit;
    }
  }

  if(verbose==1)
    {
      std::cout << "Initial amounts computed." << std::endl;
    }

  //----------------------------------
  // we now add facilities one by one
  //----------------------------------
  while(s->getP()<k) 
    {
      if(verbose==1)
	{ 
	  std::cout << "Adding facility " << s->getP()+1 << std::endl;
	}

      //---
      // determine best facility to add
      //---
      int bestj, bestf;
	
      //---
      // proportional method: facilities that save
      // the most have higher probabilitis
      //---
      for(int j=1; j<=ncand; j++) 
	{
	  //list of candidates
	  savelist[j]=save[candlist[j]];
	} 

      
		
      if(proportional) 
	{
	  double factor(alpha);
	  if(factor==0)
	    {
	      factor = 2 * BossaRandom::getDouble();
	    }
	  double min = RFWStats::min (savelist, 1, ncand);
	  double max = RFWStats::max (savelist, 1, ncand);
	  if (min == max) 
	    {
	      bestj = BossaRandom::getInteger (1, ncand);
	    }
	  else 
	    {
	      double x = BossaRandom::getDouble();
	      for(int j=1; j<=ncand; j++) 
		{
		  savelist[j]-=min;
		}
	      bestj = RFW::getWeightedIndex (savelist, 1, ncand, x, factor);
	    }
	} 
		
      //---
      // standard method: select at random among the best candidates (fraction alpha)
      //---
      else 
	{
	  if(random_alpha)
	    {
	      alpha=BossaRandom::getDouble();
	    }
	  int c = (int) ceil (alpha*int(candlist.size()-1));                  //number of candidates to consider
	  RFW::bound (c, 1, int(candlist.size()-1));                          //avoiding rounding errors
	  bestj = RFW::randomLargest(savelist, 1, int(candlist.size()-1), c); //get one of those c elements
	}
      bestf = candlist[bestj];

      // remove best from the list
      candlist[bestj] = candlist.back();
      candlist.pop_back();

      // finally, add facility
      s->add (bestf, true); //automatically updates closeness info

      //----------------------------------------------------------------------
      // make all vertices 'change hands' for the next iteration
      // (if there is a next iteration); runs in O(mn) time in the worst case
      //----------------------------------------------------------------------
      if (s->getP() < k) 
	{
	  for (int i=1; i<=n; i++) 
	    {
	      if (s->getClosest(i)==bestf) 
		{         
		  double d1 = s->getDistClosest(i);  //current closest
		  double d2 = s->getDistClosest2(i); //previous closest (has to be)
		  for (int j=s->getP()+1; j<=m; j++) 
		    { 
		      //update 'save' for facilities not in solution
		      int f = s->getFacility(j);
		      double dif = instance->getDist(i,f);
		      if (dif<d2) 
			{
			  save[f] -= (d2-dif);             //we won't save w.r.t. previous closest
			  if (dif<d1) save[f] += (d1-dif); //but we may still save w.r.t. current closest
			}
		    }
		}
	    }
	}
    }

  if(verbose==1)
    {
      std::cout << "All iterations executed." << std::endl;
    }

  //----------
  // clean up
  //----------
  delete [] save;
  if (savelist!=0) delete [] savelist;

  if(verbose==1)
    {
      std::cerr << "Greedy algorithm executed successfully." << std::endl << std::endl;
    }
}


/*****************************************************************
 *
 * Proportional worst: 
 * - starts with a random facility
 * - in each further iteration, chooses customer with probability
 *   proportional to how much thed'd save if chosen; adds closest
 *   facility to customer
 *
 * s (output): solution (input/output);
 * candlist (input): list of allowed facilities (if 0, all)
 *                   (candlist[0] must be the length of the list)
 * stats (input): print statistics?
 *
 *****************************************************************/

void PMConstructive::pworst (PMSolution *s, int k, int *candlist, double exponent, bool stats) 
{
  checkFixed (s, "pworst");
	
  if (k<=0 || k>s->getM()) fatal ("pworst", "number of facilities to insert out of range");

  BossaTimer st (true);

  int bestf, f, i, j;
  PMInstance *inst = s->getInstance();
  //int p = inst->getP();
  int n = inst->getN();
  int m = inst->getM();

  //if (p==0) fatal ("pworst", "number of facilities cannot be zero");

  double *w  = new double [n+1]; //weights used to compute probabilities
  double *dc = new double [n+1]; //distance to closest candidate

  bool list_given = (candlist!=0); //list of candidates may be given as input
  if (!list_given) {                  //if it isn't, assume every facility is a candidate
    candlist = new int [m+1];  
    candlist[0] = m;
    for (i=1; i<=m; i++) candlist[i] = i;
  }

  if (candlist[0]<k) fatal ("pworst", "not enough candidates");
	
  // find distance from each user to the closest facility
  // THIS IS NOT ALWAYS RELEVANT --- IN MOST CASES, IT'S JUST ZERO
  if (n==m) {
    for (i=1; i<=n; i++) dc[i] = inst->getDist(i,i); //just an acceleration
  } else {
    for (i=1; i<=n; i++) dc[i] = POPSTAR_INFINITY; //this is the general case
  }

  for (i=1; i<=n; i++) 
    {
      if (dc[i]==0) continue;
      for (j=1; j<=candlist[0]; j++) 
	{
	  f = candlist[j];
	  double d = inst->getDist(i,f); //WARNING: THIS CAN BE GREATLY IMPROVED
	  if (d<dc[i]) dc[i] = d;
	}
    }

  s->reset(); //start with an empty solution
  bestf = candlist[BossaRandom::getInteger(1,candlist[0])];
  s->add (bestf, true); //add a random facility
	
  //--------------
  // initialize w
  //--------------
  for (i=1; i<=n; i++) {
    w[i] = inst->getDist(i,bestf) - dc[i]; //WARNING: RANDOM ACCESS
  }

  if (exponent == 0) exponent = 2 * BossaRandom::getDouble();
  std::cout << "<" << std::setprecision(2) << exponent << "> ";

  //----------------------------------
  // main loop: add facilities
  //----------------------------------
  while (s->getP() < k) {
    //select a user at random with relative probabilities given by w
    int u = RFW::getWeightedIndex (w, 1, n, BossaRandom::getDouble(), exponent);

    //find closest facility not in the solution
    bestf = 0;
    for (j=1; j<=candlist[0]; j++) {
      f = candlist[j];
      if (s->contains(f)) continue;
      if (inst->getDist(u,f) < inst->getDist(u,bestf)) bestf = f; //WARNING: CAN BE ACCELERATED
    }
		
    //add facility to the solution
    s->add(bestf, true);

    //update weights
    for (u=1; u<=n; u++) {
      w[u] = inst->getDist(u,s->getClosest(u)) - dc[u]; //WARNING: COULD BE ACCELERATED
    }
  }

  delete [] w;
  delete [] dc;
  if (!list_given) delete [] candlist;

  if (stats) 
    {
      double result = s->cost();
      std::cout << "srandom " << std::setprecision(2) << result << std::endl;
      std::cout << "srandomtime " << std::setprecision(3) << st.getTime() << std::endl;
    }
}


/*----------------------------------------------------
  | proportional reverse: starts with a full solution,
  | removes facilities one by one until only k are left. 
  | Probability of removal grows when loss decreases
  *---------------------------------------------------*/
 
void PMConstructive::preverse(PMSolution *s, int k, bool stats) 
{
  // checkFixed (s, "preverse");
	
  if(k<=0||k>s->getM())
    { 
      fatal("preverse","number of facilities to insert out of range");
    }

  BossaTimer t (true);

  PMInstance *inst = s->getInstance();
  //int p = inst->getP(); //number of facilities that must remain open
  int n = inst->getN(); //total number of users
  int m = inst->getM();
  int i, j, f;
  double *w    = new double [m+1];
  double *loss = new double [m+1];

  //--------------------------------
  // add all facilities to solution
  //--------------------------------
  for (f=1; f<=m; f++) {s->add(f,false);}
  s->ensureConsistency();

  //----------------------------------
  // now remove facilities one by one
  //----------------------------------
  while (s->getP() > k) 
    {
      int cp = s->getP();

      for (f=1; f<=m; f++) loss[f] = 0;
      for (i=1; i<=n; i++) 
	{
	  int c1 = s->getClosest(i);
	  int c2 = s->getClosest2(i);
	  double diff = inst->getDist(i,c2) - inst->getDist(i,c1);
	  loss[s->getPosition(c1)] += diff;
	}

      double max = RFWStats::max (loss, 1, cp); //largest loss
      double min = RFWStats::min (loss, 1, cp); //smallest lost

      int best;
      if (max == min) { //same weight for all -> pick a random facility
	best = BossaRandom::getInteger (1, cp);
      } else { //else pick with weighted probabilities
	for (j=1; j<=cp; j++) {w[j] = max - loss[j];}
	best = RFW::getWeightedIndex (w, 1, cp, BossaRandom::getDouble());
      }
      int bestf = s->getFacility(best);

      s->remove(bestf, true);
    }

  s->ensureConsistency();
	
  if(stats) 
    {
      std::cout << "preverse " << std::setprecision(2) << s->cost() << std::endl;
      std::cout << "preversetime " << std::setprecision(3) << t.getTime() << std::endl;
    }
	
  delete [] loss;
  delete [] w;
}


/*----------------------------------------------------------------
  | sample: greedy, but sampling only a fraction of the candidates
  | in each iteration.
  *---------------------------------------------------------------*/

void PMConstructive::sample(PMSolution *s, 
			    int k, 
			    double fraction, 
			    bool stats,
			    const std::vector<int> &facilityLocationConstraints) 
{
  // checkFixed (s, "sample");

  if((k<=0)||(k>s->getM()))
    { 
      fatal("sample","number of facilities to insert out of range");
    }

  BossaTimer t;
  t.start();
  int m, n, verbose, i;
	
  PMInstance *inst = s->getInstance();	
  verbose = 0;
  n = inst->getN(); // number of customers
  m = inst->getM(); // number of potential facilities

  int sample_size;
  if(fraction<0) fraction = BossaRandom::getDouble();
  if(fraction>1) 
    {
      sample_size = (int)ceil ((log ((double)m/(double)k))/LN2);
    } 
  else 
    {
      sample_size = (int)ceil (fraction * (double)m);
    }
  if (sample_size <= 0) sample_size = 1;
  if (verbose > 1) {fprintf (stderr, "Fraction:%f, absolute %d.\n", fraction, sample_size);}

  //--------------------------
  // build list of candidates
  //--------------------------
  std::vector<int> candlist; // the 0th entry is a dummy entry
  candlist.push_back(0);
  candlist.reserve(m);

  for(i=s->getP()+1; i<=m; i++) 
    {
      if(facilityLocationConstraints[s->getFacility(i)]==0)
	{
	  // it's not a candidate if it's fixed open or fixed closed
	  candlist.push_back(s->getFacility(i));
	}
    }

  //---------------------------------
  // Running time: O(p m fraction n) <---- WHY? THIS DOESN'T MAKE SENSE
  //---------------------------------
  int count = 0;
  while (s->getP() < k) 
    {
      // O(pqn)
      count++;
      if (sample_size > (candlist.size()-1)) sample_size = (candlist.size()-1);

      //RFW::shuffle(candlist, candlist[0]);
      partialShuffle (&(candlist[0]), 1, candlist.size()-1, sample_size);

      //find best option in the sample
      int bestc = -1; /* WARNING, THIS HAS CHANGED */
      if(sample_size==1)
	{
	  bestc = 1;
	}
      else 
	{
	  double bestsave = -1;
	  for (int c=1; c<=sample_size; c++) 
	    { 
	      //O(qn)
	      int f = candlist[c];

	      //how much do we save by adding f?
	      double save = 0; //s->calcSave (f); //0;
	      for (int i=1; i<=n; i++) 
		{ 
		  //O(n)
		  double diff = s->getDistClosest(i) - inst->getDist(i,f); //WARNING: THIS COULD BE ACCELERATED
		  if(diff > 0)
		    {
		      save += diff;	
		    }
		}	
	      if (save > bestsave) 
		{
		  bestsave = save; 
		  bestc = c;
		}
	    }
	}
      int bestf = candlist[bestc];
      s->add(bestf, true);        //O(n)

      // remove facility from list of candidates
      candlist[bestc] = candlist.back();
      candlist.pop_back();
    }

  s->ensureConsistency();
		
  if (stats) 
    {
      std::cout << "sample " << std::setprecision(2) << s->cost() << std::endl;
      std::cout << "samplef " << std::setprecision(3) << fraction << std::endl;
      std::cout << "sampletime " << std::setprecision(3) << t.getTime() << std::endl;
    }
  if(verbose>0) 
    {
      std::cout << "(" << std::setprecision(2) << t.getTime() << ")";
    }
}

void PMConstructive::pcenter(PMSolution *s, 
			     int k,
			     const std::vector<int> &facilityLocationConstraints)
{
  int p(s->getP());
  int m(s->getM());

  // if the number of facilities opened already 
  // exceeds k, then bail - you're done.
  if(p>=k)
    {
      return;
    }

  // open the next (usually the first) facility at 
  // random; it's random primarily because we aren't 
  // using facility weights.
  bool validFound(false);
  int newFacility(-1);
  while(validFound==false)
    {
      newFacility=s->getFacility(BossaRandom::getInteger(p+1,m));
      if(facilityLocationConstraints[newFacility]!=2)
	{
	  validFound=true;
	}
    }
  s->add(newFacility,true);
  p++;

  // while we haven't reached the target number of facilities, keep going.
  while(p<k)
    {
      // find the maximum distance to any facility in the existing 
      // solution, in addition to the corresponding vertices.
      double bestEvaluation(DBL_MAX);
      int bestEvaluationCount(INT_MAX);
      std::vector<int> bestEvaluationFacilities;
      for(int loc(p+1);loc<=m;loc++)
	{
	  int facilityToOpen(s->getFacility(loc));
	  if(facilityLocationConstraints[facilityToOpen]!=2)
	    {
	      s->add(facilityToOpen,true);
	      double thisEvaluation(s->worstCaseCost());
	      int thisEvaluationCount(s->numWorstCaseScenarios());
	      if(thisEvaluation<bestEvaluation)
		{
		  bestEvaluationFacilities.clear();
		  bestEvaluationFacilities.push_back(facilityToOpen);
		  bestEvaluation=thisEvaluation;
		  bestEvaluationCount=thisEvaluationCount;
		}
	      else if(thisEvaluation==bestEvaluation)
		{
		  if(thisEvaluationCount<bestEvaluationCount)
		    {
		      bestEvaluationFacilities.clear();
		      bestEvaluationFacilities.push_back(facilityToOpen);
		      bestEvaluationCount=thisEvaluationCount;
		    }
		  else if(thisEvaluationCount==bestEvaluationCount)
		    {
		      bestEvaluationFacilities.push_back(facilityToOpen);
		    }
		}
	      s->remove(facilityToOpen,true);
	    }
	}

      int idx=BossaRandom::getInteger(0,bestEvaluationFacilities.size()-1);
      newFacility=bestEvaluationFacilities[idx];
      s->add(newFacility,true);

      p++;
    }
}

/*---------------------------------------------------------------------
  | make sure the instance requires a solution of fixed size (p-median)
  *--------------------------------------------------------------------*/

void PMConstructive::checkFixed(PMSolution *s, const char *method) 
{
  if (!s->getInstance()->fixedP()) 
    {
      fprintf (stderr, "ERROR: method \"%s\" can only be applied to p-median instances.\n", method);
      exit(1);
    }
}

/*---------------------------------------------------------------------
  | make sure the instance does not require a solution of fixed size
  | (facility location)
  *--------------------------------------------------------------------*/

void PMConstructive::checkNotFixed(PMSolution *s, const char *method) 
{
  if (s->getInstance()->fixedP()) 
    {
      fprintf (stderr, "ERROR: method \"%s\" can only be applied to facility location instances.\n", method);
      exit(1);
    }
}
