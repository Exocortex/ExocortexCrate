#include "extension.h"
#include "iarchive.h"
#include "oarchive.h"
#include "iobject.h"
#include "oobject.h"
#include "iproperty.h"
#include "oproperty.h"
#include "icompoundproperty.h"
#include "ocompoundproperty.h"
#include "ixformproperty.h"
#include "oxformproperty.h"

#include "itimesampling.h"
#include "otimesampling.h"
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

static PyMethodDef extension_methods[] =
{
   {"getVersion", (PyCFunction)extension_getVersion, METH_NOARGS, "Returns the version number of the alembic extension"},
   {"getAlembicVersion", (PyCFunction)extension_getVersion, METH_NOARGS, "Returns the Alembic.IO version number used for the extension"},
   {"getIArchive", (PyCFunction)iArchive_new, METH_VARARGS, "Takes in a filename to an Alembic file, and returns an iArchive linked to that file."},
   {"getOArchive", (PyCFunction)oArchive_new, METH_VARARGS, "Takes in a filename to create an Alembic file at, and return an oArchive linked to that file."}, 
   {NULL, NULL}
};

static PyMethodDef unlicensed_extension_methods[] = {
   {NULL, NULL}
};

bool register_object(PyObject *module, PyTypeObject &type_object, const char *object_name)
{
  if (PyType_Ready(&type_object) < 0)
    return false;

  Py_INCREF(&type_object);
  PyModule_AddObject(module, object_name, (PyObject*)&type_object);
  Py_DECREF(&type_object);
  return true;
}

EXTENSION_CALLBACK init_ExocortexAlembicPython(void)
{
   PyObject * m = Py_InitModule3("_ExocortexAlembicPython", extension_methods, "This is the core extension module. It provides access to input as well as output archive objects.");
   PyObject * d = PyModule_GetDict(m);

   register_object_iArchive(m);
   register_object_iObject(m);
   register_object_iProperty(m);
   register_object_iCompoundProperty(m);
   register_object_iXformProperty(m);
   register_object_iTimeSampling(m);

   register_object_oArchive(m);
   register_object_oObject(m);
   register_object_oProperty(m);
   register_object_oCompoundProperty(m);
   register_object_oXformProperty(m);
   register_object_oTimeSampling(m);

   extension_error = PyErr_NewException("ExocortexAlembicPython.error", NULL, NULL);
   PyDict_SetItemString(d, "error", extension_error);
}


