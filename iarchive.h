#ifndef _PYTHON_ALEMBIC_IARCHIVE_H_
#define _PYTHON_ALEMBIC_IARCHIVE_H_

#include "foundation.h"

typedef struct {
  PyObject_HEAD
  Alembic::Abc::IArchive * mArchive;
} iArchive;

PyObject * iArchive_new(PyObject * self, PyObject * args);
size_t getNbIArchives();

bool register_object_iArchive(PyObject *module);

#endif
