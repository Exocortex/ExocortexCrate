#include "stdafx.h"
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
#include "timesampling.h"


std::string resolvePath_Internal(std::string const& path)
{
	return path;
}

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

static const char* OBJ_TYPES[] =
{
   "AbcGeom_Xform_v1",
   "AbcGeom_Camera_v1",
   "AbcGeom_PolyMesh_v3",
   "AbcGeom_SubD_v3",
   "AbcGeom_Curve_v1",
   "AbcGeom_Points_v1",
   "AbcGeom_FaceSet_v1",

   0
};

static PyObject* extension_getObjectTypes(PyObject *self)
{
   PyObject *_list = PyList_New(0);
   const char **types = OBJ_TYPES;
   while (*types)
   {
      PyList_Append(_list, Py_BuildValue("s", *types));
      ++types;
   }
   return _list;
}

static const char* PRO_TYPES[] =
{
   "bool",
   "uchar",   "char",
   "uint16",   "int16",
   "uint32",   "int32",
   "uint64",   "int64",
   "half",
   "float",
   "double",
   "string",   "wstring",
   "vector2s", "vector2i", "vector2f", "vector2d", "vector3s", "vector3i", "vector3f", "vector3d",
   "point2s",  "point2i",  "point2f",  "point2d",  "point3s",  "point3i",  "point3f",  "point3d",
   "box2s",    "box2i",    "box2f",    "box2d",    "box3s",    "box3i",    "box3f",    "box3d",
   "matrix3f", "matrix3d", "matrix4f", "matrix4d",
   "color3h",  "color3f",  "color3c",  "color4h",  "color4f",  "color4c",
   "normal2f", "normal2d", "normal3f", "normal3d",

   0
};

static PyObject* extension_getPropertyTypes(PyObject *self)
{
   PyObject *_list = PyList_New(0);
   const char **types = PRO_TYPES;
   while (*types)
   {
      PyList_Append(_list, Py_BuildValue("s", *types));
      ++types;
   }
   return _list;
}

#include "CommonRegex.h"

static PyMethodDef extension_methods[] =
{
   {"getVersion", (PyCFunction)extension_getVersion, METH_NOARGS, "Returns the version number of the alembic extension"},
   {"getAlembicVersion", (PyCFunction)extension_getVersion, METH_NOARGS, "Returns the Alembic.IO version number used for the extension"},
   {"getIArchive", (PyCFunction)iArchive_new, METH_VARARGS, "Takes in a filename to an Alembic file, and returns an iArchive linked to that file."},
   {"getOArchive", (PyCFunction)oArchive_new, METH_VARARGS, "Takes in a filename to create an Alembic file at, and return an oArchive linked to that file."},

   {"getObjectTypes", (PyCFunction)extension_getObjectTypes, METH_NOARGS, "Returns a list of all valid object types. The same one listed in Appendix A."},
   {"getPropertyTypes", (PyCFunction)extension_getPropertyTypes, METH_NOARGS, "Returns a list of all valid property types. The same one listed in Appendix B."},
   {"createTimeSampling", (PyCFunction)TimeSampling_new, METH_VARARGS, "Returns a new Time Sampling"},
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


