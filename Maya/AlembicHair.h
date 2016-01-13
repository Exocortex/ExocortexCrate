#ifndef _ALEMBIC_HAIR_H_
#define _ALEMBIC_HAIR_H_

#include <maya/MFnPfxGeometry.h>
#include <maya/MUint64Array.h>
#include "AlembicObject.h"

class AlembicHair : public AlembicObject {
 private:
  AbcG::OCurves mObject;
  AbcG::OCurvesSchema mSchema;
  AbcG::OCurvesSchema::Sample mSample;

  std::vector<Abc::V3f> mPosVec;
  std::vector<AbcA::int32_t> mNbVertices;

  Abc::OV3fArrayProperty mVelocityProperty;
  Abc::OFloatArrayProperty mRadiusProperty;
  Abc::OC4fArrayProperty mColorProperty;

  std::vector<float> mRadiusVec;
  std::vector<Abc::V2f> mUvVec;
  std::vector<Abc::C4f> mColorVec;
  std::vector<Abc::V3f> mVelVec;

 public:
  AlembicHair(SceneNodePtr eNode, AlembicWriteJob* in_Job,
              Abc::OObject oParent);
  ~AlembicHair();

  virtual Abc::OObject GetObject() { return mObject; }
  virtual Abc::OCompoundProperty GetCompound() { return mSchema; }
  virtual MStatus Save(double time);
};

/*
class AlembicHairNode : public AlembicObjectNode
{
public:
   AlembicHairNode() {}
   virtual ~AlembicHairNode();

   // override virtual methods from MPxNode
   virtual void PreDestruction();
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicHairNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   AbcG::ICurvesSchema mSchema;

   // output attributes
   static MObject mOutGeometryAttr;

   // members
   SampleInfo mLastSampleInfo;
   MObject mCurvesData;
   MFnNurbsCurve mCurves;
};

class AlembicHairDeformNode : public AlembicObjectDeformNode
{
public:
   virtual ~AlembicHairDeformNode();
   // override virtual methods from MPxDeformerNode
   virtual void PreDestruction();
   virtual MStatus deform(MDataBlock & dataBlock, MItGeometry & iter, const
MMatrix & localToWorld, unsigned int geomIndex);
   static void* creator() { return (new AlembicHairDeformNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   AbcG::ICurvesSchema mSchema;

   // members
   SampleInfo mLastSampleInfo;
};
*/

#endif