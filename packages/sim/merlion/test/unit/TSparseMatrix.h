// Test suite for the Merlion upper triangular linear solver
#include <cxxtest/TestSuite.h>

#include <merlion/SparseMatrix.hpp>
#include <fstream>
#include <cstdio>

//TODO: Add file IO tests. Add tests with nonsquare matrix.

namespace {

template <typename T, typename SIZE_T>
bool _ArraysAreEqual(T* x,T* y, SIZE_T N)
{
   // Check array values are the same
   if ((x == NULL) && !(y == NULL)) {return false;}
   if (!(x == NULL) && (y == NULL)) {return false;}
   if ((x == NULL) && (y == NULL)) {return true;}
   for (SIZE_T i = 0; i < N; ++i) {
      if(x[i] != y[i]) {
	 std::cout.precision(10);
	 std::cout << x[i] << " != " << y[i] << std::endl;
	 std::cout.precision();
         return false;
      } 
   }
   return true;
}

SparseMatrix* _copy_matrix(SparseMatrix *orig) 
{
   SparseMatrix *copy(new SparseMatrix);
   if (orig->isCSRMatrix()) {
      copy->AllocateNewCSRMatrix(orig->NRows(), orig->NCols(), orig->NNonzeros());
      for (int i = 0, i_stop = orig->NRows()+1; i < i_stop; ++i) {
	 copy->pRows()[i] = orig->pRows()[i];
      }
      for (int k = 0, k_stop = orig->NNonzeros(); k < k_stop; ++k) {
	 copy->jCols()[k] = orig->jCols()[k];
      }
   }
   else if (orig->isCSCMatrix()) {
      copy->AllocateNewCSCMatrix(orig->NRows(), orig->NCols(), orig->NNonzeros());
      for (int j = 0, j_stop = orig->NCols()+1; j < j_stop; ++j) {
	 copy->pCols()[j] = orig->pCols()[j];
      }
      for (int k = 0, k_stop = orig->NNonzeros(); k < k_stop; ++k) {
	 copy->iRows()[k] = orig->iRows()[k];
      }
   }
   else if (orig->isCOOMatrix()) {
      copy->AllocateNewCOOMatrix(orig->NRows(), orig->NCols(), orig->NNonzeros());
      for (int k = 0, k_stop = orig->NNonzeros(); k < k_stop; ++k) {
	 copy->iRows()[k] = orig->iRows()[k];
	 copy->jCols()[k] = orig->jCols()[k];
      }
   }
   else {
      return NULL;
   }
   for (int k = 0, k_stop = orig->NNonzeros(); k < k_stop; ++k) {
      copy->Values()[k] = orig->Values()[k];
   }
   return copy;
}

bool _isSame(SparseMatrix *m1, SparseMatrix *m2)
{
   if (m1->isCSRMatrix() != m2->isCSRMatrix()) {return false;}
   if (m1->isCSCMatrix() != m2->isCSCMatrix()) {return false;}
   if (m1->isCOOMatrix() != m2->isCOOMatrix()) {return false;}
   if (m1->NRows() != m2->NRows()) {return false;}
   if (m1->NCols() != m2->NCols()) {return false;}
   if (m1->NNonzeros() != m2->NNonzeros()) {return false;}
   if (!::_ArraysAreEqual(m1->iRows(),m2->iRows(),m1->NNonzeros())) {return false;}
   if (!::_ArraysAreEqual(m1->pRows(),m2->pRows(),m1->NRows()+1)) {return false;}
   if (!::_ArraysAreEqual(m1->jCols(),m2->jCols(),m1->NNonzeros())) {return false;}
   if (!::_ArraysAreEqual(m1->pCols(),m2->pCols(),m1->NCols()+1)) {return false;}
   if (!::_ArraysAreEqual(m1->Values(),m2->Values(),m1->NNonzeros())) {return false;}
   return true;
}

void _test_all_conversion(SparseMatrix *tm_)
{
   SparseMatrix* copy1 = ::_copy_matrix(tm_);
   TS_ASSERT(::_isSame(tm_,copy1));
   
   tm_->TransformToCSRMatrix();
   tm_->TransformToCOOMatrix();
   tm_->TransformToCSCMatrix();
   tm_->TransformToCOOMatrix();
   tm_->TransformToCSRMatrix();
   tm_->TransformToCSCMatrix();
   tm_->TransformToCSRMatrix();
   tm_->TransformToCOOMatrix();
   tm_->TransformToCSCMatrix();
   tm_->TransformToCOOMatrix();
   tm_->TransformToCSRMatrix();
   tm_->TransformToCSCMatrix();
   
   SparseMatrix* copy2 = ::_copy_matrix(tm_);
   TS_ASSERT(::_isSame(tm_,copy2));
   TS_ASSERT(::_isSame(copy1,copy2));
}

void _test_CSC_IO(SparseMatrix *tm_)
{
   tm_->TransformToCSCMatrix();
   std::ofstream out;
   out.open("TSparseMatrix_tmp.txt",std::ios::out|std::ios::trunc);
   out.precision(20);
   tm_->PrintCSCMatrix(out,"tester");
   out.close();
   
   std::ifstream in;
   in.open("TSparseMatrix_tmp.txt",std::ios::in);
   SparseMatrix* copy_tm(new SparseMatrix);
   copy_tm->ReadCSCMatrix(in);
   in.close();
   
   //check that matrices are the same
   TS_ASSERT(::_isSame(tm_,copy_tm));
   
   //temporary test files
   remove("TSparseMatrix_tmp.txt");
}

void _test_CSC_IO_Binary(SparseMatrix *tm_)
{
   bool binary = true;
   tm_->TransformToCSCMatrix();
   std::ofstream out;
   out.open("TSparseMatrix_tmp.bin",std::ios::out|std::ios::trunc|std::ios::binary);
   tm_->PrintCSCMatrix(out,"tester",binary);
   out.close();
   
   std::ifstream in;
   in.open("TSparseMatrix_tmp.bin",std::ios::in|std::ios::binary);
   SparseMatrix* copy_tm(new SparseMatrix);
   copy_tm->ReadCSCMatrix(in,binary);
   in.close();
   
   //check that matrices are the same
   TS_ASSERT(::_isSame(tm_,copy_tm));
   
   //temporary test files
   remove("TSparseMatrix_tmp.bin");
}

void _test_CSR_IO(SparseMatrix *tm_)
{
   tm_->TransformToCSRMatrix();
   std::ofstream out;
   out.open("TSparseMatrix_tmp.txt",std::ios::out|std::ios::trunc);
   out.precision(20);
   tm_->PrintCSRMatrix(out,"tester");
   out.close();
   
   std::ifstream in;
   in.open("TSparseMatrix_tmp.txt",std::ios::in);
   SparseMatrix* copy_tm(new SparseMatrix);
   copy_tm->ReadCSRMatrix(in);
   in.close();
   
   //check that matrices are the same
   TS_ASSERT(::_isSame(tm_,copy_tm));
   
   //temporary test files
   remove("TSparseMatrix_tmp.txt");
}

void _test_CSR_IO_Binary(SparseMatrix *tm_)
{
   bool binary = true;
   tm_->TransformToCSRMatrix();
   std::ofstream out;
   out.open("TSparseMatrix_tmp.bin",std::ios::out|std::ios::trunc|std::ios::binary);
   tm_->PrintCSRMatrix(out,"tester",binary);
   out.close();
   
   std::ifstream in;
   in.open("TSparseMatrix_tmp.bin",std::ios::in|std::ios::binary);
   SparseMatrix* copy_tm(new SparseMatrix);
   copy_tm->ReadCSRMatrix(in,binary);
   in.close();
   
   //check that matrices are the same
   TS_ASSERT(::_isSame(tm_,copy_tm));
   
   //temporary test files
   remove("TSparseMatrix_tmp.bin");
}

void _test_COO_IO(SparseMatrix *tm_)
{
   tm_->TransformToCOOMatrix();
   std::ofstream out;
   out.open("TSparseMatrix_tmp.txt",std::ios::out|std::ios::trunc);
   out.precision(20);
   tm_->PrintCOOMatrix(out,"tester");
   out.close();
   
   std::ifstream in;
   in.open("TSparseMatrix_tmp.txt",std::ios::in);
   SparseMatrix* copy_tm(new SparseMatrix);
   copy_tm->ReadCOOMatrix(in);
   in.close();
   
   //check that matrices are the same
   TS_ASSERT(::_isSame(tm_,copy_tm));
   
   //temporary test files
   remove("TSparseMatrix_tmp.txt");
}

void _test_COO_IO_Binary(SparseMatrix *tm_)
{
   bool binary = true;
   tm_->TransformToCOOMatrix();
   std::ofstream out;
   out.open("TSparseMatrix_tmp.bin",std::ios::out|std::ios::trunc|std::ios::binary);
   tm_->PrintCOOMatrix(out,"tester",binary);
   out.close();
   
   std::ifstream in;
   in.open("TSparseMatrix_tmp.bin",std::ios::in|std::ios::binary);
   SparseMatrix* copy_tm(new SparseMatrix);
   copy_tm->ReadCOOMatrix(in,binary);
   in.close();
   
   //check that matrices are the same
   TS_ASSERT(::_isSame(tm_,copy_tm));
   
   //temporary test files
   remove("TSparseMatrix_tmp.bin");
}


}; // end of empty namespace

