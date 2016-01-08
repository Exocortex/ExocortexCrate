#include "timesampling.h"
#include <vector>
#include "extension.h"
#include "stdafx.h"
#include "string.h"

static PyObject *TimeSampling_getType(PyObject *obj)
{
  ALEMBIC_TRY_STATEMENT
  EA_TimeSampling *ts = (EA_TimeSampling *)obj;
  if (ts->tsampling.get() == 0) {
    return Py_BuildValue("s", "unknown");
  }

  const AbcA::TimeSamplingType &tstype = ts->tsampling->getTimeSamplingType();
  if (tstype.isUniform()) {
    return Py_BuildValue("s", "uniform");
  }
  else if (tstype.isCyclic()) {
    return Py_BuildValue("s", "cyclic");
  }
  return Py_BuildValue("s", "acyclic");
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *TimeSampling_getTimeSamples(PyObject *obj)
{
  ALEMBIC_TRY_STATEMENT
  EA_TimeSampling *ts = (EA_TimeSampling *)obj;
  if (ts->tsampling.get() == 0) {
    return PyList_New(0);
  }

  const std::vector<Abc::chrono_t> &times = ts->tsampling->getStoredTimes();

  PyObject *tuple = PyList_New(times.size());
  for (size_t i = 0; i < times.size(); ++i) {
    PyList_SetItem(tuple, i, Py_BuildValue("f", (float)times[i]));
  }
  return tuple;
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyMethodDef TS_methods[] = {
    {"getType", (PyCFunction)TimeSampling_getType, METH_NOARGS,
     "Returns the type of the time sampling."},
    {"getTimeSamples", (PyCFunction)TimeSampling_getTimeSamples, METH_NOARGS,
     "Returns the time samples."},
    {NULL, NULL}};

static PyObject *TS_getAttr(PyObject *self, char *attrName)
{
  return Py_FindMethod(TS_methods, self, attrName);
}

static void TS_delete(PyObject *self)
{
  ALEMBIC_TRY_STATEMENT((EA_TimeSampling *)self)->tsampling.reset();
  PyObject_FREE(self);
  ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject TS_Type = {
    PyObject_HEAD_INIT(&PyType_Type) 0,       // op_size
    "TimeSampling",                           // tp_name
    sizeof(EA_TimeSampling),                  // tp_basicsize
    0,                                        // tp_itemsize
    (destructor)TS_delete,                    // tp_dealloc
    0,                                        // tp_print
    (getattrfunc)TS_getAttr,                  // tp_getattr
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
    "This is the input object type. It provides access to all of the objects "
    "data, including basic data such as name + type as well as object based "
    "data such as propertynames and properties.", /* tp_doc */
    0,                                            /* tp_traverse */
    0,                                            /* tp_clear */
    0,                                            /* tp_richcompare */
    0,                                            /* tp_weaklistoffset */
    0,                                            /* tp_iter */
    0,                                            /* tp_iternext */
    TS_methods,                                   /* tp_methods */
};

PyObject *TimeSamplingCopy(const AbcA::TimeSamplingPtr tsampling)
{
  ALEMBIC_TRY_STATEMENT
  EA_TimeSampling *TS = PyObject_NEW(EA_TimeSampling, &TS_Type);
  new (&(TS->tsampling)) AbcA::TimeSamplingPtr();
  TS->tsampling = tsampling;
  return (PyObject *)TS;
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

PyObject *TimeSampling_new(PyObject *self, PyObject *args)
{
  ALEMBIC_TRY_STATEMENT

  char *type = 0;
  PyObject *arg1 = 0, *arg2 = 0;
  if (!PyArg_ParseTuple(args, "s|OO", &type, &arg1, &arg2)) {
    PyErr_SetString(getError(), "No time sampling type specified!");
    return NULL;
  }

  int ts_type = -1;  // mean invalid
  if (strcmp(type, "uniform") == 0) {
    ts_type = 0;
  }
  else if (strcmp(type, "cyclic") == 0) {
    ts_type = 1;
  }
  else if (strcmp(type, "acyclic") == 0) {
    ts_type = 2;
  }
  else {
    PyErr_SetString(getError(),
                    "invalid type, should be uniform, cyclic, or acyclic");
    return NULL;
  }

  std::vector<Abc::chrono_t> times;
  AbcA::TimeSamplingType TSType;
  if (ts_type < 1) {  // uniform
    if (arg1) {  // arg1 not null... then can assume arg2 is not null either!
      if (!PyFloat_Check(arg1)) {
        PyErr_SetString(getError(), "the argument is not a real!");
        return NULL;
      }

      new (&TSType) AbcA::TimeSamplingType(PyFloat_AS_DOUBLE(arg1));

      if (arg2) {
        if (!PyFloat_Check(arg2)) {
          PyErr_SetString(getError(), "the argument is not a real!");
          return NULL;
        }
        times.resize(1);
        times[0] = PyFloat_AS_DOUBLE(arg2);
      }
    }

    if (times.empty()) {
      times.resize(1);
      times[0] = TSType.getTimePerCycle();
    }
  }
  else {
    if (!arg1) {
      PyErr_SetString(getError(), "Missing time samples!");
      return NULL;
    }

    // read from an array or a tuple ??
    bool is_list = false;
    PyObject *(*_GetItem)(PyObject *, Py_ssize_t) = PyTuple_GetItem;
    if (!PyTuple_Check(arg1)) {
      if (PyList_Check(arg1)) {
        _GetItem = PyList_GetItem;
        is_list = true;
      }
      else {
        PyErr_SetString(getError(),
                        "The time samples must be an array or a tuple!");
        return NULL;
      }
    }

    const size_t nbTimes = is_list ? PyList_Size(arg1) : PyTuple_Size(arg1);
    if (nbTimes == 0) {
      PyErr_SetString(getError(), "No time samples specified!");
      return NULL;
    }

    times.resize(nbTimes);
    for (int i = 0; i < nbTimes; ++i) {
      PyObject *item = _GetItem(arg1, i);
      if (!PyFloat_Check(item)) {
        PyErr_SetString(getError(), "All time samples must be real values");
        return NULL;
      }
      times[i] = PyFloat_AS_DOUBLE(item);
    }

    // initialize TSType according to the type!
    if (ts_type == 1) {
      const float delta = times[times.size() - 1] - times[0];
      new (&TSType) AbcA::TimeSamplingType(times.size(), delta);
    }
    else {
      new (&TSType) AbcA::TimeSamplingType(AbcA::TimeSamplingType::kAcyclic);
    }
  }

  EA_TimeSampling *TS = PyObject_NEW(EA_TimeSampling, &TS_Type);
  new (&(TS->tsampling)) AbcA::TimeSamplingPtr();  // need to do this to have a
  // default empty
  // timesamplingptr
  TS->tsampling.reset(new AbcA::TimeSampling(TSType, times));
  return (PyObject *)TS;

  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool PyObject_TimeSampling_Check(PyObject *obj)
{
  return obj->ob_type == &TS_Type;
}

bool register_object_TS(PyObject *module)
{
  return register_object(module, TS_Type, "TimeSampling");
}
