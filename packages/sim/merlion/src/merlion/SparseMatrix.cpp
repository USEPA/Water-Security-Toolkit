#include <merlion/SparseMatrix.hpp>
#include <merlion/BlasWrapper.hpp>

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <string>
#include <algorithm>
#include <numeric>
#include <vector>
#include <set>

SparseMatrix::SparseMatrix()
   :
   isCSC_(false),
   isCSR_(false),
   isCOO_(false),
   nrows_(0),
   ncols_(0),
   nnz_(0),
   irows_(NULL),
   jcols_(NULL),
   pcols_(NULL),
   prows_(NULL),
   vals_(NULL)
{
}

SparseMatrix::~SparseMatrix()
{
   clear();
}

void SparseMatrix::clear()
{
   isCSC_ = isCSR_ = isCOO_ = false;
   nrows_ = ncols_ = nnz_ = 0;
   delete [] prows_;
   prows_ = NULL;
   delete [] irows_;
   irows_ = NULL;
   delete [] pcols_;
   pcols_ = NULL;
   delete [] jcols_;
   jcols_ = NULL;
   delete [] vals_;
   vals_ = NULL;
}


void SparseMatrix::PrintStats(std::ostream& out) const {
   std::vector<int> row_nnz(NRows(),0);
   std::vector<int> col_nnz(NCols(),0);
   if (isCOOMatrix()) {
      for (int k = 0; k < NNonzeros(); ++k) {
         ++row_nnz[iRows()[k]];
         ++col_nnz[jCols()[k]];
      }
   }
   else if (isCSCMatrix()) {
      for (int j = 0; j < NCols(); ++j) {
         int p_start = pCols()[j];
         int p_stop = pCols()[j+1]-1;
         for (int p = p_start; p <= p_stop; ++p) {
            ++col_nnz[j];
            ++row_nnz[iRows()[p]];
         }
      }
   }
   else if (isCSRMatrix()) {
      for (int i = 0; i < NRows(); ++i) {
         int p_start = pRows()[i];
         int p_stop = pRows()[i+1]-1;
         for (int p = p_start; p <= p_stop; ++p) {
            ++col_nnz[jCols()[p]];
            ++row_nnz[i];
         }
      }
   }

   out << "************************" << std::endl;
   out << "Sparse Matrix Statistics:" << std::endl;
   out << "rows             = " << NRows() << std::endl;
   out << "cols             = " << NCols() << std::endl;
   out << "nnz              = " << NNonzeros() << std::endl;
   out << "density (%)      = " << NNonzeros() / (double)(NRows()) / (double)(NCols()) * 100.0 << std::endl;
   out << "Current Format   = ";
   if (isCOOMatrix()) {
      out << "COO" << std::endl;
   }
   else if (isCSRMatrix()) {
      out << "CSR" << std::endl;
   }
   else if (isCSCMatrix()) {
      out << "CSC" << std::endl;
   }
   double mem_nnz_float = sizeof(float)*NNonzeros();
   double mem_nnz_int = sizeof(int)*NNonzeros();
   double mem_nrows_int = sizeof(int)*(NRows()+1);
   double mem_ncols_int = sizeof(int)*(NCols()+1);
   out << "COO Storage (MB) = " << (mem_nnz_float + 2*mem_nnz_int) / (double)(1e6) << std::endl;
   out << "CSR Storage (MB) = " << (mem_nnz_float + mem_nnz_int + mem_nrows_int) / (double)(1e6) << std::endl;
   out << "CSC Storage (MB) = " << (mem_nnz_float + mem_nnz_int + mem_ncols_int) / (double)(1e6) << std::endl;


   out << std::endl;
   out << "min nnz/row      = " << *std::min_element(row_nnz.begin(),row_nnz.end()) << std::endl;
   out << "avg nnz/row      = " << NNonzeros() / (double)(NRows()) << std::endl;
   out << "max nnz/row      = " << *std::max_element(row_nnz.begin(),row_nnz.end()) << std::endl;
   out << std::endl;
   out << "min nnz/col      = " << *std::min_element(col_nnz.begin(),col_nnz.end()) << std::endl;
   out << "avg nnz/col      = " << NNonzeros() / (double)(NCols()) << std::endl;
   out << "max nnz/col      = " << *std::max_element(col_nnz.begin(),col_nnz.end()) << std::endl;
   out << std::endl;
   out << "min value        = " << *std::min_element(Values(),Values()+NNonzeros()) << std::endl;
   out << "avg nz value     = " << std::accumulate(Values(),Values()+NNonzeros(),0) / (double)(NNonzeros()) << std::endl;
   out << "max value        = " << *std::max_element(Values(),Values()+NNonzeros()) << std::endl;
   out << std::endl;
   int cnt_rows = 0;
   std::set<int> nnz_sizes;
   for (std::vector<int>::const_iterator pos = row_nnz.begin(), stop = row_nnz.end(); pos != stop; ++pos) {
      if (*pos) {
         nnz_sizes.insert(*pos);
         ++cnt_rows;
      }
   }
   out << "nonempty rows        = " << cnt_rows << std::endl;
   out << "min nnz/nonemtpy row = " << *(nnz_sizes.begin()) << std::endl;
   out << "avg nnz/nonemtpy row = " << NNonzeros() / (double)(cnt_rows) << std::endl;
   out << "max nnz/nonemtpy row = " << *(nnz_sizes.rbegin()) << std::endl;
   out << std::endl;
   int cnt_cols = 0;
   nnz_sizes.clear();
   for (std::vector<int>::const_iterator pos = col_nnz.begin(), stop = col_nnz.end(); pos != stop; ++pos) {
      if (*pos) {
         nnz_sizes.insert(*pos);
         ++cnt_cols;
      }
   }
   out << "nonempty cols        = " << cnt_cols << std::endl;
   out << "min nnz/nonemtpy col = " << *(nnz_sizes.begin()) << std::endl;
   out << "avg nnz/nonemtpy col = " << NNonzeros() / (double)(cnt_cols) << std::endl;
   out << "max nnz/nonemtpy col = " << *(nnz_sizes.rbegin()) << std::endl;
   out << std::endl;
   out << "reduced density (%)  = " << NNonzeros() / (double)(cnt_rows) / (double)(cnt_cols) * 100.0 << std::endl;
   out << "************************" << std::endl;
}