///////////////////////////// Begin Testing Suits

class SparseMatrixIOTestsLargeUpperSquare : public CxxTest::TestSuite
{
 public:

   SparseMatrix *tm_; 
   
   static SparseMatrixIOTestsLargeUpperSquare* createSuite()
   {
      std::ifstream in;
      SparseMatrixIOTestsLargeUpperSquare *suite(new SparseMatrixIOTestsLargeUpperSquare);
      suite->tm_ = new SparseMatrix;
      in.open("LargeTestMatrixUpper.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->ReadCSCMatrix(in);
      in.close();
      return suite;
   }

   static void destroySuite(SparseMatrixIOTestsLargeUpperSquare* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   // Tests
   void test_all_conversion() {::_test_all_conversion(tm_);}
   void test_CSC_IO() {::_test_CSC_IO(tm_);}
   void test_CSC_IO_Binary() {::_test_CSC_IO_Binary(tm_);}
   void test_CSR_IO() {::_test_CSR_IO(tm_);}
   void test_CSR_IO_Binary() {::_test_CSR_IO_Binary(tm_);}
   void test_COO_IO() {::_test_COO_IO(tm_);}
   void test_COO_IO_Binary() {::_test_COO_IO_Binary(tm_);}

};

class SparseMatrixIOTestsSmallUpperSquare : public CxxTest::TestSuite
{
 public:

   SparseMatrix *tm_; 
   
   static SparseMatrixIOTestsSmallUpperSquare* createSuite()
   {
      std::ifstream in;
      SparseMatrixIOTestsSmallUpperSquare *suite(new SparseMatrixIOTestsSmallUpperSquare);
      suite->tm_ = new SparseMatrix;
      in.open("SmallTestMatrixUpper.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->ReadCSCMatrix(in);
      in.close();
      return suite;
   }

   static void destroySuite(SparseMatrixIOTestsSmallUpperSquare* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   // Tests
   void test_all_conversion() {::_test_all_conversion(tm_);}
   void test_CSC_IO() {::_test_CSC_IO(tm_);}
   void test_CSC_IO_Binary() {::_test_CSC_IO_Binary(tm_);}
   void test_CSR_IO() {::_test_CSR_IO(tm_);}
   void test_CSR_IO_Binary() {::_test_CSR_IO_Binary(tm_);}
   void test_COO_IO() {::_test_COO_IO(tm_);}
   void test_COO_IO_Binary() {::_test_COO_IO_Binary(tm_);}

};


class SparseMatrixIOTestsSmallNonsquare1 : public CxxTest::TestSuite
{
 public:

   SparseMatrix *tm_; 
   
   static SparseMatrixIOTestsSmallNonsquare1* createSuite()
   {
      std::ifstream in;
      SparseMatrixIOTestsSmallNonsquare1 *suite(new SparseMatrixIOTestsSmallNonsquare1);
      suite->tm_ = new SparseMatrix;
      in.open("NonSquare3b7.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->ReadCOOMatrix(in);
      suite->tm_->TransformToCSCMatrix();
      in.close();
      return suite;
   }

   static void destroySuite(SparseMatrixIOTestsSmallNonsquare1* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   // Tests
   void test_all_conversion() {::_test_all_conversion(tm_);}
   void test_CSC_IO() {::_test_CSC_IO(tm_);}
   void test_CSC_IO_Binary() {::_test_CSC_IO_Binary(tm_);}
   void test_CSR_IO() {::_test_CSR_IO(tm_);}
   void test_CSR_IO_Binary() {::_test_CSR_IO_Binary(tm_);}
   void test_COO_IO() {::_test_COO_IO(tm_);}
   void test_COO_IO_Binary() {::_test_COO_IO_Binary(tm_);}
};

class SparseMatrixIOTestsSmallNonsquare2 : public CxxTest::TestSuite
{
 public:

   SparseMatrix *tm_; 
   
   static SparseMatrixIOTestsSmallNonsquare2* createSuite()
   {
      std::ifstream in;
      SparseMatrixIOTestsSmallNonsquare2 *suite(new SparseMatrixIOTestsSmallNonsquare2);
      suite->tm_ = new SparseMatrix;
      in.open("NonSquare7b3.txt",std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Matrix.");
      }
      suite->tm_->ReadCOOMatrix(in);
      suite->tm_->TransformToCSCMatrix();
      in.close();
      return suite;
   }

   static void destroySuite(SparseMatrixIOTestsSmallNonsquare2* suite)
   {
      delete suite->tm_;
      suite->tm_ = NULL;
      delete suite;
      suite = NULL;
   }

   // Tests
   void test_all_conversion() {::_test_all_conversion(tm_);}
   void test_CSC_IO() {::_test_CSC_IO(tm_);}
   void test_CSC_IO_Binary() {::_test_CSC_IO_Binary(tm_);}
   void test_CSR_IO() {::_test_CSR_IO(tm_);}
   void test_CSR_IO_Binary() {::_test_CSR_IO_Binary(tm_);}
   void test_COO_IO() {::_test_COO_IO(tm_);}
   void test_COO_IO_Binary() {::_test_COO_IO_Binary(tm_);}
};
