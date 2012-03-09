#ifndef __ALEMBIC_SIMPLE_SPLINE__H
#define __ALEMBIC_SIMPLE_SPLINE__H

#include "Foundation.h"
#include "MNMath.h"
#include "resource.h"
#include "surf_api.h"
#include "AlembicDefinitions.h"

// can be generated via gencid.exe in the help folder of the 3DS Max.
#define EXOCORTEX_ALEMBIC_SIMPLE_SPLINE_ID    Class_ID(0xa720ab9, 0x7efc73de)

#define REF_PBLOCK 0

extern ClassDesc *GetAlembicSimpleSplineDesc();

// Alembic Functions
typedef struct _alembic_importoptions alembic_importoptions;
extern int AlembicImport_Shape(const std::string &file, const std::string &identifier, alembic_importoptions &options);

#endif // __ALEMBIC_SIMPLE_SPLINE__H

