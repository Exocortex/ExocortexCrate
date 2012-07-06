#ifndef _ALEMBIC_SUBD_H_
#define _ALEMBIC_SUBD_H_

#include "AlembicObject.h"

class AlembicSubD: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::OSubDSchema mSubDSchema;
   Alembic::AbcGeom::XformSample mXformSample;
   Alembic::AbcGeom::OSubDSchema::Sample mSubDSample;
   std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceCountVec;
   std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceIndicesVec;
   std::vector<std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> > mFaceSetsVec;
   std::vector<std::vector<Alembic::Abc::V2f> > mUvVec;
   std::vector<std::vector<Alembic::Abc::uint32_t> > mUvIndexVec;
   std::vector<Alembic::AbcGeom::OV2fGeomParam> mUvParams;
   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mBindPoseProperty;
   std::vector<Alembic::Abc::V3f> mBindPoseVec;
   std::vector<Alembic::Abc::V3f> mVelocitiesVec;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mUvOptionsProperty;
   std::vector<float> mUvOptionsVec;

public:

   AlembicSubD(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicSubD();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif
