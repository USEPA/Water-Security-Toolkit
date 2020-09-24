// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/****************************************************************
 *
 * program: grasp
 *          provides heuristic solutions to the pmedian problem
 * author: Renato Werneck (rwerneck@princeton.edu)
 * log: May 29, 2002 - file created
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "basics.h"
#include "bossa_timer.h"
#include "config.h"
#include "elite.h"
#include "instance.h"
#include "instance_factory.h"
#include "search.h"
#include "solution.h"
#include "bossa_random.h"
#include "bossa_heap.h"
#include "rfw_stats.h"
#include "search_tables.h"
#include "constructive.h"
#include "distance.h"
#include "path_relink.h"
#include "recorder.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <list>
#include <vector>
#include <algorithm>

const int GRASP_VERSION=20051101;

class Grasp 
{
private:

  void fatal(const char *func, const char *msg);

public:

  double grasp(PMSolution *s,   //output: will hold the best solution found
	       const std::vector<int> &facilityLocationConstraints,
	       PMConstructive *constructive, //input: used to run the appropriate constructive heuristic
	       int it,          //input: number of iterations to be performed
	       PMElite *elite,  //input/output: will hold best solutions found and be used for path-relinking (can be 0)
    	       char *lsmethod,   //local search method to be used
	       char *recombinationMethod,
    	       bool stats,      //input: print statistics to std::cout?
  	       bool partials);    //input: print partial information to std::cout?

  void showUsage(char *argv[], bool full=true);
  void testConstructive (PMInstance *inst, PMConstructive *tc, PMConfig *config);
  int run(int argc, char *argv[]);
  void testLocalSearch (PMInstance *inst, PMConstructive *constr, char *method, double mt, bool output_density, bool profile);
  void testPathRelinking (PMInstance *inst, PMConstructive *c, PMElite *elite, const char *method);
};

void Grasp::fatal(const char *func, const char *msg) 
{
  std::cout << func << ": " << msg << std::endl;
  exit(-1);
}

/*--------------------------------------------------------------------
  |
  | GRASP: executes a GRASP with n interations and path-relinking (if 
  |   'nelite' is greater than 0). Uses fastInterchange if 'fi' is 
  |   true, otherwise uses localSearch. The best solution found after
  |   path-relinking is returned in s.
  |
  *--------------------------------------------------------------------*/
 
