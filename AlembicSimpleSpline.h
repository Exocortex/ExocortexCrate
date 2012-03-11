#ifndef __ALEMBIC_SIMPLE_SPLINE__H
#define __ALEMBIC_SIMPLE_SPLINE__H

#include "Foundation.h"
#include "MNMath.h"
#include "resource.h"
#include "surf_api.h"
#include "AlembicDefinitions.h"
#include <iparamb2.h>
// can be generated via gencid.exe in the help folder of the 3DS Max.

ClassDesc2 *GetAlembicSimpleSplineClassDesc();

// Alembic Functions
typedef struct _alembic_importoptions alembic_importoptions;
extern int AlembicImport_Shape(const std::string &file, const std::string &identifier, alembic_importoptions &options);

#endif // __ALEMBIC_SIMPLE_SPLINE__H

