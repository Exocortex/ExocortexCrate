#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_

#include "AlembicObject.h"

void SaveXformSample(const SceneEntry &in_Ref, Alembic::AbcGeom::OXformSchema &schema, Alembic::AbcGeom::XformSample &sample, double time);

class AlembicXForm : public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::XformSample mXformSample;
public:
   AlembicXForm(const SceneEntry &in_Ref, AlembicWriteJob *in_Job);
   ~AlembicXForm();
   virtual bool Save(double time);
   virtual Alembic::Abc::OCompoundProperty GetCompound();
};

#endif