#include "stdafx.h"
#include "extension.h"
#include "oarchive.h"
#include "iarchive.h"
#include "oobject.h"
#include "oproperty.h"
#include "ocompoundproperty.h"
#include "oxformproperty.h"
#include "AlembicLicensing.h"
#include "otimesampling.h"
#include "itimesampling.h"
#include "CommonUtilities.h"

typedef std::set<std::string> str_set;

static str_set oArchive_filenames;
bool isOArchiveOpened(std::string filename)
{
   return oArchive_filenames.find(filename) != oArchive_filenames.end();
}

static bool setOArchiveOpened(std::string filename)
{
   return oArchive_filenames.insert(filename).second;
}

static bool setOArchiveClosed(std::string filename)
{
   return oArchive_filenames.erase(filename) == 1;
}

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
         {Py_INCREF(Py_False); return Py_False;}
      }

      // parse the args
      PyObject * timesTuple = NULL;
      if(!PyArg_ParseTuple(args, "O", &timesTuple))
      {
         PyErr_SetString(getError(), "No timesTuple specified!");
         {Py_INCREF(Py_False); return Py_False;}
      }

      bool is_list = false;   
      PyObject *(*_GetItem)(PyObject*, Py_ssize_t) = PyTuple_GetItem;
      if(!PyTuple_Check(timesTuple))
      {
         if (PyList_Check(timesTuple))
         {
            _GetItem = PyList_GetItem;
            is_list = true;
         }
         else
         {
            PyErr_SetString(getError(), "timesTuple argument is not a tuple!");
            {Py_INCREF(Py_False); return Py_False;}
         }
      }

      const size_t nbTimes = is_list ? PyList_Size(timesTuple) : PyTuple_Size(timesTuple);
      if(nbTimes == 0)
      {
         PyErr_SetString(getError(), "timesTuple has zero length!");
         {Py_INCREF(Py_False); return Py_False;}
      }

      // check the list/tuple if all the items are valid!
      std::list<PyObject*> valid_obj;
      for (int i = 0; i < nbTimes; ++i)
      {
         PyObject *item = _GetItem(timesTuple, i);
         PyObject *out = NULL;

         if (PyTuple_Check(item) || PyList_Check(item))
            out = oTimeSampling_new(item);                        // construct a oTimeSampling from the list
         else if (is_iTimeSampling(item))
            out = iTimeSampling_createOTimeSampling(item);  // must be an iTimeSampling!
         else
         {
            // not valid, delete previous made PyObject
            for (std::list<PyObject*>::iterator beg = valid_obj.begin(); beg != valid_obj.end(); ++beg)
               PyObject_FREE(*beg);
            PyErr_SetString(getError(), "item should be a tuple, a list, or an iTimeSampling");
            {Py_INCREF(Py_False); return Py_False;}
         }
         valid_obj.push_back(out);
      }

      // add the time sampling!
      for (std::list<PyObject*>::iterator beg = valid_obj.begin(); beg != valid_obj.end(); ++beg)
      {
         archive->mArchive->addTimeSampling(((oTimeSampling*)*beg)->ts);
         PyObject_FREE(*beg);
      }

      {Py_INCREF(Py_True); return Py_True;}
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
      Py_INCREF(objectPtr);
      return (PyObject*)objectPtr;  // NEW changed because when you get an oProperty, if it already exists, it returns it, it doesn't say there's some kind of error
   }

   // recurse to find it
   Abc::OObject parent = archive->mArchive->getTop();
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
   Abc::OObject obj;
   if(typeStr.substr(0,14) == "AbcGeom_Xform_")
   {
      casted.mType = oObjectType_Xform;
      casted.mXform = new AbcG::OXform(parent,parts[parts.size()-1],tsIndex);
      obj = Abc::OObject(*casted.mXform,Abc::kWrapExisting);
   }
   else
   {
      // if we are not a transform, ensure that we are nested below a xform!
      //INFO_MSG("parent.getMetaData().get(\"schema\") = " << parent.getMetaData().get("schema"));
      //INFO_MSG("type string = " << typeStr);

      if(typeStr.substr(0,15) == "AbcGeom_Camera_")
      {
         casted.mType = oObjectType_Camera;
         casted.mCamera = new AbcG::OCamera(parent,parts[parts.size()-1],tsIndex);
         obj = Abc::OObject(*casted.mCamera,Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,17) == "AbcGeom_PolyMesh_")
      {
         casted.mType = oObjectType_PolyMesh;
         casted.mPolyMesh = new AbcG::OPolyMesh(parent,parts[parts.size()-1],tsIndex);
         obj = Abc::OObject(*casted.mPolyMesh,Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,13) == "AbcGeom_SubD_")
      {
         casted.mType = oObjectType_SubD;
         casted.mSubD = new AbcG::OSubD(parent,parts[parts.size()-1],tsIndex);
         obj = Abc::OObject(*casted.mSubD,Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,14) == "AbcGeom_Curve_")
      {
         casted.mType = oObjectType_Curves;
         casted.mCurves = new AbcG::OCurves(parent,parts[parts.size()-1],tsIndex);
         obj = Abc::OObject(*casted.mCurves,Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,15) == "AbcGeom_Points_")
      {
         casted.mType = oObjectType_Points;
         casted.mPoints = new AbcG::OPoints(parent,parts[parts.size()-1],tsIndex);
         obj = Abc::OObject(*casted.mPoints,Abc::kWrapExisting);
      }

      // **** NEW
      else if(typeStr.substr(0,16) == "AbcGeom_FaceSet_")
      {
         casted.mType = oObjectType_FaceSet;
         casted.mFaceSet = new AbcG::OFaceSet(parent,parts[parts.size()-1],tsIndex);
         obj = Abc::OObject(*casted.mFaceSet,Abc::kWrapExisting);
      }
      else if(typeStr.substr(0,16) == "AbcGeom_NuPatch_")
      {
         casted.mType = oObjectType_NuPatch;
         casted.mNuPatch = new AbcG::ONuPatch(parent,parts[parts.size()-1],tsIndex);
         obj = Abc::OObject(*casted.mNuPatch,Abc::kWrapExisting);
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
   PyObject * newObj = oObject_new(obj,casted,archive, tsIndex);
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
   {"createTimeSampling", (PyCFunction)oArchive_createTimeSampling, METH_VARARGS, "Takes in a flat list of sample times (float) and creates a new TimeSampling. Returns True if successful, False otherwise."},
   {"createObject", (PyCFunction)oArchive_createObject, METH_VARARGS, "Takes in a valid type string, an identifier string as well as an optional timeSamplingIndex. Returns the created oObject or, if it already exists, a reference to this oObject. Valid object types can be found in AppendixA of the documentation."},
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
   setOArchiveClosed(archive->mArchive->getName());   // NEW

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

   if(!HasAlembicWriterLicense())
   {
      if(getNbOArchives() > 1)
      {
         PyErr_SetString(getError(), "[ExocortexAlembic] Demo Mode: Only two output archives at a time allowed!");
         return NULL;
      }
   }

   // NEW check if the archive is already open as an iArchive or oArchive
   if (isIArchiveOpened(fileName) || isOArchiveOpened(fileName))
   {
      PyErr_SetString(getError(), "This archive is already opened");
      return NULL;
   }

   oArchive * object = PyObject_NEW(oArchive, &oArchive_Type);
   if (object != NULL)
   {
      object->mArchive = new Abc::OArchive();
      *object->mArchive = CreateArchiveWithInfo(
         Alembic::AbcCoreHDF5::WriteArchive(  true ),
         fileName,
         getExporterName( "Python " EC_QUOTE( crate_Python_Version ) ).c_str(),
		 getExporterFileName( "Unknown" ).c_str(),
         Abc::ErrorHandler::kThrowPolicy);
      AbcG::CreateOArchiveBounds(*object->mArchive,0);

      object->mElements = new oArchiveElementVec();
      setOArchiveOpened(fileName);
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

