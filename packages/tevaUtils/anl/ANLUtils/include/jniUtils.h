#ifndef _ANLUTILS_H_
#define _ANLUTILS_H_

#include <jni.h>
#include <stdarg.h>
#ifdef WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef _WIN32
# if defined(_WIN64)
#  define is64Bit
#  undef is32Bit
#  define PTRCAST(p) (p)
#  define PTRTOJLONG(p) ((jlong)p)
# else
#  define is32Bit
#  undef is64Bit
#  define PTRCAST(p) (p)
#  define PTRTOJLONG(p) ((jlong)p)
# endif
#else
/* both mac and linux define __LP64 for 64-bit */
# if defined(__LP64__)
#  define is64Bit
#  undef is32Bit
#  define PTRCAST(p) (p)
#  define PTRTOJLONG(p) ((jlong)p)
# else
#  define is32Bit
#  undef is64Bit
#  define PTRCAST(p) (p)(int)
#  define PTRTOJLONG(p) ((jlong)(int)p)
#endif
#endif


/* create a new instance of the specified class using the zero argument constructor */
#define ANL_UTIL_NewObject(env,cls)	ANL_UTIL_NewObjectV(env, cls,"()V")
/* create a new instance of the Boolean class */
#define ANL_UTIL_NewBoolean(env, value) ANL_UTIL_NewObjectV(env, (*env)->FindClass(env,"java/lang/Boolean"),"(Z)V", value)
/* create a new instance of the Integer class */
#define ANL_UTIL_NewInteger(env, value) ANL_UTIL_NewObjectV(env, (*env)->FindClass(env,"java/lang/Integer"),"(I)V", value)
/* create a new instance of the Float class */
#define ANL_UTIL_NewFloat(env, value) ANL_UTIL_NewObjectV(env, (*env)->FindClass(env,"java/lang/Float"),"(F)V", value)
/* create a new instance of the Double class */
#define ANL_UTIL_NewDouble(env, value) ANL_UTIL_NewObjectV(env, (*env)->FindClass(env,"java/lang/Double"),"(D)V", value)

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jmethodID ANL_UTIL_getMethodID(JNIEnv *env, jobject obj,const char *name,const char *sig);
JNIEXPORT jboolean ANL_UTIL_getBooleanInstVar(JNIEnv *env, jobject obj,const char *name);
JNIEXPORT jint ANL_UTIL_getIntegerInstVar(JNIEnv *env, jobject obj,const char *name);
JNIEXPORT jlong ANL_UTIL_getLongInstVar(JNIEnv *env, jobject obj,const char *name);
JNIEXPORT void ANL_UTIL_setBooleanInstVar(JNIEnv *env, jobject obj,const char *name, jboolean value);
JNIEXPORT void ANL_UTIL_setIntegerInstVar(JNIEnv *env, jobject obj,const char *name, jint value);
JNIEXPORT void ANL_UTIL_setLongInstVar(JNIEnv *env, jobject obj,const char *name, jlong value);
JNIEXPORT jobject ANL_UTIL_getObjectInstVar(JNIEnv *env, jobject obj,const char *name, const char *sig);
JNIEXPORT jobject ANL_UTIL_getObjectStaticVar(JNIEnv *env, jobject obj,const char *name, const char *sig);
JNIEXPORT jfieldID ANL_UTIL_getFieldID(JNIEnv *env, jobject obj, const char *name, const char *sig);
JNIEXPORT void ANL_UTIL_setObjectInstVar(JNIEnv *env, jobject obj, const char *name, jobject value, const char *sig);
JNIEXPORT void jvprintf(JNIEnv *env, char *buffer, int bufferSize, const char *format, va_list ap);
JNIEXPORT void jprintf(JNIEnv *env, char *buffer, int bufferSize, const char *format, ...);
JNIEXPORT void jevprintf(JNIEnv *env, char *buffer, int bufferSize, const char *format, va_list ap);
JNIEXPORT void jeprintf(JNIEnv *env, char *buffer, int bufferSize, const char *format, ...);
JNIEXPORT void ANL_UTIL_free(void *p) ;
JNIEXPORT char *ANL_UTIL_javaStringToC(JNIEnv *env,jstring jstr);
JNIEXPORT char *ANL_UTIL_getClassNameToC(JNIEnv *env, jclass cls);
JNIEXPORT char *ANL_UTIL_getObjectsClassNameToC(JNIEnv *env, jobject obj);
JNIEXPORT jstring ANL_UTIL_getClassName(JNIEnv *env, jclass cls);
JNIEXPORT jstring ANL_UTIL_getObjectsClassName(JNIEnv *env, jobject obj);
JNIEXPORT char *ANL_UTIL_toString(JNIEnv *env,jobject obj);
JNIEXPORT int ANL_UTIL_CheckForException(JNIEnv *env,const char *format, ...);
JNIEXPORT int ANL_UTIL_CheckForExceptionNoStackTrace(JNIEnv *env,const char *format, ...);
JNIEXPORT jobject ANL_UTIL_NewObjectV(JNIEnv *env, jclass cls, const char *sig, ...);
JNIEXPORT void setDebugTimeField(JNIEnv *env, jobject obj, const char *name, long long t0);
JNIEXPORT long long getCurrentTimeMillis();
JNIEXPORT JNIEnv *ANL_UTIL_getJavaEnv();
JNIEXPORT void ANL_UTIL_RequestGC(JNIEnv *env);
#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ANLUTILS_H_ */
