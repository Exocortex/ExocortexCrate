#include "stdafx.h"
#include "extension.h"
#include "iarchive.h"
#include "oarchive.h"
#include "iobject.h"
#include "timesampling.h"
#include "AlembicLicensing.h"

typedef std::set<std::string> str_set;

static str_set iArchive_filenames;
bool isIArchiveOpened(std::string filename)
{
   return iArchive_filenames.find(filename) != iArchive_filenames.end();
}

static bool setIArchiveOpened(std::string filename)
{
   return iArchive_filenames.insert(filename).second;
}

static bool setIArchiveClosed(std::string filename)
{
   return iArchive_filenames.erase(filename) == 1;
}

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

#include <string>
#include <string.h>
static void recurseObjectChildren(PyObject *list, const Abc::IObject &obj)
{
	const int nbChildren = obj.getNumChildren();
	for (int i = 0; i < nbChildren; ++i)
	{
		const Abc::IObject &child = obj.getChild(i);
		const std::string &fullName = child.getFullName();
		const char *fName = fullName.c_str();
		const int len = fullName.size();

		char *newOne = new char[len+2];
		strncpy(newOne, fName, len);

		printf(":: %02d --> %s\n", len, newOne);

		PyObject *item = PyString_FromStringAndSize(newOne, len);
		delete [] newOne;
		PyList_Append( list, item);

		recurseObjectChildren(list, child);
	}
}

static PyObject * iArchive_getIdentifiers(PyObject * self, PyObject * args)
{
	ALEMBIC_TRY_STATEMENT

		iArchive * archive = (iArchive *)self;

		PyObject *list = PyList_New(0);
		recurseObjectChildren(list, archive->mArchive->getTop());

		return list;

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
   Abc::IObject obj = archive->mArchive->getTop();
   for(size_t i=1;i<parts.size();i++)
   {
      Abc::IObject child(obj,parts[i]);
      obj = child;
      if(!obj)
      {
         PyErr_SetString(getError(), "Invalid identifier!");
         return NULL;
      }
   }

   return iObject_new(obj, archive);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iArchive_getSampleTimes(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
      iArchive * archive = (iArchive *)self;
      Abc::IArchive *iarchive = archive->mArchive;
      const int nb_ts = iarchive->getNumTimeSamplings();

      PyObject *_list = PyList_New(nb_ts);
      for (int i = 0; i < nb_ts; ++i)
      {
			PyList_SetItem( _list, i, TimeSamplingCopy( *( iarchive->getTimeSampling((boost::uint32_t)i) ) ) );
      }
      return _list;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyMethodDef iArchive_methods[] = {
   {"getFileName", (PyCFunction)iArchive_getFileName, METH_NOARGS, "Returns the filename this archive is linked to."},
   {"getVersion", (PyCFunction)iArchive_getVersion, METH_NOARGS, "Returns the version of the archive loaded."},
   {"getIdentifiers", (PyCFunction)iArchive_getIdentifiers, METH_NOARGS, "Returns a flat string list of all of the identifiers available."},
   {"getObject", (PyCFunction)iArchive_getObject, METH_VARARGS, "Returns an iObject for the provided identifier string."},
   {"getSampleTimes", (PyCFunction)iArchive_getSampleTimes, METH_NOARGS, "Returns a two dimensional array of all TimeSamplings available in this file."},
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

   // NEW, remove the filename from the list
   setIArchiveClosed(object->mArchive->getName());

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
  "This is the input archive. It provides access to all of the archive's data, including TimeSamplings as well as objects.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  iArchive_methods,             /* tp_methods */
};

PyObject * iArchive_new(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   /*if(getNbOArchives() > 0)
   {
      PyErr_SetString(getError(), "Can only create iArchives if all oArchives are closed!");
      return NULL;
   }*/

   if(!HasAlembicWriterLicense())
   {
      if(getNbIArchives() > 1)
      {
         PyErr_SetString(getError(), "[ExocortexAlembic] Demo Mode: Only two open archives at a time allowed!");
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

   // NEW check if the archive is already open as an iArchive or oArchive
   if (isIArchiveOpened(fileName) || isOArchiveOpened(fileName))
   {
      PyErr_SetString(getError(), "This archive is already opened");
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
      object->mArchive = new Abc::IArchive( Alembic::AbcCoreHDF5::ReadArchive(), fileName);
      setIArchiveOpened(fileName);
      gNbIArchives++;
   }
   return (PyObject *)object;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_iArchive(PyObject *module)
{
  return register_object(module, iArchive_Type, "iArchive");
}

