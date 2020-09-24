// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/**************************************************
 *
 * class PMCoverInstance: instances for the maxcover
 *       problem
 *
 * author: Renato Werneck (rwerneck@princeton.edu)
 * log: 
 *      May 21, 2003: file created
 *      Jul 27, 2003: fixed reduction
 * 
 **************************************************/

/*
  Each pop corresponds to a facility.
  Each region corresponds to a user.
*/


#ifndef COVER_INSTANCE_H
#define COVER_INSTANCE_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "basics.h"
#include "distance.h"
#include "bossa_timer.h"
#include "matrix_instance.h"
#include "rfwsort.h"

#include <iostream>
#include <iomanip>

class PMCoverInstance:public PMInstance {
 private:
  double *efactor;  //if we decide set s will cover element f, 
  double *sfactor;  //the cost will be sfactor[s] * efactor[e]
		
  double maxserve;  //max of sfactor[s]*efactor[e], for all valid pairs (e,s).

  int **covered;    //covered[u] = list of elements that cover u
  double *weight;   //weight[u] = weight of element u
  double *opencost; //opencost[f] = cost of opening f

  IntDouble *templist;

  double infinity;  //large enough to never be selected
  int special_facility; //facility that will cover uncovered sets
  int special_user;     //the user that is convered only by the special facility
  bool use_special; //use the special facility (otherwise leave with infinity)
  int p;            //number of sets to select
  int nsets;        //total number of sets
  int nelements;    //total number of elements

  //checks if an element x belongs to a list a
  inline bool contains (int *a, int x) {
    for (int i=a[0]; i>0; i--) {if (a[i]==x) return true;}
    return false;
  }

  void fatal (const char *func, const char *msg) {
    fprintf (stderr, "PMCoverInstance::%s: %s.\n", func, msg);
    exit (-1);
  }
	
 protected:
  virtual int getMetric() {return COVER;}

 public:
  //PMCoverInstance(PMInstance *original, int *of, int *oc);
  PMCoverInstance(bool _use_special);		

  void readCover (FILE *file, int _p);
  virtual ~PMCoverInstance();
  virtual IntDouble *getCloser (int i, double v);
  virtual double getFacDist (int f, int g) {
    fprintf (stderr, "WARNING: PMCoverInstance::getFacDisc(%d,%d) should not be called.\n", f, g);
    return 0.0;
  }
		
  virtual void setP (int _p) {
    if (p<=0 || p>nsets) fatal ("setP", "invalid number of facilities");
    p = _p;			
  };

  //warning: this function should be avoided --- it takes a really long time to run
  virtual double getDist (int u, int f) {
    double d;
    if (f==special_facility) {
      if (!use_special) fatal ("getDist", "facility out of range");
      d = (u==special_user) ? 0 : weight[u];
    } 
    else {
      if (u==special_user) {
	d = infinity;
      } else {
	d = (contains(covered[u],f)) ? efactor[u]*sfactor[f] : infinity;
      }
    }
    return d;
  }

  virtual double getFloydTime() {return 0.0;}
  virtual double getOracleTime() {return 0.0;}

  //parameters as viewd from the outside world
  virtual int getM(void)const {return nsets + (use_special ? 1 : 0);} 
  virtual int getN(void)const {return nelements + (use_special ? 1 : 0);}
  virtual int getP(void)const {return p + (use_special ? 1 : 0);} 

  virtual double getFacCost(int f) {return (opencost==0) ? 0 : opencost[f];}
  virtual bool fixedP(void) 
    {
    return p>0;
  }

  virtual void outputParameters(std::ostream &ostr) 
    {
    PMInstance::outputParameters(ostr);
    ostr << "sets " << nsets << std::endl;
    ostr << "elements " << nelements << std::endl;
    ostr << "setgoal " << p << std::endl;
  }
};

#endif
