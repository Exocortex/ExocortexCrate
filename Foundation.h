#ifndef _FOUNDATION_H_
#define _FOUNDATION_H_

#include <boost/smart_ptr.hpp>
#include <boost/format.hpp>
#include <boost/variant.hpp>

#include <utility>
#include <limits>

#include <set>
#include <vector>
#include <map>
#include <list>

#include <stdexcept>
#include <exception>

#include <string>

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <map>

#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>

#ifndef _BOOL
  #define _BOOL
#endif

#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MFileObject.h>
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MTime.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MSelectionList.h>
#include <maya/MVector.h>
#include <maya/MQuaternion.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MPxNode.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MPxEmitterNode.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MDataHandle.h>
#include <maya/MDGContext.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MAngle.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MIntArray.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFloatArray.h>
#include <maya/MItGeometry.h>

#include "Utility.h"
#include "AlembicArchiveStorage.h"

typedef std::map<std::string,std::string> stringMap;
typedef std::map<std::string,std::string>::iterator stringMapIt;
typedef std::pair<std::string,std::string> stringPair;

#ifndef LONG
	typedef long LONG;
#endif

#ifndef ULONG
	typedef unsigned long ULONG;
#endif

#endif  // _FOUNDATION_H_
