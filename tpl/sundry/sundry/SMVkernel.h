/*  _________________________________________________________________________
 *
 *  Acro: A Common Repository for Optimizers
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top Acro directory.
 *  _________________________________________________________________________
 */

//
//  SMVkernel.h
//        Description: Thread-safe sparse matrix data structures
//                     and algorithms
//        Author:  Robert Heaphy
//        Date:    2005
//
#ifndef __SMVKERNEL_H__
#define __SMVKERNEL_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef HAVE_MALLOC_H
 #include <malloc.h>
#endif

//#include <mtgl/graph.h>

#ifdef __MTA__
 #define BRAKET
 #include <sys/mta_task.h>
 #include <machine/runtime.h>
#else
 #define BRAKET <T>
#endif

// Current ISSUE: error handling is erratic
// Current ISSUE: look for NOT IMPLEMENTED and implement


// Base classes-forward declarations
template <class T> class VectorBase;
template <class T> class MatrixBase;

// Derived classes-forward declarations
template <class T> class DenseVector;

template <class T> class SparseMatrixCSR;
template <class T> class SparseMatrixCSC;
template <class T> class SparseMatrixCOO;

template <class T> DenseVector<T>
operator* (const SparseMatrixCSR<T>& a, const DenseVector<T>& b);
template <class T> DenseVector<T> diagonal (const SparseMatrixCSR<T>& a);
template <class T> DenseVector<T> Transpose_SMVm (const SparseMatrixCSR<T>&, 
						  const DenseVector<T>&);
template <class T>
DenseVector<T> 
operator* (const SparseMatrixCSC<T>&, const DenseVector<T>&);
     
template <class T>
DenseVector<T> 
operator* (const SparseMatrixCOO<T>&, const DenseVector<T>&);
     
template <class T>
DenseVector<T> 
operator* (int const, const DenseVector<T>&);
    
template <class T>
DenseVector<T> operator* (double const, const DenseVector<T>&);

template <class T>
SparseMatrixCSR<T> operator*(int const, const SparseMatrixCSR<T>&);    

template <class T>
SparseMatrixCSR<T> 
     operator* (double const, const SparseMatrixCSR<T>&);
    
template <class T>
T 
operator* (const DenseVector<T>&, const DenseVector<T>&);
     
template <class T>
DenseVector<T> 
diagonal (const SparseMatrixCSR<T>&);
    
template <class T>
DenseVector<T> 
Transpose_SMVm (const SparseMatrixCSR<T>&, const DenseVector<T>&);

/***********************  Base Classes  ****************************/

template <class T> class VectorBase
{
  protected:
    int length;                
    T* values;                                         // vector elements
    
  public:  
    VectorBase() : length(0), values(0)  {
    }
      
    VectorBase (int const n) : length(n)  {
      this->values = (T*) malloc (n * sizeof(T));            // values(new T[n])
      
    T*  const this_values = this->values;         // force MTA to place on stack
    T   const zero        = T();                  // force MTA to place on stack
    int const finish      = this->length;         // force MTA to place on stack
    #pragma mta assert parallel
    for (int i = 0; i < finish; i++)
      this_values[i] = zero;       
    }
    
    VectorBase (const VectorBase<T>& a) : length(a.length)  {
      this->values = (T*) malloc (a.length * sizeof(T)); //values(new T[a.length])
      
      int const stop        = a.length;            // force MTA to place on stack
      T*  const this_values = this->values;        // force MTA to place on stack
      T*  const a_values    = a.values;            // force MTA to place on stack
      #pragma mta assert parallel  
      for (int i = 0; i < stop; i++)
        this_values[i] = a_values[i];
    }
      
    ~VectorBase()  {
      if (this->values) free (this->values);                  // delete[] values
      this->values = 0;
    }

    void clear() 
    {
	if(this->values) free(this->values); this->values = NULL;
    }
          
    void VectorPrint (char const* name) const;
};



