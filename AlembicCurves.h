#ifndef _ALEMBIC_CURVES_H_
#define _ALEMBIC_CURVES_H_

#include "AlembicObject.h"
#include <maya/MFnNurbsCurve.h>
#include <maya/MUint64Array.h>

class AlembicCurves: public AlembicObject
{
private:
   Alembic::AbcGeom::OCurves mObject;
   Alembic::AbcGeom::OCurvesSchema mSchema;
   Alembic::AbcGeom::OCurvesSchema::Sample mSample;

   std::vector<Alembic::Abc::V3f> mPosVec;
   std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mNbVertices;

   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mVelocityProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mRadiusProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OC4fArrayProperty mColorProperty;

   std::vector<float> mRadiusVec;
   std::vector<Alembic::Abc::V2f> mUvVec;
   std::vector<Alembic::Abc::C4f> mColorVec;
   std::vector<Alembic::Abc::V3f> mVelVec;

public:
   AlembicCurves(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicCurves();

   virtual Alembic::Abc::OObject GetObject() { return mObject; }
   virtual Alembic::Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicCurvesNode : public AlembicObjectNode
{
public:
   AlembicCurvesNode() {}
   virtual ~AlembicCurvesNode();

   // override virtual methods from MPxNode
   virtual void PreDestruction();
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicCurvesNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   Alembic::AbcGeom::ICurvesSchema mSchema;

   // output attributes
   static MObject mOutGeometryAttr;

   // members
   SampleInfo mLastSampleInfo;
   MObject mCurvesData;
   MFnNurbsCurve mCurves;
};

class AlembicCurvesDeformNode : public AlembicObjectDeformNode
{
public:
   virtual ~AlembicCurvesDeformNode();
   // override virtual methods from MPxDeformerNode
   virtual void PreDestruction();
   virtual MStatus deform(MDataBlock & dataBlock, MItGeometry & iter, const MMatrix & localToWorld, unsigned int geomIndex);
   static void* creator() { return (new AlembicCurvesDeformNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   Alembic::AbcGeom::ICurvesSchema mSchema;

   // members
   SampleInfo mLastSampleInfo;
};

#endif