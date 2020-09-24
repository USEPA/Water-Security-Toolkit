// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef BASICS_H
#define BASICS_H

#include <math.h>
#include "stdio.h"
#include "bossa_random.h"

#define POPSTAR_INFINITY 10e30
const double EPSILON = 0.001;
const double PIAPPROX = 3.141592653589793238;
const double LOG2 = 0.30102999566398119521373889472449;
const double LN2 = 0.69314718055994530941723212145818;

class IntDouble {
 public:
  int id;
  double value;
  static int compare (const void *_a, const void *_b);
};

inline int operator < (IntDouble c1, IntDouble c2) {return (c1.value < c2.value);}
inline int operator <= (IntDouble c1, IntDouble c2) {return (c1.value <= c2.value);}
inline int operator == (IntDouble c1, IntDouble c2) {return (c1.value == c2.value);}
inline int operator >= (IntDouble c1, IntDouble c2) {return (c1.value >= c2.value);}
inline int operator > (IntDouble c1, IntDouble c2) {return (c1.value > c2.value);}

class UnsignedIntPair {
 public:
  unsigned int a;
  unsigned int b;
};

inline int operator < (UnsignedIntPair x, UnsignedIntPair y) {
  return (x.a<y.a || (x.a==y.a && x.b<y.b));
}

inline int operator <= (UnsignedIntPair x, UnsignedIntPair y) {
  return (x.a<y.a || (x.a==y.a && x.b<=y.b));
}

inline int operator > (UnsignedIntPair x, UnsignedIntPair y) {
  return (x.a>y.a || (x.a==y.a && x.b>y.b));
}

inline int operator >= (UnsignedIntPair x, UnsignedIntPair y) {
  return (x.a>y.a || (x.a==y.a && x.b>=y.b));
}

inline int operator == (UnsignedIntPair x, UnsignedIntPair y) {
  return (x.a==y.a && x.b==y.b);
}


/***********************************************
 *
 * aux function: randomly permutes the first n
 * elements of vector v (starting at 1)
 *
 ***********************************************/

class RFW 
{
 public: 
  static void fatal (const char *fund, const char *msg);		
  static void shuffle (int *v, int n);
  static int getRank (double *v, int size, int pos);
  static int argmin (double *v, int p1, int p2);
  static int getWeightedIndex (double *v, int p1, int p2, double x, double factor=1);
  static int argmax (double *v, int p1, int p2);
  static int randomSmallest (double *v, int p1, int p2, int s);
  static int randomLargest (double *v, int p1, int p2, int s);
  static void bound (int &c, int min, int max);
  static bool stripPath (char *str);
  static bool stripExtension (char *str);
  static double deg2rad (double d);
  static double dms2rad (int d, int m, int s);
  static double l2norm (double x, double y, double z);
  static double sphered (double lg1, double lt1, double lg2, double lt2);
  static int intcomp (const void *a, const void *b);
  static inline double pow (double n, double i) {
    if (i==1) return n;
    if (i==0) return 1;
    if (n==0) return 0;
    return (exp (i * log(n)));
  }
};






template <class T> int partition (T *array, int p1, int p2, T pvt) {
  T *i, *j, tmp;

  i = &array[p1-1]; //- 1; //l - 1;
  j = &array[p2+1]; //r + 1;

  while (1) {
    do {i++;} while (*i < pvt);
    do {j--;} while (pvt < *j);
    if (i >= j) break;

    //in this point *i>=pvt and *j<=pvt
    //swap them
    tmp = *i; *i = *j; *j = tmp;
  };
  //facts: *i>=pvt, *j<=pvt

  return (i - &array[p1]) + p1; //this is a valid position to insert the pvt; 
  //note that it can be greater than r (can be r+1) 
}






/*
  template <class T> void partialSort (T *array, int p1, int p2, int pm) {
  int right = p2;
  int left = p1;

  while (1) {
  int pvt = arrayBossaRandom::getInteger(left, right);
  int pos = partition (array, left, right, array[pvt]);

  }
  }*/



/*
  static void testPartition () {
  const int size = 40;
  int test[size+1];

  for (int i=1; i<=100000; i++) {
  int j;
  for (j=1; j<=size; j++) {test[j] = BossaRandom::getInteger(1,size);}
  int pvt = BossaRandom::getInteger(0,size+1);

  int p = partition(test,1,size,pvt);
		
  for (j=1; j<p; j++) {if (test[j]>pvt) fprintf (stderr, "ARGH\n");}
  for (j=p; j<=size; j++) {if (test[j]<pvt) fprintf (stderr, "BLARGH\n");}
  fprintf (stderr, ".");
  }
  }
*/

/*
  void <class T> void findFirst (T*p, T *l, T *r) {
  T *i, *j, *left, *right;
  T pvt, tmp;
  *i = *left = *l;
  *j = *right = *r;

  while (1) {
  mid = left + BossaRandom::getInteger (0, right - left);
  pvt = *mid;

  *mid = *(j = right-1); //transfer



  //partition
  while (1) {	
  do {i++;} while (*i < pvt);
  do {j--;} while (pvt < *j);
  if (i >= j) break;
  tmp = *i; *i = *j; *j = tmp;
  };

  //restore pivot
  if (i < right) {		
  *(right-1) = *i; *i = pvt;
  } else {*(--i) = pvt;}


  if (i==p) break; //is the pivot in the position we wanted
		
  if (i<p) {right = (i-1);} //our goal is to the left of p
  else {left = (i+1);}
  if (left==right) break;
  }
  }*/










template <class T> int argmin (T *array, int p1, int p2) {
  int m = p1;
  for (p1=p1+1; p1<=p2; p1++) {
    if (array[p1] < array[m]) m = p1;
  }
  return m;
}

/*************************************
 *
 * 
 *
 *************************************/


template <class T> inline void partialShuffle (T *array, int left, int aright, int sright) {
  T temp;
  for (int i=left; i<=sright; i++) {
    int j = BossaRandom::getInteger (i, aright);
    temp = array[i];
    array[i] = array[j];
    array[j] = temp;
  }
}


template <class T> void resize (T** array, unsigned int oldsize, unsigned int newsize) {
  if (oldsize == newsize) return;

  fprintf (stderr, "Preparing to resize.\n");

  T* o = *array;          //old array
  T *n;
  try {	
    n = new T [newsize]; //new array
  } catch (...) {
    fprintf (stderr, "Not enough memory.\n");
  }
  if (n==0) fprintf (stderr, "Not enough memory.\n");
	
  fprintf (stderr, "New array created.\n");
  int minsize = (oldsize < newsize) ? oldsize : newsize;

  fprintf (stderr, "Minsize is %d.\n", minsize);
  for (int i=0; i<minsize; i++) {n[i] = o[i];}
  delete [] o;
  *array = n;

  fprintf (stderr, "Array resized.\n");
}


#endif
