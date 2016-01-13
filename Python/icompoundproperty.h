#ifndef _PYTHON_ALEMBIC_ICOMPOUNDPROPERTY_H_
#define _PYTHON_ALEMBIC_ICOMPOUNDPROPERTY_H_

#include "iproperty.h"  // for enum propertyTP

typedef struct {
  PyObject_HEAD Abc::ICompoundProperty *mBaseCompoundProperty;
} iCompoundProperty;

PyObject *iCompoundProperty_new(Abc::ICompoundProperty in_cprop,
                                char *in_propName);

bool register_object_iCompoundProperty(PyObject *module);

#endif