double Grasp::grasp(PMSolution *s,                    // input: the problem being solved.
		    const std::vector<int> &facilityLocationConstraints,
		    PMConstructive *constructive,     // input: used to run the appropriate constructive heuristic
		    int numberOfIterations,           // input: number of iterations to be performed
		    PMElite *elitePool,               // input/output: will hold best solutions found and be used for path-relinking (can be 0)
		    char *localSearchMethod,          // local search method to be used
		    char *recombinationMethod,        // path-relinking combination mathod to be used
		    bool outputStatistics,            // input: print statistics to std::cout?
		    bool partials                     // input: print partial information to std::cout?
		    ) 
{
  bool verbose(outputStatistics?true:false);

  PMRecorder recorder;

  BossaTimer overallTimer(true);             // overall
  BossaTimer localSearchTimer(false);        // local search
  BossaTimer constructiveSearchTimer(false); // constructive
  BossaTimer recombinationTimer(false);      // combination time

  PMInstance *instance=s->getInstance();
  PMElite *elite_best=new PMElite(instance,1);  // this will store just the best solution
  PMSolution *r=new PMSolution(instance);
  PMSolution *rs=new PMSolution(instance);      // relink solution
	
  double total_p(0.0); // total number of facilities is all local optima

  if(verbose==true)
    { 
      std::cout << "Running grasp..." << std::endl;
    }

  if(outputStatistics==true)
    {
      std::cout << std::endl;
      std::cout << "Constructive method=";
      constructive->printMethod(std::cout);
      std::cout << std::endl;
      std::cout << "Local search method=" << localSearchMethod << std::endl;
      if(elitePool!=0)
	{
	  std::cout << "Recombination method=" << recombinationMethod << std::endl;
	}
      std::cout << std::endl;
    }

  for(int i=1;i<=numberOfIterations;i++) 
    {
      // build a solution from scratch via GRASP

      constructiveSearchTimer.resume();
      s->reset();

      int p;
      if(instance->fixedP())
	{
	  p=instance->getP();
	}
      else 
	{
	  if (total_p == 0) 
	    {
	      p=(instance->getM()+1)/2;
	    } 
	  else 
	    {
	      p=(int)((double) (total_p) / (double) (i - 1));
	    }
	}

      constructive->run(s,p,facilityLocationConstraints);
      double original(s->cost());
      constructiveSearchTimer.pause();	

      ////////////////////////////////////////////////
      // refine the GRASP solution via local search //
      ////////////////////////////////////////////////

      localSearchTimer.resume();

      PMSearch::runLocalSearch(s,localSearchMethod,facilityLocationConstraints);
      double result(s->cost());
      localSearchTimer.pause();

      if(verbose==true)
	{
	  std::cout.width(5);
	  std::cout << i;
	  std::cout.width(0);
	  if(elite_best->getSolutionCost(1)==POPSTAR_INFINITY)
	    {
	      std::cout << std::fixed << std::setprecision(2) << " [UNDEFINED] " << original << " -> " << result;	      
	    }
	  else
	    {
	      std::cout << std::fixed << std::setprecision(2) << " [" << elite_best->getSolutionCost(1) << "] " << original << " -> " << result;
	    }
	}

      unsigned int seed(BossaRandom::getNonZero());

      total_p+=s->getP();

      if(elitePool!=0) 
	{
	  recombinationTimer.resume();
	  int d = elitePool->countConsistent();
	  bool insert = true;

	  //if there are elite solutions, relink
	  if(d>=1) 
	    { 
	      int e = elitePool->getRandomDifferent (s, 1, d); //a probably different solution
				
	      if (e!=0) { //solution is not identical to some solution in the pool
		double elite_cost = elitePool->getSolution (e, r); //r is the solution
		double oldcost = (elite_cost < result) ? elite_cost : result; //get best of two original

		if (!r->equals(*s)) 
		  { 
		    // if they are not equal, try to relink them
		    //--------------------------------
		    // combine both solutions into rs
		    //--------------------------------
		    double newcost = PMRelink::combine(rs, facilityLocationConstraints, s, r, recombinationMethod); //s is the one produced by local search, r is the elite one
		    if (verbose==true)
		      {
			std::cout << std::setprecision(2) << " x " << elite_cost << " -> " << newcost;
		      }
		    if(rs->computeDifference(*r)>1 && rs->computeDifference(*s)>1) 
		      {
			PMSearch::localSearch(rs,true,false,facilityLocationConstraints); //try to improve by local search if necssary
			newcost = rs->cost();
			if(verbose==true)
			  { 
			    std::cout << " -> " << std::setprecision(2) << newcost;
			  }
		      }

		    //-----------------------------------------------------------
		    // if there is an improvement, try to save 'hybrid' solution
		    //-----------------------------------------------------------
		    if (newcost < oldcost - EPSILON) 
		      {
			elitePool->add (rs, newcost);      //add to the set of elite solutions
			elite_best->add (rs, newcost); //save it in the local list of elite solutions
			if (recorder.report (i, overallTimer.getTime(), newcost)) 
			  {
			    if(partials)
			      { 
				std::cout << "partial " << std::setprecision(3) << recorder.best_time << " " << recorder.best_value << std::endl;
			      }
			  }
		      }
		  }
	      } 
	      else 
		{
		  insert=false;
		}
	    }
	  if (insert) elitePool->add (s, result);
	  recombinationTimer.pause();
	}

      if(verbose==true) 
	{
	  if(elite_best->getSolutionCost(1)==POPSTAR_INFINITY)
	    {
	      std::cout << "  " << std::setprecision(2) << " MRE=UNDEFINED ";
	    }
	  else
	    {
	      double mre=((result-elite_best->getSolutionCost(1))/elite_best->getSolutionCost(1))*100.0;
	      std::cout << "  " << std::setprecision(2) << " MRE=" << mre << "% ";
	    }

	  std::list<int> open;
	  for(int k=1;k<=p;k++) 
	    {
	      open.push_back(s->getFacility(k));
	    }
	  open.sort();

	  std::cout << "Solution=[";
	  for(std::list<int>::const_iterator iter=open.begin();iter!=open.end();iter++) 
	    {
	      std::cout << (*iter) << " ";
	    }
	  std::cout << "] ";
	}

      //------------------------
      // save original solution
      //------------------------
      elite_best->add(s, result); 
      if(recorder.report(i,overallTimer.getTime(),result)) 
	{
	  if(partials)
	    {
	      std::cout << "partial " << std::setprecision(3) << recorder.best_time << " " << recorder.best_value << std::endl;
	    }
	}
		
      if(verbose==true) 
	{
	  if(recorder.best_iteration==i)
	    { 
	      std::cout << " <<<";
	    }
	  std::cout << std::endl;
	}
      BossaRandom::randomize(seed);		
    }
	
  //----------------------------------------------------
  // copy best solution to s (and its value to solcost)
  //----------------------------------------------------
  double solcost(elite_best->getSolution(1, s));
	
  //-----------------------
  // deallocate structures
  //-----------------------
  delete elite_best;
  delete r;
  delete rs;

  ///////////////////////////////
  // output summary statistics //
  ///////////////////////////////

  if(outputStatistics) 
    {
      std::cout << std::endl;
      std::cout << "Best objective value=" << std::setprecision(2) << solcost << std::endl;
      std::cout << std::endl;
      std::cout << "Total number of iterations=" << numberOfIterations << std::endl;
      std::cout << "Iteration for best solution=" << recorder.best_iteration << std::endl;
      std::cout << std::endl;
      std::cout << "Average per-iteration time for constructive search=" << std::setprecision(2) << (constructiveSearchTimer.getTime()/(double)numberOfIterations) << std::endl;
      std::cout << "Average per-iteration time for local search=" << std::setprecision(2) << (localSearchTimer.getTime()/(double)numberOfIterations) << std::endl;
      std::cout << "Total time for constructive search=" << std::setprecision(2) << constructiveSearchTimer.getTime() << std::endl;
      std::cout << "Total time for local search=" << std::setprecision(2) << localSearchTimer.getTime() << std::endl;
      std::cout << "Total time to best solution=" << std::setprecision(2) << recorder.best_time << std::endl;
      std::cout << "Total time=" << overallTimer.getTime() << std::endl;
    }

  return solcost;
}

