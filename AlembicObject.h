#ifndef _ALEMBIC_OBJECT_H_
#define _ALEMBIC_OBJECT_H_

#include "Foundation.h"
#include "AlembicMax.h"
#include "ObjectList.h"

class SceneEntry;
class AlembicWriteJob;

class AlembicObject
{
private:
    std::vector<const SceneEntry*> mRefs;
    Abc::OObject mOParent;
protected:
    int mNumSamples;
    AbcG::OVisibilityProperty mOVisibility;
    AlembicWriteJob * mJob;
	bool bForever;
public:
    AlembicObject(const SceneEntry & in_Ref, AlembicWriteJob * in_Job);
    ~AlembicObject();

    AlembicWriteJob * GetCurrentJob();
    const SceneEntry & GetRef(unsigned long index = 0);
    int GetRefCount();
    void AddRef(const SceneEntry & in_Ref);
    Abc::OObject GetOParent();
    virtual Abc::OCompoundProperty GetCompound() = 0;
    int GetNumSamples();

    virtual bool Save(double time, bool bLastFrame) = 0;
};

typedef boost::shared_ptr < AlembicObject > AlembicObjectPtr;

#include "AlembicWriteJob.h"

#endif