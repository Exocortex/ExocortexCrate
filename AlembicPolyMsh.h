#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"

class AlembicPolyMesh: public AlembicObject
{
private:
   AbcG::OXformSchema mXformSchema;
   AbcG::OPolyMeshSchema mMeshSchema;
   AbcG::XformSample mXformSample;
   AbcG::OPolyMeshSchema::Sample mMeshSample;
   std::vector<AbcA::int32_t> mFaceCountVec;
   std::vector<AbcA::int32_t> mFaceIndicesVec;
   std::vector<Alembic::Abc::V3f> mBindPoseVec;
   std::vector<Alembic::Abc::V3f> mVelocitiesVec;
   //std::vector<std::vector<Alembic::Abc::V2f> > mUvVec;
   //std::vector<std::vector<Alembic::Abc::uint32_t> > mUvIndexVec;
   std::vector<AbcG::OV2fGeomParam> mUvParams;
   std::vector<std::vector<AbcA::int32_t> > mFaceSetsVec;
   Abc::OV3fArrayProperty mBindPoseProperty;
   Abc::OFloatArrayProperty mUvOptionsProperty;
   Abc::OInt32Property mFaceVaryingInterpolateBoundaryProperty;

   std::vector<float> mUvOptionsVec;

public:

   AlembicPolyMesh(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicPolyMesh();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};



#endif
