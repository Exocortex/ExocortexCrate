#ifndef _PYTHON_ALEMBIC_OCOMPOUNDPROPERTY_H_
#define _PYTHON_ALEMBIC_OCOMPOUNDPROPERTY_H_

   #include "foundation.h"
   #include "oproperty.h"
   #include "oobject.h"

   #include <cstring>

   typedef struct __oCompoundProperty
   {
      PyObject_HEAD
      void * mArchive;
      Alembic::Abc::OCompoundProperty * mBaseCompoundProperty;
   } oCompoundProperty;

   PyObject * oCompoundProperty_new(Alembic::Abc::OCompoundProperty compound, char * in_propName, char * in_propType, int tsIndex, void * in_Archive);
   void oCompoundProperty_deletePointers(oCompoundProperty * prop);

   bool register_object_oCompoundProperty(PyObject *module);

#endif
