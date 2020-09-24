/*  _________________________________________________________________________
 *
 *  Acro: A Common Repository for Optimizers
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the CPL License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top Acro directory.
 *  _________________________________________________________________________
 */

// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This is an implementation of the Volume algorithm for uncapacitated
// facility location problems. See the references
// F. Barahona, R. Anbil, "The Volume algorithm: producing primal solutions
// with a subgradient method," IBM report RC 21103, 1998.
// F. Barahona, F. Chudak, "Solving large scale uncapacitated facility
// location problems," IBM report RC 21515, 1999.

// Significant modifications and additions are Copyright (C) 2006, 2007,
// Sandia Corporation. 
//
// Sparse version by Erik Boman and Jon Berry, Oct 2006
// p-median option added by Erik Boman, Oct 2006.
// Optimized sparse matrix handling by Jon Berry.
// Side constraints by Cindy Phillips and Erik Boman, April 2007.

//#include <valarray>
#include <cstdio>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <cmath>
#include <cassert>
#include <math.h>
#include <limits.h>
#ifndef _WIN32
#include <sys/times.h>
#endif

#include <iostream>
#include <sundry/ufl.h>
#include <sundry/SNLRound.h>

using namespace std;

int newDummyID;
map<int, int> newID;
map<int, int>::iterator newIDiter;
vector<int> origID;
double gap;        // e.g. 0.1 --> stop when within 10% of optimal

// function prototypes
void  UFL_read_data(const char* fname, UFL& data, SparseMatrixCSR<double>& Matrix, const double scale_factor);
char *UFL_preprocess_data(const char* fname,  int num_facilities);
void UFL_output(std::string &outputFileName, double upper, double lower, VOL_ivector &locations, int nlocs, int ncust);
double evaluate_sc(SparseMatrixCSR<double>& sc, const VOL_ivector& x);

// ******************************* PMrand: ParkMiller from utilib ****
/** */
#define MPLIER 16807
/** */
#define MODLUS 2147483647
/** */
#define MOBYMP 127773
/** */
#define MOMDMP 2836

/**
 * This function returns a pseudo-random number for each invocation.
 * This C implementation, is adapted from a FORTRAN 77 adaptation of 
 * the "Integer Version 2" minimal standard number generator whose 
 * Pascal code described by 
 * \if GeneratingLaTeX Park and Miller~\cite{ParMil88}. \endif
 * \if GeneratingHTML [\ref ParMil88 "ParMil88"]. \endif
 *
 * This code is portable to any machine that has a
 * maximum integer greater than, or equal to, 2**31-1.  Thus, this code
 * should run on any 32-bit machine.  The code meets all of the
 * requirements of the "minimal standard" as described by
 * \if GeneratingLaTeX Park and Miller~\cite{ParMil88}. \endif
 * \if GeneratingHTML [\ref ParMil88 "ParMil88"]. \endif
 * Park and Miller note that the execution times for running the portable 
 * code were 2 to 50 times slower than the random number generator supplied 
 * by the vendor. 
 */
int PMrand(int* state)
{
register int hvlue, lvlue, testv;
int nextn = *state;

hvlue = nextn / MOBYMP;
lvlue = nextn % MOBYMP;                               /* nextn MOD MOBYMP */
testv = MPLIER*lvlue - MOMDMP*hvlue;
if (testv > 0) 
   nextn = testv;
else
   nextn = testv + MODLUS;
*state = nextn;
return nextn;
}
// ******************************* PMrand: ParkMiller from utilib ****

