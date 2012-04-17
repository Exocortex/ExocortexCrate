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
	float fVertexAlpha;

    _alembic_fillmesh_options()
    {
        pIObj = NULL;
        pMesh = NULL;
        pMNMesh = NULL;
        dTicks = 0;
        nDataFillFlags = 0;
		fVertexAlpha = 1.0f;
    }
} alembic_fillmesh_options;

void	AlembicImport_FillInPolyMesh(alembic_fillmesh_options &options);
int AlembicImport_PolyMesh(const std::string &path, Alembic::AbcGeom::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);
bool	AlembicImport_IsPolyObject(Alembic::AbcGeom::IPolyMeshSchema::Sample &polyMeshSample);

#endif	// __ALEMBIC_MESH_UTILITY__H
