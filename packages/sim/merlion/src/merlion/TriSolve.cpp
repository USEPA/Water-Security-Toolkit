#include <merlion/TriSolve.hpp>
#include <merlion/BlasWrapper.hpp>

// overload functions to make the blas routines automagically
// switch between double and float inside the templated solver kernals
// Why not move these to the blas wrapper?
inline void BLAS_SCALE(int N, const float *scale, float *x, int cntr)
{
   cblas_sscal(N,*scale,x,cntr);
}
inline void BLAS_SCALE(int N, const double *scale, double *x, int cntr)
{
   cblas_dscal(N,*scale,x,cntr);
}

inline void BLAS_AXPY(int N, const float *scale, float *y, int ycntr, float *x, int xcntr)
{
   cblas_saxpy(N,*scale,y,ycntr,x,xcntr);
}
inline void BLAS_AXPY(int N, const double *scale, double *y, int ycntr, double *x, int xcntr)
{
   cblas_daxpy(N,*scale,y,ycntr,x,xcntr);
}


/*
SOLVERS FOR U
FLOAT AND DOUBLE
MULTIPLE RHS
DENSE RHS OR SPARSE RHS
DENSE SOLUTION
*/
/* Kernel for usolvem */
template<typename T>
inline void _usolvem(int N, int n_start, int n_stop, int nrhs, const T *Ux, const int *Ui, const int *Up, T *X)
{
   const int P1(nrhs);
   const int P2(1);
   for (int j(n_stop); j >= n_start; --j) {
      int p_start(Up[j]);
      int p(Up[j+1]-1);
      const T UX_STOP_inv(1.0/Ux[p--]);

      /* Don't operate on X elements smaller than n_start row */
      while (Ui[p_start] < n_start) {++p_start;}
   
      BLAS_SCALE(nrhs,&UX_STOP_inv,X+j*P1,P2);
   
      for (; p >= p_start; --p) {
         T UX_P(-Ux[p]);
         int UI_P(Ui[p]*P1);
         BLAS_AXPY(nrhs,&UX_P,X+j*P1,P2,X+UI_P,P2);
      }
   }
}

/* solve UX=B where B is a sparse coord rhs matrix (**NxNRHS) and X is dense (**NxNRHS) solution matrix, U is sparse CSC format */
void usolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, const int *Bi, const int *Bj, const float *Bx, int Bnnz, float *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_sscal(num_elements, 0.0f, X+p_offset, 1);
   for (int i = 0; i < Bnnz; ++i) {
      if ((Bi[i] >= n_start) && (Bi[i] <= n_stop)) {
	 X[Bi[i]*nrhs+Bj[i]] = Bx[i]; 
      }
   }
   _usolvem<float>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}
void usolvem(int N, int n_start, int n_stop, 
   int nrhs, const double *Ux, const int *Ui, 
   const int *Up, const int *Bi, const int *Bj,
   const double *Bx, int Bnnz, double *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_dscal(num_elements, 0.0, X+p_offset, 1);
   for (int i = 0; i < Bnnz; ++i) {
      if ((Bi[i] >= n_start) && (Bi[i] <= n_stop)) {
	 X[Bi[i]*nrhs+Bj[i]] = Bx[i]; 
      }
   }
   _usolvem<double>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}

/* solve UX=B where X and B are dense (**NxNRHS) solution matrices, U is sparse CSC format */
void usolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, const float *B, float *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_scopy(num_elements, B+p_offset, 1, X+p_offset, 1);
   _usolvem<float>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}
void usolvem(int N, int n_start, int n_stop, 
   int nrhs, const double *Ux, const int *Ui, 
   const int *Up, const double *B, double *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_dcopy(num_elements, B+p_offset, 1, X+p_offset, 1);
   _usolvem<double>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}

/* solve UX=B where X is dense (**NxNRHS) matrix which is converted from rhs to solution, U is sparse CSC format */
void usolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, float *X)
{
   _usolvem<float>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}
void usolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, const int *Up, double *X)
{
   _usolvem<double>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}


/*
SOLVERS FOR U
FLOAT AND DOUBLE
SINGLE RHS
DENSE RHS OR SPARSE RHS
DENSE SOLUTION
*/
/* Kernel for usolve */
template<typename T>
inline void _usolve(int n_start, int n_stop, const T *Ux, const int *Ui, const int *Up, T *x)
{
   for (int j(n_stop); j >= n_start; --j) {
      int p_start(Up[j]);
      int p(Up[j+1]-1);
      T& xj(x[j]);

      while (Ui[p_start] < n_start) {++p_start;}
   
      xj /= Ux[p--];
      for (; p >= p_start; --p) {
         x[Ui[p]] -= Ux[p]*xj ;
      }
   }
}

/* solve Ux=b where b is a sparse rhs and x is dense, U is sparse CSC format */ 
void usolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, 
   const int *bi, const float *bx, int bnnz, float *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_sscal(num_elements, 0.0f, x+p_offset, 1);
   for (int i = 0; i < bnnz; ++i) {
      if ((bi[i] >= n_start) && (bi[i] <= n_stop)) {
	 x[bi[i]] = bx[i];
      }
   }
   _usolve<float>(n_start, n_stop, Ux, Ui, Up, x);
}
void usolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up,
   const int *bi, const double *bx, int bnnz, double *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_dscal(num_elements, 0.0f, x+p_offset, 1);
   for (int i = 0; i < bnnz; ++i) {
      if ((bi[i] >= n_start) && (bi[i] <= n_stop)) {
	 x[bi[i]] = bx[i];
      }
   }
   _usolve<double>(n_start, n_stop, Ux, Ui, Up, x);
}

/* solve Ux=b where x and b are dense, U is sparse CSC format */
void usolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, const float *b, float *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_scopy(num_elements, b+p_offset, 1, x+p_offset, 1);
   _usolve<float>(n_start, n_stop, Ux, Ui, Up, x);
}
void usolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, const double *b, double *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_dcopy(num_elements, b+p_offset, 1, x+p_offset, 1);
   _usolve<double>(n_start, n_stop, Ux, Ui, Up, x);
}

/* solve Ux=b where x is dense and is converted from a rhs to a solution, U is sparse CSC format */
void usolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, float *x)
{
   _usolve<float>(n_start, n_stop, Ux, Ui, Up, x);
}
void usolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, double *x)
{
   _usolve<double>(n_start, n_stop, Ux, Ui, Up, x);
}


/*
SOLVERS FOR U_TRANSPOSE
FLOAT AND DOUBLE
MULTIPLE RHS
DENSE RHS OR SPARSE RHS
DENSE SOLUTION
*/
/* Kernel for utsolvem */
template<typename T>
inline void _utsolvem(int N, int n_start, int n_stop, int nrhs, const T *Ux, const int *Ui, const int *Up, T *X)
{
   // Ui -> Lj
   // Up -> Lp
   const int P1(nrhs);
   const int P2(1);
   /* int P1(1), P2(N); */ /* for the less efficient case where X is NRHSxN matrix */
   for (int j(n_start); j <= n_stop; ++j) {
      int p(Up[j]);
      int p_stop(Up[j+1]-1);
      const T UX_STOP_inv(1.0/Ux[p_stop]);

      /* Don't operate on X elements smaller than n_start row */
      while (Ui[p] < n_start) {++p;}
   
      for (; p < p_stop; ++p) {
         T UX_P(-Ux[p]);
         int UI_P(Ui[p]*P1);
         BLAS_AXPY(nrhs,&UX_P,X+UI_P,P2,X+j*P1,P2);
      }
      BLAS_SCALE(nrhs,&UX_STOP_inv,X+j*P1,P2);
   }
}
/* solve U'X=B where B is a sparse coord rhs matrix (**NxNRHS) and X is dense (**NxNRHS) solution matrix, U is sparse CSC format */
void utsolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, 
   const int *Up, const int *Bi, const int *Bj,
   const float *Bx, int Bnnz, float *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_sscal(num_elements, 0.0f, X+p_offset, 1);
   for (int i = 0; i < Bnnz; ++i) {
      if ((Bi[i] >= n_start) && (Bi[i] <= n_stop)) {
	 X[Bi[i]*nrhs+Bj[i]] = Bx[i]; 
      }
   }
   _utsolvem<float>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}
void utsolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, 
   const int *Up, const int *Bi, const int *Bj,
   const double *Bx, int Bnnz, double *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_dscal(num_elements, 0.0f, X+p_offset, 1);
   for (int i = 0; i < Bnnz; ++i) {
      if ((Bi[i] >= n_start) && (Bi[i] <= n_stop)) {
	 X[Bi[i]*nrhs+Bj[i]] = Bx[i]; 
      }
   }
   _utsolvem<double>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}
/* solve U'X=B X and B are dense (**NxNRHS) solution matrices, U is sparse CSC format */
void utsolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, 
   const int *Up, const float *B, float *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_scopy(num_elements, B+p_offset, 1, X+p_offset, 1);
   _utsolvem<float>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}
void utsolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, 
   const int *Up, const double *B, double *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_dcopy(num_elements, B+p_offset, 1, X+p_offset, 1);
   _utsolvem<double>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}
/* solve U'X=B where X is dense (**NxNRHS) matrix and is converted from rhs to solution, U is sparse CSC format */
void utsolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, float *X)
{
   _utsolvem<float>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}
void utsolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, const int *Up, double *X)
{
   _utsolvem<double>(N, n_start, n_stop, nrhs, Ux, Ui, Up, X);
}


/*
SOLVERS FOR U_TRANSPOSE
FLOAT AND DOUBLE
SINGLE RHS
DENSE RHS OR SPARSE RHS
DENSE SOLUTION
*/
/* Kernel for utsolve */
template<typename T>
inline void _utsolve(int N, int n_start, int n_stop, const T *Ux, const int *Ui, const int *Up, T *x)
{
   // Ui -> Lj
   // Up -> Lp
   for (int j(n_start); j <= n_stop; ++j) {
      int p(Up[j]);
      int p_stop(Up[j+1]-1);
      T& xj(x[j]);

      /* Don't operate on x elements smaller than n_start row */
      while (Ui[p] < n_start) {++p;}
   
      for (; p < p_stop; ++p) {
         xj -= Ux[p]*x[Ui[p]];
      }
      xj /= Ux[p_stop];
   }
}
/* solve U'x=b where b is a sparse rhs and x is dense, U is sparse CSC format */ 
void utsolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, 
   const int *bi, const float *bx, int bnnz, float *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_sscal(num_elements, 0.0f, x+p_offset, 1);
   for (int i = 0; i < bnnz; ++i) {
      if ((bi[i] >= n_start) && (bi[i] <= n_stop)) {
	 x[bi[i]] = bx[i];
      }
   }
   _utsolve<float>(N, n_start, n_stop, Ux, Ui, Up, x);
}
void utsolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up,
   const int *bi, const double *bx, int bnnz, double *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_dscal(num_elements, 0.0f, x+p_offset, 1);
   for (int i = 0; i < bnnz; ++i) {
      if ((bi[i] >= n_start) && (bi[i] <= n_stop)) {
	 x[bi[i]] = bx[i];
      }
   }
   _utsolve<double>(N, n_start, n_stop, Ux, Ui, Up, x);
}
/* solve U'x=b where x and b are dense, U is sparse CSC format */
void utsolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, const float *b, float *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_scopy(num_elements, b+p_offset, 1, x+p_offset, 1);
   _utsolve<float>(N, n_start, n_stop, Ux, Ui, Up, x);
}
void utsolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, const double *b, double *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_dcopy(num_elements, b+p_offset, 1, x+p_offset, 1);
   _utsolve<double>(N, n_start, n_stop, Ux, Ui, Up, x);
}
/* solve U'x=b where x is dense and is converted from a rhs to a solution, U is sparse CSC format */
void utsolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, float *x)
{
   _utsolve<float>(N, n_start, n_stop, Ux, Ui, Up, x);
}
void utsolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, double *x)
{
   _utsolve<double>(N, n_start, n_stop, Ux, Ui, Up, x);
}








