#include "extension.h"
#include "iobject.h"
#include "iproperty.h"
#include "ixformproperty.h"
#include "foundation.h"
#include <boost/lexical_cast.hpp>

Alembic::Abc::ICompoundProperty getCompoundFromIObject(Alembic::Abc::IObject object)
{
   ALEMBIC_TRY_STATEMENT
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema();
   }
   return Alembic::Abc::ICompoundProperty();
   ALEMBIC_VALUE_CATCH_STATEMENT(Alembic::Abc::ICompoundProperty())
}

Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::IObject object)
{
   ALEMBIC_TRY_STATEMENT
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   }
   return Alembic::Abc::TimeSamplingPtr();
   ALEMBIC_VALUE_CATCH_STATEMENT(Alembic::Abc::TimeSamplingPtr())
}

size_t getNumSamplesFromObject(Alembic::Abc::IObject object)
{
   ALEMBIC_TRY_STATEMENT
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   }
   return 0;
   ALEMBIC_VALUE_CATCH_STATEMENT(0)
}

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
   Alembic::Abc::TimeSamplingPtr ts = getTimeSamplingFromObject(*object->mObject);
   if(ts)
   {
      const std::vector <Alembic::Abc::chrono_t> & times = ts->getStoredTimes();
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
   return Py_BuildValue("i",(int)getNumSamplesFromObject(*object->mObject));
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getMetaData(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   Alembic::Abc::ICompoundProperty compound = getCompoundFromIObject(*object->mObject);
   if(!compound.valid())
      return PyTuple_New(0);
   if ( compound.getPropertyHeader( ".metadata" ) == NULL )
      return PyTuple_New(0);

   Alembic::Abc::IStringArrayProperty metaDataProp = Alembic::Abc::IStringArrayProperty( compound, ".metadata" );
   Alembic::Abc::StringArraySamplePtr metaDataPtr = metaDataProp.getValue(0);

   PyObject * tuple = PyTuple_New(metaDataPtr->size());
   for(size_t i=0;i<metaDataPtr->size();i++)
      PyTuple_SetItem(tuple,i,Py_BuildValue("s",metaDataPtr->get()[i].c_str()));
   return tuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getPropertyNames(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   Alembic::Abc::ICompoundProperty compound = getCompoundFromIObject(*object->mObject);
   if(!compound.valid())
      return PyTuple_New(0);

   PyObject * tuple;
   if(Alembic::AbcGeom::IXform::matches(object->mObject->getMetaData()))
   {
      tuple = PyTuple_New(1);
      for(size_t i=0;i<compound.getNumProperties();i++)
      {
         std::string propNameStr = compound.getPropertyHeader(i).getName();
         if(propNameStr == ".xform" || propNameStr == ".vals")
         {
            PyTuple_SetItem(tuple,0,Py_BuildValue("s",compound.getPropertyHeader(i).getName().c_str()));
            break;
         }
      }
   }
   else
   {
      tuple = PyTuple_New(compound.getNumProperties());
      for(size_t i=0;i<compound.getNumProperties();i++)
         PyTuple_SetItem(tuple,i,Py_BuildValue("s",compound.getPropertyHeader(i).getName().c_str()));
   }
   return tuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iObject_getProperty(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = (iObject*)self;
   Alembic::Abc::ICompoundProperty compound = getCompoundFromIObject(*object->mObject);
   if(!compound.valid())
      return PyTuple_New(0);

   char * propName = NULL;
   if(!PyArg_ParseTuple(args, "s", &propName))
   {
      PyErr_SetString(getError(), "No property name specified!");
      return NULL;
   }

   // special case xform prop
   if(Alembic::AbcGeom::IXform::matches(object->mObject->getMetaData()))
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

static PyMethodDef iObject_methods[] = {
   {"getIdentifier", (PyCFunction)iObject_getIdentifier, METH_NOARGS},
   {"getMetaData", (PyCFunction)iObject_getMetaData, METH_NOARGS},
   {"getType", (PyCFunction)iObject_getType, METH_NOARGS},
   {"getSampleTimes", (PyCFunction)iObject_getSampleTimes, METH_NOARGS},
   {"getNbStoredSamples", (PyCFunction)iObject_getNbStoredSamples, METH_NOARGS},
   {"getPropertyNames", (PyCFunction)iObject_getPropertyNames, METH_NOARGS},
   {"getProperty", (PyCFunction)iObject_getProperty, METH_VARARGS},
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
};

PyObject * iObject_new(Alembic::Abc::IObject in_Object)
{
   ALEMBIC_TRY_STATEMENT
   iObject * object = PyObject_NEW(iObject, &iObject_Type);
   if (object != NULL)
   {
      object->mObject = new Alembic::Abc::IObject(in_Object,Alembic::Abc::kWrapExisting);
   }
   return (PyObject *)object;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}
