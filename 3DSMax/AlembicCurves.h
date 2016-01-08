#ifndef _ALEMBIC_CURVES_H_
#define _ALEMBIC_CURVES_H_

#include "AlembicObject.h"

class AlembicCurves : public AlembicObject {
 private:
  AbcG::OCurvesSchema mCurvesSchema;
  Abc::OFloatArrayProperty mKnotVectorProperty;
  Abc::OUInt16ArrayProperty mOrdersProperty;

  // Abc::OV3fArrayProperty mVelocityProperty;
  // Abc::OFloatArrayProperty mRadiusProperty;
  // Abc::OC4fArrayProperty mColorProperty;
  // Abc::OV3fArrayProperty mInTangentProperty;
  // Abc::OV3fArrayProperty mOutTangentProperty;

  // std::vector<float> mRadiusVec;
  // std::vector<Abc::V2f> mUvVec;
  // std::vector<Abc::C4f> mColorVec;
  // std::vector<Abc::V3f> mVelVec;

 public:
  AlembicCurves(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                Abc::OObject oParent);
  ~AlembicCurves();

  virtual Abc::OCompoundProperty GetCompound();
  virtual bool Save(double time, bool bLastFrame);
};

#endif