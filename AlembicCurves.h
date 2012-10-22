#ifndef _ALEMBIC_CURVES_H_
#define _ALEMBIC_CURVES_H_

#include "AlembicMax.h"
#include "AlembicObject.h"

class AlembicCurves: public AlembicObject
{
private:
   AbcG::OXformSchema mXformSchema;
   AbcG::OCurvesSchema mCurvesSchema;
   AbcG::XformSample mXformSample;
   AbcG::OCurvesSchema::Sample mCurvesSample;

   //Abc::OV3fArrayProperty mVelocityProperty;
   //Abc::OFloatArrayProperty mRadiusProperty;
   //Abc::OC4fArrayProperty mColorProperty;
   //Abc::OV3fArrayProperty mInTangentProperty;
   //Abc::OV3fArrayProperty mOutTangentProperty;

  // std::vector<float> mRadiusVec;
   //std::vector<Abc::V2f> mUvVec;
   //std::vector<Abc::C4f> mColorVec;
   //std::vector<Abc::V3f> mVelVec;

public:

   AlembicCurves(const SceneEntry &in_Ref, AlembicWriteJob * in_Job);
   ~AlembicCurves();

   virtual Abc::OCompoundProperty GetCompound();
   virtual bool Save(double time, bool bLastFrame);
};

#endif