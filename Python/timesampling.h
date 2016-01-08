#ifndef _PYTHON_ALEMBIC_TS_H_
#define _PYTHON_ALEMBIC_TS_H_

typedef struct {
  PyObject_HEAD AbcA::TimeSamplingPtr tsampling;
} EA_TimeSampling;

PyObject* TimeSamplingCopy(const AbcA::TimeSamplingPtr tsampling);
PyObject* TimeSampling_new(PyObject* self, PyObject* args);

bool PyObject_TimeSampling_Check(PyObject* obj);
bool register_object_TS(PyObject* module);

#endif
