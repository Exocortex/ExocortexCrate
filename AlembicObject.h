#ifndef _ALEMBIC_OBJECT_H_
#define _ALEMBIC_OBJECT_H_

#include "Foundation.h"

class AlembicWriteJob;
class AlembicObject;

typedef boost::shared_ptr < AlembicObject > AlembicObjectPtr;

class AlembicObject
{
private:
   MObjectArray mRefs;
   AlembicWriteJob * mJob;
   AlembicObjectPtr mParent;
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
   virtual Alembic::Abc::OObject GetObject() = 0;
   Alembic::Abc::OObject GetParentObject();
   virtual Alembic::Abc::OCompoundProperty GetCompound() = 0;
   int GetNumSamples() { return mNumSamples; }

   virtual MStatus Save(double time) = 0;
};

#include "AlembicWriteJob.h"

#endif