int main(int argc, char* argv[]) {

   //UFL_parms ufl_par("ufl.par"); // for debugging
   UFL_parms ufl_par;   // SPOT - omit parameter file
   UFL  ufl_data;
   int num_sidecon = 0; // Default is no side constraints
   gap = 1e-7;                // Default gap (must be global)

   const int scale_sidecon = 1; // Default is to scale side constraiints

   // initialize  
   ufl_data.nloc = ufl_data.ncust = ufl_data.nnz = 0;

   // SPOT - assume we have "dummy" location 
   ufl_data.have_dummy = true;

   // SPOT - allow side constraints

   if (argc < 3){
     printf("Usage: %s <objective data file> <p (#open facilities)> [gap (0<=gap<=1)] [side_constraint_data_file  upper_bound]* \n",argv[0]);
     exit(1);
   }

   // Save p  (p=0 for UFL, p>0 for p-median)
   ufl_data._p = atoi(argv[2]);

   // Check for invalid p
   if (ufl_data._p < 0)
     return 0;

   // Read data for objective (cost) function 
   //UFL_read_data(argv[1], ufl_data);
   char *tfilename = UFL_preprocess_data(argv[1], ufl_data._p);
   UFL_read_data(tfilename, ufl_data, ufl_data._dist, 1.0);
   free(tfilename);

   // Initialize integer solution to all zeros. 
   ufl_data.ix.allocate(ufl_data.nloc);
   ufl_data.ix = 0;

   // The gap is an optional argument.
   if (argc > 3)
     gap = atof(argv[3]);
   // Count # optional side constraints.
   if (argc > 4)
     num_sidecon = (argc-4)/2;
   bool json_file = num_sidecon*2 < (argc-4);
   if (json_file)
        ufl_par.int_savefile = argv[argc-1];

   if (ufl_data.nloc <= ufl_data._p-1)
     {
       int nloc = ufl_data.nloc;
       int ncust = ufl_data.ncust;
       int j;
       // Trivial solution.

       if (ufl_par.dual_savefile.length() > 0) {
         printf("Trivial solution (use all relevant locations).  No dual output implemented for this case.\n");
       }

       // We can select all relevant locations.  Compute the value of
       // the trivial solution.  This code was stolen from the
       // heuristics method and simplified

       double *xmin = new double[ncust]; // The minimum cost for servicing each customer
       for ( j=0; j < ncust; ++j){ 
         xmin[j]= 1.e31; // huge 
       }

   // Sparse code loops over rows (locations) first.
       SparseMatrixCSR<double>& _dist = ufl_data._dist;
       int rows = _dist.MatrixRows();
       for (int i=0; i<rows; i++) {
         double * begin_vals = _dist.col_values_begin(i);
         int    * begin_cols = _dist.col_indices_begin(i);
         double * end_vals = _dist.col_values_end(i);
         int    * end_cols = _dist.col_indices_end(i);
         double *ptr1 = begin_vals; 
         int    *ptr2 = begin_cols;
         for (; ptr1 < end_vals; ptr1++, ptr2++) {
           int j = *ptr2;
           double distij = *ptr1;
           if (distij < xmin[j]) 
             xmin[j]= distij;
         }
       }

   // Now xmin tells us how to assign customers
       double trivialCost = 0;
       for ( j=0; j < ncust; ++j){ 
         trivialCost += xmin[j];
       }

       VOL_ivector trivialSolution(nloc - 1); // Assume one location is dummy
       j = 0;
       for (int i = 0; i < nloc; i++)
         if (i != newDummyID)
           trivialSolution[j++] = origID[i];
       UFL_output(ufl_par.int_savefile, trivialCost, trivialCost, trivialSolution, nloc-1, ncust);
       printf(" Total Time: 0 secs.  No call to Vol.\n");
       return 0;
     }
   // Read data for side constraints

   //ufl_par.h_iter *= (int) pow(2.0,num_sidecon*5);
   int i;
   int arg = 4;
   SparseMatrixCSR<double> newsidecon;
   for (i=0; i<num_sidecon; i++){
     tfilename = UFL_preprocess_data(argv[arg++], ufl_data._p);
     double ub = atof(argv[arg++]);
     if (scale_sidecon)
       UFL_read_data(tfilename, ufl_data, newsidecon, 1.0/ub);
     else
       UFL_read_data(tfilename, ufl_data, newsidecon, 1.0);
     free(tfilename);
     ufl_data._sidecon.push_back(newsidecon);
     if (scale_sidecon)
       ufl_data._ub.push_back(1.0); // Scale rhs to be 1.0
     else
       ufl_data._ub.push_back(ub); 
   }

   // We've read in all the files now.
      newID.erase(newID.begin(), newID.end());
   
   // create the VOL_problem from the parameter file
   //VOL_problem volp("ufl.par"); // for debugging
   VOL_problem volp;    // SPOT - omit parameter file
   // volp.psize = ufl_data.nloc + ufl_data.nloc*ufl_data.ncust;
   volp.psize = ufl_data.nloc + ufl_data.nnz;
   volp.dsize = ufl_data.ncust + num_sidecon;

   // If side constraints, run heuristics very seldom
   if (num_sidecon)
       volp.parm.heurinvl = 1000;

   bool ifdual = false;
   if (ufl_par.dualfile.length() > 0) {
     // read dual solution
      ifdual = true;
      VOL_dvector& dinit = volp.dsol;
      dinit.allocate(volp.dsize);
      // read from file
      FILE * file = fopen(ufl_par.dualfile.c_str(), "r");
      if (!file) {
         printf("Failure to open file: %s\n ", ufl_par.dualfile.c_str());
         abort();
      }
      const int dsize = volp.dsize;
      int idummy;
      for (int i = 0; i < dsize; ++i) {
         fscanf(file, "%i%lf", &idummy, &dinit[i]);
      }
      fclose(file);
   }

   // We set dual bounds for side constraints. Assume primal <= constraints.

   // This would be the right place to set bounds on the dual variables
   // For UFL all the relaxed constraints are equalities, so the bounds are 
   // -/+inf, which happens to be the Volume default, so we don't have to do 
   // anything.
   // Otherwise the code to change the bounds would look something like this:

   // first the lower bounds to -inf, upper bounds to inf
   volp.dual_lb.allocate(volp.dsize);
   volp.dual_lb = -1e31;
   volp.dual_ub.allocate(volp.dsize);
   volp.dual_ub = 1e31;
   // now go through the relaxed constraints and change the lb of the ax >= b 
   // constrains to 0, and change the ub of the ax <= b constrains to 0.
   for (i = ufl_data.ncust; i < volp.dsize; ++i) {
     volp.dual_lb[i] = 0;  // Inequalities: Force non-negative dual variables.
                           // This is opposite of what the instructions say,
                           // but Jon & Cindy think it's correct (5/11/07).
   //   if ("constraint i is '<=' ") {
   //     volp.dual_ub[i] = 0;
   //   }
   //   if ("constraint i is '>=' ") {
   //     volp.dual_lb[i] = 0;
   //   }
   }

#ifndef _WIN32
   // start time measurement
   double t0;
   struct tms timearr; clock_t tres;
   tres = times(&timearr); 
   t0 = timearr.tms_utime; 
#endif

   // invoke volume algorithm
   if (volp.solve(ufl_data, ifdual) < 0) {
      printf("solve failed...\n");
   } else {
      // recompute the violation
      const int n = ufl_data.nloc;
      const int m = ufl_data.ncust;

      VOL_dvector v(volp.dsize);
      const VOL_dvector& psol = volp.psol;
      v = 1;

/***  JWB
      int i,j,k=n;
      for (j = 0; j < n; ++j){
        for (i = 0; i < m; ++i) {
          v[i] -= psol[k];
          ++k;
        }
      }
***/

   int k=0;
   SparseMatrixCSR<double>& _dist = ufl_data._dist;
   int rows = _dist.MatrixRows();
   for (int i=0; i<rows; i++) {
     double * begin_vals = _dist.col_values_begin(i);
     int    * begin_cols = _dist.col_indices_begin(i);
     double * end_vals = _dist.col_values_end(i);
     int    * end_cols = _dist.col_indices_end(i);
     double *ptr1 = begin_vals; 
     int    *ptr2 = begin_cols;
     for (; ptr1 < end_vals; ptr1++, ptr2++) {
       int j = *ptr2;
       v[j]-=psol[n+k];
       ++k;
     }
   }

   // Violation of relaxed LP solution is not interesting
      //double vc = 0.0;
      //for (int i = 0; i < m; ++i)
         // MS Visual C++ has difficulty compiling 
         //   vc += std::abs(v[i]);
         // so just use standard fabs.
         //vc += fabs(v[i]);
      //vc /= m;
      //      printf(" Average UFL (p-median) violation of final solution: %f\n", vc);

      if (ufl_par.dual_savefile.length() > 0) {
        // save dual solution
         FILE* file = fopen(ufl_par.dual_savefile.c_str(), "w");
         const VOL_dvector& u = volp.dsol;
         int n = u.size();
         int i;
         for (i = 0; i < n; ++i) {
            fprintf(file, "%8i  %f\n", i+1, u[i]);
         }
         fclose(file);
      }

      // run a few more heuristics
      double heur_val =  DBL_MAX;
      for (int i = 0; i < ufl_par.h_iter; ++i) {
         ufl_data.heuristics(volp, psol, heur_val, 0.0);
      }
      // SPOT: Need to omit the dummy location in the list, and print
      // IDs that match the input

      int numFacilities = ufl_data._p - 1;  // Don't output the dummy
      VOL_ivector solution(numFacilities);
      const VOL_ivector& x = ufl_data.ix;
      int j = 0;
      for (int i = 0; i < n; ++i) {
        if ( x[i]==1 && (i != newDummyID)){
/*
          if (j == numFacilities)
            {
              printf("FATAL SPOT ERROR: Lagrangian solver is nontrivially covering all events with p+1 sensors\n");
              exit(1);
            }
*/
          solution[j++] = origID[i];
        }
      }

      // SPOT:  Need to check upper and lower bounds.  Jon guessed.

      UFL_output(ufl_par.int_savefile, ufl_data.icost, volp.value, solution, j, m);
      for (int i=0; i<ufl_data._sidecon.size(); ++i){
        double slack = ufl_data._ub[i] - evaluate_sc(ufl_data._sidecon[i], x);
        printf(" Side constraint (scaled) slack or violation: %f\n", slack); 
      }
   }

#ifndef _WIN32
   // end time measurement
   tres = times(&timearr);
   double t = (timearr.tms_utime-t0)/100.;
   printf(" Total Time: %f secs\n", t);
#endif

   return 0;
}

