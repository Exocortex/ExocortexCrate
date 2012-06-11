#ifndef _PYTHON_ALEMBIC_OTIMESAMPING_H_
   #define _PYTHON_ALEMBIC_OTIMESAMPING_H_

   #include "foundation.h"

   typedef Alembic::Abc::TimeSampling __TimeSampling;

   typedef struct
   {
      PyObject_HEAD
      __TimeSampling ts;
   } oTimeSampling;

   PyObject * oTimeSampling_new(Alembic::Abc::TimeSamplingPtr ts_ptr);
   PyObject * oTimeSampling_new(PyObject *time_list);

   bool register_object_oTimeSampling(PyObject *module);

#endif

