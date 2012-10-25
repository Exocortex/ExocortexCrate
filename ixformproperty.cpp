#include "stdafx.h"
#include "extension.h"
#include "ixformproperty.h"
#include "iobject.h"

static std::string iXformProperty_getName_func()
{
   return ".xform";
}

static PyObject * iXformProperty_getName(PyObject * self, PyObject * args)
{
   //ALEMBIC_TRY_STATEMENT  //--- does not access any Alembic objects
   return Py_BuildValue("s",iXformProperty_getName_func().c_str());
   //ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iXformProperty_getType(PyObject * self, PyObject * args)
{
   //ALEMBIC_TRY_STATEMENT  //--- does not access any Alembic objects
   return Py_BuildValue("s","matrix4d");
   //ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iXformProperty_getSampleTimes(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iXformProperty * prop = (iXformProperty*)self;
   Abc::TimeSamplingPtr ts = prop->mXformSchema->getTimeSampling();
   const std::vector <Abc::chrono_t> & times = ts->getStoredTimes();
   PyObject * tuple = PyTuple_New(times.size());
   for(size_t i=0;i<times.size();i++)
      PyTuple_SetItem(tuple,i,Py_BuildValue("f",(float)times[i]));
   return tuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static size_t iXformProperty_getNbStoredSamples_func(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   iXformProperty * prop = (iXformProperty*)self;
   return prop->mXformSchema->getNumSamples();
   ALEMBIC_VALUE_CATCH_STATEMENT(0)
}

static PyObject * iXformProperty_getNbStoredSamples(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   return Py_BuildValue("I",(unsigned int)iXformProperty_getNbStoredSamples_func(self));
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iXformProperty_getSize(PyObject * self, PyObject * args)
{
   //ALEMBIC_TRY_STATEMENT  //--- does not access any Alembic objects
   return Py_BuildValue("I",(unsigned int)1);
   //ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iXformProperty_getValues(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iXformProperty * prop = (iXformProperty*)self;

   unsigned long long sampleIndex = 0;
   unsigned long long start = 0;
   unsigned long long count = 0;
   PyArg_ParseTuple(args, "|KKK", &sampleIndex,&start,&count);

   size_t numSamples = iXformProperty_getNbStoredSamples_func(self);
   if(sampleIndex >= numSamples)
   {
      std::string msg;
      msg.append("SampleIndex for Property '");
      msg.append(iXformProperty_getName_func());
      msg.append("' is out of bounds!");
      PyErr_SetString(getError(), msg.c_str());
      return NULL;
   }

   PyObject * tuple = NULL;
   tuple = PyTuple_New(16);

   AbcG::XformSample sample;
   prop->mXformSchema->get(sample,sampleIndex);
   Abc::M44d value = sample.getMatrix();

   PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x[0][0]));
   PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.x[0][1]));
   PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.x[0][2]));
   PyTuple_SetItem(tuple,3,Py_BuildValue("d",value.x[0][3]));
   PyTuple_SetItem(tuple,4,Py_BuildValue("d",value.x[1][0]));
   PyTuple_SetItem(tuple,5,Py_BuildValue("d",value.x[1][1]));
   PyTuple_SetItem(tuple,6,Py_BuildValue("d",value.x[1][2]));
   PyTuple_SetItem(tuple,7,Py_BuildValue("d",value.x[1][3]));
   PyTuple_SetItem(tuple,8,Py_BuildValue("d",value.x[2][0]));
   PyTuple_SetItem(tuple,9,Py_BuildValue("d",value.x[2][1]));
   PyTuple_SetItem(tuple,10,Py_BuildValue("d",value.x[2][2]));
   PyTuple_SetItem(tuple,11,Py_BuildValue("d",value.x[2][3]));
   PyTuple_SetItem(tuple,12,Py_BuildValue("d",value.x[3][0]));
   PyTuple_SetItem(tuple,13,Py_BuildValue("d",value.x[3][1]));
   PyTuple_SetItem(tuple,14,Py_BuildValue("d",value.x[3][2]));
   PyTuple_SetItem(tuple,15,Py_BuildValue("d",value.x[3][3]));

   return tuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *iXformProperty_isCompound(PyObject *self, PyObject * args)
{
   Py_INCREF(Py_False);
   return Py_False;
}

static PyMethodDef iXformProperty_methods[] =
{
   {"getName", (PyCFunction)iXformProperty_getName, METH_NOARGS, "Returns the name of this property."},
   {"getType", (PyCFunction)iXformProperty_getType, METH_NOARGS, "Returns the type of this property."},
   {"getSampleTimes", (PyCFunction)iXformProperty_getSampleTimes, METH_NOARGS, "Returns the TimeSampling this object is linked to."},
   {"getNbStoredSamples", (PyCFunction)iXformProperty_getNbStoredSamples, METH_NOARGS, "Returns the actual number of stored samples."},
   {"getSize", (PyCFunction)iXformProperty_getSize, METH_VARARGS, "Returns the size of the property. For single value properties, this method returns 1, for array value properties it returns the size of the array."},
   {"getValues", (PyCFunction)iXformProperty_getValues, METH_VARARGS, "Returns the values of the property at the (optional) sample index."},
   {"isCompound", (PyCFunction)iXformProperty_isCompound, METH_NOARGS, "To distinguish between an iXformProperty and an iCompoundProperty, always returns false for iXformProperty."},
   {NULL, NULL}
};

static PyObject * iXformProperty_getAttr(PyObject * self, char * attrName)
{
   //ALEMBIC_TRY_STATEMENT  //--- does not access any Alembic objects
   return Py_FindMethod(iXformProperty_methods, self, attrName);
   //ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static void iXformProperty_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   iXformProperty * prop = (iXformProperty*)self;
   delete(prop->mXformSchema);
   PyObject_FREE(prop);
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject iXformProperty_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "iXformProperty",                      // tp_name
  sizeof(iXformProperty),                // tp_basicsize
  0,                                // tp_itemsize
  (destructor)iXformProperty_delete,     // tp_dealloc
  0,                                // tp_print
  (getattrfunc)iXformProperty_getAttr,   // tp_getattr
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
  "This is the input Xform property. It provides access to the Xform's data, such as name, type and per sample values.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  iXformProperty_methods,             /* tp_methods */
};

PyObject * iXformProperty_new(Abc::IObject in_Object)
{
   ALEMBIC_TRY_STATEMENT
   if(!AbcG::IXform::matches(in_Object.getMetaData()))
   {
      std::string msg;
      msg.append("Object '");
      msg.append(in_Object.getFullName());
      msg.append("' is not a Xform!");
      PyErr_SetString(getError(), msg.c_str());
      return NULL;
   }

   AbcG::IXform xform(in_Object,Abc::kWrapExisting);

   iXformProperty * prop = PyObject_NEW(iXformProperty, &iXformProperty_Type);
   prop->mXformSchema = new AbcG::IXformSchema(xform.getSchema());
   return (PyObject *)prop;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_iXformProperty(PyObject *module)
{
  return register_object(module, iXformProperty_Type, "iXformProperty");
}


