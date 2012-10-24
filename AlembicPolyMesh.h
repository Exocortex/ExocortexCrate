#ifndef _ALEMBIC_POLYMESH_H_
#define _ALEMBIC_POLYMESH_H_

#include "AlembicObject.h"
#include <maya/MFnMesh.h>

class AlembicPolyMesh: public AlembicObject
{
private:
   Alembic::AbcGeom::OPolyMesh mObject;
   Alembic::AbcGeom::OPolyMeshSchema mSchema;
   int mPointCountLastFrame;
   std::vector<unsigned int> mSampleLookup;

   /*
   Alembic::AbcGeom::OPolyMeshSchema::Sample mSample;
   std::vector<Alembic::Abc::V3f> mPosVec;
   std::vector<Alembic::AbcGeom::OV2fGeomParam> mUvParams;
   std::vector<std::vector<Alembic::Abc::V2f> > mUvVec;
   std::vector<std::vector<Alembic::Abc::uint32_t> > mUvIndexVec;
   //*/

public:
   AlembicPolyMesh(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicPolyMesh();

   virtual Alembic::Abc::OObject GetObject() { return mObject; }
   virtual Alembic::Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicPolyMeshNode : public AlembicObjectNode
{
public:
   AlembicPolyMeshNode() {}
   virtual ~AlembicPolyMeshNode();

   // override virtual methods from MPxNode
   virtual void PreDestruction();
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
   Alembic::Abc::IObject mObj;
   Alembic::AbcGeom::IPolyMeshSchema mSchema;
   static MObject mNormalsAttr;
   static MObject mUvsAttr;

   // output attributes
   static MObject mOutGeometryAttr;

   // members
   SampleInfo mLastSampleInfo;
   MObject mMeshData;
   MFnMesh mMesh;
   std::vector<unsigned int> mSampleLookup;
   MIntArray mNormalFaces;
   MIntArray mNormalVertices;
};

class AlembicPolyMeshDeformNode : public AlembicObjectDeformNode
{
public:
   virtual ~AlembicPolyMeshDeformNode();
   // override virtual methods from MPxDeformerNode
   virtual void PreDestruction();
   virtual MStatus deform(MDataBlock & dataBlock, MItGeometry & iter, const MMatrix & localToWorld, unsigned int geomIndex);
   static void* creator() { return (new AlembicPolyMeshDeformNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   Alembic::Abc::IObject mObj;
   Alembic::AbcGeom::IPolyMeshSchema mSchema;

   // members
   SampleInfo mLastSampleInfo;
};

class AlembicCreateFaceSetsCommand : public MPxCommand
{
  public:
    AlembicCreateFaceSetsCommand() {}
    virtual ~AlembicCreateFaceSetsCommand()  {}

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator() { return new AlembicCreateFaceSetsCommand(); }
};

#endif
