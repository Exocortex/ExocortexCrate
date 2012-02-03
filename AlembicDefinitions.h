#ifndef __ALEMBICDEFINITIONS_H__
#define __ALEMBICDEFINITIONS_H__

// Alembic Data Fill Bit Flags
typedef unsigned int AlembicDataFillFlags;
const unsigned int ALEMBIC_DATAFILL_VERTEX_IMPORT = 1;
const unsigned int ALEMBIC_DATAFILL_VERTEX_UPDATE = 2;
const unsigned int ALEMBIC_DATAFILL_FACELIST = 4; 
const unsigned int ALEMBIC_DATAFILL_NORMALS = 8;
const unsigned int ALEMBIC_DATAFILL_UVS = 16;

enum MeshTopologyType
{
    NORMAL,
    SURFACE,
    NORMAL_SURFACE
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
   bool importVisibility;
   bool importStandins;
   bool importBboxes;
   bool attachToExisting;

   _alembic_importoptions() : importNormals(false)
	, importUVs(false)
	, importClusters(false)
	, importVisibility(false)
	, importStandins(false)
	, importBboxes(false)
	, attachToExisting(false)
   {
   }
} alembic_importoptions;

#endif 