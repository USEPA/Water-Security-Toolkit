#ifndef MERLION_SPARSEMATRIX_HPP__
#define MERLION_SPARSEMATRIX_HPP__

#include <iostream>

/** \brief A sparse matrix class
  \date 2012
  \author Gabe Hackebeil
  This is a simple sparse matrix class used to 
  represent a sparse matrix in triplet format
  (COO), Compressed Column format (CSC), or Compressed Row format (CSR). 
  The class contains conversion routines to and from each.
*/
class SparseMatrix
{
public:
   SparseMatrix();
   virtual ~SparseMatrix();
   void clear();

   void AllocateNewCSCMatrix(int nrows, int ncols, int nnz);
   void AllocateNewCOOMatrix(int nrows, int ncols, int nnz);
   void AllocateNewCSRMatrix(int nrows, int ncols, int nnz);

   void ReadCOOMatrix(std::istream& in, bool binary=false);
   void ReadCSCMatrix(std::istream& in, bool binary=false);
   void ReadCSRMatrix(std::istream& in, bool binary=false);

   void PrintCOOMatrix(std::ostream& out, std::string name="A", bool binary=false) const;
   void PrintCSCMatrix(std::ostream& out, std::string name="A", bool binary=false) const;
   void PrintCSRMatrix(std::ostream& out, std::string name="A", bool binary=false) const;

   void TransformToCOOMatrix();
   void TransformToCSCMatrix();
   void TransformToCSRMatrix();

   inline bool isCSRMatrix() const {return isCSR_;}
   inline bool isCSCMatrix() const {return isCSC_;}
   inline bool isCOOMatrix() const {return isCOO_;}

   inline int NRows() const {return nrows_;}
   inline int NCols() const {return ncols_;}
   inline int NNonzeros() const {return nnz_;}

   inline const int* iRows() const {return irows_;}
   inline int* iRows()             {return irows_;}

   inline const int* jCols() const {return jcols_;}
   inline int* jCols()             {return jcols_;}

   inline const int* pCols() const {return pcols_;}
   inline int* pCols()             {return pcols_;}

   inline const int* pRows() const {return prows_;}
   inline int* pRows()             {return prows_;}

   inline const float* Values() const {return vals_;}
   inline float* Values()             {return vals_;}

   void PrintStats(std::ostream& out) const;

protected:
   void ConvertCOOToCSC();
   void ConvertCSCToCOO();
   void ConvertCOOToCSR();
   void ConvertCSRToCOO();
   void ConvertCSCToCSR();
   void ConvertCSRToCSC();

   bool isCSC_;
   bool isCSR_;
   bool isCOO_;

   // dimensions of the matrix
   int nrows_;
   int ncols_; 

   // number of nonzeros
   int nnz_;

   // rows arrays need for any given format
   int *irows_; // COO , CSC
   int *jcols_; // COO , CSR
   int *pcols_; // CSC
   int *prows_; // CSR

   // nonzero values
   float *vals_; // COO , CSC , CSR

private:

/**@name Default Compiler Generated Methods
    * (Hidden to avoid implicit creation/calling).
    * These methods are not implemented and
    * we do not want the compiler to implement
    * them for us, so we declare them private
    * and do not define them. This ensures that
    * they will not be implicitly created/called. */
   //@{
   /// Copy Constructor
   SparseMatrix(const SparseMatrix&);
   /// Overloaded Equals Operator
   void operator=(const SparseMatrix&);
   //@}
};

#endif
