#include "extension.h"
#include "oarchive.h"
#include "oobject.h"
#include "oproperty.h"
#include "ocompoundproperty.h"
#include "oxformproperty.h"
#include "foundation.h"
#include "AlembicLicensing.h"

#include <boost/algorithm/string.hpp>
#include <cstring>
#include <iostream>

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

   oObject * objectPtr = oArchive_getObjectElement(archive,identifier);
   if(objectPtr)
   {
      //PyErr_SetString(getError(), "Object already exists!");
      //return NULL;
      return (PyObject*)objectPtr;  // NEW changed because when you get an oProperty, if it already exists, it returns it, it doesn't say there's some kind of error
   }

   // recurse to find it
   Alembic::Abc::OObject parent = archive->mArchive->getTop();
   if(parts.size() > 2)
   {
      // look for the parent in our map
      std::string parentIdentifier = "";
      for(size_t i=1;i<parts.size()-1;i++)
         parentIdentifier += "/" + parts[i];
      oObject * parentPtr = oArchive_getObjectElement(archive,parentIdentifier);
      if(!parentPtr)
      {
         PyErr_SetString(getError(), "Invalid identifier!");
         return NULL;
      }
      parent = *parentPtr->mObject;
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
      //INFO_MSG("parent.getMetaData().get(\"schema\") = " << parent.getMetaData().get("schema"));
      //INFO_MSG("type string = " << typeStr);

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

      // **** NEW
      else if(typeStr.substr(0,16) == "AbcGeom_FaceSet_")
      {
         casted.mType = oObjectType_FaceSet;
         casted.mFaceSet = new Alembic::AbcGeom::OFaceSet(parent,parts[parts.size()-1],tsIndex);
         obj = Alembic::Abc::OObject(*casted.mFaceSet,Alembic::Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,16) == "AbcGeom_NuPatch_")
      {
         casted.mType = oObjectType_NuPatch;
         casted.mNuPatch = new Alembic::AbcGeom::ONuPatch(parent,parts[parts.size()-1],tsIndex);
         obj = Alembic::Abc::OObject(*casted.mNuPatch,Alembic::Abc::kWrapExisting);
      }

      // moved at the end!! let see if it works
      else if(parent.getMetaData().get("schema").substr(0,14) != "AbcGeom_Xform_")
      {
         PyErr_SetString(getError(), "This type of object has to be put below a xform object!");
         return NULL;
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
   oArchive_registerObjectElement(archive,identifier,(oObject*)newObj);
   return newObj;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyMethodDef oArchive_methods[] = {
   {"getFileName", (PyCFunction)oArchive_getFileName, METH_NOARGS, "Returns the filename this archive is linked to."},
   {"createTimeSampling", (PyCFunction)oArchive_createTimeSampling, METH_VARARGS, "Takes in a flat list of sample times (float) and creates a new TimeSampling. Returns 1 if successful."},
   {"createObject", (PyCFunction)oArchive_createObject, METH_VARARGS, "Takes in a valid type string, an identifier string as well as an optional timeSamplingIndex. Returns the created oObject. Valid object types can be found in AppendixA of this document."},
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
   if(archive->mElements != NULL)
   {
#ifdef PYTHON_DEBUG
      printf("closing archive....\n");
#endif
      //for(int i=(int)archive->mElements->size()-1;i>=0;i--)
      for(int i=0;i<(int)archive->mElements->size();i++)
      {
         oArchiveElement & element = (*archive->mElements)[i];
         if(element.object)
         {
            element.object->mArchive = NULL;
            element.identifier.empty();
            oObject_deletePointers(element.object);
            Py_DECREF(element.object);
            element.object = NULL;
         }
         else if(element.prop)
         {
            element.prop->mArchive = NULL;
            element.identifier.empty();
            oProperty_deletePointers(element.prop);
            Py_DECREF(element.prop);
            element.prop = NULL;
         }
         else if (element.comp_prop)
         {
            element.comp_prop->mArchive = NULL;
            element.identifier.empty();
            oCompoundProperty_deletePointers(element.comp_prop);
            Py_DECREF(element.comp_prop);
            element.comp_prop = NULL;
         }
         else if(element.xform)
         {
            element.xform->mArchive = NULL;
            element.identifier.empty();
            oXformProperty_deletePointers(element.xform);
            Py_DECREF(element.xform);
            element.xform = NULL;
         }
      }

      archive->mElements->clear();
      delete(archive->mElements);
      archive->mElements = NULL;
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
  "This is the output archive. It provides methods for creating content inside the archive, including TimeSamplings as well as objects.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  oArchive_methods,             /* tp_methods */
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

      object->mElements = new oArchiveElementVec();
      gNbOArchives++;
   }
   return (PyObject *)object;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static void init_oArchiveElement(oArchiveElement &element, std::string &identifier)
{
   element.identifier = identifier;    // then set the identifier
   element.object = NULL;
   element.prop = NULL;
   element.comp_prop = NULL;
   element.xform = NULL;
}

void oArchive_registerObjectElement(oArchive * archive, std::string identifier, oObject * object)
{
   if(oArchive_getObjectElement(archive, identifier))
      return;
   oArchiveElement element;
   init_oArchiveElement(element, identifier);

   element.object = object;
   archive->mElements->push_back(element);
   Py_INCREF(object);
}

void oArchive_registerPropElement(oArchive * archive, std::string identifier, oProperty * prop)
{
   oArchiveElement element;
   init_oArchiveElement(element, identifier);

   element.prop = prop;
   archive->mElements->push_back(element);
   Py_INCREF(prop);
}

void oArchive_registerCompPropElement(oArchive * archive, std::string identifier, oCompoundProperty * comp_prop)
{
   oArchiveElement element;
   init_oArchiveElement(element, identifier);

   element.comp_prop = comp_prop;
   archive->mElements->push_back(element);
   Py_INCREF(comp_prop);
}

void oArchive_registerXformElement(oArchive * archive, std::string identifier, oXformProperty * xform)
{
   if(oArchive_getXformElement(archive,identifier))
      return;
   oArchiveElement element;
   init_oArchiveElement(element, identifier);

   element.xform = xform;
   archive->mElements->push_back(element);
   Py_INCREF(xform);
}

static oArchiveElement *oArchive_getArchiveElement(oArchive *archive, std::string identifier)
{
   oArchiveElementVec &elements = (*archive->mElements);
   oArchiveElementVec::iterator end = elements.end();
   for (oArchiveElementVec::iterator beg = elements.begin(); beg != end; ++beg)
   {
      if (beg->identifier == identifier)
         return &(*beg);   
   }
   return NULL;
}

oObject * oArchive_getObjectElement(oArchive * archive, std::string identifier)
{
   oArchiveElement *element = oArchive_getArchiveElement(archive, identifier);
   return element ? element->object : NULL;
}

oProperty * oArchive_getPropElement(oArchive * archive, std::string identifier)
{
   oArchiveElement *element = oArchive_getArchiveElement(archive, identifier);
   return element ? element->prop : NULL;
}

oCompoundProperty * oArchive_getCompPropElement(oArchive * archive, std::string identifier)
{
   oArchiveElement *element = oArchive_getArchiveElement(archive, identifier);
   return element ? element->comp_prop : NULL;
}

oXformProperty * oArchive_getXformElement(oArchive * archive, std::string identifier)
{
   oArchiveElement *element = oArchive_getArchiveElement(archive, identifier);
   return element ? element->xform : NULL;
}

bool register_object_oArchive(PyObject *module)
{
  return register_object(module, oArchive_Type, "oArchive");
}