template <class T> class MatrixBase
{
  protected:
    int nRow, nCol, nNonZero;     // number of rows, columns, and non zeros
    T*   values;                  // non zero matrix elements
    int* index;                   // generally used for sparse storage index
    bool data_owned;
    
  public:
    MatrixBase() : nRow(0), nCol(0), nNonZero(0), values(0), index(0),
		   data_owned(true)  { }
      
    MatrixBase(int const row, int const col, int const count, bool own=true)
     : nRow(row), nCol(col), nNonZero(count), index(0), data_owned(own)  
    {
	//printf("MatrixBase()\n");
	values = (T*) malloc (count * sizeof(T));  // values(new T[count]) 
        //printf("MatrixBase() alloc'd\n");

	T*  const this_values = this->values;      // force MTA to place on stack
	T   const zero        = T();
	int const finish      = this->nNonZero;
	
	#pragma mta assert parallel
	for (int i = 0; i < finish; i++) 
	{
	    this_values[i] = i;
	}
    }
      
    ~MatrixBase() { 
	if(data_owned && this->values) free(this->values); 
	this->values = 0;
	if(data_owned && this->index)  free(this->index);  
	this->index  = 0;
    }


    MatrixBase(const MatrixBase<T>& a) 
	: nRow(a.nRow), 
	  nCol(a.nCol),
	  nNonZero(a.nNonZero), 
	  /* values(new T[a.nNonZero]),*/  
	  index(0), 
	  data_owned(true)  // not debugged!
    {
        this->values = (T*) malloc (a.nNonZero * sizeof (T));
        T*  const this_values = this->values;  // force MTA to place on stack
        T*  const a_values    = a.values;      // force MTA to place on stack
        int const stop        = a.nNonZero;
	
        #pragma mta assert parallel  
        for (int i = 0; i < stop; i++)
	{
	    this_values[i] = a_values[i];
	}
    }
                 
    void MatrixPrint (char const* name);
};



/***************************  DenseVector *************************************/

template <class T = double> class DenseVector : public VectorBase<T>
{
  public:    
    DenseVector()  {}
      
    DenseVector (int const n) : VectorBase<T> (n)  {}

    DenseVector (const DenseVector<T>& a) : VectorBase<T> (a)  {}
      
    ~DenseVector()  {}

    DenseVector<T>& operator= (const DenseVector<T>& a)  
    {
	if (this->length != a.length)
	    printf("DenseVector copy assignment error\n");
        
	if (this != &a) 
	{
	    int const stop        = this->length;  // force MTA to place on stack
	    T*  const this_values = this->values;  // force MTA to place on stack
	    T*  const a_values    = a.values;      // force MTA to place on stack
        
	    #pragma mta assert parallel  
	    for (int i = 0; i < stop; i++)
	    {
		this_values[i] = a_values[i];
	    }
	}
	return *this;
    }


    const T operator[](int idx)
    {
	if(idx<0 || idx>this->length)
	{
	    fprintf(stderr, "error: index %d out of bounds in DenseVector\n",idx);
	    exit(1);
	}
	return( this->values[idx] );
    }
    
     
    DenseVector<T>& operator+= (const DenseVector<T>& a)  
    {
	int const stop        = this->length;    // force MTA to place on stack
	T*  const this_values = this->values;    // force MTA to place on stack
	T*  const a_values    = a.values;        // force MTA to place on stack
	#pragma mta assert parallel  
	for (int i = 0; i < stop; i++)
	{
	    // safe_incr(this_values[i], a_values[i]);
	    this_values[i] += a_values[i];
	}
	return *this;
    }

    DenseVector<T>& operator-= (const DenseVector<T>& a)  
    {
	int const stop        = this->length;    // force MTA to place on stack
	T*  const this_values = this->values;    // force MTA to place on stack
	T*  const a_values    = a.values;        // force MTA to place on stack
	
	#pragma mta assert parallel  
	for (int i = 0; i < stop; i++)
	{
	    // safe_incr(this->values[i], -a.values[i]);
	    this->values[i] += -a.values[i];
	}
	return *this;
    }
                    
    DenseVector<T>& operator() ()  
    {
	int const stop        = this->length;    // force MTA to place on stack
	T*  const this_values = this->values;    // force MTA to place on stack
	T   const zero        = T();
	
	#pragma mta assert parallel  
	for (int i = 0; i < stop; i++)
	{
	    this_values[i] = zero;
	}
	return *this;
    }

    T norm2 () const  
    {
	T   temp              = T();
	int const stop        = this->length;
	T*  const this_values = this->values;    // force MTA to place on stack
	
	#pragma mta assert parallel
	for (int i = 0 ; i < stop; i++)
	{
	    // safe_incr(temp, this_values[i] * this_values[i]);
	    temp += this_values[i]*this_values[i];
	}
	return (T) sqrt(temp);
    }
      
    T norm_inf () const 
    {
	T temp                = T();
	int const stop        = this->length;
	T*  const this_values = this->values;   // force MTA to place on stack
	
	#pragma mta assert parallel
	for (int i = 0; i < stop; i++)
	{
	    if (fabs(this_values[i]) > temp)
	    {
		temp = (T) fabs(this_values[i]);
	    }
	}
	return temp;
    }
      
    int VectorLength() const  
    {
	return this->length;
    }  
      
    friend DenseVector<T> 
     operator* BRAKET(const SparseMatrixCSR<T>&, const DenseVector<T>&);
     
    DenseVector<T> friend
     operator* BRAKET (const SparseMatrixCSC<T>&, const DenseVector<T>&);
     
    DenseVector<T> friend
     operator* BRAKET (const SparseMatrixCOO<T>&, const DenseVector<T>&);
     
    DenseVector<T> friend
     operator* BRAKET (int const, const DenseVector<T>&);
    
    DenseVector<T> friend
     operator* BRAKET (double const, const DenseVector<T>&);
    
    T friend
     operator* BRAKET (const DenseVector<T>&, const DenseVector<T>&);
     
    DenseVector<T> friend
     diagonal BRAKET (const SparseMatrixCSR<T>&);
    
    DenseVector<T> friend
     Transpose_SMVm BRAKET (const SparseMatrixCSR<T>&, const DenseVector<T>&);
   
    // approximate solver, asolve, derived from  asolve.c found with linbcg.c
    // in "Numerical Recipes in C", second edition, Press, Vetterling, Teukolsky,
    // Flannery, pp 86-89
    DenseVector<T> asolve (DenseVector<T>& a)  {
      DenseVector<T> temp(this->length);
  
      int const stop        = this->length;
      T*  const temp_values = temp.values;   // force MTA to place on stack
      T*  const this_values = this->values;  // force MTA to place on stack
      T*  const a_values    = a.values;      // force MTA to place on stack
      T   const zero        = T();
      #pragma mta assert parallel
      for (int i = 0; i < stop; i++)
        temp_values[i] = (this_values[i] != zero) 
         ? a_values[i]/this_values[i] : a_values[i];
      return temp;
    }
          
    // the following methods are for development and may be removed/modified
    /* DenseVector::fill() */
    void fill (T const* const val)  {
      int const stop        = this->length;
      T*  const this_values = this->values;  // to force MTA to place on stack
      #pragma mta assert parallel
      for (int i = 0; i < stop; i++) {
        this_values[i] = val[i];
      }
    }
      
    void VectorPrint(char const* name)  
    {
	printf("DenseVector Print %s length %d\n", name,this->length);
      
	int minimum = (this->length < 10) ? this->length : 10;
      
	printf("DenseVector Values: ");
	for (int i = 0; i < minimum; i++) 
	{
	    printf("%g, ", this->values[i]);
	}
	printf("\n");
    }

};