//############################################################################

struct triple {
   int loc, cust;
   double cost;
};

int cmp_triples(const void *p1, const void* p2)
{
        struct triple *tp1 = (struct triple *) p1;
        struct triple *tp2 = (struct triple *) p2;
        if (tp1->loc < tp2->loc)
                return -1;
        else if (tp1->loc > tp2->loc)
                return 1;
        else 
                return tp1->cust - tp2->cust;
}

/* Preprocessing: Write to a temporary file to save memory.
   TODO: Make this feature optional. */ 
char *UFL_preprocess_data(const char* fname, int p) {

   FILE * file = fopen(fname, "r");
   if (!file) {
      printf("Failure to open ufl datafile: %s\n ", fname);
      abort();
   }
   int nloc, ncust, nnz;
   double cost;
   double *fcost;
   char s[500];
   fgets(s, 500, file);
   int len = strlen(s) - 1;
   if (s[len] == '\n')
      s[len] = 0;
   // read number of locations, number of customers, no. of nonzeros
   sscanf(s,"%d%d%d",&nloc,&ncust,&nnz);
   if (p==0) { // UFL 
     // read location costs
     fcost = (double*) malloc(sizeof(double)*nloc);
     for (int i = 0; i < nloc; ++i) { 
       fgets(s, 500, file);
       len = strlen(s) - 1;
       if (s[len] == '\n')
          s[len] = 0;
       sscanf(s,"%lf",&cost);
       fcost[i]=cost;
     }
   }
   struct triple *triples = (struct triple *) malloc(nnz*sizeof(struct triple));
   int count=0, loc, cust;
   for (int i=0; i<nnz; i++) {
       fgets(s, 500, file);
       len = strlen(s) - 1;
       if (s[len] == '\n')
                  s[len] = 0;
       int k = sscanf(s,"%d%d%lf",&loc,&cust,&cost);
       assert(k == 3);
       triples[count].loc = loc;
       triples[count].cust = cust;
       triples[count].cost = cost;
       count++;
   }
   assert(count == nnz);
   qsort(triples, nnz, sizeof(triple), cmp_triples);
   char *tmpfname = (char*) malloc(strlen(fname) + 5);
   strcpy(tmpfname, fname);
   strcat(tmpfname, ".tmp");
   FILE * tfile = fopen(tmpfname, "w");
   if (!tfile) {
      printf("Failure to open tmp ufl datafile: %s\n ", tmpfname);
      abort();
   }
   fprintf(tfile, "%d %d %d\n", nloc, ncust, nnz);
   if (p==0) { // UFL 
     fcost = (double*) malloc(sizeof(double)*nloc);
     for (int i = 0; i < nloc; ++i) { 
       fprintf(tfile, "%lf\n", fcost[i]);
     }
     free(fcost);
   }
   for (int i=0; i<nnz; i++) {
            fprintf(tfile, "%d %d %lf\n", triples[i].loc, triples[i].cust,
                                      triples[i].cost);
   }
   fclose(tfile);
   return tmpfname;
}

