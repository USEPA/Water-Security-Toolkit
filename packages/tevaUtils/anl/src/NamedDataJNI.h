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
#ifndef NAMED_DATA_JNI_H
#define NAMED_DATA_JNI_H

#include <jni.h>

typedef struct NamedDataData {
	jclass namedDataClass;
	jclass stringClass;

	jclass intArrayClass;
	jclass floatArrayClass;
	jclass doubleArrayClass;

	// miscellaneous methods
	jmethodID ndAddVariableMethod;
	jmethodID ndHasDataMethod;

	// get methods
	jmethodID ndGetIntMethod;
	jmethodID ndGetIntArrayMethod;
	jmethodID ndGetIntArrayAtIndexMethod;

	jmethodID ndGetByteMethod;
	jmethodID ndGetByteArrayMethod;
	jmethodID ndGetByteArrayAtIndexMethod;

	jmethodID ndGetFloatMethod;
	jmethodID ndGetFloatArrayMethod;
	jmethodID ndGetFloatArrayAtIndexMethod;

	jmethodID ndGetDoubleMethod;
	jmethodID ndGetDoubleArrayMethod;
	jmethodID ndGetDoubleArrayAtIndexMethod;

	jmethodID ndGetStringMethod;
	jmethodID ndGetStringArrayMethod;

	jmethodID ndGetNamedDataMethod;

	// set methods
	jmethodID ndSetIntMethod;
	jmethodID ndSetIntArrayMethod;
	jmethodID ndSetIntArray2DMethod;
	jmethodID ndSetIntArrayAtIndexMethod;

	jmethodID ndSetByteMethod;
	jmethodID ndSetByteArrayMethod;
	jmethodID ndSetByteArrayAtIndexMethod;

	jmethodID ndSetFloatMethod;
	jmethodID ndSetFloatArrayMethod;
	jmethodID ndSetFloatArray2DMethod;
	jmethodID ndSetFloatArrayAtIndexMethod;

	jmethodID ndSetDoubleMethod;
	jmethodID ndSetDoubleArrayMethod;
	jmethodID ndSetDoubleArray2DMethod;
	jmethodID ndSetDoubleArrayAtIndexMethod;

	jmethodID ndSetStringMethod;
	jmethodID ndSetStringArrayMethod;

	jmethodID ndSetNamedDataMethod;

} NamedData, *NamedDataPtr;

#endif

