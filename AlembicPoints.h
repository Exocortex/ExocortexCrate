#ifndef _ALEMBIC_POINTS_H_
#define _ALEMBIC_POINTS_H_

#include "AlembicObject.h"
#include <xsi_pluginregistrar.h>

class AlembicPoints: public AlembicObject
{
private:
   AbcG::OPointsSchema mPointsSchema;
   AbcG::OPointsSchema::Sample mPointsSample;

   // instance lookups
   Abc::OStringArrayProperty mInstancenamesProperty;
   std::vector<std::string> mInstanceNames;
   std::map<unsigned long,size_t> mInstanceMap;

   // particle attributes (velocity == Pointvelocity, width == size in the schema)
   Abc::OV3fArrayProperty mScaleProperty;
   Abc::OQuatfArrayProperty mOrientationProperty;
   Abc::OQuatfArrayProperty mAngularVelocityProperty;
   Abc::OFloatArrayProperty mAgeProperty;
   Abc::OFloatArrayProperty mMassProperty;
   Abc::OUInt16ArrayProperty mShapeTypeProperty;
   Abc::OFloatArrayProperty mShapeTimeProperty;
   Abc::OUInt16ArrayProperty mShapeInstanceIDProperty;
   Abc::OC4fArrayProperty mColorProperty;

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

   AlembicPoints(exoNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicPoints();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

XSI::CStatus Register_alembic_points( XSI::PluginRegistrar& in_reg );

#endif