// Read cost objective data or side constraint data.
// Output: Sparse matrix, Matrix.
void UFL_read_data(const char* fname, UFL& data,
                   SparseMatrixCSR<double>& Matrix,
                   const double scale_factor) 
{
   FILE * file = fopen(fname, "r");
   if (!file) {
      printf("Failure to open ufl datafile: %s\n ", fname);
      abort();
   }


   VOL_dvector& fcost = data.fcost;
   //VOL_dvector& dist = data.dist;
   //SparseMatrixCSR<double>& _dist = data._dist;

   int& nloc = data.nloc;
   int& ncust = data.ncust;
   int& nnz = data.nnz;
   int& p = data._p;
   int len;
   int nloc2, ncust2, nnz2;
   bool firstRead = true;
#if 1
   char s[500];
   fgets(s, 500, file);
   len = strlen(s) - 1;
   if (s[len] == '\n')
      s[len] = 0;

   // read number of locations, number of customers, and no. of nonzeros
   sscanf(s,"%d%d%d",&nloc2,&ncust2,&nnz2);
   // check consistency, all constraints must have same sparsity pattern!
   // TODO: We only check nnz and ncust for now, since nloc may shrink after ID mapping.
   if (nloc==0)
     nloc = nloc2;
   else firstRead = false;
#if 0
   else if (nloc2 != nloc){
     // TODO Standardize error handling 
     printf("FATAL: nloc=%i is inconsistent with previous value %i\n",
       nloc2, nloc);
     exit(-1);
   }
#endif
   if (ncust==0)
     ncust = ncust2;
   else if (ncust2 != ncust){
     // TODO Standardize error handling 
     printf("FATAL: ncust=%i is inconsistent with previous value %i\n",
       ncust2, ncust);
     exit(-1);
   }
   if (nnz==0)
     nnz = nnz2;
   else if (nnz2 != nnz){
     // TODO Standardize error handling 
     printf("FATAL: nnz=%i is inconsistent with previous value %i\n",
       nnz2, nnz);
     exit(-1);
   }

   // This might be too long, since nloc is just the maximum node ID and some
   // might not be used, but that should not matter.
   int m_index_size = nloc + 2; // nloc+1 node IDs + entry for array size
   fcost.allocate(nloc+1);
   //dist.allocate(nloc*ncust);
   int    *m_index  =  (int*)    malloc(m_index_size * sizeof(int));
   double *m_values =  (double*) malloc(nnz    * sizeof(double));
   int    *m_columns = (int*)    malloc(nnz    * sizeof(int));
   double cost;
   int i,j,k;
   if (p==0) { // UFL 
     // read location costs 
     for (i = 0; i < nloc; ++i) { 
       fgets(s, 500, file);
       len = strlen(s) - 1;
       if (s[len] == '\n')
          s[len] = 0;
       sscanf(s,"%lf",&cost);
       fcost[i]=cost;
     }
   }
   else { // p-median
     // No facility open costs; set them to zero.
     for (i = 0; i <= nloc; ++i) 
       fcost[i]=0.0;
   }

   //dist=1.e7;
   nnz = 0;
   int nextID = 0;
   int newi = 0;
   char *fgr = fgets(s, 500, file);
   k=sscanf(s,"%d%d%lf",&i,&j,&cost);
   assert(k==3);
   int row = 0;
   int cur_loc = i;
   int next_loc = i;
   int nz_count = 0;
   m_index[row] = 0;
   while(fgr) {
     len = strlen(s) - 1;
     if (s[len] == '\n')
        s[len] = 0;
     if (next_loc != cur_loc) {
        row++;
        m_index[row] = nz_count;
        cur_loc = next_loc;
      }
     // read cost of serving a customer from a particular location
     // i=location, j=customer

     // SPOT:
     // Create a set of location IDs that is consecutive beginning at 0.
     // Customer (event) IDs are already consecutive beginning at 1.
     // Dummy location is "nloc".
     // "p" (number of sensors) was incremented by 1 because dummy
     // location will be one of the those found by ufl.
     
     newIDiter = newID.find(i);
     if (newIDiter == newID.end()){

       if (!firstRead)
         {
           printf("FATAL: Sparsity pattern does not match previous files.\n Node=%i is in file %s, but not in previous files\n",i, fname);
     exit(-1);

         }

       newi = newID[i] = nextID;

       origID.push_back(i);
       if (i == nloc2){         // sp sends dummy location as nloc
         newDummyID = nextID;
       }
       nextID++;
     }
     else{
       newi = newIDiter->second;
     }

     //dist[(i)*ncust + j-1]=cost;          original version
     //_dist[i][j-1] = cost;                map version

     m_values[nz_count] = scale_factor * cost; // scale all matrix entries
     m_columns[nz_count] = j-1;

     ++nnz;  
     nz_count++;

     fgr = fgets(s, 500, file);
     k=sscanf(s,"%d%d%lf",&i,&j,&cost);
     if(k!=3) break;
     if(i==-1)break;
     next_loc = i;
   }
   row++;
   m_index[row] = nz_count; 
   nloc = row;

   Matrix.init(nloc,ncust,nnz,m_index,m_values,m_columns);

// DEBUG
   for (int i=0; i<nloc; i++) {
     double * begin_vals = Matrix.col_values_begin(i);
     int    * begin_cols = Matrix.col_indices_begin(i);
     double * end_vals = Matrix.col_values_end(i);
     int    * end_cols = Matrix.col_indices_end(i);
     double *ptr1 = begin_vals; 
     int    *ptr2 = begin_cols;
     for (; ptr1 < end_vals; ptr1++, ptr2++) {
       int j = *ptr2;
       double distij = *ptr1;
     }
   }
// END DEBUG

#else // DEAD CODE!
   fscanf(file, "%i%i", &ncust, &nloc);
   fcost.allocate(nloc);
   //dist.allocate(nloc*ncust);
   int i,j;
   for ( j=0; j<ncust; ++j){
     for ( i=0; i<nloc; ++i){
       fscanf(file, "%f", &Matrix[i][j]);
     }
   }
   for ( i=0; i<nloc; ++i)
     fscanf(file, "%f", &fcost[i]);
#endif
   fclose(file);

   data.fix.allocate(nloc);
   data.fix = -1;
   //if (!verifyMap(dist,ncust,Matrix))  
//        fprintf(stderr, "INCORRECT MAP\n");
}

