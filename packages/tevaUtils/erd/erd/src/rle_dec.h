#ifndef _RLE_DEC_H_
#define _RLE_DEC_H_

#include "erdinternal.h"
#include <stdio.h>

int readAndDecompressRLE(FILE *stream, PERD db);

#endif