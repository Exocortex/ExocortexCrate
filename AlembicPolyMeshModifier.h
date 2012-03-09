/**********************************************************************
 *<
	FILE: Convert.h

	DESCRIPTION: Convert To type modifiers

	CREATED BY: Steve Anderson 

	HISTORY:

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __ALEMBIC_POLYMESH_MODIFIER__H
#define __ALEMBIC_POLYMESH_MODIFIER__H

#include "Foundation.h"  
#include "MNMath.h"
#include "PolyObj.h"
#include "resource.h"
#include "surf_api.h"
#include <string>
#include "AlembicDefinitions.h"

// can be generated via gencid.exe in the help folder of the 3DS Max.
#define EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID	Class_ID(0x45365012, 0x60984267)

#define REF_PBLOCK 0
#define REF_PBLOCK1 1
#define REF_PBLOCK2 2

TCHAR *GetString(int id);

extern ClassDesc *GetAlembicPolyMeshModifierDesc();

// Alembic data fill flags specific to polymesh
const unsigned int ALEMBIC_DATAFILL_POLYMESH_STATIC_IMPORT = ALEMBIC_DATAFILL_VERTEX|ALEMBIC_DATAFILL_FACELIST|ALEMBIC_DATAFILL_NORMALS;
const unsigned int ALEMBIC_DATAFILL_POLYMESH_STATIC_UPDATE = ALEMBIC_DATAFILL_VERTEX;
const unsigned int ALEMBIC_DATAFILL_POLYMESH_TOPO_IMPORT = ALEMBIC_DATAFILL_VERTEX|ALEMBIC_DATAFILL_FACELIST|ALEMBIC_DATAFILL_NORMALS;
const unsigned int ALEMBIC_DATAFILL_POLYMESH_TOPO_UPDATE = ALEMBIC_DATAFILL_VERTEX|ALEMBIC_DATAFILL_FACELIST|ALEMBIC_DATAFILL_NORMALS;

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


extern int AlembicImport_PolyMesh(const std::string &file, const std::string &identifier, alembic_importoptions &options);

extern HINSTANCE hInstance;


#endif	// __ALEMBIC_POLYMESH_MODIFIER__H