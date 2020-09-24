// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/* A C-program for MT19937: Integer     version                   */
/*  genrand() generates one pseudorandom unsigned integer (32bit) */
/* which is uniformly distributed among 0 to 2^32-1  for each     */
/* call. sgenrand(seed) set initial values to the working area    */
/* of 624 words. Before genrand(), sgenrand(seed) must be         */
/* called once. (seed is any 32-bit integer except for 0).        */
/*   Coded by Takuji Nishimura, considering the suggestions by    */
/* Topher Cooper and Marc Rieffel in July-Aug. 1997.              */

/* This library is free software; you can redistribute it and/or   */
/* modify it under the terms of the GNU Library General Public     */
/* License as published by the Free Software Foundation; either    */
/* version 2 of the License, or (at your option) any later         */
/* version.                                                        */
/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of  */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.            */
/* See the GNU Library General Public License for more details.    */
/* You should have received a copy of the GNU Library General      */
/* Public License along with this library; if not, write to the    */
/* Free Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA   */ 
/* 02111-1307  USA                                                 */

/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.       */
/* Any feedback is very welcome. For any question, comments,       */
/* see http://www.math.keio.ac.jp/matumoto/emt.html or email       */
/* matumoto@math.keio.ac.jp                                        */



/* 'encapsulated' in a C++ class by Renato Werneck */
/* rwerneck@princeton.edu                          */

/* this main() outputs first 1000 generated numbers  */



#ifndef BOSSA_RANDOM_H
#define BOSSA_RANDOM_H

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>

#ifdef BOSSA_UNIX
#ifdef TEVA_SPOT_HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

class BossaRandom {
  /* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

  /* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

  static unsigned long mt[N]; /* the array for the state vector  */
  static int mti; /* mti==N+1 means mt[N] is not initialized */

  /* initializing the array with a NONZERO seed */
  static void sgenrand(unsigned long seed);
  static unsigned long genrand();
	
 public:
  static void randomize (unsigned long s) {
    if (s == 0) randomize();
    else sgenrand(s);
  }

  //static void randomize (int s) {randomize ((unsigned long)s);}

#ifdef Bossa_UNIX
  static void randomize(void) {randomize (time(0) + getpid());}
#else
  static void randomize(void) {randomize (time(0));}
#endif

  BossaRandom(void) 
    {
      randomize();
    }
  static unsigned long getRand(void) 
    {
      return genrand();
    }
		
  static unsigned long getNonZero(void) 
    {
      unsigned int u;
      do {u = getRand();} while (u==0);
      return u;
    }

  static int getInteger(int inf, int sup) 
    {
      // fprintf (stderr, "i");
      int i = (inf + (unsigned long)getRand() % (sup - inf + 1));
      // fprintf (stderr, "[%d;%d] -> %d", inf, sup, i);
      return i;
    }

  static float getFloat(void) 
    {
      float f = (float) ((float)(getRand()) / (unsigned long)0xffffffff);
      return f;
    }

  static double getDouble(void) 
    {
      return ((double)getRand() / (unsigned long)0xffffffff);
    }

  static std::vector<int> generateRandomSequence(int targetLength)
    {
      std::vector<int> temp(targetLength);
      std::vector<int> result(targetLength);

      for(int i=0;i<targetLength;i++)
	{
	  temp[i]=i;
	}

      int ub=targetLength-1;

      for(int i=0;i<targetLength;i++) 
	{
	  int next=getInteger(0,ub);
	  result[i]=temp[next];
	  temp[next]=temp[ub];
	  ub--;
	}

      return result;
    }
};

#endif
