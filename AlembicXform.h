#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_

#include "AlembicObject.h"

class AlembicXform: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::XformSample mXformSample;
public:

   AlembicXform(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicXform();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual MStatus Save(double time);
};

#endif