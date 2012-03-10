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
#include <MNMath.h>
#include <iparamb2.h>
#include <PolyObj.h>
#include "resource.h"
#include "surf_api.h"
#include <string>
#include "AlembicDefinitions.h"
#include "AlembicMeshUtilities.h"

// can be generated via gencid.exe in the help folder of the 3DS Max.
#define EXOCORTEX_ALEMBIC_MESH_MODIFIER_ID	Class_ID(0x45365062, 0x60984267)

#define REF_PBLOCK 0
#define REF_PBLOCK1 1
#define REF_PBLOCK2 2

TCHAR *GetString(int id);

extern ClassDesc2 *GetAlembicMeshModifierClassDesc();

// Alembic Functions

extern HINSTANCE hInstance;


#endif	// __ALEMBIC_POLYMESH_MODIFIER__H
