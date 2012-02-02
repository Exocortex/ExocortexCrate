#ifndef _PYTHON_ALEMBIC_IOBJECT_H_
#define _PYTHON_ALEMBIC_IOBJECT_H_

#include "foundation.h"

Alembic::Abc::ICompoundProperty getCompoundFromIObject(Alembic::Abc::IObject object);

typedef struct {
  PyObject_HEAD
  Alembic::Abc::IObject * mObject;
} iObject;

PyObject * iObject_new(Alembic::Abc::IObject in_Object);

#endif