// Test suite for the Merlion fall-back blas implementations
#include <cxxtest/TestSuite.h>

#include <merlion/BlasWrapper.hpp>

template <typename T>
bool ArrayIsEqual(T *x, T val, int N)
{
   for (int i = 0; i < N; ++i) {
      if(x[i] != val) {
         return false;       
      }     
   }
   return true;
}

template <typename T>
bool ArraysAreEqual(T *x, T *y, int N)
{
   for (int i = 0; i < N; ++i) {
      if(x[i] != y[i]) {
         return false;       
      }     
   }
   return true;
}

template<typename T>
void _test_copy_0_incX_1_incY()
{
   int N = 5;

   T *y(new T[N]);
   for (int i = 0; i < N; ++i) {y[i] = 1.0;}       
   T x = 0.0;
   
   NO_BLAS_cblas_copy<T>(N, &x, 0, y, 1);

   TS_ASSERT(ArrayIsEqual(y,x,N));  
}

template<typename T>
void _test_copy_1_incX_1_incY()
{
   int N = 5;
      
   T *y(new T[N]);
   for (int i = 0; i < N; ++i) {y[i] = 1.0;} 
   
   T *x(new T[N]);
   for (int i = 0; i < N; ++i) {x[i] = 0.0;} 
      
   NO_BLAS_cblas_copy<T>(N, x, 1, y, 1);
      
   TS_ASSERT(ArrayIsEqual(y,x[0],N));
}

template<typename T>
void _test_scal_1_incX()
{
   int N = 5;

   T *x(new T[N]);
   for (int i = 0; i < N; ++i) {x[i] = 1.0;}       
   T s = 0.0;
   
   NO_BLAS_cblas_scal<T>(N, s, x, 1);
   TS_ASSERT(ArrayIsEqual(x,s,N));
   
   // check the we are actually scaling
   s = 1.0;
   NO_BLAS_cblas_scal<T>(N, s, x, 1);
   s = 0.0;
   TS_ASSERT(ArrayIsEqual(x,s,N));
}

template<typename T>
void _test_scal_0_incX()
{
   int N = 3;

   T *x(new T[N]);
   for (int i = 0; i < N; ++i) {x[i] = 1.0;}       
   T s = 2.0;
   
   NO_BLAS_cblas_scal<T>(N, s, x, 0);
   s = 8.0;
   TS_ASSERT(ArrayIsEqual(x,s,1));
}

template<typename T>
void _test_scal_n_incX()
{
   int N = 6;

   T *x(new T[N]);
   for (int i = 0; i < N-1; ++i) {x[i] = 1.0;}
   x[N-1] = 0.0;
   
   T s = 2.0;
   NO_BLAS_cblas_scal<T>(2, s, x, 2); // incX = 2
   s = 3.0;
   NO_BLAS_cblas_scal<T>(2, s, x+1, 3); // incX = 3
   s = 1.0;
   NO_BLAS_cblas_scal<T>(1, s, x+N-1, 1); // check that we actually scale.

   T *x_base(new T[N]);
   x_base[0] = 2.0;
   x_base[1] = 3.0;
   x_base[2] = 2.0;
   x_base[3] = 1.0;
   x_base[4] = 3.0;
   x_base[5] = 0.0;

   TS_ASSERT(ArraysAreEqual(x,x_base,N));  
}

template<typename T>
void _test_axpy_1_incX_1_incY()
{
   int N = 5;

   T *x(new T[N]);
   for (int i = 0; i < N; ++i) {x[i] = 1.0;}
   T *y(new T[N]);
   for (int i = 0; i < N; ++i) {y[i] = 0.0;}
   T s = 2.0;
   
   NO_BLAS_cblas_axpy<T>(N, s, x, 1, y, 1);
   
   TS_ASSERT(ArrayIsEqual(y,s,N));
}

template<typename T>
void _test_axpy_n_incX_n_incY()
{
   int N = 5;
   T *x(new T[N]);
   for (int i = 0; i < N; ++i) {x[i] = (T)(i);}
   T *y(new T[N]);
   for (int i = 0; i < N; ++i) {y[i] = 0.0;}
 
   T s = 2.0;
   NO_BLAS_cblas_axpy<T>(3, s, x, 2, y, 0);
   s = 3.0;
   x[0] = 3.0;
   NO_BLAS_cblas_axpy<T>(2, s, x, 0, y+1, 2);
   
   T *y_base(new T[N]);
   y_base[0] = 12.0;
   y_base[1] = 9.0;
   y_base[2] = 0.0;
   y_base[3] = 9.0;
   y_base[4] = 0.0;
   TS_ASSERT(ArraysAreEqual(y,y_base,N));
   
}

class BlasWrapperTests : public CxxTest::TestSuite
{
 public:


   //// test the double precision version of the fall back blas functions
   // cblas_dcopy   
   void test_dcopy_0_incX_1_incY() {_test_copy_0_incX_1_incY<double>();}
   void test_dcopy_1_incX_1_incY() {_test_copy_1_incX_1_incY<double>();}
   // cblas_dscal
   void test_dscal_0_incX() {_test_scal_0_incX<double>();}
   void test_dscal_1_incX() {_test_scal_1_incX<double>();}
   void test_dscal_n_incX() {_test_scal_n_incX<double>();}
   // cblas_daxpy
   void test_daxpy_1_incX_1_incY() {_test_axpy_1_incX_1_incY<double>();}
   void test_daxpy_n_incX_n_incY() {_test_axpy_n_incX_n_incY<double>();}
   ////
   
   //// test the single precision version of the fall back blas functions
   // cblas_scopy   
   void test_scopy_0_incX_1_incY() {_test_copy_0_incX_1_incY<float>();}
   void test_scopy_1_incX_1_incY() {_test_copy_1_incX_1_incY<float>();}
   // cblas_sscal
   void test_sscal_0_incX() {_test_scal_0_incX<float>();}
   void test_sscal_1_incX() {_test_scal_1_incX<float>();}
   void test_sscal_n_incX() {_test_scal_n_incX<float>();}
   // cblas_saxpy
   void test_saxpy_1_incX_1_incY() {_test_axpy_1_incX_1_incY<float>();}
   void test_saxpy_n_incX_n_incY() {_test_axpy_n_incX_n_incY<float>();}
   ////

   // test that the copy routine of this machines
   // BLAS implementation supports xinc = 0
   // This is a common problem on OSX Lion
   void test_copy_zero_inc()
   {
      int N = 1000;
      float tmp_value_s = -2.5f;

      float *ys = new float[N];
      for (int i = 0; i < N; ++i) {ys[i] = i+1;}

      cblas_scopy(N,&tmp_value_s,0,ys,1);

      for (int i = 0; i < N; ++i) {
	 if (ys[i] != tmp_value_s) {
	    TS_FAIL("The BLAS implementation being used does not support a zero increment in the SCOPY routine.");
	    TS_TRACE("It may be necessary to specify an alternate BLAS implementation in order for Merlion to work as expected.");
	    break;
	 }
      }

      double tmp_value_d = -2.5;

      double *yd = new double[N];
      for (int i = 0; i < N; ++i) {yd[i] = i+1;}

      cblas_dcopy(N,&tmp_value_d,0,yd,1);

      for (int i = 0; i < N; ++i) {
	 if (yd[i] != tmp_value_d) {
	    TS_FAIL("The BLAS implementation being used does not support a zero increment in the DCOPY routine.");	
	    TS_TRACE("It may be necessary to specify an alternate BLAS implementation in order for Merlion to work as expected.");
	    break;
	 }
      }  
   }

};
