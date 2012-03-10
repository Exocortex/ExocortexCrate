/**********************************************************************
 *<
	FILE: Convert.h

	DESCRIPTION: Convert To type modifiers

	CREATED BY: Steve Anderson 

	HISTORY:

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __ALEMBIC_MESH_UTILITY__H
#define __ALEMBIC_MESH_UTILITY__H

#include "Foundation.h"  
#include "MNMath.h"
#include "PolyObj.h"
#include "resource.h"
#include "surf_api.h"
#include <string>
#include "AlembicDefinitions.h"



// Alembic Functions

typedef struct _alembic_fillmesh_options
{
    Alembic::AbcGeom::IObject *pIObj;
    //TriObject *pTriObj;
	Mesh *pMesh;

    //PolyObject *pPolyObj;
	MNMesh *pMNMesh;
    TimeValue dTicks;
    AlembicDataFillFlags nDataFillFlags;

    _alembic_fillmesh_options()
    {
        pIObj = NULL;
        pMesh = NULL;
        pMNMesh = NULL;
        dTicks = 0;
        nDataFillFlags = 0;
    }
} alembic_fillmesh_options;

void AlembicImport_FillInPolyMesh(alembic_fillmesh_options &options);
int AlembicImport_PolyMesh(const std::string &file, const std::string &identifier, alembic_importoptions &options);


#endif	// __ALEMBIC_MESH_UTILITY__H