template <class T> DenseVector<T> inline
operator* (int const a, const DenseVector<T>& b)  
{
    DenseVector<T> temp (b.length);

    int const stop        = b.length;
    T*  const temp_values = temp.values;       // force MTA to place on stack
    T*  const b_values    = b.values;          // force MTA to place on stack

    #pragma mta assert parallel
    for (int i = 0; i < stop; i++)
    {
	temp_values[i] = a * b_values[i];
    }
    return temp;
}

template <class T> DenseVector<T> inline
operator* (double const a, const DenseVector<T>& b)  
{
    DenseVector<T> temp (b.length);
  
    int const stop        = b.length;
    T*  const temp_values = temp.values;      // force MTA to place on stack
    T*  const b_values    = b.values;          // force MTA to place on stack
    
    #pragma mta assert parallel
    for (int i = 0; i < stop; i++)
    {
	temp_values[i] = a * b_values[i];
    }
    return temp;
}

template <class T> T inline
operator* (const DenseVector<T>& a, const DenseVector<T>& b) {
   
   T temp = T();
   int const stop     = a.length;
   T*  const a_values = a.values;            // force MTA to place on stack
   T*  const b_values = b.values;            // force MTA to place on stack
   #pragma mta assert parallel
   for (int i = 0; i < stop; i++)
      //safe_incr(temp, a_values[i] * b_values[i]) ;
      temp += a_values[i]*b_values[i];
   return temp;
}
      
template <class T> DenseVector<T>  
operator- (const DenseVector<T>& a, const DenseVector<T>& b) {
   DenseVector<T> temp(a);
   temp -= b;
   return temp;
}

template <class T> DenseVector<T>  
operator+ (const DenseVector<T>& a, const DenseVector<T>& b) {
   DenseVector<T> temp(a);
   temp += b;
   return temp;
}   
  
   
   
