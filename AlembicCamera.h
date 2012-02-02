#ifndef _ALEMBIC_CAMERA_H_
#define _ALEMBIC_CAMERA_H_

#include "AlembicObject.h"

class AlembicCamera: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::OCameraSchema mCameraSchema;
   Alembic::AbcGeom::XformSample mXformSample;
   Alembic::AbcGeom::CameraSample mCameraSample;
public:

   AlembicCamera(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicCamera();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif