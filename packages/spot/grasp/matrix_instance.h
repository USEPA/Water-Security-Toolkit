// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/*---------------------------------------------------------------
  | class PMInstance: 
  |   represents the distance matrix for the problem and the list
  |   of potential facilities. Instances can be read from an input
  |   file.                   
  |
  | author: Renato Werneck (rwerneck@princeton.edu)
  | log: 
  |      May 29, 2002: file created
  *---------------------------------------------------------------*/

#ifndef matrix_instance_h
#define matrix_instance_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "basics.h"
#include "distance.h"
#include "instance.h"

#include <iostream>
#include <iomanip>

class PMMatrixInstance:public PMInstance {
 private:
  double oracle_time;
  void reset();
  void fatal (const char *func, const char *msg);
  PMDistanceOracle *oracle;

 protected:
  void resetDistances();
  void initOracle() {oracle = new PMDistanceOracle (this);}
  double **d; //distance matrix
  double *fc; //facility cost
  int n; //number of nodes (users)
  int p; //number of facilities we are aiming for
  int m; //number of potential facilities

  inline void checkFacility(int f) {
    if ((f<1) || (f>m)) 
      {
	std::cerr << "ERROR: facility " << f << " is out of range" << std::endl;
	exit(1);
      }
  }

  inline void checkUser (int u) {
    if (u<1 || u>m) 
      {
	std::cerr << "ERROR: user " << u << " is out of range" << std::endl;
	exit(-1);
      }
  }

 public:
  PMMatrixInstance();		
  PMMatrixInstance (PMInstance *original, int *of, int *oc);
  ~PMMatrixInstance();		

  virtual double getOracleTime() {return oracle->getInitTime();}
  virtual int getMetric() {return MATRIX;}
  virtual IntDouble *getCloser (int i, double v) {return oracle->getCloser(i,v);}
  void readPMM (FILE *file, int _p=0);
  void readUFL (FILE *file);
  void printMatrix (FILE *file);

  virtual double getFacDist (int f, int g) 
    {
      if (f>m || g>m) fatal ("getFacDist", "facility number out of range");
      return d[f][g]; //assumes facilities and users are the same thing
    }
		
  virtual double getDist (int u, int f) 
    {
      if((u<0)||(u>n)) fatal ("getDist", "customer number out of range");
      if((f<0)||(f>m)) fatal ("getDist", "facility number out of range");
      return d[u][f];
    }

  virtual double getFacCost(int f) {return (fc==0) ? 0 : fc[f];}
  virtual int getM(void)const {return m;} 
  virtual int getN(void)const {return n;}
  virtual int getP(void)const {return p;}
  virtual void setP (int _p) {p = _p;}
  virtual bool fixedP() {return (fc==0);}
};

#endif
