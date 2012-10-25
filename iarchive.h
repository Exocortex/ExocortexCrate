#ifndef _PYTHON_ALEMBIC_IARCHIVE_H_
#define _PYTHON_ALEMBIC_IARCHIVE_H_


typedef struct {
  PyObject_HEAD
  Abc::IArchive * mArchive;
} iArchive;

PyObject * iArchive_new(PyObject * self, PyObject * args);
size_t getNbIArchives();
bool isIArchiveOpened(std::string filename);

bool register_object_iArchive(PyObject *module);

#endif

