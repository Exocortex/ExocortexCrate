#ifndef _PYTHON_ALEMBIC_TS_H_
#define _PYTHON_ALEMBIC_TS_H_

	typedef struct
	{
		PyObject_HEAD
		AbcG::TimeSampling tsampling;
	} EA_TimeSampling;

	PyObject * TimeSamplingCopy(const AbcG::TimeSampling &tsampling);
	PyObject * TimeSampling_new(PyObject* self, PyObject* args);

	bool register_object_TS(PyObject *module);

#endif
