#include "oobject.h"
#include "extension.h"
#include "oarchive.h"
#include "ocompoundproperty.h"
#include "oproperty.h"
#include "stdafx.h"
//#include "ixformproperty.h"
#include "CommonUtilities.h"

Abc::OCompoundProperty getCompoundFromOObject(oObjectPtr in_Casted)
{
  ALEMBIC_TRY_STATEMENT
  switch (in_Casted.mType) {
    case oObjectType_Xform:
      return in_Casted.mXform->getSchema();
    case oObjectType_Camera:
      return in_Casted.mCamera->getSchema();
    case oObjectType_PolyMesh:
      return in_Casted.mPolyMesh->getSchema();
    case oObjectType_Curves:
      return in_Casted.mCurves->getSchema();
    case oObjectType_Points:
      return in_Casted.mPoints->getSchema();
    case oObjectType_SubD:
      return in_Casted.mSubD->getSchema();

    // NEW
    case oObjectType_FaceSet:
      return in_Casted.mFaceSet->getSchema();
    case oObjectType_NuPatch:
      return in_Casted.mNuPatch->getSchema();
  }
  return Abc::OCompoundProperty();
  ALEMBIC_VALUE_CATCH_STATEMENT(Abc::OCompoundProperty())
}

static PyObject *oObject_getIdentifier(PyObject *self, PyObject *args)
{
  ALEMBIC_TRY_STATEMENT
  oObject *object = (oObject *)self;
  if (object->mArchive == NULL) {
    PyErr_SetString(getError(), "Archive already closed!");
    return NULL;
  }
  return Py_BuildValue("s", object->mObject->getFullName().c_str());
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *oObject_getType(PyObject *self, PyObject *args)
{
  ALEMBIC_TRY_STATEMENT
  oObject *object = (oObject *)self;
  if (object->mArchive == NULL) {
    PyErr_SetString(getError(), "Archive already closed!");
    return NULL;
  }
  return Py_BuildValue("s",
                       object->mObject->getMetaData().get("schema").c_str());
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *oObject_setMetaData(PyObject *self, PyObject *args)
{
  ALEMBIC_TRY_STATEMENT
  oObject *object = (oObject *)self;
  if (object->mArchive == NULL) {
    PyErr_SetString(getError(), "Archive already closed!");
    return NULL;
  }

  // check if we have a string tuple
  // parse the args
  PyObject *metaDataTuple = NULL;
  if (!PyArg_ParseTuple(args, "O", &metaDataTuple)) {
    PyErr_SetString(getError(), "No metaDataTuple specified!");
    return NULL;
  }
  if (!PyTuple_Check(metaDataTuple) && !PyList_Check(metaDataTuple)) {
    PyErr_SetString(getError(), "metaDataTuple argument is not a tuple!");
    return NULL;
  }
  size_t nbStrings = 0;
  if (PyTuple_Check(metaDataTuple))
    nbStrings = PyTuple_Size(metaDataTuple);
  else
    nbStrings = PyList_Size(metaDataTuple);

  if (nbStrings != 20) {
    PyErr_SetString(getError(), "metaDataTuple doesn't have exactly 20 items!");
    return NULL;
  }

  std::vector<std::string> metaData(nbStrings);
  for (size_t i = 0; i < nbStrings; i++) {
    PyObject *item = NULL;
    if (PyTuple_Check(metaDataTuple))
      item = PyTuple_GetItem(metaDataTuple, i);
    else
      item = PyList_GetItem(metaDataTuple, i);
    char *itemStr = NULL;
    if (!PyArg_Parse(item, "s", &itemStr)) {
      PyErr_SetString(getError(),
                      "Some item of metaDataTuple is not a string!");
      return NULL;
    }
    metaData[i] = itemStr;
  }

#ifdef PYTHON_DEBUG
  printf("retrieving ocompound...\n");
  if (object->mObject == NULL) printf("what the heck?... NULL pointer?\n");
  printf("object name is: %s\n", object->mObject->getFullName().c_str());
#endif

  Abc::OCompoundProperty compound = getCompoundFromOObject(object->mCasted);
#ifdef PYTHON_DEBUG
  printf("ocompound retrieved.\n");
#endif
  if (!compound.valid()) return Py_BuildValue("i", 0);
  ;

#ifdef PYTHON_DEBUG
  printf("creating metadata property...\n");
#endif

  if (compound.getPropertyHeader(".metadata") != NULL) {
    PyErr_SetString(getError(), "Metadata already set!");
    return NULL;
  }

  Abc::OStringArrayProperty metaDataProperty =
      Abc::OStringArrayProperty(compound, ".metadata", compound.getMetaData(),
                                compound.getTimeSampling());

#ifdef PYTHON_DEBUG
  printf("metadata property created.\n");
#endif

  Abc::StringArraySample metaDataSample(&metaData.front(), metaData.size());
  metaDataProperty.set(metaDataSample);

  // todo existing properties
  // Abc::ArrayPropertyWriterPtr arrayPtr =
  // boost::static_pointer_cast<Abc::ArrayPropertyWriter>(compound.getProperty(
  // ".metadata" ).getPtr());
  // metaDataProperty = Abc::OStringArrayProperty(arrayPtr, Abc::kWrapExisting);

  return Py_BuildValue("i", 1);
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *oObject_getProperty(PyObject *self, PyObject *args)
{
  ALEMBIC_TRY_STATEMENT
  oObject *object = (oObject *)self;
  if (object->mArchive == NULL) {
    PyErr_SetString(getError(), "Archive already closed!");
    return NULL;
  }

  char *propName = NULL;
  char *propType = NULL;
  int tsIndex = object->tsIndex;
  if (!PyArg_ParseTuple(args, "s|si", &propName, &propType, &tsIndex)) {
    PyErr_SetString(getError(),
                    "No property name and/or property type specified!");
    return NULL;
  }

  // special case xform prop
  if (AbcG::OXform::matches(object->mObject->getMetaData())) {
    std::string propNameStr(propName);
    if (propNameStr == ".xform" || propNameStr == ".vals") {
      return oXformProperty_new(object->mCasted, object->mArchive,
                                (boost::uint32_t)tsIndex);
    }
  }

  return oProperty_new(getCompoundFromOObject(object->mCasted),
                       object->mObject->getFullName(), propName, propType,
                       tsIndex, object->mArchive);
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyMethodDef oObject_methods[] = {
    {"getIdentifier", (PyCFunction)oObject_getIdentifier, METH_NOARGS,
     "Returns the identifier linked to this object."},
    {"getType", (PyCFunction)oObject_getType, METH_NOARGS,
     "Returns the type of this object. Usually encodes the schema name inside "
     "of Alembic.IO."},
    {"setMetaData", (PyCFunction)oObject_setMetaData, METH_VARARGS,
     "Takes in a tuple of 20 strings to store as metadata. If you have less "
     "strings, make sure to fill the tuple with empty string to match the "
     "count of 20."},
    {"getProperty", (PyCFunction)oObject_getProperty, METH_VARARGS,
     "Return an output property (oProperty/oCompoundProperty/oXformProperty) "
     "for the given propertyName string. If the property doesn't exist yet, "
     "you will have to provide the optional propertyType string parameter. An "
     "optional tsIndex can be specified when the property is created. The "
     "default value of tsIndex is the time sampling index of the oObject. "
     "Valid property types can be found in AppendixB of this document."},
    {NULL, NULL}};
static PyObject *oObject_getAttr(PyObject *self, char *attrName)
{
  return Py_FindMethod(oObject_methods, self, attrName);
}

void oObject_deletePointers(oObject *object)
{
  if (object->mCasted.mXform != NULL) {
    switch (object->mCasted.mType) {
      case oObjectType_Xform: {
        object->mCasted.mXform->reset();
        delete (object->mCasted.mXform);
        object->mCasted.mXform = NULL;
        break;
      }
      case oObjectType_Camera: {
        object->mCasted.mCamera->reset();
        delete (object->mCasted.mCamera);
        object->mCasted.mCamera = NULL;
        break;
      }
      case oObjectType_PolyMesh: {
        object->mCasted.mPolyMesh->reset();
        delete (object->mCasted.mPolyMesh);
        object->mCasted.mPolyMesh = NULL;
        break;
      }
      case oObjectType_Curves: {
        object->mCasted.mCurves->reset();
        delete (object->mCasted.mCurves);
        object->mCasted.mCurves = NULL;
        break;
      }
      case oObjectType_Points: {
        object->mCasted.mPoints->reset();
        delete (object->mCasted.mPoints);
        object->mCasted.mPoints = NULL;
        break;
      }
      case oObjectType_SubD: {
        object->mCasted.mSubD->reset();
        delete (object->mCasted.mSubD);
        object->mCasted.mSubD = NULL;
        break;
      }
      // NEW
      case oObjectType_FaceSet: {
        object->mCasted.mFaceSet->reset();
        delete (object->mCasted.mFaceSet);
        object->mCasted.mFaceSet = NULL;
        break;
      }
      case oObjectType_NuPatch: {
        object->mCasted.mNuPatch->reset();
        delete (object->mCasted.mNuPatch);
        object->mCasted.mNuPatch = NULL;
        break;
      }
    }
  }
  if (object->mObject != NULL) {
    object->mObject->reset();
    delete (object->mObject);
    object->mObject = NULL;
  }
}

static void oObject_delete(PyObject *self)
{
  ALEMBIC_TRY_STATEMENT
  // delete the object
  oObject *object = (oObject *)self;
  oObject_deletePointers(object);
  object->mObject = NULL;
  PyObject_FREE(object);
  ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject oObject_Type = {
    PyObject_HEAD_INIT(&PyType_Type) 0,       // op_size
    "oObject",                                // tp_name
    sizeof(oObject),                          // tp_basicsize
    0,                                        // tp_itemsize
    (destructor)oObject_delete,               // tp_dealloc
    0,                                        // tp_print
    (getattrfunc)oObject_getAttr,             // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "This is the output object. It provides methods for creating properties "
    "below it and and write their data.", /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,                                    /* tp_richcompare */
    0,                                    /* tp_weaklistoffset */
    0,                                    /* tp_iter */
    0,                                    /* tp_iternext */
    oObject_methods,                      /* tp_methods */
};

PyObject *oObject_new(Abc::OObject in_Object, oObjectPtr in_Casted,
                      void *in_Archive, int tsIndex)
{
  ALEMBIC_TRY_STATEMENT
  oObject *object = PyObject_NEW(oObject, &oObject_Type);
  if (object != NULL) {
#ifdef PYTHON_DEBUG
    printf("creating new oObject from OObject: '%s'\n",
           in_Object.getFullName().c_str());
#endif
    object->mObject = new Abc::OObject(in_Object, Abc::kWrapExisting);
    object->mCasted = in_Casted;
    object->mArchive = in_Archive;
    object->tsIndex = tsIndex;
  }
  return (PyObject *)object;
  ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_oObject(PyObject *module)
{
  return register_object(module, oObject_Type, "oObject");
}
