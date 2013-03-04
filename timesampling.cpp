#include "timesampling.h"

static PyMethodDef TS_methods[] =
{
	//{"getIdentifier", (PyCFunction)iObject_getIdentifier, METH_NOARGS, "Returns the identifier linked to this object."},
	{NULL, NULL}
};

static PyObject * TS_getAttr(PyObject * self, char * attrName)
{
   return Py_FindMethod(TS_methods, self, attrName);
}

static void TS_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   // delete the object
   EA_TimeSampling *object = (EA_TimeSampling*)self;
   object->tsampling.~TimeSampling();	// call the destructor!
   PyObject_FREE(object);
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject TS_Type =
{
	PyObject_HEAD_INIT(&PyType_Type)
	0,                                // op_size
	"TimeSampling",                        // tp_name
	sizeof(EA_TimeSampling),                  // tp_basicsize
	0,                                // tp_itemsize
	(destructor)TS_delete,       // tp_dealloc
	0,                                // tp_print
	(getattrfunc)TS_getAttr,     // tp_getattr
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
	TS_methods,             /* tp_methods */
};


PyObject * TimeSamplingCopy(const AbcG::TimeSampling &tsampling)
{
	ALEMBIC_TRY_STATEMENT
	EA_TimeSampling* TS = PyObject_NEW(EA_TimeSampling, &TS_Type);

    return (PyObject*)TS;
   	ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

PyObject * TimeSampling_new(PyObject* self, PyObject* args)
{

}

bool register_object_TS(PyObject *module)
{
	return register_object(module, TS_Type, "TimeSampling");
}

