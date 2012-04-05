#ifndef _ALEMBIC_OBJECT_H_
#define _ALEMBIC_OBJECT_H_

#include "Foundation.h"
#include "ObjectList.h"

class SceneEntry;
class AlembicWriteJob;

class AlembicObject
{
private:
    std::vector<const SceneEntry*> mRefs;
    Alembic::Abc::OObject mOParent;
protected:
    int mNumSamples;
    Alembic::AbcGeom::OVisibilityProperty mOVisibility;
    AlembicWriteJob * mJob;
	bool bForever;
public:
    AlembicObject(const SceneEntry & in_Ref, AlembicWriteJob * in_Job);
    ~AlembicObject();

    AlembicWriteJob * GetCurrentJob();
    const SceneEntry & GetRef(unsigned long index = 0);
    int GetRefCount();
    void AddRef(const SceneEntry & in_Ref);
    Alembic::Abc::OObject GetOParent();
    virtual Alembic::Abc::OCompoundProperty GetCompound() = 0;
    int GetNumSamples();

    virtual bool Save(double time) = 0;
};

typedef boost::shared_ptr < AlembicObject > AlembicObjectPtr;

#include "AlembicWriteJob.h"

#endif