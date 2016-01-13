#ifndef _PYTHON_ALEMBIC_IXFORMPROPERTY_H_
#define _PYTHON_ALEMBIC_IXFORMPROPERTY_H_

typedef struct {
  PyObject_HEAD AbcG::IXformSchema *mXformSchema;
} iXformProperty;

PyObject *iXformProperty_new(Abc::IObject in_Object);

bool register_object_iXformProperty(PyObject *module);

#endif
