// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef SEARCH_H
#define SEARCH_H

#include "basics.h"
#include "solution.h"
#include "instance.h"
#include "bossa_random.h"
#include "elite.h"
#include "search_tables.h"

#include <float.h>

#include <vector>

/*-----------------------
  |
  | Generic Search Class
  |
  *----------------------*/

class PMSearch 
{
 private: 

  virtual double bestNeighbor(PMSolution *s, const std::vector<int> &facilityLocationConstraints, SearchTables *tab, int *rcand, int *icand, int &bestr, int &besti, bool first, bool *temp_array);
  virtual void   updateArrays(PMSolution *s, bool *affected, SearchTables *tab, bool add, double *stats_total_prefix_size = 0);

 protected:

  static void fatal(const char *func, const char *msg);

 public:

  virtual double search(PMSolution *s, const std::vector<int> &facilityLocationConstraints, bool best_improvement, PMElite *elite, bool use_list, double *nz = 0, double *profile=0, double terminationThreshold=0.0);
  virtual void   getBounds (int &insert, int &remove) = 0; //number of candidates for insertion and deletion
  virtual void   getCandidates (int *rcand, int *icand) = 0;
  virtual bool   reportMove (int r, int i, double profit) = 0;
  virtual bool   firstImprovement () = 0;

  static void move(PMSolution *s, const std::vector<int> &facilityLocationConstraints, int in, int &out, double &w);
  static void fastInterchange(PMSolution *s, const std::vector<int> &facilityLocationConstraints, int *itcount=0, double terminationThreshold=0.0);
  static void localSearch(PMSolution *s, bool best_improvement, bool uselist, const std::vector<int> &facilityLocationConstraints, int *itcount=0, bool stats=false, bool output_density=false, double *profile=0, double terminationThreshold=0.0);
  static void update(PMSolution *s, int in, int out);
  static void runLocalSearch(PMSolution *s,char *lsmethod,const std::vector<int> &facilityLocationConstraints,bool output_density=false,double *profile=0,double terminationThreshold=0.0);
};


/*----------------
  | Path-relinking
  *---------------*/

class PMSearchPathRelinking : public PMSearch 
{
 private:
  //int p;       //number of facilities
  int *insert; //list of candidates for insertion (insert[0] is the size)
  int *remove; //list of candidates for removal   (remove[0] is the size)
  PMInstance *inst;

 public:
  virtual void getBounds (int &ins, int &rem) {ins = rem = inst->getM();} //number of candidates for insertion and deletion
  virtual bool firstImprovement () {return false;}
  void setSolutions (PMSolution *source, PMSolution *target);
  PMSearchPathRelinking (PMInstance *_inst);
  virtual void getCandidates (int *rcand, int *icand);
  virtual bool reportMove (int r, int i, double profit);		
  virtual ~PMSearchPathRelinking ();
};


/*--------------
  | local search
  *-------------*/

class PMSearchLocal:public PMSearch 
{
 private:
	
  PMInstance *inst;
  bool *solution;
		
  int p;
  int m;
  int it; //number of local search iterations performed

 public:

  virtual bool firstImprovement(void) {return false;}
  virtual bool reportMove(int r, int i, double profit);

  virtual void getBounds(int &insert, int &remove) 
    {
      if (inst->fixedP()) 
	{
	  insert=m-inst->getP(); 
	  remove=inst->getP();
	}
      else 
	{
	  insert=remove=m;
	}
    }

  virtual void getCandidates(int *rcand, int *icand); //get sorted lists of candidates

  inline int getIterations(void) 
    {
      return it;
    }		

  PMSearchLocal(PMSolution *s);
  virtual ~PMSearchLocal(void);
};


#endif

