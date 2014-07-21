#ifndef _PYTHON_ALEMBIC_EXTENSION_H_
#define _PYTHON_ALEMBIC_EXTENSION_H_

#include "Python.h"

#define ABCPY_VERSION_FIRST 11000
#define ABCPY_VERSION_CURRENT 11000
#define ALEMBIC_VERSION 11000

PyObject * getError();

#ifndef EXTENSION_CALLBACK
	#ifdef _WIN32
		#pragma warning( disable : 4190 ) 
		#define EXTENSION_CALLBACK extern "C" __declspec(dllexport) void _cdecl
	#else
		#define EXTENSION_CALLBACK extern "C" void
	#endif
#endif

bool register_object(PyObject *module, PyTypeObject &type_object, const char *object_name);

#endif
