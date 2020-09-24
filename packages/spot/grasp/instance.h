// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/*----------------------------------------------------------------
  |
  | class PMInstance: 
  |   represents the distance matrix for the problem and the list
  |   of potential facilities. Instances can be read from an input
  |   file.                   
  |
  | author: Renato Werneck (rwerneck@princeton.edu)
  | log: 
  |      May 29, 2002: file created
  | 
  *-----------------------------------------------------------------*/

#ifndef instance_h
#define instance_h

#include <stdio.h>
#include "basics.h"

#include <iostream>
#include <iomanip>

class PMInstance 
{
 protected:

  enum {NOMETRIC, MATRIX, EUCLIDEAN, GEO, HYBRID, COVER,IMPACT}; 
  virtual int getMetric(void)=0;

 public:

  PMInstance(void){};		
  //PMInstance(PMInstance *original, int *of, int *oc)=0;
  virtual ~PMInstance(void){};
  virtual IntDouble *getCloser(int i, double v)=0;
  virtual void setP(int _p)=0; // number of facilities to open
  virtual double getDist(int u, int f)=0;
  virtual int getM(void)const=0; 
  virtual int getN(void)const=0;
  virtual int getP(void)const=0;
  virtual double getFloydTime(void){return 0.0;}
  virtual double getOracleTime(void)=0;
  virtual double getFacDist(int f, int g)=0;
  virtual double getFacCost(int f)=0;
  virtual bool fixedP(void){return true;} // pmedian-> true ; facloc->false
  virtual void outputParameters(std::ostream &ostr)const
    {
      std::cout << "Number of customers=" << getN() << std::endl;
      std::cout << "Number of facilities=" << getM() << std::endl;
      std::cout << "Number of centers=" << getP() << std::endl;
      std::cout << "Ratio of centers to facilities=" << std::setprecision(4) << double(getP())/double(getM()) << std::endl;
    }

};

#endif
