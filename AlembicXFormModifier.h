/**********************************************************************
 *<
	FILE: Convert.h

	DESCRIPTION: Convert To type modifiers

	CREATED BY: Steve Anderson

	HISTORY:

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __ALEMBIC_XFORM_MODIFIER__H
#define __ALEMBIC_XFORM_MODIFIER__H

#include "Foundation.h"
#include "MNMath.h"
#include "PolyObj.h"
#include "iparamm2.h"
#include "resource.h"
#include "surf_api.h"

// can be generated via gencid.exe in the help folder of the 3DS Max.
#define EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID	Class_ID(0x591c09b2, 0x46912cd)

#define REF_PBLOCK 0

TCHAR *GetString(int id);

extern ClassDesc *GetAlembicXFormDesc();

// Alembic functions
extern void AlembicAddtoScene_PolyMesh();

extern HINSTANCE hInstance;


#endif	// __ALEMBIC_XFORM_MODIFIER__H