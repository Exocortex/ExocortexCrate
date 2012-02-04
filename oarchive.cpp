#include "extension.h"
#include "oarchive.h"
#include "oobject.h"
#include "oproperty.h"
#include "oxformproperty.h"
#include "foundation.h"
#include "AlembicLicensing.h"

#include <boost/algorithm/string.hpp>

size_t gNbOArchives = 0;
size_t getNbOArchives() { return gNbOArchives; }

static PyObject * oArchive_getFileName(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   oArchive * archive = (oArchive *)self;
   if(archive->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }
   return Py_BuildValue("s",archive->mArchive->getName().c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * oArchive_createTimeSampling(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   oArchive * archive = (oArchive *)self;
   if(archive->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }

   // parse the args
   PyObject * timesTuple = NULL;
   if(!PyArg_ParseTuple(args, "O", &timesTuple))
   {
      PyErr_SetString(getError(), "No timesTuple specified!");
      return NULL;
   }
   if(!PyTuple_Check(timesTuple) && !PyList_Check(timesTuple))
   {
      PyErr_SetString(getError(), "timesTuple argument is not a tuple!");
      return NULL;
   }
   size_t nbTimes = 0;
   if(PyTuple_Check(timesTuple))
      nbTimes = PyTuple_Size(timesTuple);
   else
      nbTimes = PyList_Size(timesTuple);

   if(nbTimes == 0)
   {
      PyErr_SetString(getError(), "timesTuple has zero length!");
      return NULL;
   }

   std::vector<Alembic::Abc::chrono_t> frames(nbTimes);
   for(size_t i=0;i<nbTimes;i++)
   {
      PyObject * item = NULL;
      if(PyTuple_Check(timesTuple))
         item = PyTuple_GetItem(timesTuple,i);
      else
         item = PyList_GetItem(timesTuple,i);
      float timeValue = 0.0f;
      if(!PyArg_Parse(item,"f",&timeValue))
      {
         PyErr_SetString(getError(), "Some item of timesTuple is not a floating point number!");
         return NULL;
      }
      frames[i] = timeValue;
   }

   if(frames.size() > 1)
   {
      double timePerCycle = frames[frames.size()-1] - frames[0];
      Alembic::Abc::TimeSamplingType samplingType((boost::uint32_t)frames.size(),timePerCycle);
      Alembic::Abc::TimeSampling sampling(samplingType,frames);
      archive->mArchive->addTimeSampling(sampling);
   }
   else
   {
      Alembic::Abc::TimeSampling sampling(1.0,frames[0]);
      archive->mArchive->addTimeSampling(sampling);
   }

   return Py_BuildValue("i",1);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * oArchive_createObject(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   oArchive * archive = (oArchive *)self;
   if(archive->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }

   // parse the args
   char * type = NULL;
   char * identifier = NULL;
   int tsIndex = 1;
   if(!PyArg_ParseTuple(args, "ss|i", &type, &identifier, &tsIndex))
   {
      PyErr_SetString(getError(), "No type, identifier and / or timeSamplingIndex specified!");
      return NULL;
   }

   // check if the timesampling is in range
   if(tsIndex < 0 || tsIndex >= (int)archive->mArchive->getNumTimeSamplings())
   {
      PyErr_SetString(getError(), "timeSamplingIndex is out of range!");
      return NULL;
   }

   // split the path
   std::string identifierStr(identifier);
   std::vector<std::string> parts;
   boost::split(parts, identifierStr, boost::is_any_of("/"));
   if(parts.size() < 2)
   {
      PyErr_SetString(getError(), "Invalid identifier!");
      return NULL;
   }

   oArchiveObjectIt it = archive->mObjects->find(identifier);
   if(it != archive->mObjects->end())
   {
      PyErr_SetString(getError(), "Object already exists!");
      return NULL;
   }

   // recurse to find it
   Alembic::Abc::OObject parent = archive->mArchive->getTop();
   if(parts.size() > 2)
   {
      // look for the parent in our map
      std::string parentIdentifier = "/";
      for(size_t i=1;i<parts.size()-1;i++)
         parentIdentifier += parts[i];
      it = archive->mObjects->find(parentIdentifier);
      if(it == archive->mObjects->end())
      {
         PyErr_SetString(getError(), "Invalid identifier!");
         return NULL;
      }
      parent = *it->second->mObject;
   }

   // now validate the type
   std::string typeStr(type);
   oObjectPtr casted;
   Alembic::Abc::OObject obj;
   if(typeStr.substr(0,14) == "AbcGeom_Xform_")
   {
      casted.mType = oObjectType_Xform;
      casted.mXform = new Alembic::AbcGeom::OXform(parent,parts[parts.size()-1],tsIndex);
      obj = Alembic::Abc::OObject(*casted.mXform,Alembic::Abc::kWrapExisting);
   }
   else
   {
      // if we are not a transform, ensure that we are nested below a xform!
      if(parent.getMetaData().get("schema").substr(0,14) != "AbcGeom_Xform_")
      {
         PyErr_SetString(getError(), "This type of object has to be put below a xform object!");
         return NULL;
      }
      // check if there is already a child below this transform
      if(parent.getNumChildren() > 0)
      {
         PyErr_SetString(getError(), "There can only be a single child below a xform object!");
         return NULL;
      }

      if(typeStr.substr(0,15) == "AbcGeom_Camera_")
      {
         casted.mType = oObjectType_Camera;
         casted.mCamera = new Alembic::AbcGeom::OCamera(parent,parts[parts.size()-1],tsIndex);
         obj = Alembic::Abc::OObject(*casted.mCamera,Alembic::Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,17) == "AbcGeom_PolyMesh_")
      {
         casted.mType = oObjectType_PolyMesh;
         casted.mPolyMesh = new Alembic::AbcGeom::OPolyMesh(parent,parts[parts.size()-1],tsIndex);
         obj = Alembic::Abc::OObject(*casted.mPolyMesh,Alembic::Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,13) == "AbcGeom_SubD_")
      {
         casted.mType = oObjectType_SubD;
         casted.mSubD = new Alembic::AbcGeom::OSubD(parent,parts[parts.size()-1],tsIndex);
         obj = Alembic::Abc::OObject(*casted.mSubD,Alembic::Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,14) == "AbcGeom_Curve_")
      {
         casted.mType = oObjectType_Curves;
         casted.mCurves = new Alembic::AbcGeom::OCurves(parent,parts[parts.size()-1],tsIndex);
         obj = Alembic::Abc::OObject(*casted.mCurves,Alembic::Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,15) == "AbcGeom_Points_")
      {
         casted.mType = oObjectType_Points;
         casted.mPoints = new Alembic::AbcGeom::OPoints(parent,parts[parts.size()-1],tsIndex);
         obj = Alembic::Abc::OObject(*casted.mPoints,Alembic::Abc::kWrapExisting);
      }
      else
      {
         PyErr_SetString(getError(), "Object type invalid!");
         return NULL;
      }
   }

   if(!obj.valid())
   {
      PyErr_SetString(getError(), "Unexpected error!");
      return NULL;
   }

   // keep a copy of each object until we delete the archive
#ifdef PYTHON_DEBUG
   printf("creating new object: '%s'\n",identifier);
#endif
   PyObject * newObj = oObject_new(obj,casted,archive);
#ifdef PYTHON_DEBUG
   printf("inserting object into map: '%s'\n",identifier);
#endif
   // manually increase the reference count
   Py_INCREF(newObj);
   archive->mObjects->insert(oArchiveObjectPair(identifier,(oObject*)newObj));
   return newObj;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyMethodDef oArchive_methods[] = {
   {"getFileName", (PyCFunction)oArchive_getFileName, METH_NOARGS},
   {"createTimeSampling", (PyCFunction)oArchive_createTimeSampling, METH_VARARGS},
   {"createObject", (PyCFunction)oArchive_createObject, METH_VARARGS},
   {NULL, NULL}
};

static PyObject * oArchive_getAttr(PyObject * self, char * attrName)
{
   return Py_FindMethod(oArchive_methods, self, attrName);
}

static void oArchive_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   // delete the archive
   oArchive * archive = (oArchive *)self;
   if(archive->mObjects != NULL)
   {
#ifdef PYTHON_DEBUG
      printf("closing archive....\n");
#endif
      for(oArchivePropertyIt it=archive->mProperties->begin(); it != archive->mProperties->end(); it++)
      {
         it->second->mArchive = NULL;
         oProperty_deletePointers(it->second);
         Py_DECREF(it->second);
#ifdef PYTHON_DEBUG
         printf("cleared archive pointer of property '%s'\n",it->first.c_str());
#endif
      }
      for(oArchiveXformPropertyIt it=archive->mXformProperties->begin(); it != archive->mXformProperties->end(); it++)
      {
         it->second->mArchive = NULL;
         oXformProperty_deletePointers(it->second);
         Py_DECREF(it->second);
#ifdef PYTHON_DEBUG
         printf("cleared archive pointer of xform property '%s'\n",it->first.c_str());
#endif
      }
      for(oArchiveObjectIt it=archive->mObjects->begin(); it != archive->mObjects->end(); it++)
      {
         it->second->mArchive = NULL;
         oObject_deletePointers(it->second);
         Py_DECREF(it->second);

#ifdef PYTHON_DEBUG
         printf("cleared archive pointer of object '%s'\n",it->first.c_str());
#endif
      }
      archive->mObjects->clear();
      delete(archive->mObjects);
      archive->mObjects = NULL;
      archive->mProperties->clear();
      delete(archive->mProperties);
      archive->mProperties = NULL;
      archive->mXformProperties->clear();
      delete(archive->mXformProperties);
      archive->mXformProperties = NULL;
      delete(archive->mArchive);
      archive->mArchive = NULL;
#ifdef PYTHON_DEBUG
      printf("closed archive.\n");
#endif
   }
   PyObject_FREE(archive);
   gNbOArchives--;
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject oArchive_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "oArchive",                       // tp_name
  sizeof(oArchive),                 // tp_basicsize
  0,                                // tp_itemsize
  (destructor)oArchive_delete,      // tp_dealloc
  0,                                // tp_print
  (getattrfunc)oArchive_getAttr,    // tp_getattr
  0,                                // tp_setattr
  0,                                // tp_compare
};

PyObject * oArchive_new(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   // parse the args
   char * fileName = NULL;
   if(!PyArg_ParseTuple(args, "s", &fileName))
   {
      PyErr_SetString(getError(), "No filename specified!");
      return NULL;
   }

   if(!HasFullLicense())
   {
      if(getNbOArchives() > 1)
      {
         PyErr_SetString(getError(), "[ExocortexAlembic] Demo Mode: Only two output archives at a time allowed!");
         return NULL;
      }
   }

   oArchive * object = PyObject_NEW(oArchive, &oArchive_Type);
   if (object != NULL)
   {
      object->mArchive = new Alembic::Abc::OArchive();
      *object->mArchive = CreateArchiveWithInfo(
         Alembic::AbcCoreHDF5::WriteArchive(),
         fileName,
         "ExocortexPythonAlembic",
         "Orchestrated by Python",
         Alembic::Abc::ErrorHandler::kThrowPolicy);
      Alembic::AbcGeom::CreateOArchiveBounds(*object->mArchive,0);

      object->mObjects = new oArchiveObjectMap();
      object->mProperties = new oArchivePropertyMap();
      object->mXformProperties = new oArchiveXformPropertyMap();
      gNbOArchives++;
   }
   return (PyObject *)object;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}
