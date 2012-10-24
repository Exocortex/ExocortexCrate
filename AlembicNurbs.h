#ifndef _ALEMBIC_NURBS_H_
#define _ALEMBIC_NURBS_H_

#include "AlembicObject.h"

class AlembicNurbs: public AlembicObject
{
private:
   AbcG::ONuPatchSchema mNurbsSchema;
   AbcG::ONuPatchSchema::Sample mNurbsSample;

public:

   AlembicNurbs(exoNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicNurbs();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif
