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
#include <jni.h>
#include "EPANETModel.h"
#include "jniUtils.h"
#include "toolkit.h"
#include <stdlib.h>
#include <string.h>

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enEpanet
  (JNIEnv *env, jobject model, jstring inputFile, jstring reportFile, jstring outputFile, jobject progressMethod) 
{
	Java_enEpanet(env,model,inputFile,reportFile,outputFile,progressMethod);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enOpen
  (JNIEnv *env, jobject model, jstring inputFile, jstring reportFile, jstring outputFile)
{
  Java_enOpen(env,model,inputFile,reportFile,outputFile);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSaveInpfile
  (JNIEnv *env, jobject model, jstring inputFileName)
{
  Java_enSaveInpfile(env,model,inputFileName);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enClose
  (JNIEnv *env, jobject model)
{
  Java_enClose(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSolveH
  (JNIEnv *env, jobject model)
{
  Java_enSolveH(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSaveH
  (JNIEnv *env, jobject model)
{
  Java_enSaveH(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enOpenH
  (JNIEnv *env, jobject model)
{
  Java_enOpenH(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enInitH
  (JNIEnv *env, jobject model, jint flag)
{
  Java_enInitH(env,model,flag);
}

JNIEXPORT jlong JNICALL Java_anl_epanet_model_EPANETModelInterface_enRunH
  (JNIEnv *env, jobject model)
{
  return Java_enRunH(env,model);
}

JNIEXPORT jlong JNICALL Java_anl_epanet_model_EPANETModelInterface_enNextH
  (JNIEnv *env, jobject model)
{
  return Java_enNextH(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enCloseH
  (JNIEnv *env, jobject model)
{
  Java_enCloseH(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSaveHydFile
  (JNIEnv *env, jobject model, jstring hydFileName)
{
  Java_enSaveHydFile(env,model,hydFileName);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enUseHydFile
  (JNIEnv *env, jobject model, jstring hydFileName)
{
  Java_enUseHydFile(env,model,hydFileName);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSolveQ
  (JNIEnv *env, jobject model)
{
Java_enSolveQ(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enOpenQ
  (JNIEnv *env, jobject model)
{
  Java_enOpenQ(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enInitQ
  (JNIEnv *env, jobject model, jint saveFlag)
{
  Java_enInitQ(env,model,saveFlag);
}

JNIEXPORT jlong JNICALL Java_anl_epanet_model_EPANETModelInterface_enRunQ
  (JNIEnv *env, jobject model)
{
  return Java_enRunQ(env,model);
}

JNIEXPORT jlong JNICALL Java_anl_epanet_model_EPANETModelInterface_enNextQ
  (JNIEnv *env, jobject model)
{
  return Java_enNextQ(env,model);
}

JNIEXPORT jlong JNICALL Java_anl_epanet_model_EPANETModelInterface_enStepQ
  (JNIEnv *env, jobject model)
{
  return Java_enStepQ(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enCloseQ
  (JNIEnv *env, jobject model)
{
  Java_enCloseQ(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enReport
  (JNIEnv *env, jobject model)
{
  Java_enReport(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enResetReport
  (JNIEnv *env, jobject model)
{
  Java_enResetReport(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetReport
  (JNIEnv *env, jobject model, jstring command)
{
  Java_enSetReport(env,model,command);
}

JNIEXPORT jobject JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetControl
  (JNIEnv *env, jobject model, jint controlIndex)
{
  return Java_enGetControl(env,model,controlIndex);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetCount
  (JNIEnv *env, jobject model, jint countCode)
{
  return Java_enGetCount(env,model,countCode);
}

JNIEXPORT jfloat JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetOption
  (JNIEnv *env, jobject model, jint optionCode)
{
  return Java_enGetOption(env,model,optionCode);
}

JNIEXPORT jlong JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetTimeParam
  (JNIEnv *env, jobject model, jint paramCode)
{
  return Java_enGetTimeParam(env,model,paramCode);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetFlowUnits
  (JNIEnv *env, jobject model)
{
  return Java_enGetFlowUnits(env,model);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetPatternIndex
  (JNIEnv *env, jobject model, jstring patternID)
{
  return Java_enGetPatternIndex(env,model,patternID);
}

JNIEXPORT jstring JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetPatternID
  (JNIEnv *env, jobject model, jint patternIndex)
{
  return Java_enGetPatternID(env,model,patternIndex);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetPatternLen
  (JNIEnv *env, jobject model, jint patternIndex)
{
  return Java_enGetPatternLen(env,model,patternIndex);
}

JNIEXPORT jfloat JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetPatternValue
  (JNIEnv *env, jobject model, jint patternIndex, jint period)
{
  return Java_enGetPatternValue(env,model,patternIndex,period);
}

JNIEXPORT jobject JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetQualType
  (JNIEnv *env, jobject model)
{
  return Java_enGetQualType(env,model);
}

JNIEXPORT jstring JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetError
  (JNIEnv *env, jobject model, jint errcode)
{
	return Java_enGetError(env,model,errcode);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetNodeIndex
  (JNIEnv *env, jobject model, jstring nodeID)
{
	return Java_enGetNodeIndex(env,model,nodeID);
}

JNIEXPORT jstring JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetNodeID
  (JNIEnv *env, jobject model, jint nodeIndex)
{
	return Java_enGetNodeID(env,model,nodeIndex);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetNodeType
  (JNIEnv *env, jobject model, jint nodeIndex)
{
	return Java_enGetNodeType(env,model,nodeIndex);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetNodeTypes
  (JNIEnv *env, jobject model, jintArray types, jlong buf, jint bufsize)
{
	Java_enGetNodeTypes(env,model,types,buf,bufsize);
}

JNIEXPORT jfloat JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetNodeValue
  (JNIEnv *env, jobject model, jint nodeIndex, jint paramCode)
{
	return Java_enGetNodeValue(env,model,nodeIndex,paramCode);
}
JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetNodeValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray jvals, jint startIdx, jint len, jlong buf, jint bufsize)
{
	Java_enGetNodeValues(env,model,paramCode,jvals, startIdx, len, buf,bufsize);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetLinkIndex
  (JNIEnv *env, jobject model, jstring linkID)
{
	return Java_enGetLinkIndex(env,model,linkID);
}

JNIEXPORT jstring JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetLinkID
  (JNIEnv *env, jobject model, jint linkIndex)
{
	return Java_enGetLinkID(env,model,linkIndex);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetLinkType
  (JNIEnv *env, jobject model, jint linkIndex)
{
	return Java_enGetLinkType(env,model,linkIndex);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetLinkTypes
  (JNIEnv *env, jobject model, jintArray types, jlong buf, jint bufsize)
{
	Java_enGetLinkTypes(env,model,types,buf,bufsize);
}

JNIEXPORT jobject JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetLinkNodes
  (JNIEnv *env, jobject model, jint linkIndex)
{
	return Java_enGetLinkNodes(env,model,linkIndex);
}

JNIEXPORT jfloat JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetLinkValue
  (JNIEnv *env, jobject model, jint linkIndex, jint paramCode)
{
	return Java_enGetLinkValue(env,model,linkIndex,paramCode);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetLinkValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray jvals, jint startIdx, jint len, jlong buf, jint bufsize)
{
	Java_enGetLinkValues(env,model,paramCode, jvals,startIdx,len,buf,bufsize);
}

JNIEXPORT jint JNICALL Java_anl_epanet_model_EPANETModelInterface_enGetVersion
  (JNIEnv *env, jobject model)
{
	return Java_enGetVersion(env,model);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetControl
  (JNIEnv *env, jobject model, jint controlIndex, jobject enControl)
{
	Java_enSetControl(env,model,controlIndex,enControl);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetNodeValue
  (JNIEnv *env, jobject model, jint nodeIndex, jint paramCode, jfloat value)
{
	Java_enSetNodeValue(env,model,nodeIndex,paramCode,value);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetNodeValues
  (JNIEnv *env, jobject model, jint paramCode, jfloatArray values, jlong buf, jint bufsize)
{
	Java_enSetNodeValues(env,model,paramCode,values,buf,bufsize);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetConstantNodeValues
  (JNIEnv *env, jobject model, jint paramCode, jfloat constantVal, jint num)
{
	Java_enSetConstantNodeValues(env,model,paramCode,constantVal,num);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetLinkValue
  (JNIEnv *env, jobject model, jint linkIndex, jint paramCode, jfloat value)
{
	Java_enSetLinkValue(env,model,linkIndex,paramCode,value);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetPattern
  (JNIEnv *env, jobject model, jint patternIndex, jfloatArray factors)
{
	Java_enSetPattern(env,model,patternIndex,factors);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetPatternValue
  (JNIEnv *env, jobject model, jint patternIndex, jint period, jfloat value)
{
	Java_enSetPatternValue(env,model,patternIndex,period,value);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetTimeParam
  (JNIEnv *env, jobject model, jint paramCode, jlong value)
{
	Java_enSetTimeParam(env,model,paramCode,value);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetOption
  (JNIEnv *env, jobject model, jint optionCode, jfloat value)
{
	Java_enSetOption(env,model,optionCode,value);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetStatusReport
  (JNIEnv *env, jobject model, jint statusLevel)
{
	Java_enSetStatusReport(env,model,statusLevel);
}

JNIEXPORT void JNICALL Java_anl_epanet_model_EPANETModelInterface_enSetQualType
  (JNIEnv *env, jobject model, jint qualCode, jstring chemName, jstring chemUnits, jstring traceNode)
{
	Java_enSetQualType(env,model,qualCode,chemName,chemUnits,traceNode);
}

