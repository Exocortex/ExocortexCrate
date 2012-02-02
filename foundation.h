#ifndef _PYTHON_ALEMBIC_FOUNDATION_H_
#define _PYTHON_ALEMBIC_FOUNDATION_H_

#include <boost/smart_ptr.hpp>
#include <boost/format.hpp>
#include <boost/variant.hpp>

#include <utility>
#include <limits>

#include <set>
#include <vector>
#include <map>
#include <list>

#include <stdexcept>
#include <exception>

#include <string>

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Python.h>

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
   

#endif