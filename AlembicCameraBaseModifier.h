#ifndef __ALEMBIC_CAMERA_MODIFIER__H
#define __ALEMBIC_CAMERA_MODIFIER__H

#include "Foundation.h"
#include "MNMath.h" 
#include "resource.h"
#include "surf_api.h"
#include "AlembicDefinitions.h"
#include <iparamb2.h>

// can be generated via gencid.exe in the help folder of the 3DS Max.

ClassDesc2 *GetAlembicCameraBaseModifierClassDesc();

// Alembic Functions
typedef struct _alembic_importoptions alembic_importoptions;
int AlembicImport_Camera(const std::string &file, const std::string &identifier, alembic_importoptions &options);

#endif	// __ALEMBIC_CAMERA_MODIFIER__H
