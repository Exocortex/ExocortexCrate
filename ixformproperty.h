#ifndef _PYTHON_ALEMBIC_IXFORMPROPERTY_H_
#define _PYTHON_ALEMBIC_IXFORMPROPERTY_H_

#include "foundation.h"

typedef struct {
   PyObject_HEAD
   Alembic::AbcGeom::IXformSchema * mXformSchema;
} iXformProperty;

PyObject * iXformProperty_new(Alembic::Abc::IObject in_Object);

#endif