/***************************** CSR  SparseMatrix ******************************/
/* CSR : Compressed Sparse Row                                                */
template <class T = double> class SparseMatrixCSR : MatrixBase<T>
{
  private:
    int* columns;
    
  public:
    SparseMatrixCSR() : MatrixBase<T>(), columns(0)
    {
    }
      
    SparseMatrixCSR(int const row, int const col, int const count) 
		    : MatrixBase<T> (row, col, count,true)
    {
	 // columns=new int[count];
	this->columns=(int*)malloc(count *sizeof(int)); 

	// index = new int [nRow+1];
	this->index  =(int*)malloc((this->nRow+1)*sizeof(int));
       
	// force MTA to place on stack
	int* const this_columns = this->columns; 
	int  const finish       = this->nNonZero;
	#pragma mta assert parallel
	for (int i = 0; i < finish; i++)
	    this_columns[i] = 0;        
       
	// force MTA to place on stack
	int* const this_index = this->index;
	int  const stop       = this->nRow+1;
	#pragma mta assert parallel
	for (int i = 0; i < stop; i++)
	    this_index[i] = 0;
    }

    SparseMatrixCSR(const int row, const int col, const int count, 
		    int* indx, T* val, int*  cols)  
	: MatrixBase<T>(row, col, count,false),
		MatrixBase<T>::index(indx), columns(cols)
    {
    }
     
    SparseMatrixCSR(const SparseMatrixCSR<T>& a) : MatrixBase<T> (a) 
    {
	columns=(int*)malloc(this->nNonZero *sizeof (int));  //columns=new int[nNonZero];
	this->index  =(int*)malloc((this->nRow+1) *sizeof (int));  //index  =new int[nRow+1]; 
      
	int  const stop         = this->nNonZero;
	int* const this_columns = this->columns;   // force MTA to place on stack
	int* const a_columns    = a.columns;       // force MTA to place on stack
	int*   const a_index     = a.index;        // force MTA to place on stack
	
	#pragma mta assert parallel
	for (int i = 0; i < stop; i++)
	{
	    this_columns[i] = a_columns[i];
	}
        
	int* const this_index = this->index;       // force MTA to place on stack
	int  const end        = this->nRow+1;      // force MTA to place on stack
	#pragma mta assert parallel
	for (int i = 0; i < end; i++)
	{
	    this_index [i] = a_index[i]; 
	}
    }
      
    ~SparseMatrixCSR()  
    {
	if (this->data_owned && this->columns) free (this->columns);
	if (this->data_owned && this->index)   free (this->index); 
	this->columns = 0;
	this->index   = 0;
    }

    void init(const int row, const int col, const int count, 
		    int* indx, T* vals, int*  cols)  
    {
	clear();
	this->nRow = row; 
	this->nCol = col; 
	this->nNonZero = count;
	this->index = indx; 
	this->columns = cols; 
	this->values = vals; 
	this->data_owned = false;
    }


    void clear() {
	if(this->data_owned && this->values) free(this->values); 
	this->values = 0;
	if(this->data_owned && this->index)  free(this->index);  
	this->index  = 0;
    }


    SparseMatrixCSR<T>& operator= (const SparseMatrixCSR<T>& a)  {
	if (this != &a)  
	{
	    this->nRow     = a.nRow;
	    this->nCol     = a.nCol;
	    this->nNonZero = a.nNonZero;
        
//	    delete[] columns;   columns = new int [this->nNonZero];
//	    delete[] index;     index   = new int [this->nRow+1];
//	    delete[] values;    values  = new T   [this->nNonZero];
        
	    if (columns) 
		free (columns);  
	    columns = (int*) malloc(this->nNonZero * sizeof(int));
        
	    if (this->index)   
		free (this->index);
	    this->index = (int*) malloc((this->nRow+1) * sizeof(int));
        
	    if (this->values)  
		free (this->values);
	    this->values = (T*) malloc(this->nNonZero * sizeof(T));
      
	    int  const stop         = this->nNonZero;// force MTA to place on stack
	    int* const this_columns = this->columns; // force MTA to place on stack
	    T*   const this_values  = this->values;  // force MTA to place on stack
	    int* const a_columns    = a.columns;     // force MTA to place on stack
	    T*   const a_values     = a.values;      // force MTA to place on stack
	    #pragma mta assert parallel
	    for (int i = 0; i < stop; i++)  
	    {
		this_columns[i] = a_columns[i];
	    this_values[i]  = a_values[i];
	    }
        
	    int  const end        = this->nRow + 1;  // force MTA to place on stack
	    int* const this_index = this->index;     // force MTA to place on stack
	    int* const a_index    = a.index;         // force MTA to place on stack
	    #pragma mta assert parallel
	    for (int i = 0; i < end; i++)
	    {
	        this_index[i] = a_index[i];
	    }
	}
	return *this;
    }
      
    SparseMatrixCSR<T>& operator= (const SparseMatrixCSC<T>& a)  
    {
	printf("NOT IMPLEMENTED Converting from CSC to CSR\n");
	return *this;
    }
      
    DenseVector<T> friend
     operator* <T> (const SparseMatrixCSR<T>&, const DenseVector<T>&);
     
    SparseMatrixCSR<T> friend
     operator* <T> (double const, const SparseMatrixCSR<T>&);
    
    SparseMatrixCSR<T> friend
     operator* <T> (int const, const SparseMatrixCSR<T>&);    
     
    DenseVector<T> friend
     diagonal BRAKET (const SparseMatrixCSR<T>&);
    
    DenseVector<T> friend
     Transpose_SMVm BRAKET (const SparseMatrixCSR<T>&, const DenseVector<T>&);
         
    /* SparseMatrixCSR fill()
     *
     */
    void fill (int const* const indx, 
	       T   const* const val, 
	       int const* const cols)  
    {
	int  const stop       = this->nRow;     // force MTA to place on stack
	int* const this_index = this->index;    // force MTA to place on stack

	#pragma mta assert parallel
	for (int rows=0; rows < stop; rows++)
	{
	    this_index[rows] = indx[rows];
	}
	
	this_index[this->nRow] = this->nNonZero;
      
	int  const end          = this->nNonZero;  // force MTA to place on stack
	T*   const this_values  = this->values;    // force MTA to place on stack
	int* const this_columns = this->columns;   // force MTA to place on stack
	
	#pragma mta assert parallel
	for (int i=0; i < end; i++)  
	{
	    this_values[i]  = val[i];
	    this_columns[i] = cols[i];
	}
    }
    
    T* col_values_begin(int row)
    {
	if ((row >= 0) && (row < this->nRow))
		return &this->values[this->index[row]];
	else
		return 0;
    }

    T* col_values_end(int row)
    {
	int ind = row+1;
	if ((ind >= 0) && (ind <= this->nRow))
		return &this->values[this->index[ind]];
	else
		return 0;
    }

    int* col_indices_begin(int row)
    {
	if ((row >= 0) && (row < this->nRow))
		return &this->columns[this->index[row]];
	else
		return 0;
    }

    int* col_indices_end(int row)
    {
	int ind = row+1;
	if ((ind >= 0) && (ind <= this->nRow))
		return &this->columns[this->index[ind]];
	else
		return 0;
    }
    
    void MatrixPrint (char const* name) const  
    {
	printf("SparseMatrixCSR Print %s row %d col %d\n",
		name,this->nRow,this->nCol);
    }
    
    
    int MatrixRows() const {
      return this->nRow;
    }
    

    int MatrixCols() const {
      return this->nCol;
    }     
};

