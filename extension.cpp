#include "extension.h"
#include "iarchive.h"
#include "oarchive.h"
#include <time.h>

static PyObject * extension_error = NULL;
PyObject * getError() { return extension_error; }

static PyObject* extension_getVersion(PyObject* self, PyObject* args)
{
   return Py_BuildValue("i", ABCPY_VERSION_CURRENT);
}

static PyObject* extension_getAlembicVersion(PyObject* self, PyObject* args)
{
   return Py_BuildValue("i", ALEMBIC_VERSION);
}

static PyMethodDef extension_methods[] = {
   {"getVersion", (PyCFunction)extension_getVersion, METH_NOARGS},
   {"getAlembicVersion", (PyCFunction)extension_getVersion, METH_NOARGS},
   {"iArchive", (PyCFunction)iArchive_new, METH_VARARGS},
   {"oArchive", (PyCFunction)oArchive_new, METH_VARARGS},
   {NULL, NULL}
};

static PyMethodDef unlicensed_extension_methods[] = {
   {NULL, NULL}
};

EXTENSION_CALLBACK init_ExocortexAlembicPython(void)
{
   PyObject * m = Py_InitModule3("_ExocortexAlembicPython", extension_methods, "Exocortex Alembic Python Extension");
   PyObject * d = PyModule_GetDict(m);

   extension_error = PyErr_NewException("ExocortexAlembicPython.error", NULL, NULL);
   PyDict_SetItemString(d, "error", extension_error);
}