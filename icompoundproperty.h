#ifndef _PYTHON_ALEMBIC_ICOMPOUNDPROPERTY_H_
#define _PYTHON_ALEMBIC_ICOMPOUNDPROPERTY_H_

#include "foundation.h"
#include "iproperty.h" // for enum propertyTP


typedef struct
{
   PyObject_HEAD
   //bool mIsCompound;	// always true	USELESS FOR NOW
   //bool mIsArray;	// always false
   //propertyTP mPropType;
   Alembic::Abc::ICompoundProperty *mBaseCompoundProperty;
} iCompoundProperty;

PyObject * iCompoundProperty_new(Alembic::Abc::ICompoundProperty in_cprop, char * in_propName);

bool register_object_iCompoundProperty(PyObject *module);

#endif
