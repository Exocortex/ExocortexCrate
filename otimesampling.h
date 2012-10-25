#ifndef _PYTHON_ALEMBIC_OTIMESAMPING_H_
   #define _PYTHON_ALEMBIC_OTIMESAMPING_H_

   typedef struct
   {
      PyObject_HEAD
      Abc::TimeSampling ts;
   } oTimeSampling;

   PyObject * oTimeSampling_new(Abc::TimeSamplingPtr ts_ptr);
   PyObject * oTimeSampling_new(PyObject *time_list);

   bool register_object_oTimeSampling(PyObject *module);

#endif

