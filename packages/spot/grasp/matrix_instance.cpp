// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/**************************************************************
 * class PMMatrixInstance: 
 **************************************************************/

#include "instance.h"
#include "matrix_instance.h"
#include "bossa_timer.h"
#include <string.h>

#include <iostream>
#include <iomanip>

/*------------------------------------------
  | constructor: produces an empty instance,
  | with p = 0
  *-----------------------------------------*/ 

PMMatrixInstance::PMMatrixInstance() {reset();}

void PMMatrixInstance::reset(void) 
{
  oracle_time = 0.0;
  oracle = 0;   
  d = 0;       //distance matrix
  n = p = m = 0;  //cities, facilities, potential facilites: all zero
  fc = 0;      //facility weights
}


/*-----------------------------------------------------------
  | constructor: produces a restricted version of an existing
  | instance, with only a subset of the facilities and of
  | the customers
  *----------------------------------------------------------*/

PMMatrixInstance::PMMatrixInstance (PMInstance *original, int *of, int *oc) 
{
  reset();

  n = oc[0]; //number of customers in this instance
  m = of[0]; //number of facilities in this instance
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

/*------------------------------------------
  | printMatrix: outputs the distance matrix
  | (even if there is no such thing)
  *-----------------------------------------*/

void PMMatrixInstance::printMatrix (FILE *file) {
  int c, f;
  for (c=1; c<=n; c++) {
    for (f=1; f<=m; f++) {
      double d = getDist(c,f);			
      if (d==POPSTAR_INFINITY) fprintf (file, "--- ");
      else fprintf (file, "%f ", d);
    }
    fprintf (file, "\n");
  }
}


/*-----------------------------------------------
  | fatal: prints error message and exits program
  *----------------------------------------------*/

void PMMatrixInstance::fatal (const char *func, const char *msg) 
{
  std::cerr << "PMMatrixInstance::" << func << ": " << msg << std::endl;
  exit(-1);
}


/*-------------------------------------------------
  | resetDistances: sets all distances to infinity
  |
  | except those from a city to itself (set to 0);
  | this routine assumes each city is a potential
  | facility
  *------------------------------------------------*/

void PMMatrixInstance::resetDistances () 
{	
  int f, u;

  //reset distances
  d[0][0] = POPSTAR_INFINITY;
  for (f=1; f<=m; f++) d[0][f] = POPSTAR_INFINITY * POPSTAR_INFINITY;
	
  for (u=1; u<=n; u++) {
    d[u][0] = POPSTAR_INFINITY * POPSTAR_INFINITY;
    for (f=1; f<=m; f++) d[u][f] = POPSTAR_INFINITY;	
  }

  // JPW - this doesn't make sense in the case of non-facility distances
  //	for (f=1; f<=m; f++) d[f][f] = 0; //distance to itself is zero
	
  //note that we can use the fact that d[0][x] = d[x][0] = POPSTAR_INFINITY
  //except for x = 0
}


/*******************************************************
 * 
 * read file in pmm format, a complete distance matrix
 *
 *******************************************************/

void PMMatrixInstance::readPMM (FILE *input, int _p) {
  int u, f, i;
  double dist;

  int e_announced = 0;      //number of edges announced
  int e_count = 0;          //number of edges read
  bool size_read = false;   //flag
  const int LINESIZE = 256; 
  char buffer [LINESIZE+1];

  p = _p;

  while (fgets (buffer, LINESIZE, input)) {
    switch (buffer[0]) {
    case 'p':
      if (size_read) fatal ("readPMM", "duplicate header");
      if (sscanf (buffer, "p %d %d", &n, &m) == 2) {
	size_read = true;		
	e_announced = m * n;
	d = new double* [n+1]; //allocate distance matrix
	for (i=0; i<=n; i++) d[i] = new double[m+1];
	resetDistances();
      } else fatal ("readPMM", "syntax error in 'p' line");
      break;
				
    case 'a':
      if (!size_read) fatal ("readPMM", "header missing");
      if (sscanf(buffer,"a %d %d %lg",&u,&f,&dist)==3) {	
	d[u][f] = dist;
	e_count ++;
      } else fatal ("readPMM", "syntax error in 'a' line");
      break;
    }
  }	

  //ensure consistence
  if(e_announced != e_count) 
    {
      std::cout << "WARNING: error in input file (" << e_announced << " edges announced, " << e_count << " read)" << std::endl;
    }
	
  initOracle();
}


/*******************************************************
 * 
 * read file in ufl format (complete distance matrix)
 *
 *******************************************************/

void PMMatrixInstance::readUFL (FILE *input) {
  int u, f, i;
  double dist, w;

  int e_announced = 0;      //number of edges announced
  int e_count = 0;          //number of edges read
  bool size_read = false;   //flag
  const int LINESIZE = 256; 
  char buffer [LINESIZE+1];

  p = -1; //this will be a facility location problem

  while (fgets (buffer, LINESIZE, input)) {
    switch (buffer[0]) {
    case 'p':
      if (size_read) fatal ("readUFL", "duplicate header");
      if (sscanf (buffer, "p %d %d", &n, &m) == 2) {
	size_read = true;		
	e_announced = m * n;
	d = new double* [n+1]; //allocate distance matrix
	for (i=0; i<=n; i++) d[i] = new double[m+1];
	fc = new double [m+1]; //allocated facility weights
	resetDistances();
      } else fatal ("readPMM", "syntax error in 'p' line");
      break;
				
    case 'a':
      if (!size_read) fatal ("readUFL", "header missing");
      if (sscanf(buffer,"a %d %d %lg",&u,&f,&dist)==3) {	
	d[u][f] = dist;
	e_count ++;
      } else fatal ("readUFL", "syntax error in 'a' line");
      break;

    case 'f':
      if (!size_read) fatal ("readUFL", "header missing");
      if (sscanf(buffer,"f %d %lg",&f,&w)==2) {fc[f] = w;} 
      else fatal ("readUFL", "syntax error in 'a' line");
      break;
		
    }
  }	

  //ensure consistence
  if (e_announced != e_count) {
    fprintf (stderr, "WARNING: error in input file (%d edges announced, %d read)\n", 
	     e_announced, e_count);
  }
	
  initOracle();
}


/*******************************************
 *
 * destructor: deallocates distance matrix,
 * gets rid of distance oracle
 *
 *******************************************/

PMMatrixInstance::~PMMatrixInstance() {
  if (d!=0) {
    for (int i=0; i<=n; i++) delete [] d[i];
    delete [] d;
  }
  if (fc!=0) delete [] fc;
  delete oracle;
}