//############################################################################

//###### USER HOOKS
// compute reduced costs
/*
int
UFL::compute_rc(const VOL_dvector& u, VOL_dvector& rc)
{
   int i,j,k=0;
   for ( i=0; i < nloc; i++){
     rc[i]=fcost[i];
     // for (j = 0; j < ncust; ++j) {
     std::map<int,double>::iterator col_it = _dist[i].begin();
     for (; col_it!=_dist[i].end(); col_it++) {
       std::pair<int,double> p = *col_it;
       int j = p.first;
       double distij = p.second;
       //rc[nloc+k]= dist[k] - u[j];
       rc[nloc+k]= distij - u[j];
       ++k;
     }
   }
   return 0;
}
*/

// We assume the side constraint sparsity pattern is the same as 
// the costs _dist, without checking.
// TODO: If not, we need merge the two sparsity patterns.
// This affects the length of the rc vector!
int
UFL::compute_rc(const VOL_dvector& u, VOL_dvector& rc)
{
   int i,j,k=0;

   //printf("Debug in comp_rc: Multipliers u (size=%i) = \n", u.size());
   //for (i=0; i<u.size(); i++)
   //  printf("%lf ", u[i]);
   //printf("\n");
 
// Standard UFL or p-median objective

   for ( i=0; i < nloc; i++){
     rc[i]=fcost[i];
     double * begin_vals = _dist.col_values_begin(i);
     int    * begin_cols = _dist.col_indices_begin(i);
     double * end_vals = _dist.col_values_end(i);
     int    * end_cols = _dist.col_indices_end(i);
     double *ptr1 = begin_vals; 
     int    *ptr2 = begin_cols;
     for (; ptr1 < end_vals; ptr1++, ptr2++) {
       int j = *ptr2;
       double distij = *ptr1;
       rc[nloc+k]= distij - u[j];
       ++k;
     }
   }

   // Additional (linear) side constraints
   int c=0;
   vector<SparseMatrixCSR<double> >::iterator sc; // side constraint iterator
   for (sc= _sidecon.begin(), c=0; sc != _sidecon.end(); ++sc,++c) {
     k= 0;
     double mult = u[ncust+c]; // Multiplier for side constraint c
     //printf("Compute_rc for constraint %d: lambda = %f\n", c, lambda);
     for ( i=0; i < nloc; i++){
       double * begin_vals = sc->col_values_begin(i);
       double * end_vals = sc->col_values_end(i);
       double *ptr1 = begin_vals;
       for (; ptr1 < end_vals; ptr1++) {
         // double distij = *ptr1;
         rc[nloc+k] += mult * (*ptr1);
         ++k;
       }
     }
   }

   return 0;
}

// IN: dual vector u
// OUT: primal solution to the Lagrangian subproblem (x)
//      optimal value of Lagrangian subproblem (lcost)
//      v = difference between the rhs and lhs when substituting
//                  x into the relaxed constraints (v)
//      objective value of x substituted into the original problem (pcost)
//      xrc
// return value: -1 (volume should quit) 0 infeasible 1 feasible

