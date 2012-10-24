#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"

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
   std::vector<float> mUvOptionsVec;

public:

   AlembicPolyMesh(exoNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicPolyMesh();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};



#endif
