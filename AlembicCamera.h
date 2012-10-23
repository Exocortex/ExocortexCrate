#ifndef _ALEMBIC_CAMERA_H_
#define _ALEMBIC_CAMERA_H_

#include "AlembicObject.h"

class AlembicCamera: public AlembicObject
{
private:
   Alembic::AbcGeom::OCameraSchema mCameraSchema;
   Alembic::AbcGeom::CameraSample mCameraSample;
public:

   AlembicCamera(exoNodePtr eNode, AlembicWriteJob * in_Job, Alembic::Abc::OObject oParent);
   ~AlembicCamera();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif