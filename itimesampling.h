#ifndef _PYTHON_ALEMBIC_ITIMESAMPING_H_
   #define _PYTHON_ALEMBIC_ITIMESAMPING_H_

   #include "foundation.h"

   typedef struct
   {
      PyObject_HEAD
      Alembic::Abc::TimeSamplingPtr ts_ptr;
      int tsIndex;   // always useful to have!
   } iTimeSampling;

   PyObject * iTimeSampling_new(Alembic::Abc::TimeSamplingPtr ts_ptr, int tsIndex);

   bool register_object_iTimeSampling(PyObject *module);

#endif

