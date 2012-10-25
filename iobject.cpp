#include "stdafx.h"
#include "extension.h"
#include "iarchive.h"
#include "iobject.h"
#include "iproperty.h"
#include "ixformproperty.h"
#include "CommonUtilities.h"

/*
Abc::ICompoundProperty getCompoundFromIObject(Abc::IObject object)
{
   ALEMBIC_TRY_STATEMENT
   const Abc::MetaData &md = object.getMetaData();
   if(AbcG::IXform::matches(md)) {
      return AbcG::IXform(object,Abc::kWrapExisting).getSchema();
   } else if(AbcG::IPolyMesh::matches(md)) {
      return AbcG::IPolyMesh(object,Abc::kWrapExisting).getSchema();
   } else if(AbcG::ICurves::matches(md)) {
      return AbcG::ICurves(object,Abc::kWrapExisting).getSchema();
   } else if(AbcG::INuPatch::matches(md)) {
      return AbcG::INuPatch(object,Abc::kWrapExisting).getSchema();
   } else if(AbcG::IPoints::matches(md)) {
      return AbcG::IPoints(object,Abc::kWrapExisting).getSchema();
   } else if(AbcG::ISubD::matches(md)) {
      return AbcG::ISubD(object,Abc::kWrapExisting).getSchema();
   } else if(AbcG::ICamera::matches(md)) {
      return AbcG::ICamera(object,Abc::kWrapExisting).getSchema();

   // NEW
   } else if(AbcG::IFaceSet::matches(md)) {
      return AbcG::IFaceSet(object,Abc::kWrapExisting).getSchema();
   }
   return Abc::ICompoundProperty();
   ALEMBIC_VALUE_CATCH_STATEMENT(Abc::ICompoundProperty())
}

Abc::TimeSamplingPtr getTimeSamplingFromObject(Abc::IObject object)
{
   ALEMBIC_TRY_STATEMENT
   const Abc::MetaData &md = object.getMetaData();
   if(AbcG::IXform::matches(md)) {
      return AbcG::IXform(object,Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(AbcG::IPolyMesh::matches(md)) {
      return AbcG::IPolyMesh(object,Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(AbcG::ICurves::matches(md)) {
      return AbcG::ICurves(object,Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(AbcG::INuPatch::matches(md)) {
      return AbcG::INuPatch(object,Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(AbcG::IPoints::matches(md)) {
      return AbcG::IPoints(object,Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(AbcG::ISubD::matches(md)) {
      return AbcG::ISubD(object,Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(AbcG::ICamera::matches(md)) {
      return AbcG::ICamera(object,Abc::kWrapExisting).getSchema().getTimeSampling();

   // NEW
   } else if(AbcG::IFaceSet::matches(md)) {
      return AbcG::IFaceSet(object,Abc::kWrapExisting).getSchema().getTimeSampling();
   }
   return Abc::TimeSamplingPtr();
   ALEMBIC_VALUE_CATCH_STATEMENT(Abc::TimeSamplingPtr())
}

size_t getNumSamplesFromObject(Abc::IObject object)
{
   ALEMBIC_TRY_STATEMENT
   const Abc::MetaData &md = object.getMetaData();
   if(AbcG::IXform::matches(md)) {
      return AbcG::IXform(object,Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(AbcG::IPolyMesh::matches(md)) {
      return AbcG::IPolyMesh(object,Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(AbcG::ICurves::matches(md)) {
      return AbcG::ICurves(object,Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(AbcG::INuPatch::matches(md)) {
      return AbcG::INuPatch(object,Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(AbcG::IPoints::matches(md)) {
      return AbcG::IPoints(object,Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(AbcG::ISubD::matches(md)) {
      return AbcG::ISubD(object,Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(AbcG::ICamera::matches(md)) {
      return AbcG::ICamera(object,Abc::kWrapExisting).getSchema().getNumSamples();

   //NEW
   } else if(AbcG::IFaceSet::matches(md)) {
      return AbcG::IFaceSet(object,Abc::kWrapExisting).getSchema().getNumSamples();
   }
   return 0;
   ALEMBIC_VALUE_CATCH_STATEMENT(0)
}*/

