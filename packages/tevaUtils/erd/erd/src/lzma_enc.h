#ifndef _LZMA_ENC_H_
#define _LZMA_ENC_H_

#include "erdinternal.h"
#include "LzmaEnc.h"
#include "LzmaLib.h"

typedef struct LZMAOutStream
{
  ISeqOutStream SeqOutStream;
  unsigned char *buf;
  unsigned size;
  unsigned curPos;
} LZMAOutStream, *PLZMAOutStream;

enum ISStates {
	writingInit,
	writingID,
	writingRecord
};

typedef struct Float2DInStream
{
  ISeqInStream SeqInStream;
  float **f;
  int r;
  unsigned char *p;
  int remain;
  int nrows;
  int ncols;
#ifdef LZMA_TESTING
  int count;
#endif
} Float2DInStream, *PFloat2DInStream;

typedef struct QualInStream
{
  ISeqInStream SeqInStream;
  float **conc;
  int *nz;
  int curN;
  unsigned char *p;
  int curID;
  int remain;
  enum ISStates state;
  int nelems;
  int nsteps;
#ifdef LZMA_TESTING
  int count;
#endif
} QualInStream, *PQualInStream;

int lzma_compress_qual(float **conc, int *nz, int nelems, int nsteps, FILE *stream);
int lzma_compress_float_2d(float **f, int nrows, int ncols, FILE *stream);
int lzma_compress_float_1d(float *f, int num, FILE *stream);
void lzma_enc_close();
int lzma_store_qual(PERD db, FILE *stream);
void setProps(CLzmaEncProps *);

#endif
