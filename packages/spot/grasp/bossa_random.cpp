// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include "bossa_random.h"

int BossaRandom::mti = N+1;
unsigned long BossaRandom::mt[N];

void BossaRandom::sgenrand(unsigned long seed) {
  /* setting initial seeds to mt[N] using         */
  /* the generator Line 25 of Table 1 in          */
  /* [KNUTH 1981, The Art of Computer Programming */
  /*    Vol. 2 (2nd Ed.), pp102]                  */
  mt[0]= seed & 0xffffffff;
  for (mti=1; mti<N; mti++)
    mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}



unsigned long BossaRandom::genrand() {
  unsigned long y;

  static unsigned long mag01[2]={0x0, MATRIX_A};
  /* mag01[x] = x * MATRIX_A  for x=0,1 */

  if (mti >= N) { /* generate N words at one time */
    int kk;

    if (mti == N+1)   /* if sgenrand() has not been called, */
      sgenrand(4357); /* a default initial seed is used   */

    for (kk=0;kk<N-M;kk++) {
      y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
      mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    for (;kk<N-1;kk++) {
      y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
      mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
    mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

    mti = 0;
  }
  
  y = mt[mti++];
  y ^= TEMPERING_SHIFT_U(y);
  y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
  y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
  y ^= TEMPERING_SHIFT_L(y);

  return y; 
}

