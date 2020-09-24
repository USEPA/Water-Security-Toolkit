// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include "search.h"
#include "constructive.h"
#include "bossa_timer.h"

#include <iostream>
#include <iomanip>
#include <vector>

#include <float.h>

/*------------------------
  | local search procedure
  *-----------------------*/

void PMSearch::localSearch(PMSolution *s, 
			   bool best_improvement, 
			   bool uselist, 
			   const std::vector<int> &facilityLocationConstraints,
			   int *itcount, 
			   bool stats, 
			   bool output_density, 
			   double *profile,
			   double terminationThreshold) 
{
  double density;
  int it;
  BossaTimer timer;
  bool verbose(false);

  timer.start();
  if(verbose)
    { 
      std::cout << "Allocating local search object..." << std::endl;
    }
  PMSearchLocal *sm=new PMSearchLocal(s);
  if(verbose)
    {
      std::cout << "Running local search..." << std::endl;
    }
  sm->search(s,facilityLocationConstraints,best_improvement,0,uselist,output_density ? &density : 0, profile, terminationThreshold);
  if(verbose)
    {
      std::cout << "end of local search" << std::endl;
    }
  it=sm->getIterations();
  if(itcount!=0)
    {
      *itcount = it;
    }

  if(verbose)
    {
      std::cout << "[" << it << "]  ";
    }

  delete sm;

  if(stats) 
    {
      std::cout << "ls " << std::setprecision(2) << s->cost() << std::endl;
      std::cout << "lstime " << std::setprecision(2) << timer.getTime() << std::endl;
      std::cout << "lsit " << it << std::endl;
    }

  if(verbose)
    {
      std::cout << "[ls:" << std::setprecision(2) << timer.getTime() << " ";
    }
}


/*-----------------------------------------
  | update routine for the FI heuristic, as
  | described in HM97
  *----------------------------------------*/

void PMSearch::update(PMSolution *s, 
		      int in, 
		      int out) 
{ 
  //, int *closest, int *closest2) 
  PMInstance *inst = s->getInstance();
  int n = s->getN();
  int i;

  if (!s->contains(in)) fatal ("update", "invalid insertion");
  if (s->contains(out)) fatal ("update", "invalid removal"); 
	
  bool *affected = new bool [n+1];	//WARNING: THIS IS UNFAIR
  for (i=1; i<=n; i++) affected[i] = false;

  for (i=1; i<=n; i++) {
    double din = inst->getDist(i,in);
    double d1 = inst->getDist (i, s->getClosest(i));
    double d2 = inst->getDist (i, s->getClosest2(i));
		
    //case a: we're removing the closest element
    if (s->getClosest(i) == out) {
      if (din <= d2) { //first case: in is the new closest
	s->setClosest(i, in); //closest2[i] remains unaltered, it isn't out
      } else { //second case: in is not the closest
	s->setClosest(i, s->getClosest2(i));
	s->setClosest2(i, 0);
	affected[i] = true; 
      }
    }
    //we're removing an arbitrary element
    else 
      { 
	if (din < d1) { //in is the new closest
	  s->setClosest2(i, s->getClosest(i));
	  s->setClosest(i, in);
	} 
	else 
	  {
	    if (din < d2) { //in is closer than the second closest
	      s->setClosest2(i,in); //in becomes new second closest
	    } else { //last case: second closest was affected
	      if (s->getClosest2(i)==out) {affected[i] = true;}
	    }
	  }
      }
  }	
	
  // now we just have to find the second 
  // closest for affected elements
  int count = 0;
  for (i = 1; i<=n; i++) {
    if (s->getClosest(i) == s->getClosest2(i)) fatal ("update", "something is wrong");
    if (affected[i]) {
      count++;
      s->updateClosest (i);
    }
  }

  delete [] affected;
}

/*---------------------------------------------------
  | fastInterchange: executes the fast interchange
  | local search procedure, implemented as suggested
  | in HM97. 'it' (an output parameter) is the number
  | of iterations actually performed.
  *--------------------------------------------------*/

