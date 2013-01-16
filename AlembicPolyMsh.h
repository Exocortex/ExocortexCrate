#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"

#include "AlembicCustomAttributesEx.h"

class AlembicPolyMesh: public AlembicObject
{
private:
  AbcG::OPolyMeshSchema mMeshSchema;
  AbcG::OPolyMeshSchema::Sample mMeshSample;
   std::vector<AbcA::int32_t> mFaceCountVec;
   std::vector<AbcA::int32_t> mFaceIndicesVec;
   std::vector<Abc::V3f> mBindPoseVec;
   std::vector<Abc::V3f> mVelocitiesVec;
   //std::vector<std::vector<Abc::V2f> > mUvVec;
   //std::vector<std::vector<Abc::uint32_t> > mUvIndexVec;
   std::vector<AbcG::OV2fGeomParam> mUvParams;
   std::vector<std::vector<AbcA::int32_t> > mFaceSetsVec;
   Abc::OV3fArrayProperty mBindPoseProperty;
   Abc::OFloatArrayProperty mUvOptionsProperty;
   Abc::OInt32Property mFaceVaryingInterpolateBoundaryProperty;

   std::vector<float> mUvOptionsVec;

   AlembicCustomAttributesEx customAttributes;

public:

   AlembicPolyMesh(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicPolyMesh();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

XSI::CStatus Register_alembic_polyMesh( XSI::PluginRegistrar& in_reg );

#endif
