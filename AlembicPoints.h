#ifndef _ALEMBIC_POINTS_H_
#define _ALEMBIC_POINTS_H_

#include "AlembicObject.h"
#include <xsi_pluginregistrar.h>

class AlembicPoints: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::OPointsSchema mPointsSchema;
   Alembic::AbcGeom::XformSample mXformSample;
   Alembic::AbcGeom::OPointsSchema::Sample mPointsSample;

   // instance lookups
   Alembic::Abc::ALEMBIC_VERSION_NS::OStringArrayProperty mInstancenamesProperty;
   std::vector<std::string> mInstanceNames;
   std::map<unsigned long,size_t> mInstanceMap;

   // particle attributes (velocity == Pointvelocity, width == size in the schema)
   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mScaleProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OQuatfArrayProperty mOrientationProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OQuatfArrayProperty mAngularVelocityProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mAgeProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mMassProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OUInt16ArrayProperty mShapeTypeProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mShapeTimeProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OUInt16ArrayProperty mShapeInstanceIDProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OC4fArrayProperty mColorProperty;

public:

   enum ShapeType{
      ShapeType_Point,
      ShapeType_Box,
      ShapeType_Sphere,
      ShapeType_Cylinder,
      ShapeType_Cone,
      ShapeType_Disc,
      ShapeType_Rectangle,
      ShapeType_Instance,
      ShapeType_NbElements
   };

   AlembicPoints(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job, Alembic::Abc::OObject oParent);
   ~AlembicPoints();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

XSI::CStatus Register_alembic_points( XSI::PluginRegistrar& in_reg );

#endif