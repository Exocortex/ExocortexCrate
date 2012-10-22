#ifndef _ALEMBIC_SUBD_H_
#define _ALEMBIC_SUBD_H_

#include "AlembicObject.h"
#include <maya/MFnSubd.h>
#include <maya/MUint64Array.h>

class AlembicSubD: public AlembicObject
{
private:
   AbcG::OSubD mObject;
   AbcG::OSubDSchema mSchema;
   AbcG::OSubDSchema::Sample mSample;
   std::vector<Abc::V3f> mPosVec;
   std::vector<Abc::int32_t> mFaceCountVec;
   std::vector<Abc::int32_t> mFaceIndicesVec;
   std::vector<Abc::V2f> mUvVec;
   std::vector<Abc::uint32_t> mUvIndexVec;
   std::vector<unsigned int> mSampleLookup;

public:
   AlembicSubD(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicSubD();

   virtual Abc::OObject GetObject() { return mObject; }
   virtual Abc::OCompoundProperty GetCompound() { return mSchema; }
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
   AbcG::ISubDSchema mSchema;
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

class AlembicSubDDeformNode : public AlembicObjectDeformNode
{
public:
   virtual ~AlembicSubDDeformNode();
   // override virtual methods from MPxDeformerNode
   virtual void PreDestruction();
   virtual MStatus compute(const MPlug& plug, MDataBlock& dataBlock);
   virtual MStatus deform(MDataBlock & dataBlock, MItGeometry & iter, const MMatrix & localToWorld, unsigned int geomIndex);
   static void* creator() { return (new AlembicSubDDeformNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   AbcG::ISubDSchema mSchema;
   std::vector<unsigned int> mVertexLookup;

   // members
   SampleInfo mLastSampleInfo;
};

#endif