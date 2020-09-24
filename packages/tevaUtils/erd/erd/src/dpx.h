#ifndef _DPX_H_
#define _DPX_H_

// --- define WINDOWS

#undef WINDOWS
#ifdef _WIN32
  #define WINDOWS
#endif
#ifdef __WIN32__
  #define WINDOWS
#endif

/* Filename tokenizer */
#ifdef WINDOWS
#define FNTOK '\\'
#else
#define FNTOK '/'
#endif

int readDPXIndexData(PERD db, PQualitySim data, FILE *stream);
int writeDPXIndexData(void *data, FILE *stream);
void freeDPXIndexData(void **data);

#endif
