// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/**************************************************************
 *
 * class PMEuclideanInstance: 
 * 
 **************************************************************/

#include "distance.h"
#include "euclidean_instance.h"
#include "bossa_timer.h"
#include <string.h>

/******************************************
 *
 * constructor: produces an empty instance
 * 
 ******************************************/

PMEuclideanInstance::PMEuclideanInstance() {reset();}


/************************************************
 *
 * set to zero all data structures and variables
 *
 ************************************************/

void PMEuclideanInstance::reset() {
  oracle = 0;   
  n = p = m = 0;  //cities, facilities, potential facilites: all zero
  cx = cy = 0; //customer coordinates
  fx = fy = 0; //facility coordinates
}


/*****************************************
 *
 * constructor: produces a restricted 
 * version of an existing instance, with
 * only some of the facilities and some
 * of the customers
 *
 *****************************************/

/*
  PMEuclideanInstance::PMEuclideanInstance (PMInstance *original, int *of, int *oc) {
  fatal ("PMEuclideanInstance", "copy constructor not implemented");
	
  reset();
  metric = MATRIX;

  n = oc[0]; //number of customers in this solution
  m = of[0]; //number of facilities in this solution
  p = 0;     //just guessing some value
  int i;
	
  d = new double * [n+1];
  for (i=0; i<=n; i++) {
  d[i] = new double [m+1];
  d[i][0] = POPSTAR_INFINITY;
  }
  for (i=1; i<=m; i++) d[0][i] = POPSTAR_INFINITY; 
	
  //---------------
  // set distances
  //---------------
  int c0, f0, c, f;
  for (c=1; c<=n; c++) {
  c0 = oc[c]; 
  for (f=1; f<=m; f++) {
  f0 = of[f];
  d[c][f] = original->getDist(c0,f0);
  }
  }

  initOracle();
  }
*/



/************************************************
 * 
 * fatal: prints error message and exits program
 *
 ************************************************/

void PMEuclideanInstance::fatal (const char *func, const char *msg) {
  fprintf (stderr, "PMEuclideanInstance::%s: %s.\n", func, msg);
  exit(-1);
}


/*****************************************************
 * 
 * readPMI: reads an instance in PMI format
 *
 *****************************************************/

void PMEuclideanInstance::readPMI (FILE *input, int _p) {
  fprintf (stderr, "Reading PMI instance... ");
	
  int v;
  double vx, vy;
  bool size_read = false;
  const int LINESIZE = 256;
  char buffer [LINESIZE + 1];

  p = _p;

  while (fgets (buffer, LINESIZE, input)) {
    if (sscanf (buffer, "p %d %d\n", &n, &m) == 2) {
      size_read = true;

      //initialize coordinate arrays
      cx = new double [n+1]; 
      cy = new double [n+1];
      fx = new double [m+1];
      fy = new double [m+1];

      //distance between zero things should be infinity... sort of
      fx[0] = -POPSTAR_INFINITY;
      fy[0] = -POPSTAR_INFINITY;			
      cx[0] = -POPSTAR_INFINITY;
      cy[0] = -POPSTAR_INFINITY;

      continue;
    }
    if (sscanf (buffer, "f %d %lg %lg", &v, &vx, &vy) == 3) {
      if (!size_read) fatal ("readPMI", "header missing");
      fx[v] = vx;
      fy[v] = vy;
      continue;
    }
    if (sscanf (buffer, "c %d %lg %lg", &v, &vx, &vy) == 3) {
      if (!size_read) fatal ("readPMI", "header missing");
      cx[v] = vx;
      cy[v] = vy;
      continue;
    }
  }
	
  fprintf (stderr, "done.\n");
  initOracle();
}	


/*----------------------------------------------------
  | readTSP: reads an instance in TSP format - assumes
  | all cities are potential facilities
  *---------------------------------------------------*/

void PMEuclideanInstance::readTSP (FILE *input, int _p) {
  fprintf (stderr, "Reading TSP instance... ");
	
  int v;
  double vx, vy;
  bool size_read = false;
  const int LINESIZE = 256;
  char buffer [LINESIZE + 1];
  p = _p;

  //if (p <= 0) {
  //	fatal ("readTSP", "number of facilities to open (p) not defined.");
  //}

  while (fgets (buffer, LINESIZE, input)) {
    if (sscanf (buffer, "DIMENSION : %d", &n) == 1) {
      size_read = true;
      m = n;

      //initialize coordinate arrays
      cx = new double [n+1]; 
      cy = new double [n+1];
      fx = new double [m+1];
      fy = new double [m+1];	
			
      cx[0] = -POPSTAR_INFINITY;
      cy[0] = -POPSTAR_INFINITY;			
      fy[0] = -POPSTAR_INFINITY;
      fx[0] = -POPSTAR_INFINITY;

      continue;
    }
    if (sscanf (buffer, "%d %lg %lg", &v, &vx, &vy) == 3) {
      if (!size_read) fatal ("readTSP", "header missing");
      cx[v] = fx[v] = vx;
      cy[v] = fy[v] = vy;
      continue;
    }
  }
	
  fprintf (stderr, "done.\n");
	
  initOracle();
}


/***********************************************
 *
 * destructor: deallocates vectors and arrays
 *
 ***********************************************/

PMEuclideanInstance::~PMEuclideanInstance() {
  if (fx!=0) delete [] fx;
  if (fy!=0) delete [] fy;
  if (cx!=0) delete [] cx;
  if (cy!=0) delete [] cy;
  delete oracle;
}

