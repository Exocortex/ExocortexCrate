#ifndef _PYTHON_ALEMBIC_IOBJECT_H_
#define _PYTHON_ALEMBIC_IOBJECT_H_

typedef struct {
  PyObject_HEAD Abc::IObject *mObject;
  void *mArchive;  // NEW, a reference to the iArchive, just like the one in
  // oObject
  int tsIndex;  // for a quick access
} iObject;

PyObject *iObject_new(Abc::IObject in_Object, void *in_Archive);

bool register_object_iObject(PyObject *module);

#endif
