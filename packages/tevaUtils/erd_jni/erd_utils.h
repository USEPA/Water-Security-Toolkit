
#include <jni.h>

#include "erd.h"
#include "erd_data.h"

#ifndef _ERD_UTILS_H_
#define _ERD_UTILS_H_

JNIEXPORT jboolean ERD_initJavaData(JNIEnv *env, DBDataPtr *dbData, char *readerClassName);
JNIEXPORT jobject ERD_CreateNetInfoObject(JNIEnv *env, jobject reader, DBDataPtr dbData, PNetInfo pNetInfo);
JNIEXPORT jobject ERD_createSourcesArray(JNIEnv *env, DBDataPtr dbData, int nsources, PSource pSource);
JNIEXPORT jobjectArray ERD_CreateNodeInfoArray(JNIEnv *env,DBDataPtr dbData, PNetInfo pNetInfo,PNodeInfo pNodeInfo);
JNIEXPORT jobjectArray ERD_CreateLinkInfoArray(JNIEnv *env, DBDataPtr dbData, PNetInfo pNetInfo,PLinkInfo pLinkInfo);
JNIEXPORT void ERD_populateNodeInfoAndSources(JNIEnv *env, DBDataPtr dbData, jobject reader,
										 PNetInfo pNetInfo, PNodeInfo pNodeInfo,
										 PSourceData pSource, jobjectArray jNodeInfo, jobjectArray jLinkInfo);

#endif