void SparseMatrix::TransformToCOOMatrix()
{
   if (isCSR_) {
      ConvertCSRToCOO();
   }
   else if (isCSC_) {
      ConvertCSCToCOO();
   }
   else if (!isCOO_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to transform sparse format. Current matrix type is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }	
}
void SparseMatrix::TransformToCSCMatrix()
{
   if (isCSR_) {
      ConvertCSRToCSC();
   }
   else if (isCOO_) {
      ConvertCOOToCSC();
   }
   else if (!isCSC_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to transform sparse format. Current matrix type is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }	
}
void SparseMatrix::TransformToCSRMatrix()
{
   if (isCSC_) {
      ConvertCSCToCSR();
   }
   else if (isCOO_) {
      ConvertCOOToCSR();
   }
   else if (!isCSR_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to transform sparse format. Current matrix type is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }	   
}

void SparseMatrix::AllocateNewCSCMatrix(int nrows, int ncols, int nnz)
{
   clear();
   if (nrows <= 0  || ncols <= 0 || nnz <= 0) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Invalid dimensions given for CSC memory allocation." << std::endl;
      std::cerr << "Rows     = " << nrows << std::endl;
      std::cerr << "Columns  = " << ncols << std::endl;
      std::cerr << "Nonzeros = " << nnz << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   nrows_ = nrows;
   ncols_ = ncols;
   nnz_ = nnz;
   pcols_ = new int[ncols+1];
   irows_ = new int[nnz];
   vals_ = new float[nnz];
   isCSC_ = true;
}

void SparseMatrix::AllocateNewCOOMatrix(int nrows, int ncols, int nnz)
{
   clear();
   if (nrows <= 0  || ncols <= 0 || nnz <= 0) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Invalid dimensions given for COO memory allocation." << std::endl;
      std::cerr << "Rows     = " << nrows << std::endl;
      std::cerr << "Columns  = " << ncols << std::endl;
      std::cerr << "Nonzeros = " << nnz << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   nrows_ = nrows;
   ncols_ = ncols;
   nnz_ = nnz;
   jcols_ = new int[nnz];
   irows_ = new int[nnz];
   vals_ = new float[nnz];
   isCOO_ = true;
}