void Grasp::showUsage(char *argv[],bool full) 
{
  std::cout << std::endl << "Usage: " << argv[0] << " <file> [options]" << std::endl;
	
  if(full==true)
    {
      std::cout << std::endl << "Main options:" << std::endl;
      std::cout << "    -graspit <n>           : number of grasp iterations [default=32]" << std::endl;
      std::cout << "    -elite <x>             : size of the pool of elite solutions [default=10]" << std::endl;
      std::cout << "    -p <x>                 : sets number of facilities (p); overrides value in the input file (if any)" << std::endl;
      std::cout << "    -rp <x>                : same as p, but as a fraction of m (0<p<=1)" << std::endl;
      std::cout << "    -output <x>            : output solution to file 'x'" << std::endl;
      std::cout << "    -outputFacilities <x>  : output facility locations to file 'x'" << std::endl;

      std::cout << std::endl << "Additional options:" << std::endl;
      std::cout << "    -greedy      : execute greedy algorithm" << std::endl;
      std::cout << "    -grasp       : execute grasp" << std::endl;
      std::cout << "    -ls <method> : use <method> as the local search method (default is s:-1)" << std::endl;
      std::cout << "    -ch <method> : use <method> as the constructive heuristic (default is sample:1)" << std::endl;
      std::cout << "    -cf <x>      : cache factor (will be x times m/p) (negative -> full size)" << std::endl;
      std::cout << "    -seed <x>    : use <x> as the seed for the random number generator" << std::endl;
    }

  std::cout << std::endl;
  exit(-1);
}

/*------------------------------
  | test constructive heuristics
  *-----------------------------*/

