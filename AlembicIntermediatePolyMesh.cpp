#include "AlembicIntermediatePolyMesh.h"
#include "ExocortexCoreServicesAPI.h"



bool AlembicIntermediatePolyMesh::mergeWith(const AlembicIntermediatePolyMesh& srcMesh)
{
	AlembicIntermediatePolyMesh& destMesh = *this;

	//TODO: watch out for the mixing of indexed attributes with nonindexed attributes

	//if(	(destMesh.normalIndexVec.size() == 0 && srcMesh.normalIndexVec.size() > 0) ||
	//	(destMesh.normalIndexVec.size() > 0 && srcMesh.normalIndexVec.size() == 0)) {
	//	ESS_LOG_INFO( "Error: can't mix indexed normals and nonindexed normals." );
	//	return false; //don't allow the mixing of indexed normals with nonindexed normals
	//}

	//if( (destMesh.mUvIndexVec.size() == 0 && srcMesh.mUvIndexVec.size() > 0) || 
	//	(destMesh.mUvIndexVec.size() > 0 && srcMesh.mUvIndexVec.size() == 0)) {
	//	ESS_LOG_INFO( "Error: can't mix indexed UVs and nonindexed UVs." );
	//	return false; //don't allow the mixing of indexed UVs with nonindexed UVs
	//}

	destMesh.bbox.extendBy(srcMesh.bbox);

	Abc::uint32_t amountToOffsetSrcPosIndicesBy = (Abc::uint32_t)destMesh.posVec.size();

	for(int i=0; i<srcMesh.posVec.size(); i++){
		destMesh.posVec.push_back( srcMesh.posVec[i] );
	}


	Abc::uint32_t amountToOffsetSrcNormalIndicesBy = (Abc::uint32_t)destMesh.mIndexedNormals.indices.size();

	for(int i=0; i<srcMesh.mIndexedNormals.values.size(); i++){
		destMesh.mIndexedNormals.values.push_back( srcMesh.mIndexedNormals.values[i] );
	}

	for(int i=0; i<srcMesh.mIndexedNormals.indices.size(); i++){
		destMesh.mIndexedNormals.indices.push_back( srcMesh.mIndexedNormals.indices[i] + amountToOffsetSrcNormalIndicesBy );
	}


	for(int i=0; i<srcMesh.mFaceCountVec.size(); i++){
		destMesh.mFaceCountVec.push_back( srcMesh.mFaceCountVec[i]);
	}

	for(int i=0; i<srcMesh.mFaceIndicesVec.size(); i++){
		destMesh.mFaceIndicesVec.push_back( srcMesh.mFaceIndicesVec[i] + amountToOffsetSrcPosIndicesBy );
	}


	//Abc::uint32_t amountToOffsetSrcUvIndicesBy = (Abc::uint32_t)destMesh.mUvVec.size();

	//for(int i=0; i<srcMesh.mUvVec.size(); i++){
	//	destMesh.mUvVec.push_back( srcMesh.mUvVec[i] ); 
	//}

	//for(int i=0; i<srcMesh.mUvIndexVec.size(); i++){
	//	destMesh.mUvIndexVec.push_back( srcMesh.mUvIndexVec[i] + amountToOffsetSrcUvIndicesBy );
	//}


	//merge the faceset maps
	//I assume if that if a key is common, we combine the face lists (this assumption is compatible with
	//our current method of saving out the mapIds and names)
	for ( facesetmap_cit it=srcMesh.mFaceSetsMap.begin(); it != srcMesh.mFaceSetsMap.end(); it++)
	{
		if( destMesh.mFaceSetsMap.find(it->first) == destMesh.mFaceSetsMap.end() ){ // a new key
			
			destMesh.mFaceSetsMap[it->first] = it->second;
		}
		else{// the key is common

			const facesetmap_vec& srcFaceSetVec = it->second.faceIds;
			facesetmap_vec& destFaceSetVec = destMesh.mFaceSetsMap[it->first].faceIds;

			for(int i=0; i<srcFaceSetVec.size(); i++){
				destFaceSetVec.push_back(srcFaceSetVec[i]);
			}
		}
	}

	for(int i=0; i<srcMesh.mMatIdIndexVec.size(); i++){
		destMesh.mMatIdIndexVec.push_back(srcMesh.mMatIdIndexVec[i]);
	}


	////now rebuilt the matID index array
	//mMatIdIndexVec.resize(destMesh.mFaceSetsMap.size());
	//for ( facesetmap_it it=destMesh.mFaceSetsMap.begin(); it != destMesh.mFaceSetsMap.end(); it++)
	//{
	//	int matId = it->first; 
	//	facesetmap_vec& faceSetVec = it->second.faceIds;

	//	for(int i=0; i<faceSetVec.size(); i++)
	//	{
	//		mMatIdIndexVec[faceSetVec[i]] = matId;
	//	}
	//}


	return true;
}