#ifndef _PYTHON_ALEMBIC_ITIMESAMPING_H_
   #define _PYTHON_ALEMBIC_ITIMESAMPING_H_

   typedef struct
   {
      PyObject_HEAD
      Abc::TimeSamplingPtr ts_ptr;
      int tsIndex;   // always useful to have!
   } iTimeSampling;

   PyObject * iTimeSampling_new(Abc::TimeSamplingPtr ts_ptr, int tsIndex);
   PyObject * iTimeSampling_createOTimeSampling(PyObject * iTS);
   bool is_iTimeSampling(PyObject *obj);

   bool register_object_iTimeSampling(PyObject *module);

#endif

