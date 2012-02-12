#ifndef _ALEMBIC_POLYMESH_H_
#define _ALEMBIC_POLYMESH_H_

#include "AlembicObject.h"
#include <maya/MFnMesh.h>

class AlembicPolyMesh: public AlembicObject
{
private:
   Alembic::AbcGeom::OPolyMesh mObject;
   Alembic::AbcGeom::OPolyMeshSchema mSchema;
   Alembic::AbcGeom::OPolyMeshSchema::Sample mSample;
   std::vector<Alembic::Abc::V3f> mPosVec;
   std::vector<Alembic::Abc::N3f> mNormalVec;
   std::vector<Alembic::Abc::uint32_t> mNormalIndexVec;
   std::vector<Alembic::Abc::int32_t> mFaceCountVec;
   std::vector<Alembic::Abc::int32_t> mFaceIndicesVec;

public:
   AlembicPolyMesh(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicPolyMesh();

   virtual Alembic::Abc::OObject GetObject() { return mObject; }
   virtual Alembic::Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicPolyMeshNode : public MPxNode
{
public:
   AlembicPolyMeshNode() {}
   virtual ~AlembicPolyMeshNode();

   // override virtual methods from MPxNode
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicPolyMeshNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   Alembic::AbcGeom::IPolyMeshSchema mSchema;

   // output attributes
   static MObject mOutGeometryAttr;

   // members
   SampleInfo mLastSampleInfo;
   MObject mMeshData;
   MFnMesh mMesh;
};

#endif