#ifndef _ALEMBIC_MODEL_H_
#define _ALEMBIC_MODEL_H_

#include "AlembicObject.h"

class AlembicModel: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::XformSample mXformSample;
public:

   AlembicModel(exoNodePtr eNode, AlembicWriteJob * in_Job, Alembic::Abc::OObject oParent);
   ~AlembicModel();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif