#ifndef MERLION_BLASWRAPPER_HPP__
#define MERLION_BLASWRAPPER_HPP__

///
// In order to make sure Merlion compiles on machines that have blas and
// machines that do not, we provide a (likely slow) default
// implementation of the routines that we use. If blas is present, we
// use the blas library routines instead of our implementations.  - to
// minimize the effort in implementing these methods, we only implement
// the methods we use and comment out the rest. If another method is
// needed, its default implementation should be done at that time.
///
#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

// forward declare the fall-back blas implementations
template<typename T>
inline void 
NO_BLAS_cblas_copy(int size, const T *x, int incX, T *y, int incY);
template<typename T>
inline void
NO_BLAS_cblas_axpy(int size, T alpha, const T *x, int incX, T *y, int incY);
template<typename T>
inline void
NO_BLAS_cblas_scal(int size, T alpha, T *x, int incX);

#ifdef TEVA_SPOT_HAVE_BLAS

/*
// calculate x'*y
double cblas_ddot(int size, const double *x, int incX, const double *y, int incY);

// calculate 2-norm of x
double cblas_dnrm2(int size, const double *x, int incX);

// calculate absolute sum
double cblas_dasum(int size, const double *x, int incX);

// return idx for element of x that is largest in the absolute sense
int cblas_idamax(int size, const double *x, int incX);

*/
// copy from x to y (doubles)
void cblas_dcopy(int size, const double *x, int incX, double *y,
   int incY);

// copy from x to y (float)
void cblas_scopy(int size, const float *x, int incX,float *y,
   int incY);

// y=a*x+y (double)
void cblas_daxpy(int size, double alpha, const double *x, int incX,
   double *y, int incY);

// y=a*x+y (float)
void cblas_saxpy(int size, float alpha, const float *x, int incX,
   float *y, int incY);

// x = alpha*x (double)
void cblas_dscal(int size, double alpha, double *x, int incX);

// x = alpha*x (float)
void cblas_sscal(int size,float alpha,float *x, int incX);

/*
// y = alpha*A*x + beta*y (double)
void cblas_dgemv(bool trans, int nRows, int nCols, double alpha,
   const double* A, int ldA, const double* x,
   int incX, double beta, double* y, int incY);

// y = alpha*A*x + beta*y (where A is symmetric)
void cblas_dsymv(int cblascolmajor, int cblaslower, int n, double alpha, const double* A, int ldA,
   const double* x, int incX, double beta, double* y,
   int incY);

// C = alpha*A*B + beta*C (matrix-matrix multiply)
void cblas_dgemm(bool transa, bool transb, int m, int n, int k,
   double alpha, const double* A, int ldA, const double* B,
   int ldB, double beta, double* C, int ldC);

// C = alpha*A*A' + beta*C (symmetric rank-k update)
void cblas_dsyrk(bool trans, int ndim, int nrank,
   double alpha, const double* A, int ldA,
   double beta, double* C, int ldC);

// solve triangular system
// A*X = alpha*B, or X*A = alpha*B,
void cblas_dtrsm(bool trans, int ndim, int nrhs, double alpha,
   const double* A, int ldA, double* B, int ldB);

// A = alpha*x*x' + A (symmetric rank-1 update)
void cblas_dsyr(int cblascolmajor, int cblaslower, int n, double alpha, const double* x, int incX, double* A, int ldA);

// A = alpha*x*y' + alpha*y*x' + A (symmetric rank-2 update)
void cblas_dsyr2(int cblascolmajor, int cblaslower, int n, double alpha, const double* x, int incX, const double* y, int incY, double* A, int ldA);
*/

#else // Our default BLAS implementations

inline void 
cblas_dcopy(int size, const double *x, int incX, double *y, int incY)
{
   NO_BLAS_cblas_copy<double>(size, x, incX, y, incY);
}

inline void 
cblas_scopy(int size, const float *x, int incX, float *y, int incY)
{
   NO_BLAS_cblas_copy<float>(size, x, incX, y, incY);

}

inline void
cblas_daxpy(int size, double alpha, const double *x, int incX, double *y, int incY)
{
   NO_BLAS_cblas_axpy<double>(size, alpha, x, incX, y, incY);
}

inline void
cblas_saxpy(int size, float alpha, const float *x, int incX, float *y,
   int incY)
{
   NO_BLAS_cblas_axpy<float>(size, alpha, x, incX, y, incY);
}

inline void
cblas_dscal(int size, double alpha, double *x, int incX)
{
   NO_BLAS_cblas_scal<double>(size, alpha, x, incX);
}

inline void 
cblas_sscal(int size, float alpha, float *x, int incX)
{
   NO_BLAS_cblas_scal<float>(size, alpha, x, incX);
}

#endif // defined TEVA_SPOT_HAVE_BLAS

inline void BlasResetZero(int steps, double *x)
{
   cblas_dscal(steps,0.0,x,1);
}

inline void BlasResetZero(int steps, float *x)
{
   cblas_sscal(steps,0.0f,x,1);
   //memset(x,0.0f,steps*sizeof(float));
   //float tmp_zero = 0.0f;
   //memcpy(x,&tmp_zero, steps*sizeof(float));
   //bzero(x,steps*sizeof(float));
}

inline void BlasResetZero(int steps, int *x)
{
   int *stop = x+steps;
   while (x != stop) {*x++ = 0;}
}

inline void BlasInitZero(int steps, double *x)
{   
   double tmp_zero = 0.0;
   cblas_dcopy(steps,&tmp_zero,0,x,1);
}

inline void BlasInitZero(int steps, float *x)
{   
   float tmp_zero = 0.0f;
   cblas_scopy(steps,&tmp_zero,0,x,1);
}

inline void BlasInitZero(int steps, int *x)
{   
   int *stop = x+steps;
   while (x != stop) {*x++ = 0;}
}


template<typename T>
inline void 
NO_BLAS_cblas_copy(int size, const T *x, int incX, T *y, int incY)
{
   const T* y_max = y + size*incY;
   while ( y != y_max )
   {
      *y = *x;
      y += incY;
      x += incX;
   }
}

template<typename T>
inline void
NO_BLAS_cblas_axpy(int size, T alpha, const T *x, int incX, T *y,
   int incY)
{
   if ( incX == 1 && incY == 1 )
   {
      const T* x_max = x + size;
      while ( x != x_max )
      {
         *(y++) += alpha * *(x++);
      }
   }
   else
   {
      for (int i = 0; i < size; ++i)
      {
         *y += alpha * *x;
         y += incY;
         x += incX;
      }
   }
}

template<typename T>
inline void
NO_BLAS_cblas_scal(int size, T alpha, T *x, int incX)
{
   if ( incX == 1 )
   {
      const T* x_max = x + size;
      while ( x != x_max )
      {
         *(x++) *= alpha;
      }
   }
   else if ( incX == 0)
   {
      T& xv = *x;
      for (int i = 0; i < size; ++i)
      {
         xv *= alpha; 
      }
   }
   else
   {
      const T* x_max = x + size*incX;
      while ( x != x_max )
      {
         *x *= alpha;
         x += incX;
      }
   }
}
#endif // defined BLASWRAPPER_HPP__
