#ifndef __ALEMBIC_INTERMEDIATE_POLYMESH_H__
#define __ALEMBIC_INTERMEDIATE_POLYMESH_H__

//
#include "CommonMeshUtilities.h"

typedef std::vector<AbcA::int32_t> facesetmap_vec;

struct FaceSetStruct
{
	std::string name;
	facesetmap_vec faceIds;
};


   

class CommonIntermediatePolyMesh
{
public:

   CommonIntermediatePolyMesh():bGeomApprox(0)
   {}

	Abc::Box3d bbox;

	std::vector<Abc::V3f> posVec;
	std::vector<AbcA::int32_t> mFaceCountVec;
	std::vector<AbcA::int32_t> mFaceIndicesVec;  

   std::vector<Abc::V3f> mVelocitiesVec;

	IndexedNormals mIndexedNormals;
	std::vector<IndexedUVs> mIndexedUVSet;
	
	//std::vector<Abc::uint32_t> mMatIdIndexVec;
   std::vector<FaceSetStruct> mFaceSets;

   std::vector<float> mUvOptionsVec;

   std::vector<Abc::V3f> mBindPoseVec;
   
   int bGeomApprox;

   //std::vector<float> mRadiusVec;

	bool mergeWith(const CommonIntermediatePolyMesh& srcMesh);


};


#endif