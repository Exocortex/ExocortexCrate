#include "extension.h"
#include "oarchive.h"
#include "ocompoundproperty.h"
#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreAbstract/All.h>
//#include <Alembic/Abc/Arguments.h>

static PyObject * oCompoundProperty_getName(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   oCompoundProperty * prop = (oCompoundProperty*)self;
   if(prop->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }
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
   int tsIndex = 1;
   if(!PyArg_ParseTuple(args, "s|si", &propName,&propType,&tsIndex))
   {
      PyErr_SetString(getError(), "No property name and/or property type specified!");
      return NULL;
   }

   // TODO check if that case can happen
   /*
   if(Alembic::AbcGeom::OXform::matches(object->mObject->getMetaData()))
   {
      std::string propNameStr(propName);
      if(propNameStr == ".xform" || propNameStr == ".vals")
      {
         return oXformProperty_new(object->mCasted,object->mArchive,(boost::uint32_t)tsIndex);
      }
   }
   */

   // Test if it's a compound or a normal property
   /*
   std::string new_propName(cprop->extra_path);
   new_propName.append("/");
   new_propName.append(propName);
   //*/

   //*
   //if (propType && std::strcmp(propType, "compound") == 0)
      //return oCompoundProperty_new(*(cprop->mBaseCompoundProperty), propName, "compound", tsIndex, cprop->mArchive);
   return oProperty_new(*(cprop->mBaseCompoundProperty), propName, propType, tsIndex, cprop->mArchive);
   //*/

   /*
   if (propType && std::strcmp(propType, "compound") == 0)
      return oCompoundProperty_new(*(cprop->mBaseCompoundProperty), (char*)new_propName.c_str(), "compound", tsIndex, cprop->mArchive);
   return oProperty_new(*(cprop->mBaseCompoundProperty), (char*)new_propName.c_str(), propType, tsIndex, cprop->mArchive);
   //*/
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
   {"getProperty", (PyCFunction)oCompoundProperty_getProperty, METH_VARARGS, "Return an oProperty or an oCompoundProperty for the given propertyName string. If the property doesn't exist yet, you will have to provide the optional propertyType string parameter. Valid property types can be found in AppendixB of this document."},
   {"isCompound", (PyCFunction)oCompoundProperty_isCompound, METH_NOARGS, "To distinguish between an oProperty and an oCompoundProperty, always returns true for oCompoundProperty."},
   {NULL, NULL}
};

static PyObject * oCompoundProperty_getAttr(PyObject * self, char * attrName)
{
   ALEMBIC_TRY_STATEMENT
      return Py_FindMethod(oCompoundProperty_methods, self, attrName);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

void oCompoundProperty_deletePointers(oCompoundProperty * cprop)
{
   if (cprop->mBaseCompoundProperty)
   {
      delete cprop->mBaseCompoundProperty;
      cprop->mBaseCompoundProperty = NULL;
   }
}

static void oCompoundProperty_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   oCompoundProperty *cprop = (oCompoundProperty*)self;
   oCompoundProperty_deletePointers(cprop);
   PyObject_FREE(self);
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject oCompoundProperty_Type = {
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


PyObject * oCompoundProperty_new(Alembic::Abc::OCompoundProperty compound, const char * in_propName, const char * in_propType, int tsIndex, void * in_Archive)
{
   ALEMBIC_TRY_STATEMENT

   // check if we already have this property somewhere
   std::string identifier = compound.getObject().getFullName();
   identifier.append("/");
   identifier.append(in_propName);

   INFO_MSG("identifier: " << identifier);

   oArchive * archive = (oArchive*)in_Archive;

   oCompoundProperty * cprop = oArchive_getCompPropElement(archive,identifier);
   if(cprop)
   {
      INFO_MSG("already in the archive, with name: " << (cprop->mBaseCompoundProperty->getObject().getFullName()));
      return (PyObject*)cprop;
   }

   INFO_MSG("Creating a new oCompoundProperty");
   cprop = PyObject_NEW(oCompoundProperty, &oCompoundProperty_Type);
   cprop->mBaseCompoundProperty = NULL;
   cprop->mArchive = in_Archive;
   oArchive_registerCompPropElement(archive,identifier,cprop);

   // get the compound property writer
   Alembic::Abc::CompoundPropertyWriterPtr compoundWriter = GetCompoundPropertyWriterPtr(compound);
   const Alembic::Abc::PropertyHeader * propHeader = compound.getPropertyHeader( in_propName );
   INFO_MSG("propHeader: " << (propHeader ? "True" : "False"));

   if(propHeader != NULL)
   {
      // this property already exists
      Alembic::Abc::OBaseProperty baseProp = compound.getProperty( in_propName );
      cprop->mBaseCompoundProperty = new Alembic::Abc::OCompoundProperty(
                                             boost::dynamic_pointer_cast<Alembic::Abc::CompoundPropertyWriter>(baseProp.getPtr()), 
                                             Alembic::Abc::kWrapExisting);
      INFO_MSG("EXISTING compound property name: " << (cprop->mBaseCompoundProperty->getObject().getFullName()));
   }
   else
   {
      Alembic::AbcCoreAbstract::MetaData md = compound.getMetaData();

      cprop->mBaseCompoundProperty = new Alembic::Abc::OCompoundProperty(compound.getPtr(), in_propName, md);
      INFO_MSG("NEW compound property name: " << (cprop->mBaseCompoundProperty->getObject().getFullName()));
   }

   /*
   INFO_MSG("propHeader: " << (propHeader ? "True" : "False"));
   if (propHeader != NULL)
   {
      Alembic::Abc::OBaseProperty baseProp = compound.getProperty( in_propName );
      cprop->mBaseCompoundProperty = new Alembic::Abc::OCompoundProperty(boost::dynamic_pointer_cast<Alembic::Abc::CompoundPropertyWriter>(baseProp.getPtr()), Alembic::Abc::kWrapExisting);
   }
   else
   {
      INFO_MSG("Meta data");
      Alembic::AbcCoreAbstract::MetaData md = compound.getMetaData();

      //INFO_MSG("set isGeomParam");
      md.set( "isGeomParam", "true" );      

      INFO_MSG("Writer Ptr");
      Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::CompoundPropertyWriterPtr cpwp = compound.getPtr()->createCompoundProperty(in_propName, md);
      /*if (cpwp)
      {
         if (cpwp->isCompound())
         {
            INFO_MSG("IT CREATES a compound");
         }
         else
            INFO_MSG("IT CREATES something else");
      }
      else
         INFO_MSG("IT FAILS");*
      /*
      //Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::CompoundPropertyWriterPtr cpwp = compound.getPtr()->createCompoundProperty(identifier.c_str(), md);
      cprop->mBaseCompoundProperty = new Alembic::Abc::OCompoundProperty(compound, in_propName, md, 0);
      //*
      cprop->mBaseCompoundProperty = new Alembic::Abc::OCompoundProperty(cpwp, Alembic::Abc::kWrapExisting);
   }
   //*/
   //INFO_MSG("NEW compound property name: " << (cprop->mBaseCompoundProperty->getObject().getFullName()));
   return (PyObject *)cprop;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_oCompoundProperty(PyObject *module)
{
  return register_object(module, oCompoundProperty_Type, "oCompoundProperty");
}