template <class T> DenseVector<T>
operator* (const SparseMatrixCSR<T>& a, const DenseVector<T>& b) {
 
  if (a.nCol != b.length)  
  {
      printf("INCOMPATIBLE SparseMatrixCSR * DenseVector multiplication\n");
      exit(1);
  }
    
  DenseVector<T> temp(b.length);
  T*  const temp_values = temp.values;         // force MTA to place on stack
  T   const zero        = T();
  int const finish      = temp.length;
  #pragma mta assert parallel
  for (int i = 0; i < finish; i++)
    temp_values[i] = zero;
    
  #ifdef __MTA__  
    int starttimer = mta_get_clock(0);
  #endif

  int  const stop      = a.nRow;
  int* const a_index   = a.index;            // force MTA to place on stack
  T*   const a_values  = a.values;           // force MTA to place on stack
  T*   const b_values  = b.values;           // force MTA to place on stack
  int* const a_columns = a.columns;          // force MTA to place on stack
  #pragma mta assert parallel  
  for (int row = 0; row < stop; row++) {
    #pragma mta trace "next_row"
    int const start  = a_index[row];
    int const finish = a_index[row+1];
  
    for (int i = start; i < finish; i++) 
    {
	temp_values[row] += a_values[i]*b_values[ a_columns[i] ];
    }
  }
      
#ifdef __MTA__
int stoptimer = mta_get_clock(starttimer);
// printf("MVm total time %g\n", stoptimer/220000000.0);
#endif
      
  return temp; 
}

template <class T> SparseMatrixCSR<T>
operator* (int const a, const SparseMatrixCSR<T>& b) {      
  SparseMatrixCSR<T> temp (b.nRow, b.nCol, b.nNonZero);
  
  int const stop        = b.nNonZero;
  T*  const temp_values = temp.values;         // force MTA to place on stack
  T*  const b_values    = b.values;            // force MTA to place on stack
  #pragma mta assert parallel
  for (int i = 0; i < stop; i++)
    temp_values[i] = a * b_values[i];
  return temp;
}

template <class T> SparseMatrixCSR<T>
operator* (double const a, const SparseMatrixCSR<T>& b) {      
  SparseMatrixCSR<T> temp (b.nRow, b.nCol, b.nNonZero);
  
  int const stop = b.nNonZero;
  T* temp_values = temp.values;               // force MTA to place on stack
  T* b_values    = b.values;                  // force MTA to place on stack
  #pragma mta assert parallel
  for (int i = 0; i < stop; i++)
    temp_values[i] = a * b_values[i];
  return temp;
}