void PMSearch::fastInterchange(PMSolution *s, 
			       const std::vector<int> &facilityLocationConstraints,
			       int *itcount,
			       double terminationThreshold) 
{
  int p(s->getP());
  int m(s->getM());
	
  int numIterations(0);

  double currentCost(s->cost());
  bool success(true);

  while(success==true) 
    {
      double bestDelta(DBL_MAX);
      int bestIn(0);
      int bestOut(0);
		
      for(int i=p+1;i<=m;i++) 
	{
	  // try every facility outside the current solution
	  int in(s->getFacility(i)); // it will be a candidate for insertion

	  // can't open a facility at an invalid location
	  if(facilityLocationConstraints[in]!=2)
	    {
	      // the facility is not invalid
	      int out;                   
	      double delta;
	      PMSearch::move(s,facilityLocationConstraints,in,out,delta); // find the best corresponding *removal*
	      if(delta<bestDelta) 
		{
		  // if the associated cost is better than what we currently have, do it
		  bestDelta=delta;
		  bestIn=in;
		  bestOut=out;
		}
	    }
	}

      if(bestDelta<(-EPSILON))
	{
	  currentCost+=bestDelta;   // update current cost
	  s->remove(bestOut,false); // remove the old facility
	  s->add(bestIn,false);     // insert the one
	  update(s,bestIn,bestOut); // update closest information

	  if(currentCost<=terminationThreshold)
	    {
	      success=false;
	    }
	  else
	    {
	      success=true;
	    }
	}
      else
	{
	  success=false;
	}

      numIterations++;
    }	

  if(itcount!=0)
    { 
      (*itcount)=numIterations;
    }

  // we know that the solution is now consistent, but the solution doesn't
  for(int i=1;i<=s->getN();i++)
    {
      s->forceConsistent(i); 
    }
}

void PMSearch::runLocalSearch(PMSolution *s,
			      char *localSearchMethod,
			      const std::vector<int> &facilityLocationConstraints,
			      bool output_density,
			      double *profile,
			      double terminationThreshold)
{
  for(int i=0;localSearchMethod[i]!='\0';i++) 
    {
      switch(localSearchMethod[i]) 
	{
	case 'n': // none

	  break;         

	case 'f': // fast interchange 

	  fastInterchange(s,facilityLocationConstraints,0,terminationThreshold); 
	  break;                                 

	case 'm': // matrix

	  localSearch(s,true,false,facilityLocationConstraints,0,false,output_density,profile,terminationThreshold); 
	  break; 

	case 's': // sparse matrix

	  localSearch(s,true,false,facilityLocationConstraints,0,false,output_density,profile,terminationThreshold); 
	  break; 

	default: 
	  
	  fatal("runLocalSearch", "invalid local search method");
	}
    }
}

void PMSearch::fatal(const char *func, 
		     const char *msg) 
{
  std::cerr << "PMSearch::" << func << ": " << msg << std::endl;
  exit(-1);
}


/*-------------------------------------------------------------
  | given a candidate for insertion (in), finds a the best
  | candidate for removal (out), together with the variation
  | in solution cost (w) associated with the resulting swap (w)
  *------------------------------------------------------------*/

