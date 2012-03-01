/**********************************************************************
 *<
	FILE: AlembicXFormCtrl.h

	DESCRIPTION:

	CREATED BY: Tom Hudson

	HISTORY:

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#ifndef __AlembicXFormCtrl__H
#define __AlembicXFormCtrl__H

#include "Max.h"
#include "resource.h"

#define ALEMBICXFORMCTRL_REF_PBLOCK 0

extern ClassDesc* GetAlembicXFormCtrlClassDesc();

TCHAR *GetString(int id);
extern HINSTANCE hInstance;

int AlembicImport_XForm(const std::string &file, const std::string &identifier, alembic_importoptions &options);

#endif