int 
UFL::solve_subproblem(const VOL_dvector& u, const VOL_dvector& rc,
                      double& lcost, 
                      VOL_dvector& x, VOL_dvector& v, double& pcost)
{
   int i,j;

   lcost = 0.0;
   for (i = 0; i < ncust; ++i) {
      lcost += u[i];
      v[i]=1;
   }

   // VOL_ivector sol(nloc + nloc*ncust);
   VOL_ivector sol(x.size());

   // Produce a primal solution of the relaxed problem
   // For p-median, we can only open p facilities
 
   const double * rdist = rc.v + nloc;
   double sum;
   int k=0, k1=0;
   double value=0.;
   int xi;
   std::map<int,double>::iterator col_it;

   if (_p == 0) { // UFL
     for ( i=0; i < nloc; ++i ) {
       sum=0.;
       int * begin_cols = _dist.col_indices_begin(i);
       int * end_cols   = _dist.col_indices_end(i);
       for (int * ptr1 = begin_cols; ptr1 < end_cols; ptr1++) {
         if ( rdist[k]<0. ) sum+=rdist[k];
         ++k;
       }
       if (fix[i]==0) xi=0;
       else 
         if (fix[i]==1) xi=1;
         else 
           if ( fcost[i]+sum >= 0. ) xi=0;
           else xi=1;
       sol[i]=xi;
       value+=(fcost[i]+sum)*xi;
       begin_cols = _dist.col_indices_begin(i);
       end_cols   = _dist.col_indices_end(i);
       for (int * ptr1 = begin_cols; ptr1 < end_cols; ptr1++) {
         if ( rdist[k1] < 0. ) sol[nloc+k1]=xi;
         else sol[nloc+k1]=0;
         ++k1;
       }
     }
   }
   else { // _p>0, p-median 
     // p lowest rho, as in Avella et al.
     // (index, value) pairs
     std::vector<std::pair<int,double> > rho(_p+1); 
     std::pair<int,double> rho_temp; 
     int q;

     for (q=0; q<_p; q++) {
       rho[q].first = -1;
       rho[q].second = 0;
     }

     for ( i=0; i < nloc; ++i ) {
       sum=0.;
       int * begin_cols = _dist.col_indices_begin(i);
       int * end_cols   = _dist.col_indices_end(i);
       for (int * ptr1 = begin_cols; ptr1 < end_cols; ptr1++) {
         if ( rdist[k]<0. ) sum+=rdist[k];
         ++k;
       }

       // Save p lowest rho values
       rho_temp.first = i;
       // rho_temp.second = sum - u[i]; // Avelli et al. use y[j] for x[j,j]
       rho_temp.second = sum;
       rho[_p] = rho_temp;
       for (q=_p; q>0; q--){
         if (rho[q-1].first == -1 || (rho[q].second < rho[q-1].second)){
           rho_temp = rho[q-1];
           rho[q-1] = rho[q];
           rho[q] = rho_temp;
         }
         else
           break;
       }
     }

     // Set up start array for solution indices (Need only be done once)
     int * start = new int[nloc];
     k=nloc;
     for ( i=0; i < nloc; ++i ) {
       start[i] = k;
       int * begin_cols = _dist.col_indices_begin(i);
       int * end_cols   = _dist.col_indices_end(i);
       for (int * ptr1 = begin_cols; ptr1 < end_cols; ptr1++) {
         ++k;
       }
     }

     // Compute x and function value based on smallest rho values
     value = 0;
     for ( i=0; i < nloc+nnz; ++i ) {
       sol[i] = 0;
     }
     // Loop over open locations
     int ii;
     for ( ii=0; ii < _p; ++ii ) {
       i = rho[ii].first;
       if (i == -1) // Fewer useful locations than allowable sensors
         break;
       // Open a facility at location i
       sol[i]=1;
       // printf("Debug: Open facility at location %d, rho= %f\n",
         // i, rho[ii].second);
       value+= rho[ii].second;
       // for ( j=0; j < ncust; ++j )
         // k1 = nloc+rho_ind[i]*ncust+j;
       k1 = start[i];
       int * begin_cols = _dist.col_indices_begin(i);
       int * end_cols   = _dist.col_indices_end(i);
       for (int * ptr1 = begin_cols; ptr1 < end_cols; ptr1++) {
         // Set customer-facility variables
         if ( rdist[k1-nloc] < 0. ) sol[k1] = 1;
         ++k1;
       }
     }
     delete [] start;
   }

   lcost += value; 

   pcost = 0.0;
   x = 0.0;
   for (i = 0; i < nloc; ++i) {
     if (_p == 0) // Include facility open costs for UFL, not p-median
       pcost += fcost[i] * sol[i];
     x[i] = sol[i];
   }

   // Compute x, v, pcost from sol
   k = 0;
   for ( i=0; i < nloc; i++){
     double * begin_vals = _dist.col_values_begin(i);
     int    * begin_cols = _dist.col_indices_begin(i);
     double * end_vals = _dist.col_values_end(i);
     int    * end_cols = _dist.col_indices_end(i);
     double *ptr1 = begin_vals; 
     int    *ptr2 = begin_cols;
     for (; ptr1 < end_vals; ptr1++, ptr2++) {
       int j = *ptr2;
       double distij = *ptr1;
       x[nloc+k]=sol[nloc+k];
       pcost+= distij*sol[nloc+k];
       v[j]-=sol[nloc+k];
       ++k;
     }
   }

   // Compute slack/violations of side constraints
   for (i=0; i<_ub.size(); i++)
     v[ncust+i] = - _ub[i]; 

   int c;
   vector<SparseMatrixCSR<double> >::iterator sc; // side constraint iterator
   for (sc= _sidecon.begin(), c=0; sc != _sidecon.end(); ++sc,++c) {
     k = 0;
     for ( i=0; i < nloc; i++){
       double * begin_vals = sc->col_values_begin(i);
       int    * begin_cols = sc->col_indices_begin(i);
       double * end_vals = sc->col_values_end(i);
       int    * end_cols = sc->col_indices_end(i);
       double *ptr1 = begin_vals; 
       int    *ptr2 = begin_cols;
       for (; ptr1 < end_vals; ptr1++, ptr2++) {
         int j = *ptr2;
         //double distij = *ptr1;
         v[ncust+c] += (*ptr1) * x[nloc+k]; // Update side constraint c
         ++k;
       }
     }
     // Update lcost (Lagrangian value)
     if (_ub[c] < 0) _ub[c] = 0;
     lcost -= u[ncust+c] * _ub[c]; 
   }


   return 1;
}

// comparison functor for auxiliary sorting
class cmp {
public:
        cmp(const VOL_dvector &x) : val(x) {}
        bool operator()(int i, int j)
        {
                if (i == j) return false;
                if (val[i] > val[j]) return true;
                return false;
        }
private:
        const VOL_dvector &val;
};

// IN:  fractional primal solution (x),
//      best integer feasible soln value so far (icost)
// OUT: integral primal solution (ix) if better than best so far
//      and primal value (icost)
// returns -1 if Volume should stop, 0/1 if feasible solution wasn't/was
//         found.
// We use randomized rounding. We look at x[i] as the probability
// of opening facility i.

/*  This was our first pass at rounding; the new method below uses SNLRound 
int
UFL::heuristics(const VOL_problem& p,
                const VOL_dvector& x, double& new_icost, double lb)
{
   int nopen=0;
   int state = 12345;
   int i, j, n = nloc;
   cmp descending(x);
   int *sorted = new int[nloc];  
   double r;
   VOL_ivector nsol(nloc + nnz);
   nsol=0;

   for (int i=0; i<nloc; ++i)
     sorted[i] = i;

   if (have_dummy){
     nsol[nloc-1] = 1; // always open dummy location
     ++nopen;
     --n;
   }
   
   // p-median: Sort to try the best locations first 
   if (_p){
     sort(sorted, sorted+n, descending); 
   }

   // Randomized rounding. Loop over facilities
   // and flip a biased coin to decide whether to open.
   // TODO: Use Cindy's and Jon's algorithm.

   // May need a couple passes to open enough facilities
   int maxpass= (_p>0 ? 3 : 1);
   for (int pass=0; (nopen<_p) && (pass<maxpass); ++pass){ 
     for ( i=0; i < n; ++i){ // n is nloc or nloc-1 
       j = sorted[i];
  #ifndef _WIN32
       r=drand48();
  #else
       r= PMrand(&state) / (double) INT_MAX;
  #endif
  
       // p-median: Open facilities until we have p. 
       if (r < x[j]) { 
         nsol[j]=1; // Pick j
         ++nopen;
         // printf("Debug: Picked %i (x=%f) \n", j, x[j]);
         if (_p && (nopen == _p))  // We have picked p; stop!
           break;
       }
     }
   }
   vector<int> placements(nloc);
   for (int i=0; i<nloc; i++) {
        if (nsol[i]) placements.push_back(i);
   }
   delete [] sorted;
   return heuristics_aux(placements, p, x, new_icost, lb);
}
*/

int
UFL::heuristics(const VOL_problem& p,
                const VOL_dvector& x, double& new_icost, double lb)
{
   //
   // Rescale the vector to have weight 1.0 in the dummy
   //
   if (x.v[nloc-1] < 1) {
      double scale = (_p-1.0)/(_p - x.v[nloc-1]);
      for (size_t i=0; i<nloc-1; i++)
          x.v[i] = x.v[i] * scale;
      x.v[nloc-1]=1.0;
   }
   #if 0
   std::cerr << "x.v: ";
   double sum=0.0;
   for (size_t i=0; i<nloc; i++) {
     sum += x.v[i];
     std::cerr << " " << x.v[i];
   }
   std::cerr << " SUM=" << sum << std::endl;
   std::cerr << std::endl;
   #endif

   // VOL_ivector nsol(nloc + nloc*ncust);
   int num_sidecon = p.dsize - ncust;
   //int num_iter = 100 * (int) pow(2.0,num_sidecon*5);
   int num_iter = 10;
   vector<vector<int> > placements = 
        multipleRandomizedSensorPlacements(x.v,nloc,_p, num_iter);

   int retval = 0;
   for (int i=0; i<num_iter; i++) {
        #if 0
        std::cerr << "SOLN: ";
        for (size_t j=0; j<placements[i].size(); j++)
            std::cerr << " " << placements[i][j];
        std::cerr << "  NLOC=" << nloc-1 << std::endl;
        std::cerr << std::endl;
        #endif

        // check if the dummy is in the solution.  if not, we reject it.
        bool no_good = true;
           for (int j=0; j<placements[i].size(); j++) {
                if (placements[i][j] == nloc-1)
                        no_good = false;
        }
        if (no_good) continue;

           retval = heuristics_aux(placements[i], p, x, new_icost, lb);
        if (retval == -1) return retval;
   }
   return retval;   // weak; this just returns the most recent return code,
                    // which may be 'infeasible' (even though one of the
                    // rounding results might have been feasible.
}