void PMSearch::move(PMSolution *s,
		    const std::vector<int> &facilityLocationConstraints,
		    int in,
		    int &out,
		    double &w) 
{
  if(s->contains(in)==true)
    {
      fatal("move","invalid insertion facility");
    }

  int p(s->getP());
  int n(s->getN()); // number of customers
  int m(s->getM()); // number of facilities
  PMInstance *instance(s->getInstance());

  w=0; // cost increase obtained by best interchange
	
  double *v=new double[m+1]; // net loss for replacing a given facility // WARNING: THIS IS UNFAIR
  for(int i=1;i<=p;i++)
    {
      // only a subset of the elements are ever used
      v[s->getFacility(i)]=0.0; 
    }
	
  // compute w and n
  for(int i=1;i<=n;i++) 
    {
      double din(instance->getDist(i,in));
      double d1(s->getDistClosest(i));
      double d2(s->getDistClosest2(i));

      if(din<d1) 
	{   
	  // WARNING: THIS LINE WAS WRONG IN THE PAPER (originally, din < d2)
	  w+=(din-d1); // in will be the new closest, we'll save something
	} 
      else 
	{
	  // remove closest[i]: save d1, but must spend either din or d2
	  double min=(din<d2)?din:d2;
	  v[s->getClosest(i)]+=(min-d1); // we'll have to pay for the difference 
	}
    }	

  // find best facility to remove
  int best(-1);
  for(int i=1;i<=p;i++) 
    {
      int outCandidate(s->getFacility(i));

      // can't displace a fixed facility...
      if(facilityLocationConstraints[outCandidate]!=1)
	{
	  if((best==-1)||(v[outCandidate]<v[best]))
	    {
	      best=outCandidate;
	    }
	}
    }

  out=best;
  w+=v[best];

  delete [] v;
}

/*************************************************************
 *
 * bestNeighbor: use save, lose, and extra to find the best
 *               neighbor in O(p(m-p)) time. Returns the profit
 * that would be obtained if bestr was replaced with besti.
 * This function is used by localSearch.
 * 
 *************************************************************/

double PMSearch::bestNeighbor(PMSolution *s,     //input
			      const std::vector<int> &facilityLocationConstraints,
			      SearchTables *tab, //input: save, lose, extra...
			      int *rcand,        //input (list of candidates for removal)
			      int *icand,        //input (list of candidates for insertion)
			      int &bestr,        //output: best element to remove
			      int &besti,        //output: best element to insert
			      bool first,        //input: use first improvement?
			      bool *temp_array)  //avoids extra memory allocation
{

  double *save = tab->save;
  double *lose = tab->lose;
  double **extra = tab->extra;
  double bestprofit = -POPSTAR_INFINITY;

  if(first) RFW::shuffle(rcand,rcand[0]);

  bestr = 0;
  besti = 0;

  int bestji, bestjr; //just insertion or just removal

  if(tab->usingList()) 
    {
      // NOTE: This variant is not used in the EPA distribution, and
      //       has not been maintained. Specifically, it does not
      //       obey any of the facility location constraints.
      int j;
      //------------------------------------------------
      // build incidence vector of insertion candidates
      //------------------------------------------------
      int m = s->getInstance()->getM();
      //bool *iscand = new bool [m+1];  //WARNING: I shouldn't be allocating stuff here...
      bool *iscand = temp_array;
      for (j=0; j<=m; j++) iscand[j] = false;
      for (j=icand[0]; j>0; j--) iscand[icand[j]] = true;
		
      //----------------------------------------------------
      // calculate maxsave and minlose; this will give the
      // best profit assuming all extras are zero; positive
      // extras can only improve the result
      //----------------------------------------------------
      double minlose = POPSTAR_INFINITY;
      for (j=rcand[0]; j>0; j--) 
	{
	  int r = rcand[j];
	  double v = lose[r]; //already includes opening cost
	  if (v<minlose || bestr==0) {bestr = r; minlose = v;}
	}
      double maxsave = -POPSTAR_INFINITY;
      for (j=icand[0]; j>0; j--) 
	{
	  int i = icand[j];
	  double v = save[i]; //already includes opening cost
	  if (v>maxsave || besti==0) {besti = i; maxsave = v;}
	}
      bestprofit = save[besti] - lose[bestr];
      bestji = besti;
      bestjr = bestr;

      //-------------------------------------------------
      // find the best moves considering "extra" factors
      //-------------------------------------------------
      for (j=rcand[0]; j>0; j--) 
	{
	  int r = rcand[j];
	  ExtraNode *current = tab->list[r]->next;
	  while(current->node <= m) 
	    {
	      int i = current->node;
	      if (iscand[i]) {
		double profit = save[i] - lose[r] + current->value;//extra[r][i];
		if (profit > bestprofit) 
		  {
		    bestprofit = profit;
		    bestr = r; besti = i;
		  }
	      }
	      current = current->next;
	    }
	}

      if(s->getInstance()->fixedP()==false) 
	{
	  if (bestprofit < maxsave) 
	    {
	      bestprofit = maxsave;
	      besti = bestji;
	      bestr = 0;
	    }
	  if (bestprofit < -minlose) 
	    {
	      bestprofit = -minlose;
	      bestr = bestjr;
	      besti = 0;
	    }

	  if (!(bestr || besti)) fatal ("bestNeighbor", "move cannot be made");

	}
      //delete [] iscand;
    } 
  else 
    {
      if(s->getInstance()->fixedP()==false) 
	{
	  fatal("bestNeighbor", "full matrix representation only implemented for the p-median problem");
	}
      for(int j=rcand[0];j>0;j--) 
	{
	  int r(rcand[j]); // candidate for removal

	  // can't close a fixed facility
	  if(facilityLocationConstraints[r]!=1)
	    {
	      for(int k=icand[0];k>0;k--) 
		{
		  int i(icand[k]); // candidate for insertion

		  // can't open an invalid facility
		  if(facilityLocationConstraints[i]!=2)
		    {
		      double profit=save[i]-lose[r]+extra[r][i];
		      if(profit>bestprofit) 
			{
			  bestprofit=profit;
			  bestr=r; 
			  besti=i;
			}			

		      if((first==true)&&(bestprofit>EPSILON))
			{ 
			  break;
			}
		    }
		}
	    }
	}
    } 

  return bestprofit;
}

