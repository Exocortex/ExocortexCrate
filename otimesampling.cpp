#include "stdafx.h"
#include "extension.h"
#include "otimesampling.h"

static PyMethodDef oTimeSampling_methods[] =
{
   //{"getProperty", (PyCFunction)iObject_getProperty, METH_VARARGS, "Returns an iProperty for the given propertyName string."},
   //{"getSampleTimes", (PyCFunction)iTimeSampling_getSampleTimes, METH_NOARGS, "Returns a list fo sample times."},
   //{"getTsIndex", (PyCFunction)iTimeSampling_getTsIndex, METH_NOARGS, "Returns time sampling index of this time sampling."},
   {NULL, NULL}
};
static PyObject * oTimeSampling_getAttr(PyObject * self, char * attrName)
{
   return Py_FindMethod(oTimeSampling_methods, self, attrName);
}

static void oTimeSampling_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT //--- nothing from alembic here!
      ((oTimeSampling *)self)->ts.~TimeSampling();  // but sure to clean up everything!
      PyObject_FREE(self);
   ALEMBIC_VOID_CATCH_STATEMENT
}

/*       LATER
         static PyObject *oTimeSampling_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
         {
             Noddy *self;

             self = (Noddy *)type->tp_alloc(type, 0);
             if (self != NULL) {
                 self->first = PyString_FromString("");
                 if (self->first == NULL)
                   {
                     Py_DECREF(self);
                     return NULL;
                   }
                 
                 self->last = PyString_FromString("");
                 if (self->last == NULL)
                   {
                     Py_DECREF(self);
                     return NULL;
                   }

                 self->number = 0;
             }

             return (PyObject *)self;
         }

         static int oTimeSampling_init(Noddy *self, PyObject *args, PyObject *kwds)
         {
             PyObject *first=NULL, *last=NULL, *tmp;

             static char *kwlist[] = {"first", "last", "number", NULL};

             if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist, 
                                               &first, &last, 
                                               &self->number))
                 return -1; 

             if (first) {
                 tmp = self->first;
                 Py_INCREF(first);
                 self->first = first;
                 Py_XDECREF(tmp);
             }

             if (last) {
                 tmp = self->last;
                 Py_INCREF(last);
                 self->last = last;
                 Py_XDECREF(tmp);
             }

             return 0;
         }
//*/

static PyTypeObject oTimeSampling_Type =
{
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "oTimeSampling",                        // tp_name
  sizeof(oTimeSampling),                  // tp_basicsize
  0,                                // tp_itemsize
  (destructor)oTimeSampling_delete,       // tp_dealloc
  0,                                // tp_print
  (getattrfunc)oTimeSampling_getAttr,     // tp_getattr
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
  "This is the output time sampling.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  oTimeSampling_methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,//(initproc)oTimeSampling_init,      /* tp_init */    // LATER
    0,                         /* tp_alloc */
    0,//oTimeSampling_new,                 /* tp_new */     // LATER

};

PyObject * oTimeSampling_new(Abc::TimeSamplingPtr ts_ptr)
{
   ALEMBIC_TRY_STATEMENT
      oTimeSampling * ts = PyObject_NEW(oTimeSampling, &oTimeSampling_Type);
      if (ts != NULL)
      {
         new(&(ts->ts)) Abc::TimeSampling(*ts_ptr);
      }
      return (PyObject*)ts;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

PyObject * oTimeSampling_new(PyObject *time_list)
{
   bool is_list = false;
   PyObject *(*_GetItem)(PyObject*, Py_ssize_t) = PyTuple_GetItem;
   if(!PyTuple_Check(time_list))
   {
      if (PyList_Check(time_list))
      {
         _GetItem = PyList_GetItem;
         is_list = true;
      }
      else
      {
         PyErr_SetString(getError(), "times argument is not a tuple or a list!");
         return NULL;
      }
   }

   const size_t nbTimes = is_list ? PyList_Size(time_list) : PyTuple_Size(time_list);
   std::vector<Abc::chrono_t> ts_vector(nbTimes);

   float prev = -1.0f;
   bool not_first = false;
   for (int i = 0; i < nbTimes; ++i)
   {
      PyObject * item = _GetItem(time_list, i);
      float timeValue = 0.0f;
      if(!PyArg_Parse(item,"f",&timeValue))
      {
         PyErr_SetString(getError(), "An item in times is not a floating point number!");
         return NULL;
      }

      if (not_first && prev >= timeValue)
      {
         PyErr_SetString(getError(), "Time samples not in chronological order!");
         return NULL;
      }
      not_first = true;
      prev = timeValue;
      ts_vector[i] = timeValue;
   }

   ALEMBIC_TRY_STATEMENT

      oTimeSampling * ts = PyObject_NEW(oTimeSampling, &oTimeSampling_Type);
      if (ts != NULL)
      {
         const boost::uint32_t sz = ts_vector.size();
         if (sz > 1)
         {
            const double timePerCycle = ts_vector[sz-1] - ts_vector[0];
            Abc::TimeSamplingType samplingType(sz,timePerCycle);
            new(&(ts->ts)) Abc::TimeSampling(samplingType, ts_vector);      // The same TS type previously used by the Python API
         }
         else
         {
            new(&(ts->ts)) Abc::TimeSampling(1.0, ts_vector[0]);
         }
      }
      return (PyObject*)ts;

   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_oTimeSampling(PyObject *module)
{
   return register_object(module, oTimeSampling_Type, "oTimeSampling");
}

