// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include "basics.h"
#include "bossa_random.h"
#include <math.h>

int IntDouble::compare (const void *_a, const void *_b) {
  IntDouble *a = (IntDouble*) _a;
  IntDouble *b = (IntDouble*) _b;
  if (a->value < b->value) return -1;
  if (a->value > b->value) return 1;
  return 0;
}

int RFW::intcomp (const void *_a, const void *_b) {
  int a = *(int*)_a;
  int b = *(int*)_b;
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

void RFW::shuffle (int *v, int n) {
  int i, j, t;
  for (i=1; i<=n; i++) {
    j = BossaRandom::getInteger (i,n);
    t = v[j];
    v[j]  = v[i];
    v[i]  = t;
  }
}

void RFW::fatal (const char *func, const char *msg) {
  fprintf (stderr, "RFW::%s: %s.\n", func, msg);
  exit (-1);
}


//------------------------------------------
// get rank of the e-th element in vector v
//------------------------------------------
int RFW::getRank (double *v, int size, int pos) {
  double value = v[pos];
  int r=1;
  for (int i=1; i<=size; i++) {if (v[i]<value || (v[i]==value && i<pos)) r++;}
  return r;
}


int RFW::getWeightedIndex (double *v, int p1, int p2, double x, double alpha) {
  if (x<0 || x>1) fatal ("getWeightedIndex", "x value out of range");
	
  int i;
  double partial, goal, sum=0;

  //find sum
  for (i=p1; i<=p2; i++) sum+=RFW::pow(v[i], alpha);
  goal = sum * x;

  //fprintf (stderr, "(%.2f * %.2f = %.2f)\n", sum, x, goal);

  //find the value we want
  partial = 0;
  for (i=p1; i<=p2; i++) {
    partial += RFW::pow(v[i],alpha);
    if (partial >= goal) break;
  }
  if (i>p2) i=p2; //just avoiding rounding errors

  return i;
}



/************************************************
 * 
 * argmin (argmax): find the position in which
 * the min (max) element is found. 
 * 
 * In both functions, we assume that, in case of
 * tie, elements to the left are smaller than
 * elements to the right
 *
 ************************************************/

int RFW::argmin (double *v, int p1, int p2) {
  int x = p1;
  for (int i=p1+1; i<=p2; i++) {
    if (v[i]<v[x]) x=i;
  }
  return x;
}

int RFW::argmax (double *v, int p1, int p2) {
  int x = p1;
  for (int i=p1+1; i<=p2; i++) {
    if (v[i]>=v[x]) x=i;
  }
  return x;
}


/******************************************************
 *
 * picks at random one of the s smallest elements of v
 *
 * expected running time: size * log (size / s)
 * (this can be made to run in linear expected time, 
 * but this seems to require changing the input or
 * using extra memory)
 * 
 ******************************************************/

int RFW::randomSmallest (double *v, int p1, int p2, int s) {
  if (s<=1) return argmin (v,p1,p2);
  int x = BossaRandom::getInteger (p1,p2);
  for (;;) {
    double value = v[x];
    int y=-1, r=1;
    for (int i=p1; i<=p2; i++) {
      if (v[i]<value || (v[i]==value && i<x)) { //smaller
	r++;
	if (BossaRandom::getInteger (1,r-1)==1) y = i;
      }
    }
    if (r<=s) break;
    else x = y;
  }
  return x;
}

int RFW::randomLargest (double *v, int p1, int p2, int s) {
  if (s<=1) return argmax(v,p1,p2);
  int x = BossaRandom::getInteger (p1,p2);
  int size = p2-p1+1;
  for (;;) {
    double value = v[x];
    int y=-1, r=size;
    for (int i=p1; i<=p2; i++) {
      if (v[i]>value || (v[i]==value && i>x)) { //greater?
	r--;
	if (BossaRandom::getInteger (r,size-1)==r) y=i;
      }
    }
    if (r>size-s) break;
    else x = y;
  }
  return x;
}


void RFW::bound (int &c, int min, int max) {
  if (c < min) {c = min; return;}
  if (c > max) {c = max; return;}
}
			

bool RFW::stripPath (char *str) {
  int b=0; //beginning
  int e=0; //last slash or backslash
  for (e=0; str[e]!=0; e++) {
    if (str[e]=='/'||str[e]=='\\') b=e;
  }
  if (b!=0) {
    for (int i=b+1; i<=e; i++) str[i-b-1] = str[i];
  }
  return (b!=0);
}

bool RFW::stripExtension (char *str) {
  int i=0;
  while (str[i]!=0) i++;
  for (; i>=0; i--) {if (str[i]=='.') break;}
  if (i>=0) str[i] = '\0';
  return (i>=0);
}

double RFW::deg2rad (double d) {return (PIAPPROX * d / 180.0);}

double RFW::dms2rad (int d, int m, int s) {
  double deg = (double) d + (double)m / 60.0 + (double) s / 3600.00;
  return deg2rad(deg);	
}

double RFW::l2norm (double x, double y, double z) {
  return sqrt (x*x + y*y + z*z);
}

double RFW::sphered (double lg1, double lt1, double lg2, double lt2) {			
  double x1, y1, z1;
  double x2, y2, z2;
  //fprintf (stderr, "lg1=%f lt1=%f lg2=%f lt2=%f dp=%f\n", lg1, lt1, lg2, lt2, dp);
			
  //if (lg1<0) lg1 += PIAPPROX;
  //if (lg2<0) lg2 += PIAPPROX;
  //lt1 += PIAPPROX/2.0;
  //lt2 += PIAPPROX/2.0;

  x1 = cos (lg1) * cos (lt1);
  x2 = cos (lg2) * cos (lt2);
  y1 = sin (lg1) * cos (lt1);
  y2 = sin (lg2) * cos (lt2);
  z1 = sin(lt1);
  z2 = sin(lt2);


  double dp = x1*x2 + y1*y2 + z1*z2;
  //fprintf (stderr, "lg1=%f lt1=%f lg2=%f lt2=%f n1=%f n2=%f dp=%f\n", lg1, lt1, lg2, lt2, l2norm (x1,y1,z1), l2norm(x2,y2,z2), dp);

  if (dp>1) dp = 1;
  if (dp<-1) dp = -1;


  return acos (dp);
}
