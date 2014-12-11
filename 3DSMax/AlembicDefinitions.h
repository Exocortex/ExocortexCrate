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
const unsigned int ALEMBIC_DATAFILL_MATERIALIDS = 32;
const unsigned int ALEMBIC_DATAFILL_BINDPOSE = 64;
const unsigned int ALEMBIC_DATAFILL_SPLINE_KNOTS = 128;
const unsigned int ALEMBIC_DATAFILL_ALLOCATE_UV_STORAGE = 256;
const unsigned int ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES = 512;

enum MeshTopologyType
{
    SURFACE = 1,
    POINTCACHE = 2,
    SURFACE_NORMAL = 3
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
   bool importMaterialIds;
   bool importStandins;
   bool importBboxes;
   bool failOnUnsupported;
   bool loadGeometryInTopologyModifier;
   bool attachToExisting;
   bool loadTimeControl;
   bool loadCurvesAsNurbs;
   bool loadUserProperties;
   bool enableMeshSmooth;
   bool enableInstances;
   bool enableReferences;
   VisImportOption importVisibility;
   SceneEnumProc sceneEnumProc;
   ObjectList currentSceneList;
   HelperObject *pTimeControl;
   int crateBuildNum;

public:
   _alembic_importoptions() : importNormals(false)
	, importUVs(false)
	, importMaterialIds(false)
	, importStandins(false)
	, importBboxes(false)
	, attachToExisting(false)
	, failOnUnsupported(false)
    , loadGeometryInTopologyModifier(false)
    , loadTimeControl(false)
    , loadCurvesAsNurbs(false)
    , loadUserProperties(false)
    , enableMeshSmooth(false)
    , enableInstances(false)
    , enableReferences(false)
    , importVisibility(VisImport_JustImportValue)
	, pTimeControl(NULL)
   , crateBuildNum(-1)
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