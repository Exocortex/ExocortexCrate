#ifndef _ALEMBIC_CAMERA_H_
#define _ALEMBIC_CAMERA_H_

#include "AlembicObject.h"

class AlembicCamera : public AlembicObject {
 private:
  AbcG::OCameraSchema mCameraSchema;
  AbcG::CameraSample mCameraSample;

 public:
  AlembicCamera(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                Abc::OObject oParent);
  ~AlembicCamera();

  virtual Abc::OCompoundProperty GetCompound();
  virtual XSI::CStatus Save(double time);
};

#endif