#include <merlion/BlasWrapper.hpp>


#ifdef TEVA_SPOT_HAVE_BLAS

// macro for converting fortran name to C name
//#define F77_FUNC(name) name
#define F77_FUNC(name) name ## _
//#define F77_FUNC(name) name ## __

///
// In order to make sure Merlion compiles on machines that have blas and
// machines that do not, we provide a (likely slow) default
// implementation of the routines that we use. If blas is present, we
// use the blas library routines instead of our implementations.  - to
// minimize the effort in implementing these methods, we only implement
// the methods we use and comment out the rest. If another method is
// needed, its default implementation should be done at that time.
///

extern "C"
{
   //  double F77_FUNC(ddot)(int *n, const double *x, int *incX, const double *y, int *incY);

   //  double F77_FUNC(dnrm2)(int *n, const double *x, int *incX);

   //  double F77_FUNC(dasum)(int *n, const double *x, int *incX);

   //  int F77_FUNC(idamax)(int *n, const double *x, int *incX);

   void F77_FUNC(dcopy)(int *n, const double *x, int *incX, double *y, int *incY);

   void F77_FUNC(scopy)(int *n, const float *x, int *incX, float *y, int *incY);

   void F77_FUNC(daxpy)(int *n, const double *alpha, const double *x, int *incX, double *y, int *incY);

   void F77_FUNC(saxpy)(int *n, const float *alpha, const float *x, int *incX, float *y, int *incY);

   void F77_FUNC(dscal)(int *n, const double *alpha, const double *x, int *incX);

   void F77_FUNC(sscal)(int *n, const float *alpha, const float *x, int *incX);

   //  void F77_FUNC(dgemv)(char* trans, int *m, int *n, const double *alpha, const double *a, int *lda,
   //		       const double *x, int *incX, const double *beta, double *y, int *incY, int trans_len);

   //  void F77_FUNC(dsymv)(char* uplo, int *n, const double *alpha, const double *a, int *lda,
   //		       const double *x, int *incX, const double *beta, double *y, int *incY, int uplo_len);

   //  void F77_FUNC(dgemm)(char* transa, char* transb, int *m, int *n, int *k, const double *alpha, const double *a, int *lda,
   //		       const double *b, int *ldb, const double *beta, double *c, int *ldc, int transa_len, int transb_len);

   //  void F77_FUNC(dsyrk)(char* uplo, char* trans, int *n, int *k, const double *alpha, const double *a, int *lda,
   //		       const double *beta, double *c, int *ldc, int uplo_len, int trans_len);

   //  void F77_FUNC(dtrsm)(char* side, char* uplo, char* transa, char* diag, int *m, int *n, const double *alpha, 
   //		       const double *a, int *lda, const double *b, int *ldb, int side_len, int uplo_len,
   //		       int transa_len, int diag_len);

   //  void F77_FUNC(dsyr)(char* uplo, int *n, const double *alpha, const double *x, int *incX,
   //		      const double *a, int *lda, int uplo_len);

   //  void F77_FUNC(dsyr2)(char* uplo, int *n, const double *alpha, const double *x, int *incX,
   //		       const double *y, int *incY, const double *a, int *lda, int uplo_len);
} // extern "C"


/*
double cblas_ddot(int size, const double *x, int incX, const double *y,
   int incY)
{
   int n=size, incX_=incX, incY_=incY;
   return F77_FUNC(ddot)(&n, x, &incX_, y, &incY_);
}

double cblas_dnrm2(int size, const double *x, int incX)
{
   int n=size, incX_=incX;
   return F77_FUNC(dnrm2)(&n, x, &incX_);
}

double cblas_dasum(int size, const double *x, int incX)
{
   int n=size, incX_=incX;
   return F77_FUNC(dasum)(&n, x, &incX_);
}

int cblas_idamax(int size, const double *x, int incX)
{
   int n=size, incX_=incX;  
   return (int) F77_FUNC(idamax)(&n, x, &incX_);
}
*/
void cblas_dcopy(int size, const double *x, int incX, double *y, int incY)
{
   int N=size, incX_=incX, incY_=incY;  
   F77_FUNC(dcopy)(&N, x, &incX_, y, &incY_);
}

void cblas_scopy(int size, const float *x, int incX, float *y, int incY)
{
   int N=size, incX_=incX, incY_=incY; 
   F77_FUNC(scopy)(&N, x, &incX_, y, &incY_);
}

