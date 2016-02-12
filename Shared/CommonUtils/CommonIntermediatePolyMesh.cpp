#include "CommonIntermediatePolyMesh.h"

bool CommonIntermediatePolyMesh::mergeWith(
    const CommonIntermediatePolyMesh& srcMesh)
{
  ESS_PROFILE_FUNC();
  CommonIntermediatePolyMesh& destMesh = *this;

  // TODO: watch out for the mixing of indexed attributes with nonindexed
  // attributes

  // if(	(destMesh.normalIndexVec.size() == 0 && srcMesh.normalIndexVec.size()
  // > 0) ||
  //	(destMesh.normalIndexVec.size() > 0 && srcMesh.normalIndexVec.size() ==
  //0)) {
  //	ESS_LOG_INFO( "Error: can't mix indexed normals and nonindexed normals."
  //);
  //	return false; //don't allow the mixing of indexed normals with
  //nonindexed normals
  //}

  // if( (destMesh.mUvIndexVec.size() == 0 && srcMesh.mUvIndexVec.size() > 0) ||
  //	(destMesh.mUvIndexVec.size() > 0 && srcMesh.mUvIndexVec.size() == 0)) {
  //	ESS_LOG_INFO( "Error: can't mix indexed UVs and nonindexed UVs." );
  //	return false; //don't allow the mixing of indexed UVs with nonindexed
  //UVs
  //}

  destMesh.bbox.extendBy(srcMesh.bbox);

  Abc::uint32_t amountToOffsetSrcPosIndicesBy =
      (Abc::uint32_t)destMesh.posVec.size();

  for (int i = 0; i < srcMesh.posVec.size(); i++) {
    destMesh.posVec.push_back(srcMesh.posVec[i]);
  }

  for (int i = 0; i < srcMesh.mVelocitiesVec.size(); i++) {
    destMesh.mVelocitiesVec.push_back(srcMesh.mVelocitiesVec[i]);
  }

  Abc::uint32_t amountToOffsetSrcNormalIndicesBy =
      (Abc::uint32_t)destMesh.mIndexedNormals.values.size();

  for (int i = 0; i < srcMesh.mIndexedNormals.values.size(); i++) {
    destMesh.mIndexedNormals.values.push_back(
        srcMesh.mIndexedNormals.values[i]);
  }

  for (int i = 0; i < srcMesh.mIndexedNormals.indices.size(); i++) {
    destMesh.mIndexedNormals.indices.push_back(
        srcMesh.mIndexedNormals.indices[i] + amountToOffsetSrcNormalIndicesBy);
  }

  Abc::uint32_t amountToOffsetFaceIdBy =
      (Abc::uint32_t)destMesh.mFaceCountVec.size();

  for (int i = 0; i < srcMesh.mFaceCountVec.size(); i++) {
    destMesh.mFaceCountVec.push_back(srcMesh.mFaceCountVec[i]);
  }

  for (int i = 0; i < srcMesh.mFaceIndicesVec.size(); i++) {
    destMesh.mFaceIndicesVec.push_back(srcMesh.mFaceIndicesVec[i] +
                                       amountToOffsetSrcPosIndicesBy);
  }

  std::map<std::string, int> uniqueUVNamesMap;
  std::vector<std::string> uniqueUVNames;
  for (int i = 0; i < destMesh.mIndexedUVSet.size(); i++) {
    std::string name = destMesh.mIndexedUVSet[i].name;
    if (uniqueUVNamesMap.find(name) == uniqueUVNamesMap.end()) {
      uniqueUVNamesMap.insert(std::map<std::string, int>::value_type(name, 1));
      uniqueUVNames.push_back(name);
    }
  }
  for (int i = 0; i < srcMesh.mIndexedUVSet.size(); i++) {
    std::string name = srcMesh.mIndexedUVSet[i].name;
    if (uniqueUVNamesMap.find(name) == uniqueUVNamesMap.end()) {
      uniqueUVNamesMap.insert(std::map<std::string, int>::value_type(name, 1));
      uniqueUVNames.push_back(name);
    }
  }

  for (int i = 0; i < uniqueUVNames.size(); i++) {
    IndexedUVs* pDestIndexedUVs = NULL;
    IndexedUVs const* pSrcIndexedUVs = NULL;
    std::string name = uniqueUVNames[i];
    for (int j = 0; j < srcMesh.mIndexedUVSet.size(); j++) {
      if (name.compare(srcMesh.mIndexedUVSet[j].name) == 0) {
        pSrcIndexedUVs = &(srcMesh.mIndexedUVSet[j]);
      }
    }
    for (int j = 0; j < destMesh.mIndexedUVSet.size(); j++) {
      if (name.compare(destMesh.mIndexedUVSet[j].name) == 0) {
        pDestIndexedUVs = &(destMesh.mIndexedUVSet[j]);
      }
    }

    if (pSrcIndexedUVs == NULL && pDestIndexedUVs == NULL) {
      // This shoudn't happen, but lets be safe
      continue;
    }

    // there is no map with that name to copy to, so create a new set
    // a uv index is required for each vertex
    if (pDestIndexedUVs == NULL) {
      IndexedUVs newUVs;
      newUVs.name = name;
      destMesh.mIndexedUVSet.push_back(newUVs);
      pDestIndexedUVs =
          &(destMesh.mIndexedUVSet[destMesh.mIndexedUVSet.size() - 1]);
      for (int k = 0; k < destMesh.mFaceIndicesVec.size(); k++) {
        pDestIndexedUVs->indices.push_back(0);
      }
    }

    if (pSrcIndexedUVs != NULL) {
      int sourceUVOffset = (int)pDestIndexedUVs->values.size();
      for (int k = 0; k < pSrcIndexedUVs->indices.size(); k++) {
        pDestIndexedUVs->indices.push_back(pSrcIndexedUVs->indices[k] +
                                           sourceUVOffset);
      }
      for (int k = 0; k < pSrcIndexedUVs->values.size(); k++) {
        pDestIndexedUVs->values.push_back(pSrcIndexedUVs->values[k]);
      }
    }
    else {
      for (int k = 0; k < srcMesh.mFaceIndicesVec.size(); k++) {
        pDestIndexedUVs->indices.push_back(0);
      }
    }
  }

  // for(FaceSetMap::const_iterator it=srcMesh.mFaceSets.begin(); it !=
  // srcMesh.mFaceSets.end(); it++){
  //   FaceSetStruct& faceSet = destMesh.mFaceSets[it->first];
  //   for(int i=0; i<it->second.faceIds.size(); i++){
  //      faceSet.faceIds.push_back(amountToOffsetFaceIdBy +
  //      it->second.faceIds[i]);
  //   }
  //}

  ////now rebuilt the matID index array
  // mMatIdIndexVec.resize(destMesh.mFaceSetsMap.size());
  // for ( facesetmap_it it=destMesh.mFaceSetsMap.begin(); it !=
  // destMesh.mFaceSetsMap.end(); it++)
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

//
// void CommonIntermediatePolyMesh::clear()
//{
//   //a safer way to clear.
//   *this = CommonIntermediatePolyMesh();
//
// //  bbox = Abc::Box3d();
//
//	//posVec.clear();
//	//mFaceCountVec.clear();
//	//mFaceIndicesVec.clear();
//
// //  mVelocitiesVec.clear();
//
// //  mIndexedNormals.name = std::string();
//	//mIndexedNormals.indices.clear();
// //  mIndexedNormals.values.clear();
//	//mIndexedUVSet.clear();
//	//
// //  //mMatIdIndexVec.clear();
//
// //  mFaceSets.clear();
// //  mUvOptionsVec.clear();
// //  mBindPoseVec.clear();
// //  bGeomApprox = 0;
//}