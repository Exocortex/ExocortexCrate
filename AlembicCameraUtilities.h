#ifndef __ALEMBIC_CAMERA_MODIFIER__H
#define __ALEMBIC_CAMERA_MODIFIER__H


//#include "Alembic.h"

#include "resource.h"
#include "AlembicDefinitions.h"
#include "AlembicNames.h"
#include "Utility.h"

// Alembic Functions
typedef struct _alembic_importoptions alembic_importoptions;

bool getCameraSampleVal(AbcG::ICamera& objCamera, SampleInfo& sampleInfo, AbcG::CameraSample sample, const char* name, double& sampleVal);

int AlembicImport_Camera(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);

void createCameraModifier(const std::string& path, const std::string& identifier, INode* pNode);

#endif	// __ALEMBIC_CAMERA_MODIFIER__H
