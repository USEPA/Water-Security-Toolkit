// Test suite for the Merlion upper triangular linear solver
#include <cxxtest/TestSuite.h>

#include <merlion/TriSolve.hpp>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <math.h>

#define VERBOSE_FAIL 0

namespace {

double LargeMatrixAbsTol = 1e-5;
double SmallMatrixAbsTol = 0.0;

template <typename T>
bool ArraysAreEqual(T& x, T& y, int start, int stop, int nrhs, double tol=0.0)
{
   // Check array values do not differ by more than tol
   for (int i = start; i <= stop; ++i) {
      for (int r = 0; r < nrhs; ++r) {
	 if(fabs(x[i*nrhs+r]-y[i*nrhs+r]) > tol) {
	    std::cout.precision(10);
	    std::cout << "Arrays not equal within tol=" << tol << std::endl;
#if VERBOSE_FAIL
	    for (int ii = start; ii <= stop; ++ii) {
	       for (int rr = 0; rr < nrhs; ++rr) {
		  std::cout << "x[" << ii*nrhs+rr << "] = " << x[ii*nrhs+rr] << ", y[" << ii*nrhs+rr << "] = " << y[ii*nrhs+rr] << std::endl;
	       }
	    }
#endif
	    std::cout.precision();
	    return false;
	 }
      }
   }
   return true;
}

template <typename T>
bool ArrayEquals(T& x, double y, int start, int stop, int nrhs, double tol=0.0)
{
   // Check array values do not differ by more than tol
   for (int i = start; i <= stop; ++i) {
      for (int r = 0; r < nrhs; ++r) {
	 if(fabs(x[i*nrhs+r]-y) > tol) {
	    std::cout.precision(10);
	    std::cout << "Array not equal to value within tol=" << tol << std::endl;
#if VERBOSE_FAIL
	    for (int ii = start; ii <= stop; ++ii) {
	       for (int rr = 0; rr < nrhs; ++rr) {
		  std::cout << "x[" << ii*nrhs+rr << "] = " << x[ii*nrhs+rr] << ", y = " << y << std::endl;
	       }
	    }
#endif
	    std::cout.precision();
	    return false;
	 } 
      }
   }
   return true;
}

// A simple matrix class for reading in test files
template<typename T>
class TestCSCMatrix
{
 public:
   ~TestCSCMatrix()
   {
      N = 0;
      nnz = 0;
      delete [] Ax;
      Ax = NULL;
      delete [] Ai;
      Ai = NULL;
      delete [] Ap;
      Ap = NULL;
      rhs.clear();
      ans.clear();
   }
   
   TestCSCMatrix()
      :
      N(0),
      nnz(0),
      Ax(NULL),
      Ai(NULL),
      Ap(NULL)
   {
      rhs.clear();
      ans.clear();
   }

   bool Read(std::istream& in)
   {
      std::string tag;
      // First Read in a sparse matrix in CSC format
      in >> tag;
      if (tag != "CSCMatrix") {return false;}
      in >> tag;
      tag = "";
      int tmp;
      in >> tmp;
      in >> N;
      if (tmp != N) {return false;}
      in >> nnz;
      Ax = new T[nnz];
      Ai = new int[nnz];
      Ap = new int[N+1];
      Ap[0] = 0;
      for (int j = 0; j < N; ++j) {
	 int col_nnz;
	 in >> col_nnz;
	 Ap[j+1] = Ap[j]+col_nnz;
	 for (int p = Ap[j], p_stop = Ap[j+1]; p < p_stop; ++p) {
	    in >> Ai[p];
	    in >> Ax[p];
	 }
      }
      // Now read in each right hand side along
      // with its corresponding solution for
      // Ax = b as well as (U^t)x = b
      in >> tag;
      if (tag != "NRHS") {return false;}
      tag = "";
      int nrhs;
      in >> nrhs;
      if (nrhs <= 0) {return false;}
      for (int r = 0; r < nrhs; ++r) {
	 rhs.push_back(std::vector<T>(N));
	 ans["A"].push_back(std::vector<T>(N));
	 ans["At"].push_back(std::vector<T>(N));
	 in >> tag;
	 if (tag != "Vector") {return false;}
	 in >> tag;
	 tag = "";
	 for (int i = 0; i < N; ++i) {
	    in >> rhs[r][i];
	 }
	 in >> tag;
	 if (tag != "Vector") {return false;}
	 in >> tag;
	 tag = "";
	 for (int i = 0; i < N; ++i) {
	    in >> ans["A"][r][i];
	 }
	 in >> tag;
	 if (tag != "Vector") {return false;}
	 in >> tag;
	 tag = "";
	 for (int i = 0; i < N; ++i) {
	    in >> ans["At"][r][i];
	 }
      }
      return true;
   }

