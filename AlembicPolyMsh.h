#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"

class AlembicPolyMesh: public AlembicObject
{
private:
   Alembic::AbcGeom::OPolyMeshSchema mMeshSchema;
   Alembic::AbcGeom::OPolyMeshSchema::Sample mMeshSample;
   std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceCountVec;
   std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceIndicesVec;
   std::vector<Alembic::Abc::V3f> mBindPoseVec;
   std::vector<Alembic::Abc::V3f> mVelocitiesVec;
   //std::vector<std::vector<Alembic::Abc::V2f> > mUvVec;
   //std::vector<std::vector<Alembic::Abc::uint32_t> > mUvIndexVec;
   std::vector<Alembic::AbcGeom::OV2fGeomParam> mUvParams;
   std::vector<std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> > mFaceSetsVec;
   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mBindPoseProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mUvOptionsProperty;
   std::vector<float> mUvOptionsVec;

public:

   AlembicPolyMesh(exoNodePtr eNode, AlembicWriteJob * in_Job, Alembic::Abc::OObject oParent);
   ~AlembicPolyMesh();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};



#endif