static PyObject * iObject_getIdentifier(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   return Py_BuildValue("s",object->mObject->getFullName().c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getType(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   return Py_BuildValue("s",object->mObject->getMetaData().get("schema").c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getSampleTimes(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   Abc::TimeSamplingPtr ts = getTimeSamplingFromObject(*(object->mObject));
   if(ts)
   {
      const std::vector <Abc::chrono_t> & times = ts->getStoredTimes();
      PyObject * tuple = PyTuple_New(times.size());
      for(size_t i=0;i<times.size();i++)
         PyTuple_SetItem(tuple,i,Py_BuildValue("f",(float)times[i]));
      return tuple;
   }
   return Py_BuildValue("s","unsupported");
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getNbStoredSamples(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   return Py_BuildValue("i",(int)getNumSamplesFromObject(*(object->mObject)) );
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getMetaData(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
      iObject * object = (iObject*)self;
      Abc::ICompoundProperty compound = getCompoundFromObject(*object->mObject);
      if (!compound.valid() || compound.getPropertyHeader( ".metadata" ) == NULL )  // create an empty Meta Data with empty strings
      {
         PyObject * tuple = PyTuple_New(20);
         for (int i = 0; i < 20; ++i)
            PyTuple_SetItem(tuple,i,Py_BuildValue("s",""));
         return tuple;
      }

      Abc::IStringArrayProperty metaDataProp = Abc::IStringArrayProperty( compound, ".metadata" );
      Abc::StringArraySamplePtr metaDataPtr = metaDataProp.getValue(0);

      PyObject * tuple = PyTuple_New(20);    // needs to be exactly 20
      size_t i = 0;
      for(;i<metaDataPtr->size(); ++i)  // put the meta data in the tuple
         PyTuple_SetItem(tuple,i,Py_BuildValue("s",metaDataPtr->get()[i].c_str()));

      for (; i < 20; ++i)               // fill the rest with empty strings
         PyTuple_SetItem(tuple,i,Py_BuildValue("s",""));

      return tuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getPropertyNames(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   Abc::ICompoundProperty compound = getCompoundFromObject(*object->mObject);
   if(!compound.valid())
      return PyTuple_New(0);

   PyObject * tuple = NULL;
   if(AbcG::IXform::matches(object->mObject->getMetaData()))
   {
      for(size_t i=0;i<compound.getNumProperties();i++)
      {
         std::string propNameStr = compound.getPropertyHeader(i).getName();
         if(propNameStr == ".xform" || propNameStr == ".vals")
         {
            tuple = PyTuple_New(1);
            PyTuple_SetItem(tuple,0,Py_BuildValue("s",propNameStr.c_str()));
            break;
         }
      }
   }
   else
   {
      tuple = PyTuple_New(compound.getNumProperties());
      for(size_t i=0;i<compound.getNumProperties();i++) 
      {
         PyTuple_SetItem(tuple,i,Py_BuildValue("s",compound.getPropertyHeader(i).getName().c_str()));
      }
   }

   if(tuple == NULL)
      tuple = PyTuple_New(0);
   return tuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getProperty(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   Abc::ICompoundProperty compound = getCompoundFromObject(*object->mObject);
   if(!compound.valid())
      return PyTuple_New(0);

   char * propName = NULL;
   if(!PyArg_ParseTuple(args, "s", &propName))
   {
      PyErr_SetString(getError(), "No property name specified!");
      return NULL;
   }

   // special case xform prop
   if(AbcG::IXform::matches(object->mObject->getMetaData()))
   {
      std::string propNameStr(propName);
      if(propNameStr == ".xform" || propNameStr == ".vals")
      {
         return iXformProperty_new(*object->mObject);
      }
      else
      {
         std::string msg;
         msg.append("Property '");
         msg.append(propName);
         msg.append("' not found on '");
         msg.append(object->mObject->getFullName());
         msg.append("'!");
         PyErr_SetString(getError(), msg.c_str());
         return NULL;
      }
   }

   return iProperty_new(*object->mObject,propName);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getTsIndex(PyObject * self, PyObject * args)
{
   iObject *obj = (iObject*)self;
   if (obj->tsIndex == -1)
   {
      ALEMBIC_TRY_STATEMENT
         Abc::TimeSamplingPtr ts_ptr = getTimeSamplingFromObject(*obj->mObject);
         const int nb_ts = ((iArchive*)obj->mArchive)->mArchive->getNumTimeSamplings();
         for (int i = 0; i < nb_ts; ++i)
         {
            Abc::TimeSamplingPtr ts = ((iArchive*)obj->mArchive)->mArchive->getTimeSampling((boost::uint32_t)i);
            if (ts == ts_ptr)
            {
               obj->tsIndex = i;
               break;
            }
         }
      ALEMBIC_PYOBJECT_CATCH_STATEMENT
   }
   return Py_BuildValue("i",(int)obj->tsIndex);
}

static PyMethodDef iObject_methods[] = {
   {"getIdentifier", (PyCFunction)iObject_getIdentifier, METH_NOARGS, "Returns the identifier linked to this object."},
   {"getMetaData", (PyCFunction)iObject_getMetaData, METH_NOARGS, "Returns the string array storing the metadata, if it exists on this object. Otherwise it returns an empty tuple."},
   {"getType", (PyCFunction)iObject_getType, METH_NOARGS, "Returns the type of this object. Usually encodes the schema name inside of Alembic.IO."},
   {"getSampleTimes", (PyCFunction)iObject_getSampleTimes, METH_NOARGS, "Returns the TimeSampling this object is linked to."},
   {"getNbStoredSamples", (PyCFunction)iObject_getNbStoredSamples, METH_NOARGS, "Returns the actual number of stored samples."},
   {"getPropertyNames", (PyCFunction)iObject_getPropertyNames, METH_NOARGS, "Returns a string list of all property names below this object."},
   {"getProperty", (PyCFunction)iObject_getProperty, METH_VARARGS, "Returns an input property (iProperty/iCompoundProperty/iXformProperty) for the given propertyName string."},
   {"getTsIndex", (PyCFunction)iObject_getTsIndex, METH_NOARGS, "Returns time sampling index used by this object."},
   {NULL, NULL}
};
static PyObject * iObject_getAttr(PyObject * self, char * attrName)
{
   return Py_FindMethod(iObject_methods, self, attrName);
}

static void iObject_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   // delete the object
   iObject * object = (iObject *)self;
   delete(object->mObject);
   object->mObject = NULL;
   PyObject_FREE(object);
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject iObject_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "iObject",                        // tp_name
  sizeof(iObject),                  // tp_basicsize
  0,                                // tp_itemsize
  (destructor)iObject_delete,       // tp_dealloc
  0,                                // tp_print
  (getattrfunc)iObject_getAttr,     // tp_getattr
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
  "This is the input object type. It provides access to all of the objects data, including basic data such as name + type as well as object based data such as propertynames and properties.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  iObject_methods,             /* tp_methods */
};

PyObject * iObject_new(Abc::IObject in_Object, void *in_Archive)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = PyObject_NEW(iObject, &iObject_Type);
   if (object != NULL)
   {
      object->mObject = new Abc::IObject(in_Object,Abc::kWrapExisting);
      object->mArchive = in_Archive;
      object->tsIndex = -1;
   }
   return (PyObject *)object;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_iObject(PyObject *module)
{
  return register_object(module, iObject_Type, "iObject");
}


