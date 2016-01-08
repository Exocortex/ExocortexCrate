#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicIntermediatePolyMesh3DSMax.h"
#include "AlembicObject.h"
#include "AlembicPropertyUtils.h"

class AlembicPolyMesh : public AlembicObject {
 private:
  AbcG::OPolyMeshSchema mMeshSchema;
  AbcG::OPolyMeshSchema::Sample mMeshSample;
  Abc::OUInt32ArrayProperty mMatIdProperty;
  Abc::OStringArrayProperty mMatNamesProperty;
  Abc::OV3fArrayProperty mVelocityProperty;

  std::vector<AbcG::OV2fGeomParam> mUvParams;

  materialsMergeStr materialsMerge;

  dynamicTopoVelocityCalc velocityCalc;

  AlembicCustomAttributesEx customAttributes;

 public:
  AlembicPolyMesh(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                  Abc::OObject oParent);
  ~AlembicPolyMesh();

  virtual Abc::OCompoundProperty GetCompound();
  virtual bool Save(double time, bool bLastFrame);

  void SaveMaterialsProperty(bool bFirstFrame, bool bLastFrame);
};

#endif
