#include "extension.h"
#include "oarchive.h"
#include "ocompoundproperty.h"
#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreAbstract/All.h>

static PyObject * oCompoundProperty_getName(PyObject * self, PyObject * args)
{
   oCompoundProperty * prop = (oCompoundProperty*)self;
   if(prop->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }
   ALEMBIC_TRY_STATEMENT
      return Py_BuildValue("s",prop->mBaseCompoundProperty->getName().c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *oCompoundProperty_getProperty(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
      oCompoundProperty * cprop = (oCompoundProperty*)self;
      if(cprop->mArchive == NULL)
      {
         PyErr_SetString(getError(), "Archive already closed!");
         return NULL;
      }

      char * propName = NULL;
      char * propType = NULL;
      int tsIndex = cprop->tsIndex;
      if(!PyArg_ParseTuple(args, "s|si", &propName, &propType, &tsIndex))
      {
         PyErr_SetString(getError(), "No property name and/or property type specified!");
         return NULL;
      }

      return oProperty_new(*(cprop->mBaseCompoundProperty), *(cprop->mFullName), propName, propType, tsIndex, cprop->mArchive);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *oCompoundProperty_isCompound(PyObject *self)
{
   Py_INCREF(Py_True);
   return Py_True;
}

static PyMethodDef oCompoundProperty_methods[] =
{
   {"getName", (PyCFunction)oCompoundProperty_getName, METH_NOARGS, "Returns the name of the compound property."},
   {"getProperty", (PyCFunction)oCompoundProperty_getProperty, METH_VARARGS, "Return an output property (oProperty/oCompoundProperty/oXformProperty) for the given propertyName string. If the property doesn’t exist yet, you will have to provide the optional propertyType string parameter. An optional tsIndex can be specified when the property is created. Valid property types can be found in AppendixB of this document."},
   {"isCompound", (PyCFunction)oCompoundProperty_isCompound, METH_NOARGS, "To distinguish between an oProperty and an oCompoundProperty, always returns true for oCompoundProperty."},
   {NULL, NULL}
};

static PyObject * oCompoundProperty_getAttr(PyObject * self, char * attrName)
{
   return Py_FindMethod(oCompoundProperty_methods, self, attrName);
}

void oCompoundProperty_deletePointers(oCompoundProperty * cprop)
{
   ALEMBIC_TRY_STATEMENT
      if (cprop->mBaseCompoundProperty)
      {
         delete cprop->mBaseCompoundProperty;
         cprop->mBaseCompoundProperty = NULL;
         delete cprop->mFullName;
         cprop->mFullName = NULL;
      }
   ALEMBIC_VOID_CATCH_STATEMENT
}

static void oCompoundProperty_delete(PyObject * self)
{
   oCompoundProperty *cprop = (oCompoundProperty*)self;
   oCompoundProperty_deletePointers(cprop);
   PyObject_FREE(self);
}

static PyTypeObject oCompoundProperty_Type =
{
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "oCompoundProperty",                      // tp_name
  sizeof(oCompoundProperty),                // tp_basicsize
  0,                                // tp_itemsize
  (destructor)oCompoundProperty_delete,     // tp_dealloc
  0,                                // tp_print
  (getattrfunc)oCompoundProperty_getAttr,   // tp_getattr
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
  "This is the output property. It provides methods for setting data on the property.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  oCompoundProperty_methods,             /* tp_methods */
};

PyObject * oCompoundProperty_new(Alembic::Abc::OCompoundProperty compound, std::string compoundFullName, const char * in_propName, int tsIndex, void * in_Archive)
{
   ALEMBIC_TRY_STATEMENT

      // check if we already have this property somewhere
      std::string identifier = compoundFullName;
      identifier.append("/");
      identifier.append(in_propName);

      oArchive * archive = (oArchive*)in_Archive;

      oCompoundProperty * cprop = oArchive_getCompPropElement(archive,identifier);
      if(cprop)
      {
         Py_INCREF(cprop);
         return (PyObject*)cprop;
      }

      //INFO_MSG("Creating a new oCompoundProperty");
      cprop = PyObject_NEW(oCompoundProperty, &oCompoundProperty_Type);
      cprop->mBaseCompoundProperty = NULL;
      cprop->mArchive = in_Archive;
      oArchive_registerCompPropElement(archive,identifier,cprop);
      cprop->mFullName = new std::string(identifier);
      cprop->tsIndex = tsIndex;

      const Alembic::Abc::PropertyHeader * propHeader = compound.getPropertyHeader( in_propName );
      if(propHeader != NULL)
      {
         // this property already exists
         Alembic::Abc::OBaseProperty baseProp = compound.getProperty( in_propName );
         cprop->mBaseCompoundProperty = new Alembic::Abc::OCompoundProperty( baseProp.getPtr(), 
                                                Alembic::Abc::kWrapExisting);
      }
      else
      {
         std::string propName(in_propName);
         cprop->mBaseCompoundProperty = new Alembic::Abc::OCompoundProperty(compound.getPtr(), propName);

         /* helge: this is buggy. there is a bug in alembic which doesn't use the right name
            for created compound properties. they do use the right header though. so checking
            for a name here doesn't actually work */
      }

      return (PyObject *)cprop;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_oCompoundProperty(PyObject *module)
{
  return register_object(module, oCompoundProperty_Type, "oCompoundProperty");
}


