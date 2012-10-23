#ifndef _ALEMBIC_NURBS_H_
#define _ALEMBIC_NURBS_H_

#include "AlembicObject.h"

class AlembicNurbs: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::ONuPatchSchema mNurbsSchema;
   Alembic::AbcGeom::XformSample mXformSample;
   Alembic::AbcGeom::ONuPatchSchema::Sample mNurbsSample;

public:

   AlembicNurbs(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job, Alembic::Abc::OObject oParent);
   ~AlembicNurbs();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif
