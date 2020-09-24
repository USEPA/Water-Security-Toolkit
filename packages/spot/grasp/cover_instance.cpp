// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include "cover_instance.h"

/*------------
  | destructor
  *-----------*/

PMCoverInstance::~PMCoverInstance() {
  if (covered!=0) {
    for (int i=1; i<=nelements; i++) {delete [] covered[i];}
    delete [] covered;
  }
  if (efactor!=0) delete [] efactor;
  if (sfactor!=0) delete [] sfactor;
  if (templist!=0) delete [] templist;
  if (weight!=0) delete [] weight;
  if (opencost!=0) delete [] opencost;
}


/*-------------
  | constructor 
  *------------*/

PMCoverInstance::PMCoverInstance(bool _use_special) {
  maxserve = 0;
  covered = 0;
  templist = 0;
  weight = 0;
  efactor = 0;
  sfactor = 0;
  opencost = 0;
  use_special = _use_special;
}


/*-------------------------------------------------
  | get list of all facilities whose distance to 
  | user i is smaller than v (sorted by label)
  *------------------------------------------------*/

IntDouble *PMCoverInstance::getCloser (int i, double v) {
  if (i==special_user) { //special user: distance zero to the special_facility, infinity to everybody else
    if (v<=0) {
      templist[0].id = 0;
    } else if (v<=infinity) {
      templist[0].id = 1;                //size: one
      templist[1].id = special_facility; //just the special facility
      templist[1].value = 0.0;	       //its value is zero
    } else {
      templist[0].id = nsets + 1;
      for (int k=nsets; k>=1; k--) {
	templist[k].id = k;
	templist[k].value = infinity;
      }
      templist[nsets+1].id = nsets+1;
      templist[nsets+1].value = 0;
    }
  } else { //normal user: standard distances to everybody that covers it, infinity to everybody else
    double ef = efactor[i];    //factor associated with the element
    int size  = covered[i][0]; //number of sets covering the element
		
    if (v > infinity) { //infinity: return everybody
      int nfac = nsets + ((use_special) ? 1 : 0); 

      //include all facilities in the list
      templist[0].id = nfac; //total number of facilities
      for (int f=nfac; f>0; f--) {
	templist[f].id = f;           //set id
	templist[f].value = infinity; //set value (may be updated below)
      }

      //update values for facilities that actually cover the element
      for (int k=size; k>1; k--) { 
	int f = covered[i][k];      //facility label   
	double w = sfactor[f] * ef; //weight
	templist[f].value = w;      //total value
      }

      if (use_special) templist[nsets+1].value = weight[i];

    } else { //normal user, non-infinity value
      templist[0].id = 0;       //start with an empty list
      for (int k=1; k<=size; k++) {    //create a list with all elements covered
	int f = covered[i][k];       //current facilty 
	double w = sfactor[f] * ef;  //service cost: set factor times element factor

	if (w < v) { //add to our list if we have time
	  templist[++templist[0].id].id = f;  //increase answer size, add element
	  templist[templist[0].id].value = w; //set its value
	}
      }

      //we may need to add the special facility
      if (use_special && v>weight[i]) {
	templist[++templist[0].id].id = special_facility;
	templist[templist[0].id].value = weight[i];
      }
    }
  }
  return templist;
}


/*-----------------
  | read input file
  *----------------*/

void PMCoverInstance::readCover (FILE *file, int _p) {
  fprintf (stderr, "Reading cover instance... ");

  //auxiliary variables to help in reading
  const int LINESIZE = 256;
  int line = 0;
  int nz = 0;
  char buffer [LINESIZE + 1];
  bool ok;
  int target = 1;

  p = _p;
	
  double maxweight = -1; //maximum weight of a single element

  /*------------------------------------
    | main loop: read one line at a time
    *-----------------------------------*/
  while (fgets (buffer, LINESIZE, file)) {
    line ++;
    if (line>=target) {
      target *= 2;
      fprintf (stderr, "Line %d read...\n", line);
    }
    int e, c, s;
    double f, w, o;
		
    switch (buffer[0]) {
			
      /*---
	| problem size
	*--*/
    case 'p': 
      if (sscanf (buffer, "p %d %d\n", &nelements, &nsets)==2) {
	//initialize all vectors
	covered  = new int   *[nelements+1];   
	weight   = new double [nelements+1];
	efactor  = new double [nelements+1];
	sfactor  = new double [nsets+1];
	opencost = new double [nsets+1]; 
	templist = new IntDouble [nsets + 2];
	for (e=1; e<=nelements; e++) {
	  efactor[e] = 0; //just in case
	  covered[e] = 0;
	}
	for (s=1; s<=nsets; s++) {sfactor[s] = 0;}
      } else {
	fprintf (stderr, "Error reading line %d: wrong format for 'p' line.\n", line);
	exit (-1);
      }
      break;

      /*-----
	| element description
	*----*/
    case 'e':
      //the fourth argument (the factor) may or may not be present
      //if not present, will be zero
      ok = (sscanf (buffer, "e %d %lg %d %lg", &e, &w, &c, &f) == 4);
      if (!ok) {f = 0; ok = (sscanf (buffer, "e %d %lg %d", &e, &w, &c)==3);}

      //Now we have the element number (e), its weight (w), the number
      //of sets that cover it (c), and the factor (f).
      if (ok) {
	//is element number within range?
	if (e<1 || e>nelements) {
	  fprintf (stderr, "Error reading line %d: element label (%d) out of range (%d:%d).\n", line, e, 1, nelements);
	  exit (-1);
	}

	//if necessary, initialize list of covering sets
	if (covered[e]==0) {
	  covered[e] = new int [c+1]; //list of sets that cover e
	  covered[e][0] = 0;          //start with no set; will increase as appropriate 
	  weight[e] = w;              //the weight we already know
	  if (w>maxweight) {maxweight = w;} //update maxweight
	} else {
	  fprintf (stderr, "WARNING: element %d is defined more than once (line %d).\n", e, line);
	}
	efactor[e] = f; //set factor
      } else {
	fprintf (stderr, "Error reading line %d: wrong format in 'e' line.\n", line);
	exit (-1);
      }
      break;

      /*---
	| set description
	*---*/
    case 's': //set
      // Again, the fourth argument (factor) may or may not be present.
      // If not present, the default value will be zero.
      ok = (sscanf (buffer, "s %d %d %lg %lg", &s, &c, &o, &f) == 4);
      if (!ok) {
	f = 0;
	ok = (sscanf (buffer, "s %d %d %lg", &s, &c, &o)==3);
      }
				
      if (ok) {
	sfactor[s] = f;
	opencost[s] = o;
      } else {
	fprintf (stderr, "Error reading line %d: wrong format in 's' line.\n", line);
	exit (-1);
      }
      break;

    case 'c': //pair (element, sets)
      if (sscanf (buffer, "c %d %d", &e, &s)==2) {
	if (e<1 || e>nelements) {
	  fprintf (stderr, "Error reading line %d: element label (%d) out of range (%d:%d).\n", line, e, 1, nelements);
	  exit (-1);
	}

	if (s<1 || s>nsets) {
	  fprintf (stderr, "Error reading line %d: set label (%d) out of range (%d:%d).\n", line, s, 1, nsets);
	  exit (-1);
	}
	nz++;
	covered[e][++covered[e][0]] = s; //add one more element to the list
      } else {
	fprintf (stderr, "Error reading line %d: wrong format in 'c' line.\n", line);
	exit (-1);
      }

      /*
	if (curelement==0) {
	int e;
	if (sscanf (buffer, "%d %d", &e, &togo) == 2) {
	curelement = e;
	covered[curelement] = new int [togo+1];
	covered[curelement][0] = togo;
	}
	} else {
	int s;
	if (sscanf (buffer, "%d", &s) == 1) {
	covered[curelement][togo--] = s;
	if (togo==0) curelement = 0;
	}
	}
      */
      break;
    }

  }
	
  //sort 'covered' arrays
  target = 1;

  maxserve = 0; //maximum total cost of serving an element
  for (int e=1; e<=nelements; e++) {
    if (e>=target) {
      target *=2; 
      fprintf (stderr, "%d adjacency lists sorted...\n", e);
    }
    if (covered[e]==0) {
      fprintf (stderr, "WARNING: no set covers element %d.\n", e);
      covered[e] = new int [1];
      covered[e][0] = 0;
    }
    int size = covered[e][0];
    if (size>0) sort (&covered[e][1], &covered[e][size]);

    //compute update maxweight
    double ef = efactor[e]; //this is fixed
    for (int k=covered[e][0]; k>=1; k--) {
      int f = covered[e][k];                  //get facility
      double serve = sfactor[f] * ef;         //factor associated with the pair
      if (serve > maxserve) maxserve = serve; //if big enough, update max
    }
  }

  fprintf (stderr, "done.\n");	
	
  //define special values

  maxweight += maxserve;

  infinity = maxweight*nsets + 1;
  special_facility = nsets+1;
  special_user     = nelements+1;

  std::cout << "setinfinity " << std::setprecision(5) << infinity << std::endl;
  std::cout << "setmaxweight " << std::setprecision(5) << maxweight << std::endl;
  std::cout << "setnz " << nz << std::endl;
  std::cout << "rsetnz " << std::setprecision(6) << (double)nz / (double)(nsets * nelements) << std::endl;
}