/*
SOLVERS FOR L
FLOAT AND DOUBLE
MULTIPLE RHS
DENSE RHS OR SPARSE RHS
DENSE SOLUTION
*/
/* Kernel for lsolvem */
template<typename T>
inline void _lsolvem(int N, int n_start, int n_stop, int nrhs, const T *Lx, const int *Li, const int *Lp, T *X)
{
   const int P1(nrhs);
   const int P2(1);
   for (int j(n_start); j <= n_stop; ++j) {
      int p(Lp[j]);
      int p_stop(Lp[j+1]-1);
      const T LX_START_inv(1.0/Lx[p++]);

      /* Don't operate on X elements smaller than n_start row */
      while (Li[p_stop] > n_stop) {--p_stop;}
   
      BLAS_SCALE(nrhs,&LX_START_inv,X+j*P1,P2);
   
      for (; p <= p_stop; ++p) {
         T LX_P(-Lx[p]);
         int LI_P(Li[p]*P1);
         BLAS_AXPY(nrhs,&LX_P,X+j*P1,P2,X+LI_P,P2);
      }
   }
}

/* solve LX=B where B is a sparse coord rhs matrix (**NxNRHS) and X is dense (**NxNRHS) solution matrix, L is sparse CSC format */
void lsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, const int *Bi, const int *Bj, const float *Bx, int Bnnz, float *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_sscal(num_elements, 0.0f, X+p_offset, 1);
   for (int i = 0; i < Bnnz; ++i) {
      if ((Bi[i] >= n_start) && (Bi[i] <= n_stop)) {
	 X[Bi[i]*nrhs+Bj[i]] = Bx[i]; 
      }
   }
   _lsolvem<float>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}
void lsolvem(int N, int n_start, int n_stop, 
   int nrhs, const double *Lx, const int *Li, 
   const int *Lp, const int *Bi, const int *Bj,
   const double *Bx, int Bnnz, double *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_dscal(num_elements, 0.0f, X+p_offset, 1);
   for (int i = 0; i < Bnnz; ++i) {
      if ((Bi[i] >= n_start) && (Bi[i] <= n_stop)) {
	 X[Bi[i]*nrhs+Bj[i]] = Bx[i]; 
      }
   }
   _lsolvem<double>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}

/* solve LX=B where X and B are dense (**NxNRHS) solution matrices, L is sparse CSC format */
void lsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, const float *B, float *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_scopy(num_elements, B+p_offset, 1, X+p_offset, 1);
   _lsolvem<float>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}
void lsolvem(int N, int n_start, int n_stop, 
   int nrhs, const double *Lx, const int *Li, 
   const int *Lp, const double *B, double *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_dcopy(num_elements, B+p_offset, 1, X+p_offset, 1);
   _lsolvem<double>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}

/* solve LX=B where X is dense (**NxNRHS) matrix which is converted from rhs to solution, L is sparse CSC format */
void lsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, float *X)
{
   _lsolvem<float>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}
void lsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, const int *Lp, double *X)
{
   _lsolvem<double>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}


/*
SOLVERS FOR L
FLOAT AND DOUBLE
SINGLE RHS
DENSE RHS OR SPARSE RHS
DENSE SOLUTION
*/
/* Kernel for lsolve */
template<typename T>
inline void _lsolve(int n_start, int n_stop, const T *Lx, const int *Li, const int *Lp, T *x)
{
   for (int j(n_start); j <= n_stop; ++j) {
      int p(Lp[j]);
      int p_stop(Lp[j+1]-1);
      T& xj(x[j]);

      while (Li[p_stop] > n_stop) {--p_stop;}
   
      xj /= Lx[p++];
      for (; p <= p_stop; ++p) {
         x[Li[p]] -= Lx[p]*xj ;
      }
   }
}

