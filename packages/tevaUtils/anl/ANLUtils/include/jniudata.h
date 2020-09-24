#ifndef _JNIUDATA_H_
#define _JNIUDATA_H_

#include <jni.h>

typedef struct JavaClassData {
	jclass systemClass;
	jmethodID systemGC;
} JavaClassData, *JavaClassDataPtr;


#endif /* #ifndef _ANLUTILS_H_ */
