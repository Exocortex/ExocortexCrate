#include "stdafx.h"
#include "extension.h"
#include "oxformproperty.h"
#include "oobject.h"
#include "oarchive.h"

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

   if(prop->mMembers->mXformSchema.getNumSamples() >= prop->mMaxNbSamples)
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
   prop->mMembers->mSample.setInheritsXforms(true);
   prop->mMembers->mSample.setMatrix(matrix);
   prop->mMembers->mXformSchema.set(prop->mMembers->mSample);

   return Py_BuildValue("I",prop->mMembers->mXformSchema.getNumSamples());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *oXformProperty_isCompound(PyObject *self)
{
   Py_INCREF(Py_False);
   return Py_False;
}

static PyMethodDef oXformProperty_methods[] = {
   {"getName", (PyCFunction)oXformProperty_getName, METH_NOARGS, "Returns the name of the property."},
   {"setValues", (PyCFunction)oXformProperty_setValues, METH_VARARGS, "Appends a new sample to the property, given the values provided. The values have to be a flat list of components, matching the count of the property. For example if this is a vector3farray property the tuple has to contain a multiple of 3 float values."},
   {"isCompound", (PyCFunction)oXformProperty_isCompound, METH_NOARGS, "To distinguish between an oXformProperty and an oCompoundProperty, always returns false for oXformProperty."},
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
   if(prop->mMembers == NULL)
      return;
   prop->mMembers->mXformSchema.reset();
   delete(prop->mMembers);
   prop->mMembers = NULL;
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
  "This is the output Xform property. It provides methods for setting data on the Xform.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  oXformProperty_methods,             /* tp_methods */
};

PyObject * oXformProperty_new(oObjectPtr in_casted, void * in_Archive, boost::uint32_t tsIndex)
{
   ALEMBIC_TRY_STATEMENT

   // check if we already have this property somewhere
   std::string identifier = in_casted.mXform->getFullName() + "/.vals";
   oArchive * archive = (oArchive*)in_Archive;
   oXformProperty * prop = oArchive_getXformElement(archive,identifier);
   if(prop)
   {
      Py_INCREF(prop);
      return (PyObject*)prop;
   }

   // if we don't have it yet, create a new one and insert it into our map
   prop = PyObject_NEW(oXformProperty, &oXformProperty_Type);
   prop->mMaxNbSamples = archive->mArchive->getTimeSampling(tsIndex)->getNumStoredTimes();
   prop->mMembers = new oXformMembers();
   prop->mMembers->mXformSchema = in_casted.mXform->getSchema();
   //prop->mXformSchema = new AbcG::OXformSchema(in_casted.mXform->getSchema().getPtr(),Abc::kWrapExisting);
   //prop->mSample = new AbcG::XformSample();
   prop->mArchive = in_Archive;
   oArchive_registerXformElement(archive,identifier,prop);

   return (PyObject *)prop;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_oXformProperty(PyObject *module)
{
   return register_object(module, oXformProperty_Type, "oXformProperty");
}

