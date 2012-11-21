#ifndef _PYTHON_ALEMBIC_FOUNDATION_H_
   #define _PYTHON_ALEMBIC_FOUNDATION_H_

	#include "CommonAlembic.h"
	#ifdef _DEBUG
		#undef _DEBUG	// so python won't try to use pythonXX_d.dll!
		#include <Python.h>
		#define _DEBUG
	#else
		#include <Python.h>
	#endif

   using namespace std;

   //#define PYTHON_DEBUG 1

   #define ALEMBIC_TRY_STATEMENT try {
   #define ALEMBIC_VOID_CATCH_STATEMENT } catch (Alembic::Util::Exception e) { \
      PyErr_SetString(getError(), e.what()); \
      return; \
   }
   #define ALEMBIC_PYOBJECT_CATCH_STATEMENT } catch (Alembic::Util::Exception e) { \
      PyErr_SetString(getError(), e.what()); \
      return NULL; \
   }
   #define ALEMBIC_VALUE_CATCH_STATEMENT(value) } catch (Alembic::Util::Exception e) { \
      PyErr_SetString(getError(), e.what()); \
      return value; \
   }

   #ifdef _DEBUG
      #define INFO_MSG(msg)    EC_LOG_INFO( "[" << __FILE__ << ":" << __LINE__ << "] " << msg );
   #else
      #define INFO_MSG(msg)
   #endif   

#endif
