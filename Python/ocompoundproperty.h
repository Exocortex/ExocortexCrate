#ifndef _PYTHON_ALEMBIC_OCOMPOUNDPROPERTY_H_
   #define _PYTHON_ALEMBIC_OCOMPOUNDPROPERTY_H_

   #include "oproperty.h"
   #include "oobject.h"

   #include <cstring>

   typedef struct __oCompoundProperty
   {
      PyObject_HEAD
      void * mArchive;
      Abc::OCompoundProperty * mBaseCompoundProperty;
      std::string * mFullName;
      int tsIndex;      // for quick access!
   } oCompoundProperty;

   PyObject * oCompoundProperty_new(Abc::OCompoundProperty compound, std::string compoundFullName, const char * in_propName, int tsIndex, void * in_Archive);
   void oCompoundProperty_deletePointers(oCompoundProperty * prop);

   bool register_object_oCompoundProperty(PyObject *module);

#endif


