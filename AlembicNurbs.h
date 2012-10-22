#ifndef _ALEMBIC_NURBS_H_
#define _ALEMBIC_NURBS_H_

#include "AlembicObject.h"

class AlembicNurbs: public AlembicObject
{
private:
  AbcG::OXformSchema mXformSchema;
  AbcG::ONuPatchSchema mNurbsSchema;
  AbcG::XformSample mXformSample;
  AbcG::ONuPatchSchema::Sample mNurbsSample;

public:

   AlembicNurbs(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicNurbs();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif
