#ifndef _ALEMBIC_POINTS_H_
#define _ALEMBIC_POINTS_H_

#include "AlembicObject.h"
#include <maya/MFnParticleSystem.h>

class AlembicPoints: public AlembicObject
{
private:
   Alembic::AbcGeom::OPoints mObject;
   Alembic::AbcGeom::OPointsSchema mSchema;
   Alembic::AbcGeom::OPointsSchema::Sample mSample;

   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mAgeProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mMassProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OC4fArrayProperty mColorProperty;

   /*
   // instance lookups
   Alembic::Abc::ALEMBIC_VERSION_NS::OStringArrayProperty mInstancenamesProperty;
   std::vector<std::string> mInstanceNames;
   std::map<unsigned long,size_t> mInstanceMap;

   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mScaleProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OQuatfArrayProperty mOrientationProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OQuatfArrayProperty mAngularVelocityProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OUInt16ArrayProperty mShapeTypeProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mShapeTimeProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OUInt16ArrayProperty mShapeInstanceIDProperty;
   */

public:
   AlembicPoints(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicPoints();

   virtual Alembic::Abc::OObject GetObject() { return mObject; }
   virtual Alembic::Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicPointsNode : public AlembicObjectEmitterNode
{
public:
  AlembicPointsNode(): mInstancerName("") {}
   virtual ~AlembicPointsNode();

   // override virtual methods from MPxNode
   virtual void PreDestruction();
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicPointsNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   MString mInstancerName;
   Alembic::AbcGeom::IPointsSchema mSchema;
   Alembic::AbcGeom::IPoints obj;

   void instanceInitialize(Alembic::AbcGeom::IPoints obj, MString particleShapeName);

   // members
   SampleInfo mLastSampleInfo;
};

#endif