template <class T> DenseVector<T> diagonal (const SparseMatrixCSR<T>& a)  {
  if (a.nRow != a.nCol) {
      printf("diagonal called on non square matrix\n");
      printf("nRow=%d, nCol=%d\n", a.nRow, a.nCol);
      exit(1);
  }
    
  DenseVector<T> temp(a.nRow);
  temp();
  
  int const finish = a.nRow;
  int* const a_index   = a.index;              // force MTA to place on stack
  int* const a_columns = a.columns;            // force MTA to place on stack
  T* const a_values    = a.values;             // force MTA to place on stack
  T* const temp_values = temp.values;          // force MTA to place on stack
  #pragma mta assert parallel
  #pragma mta loop future
  for (int row = 0; row < finish; row++)  {
    int const start = a_index[row];
    int const stop  = a_index[row+1];
    
    #pragma mta assert parallel
    for (int i = start; i < stop; i++)
      if (row == a_columns[i]) {
        temp_values[row] = a_values[i];
        #ifndef __MTA__
          break;
        #endif
      }
  }
  return temp;
}
  
template <class T> DenseVector<T> Transpose_SMVm (const SparseMatrixCSR<T>& a,
  const DenseVector<T>& b)  {
     
  if (a.nCol != b.length)  
  {
      printf("INCOMPATIBLE Transpose (SparseMatrixCSR) * DenseVector multiplication\n");
      exit(1);
  }
    
  DenseVector<T> temp(b.length);
  T* const temp_values = temp.values;        // force MTA to place on stack
  T* const a_values    = a.values;           // force MTA to place on stack
  T* const b_values    = b.values;           // force MTA to place on stack
  int* const a_index   = a.index;            // force MTA to place on stack
  int* const a_columns = a.columns;          // force MTA to place on stack
  int const stop = temp.length;
     
  #pragma mta assert parallel
  T const zero = T();
  for (int i = 0; i < stop; i++)
    temp_values[i] = zero;
    
  int const finish = temp.length;

  #pragma mta assert parallel    
  for (int row = 0; row < finish; row++)  {
    int const start = a_index[row];            // force MTA to place on stack
    int const stop  = a_index[row+1];          // force MTA to place on stack
    
    for (int i = start; i < stop; i++) {
        //mt_incr(temp_values[a_columns[i]], a_values[i] * b_values[row]);
	temp_values[ a_columns[i] ] += a_values[i] * b_values[row];
    }
  }
  return temp;         
}



/***************************** CSC  SparseMatrix ******************************/

template <class T = double> class SparseMatrixCSC : MatrixBase<T> {
  private:
    int *rows;
  public:
    SparseMatrixCSC<T> () : MatrixBase<T> () {
      rows = 0;
    }
       
    SparseMatrixCSC<T> (int const row, int const col, int const count)
     : MatrixBase<T> (row, col, count)  {
       rows  = (int*) malloc (count    * sizeof(int)); // rows (new int [count])
       this->index = (int*) malloc ((this->nCol+1) * sizeof(int)); // index(new int [this->nCol+1])
    }
       
    SparseMatrixCSC<T> (const SparseMatrixCSC<T>& a) : MatrixBase<T> (a)  {
      rows =(int*)malloc(this->nNonZero *sizeof(int)); // rows(new int[nNonZero])
      this->index=(int*)malloc((this->nCol+1) *sizeof(int)); // index(new int[nCol+1])    
      
      int* const a_rows    = a.rows;            // force MTA to place on stack
      int* const this_rows = this->rows;        // force MTA to place on stack
      int  const stop      = this->nNonZero;
      #pragma mta assert parallel
      for (int i = 0; i < stop; i++)
        this_rows[i] = a_rows[i];
        
      int* const this_index = this->index;      // force MTA to place on stack
      int* const a_index    = a.index;          // force MTA to place on stack
      int  const end        = this->nCol + 1;
      #pragma mta assert parallel
      for (int i = 0; i < end; i++)
        this_index[i] = a_index[i];
    }
      
    ~SparseMatrixCSC<T> ()  {    
        if (this->data_owned && this->rows)  free (this->rows);
        if (this->data_owned && this->index) free (this->index);
        this->rows  = 0;
        this->index = 0;
    }
 
    SparseMatrixCSC<T>& operator= (const SparseMatrixCSR<T>& a)  
    {
	printf("NOT IMPLEMENTED: Converting from CSR to CSC\n");
        return *this;
    }
      
    SparseMatrixCSC<T>& operator= (const SparseMatrixCSC<T>& a)  {
      if (this != &a)  {
        this->nRow     = a.nRow;
        this->nCol     = a.nCol;
        this->nNonZero = a.nNonZero;
      
        int* const this_rows   = this->rows;       // force MTA to place on stack
        int* const a_rows      = a.rows;           // force MTA to place on stack
        T*   const this_values = this->values;     // force MTA to place on stack
        T*   const a_values    = a.values;         // force MTA to place on stack
        int  const stop        = this->nNonZero;
        #pragma mta assert parallel
        for (int i = 0; i < stop; i++)  {
          this_rows[i]   = a_rows[i];
          this_values[i] = a_values[i];
        }
        
        int* const this_index = this->index;      // force MTA to place on stack
        int* const a_index    = a.index;          // force MTA to place on stack
        int  const end        = this->nCol + 1;
        #pragma mta assert parallel
        for (int i = 0; i < end; i++)
          this_index[i] = a_index[i];
      }
      return *this;
    }
      
    SparseMatrixCSC<T>& operator() (SparseMatrixCSR<T>&);
    
    friend DenseVector<T>
     operator* <T> (const SparseMatrixCSC<T>&, const DenseVector<T>&);
    
    void MatrixPrint (char const* name)  
    {
	printf("SparseMatrixCSC Print %s row %d col %d\n",
		name,this->nRow,this->nCol);
    }
      
    int MatrixRows() const {
      return this->nRow;
    }
      
    int MatrixCols() const {
      return this->nCol;
    }
};