/*--------------------------------------------------------------
  | updateArrays: updates the contents of save, lose, and extra
  | - checks only the neighborhood of vertex j
  | - uses closest and closest2, which must be up-to-date
  | - this function is a subroutine of the search procedure
  *-------------------------------------------------------------*/

void PMSearch::updateArrays(PMSolution *s,     //input: current solution
			    bool *affected,    //input: list of vertices whose neighborhood we need to check
			    SearchTables *tab, //input/output: these are the arrays that will be updated
			    bool add,          //input: if true, add contributions; if false, subtract them
			    double *stats_total_prefix_size
			    ) 
{
  double *save = tab->save;
  double *lose = tab->lose;
  PMInstance *inst = s->getInstance();
  int n = s->getN();

  int count_affected = 0;
  int count_nz = 0;

  for(int j=1; j<=n; j++) 
    {
      if (!affected[j]) continue;
      int r = s->getClosest(j);           //element that is closest to j
      double d1 = s->getDistClosest (j);  //distance to the closest
      double d2 = s->getDistClosest2 (j); //distance to the second closest
      tab->setCurrent(r);                 //future operations will refer to row r
      IntDouble *closelist = inst->getCloser(j, d2); //case 1

      if (stats_total_prefix_size) *stats_total_prefix_size += closelist[0].id;

      count_affected ++;
      count_nz += closelist[0].id;

      if (add) 
	{
	  lose[r] += (d2-d1);
	  for (int x=1; x<=closelist[0].id; x++) 
	    { 
	      int i = closelist[x].id;         //get the facility
	      if (s->contains(i)) continue;    //must not be in the solution
	      double dji = closelist[x].value;
	      double profit = d1 - dji;   
	      double correction = d2;
	      if (profit>0) {correction-=d1; save[i]+=profit;} //case 3
	      else {correction-=dji;}                 //case 2
	      tab->addExtra(i,correction);
	    }
	} 
      else
	{
	  lose[r] -= (d2-d1);
	  for (int x=1; x<=closelist[0].id; x++) 
	    {
	      int i = closelist[x].id;
	      if (s->contains(i)) continue;
	      double dji = closelist[x].value;
	      double profit = d1-dji;
	      double correction = d2;
	      if (profit>0) {correction-=d1; save[i]-=profit;} //case 3
	      else {correction-=dji;}                 //case 2
	      tab->addExtra(i,-correction);
	    }
	}
    }
}