void Grasp::testConstructive(PMInstance *inst, PMConstructive *tc, PMConfig *config) 
{
	
  int i,j,k,p;
  k = config->tc; //number or iterations
  double *v, *lit;

  p = inst->getP();

  PMSolution **s;
	
  s = new PMSolution* [k+1];
  v = new double [k+1];
  lit = new double [k+1];
  for (i=1; i<=k; i++) s[i] = new PMSolution (inst);

  std::cout << "tcmethod ";
  tc->printMethod(std::cout);
  std::cout << std::endl;

  std::vector<int> facilityLocationConstraints(inst->getM()+1,0);

  bool run_ls = true;

  BossaTimer t;
  t.start();
  for (i=1; i<=k; i++) 
    {
      BossaRandom::randomize(config->seed - 1 + i);
		
      int p;
      if (inst->fixedP()) {p = inst->getP();}
      else {p = (inst->getM()+1)/2;}
      tc->run(s[i], p,facilityLocationConstraints);

      lit[i] = 0.0;
      if (run_ls) {
	int it;
	PMSearch::localSearch(s[i],true,false,facilityLocationConstraints,&it);
	lit[i] = (double)it;
      }
      v[i] = s[i]->cost();
    }

  std::cout << "tctime " << std::setprecision(2) << t.getTime() / (double)k << std::endl;
  std::cout << "tccount " << std::setprecision(2) << (double)k << std::endl;
  std::cout << "tcmin " << std::setprecision(2) << RFWStats::min(v,1,k) << std::endl;
  std::cout << "tcavg " << std::setprecision(2) << RFWStats::average(v,1,k) << std::endl;
  std::cout << "tcstddev " << std::setprecision(2) << RFWStats::stddev(v,1,k) << std::endl;
  std::cout << "tcmed " << std::setprecision(2) << RFWStats::median(v,1,k) << std::endl;
  std::cout << "tcmax " << std::setprecision(2) << RFWStats::max(v,1,k) << std::endl;

  std::cout << "tcminit " << std::setprecision(2) << RFWStats::min(lit,1,k) << std::endl;
  std::cout << "tcavgit " << std::setprecision(2) << RFWStats::average(lit,1,k) << std::endl;
  std::cout << "tcstddevit " << std::setprecision(2) << RFWStats::stddev(lit,1,k) << std::endl;
  std::cout << "tcmedit " << std::setprecision(2) << RFWStats::median(lit,1,k) << std::endl;
  std::cout << "tcmaxit " << std::setprecision(2) << RFWStats::max(lit,1,k) << std::endl;

  int npairs = k * (k-1) / 2;
  double *diff = new double [npairs+1];

  int c = 0;
  for (i=1; i<k; i++) {
    for (j=i+1; j<=k; j++) {
      c++;
      diff[c] = (double) (s[i]->computeDifference(*(s[j])));
    }
  }

  std::cout << "tcmindiff " << std::setprecision(3) << RFWStats::min(diff,1,npairs) / (double)p << std::endl;
  std::cout << "tcavgdiff " << std::setprecision(3) << RFWStats::average(diff,1,npairs)/ (double)p << std::endl;
  std::cout << "tcmeddiff " << std::setprecision(3) << RFWStats::median(diff,1,npairs)/ (double)p  << std::endl;
  std::cout << "tcmaxdiff " << std::setprecision(3) << RFWStats::max(diff,1,npairs)/ (double)p << std::endl;
	
  for (i=1; i<=k; i++) delete s[i];
  delete [] s;
  delete [] v;
}


/*---------------------
  | Test path-relinking
  *--------------------*/
