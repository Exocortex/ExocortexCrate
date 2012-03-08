#ifndef _ALEMBIC_SUBD_H_
#define _ALEMBIC_SUBD_H_

#include "AlembicObject.h"
#include <maya/MFnSubd.h>
#include <maya/MUint64Array.h>

class AlembicSubD: public AlembicObject
{
private:
   Alembic::AbcGeom::OSubD mObject;
   Alembic::AbcGeom::OSubDSchema mSchema;
   Alembic::AbcGeom::OSubDSchema::Sample mSample;
   std::vector<Alembic::Abc::V3f> mPosVec;
   std::vector<Alembic::Abc::int32_t> mFaceCountVec;
   std::vector<Alembic::Abc::int32_t> mFaceIndicesVec;
   std::vector<Alembic::Abc::V2f> mUvVec;
   std::vector<Alembic::Abc::uint32_t> mUvIndexVec;
   std::vector<unsigned int> mSampleLookup;

public:
   AlembicSubD(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicSubD();

   virtual Alembic::Abc::OObject GetObject() { return mObject; }
   virtual Alembic::Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicSubDNode : public AlembicObjectNode
{
public:
   AlembicSubDNode() {}
   virtual ~AlembicSubDNode();

   // override virtual methods from MPxNode
   virtual void PreDestruction();
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicSubDNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   Alembic::AbcGeom::ISubDSchema mSchema;
   static MObject mUvsAttr;

   // output attributes
   static MObject mOutGeometryAttr;
   static MObject mOutDispResolutionAttr;

   // members
   SampleInfo mLastSampleInfo;
   MObject mSubDData;
   MFnSubd mSubD;
   std::vector<unsigned int> mSampleLookup;
};

#endif