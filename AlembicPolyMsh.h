#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"
#include "AlembicIntermediatePolyMesh3DSMax.h"

class AlembicPolyMesh: public AlembicObject
{
private:
   AbcG::OXformSchema mXformSchema;
   AbcG::OPolyMeshSchema mMeshSchema;
   AbcG::XformSample mXformSample;
   AbcG::OPolyMeshSchema::Sample mMeshSample;
   Abc::OUInt32ArrayProperty mMatIdProperty;
   Abc::OStringArrayProperty mMatNamesProperty;
   Abc::OV3fArrayProperty mVelocityProperty;

   std::vector<AbcG::OV2fGeomParam> mUvParams;

   materialsMergeStr materialsMerge;

   dynamicTopoVelocityCalc velocityCalc;

public:

   AlembicPolyMesh(const SceneEntry &in_Ref, AlembicWriteJob *in_Job);
   ~AlembicPolyMesh();

   virtual Abc::OCompoundProperty GetCompound();
   virtual bool Save(double time, bool bLastFrame);

   void SaveMaterialsProperty(bool bFirstFrame, bool bLastFrame);
};

#endif
