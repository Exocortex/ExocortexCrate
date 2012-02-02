#ifndef _PYTHON_ALEMBIC_OXFORMPROPERTY_H_
#define _PYTHON_ALEMBIC_OXFORMPROPERTY_H_

#include "foundation.h"
#include "iproperty.h"
#include "oobject.h"

typedef struct {
   PyObject_HEAD
   void * mArchive;
   Alembic::AbcGeom::OXformSchema * mXformSchema;
} oXformProperty;

PyObject * oXformProperty_new(oObjectPtr in_casted, void * in_Archive);
void oXformProperty_deletePointers(oXformProperty * prop);

#endif