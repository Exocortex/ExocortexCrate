#ifndef __ALEMBIC_CAMERA_MODIFIER__H
#define __ALEMBIC_CAMERA_MODIFIER__H

#include "Foundation.h"
#include "AlembicMax.h"
#include "resource.h"
#include "AlembicDefinitions.h"
#include "AlembicNames.h"

// Alembic Functions
typedef struct _alembic_importoptions alembic_importoptions;

int AlembicImport_Camera(const std::string &path, Alembic::AbcGeom::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);

#endif	// __ALEMBIC_CAMERA_MODIFIER__H
