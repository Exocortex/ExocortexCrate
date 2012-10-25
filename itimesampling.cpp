#include "stdafx.h"
#include "extension.h"
#include "itimesampling.h"
#include "otimesampling.h"

static PyObject * iTimeSampling_getSampleTimes(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
      iTimeSampling *ts = (iTimeSampling *)self;
      const std::vector<Abc::chrono_t> & times = ts->ts_ptr->getStoredTimes();
      PyObject* ts_list = PyList_New(times.size());
      int ii = 0;
      for (std::vector<Abc::chrono_t>::const_iterator beg = times.begin(); beg != times.end(); ++beg, ++ii)
         PyList_SetItem(ts_list, ii, Py_BuildValue("f",(float)*beg));
      return ts_list;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iTimeSampling_getTsIndex(PyObject * self, PyObject * args)
{
   return Py_BuildValue("i",((iTimeSampling*)self)->tsIndex);
}

PyObject * iTimeSampling_createOTimeSampling(PyObject * iTS)
{
   ALEMBIC_TRY_STATEMENT
      return oTimeSampling_new(((iTimeSampling*)iTS)->ts_ptr);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyMethodDef iTimeSampling_methods[] =
{
   {"getSampleTimes", (PyCFunction)iTimeSampling_getSampleTimes, METH_NOARGS, "Returns a list fo sample times."},
   {"getTsIndex", (PyCFunction)iTimeSampling_getTsIndex, METH_NOARGS, "Returns time sampling index of this time sampling."},
   {NULL, NULL}
};
static PyObject * iTimeSampling_getAttr(PyObject * self, char * attrName)
{
   return Py_FindMethod(iTimeSampling_methods, self, attrName);
}

static void iTimeSampling_delete(PyObject * self)
{
   //ALEMBIC_TRY_STATEMENT //--- nothing from alembic here!
   iTimeSampling *ts = (iTimeSampling *)self;
   //ts->ts_ptr ??
   PyObject_FREE(self);
   //ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject iTimeSampling_Type =
{
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "iTimeSampling",                        // tp_name
  sizeof(iTimeSampling),                  // tp_basicsize
  0,                                // tp_itemsize
  (destructor)iTimeSampling_delete,       // tp_dealloc
  0,                                // tp_print
  (getattrfunc)iTimeSampling_getAttr,     // tp_getattr
  0,                                // tp_setattr
  0,                                // tp_compare
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "This is the input time sampling.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  iTimeSampling_methods,             /* tp_methods */
};

PyObject * iTimeSampling_new(Abc::TimeSamplingPtr ts_ptr, int tsIndex)
{
   ALEMBIC_TRY_STATEMENT
      iTimeSampling * ts = PyObject_NEW(iTimeSampling, &iTimeSampling_Type);
      if (ts != NULL)
      {
         ts->ts_ptr = ts_ptr;
         ts->tsIndex = tsIndex;
      }
      return (PyObject*)ts;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool is_iTimeSampling(PyObject *obj)
{
   return obj->ob_type == &iTimeSampling_Type;
}

bool register_object_iTimeSampling(PyObject *module)
{
   return register_object(module, iTimeSampling_Type, "iTimeSampling");
}

