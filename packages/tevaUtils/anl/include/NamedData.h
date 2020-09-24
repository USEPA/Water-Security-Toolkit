/*
 * Copyright © 2008 UChicago Argonne, LLC
 * NOTICE: This computer software, TEVA-SPOT, was prepared for UChicago Argonne, LLC
 * as the operator of Argonne National Laboratory under Contract No. DE-AC02-06CH11357
 * with the Department of Energy (DOE). All rights in the computer software are reserved
 * by DOE on behalf of the United States Government and the Contractor as provided in
 * the Contract.
 * NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
 *
 * This software is distributed under the BSD License.
 */
#ifndef _NAMED_DATA_H_
#define _NAMED_DATA_H_

#include <jni.h>

#ifdef _WINDOWS
  #ifdef __cplusplus
  #define EXPORT extern "C" __declspec(dllexport)
  #else
  #define EXPORT __declspec(dllexport)
  #endif
#else
#define EXPORT
#endif

typedef struct NamedDataRef {
	JNIEnv *env;
	jobject obj;
} NamedDataRef, *NamedDataRefPtr;

#define NUM_TYPES            15

#define INT_TYPE              0
#define INT_ARRAY_TYPE        1
#define INT_ARRAY_2D_TYPE     2

#define FLOAT_TYPE            3
#define FLOAT_ARRAY_TYPE      4
#define FLOAT_ARRAY_2D_TYPE   5

#define DOUBLE_TYPE           6
#define DOUBLE_ARRAY_TYPE     7
#define DOUBLE_ARRAY_2D_TYPE  8

#define BYTE_TYPE             9
#define BYTE_ARRAY_TYPE      10
#define BYTE_ARRAY_2D_TYPE   11

#define STRING_TYPE          12
#define STRING_ARRAY_TYPE    13

#define NAMED_DATA_TYPE      14

#define ND_SUCCESS 1
#define ND_FAILURE 0


EXPORT int addVariable(NamedDataRef *dataObject, char *name, int type);
EXPORT int hasData(NamedDataRef *dataObject, char *name, int dataType, int *bval);
EXPORT void nd_free(void *ptr);
EXPORT void nd_freeStringArray(char **ptr, int len);

EXPORT int setInt(NamedDataRef *dataObject, char *name, int val);
EXPORT int setIntArray(NamedDataRef *dataObject, char *name, int *val, int length);
EXPORT int set2DIntArray(NamedDataRef *dataObject, char *name, int **val, int length1, int length2);
//EXPORT int setIntArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, int *val, int length);

EXPORT int setByte(NamedDataRef *dataObject, char *name, char val);
EXPORT int setByteArray(NamedDataRef *dataObject, char *name, char *val, int length);
//int setByteArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, char *val, int length);

EXPORT int setFloat(NamedDataRef *dataObject, char *name, float val);
EXPORT int setFloatArray(NamedDataRef *dataObject, char *name, float *val, int length);
EXPORT int set2DFloatArray(NamedDataRef *dataObject, char *name, float **val, int length1, int length2);
//EXPORT int setFloatArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, float *val, int length);

EXPORT int setDouble(NamedDataRef *dataObject, char *name, double val);
EXPORT int setDoubleArray(NamedDataRef *dataObject, char *name, double *val, int length);
EXPORT int set2DDoubleArray(NamedDataRef *dataObject, char *name, double **val, int length1, int length2);
//EXPORT int setDoubleArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, double *val, int length);

EXPORT int setString(NamedDataRef *dataObject, char *name, char *val);
EXPORT int setStringArray(NamedDataRef *dataObject, char *name, char **val, int length);

EXPORT int setNamedData(NamedDataRef *dataObject, char *name, NamedDataRef *val);

EXPORT int getInt(NamedDataRef *dataObject, char *name, int *val);
EXPORT int getIntArray(NamedDataRef *dataObject, char *name, int **val);
EXPORT int getIntArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, int **val);

EXPORT int getByte(NamedDataRef *dataObject, char *name, char *val);
EXPORT int getByteArray(NamedDataRef *dataObject, char *name, char **val);
EXPORT int getByteArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, char **val);

EXPORT int getFloat(NamedDataRef *dataObject, char *name, float *val);
EXPORT int getFloatArray(NamedDataRef *dataObject, char *name, float **val);
EXPORT int getFloatArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, float **val);

EXPORT int getDouble(NamedDataRef *dataObject, char *name, double *val);
EXPORT int getDoubleArray(NamedDataRef *dataObject, char *name, double ** val);
EXPORT int getDoubleArrayAtIndex(NamedDataRef *dataObject, char *name, int idx, double **val);

EXPORT int getString(NamedDataRef *dataObject, char *name, char **val);
EXPORT int getStringArray(NamedDataRef *dataObject, char *name, char ***val);

EXPORT int getNamedData(NamedDataRef *dataObject, char *name, NamedDataRef **val);

#endif