/* solve Lx=b where b is a sparse rhs and x is dense, L is sparse CSC format */ 
void lsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, 
   const int *bi, const float *bx, int bnnz, float *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_sscal(num_elements, 0.0f, x+p_offset, 1);
   for (int i = 0; i < bnnz; ++i) {
      if ((bi[i] >= n_start) && (bi[i] <= n_stop)) {
	 x[bi[i]] = bx[i];
      }
   }
   _lsolve<float>(n_start, n_stop, Lx, Li, Lp, x);
}
void lsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp,
   const int *bi, const double *bx, int bnnz, double *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_dscal(num_elements, 0.0f, x+p_offset, 1);
   for (int i = 0; i < bnnz; ++i) {
      if ((bi[i] >= n_start) && (bi[i] <= n_stop)) {
	 x[bi[i]] = bx[i];
      }
   }
   _lsolve<double>(n_start, n_stop, Lx, Li, Lp, x);
}

/* solve Lx=b where x and b are dense, L is sparse CSC format */
void lsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, const float *b, float *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_scopy(num_elements, b+p_offset, 1, x+p_offset, 1);
   _lsolve<float>(n_start, n_stop, Lx, Li, Lp, x);
}
void lsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, const double *b, double *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_dcopy(num_elements, b+p_offset, 1, x+p_offset, 1);
   _lsolve<double>(n_start, n_stop, Lx, Li, Lp, x);
}

/* solve Lx=b where x is dense and is converted from a rhs to a solution, L is sparse CSC format */
void lsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, float *x)
{
   _lsolve<float>(n_start, n_stop, Lx, Li, Lp, x);
}
void lsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, double *x)
{
   _lsolve<double>(n_start, n_stop, Lx, Li, Lp, x);
}


/*
SOLVERS FOR L_TRANSPOSE
FLOAT AND DOUBLE
MULTIPLE RHS
DENSE RHS OR SPARSE RHS
DENSE SOLUTION
*/
/* Kernel for ltsolvem */
template<typename T>
inline void _ltsolvem(int N, int n_start, int n_stop, int nrhs, const T *Lx, const int *Li, const int *Lp, T *X)
{
   // Li -> Uj
   // Lp -> Up
   const int P1(nrhs);
   const int P2(1);
   /* int P1(1), P2(N); */ /* for the less efficient case where X is NRHSxN matrix */
   for (int j(n_stop); j >= n_start; --j) {
      int p_start(Lp[j]);
      int p(Lp[j+1]-1);
      const T LX_START_inv(1.0/Lx[p_start]);

      /* Don't operate on X elements smaller than n_start row */
      while (Li[p] > n_stop) {--p;}
   
      for (; p > p_start; --p) {
         T LX_P(-Lx[p]);
         int LI_P(Li[p]*P1);
         BLAS_AXPY(nrhs,&LX_P,X+LI_P,P2,X+j*P1,P2);
      }
      BLAS_SCALE(nrhs,&LX_START_inv,X+j*P1,P2);
   }
}
/* solve L'X=B where B is a sparse coord rhs matrix (**NxNRHS) and X is dense (**NxNRHS) solution matrix, L is sparse CSC format */
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, 
   const int *Lp, const int *Bi, const int *Bj,
   const float *Bx, int Bnnz, float *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_sscal(num_elements, 0.0f, X+p_offset, 1);
   for (int i = 0; i < Bnnz; ++i) {
      if ((Bi[i] >= n_start) && (Bi[i] <= n_stop)) {
	 X[Bi[i]*nrhs+Bj[i]] = Bx[i]; 
      }
   }
   _ltsolvem<float>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, 
   const int *Lp, const int *Bi, const int *Bj,
   const double *Bx, int Bnnz, double *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_dscal(num_elements, 0.0f, X+p_offset, 1);
   for (int i = 0; i < Bnnz; ++i) {
      if ((Bi[i] >= n_start) && (Bi[i] <= n_stop)) {
	 X[Bi[i]*nrhs+Bj[i]] = Bx[i]; 
      }
   }
   _ltsolvem<double>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}
