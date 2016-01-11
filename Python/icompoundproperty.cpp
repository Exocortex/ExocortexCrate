#include "stdafx.h"

#include "icompoundproperty.h"
#include "AlembicLicensing.h"
#include "extension.h"
#include "iobject.h"
#include "timesampling.h"

#undef iProperty

#ifdef __cplusplus__
extern "C" {
#endif

static PyObject *iCompoundProperty_getName(PyObject *self, PyObject *args)
{
  ALEMBIC_TRY_STATEMENT
  return Py_BuildValue(
      "s",
      ((iCompoundProperty *)self)->mBaseCompoundProperty->getName().c_str());
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *iCompoundProperty_getType(PyObject *self, PyObject *args)
{
  return Py_BuildValue("s", "compound");
}

static PyObject *iCompoundProperty_getSampleTimes(PyObject *self,
                                                  PyObject *args)
{
  ALEMBIC_TRY_STATEMENT
  Abc::TimeSamplingPtr ts =
      ((iCompoundProperty *)self)->mBaseCompoundProperty->getTimeSampling();
  if (ts) {
    return TimeSamplingCopy(ts);
  }
  return Py_BuildValue("s", "unsupported");
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *iCompoundProperty_getNbStoredSamples(PyObject *self,
                                                      PyObject *args)
{
  return Py_BuildValue("I", (unsigned int)0);
}

static PyObject *iCompoundProperty_getSize(PyObject *self, PyObject *args)
{
  return Py_BuildValue("I", (unsigned int)0);
}

static PyObject *iCompoundProperty_getValues(PyObject *self, PyObject *args)
{
  return PyTuple_New(0);
}

static PyObject *iCompoundProperty_getPropertyNames(PyObject *self,
                                                    PyObject *args)
{
  ALEMBIC_TRY_STATEMENT
  Abc::ICompoundProperty *icprop =
      ((iCompoundProperty *)self)->mBaseCompoundProperty;
  const int nb_prop = icprop->getNumProperties();

  PyObject *tuple = PyTuple_New(nb_prop);
  for (int i = 0; i < nb_prop; ++i)
    PyTuple_SetItem(
        tuple, i,
        Py_BuildValue("s", icprop->getPropertyHeader(i).getName().c_str()));

  return tuple;
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *iCompoundProperty_getProperty(PyObject *self, PyObject *args)
{
  ALEMBIC_TRY_STATEMENT
  char *propName = NULL;
  if (!PyArg_ParseTuple(args, "s", &propName)) {
    PyErr_SetString(getError(), "No property name specified!");
    return NULL;
  }
  return iProperty_new(*((iCompoundProperty *)self)->mBaseCompoundProperty,
                       propName);
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *iCompoundProperty_isCompound(PyObject *self, PyObject *args)
{
  Py_INCREF(Py_True);
  return Py_True;
}

static PyMethodDef iCompoundProperty_methods[] = {
    {"getName", (PyCFunction)iCompoundProperty_getName, METH_NOARGS,
     "Returns the name of this compound property."},
    {"getType", (PyCFunction)iCompoundProperty_getType, METH_NOARGS,
     "Returns the type, always \"compound\"."},
    {"getSampleTimes", (PyCFunction)iCompoundProperty_getSampleTimes,
     METH_NOARGS,
     "Returns the TimeSampling this object is linked to, always zero for a "
     "compound."},
    {"getNbStoredSamples", (PyCFunction)iCompoundProperty_getNbStoredSamples,
     METH_NOARGS,
     "Returns the actual number of stored samples, always zero for a "
     "compound."},
    {"getSize", (PyCFunction)iCompoundProperty_getSize, METH_VARARGS,
     "Returns zero because a compound property does not have any values."},
    {"getValues", (PyCFunction)iCompoundProperty_getValues, METH_VARARGS,
     "Returns an empty tuple because a compound property does not have any "
     "values."},
    {"getPropertyNames", (PyCFunction)iCompoundProperty_getPropertyNames,
     METH_NOARGS,
     "Returns a string list of all property names below this compound."},
    {"getProperty", (PyCFunction)iCompoundProperty_getProperty, METH_VARARGS,
     "Returns an input property (iProperty/iCompoundProperty/iXformProperty) "
     "for the given propertyName string."},
    {"isCompound", (PyCFunction)iCompoundProperty_isCompound, METH_NOARGS,
     "To distinguish between an iProperty and an iCompoundProperty, always "
     "returns true for iCompoundProperty."},
    {NULL, NULL}};

static PyObject *iCompoundProperty_getAttr(PyObject *self, char *attrName)
{
  return Py_FindMethod(iCompoundProperty_methods, self, attrName);
}

static void iCompoundProperty_delete(PyObject *self)
{
  ALEMBIC_TRY_STATEMENT
  delete ((iCompoundProperty *)self)->mBaseCompoundProperty;
  PyObject_FREE(self);
  ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject iCompoundProperty_Type = {
    PyObject_HEAD_INIT(&PyType_Type) 0,       // op_size
    "iCompoundProperty",                      // tp_name
    sizeof(iCompoundProperty),                // tp_basicsize
    0,                                        // tp_itemsize
    (destructor)iCompoundProperty_delete,     // tp_dealloc
    0,                                        // tp_print
    (getattrfunc)iCompoundProperty_getAttr,   // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "This is the input compound property. It provides access to the "
    "compound property’s data, such as name, type and per sample values. "
    "iCompoundProperty has the same set of function as iProperty and "
    "iXformProperty so it can be used like a normal property but many "
    "functions have default values typical to a compound. "
    "iCompoundProperty also behave like an iObject because it has "
    "properties under it.",    /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    iCompoundProperty_methods, /* tp_methods */
};

#ifdef __cplusplus__
}
#endif

PyObject *iCompoundProperty_new(Abc::ICompoundProperty in_cprop,
                                char *in_propName)
{
  ALEMBIC_TRY_STATEMENT
  iCompoundProperty *prop =
      PyObject_NEW(iCompoundProperty, &iCompoundProperty_Type);
  prop->mBaseCompoundProperty =
      new Abc::ICompoundProperty(in_cprop, in_propName);
  return (PyObject *)prop;
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_iCompoundProperty(PyObject *module)
{
  return register_object(module, iCompoundProperty_Type, "iCompoundProperty");
}
