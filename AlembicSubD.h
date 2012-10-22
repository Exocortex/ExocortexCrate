#ifndef _ALEMBIC_SUBD_H_
#define _ALEMBIC_SUBD_H_

#include "AlembicObject.h"

class AlembicSubD: public AlembicObject
{
private:
  AbcG::OXformSchema mXformSchema;
  AbcG::OSubDSchema mSubDSchema;
  AbcG::XformSample mXformSample;
  AbcG::OSubDSchema::Sample mSubDSample;
   std::vector<AbcA::int32_t> mFaceCountVec;
   std::vector<AbcA::int32_t> mFaceIndicesVec;
   std::vector<std::vector<AbcA::int32_t> > mFaceSetsVec;
   std::vector<std::vector<Abc::V2f> > mUvVec;
   std::vector<std::vector<Abc::uint32_t> > mUvIndexVec;
   std::vector<AbcG::OV2fGeomParam> mUvParams;
   Abc::OV3fArrayProperty mBindPoseProperty;
   std::vector<Abc::V3f> mBindPoseVec;
   std::vector<Abc::V3f> mVelocitiesVec;
   Abc::OFloatArrayProperty mUvOptionsProperty;
   std::vector<float> mUvOptionsVec;

public:

   AlembicSubD(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicSubD();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif
