#ifndef __ALEMBIC_METADATA_UTILS__H
#define __ALEMBIC_METADATA_UTILS__H

#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include "AlembicMax.h"

class AlembicObject;

void importMetadata(Alembic::AbcGeom::IObject& iObj);
void SaveMetaData(INode* node, AlembicObject* object);

#endif
