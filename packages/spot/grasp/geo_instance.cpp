// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/**************************************************************
 * class PMGeoInstance: 
 * author: Renato Werneck (rwerneck@princeton.edu)
 * log: 
 *      May 29, 2002: file created
 *      June 12, 2002: lots of updates, added support for the
 *                     case where n and m are different
 * 
 **************************************************************/

#include "distance.h"
#include "bossa_timer.h"
#include <string.h>
#include "geo_instance.h"

#include <iostream>
#include <iomanip>

/******************************************
 *
 * constructor: produces an empty instance
 * 
 ******************************************/

PMGeoInstance::PMGeoInstance() {reset();}

void PMGeoInstance::reset() {
  oracle = 0;   
  n = p = m = 0;  //cities, facilities, potential facilites: all zero
  flatitude = flongitude = clatitude = clongitude = 0; //facility and customer coordinates
}



/************************************************
 * 
 * fatal: prints error message and exits program
 *
 ************************************************/

void PMGeoInstance::fatal (const char *func, const char *msg) 
{
  std::cerr << "PMGeoInstance::" << func << ": " << msg << std::endl;
  exit(-1);
}


/*****************************************************
 * 
 * readGEO: reads an instance in GEO format
 *
 *****************************************************/

void PMGeoInstance::readGEO (FILE *input, int _p) 
{
  std::cerr << "Reading GEO instance..." << std::endl;

  int v, ad, am, as, od, om, os;
  char ew, ns;

  bool size_read = false;
  const int LINESIZE = 256;
  char buffer [LINESIZE + 1];
  p = _p;
  //if (p<=0) fatal ("readGEO", "invalid number of facilities");

  while (fgets (buffer, LINESIZE, input)) {
    if (sscanf (buffer, "p %d %d\n", &n, &m) == 2) {
      size_read = true;
      clatitude  = new double [n+1]; 
      clongitude = new double [n+1];
      flatitude  = new double [m+1];
      flongitude = new double [m+1];
      continue;
    }
    if (sscanf (buffer, "f %d %d %d %d %c %d %d %d %c", &v, &ad, &am, &as, &ns, &od, &om, &os, &ew) == 9) {
      if (!size_read) fatal ("readGEO", "header missing");

      flongitude[v] = RFW::dms2rad (od, om, os);
      if (ew=='W' || ew=='w') {flongitude[v] *= -1.0;} //west is negative

      flatitude [v] = RFW::dms2rad (ad, am, as);
      if (ns=='S' || ns=='s') {flatitude[v] *= -1.0;} //south is negative

      continue;
    }
    if (sscanf (buffer, "c %d %d %d %d %c %d %d %d %c", &v, &ad, &am, &as, &ns, &od, &om, &os, &ew) == 9) {
      if (!size_read) fatal ("readGEO", "header missing");
      clongitude[v] = RFW::dms2rad (od, om, os);
      if (ew=='W' || ew=='w') clongitude[v] *= -1.0; //west is negative
      clatitude [v] = RFW::dms2rad (ad, am, as);
      if (ns=='S' || ns=='s') clatitude[v] *= -1.0; //south is negative
      continue;
    }
  }
	
  std::cerr << "Done" << std::endl;
  initOracle();	
}	


/***********************************************
 *
 * destructor: deallocates vectors and arrays
 *
 ***********************************************/

PMGeoInstance::~PMGeoInstance() {
  if (flongitude!=0) delete [] flongitude;
  if (flatitude !=0) delete [] flatitude;
  if (clongitude!=0) delete [] clongitude;
  if (clatitude !=0) delete [] clatitude;
  delete oracle;
}