void Grasp::testPathRelinking(PMInstance *inst, PMConstructive *c, PMElite *elite, const char *method) 
{
  if (elite==0) fatal ("testPathRelinking", "number of elite solutions cannot be zero");
  std::cout << "Resetting list of elite solutions..." << std::endl;
  PMElite *single_elite = new PMElite (inst, 1);
	
  elite->reset();
  std::cout << "Creating new solution..." << std::endl;
  PMSolution *s1 = new PMSolution (inst);
  PMSolution *s2 = new PMSolution (inst);
  PMSolution *s  = new PMSolution (inst);
  PMSearchPathRelinking *sm = new PMSearchPathRelinking (inst);

  int i, j, t;
  int e = elite->getCapacity();
  int maxtries = 2 * e;

  //----------------------------------
  // fill the pool of elite solutions
  //----------------------------------
  std::cout << "Entering main loop..." << std::endl;
  std::vector<int> facilityLocationConstraints(inst->getM()+1,0);
  for (t=1; t<=maxtries; t++) 
    {
      std::cout << t;

      s1->reset();   //start with empty solution
      c->run(s1,0,facilityLocationConstraints);  //constructive algorithm on top of it
      PMSearch::localSearch(s1,true,false,facilityLocationConstraints,false,false); //local search
      elite->add(s1, s1->cost()); //add to list
      std::cout << t << "/" << elite->countConsistent() << std::endl;
      if (elite->countConsistent()>=e) break;
    }

  enum {DOWN, HOR, UP};

  int m = inst->getM(); //number of candidate facilities
	
	
  double *imp  [3];
  int    *count[3]; 
  double *sum  [3];

  for (i=0; i<3; i++) {
    count[i] = new int [m+1];
    imp  [i] = new double [m+1];
    sum  [i] = new double [m+1];
    for (j=0; j<=m; j++) {
      count[i][j] = 0;
      imp  [i][j] = 0;
      sum  [i][j] = 0;
    }
  }

  int ns = elite->countConsistent();

  int eliteplus = 10;

  int bestuniform = 0; //number of times uniform won
  int bestbiased  = 0; //number of times biased won
  int ties        = 0; //number of times there was a tie
  double sumuniform = 0;
  double sumbiased  = 0;

  for (int i2=eliteplus+1; i2<=ns; i2++) {
    int totalcount = 0;
    int rightcount = 0;
    int totaldiff = 0;
    int rightdiff = 0;
    for (int i1=1; i1<=eliteplus; i1++) {

      if (i1==i2) continue;
      double v1 = elite->getSolution (i1, s1);
      double v2 = elite->getSolution (i2, s2);
      int diff = s1->computeDifference(*s2);

      int dir = HOR;
      if (v1<v2) dir = UP;
      else if (v1 > v2) dir = DOWN;

      PMRelink::combine(s, facilityLocationConstraints, s1, s2, method);

      /*
	sm->setSolutions (s1, s2);           //define the extremes of the path
	single_elite->reset();               //the best solution in the path will be here
	s->copyFrom (s1);              
	search (s, true, sm, single_elite, true);  //excecute path-relinking
      */
      //double vsol = single_elite->getSolution (1, s);   //copy solution to s and get value
      double vsol = s->cost();
      if (s->computeDifference(*s1)>1 && s->computeDifference(*s2)>1) 
	{
	  PMSearch::localSearch(s,true,false,facilityLocationConstraints);  //try to improve solution with local search
	  vsol = s->cost();   //get the value
	}

      //calculate profit
      double profit;
      if (v1<v2) profit = v1 - vsol;
      else profit = v2 - vsol;
      if (profit<0) profit = 0;

      //bool success = (profit > 0);

      std::cout << "[" << i1 << ":" << i2 << "] " << v1 << " x " << v2 << "(" << dir << "," << diff << ") -> " << vsol << "(" << profit << ")" << std::endl;

      count[dir][diff] ++;
      imp  [dir][diff] += profit;
      sum  [dir][diff] += v1+v2;

      totalcount ++;
      totaldiff  += diff;

      if (profit) {
	rightcount ++;
	rightdiff += diff;
      }

    }
    double probcount = (double)rightcount / (double)totalcount;
    double probdiff  = (double)rightdiff  / (double)totaldiff;

    double ratio;
    if (probcount>0) ratio = probdiff / probcount;
    else ratio = 1;

    std::cout << "tpd " << i2 << probdiff << probcount << ratio << std::endl;
    std::cout << "tpd " << i2 << probdiff << probcount << ratio << std::endl;

    double mindiff = 0.001;
    if (ratio > 1 + mindiff) 
      {
	bestbiased ++;	
      } 
    else if (ratio < 1 - mindiff) 
      {
	bestuniform ++;
      } 
    else 
      {
	ties ++;
      }

    sumbiased += probdiff;
    sumuniform += probcount;
  }

  {
    int x = bestbiased + bestuniform + ties;
    std::cout << "trt " << (double)ties/(double)x << std::endl;
    std::cout << "tru " << (double)bestuniform/(double)x << std::endl;
    std::cout << "trb " << (double)bestbiased/(double)x << std::endl;
    std::cout << "trpu " << (double)sumuniform/(double)x << std::endl;
    std::cout << "trpb " << (double)sumbiased/(double)x << std::endl;
    std::cout << "trpr " << (double)sumbiased/(double)sumuniform << std::endl;
  }

  for(i=0; i<3; i++) 
    {
      for(j=0; j<=m; j++) 
	{
	  if (count[i][j]==0) continue;
	  std::cout << "tpr " << i << " " << j << " " << count[i][j] << " " << imp[i][j] << std::endl;
	}
    }

  for(j=0; j<=m; j++) 
    {
      int c = count[0][j] + count[1][j] + count[2][j];
      double v = imp[0][j] + imp[1][j] + imp[2][j];
      double a = (sum[0][j] + sum[1][j] + sum[2][j]) / (2.0 * (double)c);
      double im = v / (double)c;
      if (c>0) 
	{
	  std::cout << "tpa " << j << " " << c << " " << a << " " << im << " " << a-im << std::endl;
	}
    }

  delete single_elite;
  delete sm;
  delete s1;
  delete s2;
  delete s;
  for(i=0; i<3; i++) 
    {
      delete [] count[i];
      delete [] imp[i];
      delete [] sum[i];
    }
}


