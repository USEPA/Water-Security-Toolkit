#ifndef _RLE_ENC_H_
#define _RLE_ENC_H_

#include "erdinternal.h"

int compressAllAndStoreRLE(PERD db, FILE *stream);
int compressAndStoreRLE(float *dtc, int nsteps, FILE *stream);

#endif