template <class T> DenseVector<T>
operator* (const SparseMatrixCSC<T>& a, const DenseVector<T>& b) {
  if (a.nCol != b.length)  
  {
      printf("INCOMPATIBLE SparseMatrixCSC * DenseVector multiplication\n");
      exit(1);
  }
    
  DenseVector<T> temp(b.length);
  T*  const temp_values = temp.values;       // force MTA to place on stack
  int const istop       = temp.length;
  T   const zero        = T();
  #pragma mta assert parallel
  for (int i = 0; i < istop; i++)
    temp_values[i] = zero;
    
  int  const colstop  = a.nCol; 
  int* const a_index  = a.index;          // force MTA to place on stack
  int* const a_rows   = a.rows;           // force MTA to place on stack
  T*   const a_values = a.values;         // force MTA to place on stack 
  T* const b_values    = b.values;           // force MTA to place on stack
  #pragma mta assert parallel
  #pragma mta loop future
  for (int col = 0; col < colstop; col++) {
    int const start = a_index[col];
    int const stop  = a_index[col+1];
  
    #pragma mta assert parallel
    for (int i = start; i < stop; i++) {
        //mt_incr(temp_values[a_rows[i]], a_values[i] * b_values[a_rows[i]]);
        temp_values[ a_rows[i] ] += a_values[i] * b_values[ a_rows[i] ];
    }
  }
  return temp;
}



/***************************** COO  SparseMatrix ******************************/
/* COO : */

template <class T = double> class SparseMatrixCOO : MatrixBase<T>
{
  private:
    int *columns;
    int *rows;
    
  public:
    SparseMatrixCOO() : MatrixBase<T> ()  {
      this->columns = 0;
      this->rows    = 0;
      }
      
    SparseMatrixCOO(int const row, int const col, int const nnz) 
     : MatrixBase<T> (row, col, nnz)  {
        rows    = (int*) malloc (nnz * sizeof(int));   // rows    = new int[nnz]
        columns = (int*) malloc (nnz * sizeof(int));   // columns = new int[nnz]
    }
        
    SparseMatrixCOO (SparseMatrixCOO<T>& a) : MatrixBase<T> (a)  { 
      rows   =(int*)malloc(a.nNonZero*sizeof(int)); //rows   =new int[a.nNonZero]
      columns=(int*)malloc(a.nNonZero*sizeof(int)); //columns=new int[a.nNonZero]
      
      int  const stop         = this->nNonZero;
      T*   const this_values  = this->values;     // MTA -to force onto stack 
      T*   const a_values     = a.values;         // MTA -to force onto stack 
      int* const this_rows    = this->rows;       // MTA -to force onto stack 
      int* const this_columns = this->columns;    // MTA -to force onto stack 
      int* const a_rows       = a.rows;           // MTA -to force onto stack 
      int* const a_columns    = a.columns;        // MTA -to force onto stack 
      #pragma mta assert parallel
      for (int i = 0; i < stop; i++)  {
        this_values[i]  = a_values[i];
        this_rows[i]    = a_rows[i];
        this_columns[i] = a_columns[i];
      }
    }
     
    ~SparseMatrixCOO()  {
       if (this->columns)  free (this->columns);         // delete[] columns
       if (this->index)    free (this->index);           // delete[] index
       if (this->rows)     free (this->rows);            // delete[] rows
       this->columns = 0;
       this->index   = 0;
       this->rows    = 0;
    }
      
    SparseMatrixCOO<T>& operator= (const SparseMatrixCOO<T>& a)  {
      if (this != &a)  {
        this->nRow     = a.nRow;
        this->nCol     = a.nCol;
        this->nNonZero = a.nNonZero;
        this->index    = 0;
        
        if (rows)   free(rows);                      // delete[] rows; 
        rows=(int*)malloc(a.nNonZero*sizeof(int));   // rows = new int[a.nNonZero]
        
        if (columns) free(columns);                   //delete[] columns; 
        columns=(int*)malloc(a.nNonZero*sizeof(int)); //columns=new int[a.nNonZero]
        
        if (this->values)  free(this->values);                   //delete[] values; 
        this->values=(int*)malloc(a.nNonZero*sizeof(int)); //values=new int[a.nNonZero]
      
        int  const stop         = this->nNonZero;
        T*   const this_values  = this->values;     // MTA -to force onto stack 
        T*   const a_values     = a.values;         // MTA -to force onto stack 
        int* const this_rows    = this->rows;       // MTA -to force onto stack 
        int* const this_columns = this->columns;    // MTA -to force onto stack 
        int* const a_rows       = a.rows;           // MTA -to force onto stack 
        int* const a_columns    = a.columns;        // MTA -to force onto stack 
        #pragma mta assert parallel
        for (int i = 0; i < stop; i++)  {
          this_values[i]  = a_values[i];
          this_rows[i]    = a_rows[i];
          this_columns[i] = a_columns[i];
        }
      }
      return *this;
    }    
    
    SparseMatrixCOO<T>& operator= (const SparseMatrixCSR<T>& a)  
    {
        printf("NOT IMPLEMENTED: Converting from CSR to COO\n");
        return *this;
    }
      
    SparseMatrixCOO<T>& operator= (const SparseMatrixCSC<T>& a)  
    {
        printf("NOT IMPLEMENTED: Converting from CSC to COO\n");
        return *this;
    }
           
    SparseMatrixCOO<T>& operator() (const SparseMatrixCSR<T>&);
    SparseMatrixCOO<T>& operator() (const SparseMatrixCSC<T>&);
    
    DenseVector<T> friend
     operator* <T> (const SparseMatrixCOO<T>&, const DenseVector<T>&);
    
    void MatrixPrint (char const* name)  
    {
	printf("SparseMatrixCOO Print %s row $d col %d\n",name,this->nRow,this->nCol);
    }
};



