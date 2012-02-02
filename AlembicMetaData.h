#ifndef _ALEMBIC_METADATA_H_
#define _ALEMBIC_METADATA_H_

#include <xsi_ref.h>

class AlembicObject;

void SaveMetaData(XSI::CRef x3dRef, AlembicObject * object);

#include "AlembicObject.h"

#endif