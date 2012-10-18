#ifndef _PYTHON_ALEMBIC_IOBJECT_H_
#define _PYTHON_ALEMBIC_IOBJECT_H_

#include "foundation.h"

//Alembic::Abc::ICompoundProperty getCompoundFromIObject(Alembic::Abc::IObject object);

typedef struct {
  PyObject_HEAD
  Alembic::Abc::IObject * mObject;
   void *mArchive;   // NEW, a reference to the iArchive, just like the one in oObject
   int tsIndex;      // for a quick access
} iObject;

PyObject * iObject_new(Alembic::Abc::IObject in_Object, void *in_Archive);

bool register_object_iObject(PyObject *module);

#endif
