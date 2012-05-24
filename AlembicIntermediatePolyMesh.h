#ifndef __ALEMBIC_INTERMEDIATE_POLYMESH_H__
#define __ALEMBIC_INTERMEDIATE_POLYMESH_H__

//#include "Foundation.h"
#include "Alembic.h"

typedef std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> facesetmap_vec;
typedef std::map<int, facesetmap_vec> facesetmap;
typedef std::map<int, facesetmap_vec>::iterator facesetmap_it;
typedef std::map<int, facesetmap_vec>::const_iterator facesetmap_cit;
typedef std::pair<int, facesetmap_vec> facesetmap_insert_pair;
typedef std::pair<facesetmap_it, bool> facesetmap_ret_pair;

typedef std::map<int, std::string> matnamemap;
//typedef std::pair<int, std::string> matnamemap_insert_pair;

class AlembicIntermediatePolyMesh
{
public:

	AlembicIntermediatePolyMesh():nLargestMatId(0)
	{}

	Alembic::Abc::Box3d bbox;

	std::vector<Alembic::Abc::V3f> posVec;

    std::vector<Alembic::Abc::N3f> normalVec;
    std::vector<Alembic::Abc::uint32_t> normalIndexVec;//will have size 0 if not using indexed normals

	std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceCountVec;
	std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceIndicesVec;  

	std::vector<Alembic::Abc::V2f> mUvVec;
	std::vector<Alembic::Abc::uint32_t> mUvIndexVec;//will have size 0 if not using indexed UVs

	std::vector<Alembic::Abc::uint32_t> mMatIdIndexVec;
	matnamemap mMatNamesMap;
	facesetmap mFaceSetsMap;

   //std::vector<Alembic::Abc::V3f> mBindPoseVec;
   //std::vector<Alembic::Abc::V3f> mVelocitiesVec;
   //std::vector<float> mRadiusVec;
  
	LONG sampleCount;//TODO: do I need this?

	//TODO: add method to setup sizes for multiple merges

	bool mergeWith(const AlembicIntermediatePolyMesh& srcMesh);

	int nLargestMatId;//used for merge the material IDs

};


#endif