/*---------------------------------------------------------------
  | procedure to test (time) local search methods. First, 
  | builds a solution using the method specified by constr. Then,
  | executes local search on the resulting solution; if the 
  | minimum time not reached (mt), restores the original solution 
  | and does it again --- until mt is reached. Prints the average 
  | time to run everything.
  |
  | If output_density is true, also outputs the fraction of the
  | entries in extra that are nonzero. This has an important
  | effect on running time, so use false when you are really 
  | interested in time.
  *--------------------------------------------------------------*/

void Grasp::testLocalSearch(PMInstance *inst, PMConstructive *constr, char *method, double mt, bool output_density, bool profile) 
{
  double times[4];

  std::vector<int> facilityLocationConstraints(inst->getM()+1,0);
	
  BossaTimer timer(true);
  PMSolution *greedy = new PMSolution (inst);
  constr->run(greedy,0,facilityLocationConstraints);
  std::cout << "lsconsttime " << std::setprecision(2) << timer.getTime() << std::endl;
  std::cout << "lsconstmethod ";
  constr->printMethod(std::cout);
  std::cout << std::endl;
  std::cout << "lstestmaxtime " << mt << std::endl;
  std::cout << "lstestmethod " << method << std::endl;
  std::cout << "lstestcf " << PMDistanceOracle::cache_factor << std::endl;
  std::cout << "lstestmethodcf " << method << ":" << std::setprecision(1) << PMDistanceOracle::cache_factor << std::endl;

  PMSolution *temp = new PMSolution (inst);
	
  double t;
  for (int i=3; i>=0; i--) times[i] = 0.0;
  int runs = 0;
  timer.start();
		
  while (1) 
    {
      temp->copyFrom(*greedy);
      PMSearch::runLocalSearch(temp, method, facilityLocationConstraints, output_density, profile ? times : 0); 
      t = timer.getTime();
      runs++;
      if (t > mt) break;
    }

  if (profile) 
    {
      double btime, ctime, rtime, utime, ttime;
      btime = times[0];
      ctime = times[1];
      rtime = times[2];
      utime = times[3];
      ttime = times[0]+times[1]+times[2]+times[3];
		
      std::cout << "lstestbtime " << std::setprecision(4) << btime << std::endl;
      std::cout << "lstestbtimef " << std::setprecision(3) << btime / ttime << std::endl;
      std::cout << "lstestbtimep " << std::setprecision(3) << 100 * (btime / ttime) << std::endl;
      std::cout << "lstestctime " << std::setprecision(4) << ctime << std::endl;
      std::cout << "lstestctimef " << std::setprecision(3) << ctime / ttime << std::endl;
      std::cout << "lstestctimep " << std::setprecision(3) << 100 * (ctime / ttime) << std::endl;
      std::cout << "lstestrtime " << std::setprecision(4) << rtime << std::endl;
      std::cout << "lstestrtimef " << std::setprecision(3) << rtime / ttime << std::endl;
      std::cout << "lstestrtimep " << std::setprecision(3) << 100 * (rtime/ttime) << std::endl;
      std::cout << "lstestutime " << std::setprecision(4) << utime << std::endl;
      std::cout << "lstestutimef " << std::setprecision(3) << utime / ttime << std::endl;
      std::cout << "lstestutimep " << std::setprecision(3) << 100 * (utime/ttime) << std::endl;
    }

  std::cout << "lstestconstr " << std::setprecision(2) << greedy->cost() << std::endl;
  std::cout << "lstestvalue " << std::setprecision(2) << temp->cost() << std::endl;
  std::cout << "lstestruns " << runs << std::endl;
  std::cout << "lstesttime " << std::setprecision(6) << t / (double)runs << std::endl;

  delete greedy;
  delete temp;
}