   int N, nnz;
   T *Ax;
   int *Ai, *Ap;
   std::vector<std::vector<T> > rhs;
   std::map<std::string, std::vector<std::vector<T> > > ans;
};

template<typename T>
void _test_mult_rhs_v1(TestCSCMatrix<T>& tm, std::string type, double tol=0.0, bool short_test=false)
{
   //
   //  Test the multiple rhs solve routine that
   //  takes in a sparse rhs in triplet format
   //
   std::string solution_type = "A";
   if ((type == "UT") || (type == "LT")) {
      solution_type = "At";
   }
   T init_val = -1111.1111;
   int N = tm.N;
   int nrhs = tm.rhs.size();
   std::vector<T> sol(N*nrhs, init_val);
   std::vector<T> Bx;
   std::vector<int> Bi;
   std::vector<int> Bj;
   std::vector<T> ans(N*nrhs);
   const int MAX_START_ROW = (short_test)?(0):(N-1);
   for (int start_row = 0; start_row <= MAX_START_ROW; ++start_row) {
      for (int stop_row = ((short_test)?(N-1):(start_row)); stop_row <= N-1; ++stop_row) {
	 for (int rhs = 0; rhs < nrhs; ++rhs) {
	    for (int i = 0; i < N; ++i) {
	       if (tm.rhs[rhs][i] != 0.0) {
		  Bx.push_back(tm.rhs[rhs][i]);
		  Bi.push_back(i);
		  Bj.push_back(rhs);
	       }
	       ans[i*nrhs + rhs] = tm.ans[solution_type][rhs][i];
	    }
	 }
	 if (type == "UT") {
	    utsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(Bi[0]),&(Bj[0]),&(Bx[0]),Bx.size(),&(sol[0]));
	    if (start_row == 0) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "U") {
	    usolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(Bi[0]),&(Bj[0]),&(Bx[0]),Bx.size(),&(sol[0]));
	    if (stop_row == N-1) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "LT") {
	    ltsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(Bi[0]),&(Bj[0]),&(Bx[0]),Bx.size(),&(sol[0]));
	    if (stop_row == N-1) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "L") {
	    lsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(Bi[0]),&(Bj[0]),&(Bx[0]),Bx.size(),&(sol[0]));
	    if (start_row == 0) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else {
	    TS_FAIL("Bad Test");
	 }
	 TS_ASSERT(::ArrayEquals(sol,init_val,0,start_row-1,nrhs));
	 TS_ASSERT(::ArrayEquals(sol,init_val,stop_row+1,N-1,nrhs));
	 sol.clear();
	 sol.assign(N*nrhs, init_val);
	 Bx.clear();
	 Bi.clear();
	 Bj.clear();
      }
   }
}
template<typename T>
void _test_mult_rhs_v2(TestCSCMatrix<T>& tm, std::string type, double tol=0.0, bool short_test=false)
{
   //
   //  Test the multiple rhs solve routine that
   //  takes in a dense rhs and dense solution vector
   //
   std::string solution_type = "A";
   if ((type == "UT") || (type == "LT")) {
      solution_type = "At";
   }
   T init_val = -1111.1111;
   int N = tm.N;
   int nrhs = tm.rhs.size();
   std::vector<T> sol(N*nrhs, init_val);
   std::vector<T> B(N*nrhs);
   std::vector<T> ans(N*nrhs);
   const int MAX_START_ROW = (short_test)?(0):(N-1);
   for (int start_row = 0; start_row <= MAX_START_ROW; ++start_row) {
      for (int stop_row = ((short_test)?(N-1):(start_row)); stop_row <= N-1; ++stop_row) {
	 for (int rhs = 0; rhs < nrhs; ++rhs) {
	    for (int i = 0; i < N; ++i) {
	       B[i*nrhs + rhs] = tm.rhs[rhs][i];
	       ans[i*nrhs + rhs] = tm.ans[solution_type][rhs][i];
	    }
	 }
	 if (type == "UT") {
	    utsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(B[0]),&(sol[0]));
	    if (start_row == 0) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "U") {
	    usolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(B[0]),&(sol[0]));
	    if (stop_row == N-1) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "LT") {
	    ltsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(B[0]),&(sol[0]));
	    if (stop_row == N-1) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "L") {
	    lsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(B[0]),&(sol[0]));
	    if (start_row == 0) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else {
	    TS_FAIL("Bad Test");
	 }
	 TS_ASSERT(::ArrayEquals(sol,init_val,0,start_row-1,nrhs));
	 TS_ASSERT(::ArrayEquals(sol,init_val,stop_row+1,N-1,nrhs));
	 sol.clear();
	 sol.assign(N*nrhs, init_val);
      }
   }
}
template<typename T>
void _test_mult_rhs_v3(TestCSCMatrix<T>& tm, std::string type, double tol=0.0, bool short_test=false)
{
   //
   //  Test the multiple rhs solve routine that
   //  transforms a dense rhs into a dense solution vector
   //
   std::string solution_type = "A";
   if ((type == "UT") || (type == "LT")) {
      solution_type = "At";
   }
   T init_val = -1111.1111;
   int N = tm.N;
   int nrhs = tm.rhs.size();
   std::vector<T> sol(N*nrhs, init_val);
   std::vector<T> ans(N*nrhs);
   const int MAX_START_ROW = (short_test)?(0):(N-1);
   for (int start_row = 0; start_row <= MAX_START_ROW; ++start_row) {
      for (int stop_row = ((short_test)?(N-1):(start_row)); stop_row <= N-1; ++stop_row) {
	 for (int rhs = 0; rhs < nrhs; ++rhs) {
	    for (int i = 0; i < N; ++i) {
	       if ((i >= start_row) && (i <= stop_row)) {
		  sol[i*nrhs + rhs] = tm.rhs[rhs][i];
	       }
	       ans[i*nrhs + rhs] = tm.ans[solution_type][rhs][i];
	    }
	 }
	 if (type == "UT") {
	    utsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(sol[0]));
	    if (start_row == 0) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "U") {
	    usolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(sol[0]));
	    if (stop_row == N-1) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "LT") {
	    ltsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(sol[0]));
	    if (stop_row == N-1) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else if (type == "L") {
	    lsolvem(N,start_row,stop_row,nrhs,tm.Ax,tm.Ai,tm.Ap,&(sol[0]));
	    if (start_row == 0) {
	       TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	    }
	 }
	 else {
	    TS_FAIL("Bad Test");
	 }
	 TS_ASSERT(::ArrayEquals(sol,init_val,0,start_row-1,nrhs));
	 TS_ASSERT(::ArrayEquals(sol,init_val,stop_row+1,N-1,nrhs));
	 sol.clear();
	 sol.assign(N*nrhs, init_val);
      }
   }
}
template<typename T>
void _test_single_rhs_v1(TestCSCMatrix<T>& tm, std::string type, double tol=0.0, bool short_test=false)
{
   //
   //  Test the single rhs solve routine that
   //  takes in a sparse rhs in triplet format
   //
   std::string solution_type = "A";
   if ((type == "UT") || (type == "LT")) {
      solution_type = "At";
   }
   T init_val = -1111.1111;
   int N = tm.N;
   int nrhs = 1;
   const int MAX_START_ROW = (short_test)?(0):(N-1);
   for (int start_row = 0; start_row <= MAX_START_ROW; ++start_row) {
      for (int stop_row = ((short_test)?(N-1):(start_row)); stop_row <= N-1; ++stop_row) {
	 for (int rhs = 0; rhs < nrhs; ++rhs) {
	    std::vector<T> sol(N,init_val);
	    std::vector<T> ans(tm.ans[solution_type][rhs].begin(),tm.ans[solution_type][rhs].end());
	    std::vector<int> bi;
	    std::vector<T> bx;
	    for (int i = 0; i < N; ++i) {
	       if (tm.rhs[rhs][i] != 0.0) {
		  bi.push_back(i);
		  bx.push_back(tm.rhs[rhs][i]);
	       }
	    }
	    if (type == "UT") {
	       utsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(bi[0]),&(bx[0]),bx.size(),&(sol[0]));
	       if (start_row == 0) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "U") {
	       usolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(bi[0]),&(bx[0]),bx.size(),&(sol[0]));
	       if (stop_row == N-1) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "LT") {
	       ltsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(bi[0]),&(bx[0]),bx.size(),&(sol[0]));
	       if (stop_row == N-1) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "L") {
	       lsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(bi[0]),&(bx[0]),bx.size(),&(sol[0]));
	       if (start_row == 0) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else {
	       TS_FAIL("Bad Test");
	    }
	    TS_ASSERT(::ArrayEquals(sol,init_val,0,start_row-1,nrhs));
	    TS_ASSERT(::ArrayEquals(sol,init_val,stop_row+1,N-1,nrhs));
	 }
      }
   }
}
template<typename T>
void _test_single_rhs_v2(TestCSCMatrix<T>& tm, std::string type, double tol=0.0, bool short_test=false)
{
   //
   //  Test the single rhs solve routine that
   //  takes in a dense rhs and dense solution vector
   //
   std::string solution_type = "A";
   if ((type == "UT") || (type == "LT")) {
      solution_type = "At";
   }
   T init_val = -1111.1111;
   int N = tm.N;
   int nrhs = 1;
   const int MAX_START_ROW = (short_test)?(0):(N-1);
   for (int start_row = 0; start_row <= MAX_START_ROW; ++start_row) {
      for (int stop_row = ((short_test)?(N-1):(start_row)); stop_row <= N-1; ++stop_row) {
	 for (int rhs = 0; rhs < nrhs; ++rhs) {
	    std::vector<T> sol(N,init_val);
	    std::vector<T> ans(tm.ans[solution_type][rhs].begin(),tm.ans[solution_type][rhs].end());
	    std::vector<T> b(tm.rhs[rhs].begin(),tm.rhs[rhs].end());
	    if (type == "UT") {
	       utsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(b[0]), &(sol[0]));
	       if (start_row == 0) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "U") {
	       usolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(b[0]), &(sol[0]));
	       if (stop_row == N-1) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "LT") {
	       ltsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(b[0]), &(sol[0]));
	       if (stop_row == N-1) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "L") {
	       lsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(b[0]), &(sol[0]));
	       if (start_row == 0) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else {
	       TS_FAIL("Bad Test");
	    }
	    TS_ASSERT(::ArrayEquals(sol,init_val,0,start_row-1,nrhs));
	    TS_ASSERT(::ArrayEquals(sol,init_val,stop_row+1,N-1,nrhs));
	 }
      }
   }
}
template<typename T>
void _test_single_rhs_v3(TestCSCMatrix<T>& tm, std::string type, double tol=0.0, bool short_test=false)
{
   //
   //  Test the single rhs solve routine that
   //  transforms a dense rhs into a dense solution vector
   //
   std::string solution_type = "A";
   if ((type == "UT") || (type == "LT")) {
      solution_type = "At";
   }
   T init_val = -1111.1111;
   int N = tm.N;
   int nrhs = 1;
   const int MAX_START_ROW = (short_test)?(0):(N-1);
   for (int start_row = 0; start_row <= MAX_START_ROW; ++start_row) {
      for (int stop_row = ((short_test)?(N-1):(start_row)); stop_row <= N-1; ++stop_row) {
	 for (int rhs = 0; rhs < nrhs; ++rhs) {
	    std::vector<T> sol(N, init_val);
	    std::vector<T> ans(tm.ans[solution_type][rhs].begin(),tm.ans[solution_type][rhs].end());
	    for (int i = start_row; i <= stop_row; ++i) {
	       sol[i] = tm.rhs[rhs][i];
	    }
	    if (type == "UT") {
	       utsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(sol[0]));
	       if (start_row == 0) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "U") {
	       usolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(sol[0]));
	       if (stop_row == N-1) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "LT") {
	       ltsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(sol[0]));
	       if (stop_row == N-1) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else if (type == "L") {
	       lsolve(N,start_row,stop_row,tm.Ax,tm.Ai,tm.Ap,&(sol[0]));
	       if (start_row == 0) {
		  TS_ASSERT(::ArraysAreEqual(sol,ans,start_row,stop_row,nrhs,tol));
	       }
	    }
	    else {
	       TS_FAIL("Bad Test");
	    }
	    TS_ASSERT(::ArrayEquals(sol,init_val,0,start_row-1,nrhs));
	    TS_ASSERT(::ArrayEquals(sol,init_val,stop_row+1,N-1,nrhs));
	 }
      }
   }
}

}; // End of empty namespace

