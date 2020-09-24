// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/******************************
 *
 * class PMEuclideanInstance
 * 
 ******************************/

#ifndef euclidean_instance_h
#define euclidean_instance_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "basics.h"
#include "distance.h"
#include "bossa_timer.h"
#include "instance.h"

class PMEuclideanInstance:public PMInstance {
 protected:
  virtual int getMetric() {return EUCLIDEAN;}

  double *fx, *fy; //facility coordinates
  double *cx, *cy; //customer coordinates

  int n; //number of nodes (users)
  int p; //number of facilities in the solution
  int m; //number of potential facilities

  void fatal (const char *func, const char *msg);
  void initOracle() {oracle = new PMDistanceOracle (this);}
  void reset();
  PMDistanceOracle *oracle;

 public:
  PMEuclideanInstance();		
  //PMEuclideanInstance(PMInstance *original, int *of, int *oc);
  virtual ~PMEuclideanInstance();
		
  inline IntDouble *getCloser(int i, double v) {return oracle->getCloser(i,v);}

  void readTSP (FILE *file, int _p=0);
  void readPMI (FILE *file, int _p=0);
  inline void setP (int _p) {p = _p;}

  double getFacDist (int f, int g) {
    if (f>m || g>m) fatal ("getFacDist", "facility number out of range");
    double dx = (fx[f] - fx[g]);
    double dy = (fy[f] - fy[g]);
    dx *= dx;
    dy *= dy;
    return sqrt (dx+dy);
  }

  double getFacCost (int f) {return 0;}

  double getDist (int u, int f) {
    if (u>n || f>m || u<0 || f<0) {
      fprintf (stderr, "\ngetDist(%d,%d) called\n", u, f);
      fatal ("getDist", "customer of facility number out of range");
    }
    double dx = (cx[u]-fx[f]);
    double dy = (cy[u]-fy[f]);
    dx *= dx;
    dy *= dy;
    return sqrt(dx+dy);
  }

  virtual int getM(void)const {return m;} 
  virtual int getN(void)const {return n;}
  virtual int getP(void)const {return p;}; 
  virtual double getOracleTime() {return oracle->getInitTime();}
};

#endif

