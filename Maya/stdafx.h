#ifndef _FOUNDATION_H_
#define _FOUNDATION_H_

#include "CommonAlembic.h"

#ifndef _BOOL
#define _BOOL
#endif

#include <maya/MAngle.h>
#include <maya/MAnimControl.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MCommonRenderSettingsData.h>
#include <maya/MDGContext.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataHandle.h>
#include <maya/MFileIO.h>
#include <maya/MFileObject.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnInstancer.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNurbsCurveData.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringData.h>
#include <maya/MFnSubdData.h>
#include <maya/MFnSubdNames.h>
#include <maya/MFnTransform.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItGeometry.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItSubdVertex.h>
#include <maya/MMatrix.h>
#include <maya/MMatrixArray.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MProgressWindow.h>
#include <maya/MPxCommand.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MPxEmitterNode.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MPxNode.h>
#include <maya/MQuaternion.h>
#include <maya/MRenderLine.h>
#include <maya/MRenderLineArray.h>
#include <maya/MRenderUtil.h>
#include <maya/MSceneMessage.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
#include <maya/MTime.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MVectorArray.h>

#include "AlembicArchiveStorage.h"
#include "Utility.h"

#ifndef LONG
typedef long LONG;
#endif

#ifndef ULONG
typedef unsigned long ULONG;
#endif

#endif  // _FOUNDATION_H_
