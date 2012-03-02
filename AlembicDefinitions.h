#ifndef __ALEMBICDEFINITIONS_H__
#define __ALEMBICDEFINITIONS_H__

#include "SceneEnumProc.h"
#include "ObjectList.h"

// Alembic Data Fill Bit Flags
typedef unsigned int AlembicDataFillFlags;
const unsigned int ALEMBIC_DATAFILL_VERTEX = 1;
const unsigned int ALEMBIC_DATAFILL_FACELIST = 2; 
const unsigned int ALEMBIC_DATAFILL_NORMALS = 4;
const unsigned int ALEMBIC_DATAFILL_UVS = 8;
const unsigned int ALEMBIC_DATAFILL_BOUNDINGBOX = 16;
const unsigned int ALEMBIC_DATAFILL_FACESETS = 32;
const unsigned int ALEMBIC_DATAFILL_BINDPOSE = 64;

enum MeshTopologyType
{
    SURFACE = 1,
    NORMAL = 2,
    NORMAL_SURFACE = 3
};

enum VisImportOption
{
    VisImport_JustImportValue = 1,
    VisImport_ConnectedControllers = 2 
};

enum alembic_return_code
{
	alembic_success = 0,
	alembic_invalidarg,
	alembic_failure,
};

typedef struct _alembic_importoptions
{
   bool importNormals;
   bool importUVs;
   bool importClusters;
   bool importStandins;
   bool importBboxes;
   bool attachToExisting;
   VisImportOption importVisibility;
   SceneEnumProc sceneEnumProc;
   ObjectList currentSceneList;

public:
   _alembic_importoptions() : importNormals(false)
	, importUVs(false)
	, importClusters(false)
	, importStandins(false)
	, importBboxes(false)
	, attachToExisting(false)
    , importVisibility(VisImport_JustImportValue)
   {
   }
} alembic_importoptions;

typedef struct _alembic_nodeprops
{
    std::string m_File;
	std::string m_Identifier;
    unsigned int m_UpdateDataFillFlags;
public:
    _alembic_nodeprops() : m_File("")
        , m_Identifier("")
        , m_UpdateDataFillFlags(0)
    {
    }
} alembic_nodeprops;

#endif 