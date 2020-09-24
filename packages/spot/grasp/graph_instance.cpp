// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/**************************************************************
 * 
 * class PMGraphInstance: 
 * 
 **************************************************************/

#include "basics.h"
#include "distance.h"
#include "bossa_timer.h"
#include "graph_instance.h"
#include <string.h>

/******************************************
 *
 * constructor: produces an empty instance
 * 
 ******************************************/

PMGraphInstance::PMGraphInstance():PMMatrixInstance() {
  floyd_time = 0.0;
  graph = 0;
}


/**************************************************************
 *
 * floyd(): Floyd's algorithm to calculate all distances
 *          assumes array already initialized with infinity in
 *          all positions for which the distance is unknown;
 * this function assumes that n=m (all cities are potential
 * facilities)
 *
 **************************************************************/

void PMGraphInstance::floyd() {
  BossaTimer t;
  t.start();

  double ne  = (double) graph->getNEdges();
  double lne = log (ne) / LN2;
  double nv  = (double) n;


  bool dense = (nv * ne * lne * 10 > ne * ne * ne); 
  //fprintf (stderr, "%f %f %f %d\n", nv, ne, lne, dense);

  if (dense) {
    int i, j, k;
    for (i=1; i<=n; i++) {
      for (j=1; j<=n; j++) {
	for (k=1; k<=n; k++) {
	  double sum = d[j][i] + d[i][k];
	  if (sum < d[j][k]) {	
	    d[j][k] = sum;
	  }
	}
      }
    }
  } else {
    for (int i=1; i<=n; i++) graph->dijkstra (i, d[i]);
  }

  floyd_time += t.getTime();
  std::cout << "dense " << dense << std::endl;
  std::cout << "floydtime " << std::setprecision(2) << floyd_time << std::endl;
}


/************************************************
 * 
 * fatal: prints error message and exits program
 *
 ************************************************/

void PMGraphInstance::fatal (const char *func, const char *msg) {
  fprintf (stderr, "PMGraphInstance::%s: %s.\n", func, msg);
  exit(-1);
}

/*******************************************************
 * 
 * readDimacs: reads file in Dimacs format and executes
 *             Floyd's algorithm to compute the full
 *             distance matrix
 *
 *******************************************************/

void PMGraphInstance::readDimacs (FILE *input, int _p) {
  int temp_p, v1, v2, i;
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
      if (size_read) fatal ("readDimacs", "duplicate header");
      if (sscanf (buffer, "p %d %d %d", &n, &e_announced, &temp_p) != 3) 
	fatal ("readDimacs", "syntax error in 'p' line");
      size_read = true;		
      m = n;
      //allocate and initialize distance matrix (n+1 lines, m+1 columns)
      d = new double* [n+1]; 
      for (i=0; i<=n; i++) d[i] = new double[m+1];
      graph = new PMGraph (n, e_announced);
      resetDistances();			

      if (p<=0) p = temp_p;
      break;
				
    case 'e':
      if (!size_read) fatal ("readDimacs", "header missing");
      if (sscanf(buffer,"e %d %d %lg",&v1,&v2,&dist)==3) {	
	graph->addEdge (v1, v2, dist);
	d[v1][v2] = dist;
	d[v2][v1] = dist;
	e_count ++;
      } else fatal ("readDimacs", "syntax error in 'e' line");
    }
  }	

  if (e_announced!=e_count) fatal ("readDimacs", "invalid number of edges");
	
  floyd(); //complete distance matrix
  initOracle();
}




/***********************************************
 *
 * destructor: deallocates vectors and arrays
 *
 ***********************************************/
PMGraphInstance::~PMGraphInstance() {delete graph;};

