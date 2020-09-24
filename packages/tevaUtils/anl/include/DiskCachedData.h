#ifndef _DISK_CACHED_DATA_H_
#define _DISK_CACHED_DATA_H_

#include <stdio.h>
#include <jni.h>
#include "erd.h"

#ifdef _WIN32
  #ifdef __cplusplus
  #define LIBEXPORT(type) extern "C" __declspec(dllexport) type __stdcall
  #else
  #define LIBEXPORT(type) __declspec(dllexport) type __stdcall
  #endif
#else
#define LIBEXPORT(type) type
#endif


/* Global Data Types */
typedef struct CachedData {
    float val;
	char *simID;
	char *injDef;
	char *injNodeType;
	__file_pos_t fileOffset;
} CachedData, *CachedDataPtr;

typedef struct DiskCachedData {
	FILE *fp;
	char fn[256];
	int num;
	int size;
	CachedDataPtr *data;
}DiskCachedData, *DiskCachedDataPtr;

LIBEXPORT(void) freeDiskCachedData(JNIEnv *env,DiskCachedDataPtr *dcd,int numScenarios);
LIBEXPORT(DiskCachedDataPtr) initDiskCachedData(JNIEnv * env,char *fn, int nelems,int numScenarios);
LIBEXPORT(void) cacheData(JNIEnv *env,DiskCachedDataPtr dcd, float val, char *simID, char *injDef, void *data, int size, char *type);
LIBEXPORT(void) getCachedData(DiskCachedDataPtr dcd, int i,void *data, int size);
LIBEXPORT(void) SortCachedData(DiskCachedDataPtr dcd);

#endif
