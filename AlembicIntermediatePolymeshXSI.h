#ifndef _ALEMBIC_INTERMEDIATE_POLYMESH_XSI_H_
#define _ALEMBIC_INTERMEDIATE_POLYMESH_XSI_H_

#include "CommonIntermediatePolyMesh.h"





class IntermediatePolyMeshXSI : public CommonIntermediatePolyMesh
{

public:

   virtual void Save(SceneNodePtr eNode, const Imath::M44f& transform44f, const CommonOptions& options, double time);

   void clearNonConstProperties();
};




#endif