#ifndef _ALEMBIC_POINTS_H_
#define _ALEMBIC_POINTS_H_

#include "AlembicObject.h"
#include <maya/MFnParticleSystem.h>
#include <maya/MFnInstancer.h>
#include <list>

class AlembicPoints: public AlembicObject
{
private:
   bool hasInstancer;
   MString instName;

   AbcG::OPoints mObject;
   AbcG::OPointsSchema mSchema;
   AbcG::OPointsSchema::Sample mSample;

   Abc::OFloatArrayProperty mAgeProperty;
   Abc::OFloatArrayProperty mMassProperty;
   Abc::OC4fArrayProperty mColorProperty;

   Abc::OQuatfArrayProperty mAngularVelocityProperty;
   Abc::OStringArrayProperty mInstanceNamesProperty;    // only written once, when mNumSample == 0
   Abc::OQuatfArrayProperty mOrientationProperty;
   Abc::OV3fArrayProperty mScaleProperty;
   Abc::OUInt16ArrayProperty mShapeInstanceIdProperty; 
   Abc::OFloatArrayProperty mShapeTimeProperty;
   Abc::OUInt16ArrayProperty mShapeTypeProperty;

   // instancing functions
   bool listIntanceNames(std::vector<std::string> &names);
   bool sampleInstanceProperties(std::vector<Abc::Quatf> angularVel, std::vector<Abc::Quatf> orientation, std::vector<Abc::v4::uint16_t> shapeId, std::vector<Abc::v4::uint16_t> shapeType, std::vector<Abc::float32_t> shapeTime);

public:
   AlembicPoints(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicPoints();

   virtual Abc::OObject GetObject() { return mObject; }
   virtual Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicPointsNode;

typedef std::list<AlembicPointsNode*> AlembicPointsNodeList;
typedef AlembicPointsNodeList::iterator AlembicPointsNodeListIter;

class AlembicPointsNode : public AlembicObjectEmitterNode
{
public:
  AlembicPointsNode();
   virtual ~AlembicPointsNode();

   void instanceInitialize(void);

   // override virtual methods from MPxNode
   virtual void PreDestruction();
   virtual void PostConstructor(void);
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
   AbcG::IPointsSchema mSchema;
   AbcG::IPoints obj;
   AlembicPointsNodeListIter listPosition;

   MStatus init(const MString &filename, const MString &identifier);

   // members
   SampleInfo mLastSampleInfo;
};

class AlembicPostImportPointsCommand : public MPxCommand
{
public:
  AlembicPostImportPointsCommand(void);
  virtual ~AlembicPostImportPointsCommand(void);

  virtual bool isUndoable(void) const { return false; }
  MStatus doIt(const MArgList& args);

  static MSyntax createSyntax(void);
  static void* creator(void);
};

#endif