#ifndef _PYTHON_ALEMBIC_OARCHIVE_H_
#define _PYTHON_ALEMBIC_OARCHIVE_H_

#include "foundation.h"
#include "oobject.h"
#include "oproperty.h"
#include "ocompoundproperty.h"
#include "oxformproperty.h"

struct oArchiveElement
{
   std::string identifier;
   oObject * object;
   oProperty * prop;
   oCompoundProperty * comp_prop;
   oXformProperty * xform;
};

typedef std::vector<oArchiveElement> oArchiveElementVec;

typedef struct {
  PyObject_HEAD
  Alembic::Abc::OArchive * mArchive;
  oArchiveElementVec * mElements;
} oArchive;

void oArchive_registerObjectElement(oArchive * archive, std::string identifier, oObject * object);
void oArchive_registerPropElement(oArchive * archive, std::string identifier, oProperty * prop);
void oArchive_registerCompPropElement(oArchive * archive, std::string identifier, oCompoundProperty * comp_prop);
void oArchive_registerXformElement(oArchive * archive, std::string identifier, oXformProperty * xform);

oObject * oArchive_getObjectElement(oArchive * archive, std::string identifier);
oProperty * oArchive_getPropElement(oArchive * archive, std::string identifier);
oCompoundProperty * oArchive_getCompPropElement(oArchive * archive, std::string identifier);
oXformProperty * oArchive_getXformElement(oArchive * archive, std::string identifier);

PyObject * oArchive_new(PyObject * self, PyObject * args);
size_t getNbOArchives();

bool register_object_oArchive(PyObject *module);

#endif