int Grasp::run(int argc, char *argv[]) 
{
  BossaTimer timer;
  PMConfig configuration;

  timer.start();

  ///////////////////////////
  // read input parameters //
  ///////////////////////////

  if((argc<2)||
     (configuration.readParameters(argc,argv)==false))
    {
      showUsage(argv);
    }

  PMDistanceOracle::cache_factor=configuration.cf;
  std::cout << "Oracle cache factor=" << std::setprecision(3) << PMDistanceOracle::cache_factor << std::endl;
  
  ///////////////////////////////
  // load the problem instance //
  ///////////////////////////////

  std::cout << "Loading problem instance" << std::endl;

  char *inputFileName(argv[1]);

  std::cout << "Instance filename=" << inputFileName << std::endl;

  PMInstance *instance(readInstance(inputFileName,configuration.pvalue));
  if(configuration.pvalue>0 && configuration.pvalue<=instance->getM()) instance->setP (configuration.pvalue);
  if(configuration.rpvalue>0 && configuration.rpvalue<=1) instance->setP((int) ceil ((double)(instance->getM()) * configuration.rpvalue));
  if((instance->fixedP()==true)&&
     ((instance->getP()<=0)||(instance->getP()>instance->getM())))
    {
      std::cerr << "ERROR: Number of facilities to open (p) must be between 1 and " << instance->getM() << " (use the '-p' option)." << std::endl;
      exit(-1);
    }

  instance->outputParameters(std::cout);

  std::cout << "Fixed:" << instance->fixedP() << " p=" << instance->getP() << std::endl;

  std::cout << "Version=" << GRASP_VERSION << std::endl;
  std::cout << "Initialization time=" << std::setprecision(2) << timer.getTime() << std::endl;
  std::cout << "Oracle initialization time=" << std::setprecision(2) << instance->getOracleTime() << std::endl;

  // empty for now.
  std::vector<int> facilityLocationConstraints(instance->getM()+1,0);

  ///////////////////////////////////////
  // initialize the random number seed //
  ///////////////////////////////////////

  BossaRandom::randomize(configuration.seed);
  std::cout << std::endl << "Random number seed=" << configuration.seed << std::endl << std::endl;

  //----------------------------------------
  // create a variable to hold the solution
  //----------------------------------------

  PMSolution *s=new PMSolution(instance); // this variable will hold the solution
	
  if(strlen(configuration.inputsol)>0) 
    {
      s->readSolution(configuration.inputsol);	
    } 
  else 
    {
      PMConstructive::addRandom(s,instance->getP(),facilityLocationConstraints); // starts with a random solution
    }

  // parameters for the constructive algorithm
  PMConstructive constructive;
  constructive.setMethod(configuration.ch);

  // create pool of elite solutions
  PMElite *elite=(configuration.elite>0)?new PMElite(instance,configuration.elite):0; 

  // run grasp, updating s and populating 'elite'
  if(configuration.run_grasp)
    { 
      grasp(s,
	    facilityLocationConstraints,
	    &constructive,
	    configuration.graspit,
	    elite,
	    configuration.ls,
	    configuration.cmg,
	    true,
	    configuration.partials);
    }

  ////////////////////////
  // run path relinking //
  ////////////////////////
	
  std::cout << std::endl;
  if((elite!=0)&&(configuration.test_pr==false)&&(configuration.postopt==true)) 
    {
      std::cout << "Executing path relinking (postopt 1)" << std::endl;
      PMRelink::pathRelinking(s, elite, configuration.cmp, facilityLocationConstraints, true);
    } 

  std::cout << "usepr " << ((elite!=0)?1:0) << std::endl;
  std::cout << std::endl;

  ///////////////////////////////////////
  // test individual algorithm modules //
  ///////////////////////////////////////

  if(configuration.tc>0)
    {
      testConstructive(instance,&constructive,&configuration);
    }
  if(configuration.tls)
    {
      testLocalSearch(instance,&constructive,configuration.ls,configuration.tlstime,configuration.output_density,configuration.profile);
    }
  if(configuration.test_pr)
    {
      testPathRelinking(instance,&constructive,elite,configuration.cmg);
    }

  double totaltime(timer.getTime());
  std::cout << "totaltime " << std::setprecision(2) << totaltime << std::endl;
  totaltime += instance->getOracleTime();
  std::cout << "totaltimeo " << std::setprecision(2) << totaltime << std::endl;
  totaltime += instance->getFloydTime();
  std::cout << "totaltimeof " << std::setprecision(2) << totaltime << std::endl;
  std::cout << "cputime " << std::setprecision(2) << totaltime << std::endl;
  std::cout << "bestsol " << std::setprecision(2) << s->cost() << std::endl;

  // output best solution to file
  if(configuration.output)
    {
      char *outputFileName=new char[strlen(inputFileName)+1];
      strcpy(outputFileName,inputFileName);
      RFW::stripPath(outputFileName);
      RFW::stripExtension(outputFileName);

      s->printSolution(configuration.outname,outputFileName);

      delete [] outputFileName;
    }

  // output facility locations to file
  if(configuration.outputFacilities) 
    {
      int numFacs=s->getP();
      std::vector<int> locations;
      locations.reserve(numFacs);
      for(int i=1;i<=numFacs;i++)
	{
	  locations.push_back(s->getFacility(i));
	}
      std::sort(locations.begin(),locations.end());

      std::ofstream facilityOutputFile(configuration.outFacilityName);
      if(!facilityOutputFile)
	{
	  std::cerr << "***Failed to open facility output file=" << configuration.outFacilityName << std::endl;
	  return 0;
	}

      facilityOutputFile << "#" << std::endl;
      facilityOutputFile << "# Generated by GRASP solver interface" << std::endl;
      facilityOutputFile << "#" << std::endl;

      facilityOutputFile << rand() << " "; // the ID
      facilityOutputFile << numFacs << " "; // the number of sensors
      for(size_t i=0;i<locations.size();i++)
	{
	  facilityOutputFile << locations[i] << " ";	  
	}
      facilityOutputFile << std::endl;

      facilityOutputFile.close();
    }

  std::cout << std::endl;
  std::cout << "Number of injections " << instance->getN() << std::endl;
  std::cout << "Number of potential sensor locations " << instance->getM() << std::endl;
  std::cout << "Normalized impact " << s->cost()/double(instance->getN()) << std::endl;
  std::cout << std::endl;

  //clean up
  delete elite;
  delete s;	
  delete instance;

  return 0;
}

/*--------------------------------
  | main function: just passes the 
  | input parameters to the solver
  *-------------------------------*/

int main (int argc, char *argv[]) 
{
  Grasp *grasp=new Grasp();

  grasp->run(argc,argv);

  delete grasp;

  return 0;
}
