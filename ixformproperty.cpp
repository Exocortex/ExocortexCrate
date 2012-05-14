#include "extension.h"
#include "ixformproperty.h"
#include "iobject.h"
#include <boost/lexical_cast.hpp>

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
   Alembic::Abc::TimeSamplingPtr ts = prop->mXformSchema->getTimeSampling();
   const std::vector <Alembic::Abc::chrono_t> & times = ts->getStoredTimes();
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

   Alembic::AbcGeom::XformSample sample;
   prop->mXformSchema->get(sample,sampleIndex);
   Alembic::Abc::M44d value = sample.getMatrix();

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
   {"getName", (PyCFunction)iXformProperty_getName, METH_NOARGS},
   {"getType", (PyCFunction)iXformProperty_getType, METH_NOARGS},
   {"getSampleTimes", (PyCFunction)iXformProperty_getSampleTimes, METH_NOARGS},
   {"getNbStoredSamples", (PyCFunction)iXformProperty_getNbStoredSamples, METH_NOARGS},
   {"getSize", (PyCFunction)iXformProperty_getSize, METH_VARARGS},
   {"getValues", (PyCFunction)iXformProperty_getValues, METH_VARARGS},
   {"isCompound", (PyCFunction)iXformProperty_isCompound, METH_NOARGS, "To distinguish between an iProperty and an iCompoundProperty, always returns false for iXformProperty."},
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
};

PyObject * iXformProperty_new(Alembic::Abc::IObject in_Object)
{
   ALEMBIC_TRY_STATEMENT
   if(!Alembic::AbcGeom::IXform::matches(in_Object.getMetaData()))
   {
      std::string msg;
      msg.append("Object '");
      msg.append(in_Object.getFullName());
      msg.append("' is not a Xform!");
      PyErr_SetString(getError(), msg.c_str());
      return NULL;
   }

   Alembic::AbcGeom::IXform xform(in_Object,Alembic::Abc::kWrapExisting);

   iXformProperty * prop = PyObject_NEW(iXformProperty, &iXformProperty_Type);
   prop->mXformSchema = new Alembic::AbcGeom::IXformSchema(xform.getSchema());
   return (PyObject *)prop;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}
