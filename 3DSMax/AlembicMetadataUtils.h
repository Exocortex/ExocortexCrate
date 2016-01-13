#ifndef __ALEMBIC_METADATA_UTILS__H
#define __ALEMBIC_METADATA_UTILS__H

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcGeom/All.h>

class AlembicObject;

void importMetadata(INode* pNode, AbcG::IObject& iObj);
void SaveMetaData(INode* node, AlembicObject* object);

#endif
