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
    AlembicCamera(const SceneEntry &in_Ref, AlembicWriteJob *in_Job);
    ~AlembicCamera();

    virtual Alembic::Abc::OCompoundProperty GetCompound();
    virtual bool Save(double time);
};

#endif // _ALEMBIC_CAMERA_H_
