#ifndef _ALEMBIC_MODEL_H_
#define _ALEMBIC_MODEL_H_

#include "AlembicObject.h"

class AlembicModel: public AlembicObject
{
private:
  AbcG::OXformSchema mXformSchema;
  AbcG::XformSample mXformSample;
public:

   AlembicModel(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicModel();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif