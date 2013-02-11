#ifndef __ALEMBIC_NURBS_UTILITIES_H
#define __ALEMBIC_NURBS_UTILITIES_H

#include "resource.h"
#include "AlembicDefinitions.h"
#include <surf_api.h> 
// Alembic Functions


typedef struct _alembic_NURBSload_options
{
public:
    _alembic_NURBSload_options(): pIObj(NULL), pObject(NULL), dTicks(0), nDataFillFlags(0)
    {}

    AbcG::IObject  *pIObj;

	Object						*pObject;
    TimeValue                   dTicks;
    Interval                    validInterval;
    AlembicDataFillFlags        nDataFillFlags;
} alembic_NURBSload_options;

//bool LoadNurbs(NURBSSet& nset, Abc::P3fArraySamplePtr pCurvePos, Abc::Int32ArraySamplePtr pCurveNbVertices, Abc::FloatArraySamplePtr pKnotVec, TimeValue time );
void AlembicImport_LoadNURBS_Internal(alembic_NURBSload_options &options);
int AlembicImport_NURBS(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);
bool isAlembicNurbsCurveTopoDynamic( AbcG::IObject *pIObj );

#endif 