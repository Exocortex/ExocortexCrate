#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_

#include "AlembicObject.h"

void SaveXformSample(const SceneEntry &in_Ref, Alembic::AbcGeom::OXformSchema &schema, Alembic::AbcGeom::XformSample &sample, double time);

#endif