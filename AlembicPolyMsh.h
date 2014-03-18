#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"
#include "AlembicCustomAttributesEx.h"
#include "AlembicIntermediatePolymeshXSI.h"

class AlembicPolyMesh: public AlembicObject
{
private:
  AbcG::OPolyMeshSchema mMeshSchema;
  AbcG::OPolyMeshSchema::Sample mMeshSample;

   std::vector<AbcG::OV2fGeomParam> mUvParams;
   Abc::OV3fArrayProperty mBindPoseProperty;
   Abc::OFloatArrayProperty mUvOptionsProperty;
   Abc::OInt32Property mFaceVaryingInterpolateBoundaryProperty;

   AlembicCustomAttributesEx customAttributes;

public:

   AlembicPolyMesh(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicPolyMesh();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

XSI::CStatus Register_alembic_polyMesh( XSI::PluginRegistrar& in_reg );

#endif
