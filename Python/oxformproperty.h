#ifndef _PYTHON_ALEMBIC_OXFORMPROPERTY_H_
#define _PYTHON_ALEMBIC_OXFORMPROPERTY_H_

#include "iproperty.h"
#include "oobject.h"

struct oXformMembers {
  AbcG::OXformSchema mXformSchema;
  AbcG::XformSample mSample;
};

typedef struct {
  PyObject_HEAD void *mArchive;
  size_t mMaxNbSamples;
  oXformMembers *mMembers;
} oXformProperty;

PyObject *oXformProperty_new(oObjectPtr in_casted, void *in_Archive,
                             boost::uint32_t tsIndex);
void oXformProperty_deletePointers(oXformProperty *prop);

bool register_object_oXformProperty(PyObject *module);

#endif
