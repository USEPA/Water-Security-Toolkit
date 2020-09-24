// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef rfw_stats_h
#define rfw_stats_h

#include "bossa_random.h"

class RFWStats {
 public:
  static int dcmp (const void *a, const void *b) {
    double da = *(double *)a;
    double db = *(double *)b;
			
    if (da<db) return -1;
    if (da>db) return 1;
    return 0;
  }


  static double sum (double *v, int p1, int p2) {
    double total = 0;
    for (int i=p1; i<=p2; i++) total += v[i];
    return total;
  }

  static double average (double *v, int p1, int p2) {
    double n = (double) (p2 - p1 + 1);
    return sum(v,p1,p2) / n;
  }

  static double sumsq (double *v, int p1, int p2) {
    double total = 0;
    for (int i=p1; i<=p2; i++) total += (v[i]*v[i]);
    return total;
  }

  static int argmin (double *v, int p1, int p2) {
    int m = p1;
    for (int i=p1+1; i<=p2; i++) {
      if (v[i]<v[m]) m = i;
    }
    return m;
  }

  static double min (double *v, int p1, int p2) {
    return v[argmin(v,p1,p2)];
  }

  static int argmax (double *v, int p1, int p2) {
    int m=p1;
    for (int i=p1; i<=p2; i++) {
      if (v[i]>v[m]) m = i;
    }
    return m;
  }

  static double max (double *v, int p1, int p2) {
    return v[argmax(v,p1,p2)];
  }

  static double variance (double *v, int p1, int p2) {
    double ssq = sumsq (v, p1, p2);
    double avg = average (v, p1, p2);
    double n = (double) (p2 - p1 + 1);
    double var = (ssq / n) - avg * avg;

    return var;
  }

  static double stddev (double *v, int p1, int p2) {
    return sqrt (variance(v,p1,p2));
  }

  static void output (FILE *file, double *v, int p1, int p2) {
    for (int i=p1; i<=p2; i++) {
      fprintf (file, " %.1f", v[i]);
    }
    fprintf (stderr, "\n");
  }



  //return the position of the last element strictly less than x
  static int partition (double x, double *v, int p1, int p2) {
    int left = p1;
    int right = p2;

    output (stderr, v, p1, p2);

    for (;;) {
      if (left>=right) break;
      if (v[left]<x) left++; 
      else if (v[right]>x) right--;
      else {
	double temp = v[left];
	v[left] = v[right];
	v[right] = temp;
	left++;
	right--;
      }
    }
    output (stderr, v, p1, p2);
    return right;

  }

  static void sort (double *v, int p1, int p2) {
    qsort (&v[p1], p2-p1+1, sizeof (double), &RFWStats::dcmp);
  }

  static bool sorted (double *v, int p1, int p2) {
    for (int i=p1+1; i<=p2; i++) {
      if (v[i]<v[i-1]) return false;
    }
    return true;
  }

  static double getk (int k, double *v, int p1, int p2) {
    if (!sorted(v,p1,p2)) sort (v, p1, p2); 
    return v[k];
  }

  static double median (double *v, int p1, int p2) {
    if (!sorted(v,p1,p2)) sort (v, p1, p2);
    int n = p2 - p1 + 1;
    int n2 = n/2 + 1;

    if (n%2 == 0) {
      return (v[n2] + v[n2-1]) / 2;
    } else {
      return (v[n2]);
    }
  }
};


#endif
