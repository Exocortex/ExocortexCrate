#include "extension.h"
#include "iarchive.h"
#include "oarchive.h"
#include "iobject.h"
#include "foundation.h"
#include "AlembicLicensing.h"

#include <boost/algorithm/string.hpp>

size_t gNbIArchives = 0;
size_t getNbIArchives() { return gNbIArchives; }

static PyObject * iArchive_getFileName(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iArchive * archive = (iArchive *)self;
   return Py_BuildValue("s",archive->mArchive->getName().c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iArchive_getVersion(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iArchive * archive = (iArchive *)self;
   return Py_BuildValue("i",(int)archive->mArchive->getArchiveVersion());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iArchive_getIdentifiers(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iArchive * archive = (iArchive *)self;

   std::vector<Alembic::Abc::IObject> objects;
   objects.push_back(archive->mArchive->getTop());
   for(size_t i=0;i<objects.size();i++)
   {
      // first, let's recurse
      for(size_t j=0;j<objects[i].getNumChildren();j++)
         objects.push_back(objects[i].getChild(j));
   }

   PyObject * tuple = PyTuple_New(objects.size()-1);

   // now let's loop over all objects
   for(size_t i=1;i<objects.size();i++)
   {
      PyTuple_SetItem(tuple,i-1,Py_BuildValue("s",objects[i].getFullName().c_str()));
   }
   return tuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iArchive_getObject(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   // parse the args
   char * identifier = NULL;
   if(!PyArg_ParseTuple(args, "s", &identifier))
   {
      PyErr_SetString(getError(), "No identifier specified!");
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

   // recurse to find it
   iArchive * archive = (iArchive *)self;
   Alembic::Abc::IObject obj = archive->mArchive->getTop();
   for(size_t i=1;i<parts.size();i++)
   {
      Alembic::Abc::IObject child(obj,parts[i]);
      obj = child;
      if(!obj)
      {
         PyErr_SetString(getError(), "Invalid identifier!");
         return NULL;
      }
   }

   return iObject_new(obj);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iArchive_getSampleTimes(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iArchive * archive = (iArchive *)self;
   PyObject * mainTuple = PyTuple_New(archive->mArchive->getNumTimeSamplings());
   for(size_t i=0;i<archive->mArchive->getNumTimeSamplings();i++)
   {
      Alembic::Abc::TimeSamplingPtr ts = archive->mArchive->getTimeSampling((boost::uint32_t)i);
      const std::vector <Alembic::Abc::chrono_t> & times = ts->getStoredTimes();
      PyObject * tuple = PyTuple_New(times.size());
      for(size_t j=0;j<times.size();j++)
         PyTuple_SetItem(tuple,j,Py_BuildValue("f",(float)times[j]));
      PyTuple_SetItem(mainTuple,i,tuple);
   }
   return mainTuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyMethodDef iArchive_methods[] = {
   {"getFileName", (PyCFunction)iArchive_getFileName, METH_NOARGS},
   {"getVersion", (PyCFunction)iArchive_getVersion, METH_NOARGS},
   {"getIdentifiers", (PyCFunction)iArchive_getIdentifiers, METH_NOARGS},
   {"getObject", (PyCFunction)iArchive_getObject, METH_VARARGS},
   {"getSampleTimes", (PyCFunction)iArchive_getSampleTimes, METH_NOARGS},
   {NULL, NULL}
};

static PyObject * iArchive_getAttr(PyObject * self, char * attrName)
{
   return Py_FindMethod(iArchive_methods, self, attrName);
}

static void iArchive_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   // delete the archive
   iArchive * object = (iArchive *)self;
   delete(object->mArchive);
   PyObject_FREE(object);
   gNbIArchives--;
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject iArchive_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "iArchive",                       // tp_name
  sizeof(iArchive),                 // tp_basicsize
  0,                                // tp_itemsize
  (destructor)iArchive_delete,      // tp_dealloc
  0,                                // tp_print
  (getattrfunc)iArchive_getAttr,    // tp_getattr
  0,                                // tp_setattr
  0,                                // tp_compare
};

PyObject * iArchive_new(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   if(getNbOArchives() > 0)
   {
      PyErr_SetString(getError(), "Can only create iArchives if all oArchives are closed!");
      return NULL;
   }

   if(!HasFullLicense())
   {
      if(getNbIArchives() > 0)
      {
         PyErr_SetString(getError(), "[ExocortexAlembic] Demo Mode: Only one open archive at a time allowed!");
         return NULL;
      }
   }

   // parse the args
   char * fileName = NULL;
   if(!PyArg_ParseTuple(args, "s", &fileName))
   {
      PyErr_SetString(getError(), "No filename specified!");
      return NULL;
   }

   // check if the filename exists
   FILE * file = fopen(fileName,"rb");
   if(file == NULL)
   {
      PyErr_SetString(getError(), "File does not exist!");
      return NULL;
   }
   fclose(file);

   iArchive * object = PyObject_NEW(iArchive, &iArchive_Type);
   if (object != NULL)
   {
      object->mArchive = new Alembic::Abc::IArchive( Alembic::AbcCoreHDF5::ReadArchive(), fileName);
      gNbIArchives++;
   }
   return (PyObject *)object;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}
