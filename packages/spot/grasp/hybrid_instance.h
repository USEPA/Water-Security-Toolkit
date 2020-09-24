// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/*******
 *
 * class PMGraphInstance: 
 * 
 *******/

#ifndef hybrid_instance_h
#define hybrid_instance_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "graph_instance.h"
#include "matrix_instance.h"
#include "basics.h"
#include "bossa_timer.h"
#include "solution.h"

class PMHybridInstance:public PMInstance {
 private:
  virtual void fatal (const char *func, const char *msg);
  PMDistanceOracle *oracle;
  PMInstance *original;
  int original_n;         //original number of users
  int original_m;         //original number of potential facilities
  int *original_closest;  //for every facility, represents the element that is closest to i
  int *original_closest_2; //for every facility, represents the second closest element to i (among the original ones)
  bool *fixed_original;   //if true, the facility was in both original solutions
  int *clabel; //clabel[i] = original label of customer i
  int *flabel; //flabel[i] = original label of facility i
  double *cud; //distance from a customer to the universal facility
  int m;       //number of facilities in this instance (including the 'universal' facility)
  int n;       //number of customers in this instance
  int p;       //number of facilities that must be open in this instance
		
  void initOracle() {oracle = new PMDistanceOracle (this);}
  double base_cost;

  void updateOriginalCloseness (int f);

 public:

  virtual int getMetric() {return HYBRID;}
  PMHybridInstance();
  inline IntDouble *getCloser(int i, double v) {return oracle->getCloser(i,v);}

  virtual double getFacCost (int f) {return 0;}
			
  void restoreSolution (PMSolution *solcur, PMSolution *solorg);

  PMHybridInstance (PMSolution *s, PMSolution *t);
  //PMHybridInstance (PMInstance *original, bool *customers, bool *facilities);

  inline double getBaseCost () {return base_cost;}
  inline int getFacLabel (int i) {return flabel[i];}
  inline int getLabel (int i) {return clabel[i];}
  inline bool isFixedOriginal (int i) {return fixed_original[i];}

  void original2sub (PMSolution *solorg, PMSolution *solsub);

  ~PMHybridInstance();
  double getFacDist (int f, int g);

  double getDist (int u, int f);

  double getFloydTime() {return 0.0;}
  double getOracleTime() {return oracle->getInitTime();}
  virtual int getP(void)const {return p;}
  virtual int getN(void)const {return n;}
  virtual int getM(void)const {return m;}
  virtual void setP(int _p) {p = _p;}
};

#endif

