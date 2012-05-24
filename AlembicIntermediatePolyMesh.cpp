#include "AlembicIntermediatePolyMesh.h"
#include "ExocortexCoreServicesAPI.h"



bool AlembicIntermediatePolyMesh::mergeWith(const AlembicIntermediatePolyMesh& srcMesh)
{
	AlembicIntermediatePolyMesh& destMesh = *this;

	//TODO: watch out for the mixing of indexed attributes with nonindexed attributes

	if(	(destMesh.normalIndexVec.size() == 0 && srcMesh.normalIndexVec.size() > 0) ||
		(destMesh.normalIndexVec.size() > 0 && srcMesh.normalIndexVec.size() == 0)) {
		ESS_LOG_INFO( "Error: can't mix indexed normals and nonindexed normals." );
		return false; //don't allow the mixing of indexed normals with nonindexed normals
	}

	if( (destMesh.mUvIndexVec.size() == 0 && srcMesh.mUvIndexVec.size() > 0) || 
		(destMesh.mUvIndexVec.size() > 0 && srcMesh.mUvIndexVec.size() == 0)) {
		ESS_LOG_INFO( "Error: can't mix indexed UVs and nonindexed UVs." );
		return false; //don't allow the mixing of indexed UVs with nonindexed UVs
	}

	destMesh.bbox.extendBy(srcMesh.bbox);

	Alembic::Abc::uint32_t amountToOffsetSrcPosIndicesBy = (Alembic::Abc::uint32_t)destMesh.posVec.size();

	for(int i=0; i<srcMesh.posVec.size(); i++){
		destMesh.posVec.push_back( srcMesh.posVec[i] );
	}


	Alembic::Abc::uint32_t amountToOffsetSrcNormalIndicesBy = (Alembic::Abc::uint32_t)destMesh.normalVec.size();

	for(int i=0; i<srcMesh.normalVec.size(); i++){
		destMesh.normalVec.push_back( srcMesh.normalVec[i] );
	}

	for(int i=0; i<srcMesh.normalIndexVec.size(); i++){
		destMesh.normalIndexVec.push_back( srcMesh.normalIndexVec[i] + amountToOffsetSrcNormalIndicesBy );
	}


	for(int i=0; i<srcMesh.mFaceCountVec.size(); i++){
		destMesh.mFaceCountVec.push_back( srcMesh.mFaceCountVec[i]);
	}

	for(int i=0; i<srcMesh.mFaceIndicesVec.size(); i++){
		destMesh.mFaceIndicesVec.push_back( srcMesh.mFaceIndicesVec[i] + amountToOffsetSrcPosIndicesBy );
	}


	Alembic::Abc::uint32_t amountToOffsetSrcUvIndicesBy = (Alembic::Abc::uint32_t)destMesh.mUvVec.size();

	for(int i=0; i<srcMesh.mUvVec.size(); i++){
		destMesh.mUvVec.push_back( srcMesh.mUvVec[i] ); 
	}

	for(int i=0; i<srcMesh.mUvIndexVec.size(); i++){
		destMesh.mUvIndexVec.push_back( srcMesh.mUvIndexVec[i] + amountToOffsetSrcUvIndicesBy );
	}




	//if(nLargestMatId == 0){ //merge has never been run before, so reassign the dest matIDs as well
	//	for ( facesetmap_it it=destMesh.mFaceSetsMap.begin(); it != destMesh.mFaceSetsMap.end(); it++)
	//	{
	//		it->first = nLargestMatId; 
	//		nLargestMatId++;
	//	}
	//}

	////reassign the src matIDs, and append this map to the dest map
	//for ( facesetmap_cit it=srcMesh.mFaceSetsMap.begin(); it != srcMesh.mFaceSetsMap.end(); it++)
	//{
	//	destMesh.mFaceSetsMap.insert(facesetmap_insert_pair(nLargestMatId, it->second));
	//	nLargestMatId++;
	//}

	////the names have the same order after merge, so just append one vector to the other
	//for(int i=0; i<srcMesh.mMatNames.size(); i++){
	//	destMesh.mMatNames.push_back( srcMesh.mMatNames[i] );
	//}

	//mMatIdIndexVec.resize(destMesh.mFaceSetsMap.size());

	////now rebuilt the matID index array
	//for ( facesetmap_it it=destMesh.mFaceSetsMap.begin(); it != destMesh.mFaceSetsMap.end(); it++)
	//{
	//	int matId = it->first; 
	//	facesetmap_vec& faceSetVec = it->second;

	//	for(int i=0; i<faceSetVec.size(); i++)
	//	{
	//		mMatIdIndexVec[faceSetVec[i]] = matId;
	//	}
	//}


	return true;
}