/* solve L'X=B X and B are dense (**NxNRHS) solution matrices, L is sparse CSC format */
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, 
   const int *Lp, const float *B, float *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_scopy(num_elements, B+p_offset, 1, X+p_offset, 1);
   _ltsolvem<float>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, 
   const int *Lp, const double *B, double *X)
{
   const int num_elements = nrhs*(n_stop-n_start+1);
   const int p_offset = n_start*nrhs;
   cblas_dcopy(num_elements, B+p_offset, 1, X+p_offset, 1);
   _ltsolvem<double>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}
/* solve L'X=B where X is dense (**NxNRHS) matrix and is converted from rhs to solution, L is sparse CSC format */
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, float *X)
{
   _ltsolvem<float>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, const int *Lp, double *X)
{
   _ltsolvem<double>(N, n_start, n_stop, nrhs, Lx, Li, Lp, X);
}


/*
SOLVERS FOR L_TRANSPOSE
FLOAT AND DOUBLE
SINGLE RHS
DENSE RHS OR SPARSE RHS
DENSE SOLUTION
*/
/* Kernel for ltsolve */
template<typename T>
inline void _ltsolve(int N, int n_start, int n_stop, const T *Lx, const int *Li, const int *Lp, T *x)
{
   // Li -> Uj
   // Lp -> Up
   for (int j(n_stop); j >= n_start; --j) {
      int p_start(Lp[j]);
      int p(Lp[j+1]-1);
      T& xj(x[j]);

      /* Don't operate on x elements smaller than n_start row */
      while (Li[p] > n_stop) {--p;}
   
      for (; p > p_start; --p) {
         xj -= Lx[p]*x[Li[p]];
      }
      xj /= Lx[p_start];
   }
}
/* solve L'x=b where b is a sparse rhs and x is dense, L is sparse CSC format */ 
void ltsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, 
   const int *bi, const float *bx, int bnnz, float *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_sscal(num_elements, 0.0f, x+p_offset, 1);
   for (int i = 0; i < bnnz; ++i) {
      if ((bi[i] >= n_start) && (bi[i] <= n_stop)) {
	 x[bi[i]] = bx[i];
      }
   }
   _ltsolve<float>(N, n_start, n_stop, Lx, Li, Lp, x);
}
void ltsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp,
   const int *bi, const double *bx, int bnnz, double *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_dscal(num_elements, 0.0f, x+p_offset, 1);
   for (int i = 0; i < bnnz; ++i) {
      if ((bi[i] >= n_start) && (bi[i] <= n_stop)) {
	 x[bi[i]] = bx[i];
      }
   }
   _ltsolve<double>(N, n_start, n_stop, Lx, Li, Lp, x);
}
/* solve L'x=b where x and b are dense, L is sparse CSC format */
void ltsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, const float *b, float *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_scopy(num_elements, b+p_offset, 1, x+p_offset, 1);
   _ltsolve<float>(N, n_start, n_stop, Lx, Li, Lp, x);
}
void ltsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, const double *b, double *x)
{
   const int num_elements = n_stop-n_start+1;
   const int p_offset = n_start;
   cblas_dcopy(num_elements, b+p_offset, 1, x+p_offset, 1);
   _ltsolve<double>(N, n_start, n_stop, Lx, Li, Lp, x);
}
/* solve L'x=b where x is dense and is converted from a rhs to a solution, L is sparse CSC format */
void ltsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, float *x)
{
   _ltsolve<float>(N, n_start, n_stop, Lx, Li, Lp, x);
}
void ltsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, double *x)
{
   _ltsolve<double>(N, n_start, n_stop, Lx, Li, Lp, x);
}
