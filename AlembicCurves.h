#ifndef _ALEMBIC_CURVES_H_
#define _ALEMBIC_CURVES_H_

#include "AlembicObject.h"
#include <xsi_pluginregistrar.h>

class AlembicCurves: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::OCurvesSchema mCurvesSchema;
   Alembic::AbcGeom::XformSample mXformSample;
   Alembic::AbcGeom::OCurvesSchema::Sample mCurvesSample;
   std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mNbVertices;

   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mVelocityProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mRadiusProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OC4fArrayProperty mColorProperty;
   std::vector<float> mRadiusVec;
   std::vector<Alembic::Abc::V2f> mUvVec;
   std::vector<Alembic::Abc::C4f> mColorVec;
   std::vector<Alembic::Abc::V3f> mVelVec;

public:

   AlembicCurves(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicCurves();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

XSI::CStatus Register_alembic_curves( XSI::PluginRegistrar& in_reg );

#endif