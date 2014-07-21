#ifndef _ALEMBIC_CAMERA_H_
#define _ALEMBIC_CAMERA_H_

#include "AlembicObject.h"

class AlembicCamera: public AlembicObject
{
private:
    AbcG::OCameraSchema mCameraSchema;
    AbcG::CameraSample mCameraSample;

public:
    AlembicCamera(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
    ~AlembicCamera();

    virtual Abc::OCompoundProperty GetCompound();
    virtual bool Save(double time, bool bLastFrame);
};

#endif // _ALEMBIC_CAMERA_H_
