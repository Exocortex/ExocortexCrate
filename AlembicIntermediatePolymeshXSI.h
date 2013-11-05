#ifndef _ALEMBIC_INTERMEDIATE_POLYMESH_XSI_H_
#define _ALEMBIC_INTERMEDIATE_POLYMESH_XSI_H_

#include "CommonIntermediatePolyMesh.h"



class IntermediatePolyMeshXSI : public CommonIntermediatePolyMesh
{

public:

   void Save(XSI::Primitive prim, double time, std::map<XSI::CString,XSI::CValue>& options, int mNumSamples);

   void clearNonConstProperties();
};




#endif