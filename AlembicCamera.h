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
    AlembicCamera(const SceneEntry &in_Ref, AlembicWriteJob *in_Job);
    ~AlembicCamera();

    virtual Abc::OCompoundProperty GetCompound();
    virtual bool Save(double time, bool bLastFrame);
};

#endif // _ALEMBIC_CAMERA_H_
