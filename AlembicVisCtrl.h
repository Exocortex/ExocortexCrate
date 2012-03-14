#ifndef __ALEMBIC_VISIBILITY_CONTROLLER__H
#define __ALEMBIC_VISIBILITY_CONTROLLER__H

#include "Max.h"
#include "resource.h"
#include <iparamb2.h>


ClassDesc2* GetAlembicVisCtrlClassDesc();

extern HINSTANCE hInstance;

void AlembicImport_SetupVisControl( Alembic::AbcGeom::IObject &obj, INode *pNode, alembic_importoptions &options );

#endif
