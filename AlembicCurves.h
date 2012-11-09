#ifndef _ALEMBIC_CURVES_H_
#define _ALEMBIC_CURVES_H_

#include "AlembicObject.h"

class AlembicCurves: public AlembicObject
{
private:
  AbcG::OCurvesSchema mCurvesSchema;
  AbcG::OCurvesSchema::Sample mCurvesSample;
   std::vector<AbcA::int32_t> mNbVertices;

   Abc::OV3fArrayProperty mVelocityProperty;
   Abc::OFloatArrayProperty mRadiusProperty;
   Abc::OC4fArrayProperty mColorProperty;
   Abc::OInt32ArrayProperty mFaceIndexProperty;
   Abc::OInt32ArrayProperty mVertexIndexProperty;
   
   std::vector<float> mRadiusVec;
   std::vector<Abc::V2f> mUvVec;
   std::vector<Abc::C4f> mColorVec;
   std::vector<Abc::V3f> mVelVec;

public:

   AlembicCurves(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicCurves();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

XSI::CStatus Register_alembic_curves( XSI::PluginRegistrar& in_reg );

#endif