///////////////////////////// Begin Testing Suits

class TriSolveTestsLargeUpperFloat : public CxxTest::TestSuite
{
 public:

   ::TestCSCMatrix<float> *tm_; 
   
   static TriSolveTestsLargeUpperFloat* createSuite()
   {
      std::ifstream in;
      TriSolveTestsLargeUpperFloat *suite(new TriSolveTestsLargeUpperFloat);
      suite->tm_ = new ::TestCSCMatrix<float>;
      in.open("LargeTestMatrixUpper.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->Read(in);
      in.close();
      return suite;
   }

   static void destroySuite(TriSolveTestsLargeUpperFloat* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   //Large Matrix Tests Float Precision
   // Ux = b
   void test_mult_rhs_v1() {::_test_mult_rhs_v1<float>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_mult_rhs_v2() {::_test_mult_rhs_v2<float>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_mult_rhs_v3() {::_test_mult_rhs_v3<float>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_single_rhs_v1() {::_test_single_rhs_v1<float>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_single_rhs_v2() {::_test_single_rhs_v2<float>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_single_rhs_v3() {::_test_single_rhs_v3<float>(*tm_,"U",::LargeMatrixAbsTol,true);}
   
   // (U^t)x = b
   void test_trans_mult_rhs_v1() {::_test_mult_rhs_v1<float>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_mult_rhs_v2() {::_test_mult_rhs_v2<float>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_mult_rhs_v3() {::_test_mult_rhs_v3<float>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_single_rhs_v1() {::_test_single_rhs_v1<float>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_single_rhs_v2() {::_test_single_rhs_v2<float>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_single_rhs_v3() {::_test_single_rhs_v3<float>(*tm_,"UT",::LargeMatrixAbsTol,true);}
};


class TriSolveTestsLargeUpperDouble : public CxxTest::TestSuite
{
 public:

   ::TestCSCMatrix<double> *tm_; 
   
   static TriSolveTestsLargeUpperDouble* createSuite()
   {
      std::ifstream in;
      TriSolveTestsLargeUpperDouble *suite(new TriSolveTestsLargeUpperDouble);
      suite->tm_ = new ::TestCSCMatrix<double>;
      in.open("LargeTestMatrixUpper.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->Read(in);
      in.close();
      return suite;
   }

   static void destroySuite(TriSolveTestsLargeUpperDouble* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   //Large Matrix Tests Double Precision
   // Ux = b
   void test_mult_rhs_v1() {::_test_mult_rhs_v1<double>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_mult_rhs_v2() {::_test_mult_rhs_v2<double>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_mult_rhs_v3() {::_test_mult_rhs_v3<double>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_single_rhs_v1() {::_test_single_rhs_v1<double>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_single_rhs_v2() {::_test_single_rhs_v2<double>(*tm_,"U",::LargeMatrixAbsTol,true);}
   void test_single_rhs_v3() {::_test_single_rhs_v3<double>(*tm_,"U",::LargeMatrixAbsTol,true);}
   
   // (U^t)x = b
   void test_trans_mult_rhs_v1() {::_test_mult_rhs_v1<double>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_mult_rhs_v2() {::_test_mult_rhs_v2<double>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_mult_rhs_v3() {::_test_mult_rhs_v3<double>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_single_rhs_v1() {::_test_single_rhs_v1<double>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_single_rhs_v2() {::_test_single_rhs_v2<double>(*tm_,"UT",::LargeMatrixAbsTol,true);}
   void test_trans_single_rhs_v3() {::_test_single_rhs_v3<double>(*tm_,"UT",::LargeMatrixAbsTol,true);}
};


class TriSolveTestsSmallUpperFloat : public CxxTest::TestSuite
{
 public:

   ::TestCSCMatrix<float> *tm_; 
   
   static TriSolveTestsSmallUpperFloat* createSuite()
   {
      std::ifstream in;
      TriSolveTestsSmallUpperFloat *suite(new TriSolveTestsSmallUpperFloat);
      suite->tm_ = new ::TestCSCMatrix<float>;
      in.open("SmallTestMatrixUpper.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->Read(in);
      in.close();
      return suite;
   }

   static void destroySuite(TriSolveTestsSmallUpperFloat* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   //Small Matrix Tests Float Precision
   // Ux = b
   void test_mult_rhs_v1() {::_test_mult_rhs_v1<float>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_mult_rhs_v2() {::_test_mult_rhs_v2<float>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_mult_rhs_v3() {::_test_mult_rhs_v3<float>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_single_rhs_v1() {::_test_single_rhs_v1<float>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_single_rhs_v2() {::_test_single_rhs_v2<float>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_single_rhs_v3() {::_test_single_rhs_v3<float>(*tm_,"U",::SmallMatrixAbsTol);}
   
   // (U^t)x = b
   void test_trans_mult_rhs_v1() {::_test_mult_rhs_v1<float>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_mult_rhs_v2() {::_test_mult_rhs_v2<float>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_mult_rhs_v3() {::_test_mult_rhs_v3<float>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v1() {::_test_single_rhs_v1<float>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v2() {::_test_single_rhs_v2<float>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v3() {::_test_single_rhs_v3<float>(*tm_,"UT",::SmallMatrixAbsTol);}
};


class TriSolveTestsSmallUpperDouble : public CxxTest::TestSuite
{
 public:

   ::TestCSCMatrix<double> *tm_; 
   
   static TriSolveTestsSmallUpperDouble* createSuite()
   {
      std::ifstream in;
      TriSolveTestsSmallUpperDouble *suite(new TriSolveTestsSmallUpperDouble);
      suite->tm_ = new ::TestCSCMatrix<double>;
      in.open("SmallTestMatrixUpper.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->Read(in);
      in.close();
      return suite;
   }

   static void destroySuite(TriSolveTestsSmallUpperDouble* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   //Small Matrix Tests Double Precision
   // Ux = b
   void test_mult_rhs_v1() {::_test_mult_rhs_v1<double>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_mult_rhs_v2() {::_test_mult_rhs_v2<double>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_mult_rhs_v3() {::_test_mult_rhs_v3<double>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_single_rhs_v1() {::_test_single_rhs_v1<double>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_single_rhs_v2() {::_test_single_rhs_v2<double>(*tm_,"U",::SmallMatrixAbsTol);}
   void test_single_rhs_v3() {::_test_single_rhs_v3<double>(*tm_,"U",::SmallMatrixAbsTol);}
   
   // (U^t)x = b
   void test_trans_mult_rhs_v1() {::_test_mult_rhs_v1<double>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_mult_rhs_v2() {::_test_mult_rhs_v2<double>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_mult_rhs_v3() {::_test_mult_rhs_v3<double>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v1() {::_test_single_rhs_v1<double>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v2() {::_test_single_rhs_v2<double>(*tm_,"UT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v3() {::_test_single_rhs_v3<double>(*tm_,"UT",::SmallMatrixAbsTol);}
};



class TriSolveTestsSmallLowerFloat : public CxxTest::TestSuite
{
 public:

   ::TestCSCMatrix<float> *tm_; 
   
   static TriSolveTestsSmallLowerFloat* createSuite()
   {
      std::ifstream in;
      TriSolveTestsSmallLowerFloat *suite(new TriSolveTestsSmallLowerFloat);
      suite->tm_ = new ::TestCSCMatrix<float>;
      in.open("SmallTestMatrixLower.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->Read(in);
      in.close();
      return suite;
   }

   static void destroySuite(TriSolveTestsSmallLowerFloat* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   //Small Matrix Tests Float Precision
   // Lx = b
   void test_mult_rhs_v1() {::_test_mult_rhs_v1<float>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_mult_rhs_v2() {::_test_mult_rhs_v2<float>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_mult_rhs_v3() {::_test_mult_rhs_v3<float>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_single_rhs_v1() {::_test_single_rhs_v1<float>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_single_rhs_v2() {::_test_single_rhs_v2<float>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_single_rhs_v3() {::_test_single_rhs_v3<float>(*tm_,"L",::SmallMatrixAbsTol);}
   
   // (L^t)x = b
   void test_trans_mult_rhs_v1() {::_test_mult_rhs_v1<float>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_mult_rhs_v2() {::_test_mult_rhs_v2<float>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_mult_rhs_v3() {::_test_mult_rhs_v3<float>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v1() {::_test_single_rhs_v1<float>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v2() {::_test_single_rhs_v2<float>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v3() {::_test_single_rhs_v3<float>(*tm_,"LT",::SmallMatrixAbsTol);}
};

class TriSolveTestsSmallLowerDouble : public CxxTest::TestSuite
{
 public:

   ::TestCSCMatrix<double> *tm_; 
   
   static TriSolveTestsSmallLowerDouble* createSuite()
   {
      std::ifstream in;
      TriSolveTestsSmallLowerDouble *suite(new TriSolveTestsSmallLowerDouble);
      suite->tm_ = new ::TestCSCMatrix<double>;
      in.open("SmallTestMatrixLower.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->Read(in);
      in.close();
      return suite;
   }

   static void destroySuite(TriSolveTestsSmallLowerDouble* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   //Small Matrix Tests Double Precision
   // Lx = b
   void test_mult_rhs_v1() {::_test_mult_rhs_v1<double>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_mult_rhs_v2() {::_test_mult_rhs_v2<double>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_mult_rhs_v3() {::_test_mult_rhs_v3<double>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_single_rhs_v1() {::_test_single_rhs_v1<double>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_single_rhs_v2() {::_test_single_rhs_v2<double>(*tm_,"L",::SmallMatrixAbsTol);}
   void test_single_rhs_v3() {::_test_single_rhs_v3<double>(*tm_,"L",::SmallMatrixAbsTol);}
   
   // (L^t)x = b
   void test_trans_mult_rhs_v1() {::_test_mult_rhs_v1<double>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_mult_rhs_v2() {::_test_mult_rhs_v2<double>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_mult_rhs_v3() {::_test_mult_rhs_v3<double>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v1() {::_test_single_rhs_v1<double>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v2() {::_test_single_rhs_v2<double>(*tm_,"LT",::SmallMatrixAbsTol);}
   void test_trans_single_rhs_v3() {::_test_single_rhs_v3<double>(*tm_,"LT",::SmallMatrixAbsTol);}
};
