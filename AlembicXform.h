#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_

#include "AlembicObject.h"

void SaveXformSample(XSI::CRef kinestateRef, Alembic::AbcGeom::OXformSchema & schema, Alembic::AbcGeom::XformSample & sample, double time, bool xformCache = false);

#endif