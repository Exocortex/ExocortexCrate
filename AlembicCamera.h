#ifndef _ALEMBIC_CAMERA_H_
#define _ALEMBIC_CAMERA_H_

#include "AlembicObject.h"

class AlembicCamera: public AlembicObject
{
private:
  AbcG::OXformSchema mXformSchema;
  AbcG::OCameraSchema mCameraSchema;
  AbcG::XformSample mXformSample;
  AbcG::CameraSample mCameraSample;
public:

   AlembicCamera(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicCamera();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif