#ifndef _LZMA_DEC_H_
#define _LZMA_DEC_H_

#include "LzmaDec.h"
#include "LzmaLib.h"

int readAndDecompressLZMA(FILE *stream, PERD db);
int lzma_decompress_float_2d(float **f, int nrows, int ncols, FILE *stream);
int lzma_decompress_float_1d(float *f, int num, FILE *stream);
void lzma_dec_close();

#endif