int
UFL::heuristics_aux(vector<int>& facilities, const VOL_problem& p,
                const VOL_dvector& x, double& new_icost, double lb)
{
   VOL_ivector nsol(nloc + nnz);
   nsol=0;
   int i,j;
   int n = nloc;
   double r,value=0;

   if (have_dummy){
     nsol[nloc-1] = 1; // always open dummy location
   }
   for (int i=0; i<facilities.size(); i++) {
        nsol[facilities[i]] = 1;
   }
   if (_p == 0) {
           for (int i=0; i<nloc; i++) {
                value+=fcost[j]*nsol[j]; 
        }
   }
   
   // We assign customers to locations.
   double *xmin = new double[ncust];
   int *kmin = new int[ncust];
   int k=0;

   for ( j=0; j < ncust; ++j){ 
     xmin[j]= 1.e31; // huge 
     kmin[j]= -1;
   }

   // Sparse code loops over rows (locations) first.
   int rows = _dist.MatrixRows();
   for (int i=0; i<rows; i++) {
     double * begin_vals = _dist.col_values_begin(i);
     int    * begin_cols = _dist.col_indices_begin(i);
     double * end_vals = _dist.col_values_end(i);
     int    * end_cols = _dist.col_indices_end(i);
     double *ptr1 = begin_vals; 
     int    *ptr2 = begin_cols;
     for (; ptr1 < end_vals; ptr1++, ptr2++) {
       int j = *ptr2;
       double distij = *ptr1;
       if ( nsol[i] && (distij < xmin[j]) ){ 
         xmin[j]= distij;
         kmin[j]= k; 
       }
       ++k;
     }
   }

   // Now xmin tells us how to assign customers
   for ( j=0; j < ncust; ++j){ 
     value += xmin[j];
     if (kmin[j] >= 0) {
       nsol[nloc+kmin[j]]= 1;
     }
     //else
       //printf("Warning: heuristic could not serve customer %d\n", j);
   }
   delete [] xmin;
   delete [] kmin;

   // Check side constraints.
   // "Best" solution is either lowest objective value
   // among feasible solutions, or smallest constraint violation.
   // We could keep a cache of Pareto optimal solutions here.

   double maxviol = 0;
   for (int c=0; c < _sidecon.size(); ++c) {
     double scval = evaluate_sc(_sidecon[c], nsol);
     if (scval < _ub[c]) { 
       // feasible
     }
     else {
       // infeasible for this side constraint
       // maxviol = MAX(maxviol, scval - _ub[c]);
       if (scval - _ub[c] > maxviol)
         maxviol = scval - _ub[c];
     }
   }

   // Update best solution (ix and and icost)
   // Look for lower value, or smaller violation

   // Vol expects new_icost to be the value of the current
   // best integer feasible solution found during _this call_
   // to heuristics.  icost is what ufl maintains as the best
   // integer feasible solution found at any time. 
   // We update new_icost only if the solution just found is 
   // really feasible (including side constraints).
   if (maxviol<=0 && ix_sc_viol<=0) new_icost = value;
   if (((maxviol==0) && (value < icost)) // feasible, lower value
       || (maxviol < ix_sc_viol)) // smaller violation
   {
      icost = value;
      ix = nsol;
   }

   // Close enough to lower bound?
   if (fabs((icost - lb)/lb) < gap) {
        return -1;
   }
   if (maxviol>0 || ix_sc_viol>0) return 0;

   //printf("int sol %f\n", new_icost);

   return 1;
}

double evaluate_sc(SparseMatrixCSR<double>& sc, const VOL_ivector& x)
{
  double val = 0;
  int k = 0;
  int nloc = sc.MatrixRows();
  for ( int i=0; i < nloc; i++){
    double * begin_vals = sc.col_values_begin(i);
    int    * begin_cols = sc.col_indices_begin(i);
    double * end_vals = sc.col_values_end(i);
    int    * end_cols = sc.col_indices_end(i);
    double *ptr1 = begin_vals;
    int    *ptr2 = begin_cols;
    for (; ptr1 < end_vals; ptr1++, ptr2++) {
      int j = *ptr2;
      //double distij = *ptr1;
      val += (*ptr1) * x[nloc+k]; // Update side constraint c
      ++k;
    }
  }
  return val;
}



void UFL_output(std::string &outputFileName, double upper, double lower,
                VOL_ivector &locations, int nlocs, int ncust)
{

  // save integer solution
  if (1) {
    FILE* file = fopen("sensors.txt", "w");
    int i,j;
    fprintf(file, "Best integer solution value: %f\n", upper);
    fprintf(file, "Lower bound: %f\n", lower);
    fprintf(file, "Sensor locations\n");
    for (i = 0,j=0; i < nlocs; ++i) {
      fprintf(file, "%6d ", locations[i]);
      if (j && (j%10 == 0)) fprintf(file, "\n");
      j++;
    }
    fprintf(file, "\n");
  
#if 0
  // CAP: this was moved when I pulled this code out.  No guarantee it has what
  // it needs to function or even compile right now.

         //fprintf(file, "Assignment of customers\n");
         //for (i = 0; i < n; ++i) {
           //for (j = 0; j < m; ++j) {
             //if ( x[k]==1 ) 
               //fprintf(file, "customer %i  location %i\n", j+1, i+1);
             //++k;
           //}
         //}
         // sparse replacement code
         for (int i=0; i < n; i++){
           std::map<int,double>::iterator col_it = ufl_data._dist[i].begin();
           for (; col_it!=ufl_data._dist[i].end(); col_it++) {
             std::pair<int,double> p = *col_it;
             int j = p.first;
             if ( x[k]==1 )
               fprintf(file, "customer %i  location %i\n", j+1, i+1);
             ++k;
           }
         }
#endif

         fclose(file);
  }

  if (outputFileName.size() > 0) {
    FILE* file = fopen(outputFileName.c_str(), "w");
    fprintf(file, "{\n");
    fprintf(file, "  \"lower bound\": %f,\n", lower);
    fprintf(file, "  \"solutions\": [\n");
    fprintf(file, "  {\n");
    fprintf(file, "  \"value\": %f,\n", upper);
    fprintf(file, "  \"ids\": [ ");
    for (int i = 0; i < nlocs-1; ++i) {
        fprintf(file, "%d, ", locations[i]);
    }
    fprintf(file, "%6d ", locations[nlocs-1]);
    fprintf(file, " ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "]\n");
    fprintf(file, "}\n");
    fclose(file);
    }

  printf(" ncust: %d\n", ncust);
  printf(" Best integer solution value: %f\n", upper);
  printf(" Lower bound: %f\n", lower);
}
