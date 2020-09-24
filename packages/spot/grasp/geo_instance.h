// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/*******
 *
 * class PMGeoInstance: 
 * 
 *******/

#ifndef geo_instance_h
#define geo_instance_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "basics.h"
#include "distance.h"
#include "bossa_timer.h"
#include "instance.h"

class PMGeoInstance:public PMInstance {
 protected:
  virtual int getMetric() {return GEO;}
		
  double *flongitude, *flatitude; //latitudes for facilities
  double *clongitude, *clatitude; //longitudes for facilities

  int n; //number of nodes (users)
  int p; //number of facilities in the solution
  int m; //number of potential facilities

  void fatal (const char *func, const char *msg);
  void reset();
  PMDistanceOracle *oracle;

  void initOracle() {
    oracle = new PMDistanceOracle (this);
  }


 public:
  PMGeoInstance();		
  virtual ~PMGeoInstance();
  virtual IntDouble *getCloser(int i, double v) {return oracle->getCloser(i,v);}
  void readGEO (FILE *file, int _p);
  virtual void setP (int _p) {p = _p;}
  virtual double getFacCost (int f) {return 0;}

  virtual double getFacDist (int f, int g) {
    if (f>m || g>m) fatal ("getFacDist", "facility number out of range");

    if (f==0 || g==0) return POPSTAR_INFINITY;
    return RFW::sphered (flongitude[f],flatitude[f],flongitude[g],flatitude[g]);
  }
		

  inline double getDist (int u, int f) {
    if (u>n) fatal ("getDist", "customer number out of range");
    if (f>m) fatal ("getDist", "facility number out of range");

    if (u==0 || f==0) return POPSTAR_INFINITY;
    const double R = 3963.0; //miles
    return R * RFW::sphered (clongitude[u],clatitude[u],flongitude[f],flatitude[f]);
  }
  virtual int getM(void)const {return m;} 
  virtual int getN(void)const {return n;}
  virtual int getP(void)const {return p;}; 
  virtual double getFloydTime() {return 0.0;}
  virtual double getOracleTime() {return oracle->getInitTime();}
};

#endif