void SparseMatrix::AllocateNewCSRMatrix(int nrows, int ncols, int nnz)
{
   clear();
   if (nrows <= 0  || ncols <= 0 || nnz <= 0) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Invalid dimensions given for COO memory allocation." << std::endl;
      std::cerr << "Rows     = " << nrows << std::endl;
      std::cerr << "Columns  = " << ncols << std::endl;
      std::cerr << "Nonzeros = " << nnz << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   nrows_ = nrows;
   ncols_ = ncols;
   nnz_ = nnz;
   jcols_ = new int[nnz];
   prows_ = new int[nrows+1];
   vals_ = new float[nnz];
   isCSR_ = true;
}

void SparseMatrix::ConvertCOOToCSC()
{
   /* This routine will convert a sparse COO matrix (Ai[nnz], Aj[nnz], Ax[nnz])
      to a sparse CSC matrix (Ci[nnz], Cp[ncols+1], Cx[nnz]). A CSC matrix with a single 
      column pointer array (Cp) requires that the row indices array (Ci) list row
      indices in ascending order for each column. For this reason the COO matrix is
      converted to an intermediate CSR matrix (Rp[nrows+1], Rj[nnz], Rx[nnz] which is
      then converted to a CSC matrix.
   */
   if (!irows_ || !jcols_ || !vals_ || !isCOO_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to perform conversion. COO matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   assert(!pcols_ && !isCSC_);

   // create a workspace array
   int *W(new int[std::max(nrows_,ncols_)]);
   BlasInitZero(std::max(nrows_,ncols_), W);

   // count elements in each row and do some standard checking of the array values
   for (int k = 0; k < nnz_; ++k) {
      int i = irows_[k];
      int j = jcols_[k];
      if (i < 0 || i >= nrows_ || j < 0 || j >= ncols_) {
         delete [] W;
         W = NULL;
         std::cerr << std::endl;
         std::cerr << "SparseMatrix ERROR: Unable to perform conversion. Invalid row/column indices." << std::endl;
         std::cerr << std::endl;
         exit(1);
      }
      W[i]++;
   }

   // create the row pointer
   // and convert W[i] to represent
   // the start of each row
   int *Rp(new int[nrows_+1]);
   Rp[0] = 0;
   for (int i = 0; i < nrows_; ++i) {
      Rp[i+1] = Rp[i] + W[i];
      W[i] = Rp[i];
   }

   // Create the CSR matrix
   // increment W[i] each time
   // an element is added to a row.
   int *Rj(new int[nnz_]);
   float *Rx(new float[nnz_]);
   for (int k = 0; k < nnz_; ++k) {
      int p = W[irows_[k]]++;
      Rj[p] = jcols_[k];
      Rx[p] = vals_[k];
   }

   // Count the elements in each column
   // using the CSR matrix.
   BlasResetZero(ncols_,W);
   for (int i = 0; i < nrows_; ++i) {
      int p_start = Rp[i];
      int p_stop = Rp[i+1];
      for (int p = p_start; p < p_stop; ++p) {
         W[Rj[p]]++;
      }
   }

   // delete the current COO column index array;
   delete [] jcols_;
   jcols_ = NULL;

   // Create the column pointer array
   // and convert W[j] to represent
   // the start of each column.
   pcols_ = new int[ncols_+1];
   pcols_[0] = 0;
   for (int j = 0; j < ncols_; ++j) {
      pcols_[j+1] = pcols_[j] + W[j];
      W[j] = pcols_[j];
   }

   // Create the CSC matrix
   // Because we create the CSC matrix
   // row by row, the row indices will
   // be in ascending order for each column.
   for (int i = 0; i < nrows_; ++i) {
      int p_start = Rp[i];
      int p_stop = Rp[i+1];
      for (int p = p_start; p < p_stop; ++p) {
         int cp = W[Rj[p]]++;
         irows_[cp] = i;
         vals_[cp] = Rx[p];
      }
   }

   // delete the temporary CSR matrix
   delete [] Rx;
   Rx = NULL;
   delete [] Rp;
   Rp = NULL;
   delete [] Rj;
   Rj = NULL;

   // delete the temporary workspace array
   delete [] W;
   W = NULL;

   // set the new matrix type
   isCOO_ = false;
   isCSC_ = true;
}

void SparseMatrix::ConvertCSCToCOO()
{
   /* This routine will convert a sparse CSC matrix (Ci[nnz], Cp[ncols+1], Cx[nnz])
      to a sparse COO matrix (Ai[nnz], Aj[nnz], Ax[nnz]). A simple conversion
      of the column pointer array to a column indices array in done.
   */
   if (!irows_ || !pcols_ || !vals_ || !isCSC_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to perform conversion. CSC matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   assert(!jcols_ && !isCOO_);

   jcols_ = new int[nnz_];
   // fill in the column indices array and check that matrix array values make sense        
   for (int j = 0; j < ncols_; ++j) {
      int p_start = pcols_[j];
      int p_stop = pcols_[j+1];
      for (int p = p_start; p < p_stop; ++p) {
         int i = irows_[p];
         if (i < 0 || i >= nrows_) {
            std::cerr << std::endl;
            std::cerr << "SparseMatrix ERROR: Unable to perform conversion. Invalid row/column indices." << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         jcols_[p] = j;
      }
   }

   // delete the current CSC column pointer array
   delete [] pcols_;
   pcols_ = NULL;

   isCSC_ = false;
   isCOO_ = true;
}

void SparseMatrix::ConvertCOOToCSR()
{
   /* This routine will convert a sparse COO matrix (Ai[nnz], Aj[nnz], Ax[nnz])
      to a sparse CSR matrix (Rp[nrows+1], Rj[nnz], Rx[nnz]). A CSR matrix with a single 
      row pointer array (Rp) requires that the column indices array (Rj) list column
      indices in ascending order for each row. For this reason the COO matrix is
      converted to an intermediate CSC matrix (Ci[nnz], Cp[ncols+1], Cx[nnz] which is
      then converted to a CSR matrix.
   */
   if (!irows_ || !jcols_ || !vals_ || !isCOO_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to perform conversion. COO matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   assert(!prows_ && !isCSR_);

   // create a workspace array
   int *W(new int[std::max(nrows_,ncols_)]);
   BlasInitZero(std::max(nrows_,ncols_),W);

   // count elements in each column and do some standard checking of the array values
   for (int k = 0; k < nnz_; ++k) {
      int i = irows_[k];
      int j = jcols_[k];
      if (i < 0 || i >= nrows_ || j < 0 || j >= ncols_) {
         delete [] W;
         W = NULL;
         std::cerr << std::endl;
         std::cerr << "SparseMatrix ERROR: Unable to perform conversion. Invalid row/column indices." << std::endl;
         std::cerr << std::endl;
         exit(1);
      }
      W[j]++;
   }

   // create the column pointer
   // and convert W[j] to represent
   // the start of each column
   int *Cp(new int[ncols_+1]);
   Cp[0] = 0;
   for (int j = 0; j < ncols_; ++j) {
      Cp[j+1] = Cp[j] + W[j];
      W[j] = Cp[j];
   }

   // Create the CSC matrix
   // increment W[j] each time
   // an element is added to a column.
   int *Ci(new int[nnz_]);
   float *Cx(new float[nnz_]);
   for (int k = 0; k < nnz_; ++k) {
      int p = W[jcols_[k]]++;
      Ci[p] = irows_[k];
      Cx[p] = vals_[k];
   }

   // Count the elements in each row
   // using the CSC matrix.
   BlasResetZero(nrows_,W);
   for (int j = 0; j < ncols_; ++j) {
      int p_start = Cp[j];
      int p_stop = Cp[j+1];
      for (int p = p_start; p < p_stop; ++p) {
         W[Ci[p]]++;
      }
   }

   // delete the current COO row index array;
   delete [] irows_;
   irows_ = NULL;

   // Create the row pointer array
   // and convert W[i] to represent
   // the start of each row.
   prows_ = new int[nrows_+1];
   prows_[0] = 0;
   for (int i = 0; i < nrows_; ++i) {
      prows_[i+1] = prows_[i] + W[i];
      W[i] = prows_[i];
   }

   // Create the CSR matrix
   // Because we create the CSR matrix
   // column by column, the column indices will
   // be in ascending order for each row.
   for (int j = 0; j < ncols_; ++j) {
      int p_start = Cp[j];
      int p_stop = Cp[j+1];
      for (int p = p_start; p < p_stop; ++p) {
         int rp = W[Ci[p]]++;
         jcols_[rp] = j;
         vals_[rp] = Cx[p];
      }
   }

   // delete the temporary CSC matrix
   delete [] Cx;
   Cx = NULL;
   delete [] Ci;
   Ci = NULL;
   delete [] Cp;
   Cp = NULL;

   // delete the temporary workspace array
   delete [] W;
   W = NULL;

   isCOO_ = false;
   isCSR_ = true;
}

void SparseMatrix::ConvertCSRToCOO()
{
   /* This routine will convert a sparse CSR matrix (Rp[nrows+1], Rj[nnz], Rx[nnz])
      to a sparse COO matrix (Ai[nnz], Aj[nnz], Ax[nnz]). A simple conversion
      of the row pointer array to a row indices array in done.
   */
   if (!prows_ || !jcols_ || !vals_ || !isCSR_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to perform conversion. CSR matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   assert(!irows_ && !isCOO_);

   irows_ = new int[nnz_];
   // fill in the row indices array and check check that matrix array values make sense
   for (int i = 0; i < nrows_; ++i) {
      int p_start = prows_[i];
      int p_stop = prows_[i+1];
      for (int p = p_start; p < p_stop; ++p) {
         int j = jcols_[p];
         if (j < 0 || j >= ncols_) {
            std::cerr << std::endl;
            std::cerr << "SparseMatrix ERROR: Unable to perform conversion. Invalid row/column indices." << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         irows_[p] = i;
      }
   }

   // delete the current CSR row pointer array
   delete [] prows_;
   prows_ = NULL;

   isCSR_ = false;
   isCOO_ = true;
}

void SparseMatrix::ConvertCSCToCSR()
{
   /* This routine will convert a sparse CSC matrix (Ci[nnz], Cp[ncols+1], Cx[nnz])
      to a sparse CSR matrix (Rp[nrows+1], Rj[nnz], Rx[nnz]). A CSR matrix with a single 
      row pointer array (Rp) requires that the column indices array (Rj) list column
      indices in ascending order for each row.
   */
   if (!irows_ || !pcols_ || !vals_ || !isCSC_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to perform conversion. CSC matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   assert(!prows_ && !jcols_ && !isCSR_);

   // create a workspace array
   int *W(new int[nrows_]);
   BlasInitZero(nrows_,W);

   //Count the elements in each row and check that matrix array values make sense
   for (int j = 0; j < ncols_; ++j) {
      int p_start = pcols_[j];
      int p_stop = pcols_[j+1];
      for (int p = p_start; p < p_stop; ++p) {
         int i = irows_[p];
         if (i < 0 || i >= nrows_) {
            std::cerr << std::endl;
            std::cerr << "SparseMatrix ERROR: Unable to perform conversion. Invalid row/column indices." << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         W[i]++;
      }
   }

   // Create the row pointer array
   // and convert W[i] to represent
   // the start of each row.
   prows_ = new int[nrows_+1];
   prows_[0] = 0;
   for (int i = 0; i < nrows_; ++i) {
      prows_[i+1] = prows_[i] + W[i];
      W[i] = prows_[i];
   }

   float *vals_csc(new float[nnz_]);
   cblas_scopy(nnz_,vals_,1,vals_csc,1);

   jcols_ = new int[nnz_];

   // Create the CSR matrix
   // Because we create the CSR matrix
   // column by column, the column indices will
   // be in ascending order for each row.
   for (int j = 0; j < ncols_; ++j) {
      int p_start = pcols_[j];
      int p_stop = pcols_[j+1];
      for (int p = p_start; p < p_stop; ++p) {
         int rp = W[irows_[p]]++;
         jcols_[rp] = j;
         vals_[rp] = vals_csc[p];
      }
   }

   // delete the row indices array
   delete [] irows_;
   irows_ = NULL;
   // delete the column pointer array
   delete [] pcols_;
   pcols_ = NULL;

   // delete the temporary workspace arrays
   delete [] W;
   W = NULL;
   delete [] vals_csc;
   vals_csc = NULL;

   isCSC_ = false;
   isCSR_ = true;  
}

void SparseMatrix::ConvertCSRToCSC()
{
   /* This routine will convert a sparse CSR matrix (Rp[nrows+1], Rj[nnz], Rx[nnz])
      to a sparse CSC matrix (Ci[nnz], Cp[ncols+1], Cx[nnz]). A CSC matrix with a single 
      column pointer array (Cp) requires that the row indices array (Ci) list row
      indices in ascending order for each column.
   */
   if (!prows_ || !jcols_ || !vals_ || !isCSR_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable to perform conversion. CSC matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   assert(!irows_ && !pcols_ && !isCSC_);

   // create a workspace array
   int *W(new int[ncols_]);
   BlasInitZero(ncols_,W);    

   // check that matrix array values make sense
   for (int i = 0; i < nrows_; ++i) {
      int p_start = prows_[i];
      int p_stop = prows_[i+1];
      for (int p = p_start; p < p_stop; ++p) {
         int j = jcols_[p];
         if (j < 0 || j >= ncols_) {
            std::cerr << std::endl;
            std::cerr << "SparseMatrix ERROR: Unable to perform conversion. Invalid row/column indices." << std::endl;
            std::cerr << std::endl;
            exit(1);
         }
         W[j]++;
      }
   }

   // Create the column pointer array
   // and convert W[j] to represent
   // the start of each column.
   pcols_ = new int[ncols_+1];
   pcols_[0] = 0;
   for (int j = 0; j < ncols_; ++j) {
      pcols_[j+1] = pcols_[j] + W[j];
      W[j] = pcols_[j];
   }

   float *vals_csr(new float[nnz_]);
   cblas_scopy(nnz_,vals_,1,vals_csr,1);

   irows_ = new int[nnz_];

   // Create the CSC matrix
   // Because we create the CSC matrix
   // row by row, the row indices will
   // be in ascending order for each column.
   for (int i = 0; i < nrows_; ++i) {
      int p_start = prows_[i];
      int p_stop = prows_[i+1];
      for (int p = p_start; p < p_stop; ++p) {
         int cp = W[jcols_[p]]++;
         irows_[cp] = i;
         vals_[cp] = vals_csr[p];
      }
   }

   // delete the column indices array
   delete [] jcols_;
   jcols_ = NULL;
   // delete the row pointer array
   delete [] prows_;
   prows_ = NULL;

   // delete the temporary workspace arrays
   delete [] W;
   W = NULL;
   delete [] vals_csr;
   vals_csr = NULL;


   isCSR_ = false;
   isCSC_ = true;
}

void SparseMatrix::ReadCOOMatrix(std::istream& in, bool binary/*=false*/)
{
   clear();
   isCOO_ = true;
   if (binary) {
      std::string name;
      name.resize(9);
      in.read((char*)name.c_str(),sizeof(char)*9);
      if (name != "COOMatrix") {
	 std::cerr << std::endl;
	 std::cerr << "SparseMatrix ERROR: First tag when reading COO Matrix must read: COOMatrix" << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      std::string::size_type s;
      in.read((char*)(&s),sizeof(s));
      name.resize(s);
      in.read((char*)name.c_str(),sizeof(char)*s);
      int nrows, ncols, nnz;
      in.read((char*)(&nrows),sizeof(nrows));
      in.read((char*)(&ncols),sizeof(ncols));
      in.read((char*)(&nnz),sizeof(nnz));
      AllocateNewCOOMatrix(nrows, ncols, nnz);
      in.read((char*)(irows_),sizeof(irows_[0])*nnz_);
      in.read((char*)(jcols_),sizeof(jcols_[0])*nnz_);
      in.read((char*)(vals_),sizeof(vals_[0])*nnz_);
   }
   else {
      std::string name;
      in >> name;
      if (name != "COOMatrix") {
	 std::cerr << std::endl;
	 std::cerr << "SparseMatrix ERROR: First line when reading COO Matrix must read: COOMatrix <name>" << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      in >> name; // not used
      
      int nrows, ncols, nnz;
      in >> nrows;
      in >> ncols;
      in >> nnz;
      AllocateNewCOOMatrix(nrows, ncols, nnz);
      
      for (int i=0; i<nnz_; i++) {
	 in >> irows_[i];
	 in >> jcols_[i];
	 in >> vals_[i];
	 if (irows_[i] < 0 || irows_[i] >= nrows_ || jcols_[i] < 0 || jcols_[i] >= ncols_) {
	    std::cerr << std::endl;
	    std::cerr << "SparseMatrix ERROR: Invalid dimensions given in matrix file." << std::endl;
	    std::cerr << std::endl;
	    clear();
	    exit(1);
	 }
      }
   }
}
void SparseMatrix::ReadCSCMatrix(std::istream& in, bool binary/*=false*/)
{
   clear();
   isCSC_ = true;
   if (binary) {
      std::string name;
      name.resize(9);
      in.read((char*)name.c_str(),sizeof(char)*9);
      if (name != "CSCMatrix") {
	 std::cerr << std::endl;
	 std::cerr << "SparseMatrix ERROR: First tag when reading CSC Matrix must read: CSCMatrix" << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      std::string::size_type s;
      in.read((char*)(&s),sizeof(s));
      name.resize(s);
      in.read((char*)name.c_str(),sizeof(char)*s);
      int nrows, ncols, nnz;
      in.read((char*)(&nrows),sizeof(nrows));
      in.read((char*)(&ncols),sizeof(ncols));
      in.read((char*)(&nnz),sizeof(nnz));
      AllocateNewCSCMatrix(nrows, ncols, nnz);
      in.read((char*)(irows_),sizeof(irows_[0])*nnz_);
      in.read((char*)(pcols_),sizeof(pcols_[0])*(ncols_+1));
      in.read((char*)(vals_),sizeof(vals_[0])*nnz_);
   }
   else {
      std::string name;
      in >> name;
      if (name != "CSCMatrix") {
	 std::cerr << std::endl;
	 std::cerr << "SparseMatrix ERROR: First line when reading CSC Matrix must read: CSCMatrix <name>" << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      in >> name; // not used
      
      int nrows, ncols, nnz;
      in >> nrows;
      in >> ncols;
      in >> nnz;
      AllocateNewCSCMatrix(nrows, ncols, nnz);

      // it is assumed the columns are listed in ascending order
      pcols_[0] = 0;
      for (int j = 0; j < ncols_; ++j) {
	 int col_nnz;
	 in >> col_nnz;
	 pcols_[j+1] = pcols_[j] + col_nnz;
	 int p_start = pcols_[j];
	 int p_stop = pcols_[j+1];
	 for (int p = p_start; p < p_stop; ++p) {
	    in >> irows_[p];
	    in >> vals_[p];
	    if (irows_[p] < 0 || irows_[p] >= nrows_) {
	       std::cerr << std::endl;
	       std::cerr << "SparseMatrix ERROR: Invalid dimensions given in matrix file." << std::endl;
	       std::cerr << std::endl;
	       clear();
	       exit(1);
	    }
	 }
      }
   }
}
void SparseMatrix::ReadCSRMatrix(std::istream& in, bool binary/*=false*/)
{
   clear();
   isCSR_ = true;
   if (binary) {
      std::string name;
      name.resize(9);
      in.read((char*)name.c_str(),sizeof(char)*9);
      if (name != "CSRMatrix") {
	 std::cerr << std::endl;
	 std::cerr << "SparseMatrix ERROR: First tag when reading CSR Matrix must read: CSRMatrix" << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      std::string::size_type s;
      in.read((char*)(&s),sizeof(s));
      name.resize(s);
      in.read((char*)name.c_str(),sizeof(char)*s);
      int nrows, ncols, nnz;
      in.read((char*)(&nrows),sizeof(nrows));
      in.read((char*)(&ncols),sizeof(ncols));
      in.read((char*)(&nnz),sizeof(nnz));
      AllocateNewCSRMatrix(nrows, ncols, nnz);
      in.read((char*)(prows_),sizeof(prows_[0])*(nrows_+1));
      in.read((char*)(jcols_),sizeof(jcols_[0])*nnz_);
      in.read((char*)(vals_),sizeof(vals_[0])*nnz_);
   }
   else {
      std::string name;
      in >> name;
      if (name != "CSRMatrix") {
	 std::cerr << std::endl;
	 std::cerr << "SparseMatrix ERROR: First line when reading CSR Matrix must read: CSRMatrix <name>" << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      in >> name; // not used
      
      int nrows, ncols, nnz;
      in >> nrows;
      in >> ncols;
      in >> nnz;
      AllocateNewCSRMatrix(nrows, ncols, nnz);
      
      // it is assumed the rows are listed in ascending order
      prows_[0] = 0;
      for (int i = 0; i < nrows_; ++i) {
	 int row_nnz;
	 in >> row_nnz;
	 prows_[i+1] = prows_[i]+row_nnz;
	 int p_start = prows_[i];
	 int p_stop = prows_[i+1];
	 for (int p = p_start; p < p_stop; ++p) {
	    in >> jcols_[p];
	    in >> vals_[p];
	    if (jcols_[p] < 0 || jcols_[p] >= ncols_) {
	       std::cerr << std::endl;
	       std::cerr << "SparseMatrix ERROR: Invalid dimensions given in matrix file." << std::endl;
	       std::cerr << std::endl;
	       clear();
	       exit(1);
	    }
	 }
      }
   }
}

void SparseMatrix::PrintCOOMatrix(std::ostream& out, std::string name/*="A"*/, bool binary/*=false*/) const
{
   if (!irows_ || !jcols_ || !vals_ || !isCOO_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable print COO matrix. COO matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   if (binary) {
      out.write("COOMatrix",sizeof(char)*9);
      std::string::size_type s = name.size();
      out.write((char*)(&s),sizeof(s));
      out.write((char*)(name.c_str()),sizeof(char)*name.size());
      out.write((char*)(&nrows_),sizeof(nrows_));
      out.write((char*)(&ncols_),sizeof(ncols_));
      out.write((char*)(&nnz_),sizeof(nnz_));
      out.write((char*)(irows_),sizeof(irows_[0])*nnz_);
      out.write((char*)(jcols_),sizeof(jcols_[0])*nnz_);
      out.write((char*)(vals_),sizeof(vals_[0])*nnz_);
   }
   else {
      out << "COOMatrix " << name << "\n";
      out << nrows_ << "\n";
      out << ncols_ << "\n";
      out << nnz_ << "\n";
      
      for (int i=0; i<nnz_; i++) {
	 out << irows_[i] << " " << jcols_[i] << " " << vals_[i] << "\n";
      }
   }
}

void SparseMatrix::PrintCSCMatrix(std::ostream& out, std::string name/*="A"*/, bool binary/*=false*/) const
{
   if (!irows_ || !pcols_ || !vals_ || !isCSC_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable print CSC matrix. CSC matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   if (binary) {
      out.write("CSCMatrix",sizeof(char)*9);
      std::string::size_type s = name.size();
      out.write((char*)(&s),sizeof(s));
      out.write((char*)(name.c_str()),sizeof(char)*name.size());
      out.write((char*)(&nrows_),sizeof(nrows_));
      out.write((char*)(&ncols_),sizeof(ncols_));
      out.write((char*)(&nnz_),sizeof(nnz_));
      out.write((char*)(irows_),sizeof(irows_[0])*nnz_);
      out.write((char*)(pcols_),sizeof(pcols_[0])*(ncols_+1));
      out.write((char*)(vals_),sizeof(vals_[0])*nnz_);
   }
   else {
      out << "CSCMatrix " << name << "\n";
      out << nrows_ << "\n";
      out << ncols_ << "\n";
      out << nnz_ << "\n";

      for (int j = 0; j < ncols_; ++j) {
	 int p_start = pcols_[j];
	 int p_stop = pcols_[j+1];
	 int col_nnz = p_stop-p_start;
	 out << col_nnz << "\n";
	 for (int p = p_start; p < p_stop; ++p) {
	    out << irows_[p] << " " << vals_[p] << "\n";
	 }
      }
   }
}

void SparseMatrix::PrintCSRMatrix(std::ostream& out, std::string name/*="A"*/, bool binary/*=false*/) const
{
   if (!prows_ || !jcols_ || !vals_ || !isCSR_) {
      std::cerr << std::endl;
      std::cerr << "SparseMatrix ERROR: Unable print CSR matrix. CSR matrix is undefined." << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   if (binary) {
      out.write("CSRMatrix",sizeof(char)*9);
      std::string::size_type s = name.size();
      out.write((char*)(&s),sizeof(s));
      out.write((char*)(name.c_str()),sizeof(char)*name.size());
      out.write((char*)(&nrows_),sizeof(nrows_));
      out.write((char*)(&ncols_),sizeof(ncols_));
      out.write((char*)(&nnz_),sizeof(nnz_));
      out.write((char*)(prows_),sizeof(prows_[0])*(nrows_+1));
      out.write((char*)(jcols_),sizeof(jcols_[0])*nnz_);
      out.write((char*)(vals_),sizeof(vals_[0])*nnz_);
   }
   else {
      out << "CSRMatrix " << name << "\n";
      out << nrows_ << "\n";
      out << ncols_ << "\n";
      out << nnz_ << "\n";

      for (int i = 0; i < nrows_; ++i) {
	 int p_start = prows_[i];
	 int p_stop = prows_[i+1];
	 int row_nnz = p_stop-p_start;
	 out << row_nnz << "\n";
	 for (int p = p_start; p < p_stop; ++p) {
	    out << jcols_[p] << " " << vals_[p] << "\n";
	 }
      }
   }
}
