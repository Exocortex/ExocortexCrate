#ifndef _ALEMBIC_OBJECT_H_
#define _ALEMBIC_OBJECT_H_

#include "Foundation.h"

class AlembicWriteJob;

class AlembicObject
{
private:
   MObjectArray mRefs;
   AlembicWriteJob * mJob;
   Alembic::Abc::OObject mOParent;
protected:
   int mNumSamples;
   Alembic::AbcGeom::OVisibilityProperty mOVisibility;

public:
   AlembicObject(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicObject();

   AlembicWriteJob * GetJob() { return mJob; }
   const MObject & GetRef(ULONG index = 0) { return mRefs[index]; }
   ULONG GetRefCount() { return mRefs.length(); }
   void AddRef(const MObject & in_Ref) { mRefs.append(in_Ref); }
   Alembic::Abc::OObject GetOParent() { return mOParent; }
   virtual Alembic::Abc::OCompoundProperty GetCompound() = 0;
   int GetNumSamples() { return mNumSamples; }

   virtual MStatus Save(double time) = 0;
};

typedef boost::shared_ptr < AlembicObject > AlembicObjectPtr;

#include "AlembicWriteJob.h"

#endif