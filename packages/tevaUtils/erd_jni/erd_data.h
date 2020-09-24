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

#ifndef _ERD_DATA_H_
#define _ERD_DATA_H_

typedef struct DBData {
	jclass readerClass;
	jclass netInfoClass;
	jclass nodeInfoClass;
	jclass linkInfoClass;
	jclass sourceClass;
	jclass speciesClass;
	jclass dataIndexClass;
	jclass compressedDataClass;

	jclass stringClass;
	jclass floatArrayClass;

	jmethodID readerCreateSpeciesMethod;
	jmethodID readerSetCompressionLevel;
	jmethodID netInfoConstructor;
	jmethodID nodeInfoConstructor;
	jmethodID linkInfoConstructor;
	jmethodID sourceConstructor;
	jmethodID dataIndexConstructor;
	jmethodID compressedDataConstructor;

	jfieldID readerSources;

	jfieldID nodeInfoQ;
	jfieldID nodeInfoC;
	jfieldID nodeInfoNZ; // one for each species

	jfieldID linkInfoVelocity;
	jfieldID linkInfoFlow;
    jfieldID linkInfoVX;
    jfieldID linkInfoVY;
	jfieldID linkInfoC;
	jfieldID linkInfoNZ; // one for each species
} DBData, *DBDataPtr;


#endif