/*------------------------------------------------------------
  | search: in each iteration, removes one facility from
  |         the solution and inserts another one to keep
  | its place. The pair of facilities involved is the one that
  | minimizes the total profit. Each iteration runs in O(n^2)
  | time. The stop criterion depends on sm.
  *-----------------------------------------------------------*/

double PMSearch::search(PMSolution *s, 
			const std::vector<int> &facilityLocationConstraints,
			bool best_improvement, 
			PMElite *elite, 
			bool use_list, 
			double *density, 
			double *times,
			double terminationThreshold) 
{
  bool debug(false);
  bool in_valley(false);

  if(!best_improvement)
    {
      fatal("search", "Sorry, only best improvement is implemented.\n");
    }

  //-----------------------
  // variable declarations
  //-----------------------
  BossaTimer tc(false), tb(false), tu(false), tr(true); //timers

  SearchTables *tab;      //contains gain, loss, and extra
  bool *affected;         //vertices affected (incidence list)
  int *icand;             //candidates for insertion
  int *rcand;             //candidates for removal
  bool *temp_array;       //temporary array to be used by best neighbor
  int i, n, m;
  double tot_profit = 0;  //highest profit in history
  double cur_profit = 0;  //current profit (w.r.t. original solution)

  PMInstance *inst = s->getInstance(); //the instance we're dealing with
  n = s->getN(); //total number of cities
  m = s->getM(); //number of potential facilities

  bool stats = (density != 0);
  double stats_prefix = 0.0; //total size of all prefixes checked
  double stats_affected = 0.0; //total number of users affected
  double stats_extra_density = 0.0; //sum of densities over all iterations

	
  //------------
  // allocation 
  //------------
  int insbound, rembound;
  getBounds (insbound, rembound);

  affected = new bool [n+1];   //list of affected users
  tab      = new SearchTables (m, insbound * rembound, use_list); //lose, gain, extra
  icand    = new int [m+1];    //number of candidates to insert
  rcand    = new int [m+1];    //number of candidates to remove
  temp_array = new bool [m+1]; //to be used by bestNeighbor
	
	
  //-----------------
  // initialization
  //-----------------
  tab->init(s); //allocate arrays, initialize stuff to zero, etc.
  for(i=1; i<=n; i++) affected[i] = true; //everything must be initialized

  //if (stats) stats_affected = (double)n;

  s->ensureConsistency();

  bool success = true;

  int it_count = 0;
	
  while(success) 
    {
      it_count ++;

      //------------------------------
      // update save, lose, and extra	
      //------------------------------		
      tr.pause(); 
      tu.resume();
      updateArrays(s, affected, tab, true, stats ? &stats_prefix : 0);
      tu.pause();	
      tr.resume();

      if (stats) 
	{
	  int extra_nz, p;
	  p = s->getP();
	  extra_nz = tab->countNonZeros();
	  stats_extra_density += ((double)extra_nz) / (double)(p * (m-p));
	}

      //------------------------------------------------------------
      // build list of candidates for removal and insertion
      // the set depends on what search method we're dealing with
      //------------------------------------------------------------
      getCandidates(rcand,icand);

      //--------------------------
      // find the best candidates
      //--------------------------
      int bestr, besti;
      tr.pause(); tb.resume();
      double profit=bestNeighbor(s, facilityLocationConstraints, tab, rcand, icand, bestr, besti, firstImprovement(), temp_array);
      tb.pause(); tr.resume();
		
      //------------------------------------
      // check if that's indeed a good move
      //------------------------------------
      success = reportMove(bestr, besti, profit);
      //-----------------------------------------------------------------
      // if we are leaving a valley, we might want to store the solution
      //-----------------------------------------------------------------
      if (elite!=0) 
	{
	  if (profit>0) 
	    {
	      //just went down
	      in_valley = true;
	    } 
	  else if (profit < 0) 
	    { 
	      //went up --- no longer in valley
	      if (in_valley) elite->add(s, s->cost()); //we have a local optimum in our hands
	      in_valley = false;
	    }
	}

      //----------------
      // compute profit
      //----------------
      cur_profit += profit;
      if (cur_profit>tot_profit) tot_profit = cur_profit;

      //--------------------------------------------------------
      // update things for the next iteration (if there is one)
      //--------------------------------------------------------
      if (success) 
	{
	  //------------------------------------------------------------------
	  // we'll have to determine all vertices that were somehow affected
	  //------------------------------------------------------------------

	  for (i=1; i<=n; i++) 
	    {
	      affected[i] = 
		(s->getClosest(i)==bestr)  || //closest vertex removed
		(s->getClosest2(i)==bestr) || //second closest vertex removed
		(s->getDistClosest2(i) > inst->getDist(i,besti)); // new vertex is very close

	      if (stats && affected[i]) stats_affected ++;
	    }

	  //------------------------------------------------------
	  // for every vertex that was affected, we must subtract
	  // its contributions to save, lose, and extra. New 
	  // contributions will be computed by updateArray later
	  //------------------------------------------------------
	  tr.pause(); 
	  tu.resume();
	  updateArrays(s, affected, tab, false);
	  tu.pause(); 
	  tr.resume();

	  //------------------------------------
	  // now we finally change the solution
	  //------------------------------------
	  tr.pause(); 
	  tc.resume();
	  s->swap (bestr, besti, true);
	  tc.pause(); 
	  tr.resume();

	  //----------------------------------------------------
	  // lose[besti] and save[bestr] were undefined before,
	  // we have to initialize them now. The actual values
	  // will be calculated later in by updateArray
	  //----------------------------------------------------
	  if (besti>0) tab->lose[besti] = -inst->getFacCost(besti);
	  if (bestr>0) tab->save[bestr] = -inst->getFacCost(bestr);
			
	  //-----------------------------------------------------------
	  // the search table must know it's dealing with a different
	  // set of vertices now
	  //-----------------------------------------------------------
	  tab->reportMove(besti, bestr);
	}

      if((terminationThreshold>0.0)&&(s->cost()<=terminationThreshold))
	{
	  success=false;
	}
    }
	
  if(debug) 
    {
      std::cout << std::endl << std::setprecision(2) << "[update:" << tu.getTime() << " best:" << tb.getTime() << " rest:" << tr.getTime() << " close:" << tc.getTime() << "]" << std::endl;
    }

  if (stats) 
    {
      *density = stats_extra_density / it_count;		
      std::cout << "extradensity " << std::setprecision(5) << *density << std::endl;
      std::cout << "extrapdensity " << std::setprecision(3) << (*density) * 100.0 << std::endl;
		
      double a = 0;
      if (it_count > 1) {
	a = stats_affected / ((it_count-1) * n); 
      }
      std::cout << "affected " << std::setprecision(5) << a << std::endl;
      std::cout << "affectedp " << std::setprecision(3) << a * 100.0 << std::endl;

      //total number of elements that could be checked:
      //m for each affected user (including the n initial users)
      double x = stats_prefix / ((stats_affected+n) * m); 
      std::cout << "prefix " << std::setprecision(5) << x << std::endl;
      std::cout << "prefixp " << std::setprecision(3) << x * 100.0 << std::endl;
      std::cout << "lsiterations " << it_count << std::endl;
    }

  //-------------------
  // deallocate arrays
  //-------------------
  delete [] icand;
  delete [] rcand;
  delete [] affected;
  delete [] temp_array;
  delete tab;

  //---------------------------
  // return how much was saved
  //---------------------------
  if (times) 
    {
      times[1] += tb.getTime(); //finding best move
      times[0] += tc.getTime(); //closeness
      times[3] += tr.getTime(); //rest
      times[2] += tu.getTime(); //updating structures
    }

  return tot_profit;
}



