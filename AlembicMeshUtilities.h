#ifndef __ALEMBIC_MESH_UTILITY__H
#define __ALEMBIC_MESH_UTILITY__H

#include "Foundation.h"   
#include "AlembicMax.h"
#include "resource.h"
#include "AlembicDefinitions.h"
#include "utility.h"

// Alembic Functions

typedef struct _alembic_fillmesh_options
{
    Alembic::AbcGeom::IObject *pIObj;
	std::string fileName;
	std::string identifier;
	
	MNMesh *pMNMesh;
    TimeValue dTicks;
    AlembicDataFillFlags nDataFillFlags;
	float fVertexAlpha;

    _alembic_fillmesh_options()
    {
        pIObj = NULL;
        pMNMesh = NULL;
        dTicks = 0;
        nDataFillFlags = 0;
		fVertexAlpha = 1.0f;
    }
} alembic_fillmesh_options;

void	AlembicImport_FillInPolyMesh(alembic_fillmesh_options &options);
int AlembicImport_PolyMesh(const std::string &path, Alembic::AbcGeom::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);
bool	AlembicImport_IsPolyObject(Alembic::AbcGeom::IPolyMeshSchema::Sample &polyMeshSample);




/*

const MCHAR* PointCacheAssetAccessor::GetPath() const	{
	const MCHAR *fname;
	Interval iv;
	mPointCache->pblock->GetValue(pb_cache_file,0,fname,iv);
	return fname;
}


void PointCacheAssetAccessor::SetPath(const MSTR& aNewPath)	{
	mPointCache->pblock->SetValue(pb_cache_file, 0, aNewPath.data());
}

int PointCacheAssetAccessor::GetAssetType() const	{
	return IAssetAccessor::kAnimationAsset;
}*/


/*
class AlembicPathAccessor : public IAssetAccessor	{
public:

	AlembicPathAccessor(Modifier* pModifier) : pModifier(pModifier) {}

	virtual MaxSDK::AssetManagement::AssetType GetAssetType() const	{ return MaxSDK::AssetManagement::kBitmapAsset; }

	// path accessor functions
	virtual MaxSDK::AssetManagement::AssetUser GetAsset() const 	{
		pModifier->GetParamBlockByID( 0 )->GetAssetUser( GetParamIdByName( "path" ) );
	}
	virtual bool SetAsset(const MaxSDK::AssetManagement::AssetUser& aNewAsset) {
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( "path" ), aNewAsset );
		return true;
	}

protected:
	Modifier* pModifier;
};*/



#endif	// __ALEMBIC_MESH_UTILITY__H
