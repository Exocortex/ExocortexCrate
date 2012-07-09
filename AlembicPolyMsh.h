#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"
#include "AlembicIntermediatePolyMesh3DSMax.h"

class AlembicPolyMesh: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::OPolyMeshSchema mMeshSchema;
   Alembic::AbcGeom::XformSample mXformSample;
   Alembic::AbcGeom::OPolyMeshSchema::Sample mMeshSample;
   Alembic::Abc::ALEMBIC_VERSION_NS::OUInt32ArrayProperty mMatIdProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OStringArrayProperty mMatNamesProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mVelocityProperty;

   std::vector<Alembic::AbcGeom::OV2fGeomParam> mUvParams;

   materialsMergeStr materialsMerge;

public:

   AlembicPolyMesh(const SceneEntry &in_Ref, AlembicWriteJob *in_Job);
   ~AlembicPolyMesh();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual bool Save(double time, bool bLastFrame);

   void SaveMaterialsProperty(bool bFirstFrame, bool bLastFrame);
};

#endif