/**************************
 *
 * PMSearchPathRelinking
 *
 **************************/

/*----------------------------------------------------------------------
  | Define source and target solutions for the path-relinking procedure;
  | creates the sets of candidates for insertion and deletion.
  *---------------------------------------------------------------------*/
			
void PMSearchPathRelinking::setSolutions(PMSolution *source, PMSolution *target) 
{
  //some checks
  if (source->getInstance()!=target->getInstance()) fatal ("setSolutions", "source and target solutions must refer to same instance.");
  if (source->getInstance()!=inst) fatal ("setSolutions", "solutions not consistent with instance");


  insert[0] = remove[0] = 0; //begin with empty lists	

  int p = source->getP();
  if (target->getP() > p) p = target->getP();

  for (int i=p; i>0; i--) {
    //facility in source, but not target: candidate for removal
    int f = source->getFacility(i);
    if (source->contains(f) && !target->contains(f)) {remove[++remove[0]] = f;}

    //facility in target, but not source: candidate for insertion
    f = target->getFacility(i);
    if (target->contains(f) && !source->contains(f)) {insert[++insert[0]] = f;}
  }
}


/*-------------
  | constructor
  *------------*/

PMSearchPathRelinking::PMSearchPathRelinking(PMInstance *_inst) 
{
  inst = _inst;
  insert = new int [inst->getM()+1]; //list of candidates for insertion (size at most m)
  remove = new int [inst->getM()+1]; //list of candidates for removal (ditto)
}


