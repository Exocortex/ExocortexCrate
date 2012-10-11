#ifndef _ALEMBIC_POINTS_H_
#define _ALEMBIC_POINTS_H_

#include "AlembicObject.h"
#include <maya/MFnParticleSystem.h>
#include <list>

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
   Alembic::AbcGeom::IPointsSchema mSchema;
   Alembic::AbcGeom::IPoints obj;
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