void cblas_daxpy(int size, double alpha, const double *x, int incX, double *y,
   int incY)
{
   int N=size, incX_=incX, incY_=incY;  
   F77_FUNC(daxpy)(&N, &alpha, x, &incX_, y, &incY_);
}

void cblas_saxpy(int size, float alpha, const float *x, int incX, float *y,
   int incY)
{
   int N=size, incX_=incX, incY_=incY;  
   F77_FUNC(saxpy)(&N, &alpha, x, &incX_, y, &incY_);
}

void cblas_dscal(int size, double alpha, double *x, int incX)
{
   int N=size, incX_=incX;  
   F77_FUNC(dscal)(&N, &alpha, x, &incX_);
}

void cblas_sscal(int size, float alpha, float *x, int incX)
{
   int N=size, incX_=incX;  
   F77_FUNC(sscal)(&N, &alpha, x, &incX_);
}
/*
void cblas_dgemv(bool trans, int nRows, int nCols, double alpha,
   const double* A, int ldA, const double* x,
   int incX, double beta, double* y, int incY)
{
   int M=nRows, N=nCols, LDA=ldA, incX_=incX, incY_=incY;

   char TRANS;
   if (trans) {
      TRANS = 'T';
   }
   else {
      TRANS = 'N';
   }

   F77_FUNC(dgemv)(&TRANS, &M, &N, &alpha, A, &LDA, x,
      &incX_, &beta, y, &incY_, 1);
}

void cblas_dsymv(int cblascolmajor, int cblaslower, int n, double alpha, const double* A, int ldA,
   const double* x, int incX, double beta, double* y, int incY)
{
   int N=n, LDA=ldA, incX_=incX, incY_=incY;

   char UPLO='L';

   F77_FUNC(dsymv)(&UPLO, &N, &alpha, A, &LDA, x,
      &incX_, &beta, y, &incY_, 1);
}

void cblas_dsyr(int cblascolmajor, int cblaslower, int n, double alpha, const double* x, int incX, double* A, int ldA)
{
   int N=n, incX_=incX, LDA=ldA;
   char UPLO='L';

   F77_FUNC(dsyr)(&UPLO, &N, &alpha, x, &incX_, A, &LDA, 1);
}

void cblas_dsyr2(int cblascolmajor, int cblaslower, int n, double alpha, const double* x, int incX, 
   const double* y, int incY, double* A, int ldA)
{
   int N=n, incX_=incX, incY_=incY, LDA=ldA;
   char UPLO='L';

   F77_FUNC(dsyr2)(&UPLO, &N, &alpha, x, &incX_, y, &incY_, A, &LDA, 1);
}

void cblas_dgemm(bool transa, bool transb, int m, int n, int k,
   double alpha, const double* A, int ldA, const double* B,
   int ldB, double beta, double* C, int ldC)
{
   int M=m, N=n, K=k, LDA=ldA, LDB=ldB, LDC=ldC;

   char TRANSA;
   if (transa) {
      TRANSA = 'T';
   }
   else {
      TRANSA = 'N';
   }
   char TRANSB;
   if (transb) {
      TRANSB = 'T';
   }
   else {
      TRANSB = 'N';
   }

   F77_FUNC(dgemm)(&TRANSA, &TRANSB, &M, &N, &K, &alpha, A, &LDA,
      B, &LDB, &beta, C, &LDC, 1, 1);
}

void cblas_dsyrk(bool trans, int ndim, int nrank,
   double alpha, const double* A, int ldA,
   double beta, double* C, int ldC)
{
   int N=ndim, K=nrank, LDA=ldA, LDC=ldC;

   char UPLO='L';
   char TRANS;
   if (trans) {
      TRANS = 'T';
   }
   else {
      TRANS = 'N';
   }

   F77_FUNC(dsyrk)(&UPLO, &TRANS, &N, &K, &alpha, A, &LDA,
      &beta, C, &LDC, 1, 1);
}

void cblas_dtrsm(bool trans, int ndim, int nrhs, double alpha,
   const double* A, int ldA, double* B, int ldB)
{
   int M=ndim, N=nrhs, LDA=ldA, LDB=ldB;

   char SIDE = 'L';
   char UPLO = 'L';
   char TRANSA;
   if (trans) {
      TRANSA = 'T';
   }
   else {
      TRANSA = 'N';
   }
   char DIAG = 'N';

   F77_FUNC(dtrsm)(&SIDE, &UPLO, &TRANSA, &DIAG, &M, &N,
      &alpha, A, &LDA, B, &LDB, 1, 1, 1, 1);
}
*/

#endif