/*------------------------
  | get list of candidates
  *-----------------------*/

void PMSearchPathRelinking::getCandidates(int *rcand,int *icand) 
{
  int i;
  for (i=remove[0]; i>=0; i--) rcand[i] = remove[i]; //note that size will be copied also
  for (i=insert[0]; i>=0; i--) icand[i] = insert[i];
}


/*----------------------------------------------------------
  | report a removal and/or insertion; returns true if there
  | is still some move to be performed
  *---------------------------------------------------------*/

bool PMSearchPathRelinking::reportMove(int r, int i, double profit) 
{

  if (i>0) { //remove i from list of possible insertions
    int j;
    for (j=insert[0]; j>0; j--) {
      if (insert[j]==i) {insert[j] = insert[insert[0]--]; break;} //last element goes to position j
    }
    if (j==0) fatal ("reportMove", "invalid insertion");
  }

  if (r>0) { //remove r from list of possible removals
    int j;
    for (j=remove[0]; j>0; j--) {
      if (remove[j]==r) {remove[j] = remove[remove[0]--]; break;} //last element goes to position j
    }
    if (j==0) fatal ("reportMove", "invalid removal");
  }

  return ((insert[0]>0) && (remove[0]>0)); //go on if there is still some move to make
}

/*------------
  | destructor 
  *-----------*/

PMSearchPathRelinking::~PMSearchPathRelinking(void) 
{
  if (insert) delete [] insert;
  if (remove) delete [] remove;
}



/****************************************
 *
 * local search
 *
 ****************************************/

bool PMSearchLocal::reportMove (int r, int i, double profit) 
{
  if (r>0) 
    {
      if (!solution[r]) fatal ("reportMove", "inconsistent removal");
      solution[r] = false;  //remove r
    }
  if (i>0) 
    {
      if (solution[i]) fatal ("reportMove", "inconsistent insertion");
      solution[i] = true;   //insert i
    }
  it ++;
  return (profit > EPSILON);
}

//must be in order
void PMSearchLocal::getCandidates (int *rcand, int *icand) 
{
  rcand[0] = icand[0] = 0;
  for (int i=1; i<=m; i++) {
    if (solution[i]) {rcand[0]++; rcand[rcand[0]]=i;}
    else {icand[0]++; icand[icand[0]]=i;}
  }
}

PMSearchLocal::PMSearchLocal(PMSolution *s) 
{
  it = 0;
  inst = s->getInstance();
  //p = inst->getP();
  m = inst->getM();
  solution = new bool [m+1];
  for (int i=1; i<=m; i++) solution[i] = s->contains(i);	
}

PMSearchLocal::~PMSearchLocal(void) 
{
  delete [] solution;
}


