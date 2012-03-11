#ifndef __ALEMBIC_CAMERA_MODIFIER__H
#define __ALEMBIC_CAMERA_MODIFIER__H

#include "Foundation.h"
#include "MNMath.h" 
#include "resource.h"
#include "surf_api.h"
#include "AlembicDefinitions.h"

// can be generated via gencid.exe in the help folder of the 3DS Max.

#define REF_PBLOCK 0

extern ClassDesc *GetAlembicCameraModifierClassDesc();

// Alembic Functions
typedef struct _alembic_importoptions alembic_importoptions;
extern int AlembicImport_Camera(const std::string &file, const std::string &identifier, alembic_importoptions &options);

#endif	// __ALEMBIC_CAMERA_MODIFIER__H