template <class T> DenseVector<T>
operator* (const SparseMatrixCOO<T>& a, const DenseVector<T>& b)
{
  if (a.nCol != b.length)  
  {
      printf("INCOMPATIBLE SparseMatrixCoo * DenseVector multiplication\n");
      exit(1);
  } 
    
  DenseVector<T> temp(b.length);
  int const stop        = temp.length;
  T*  const temp_values = temp.values;
  T   const zero        = T();
  #pragma mta assert parallel
  for (int i = 0; i < stop; i++)
    temp_values[i] = zero;
  
  int  const end       = a.nNonZero;
  T*   const a_values  = a.values;        // MTA -to force onto stack 
  T*   const b_values  = b.values;        // MTA -to force onto stack 
  int* const a_rows    = a.rows;          // MTA -to force onto stack 
  int* const b_rows    = b.rows;          // MTA -to force onto stack 
  int* const a_columns = a.columns;       // MTA -to force onto stack 
  #pragma mta assert parallel
  for (int i = 0; i < end; i++)
    mt_inc (temp_values[a_rows[i]], a_values[i] * b_values[a_columns[i]]);
  return temp;
}


/* biconjugate gradient solver derived from linbcg.c in "Numerical Recipes */
/* in C", second edition, Press, Vetterling, Teukolsky, Flannery, pp 86-89 */
template <class T> DenseVector<T>&
linbcg (const SparseMatrixCSR<T>& A,
  DenseVector<T>& x,
  const DenseVector<T>& b, 
  int const itermax, 
  T& err, 
  T const tol)
{
  int const length = A.MatrixRows();
  double const bnorm = b.norm2();
  double ak, akden, bk, bknum, bkden;

  DenseVector<T> p(length), pp(length);
  DenseVector<T> r(length), rr(length);
  DenseVector<T> z(length), zz(length);
  DenseVector<T> d = diagonal(A);
  
  r  = b - (A * x); 
  z  = d.asolve (r);
  rr = r;
  
  for (int iter = 0; iter < itermax; iter++)  {
    zz = d.asolve (rr);

    bknum = z * rr;
    
    if (iter == 0)  {
       p  = z;
       pp = zz;       
       }
    else  {
       bk = bknum / bkden;
       p  = (bk * p)  + z;
       pp = (bk * pp) + zz;
       }
    bkden =  bknum;

    z     = A * p;
    akden = z * pp;
    ak    = bknum / akden;

    zz = Transpose_SMVm(A, pp);

    x  += (ak * p);
    r  -= (ak * z);
    rr -= (ak * zz);

    z = d.asolve(r); 
    err = r.norm2() / bnorm;
    if (err < tol)
      break;
    }
  return x;
}
#endif

// EOF
