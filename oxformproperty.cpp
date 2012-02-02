#include "extension.h"
#include "oxformproperty.h"
#include "oobject.h"
#include "oarchive.h"
#include <boost/lexical_cast.hpp>

static std::string oXformProperty_getName_func()
{
   return ".xform";
}

static PyObject * oXformProperty_getName(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   return Py_BuildValue("s",oXformProperty_getName_func().c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * oXformProperty_setValues(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT

   oXformProperty * prop = (oXformProperty*)self;
   if(prop->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }

   PyObject * tuple = NULL;
   if(!PyArg_ParseTuple(args, "O", &tuple))
   {
      PyErr_SetString(getError(), "No sample tuple specified!");
      return NULL;
   }
   if(!PyTuple_Check(tuple) && !PyList_Check(tuple))
   {
      PyErr_SetString(getError(), "Sample tuple argument is not a tuple!");
      return NULL;
   }

   if(prop->mXformSchema->getNumSamples() >= prop->mXformSchema->getTimeSampling()->getNumStoredTimes())
   {
      PyErr_SetString(getError(), "Already stored the maximum number of samples!");
      return NULL;
   }

   size_t nbItems = 0;
   if(PyTuple_Check(tuple))
      nbItems = PyTuple_Size(tuple);
   else
      nbItems = PyList_Size(tuple);

   if(nbItems != 16)
   {
      PyErr_SetString(getError(), "Sample tuple should contain 16 items!");
      return NULL;
   }

   std::vector<double> values(16);
   for(size_t i=0;i<16;i++)
   {
      PyObject * item = NULL;
      if(PyTuple_Check(tuple))
         item = PyTuple_GetItem(tuple,i);
      else
         item = PyList_GetItem(tuple,i);
      if(!PyArg_Parse(item,"d",&values[i]))
      {
         PyErr_SetString(getError(), "Some item of the sample tuple is not a floating point number!");
         return NULL;
      }
   }

   Imath::M44d matrix(values[0],values[1],values[2],values[3],values[4],values[5],values[6],values[7],
                      values[8],values[9],values[10],values[11],values[12],values[13],values[14],values[15]);
   Alembic::AbcGeom::XformSample sample;
   sample.setMatrix(matrix);
   prop->mXformSchema->set(sample);

   return Py_BuildValue("I",prop->mXformSchema->getNumSamples());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyMethodDef oXformProperty_methods[] = {
   {"getName", (PyCFunction)oXformProperty_getName, METH_NOARGS},
   {"setValues", (PyCFunction)oXformProperty_setValues, METH_VARARGS},
   {NULL, NULL}
};

static PyObject * oXformProperty_getAttr(PyObject * self, char * attrName)
{
   ALEMBIC_TRY_STATEMENT
   return Py_FindMethod(oXformProperty_methods, self, attrName);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

void oXformProperty_deletePointers(oXformProperty * prop)
{
   ALEMBIC_TRY_STATEMENT
   if(prop->mXformSchema == NULL)
      return;
   delete(prop->mXformSchema);
   prop->mXformSchema = NULL;
   ALEMBIC_VOID_CATCH_STATEMENT
}


static void oXformProperty_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   oXformProperty * prop = (oXformProperty*)self;
   oXformProperty_deletePointers(prop);
   PyObject_FREE(prop);
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject oXformProperty_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "oXformProperty",                 // tp_name
  sizeof(oXformProperty),           // tp_basicsize
  0,                                // tp_itemsize
  (destructor)oXformProperty_delete,     // tp_dealloc
  0,                                // tp_print
  (getattrfunc)oXformProperty_getAttr,   // tp_getattr
  0,                                // tp_setattr
  0,                                // tp_compare
};

PyObject * oXformProperty_new(oObjectPtr in_casted, void * in_Archive)
{
   ALEMBIC_TRY_STATEMENT

   // check if we already have this property somewhere
   std::string identifier = in_casted.mXform->getFullName();
   oArchive * archive = (oArchive*)in_Archive;
   oArchiveXformPropertyIt it = archive->mXformProperties->find(identifier);
   if(it != archive->mXformProperties->end())
      return (PyObject*)it->second;

   // if we don't have it yet, create a new one and insert it into our map
   oXformProperty * prop = PyObject_NEW(oXformProperty, &oXformProperty_Type);
   prop->mXformSchema = new Alembic::AbcGeom::OXformSchema(in_casted.mXform->getSchema().getPtr(),Alembic::Abc::kWrapExisting);
   prop->mArchive = in_Archive;
   Py_INCREF((PyObject *)prop);
   archive->mXformProperties->insert(oArchiveXformPropertyPair(identifier,prop));

   return (PyObject *)prop;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}
