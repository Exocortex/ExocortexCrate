#ifndef _PYTHON_ALEMBIC_OARCHIVE_H_
#define _PYTHON_ALEMBIC_OARCHIVE_H_

#include "foundation.h"
#include "oobject.h"
#include "oproperty.h"
#include "oxformproperty.h"

typedef std::map<std::string,oObject*> oArchiveObjectMap;
typedef oArchiveObjectMap::iterator oArchiveObjectIt;
typedef std::pair<std::string,oObject*> oArchiveObjectPair;
typedef std::map<std::string,oProperty*> oArchivePropertyMap;
typedef oArchivePropertyMap::iterator oArchivePropertyIt;
typedef std::pair<std::string,oProperty*> oArchivePropertyPair;
typedef std::map<std::string,oXformProperty*> oArchiveXformPropertyMap;
typedef oArchiveXformPropertyMap::iterator oArchiveXformPropertyIt;
typedef std::pair<std::string,oXformProperty*> oArchiveXformPropertyPair;

typedef struct {
  PyObject_HEAD
  Alembic::Abc::OArchive * mArchive;
  oArchiveObjectMap * mObjects;
  oArchivePropertyMap * mProperties;
  oArchiveXformPropertyMap * mXformProperties;
} oArchive;

PyObject * oArchive_new(PyObject * self, PyObject * args);
size_t getNbOArchives();

#endif