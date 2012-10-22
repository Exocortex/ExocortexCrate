#ifndef __ALEMBIC_INTERMEDIATE_POLYMESH_H__
#define __ALEMBIC_INTERMEDIATE_POLYMESH_H__

//#include "Foundation.h"
#include "Alembic.h"

typedef std::vector<AbcA::int32_t> facesetmap_vec;

struct faceSetStr
{
	std::string name;
	facesetmap_vec faceIds;
	int originalMatId;
};

typedef std::map<int, faceSetStr> facesetmap;
typedef std::map<int, faceSetStr>::iterator facesetmap_it;
typedef std::map<int, faceSetStr>::const_iterator facesetmap_cit;
typedef std::pair<int, faceSetStr> facesetmap_insert_pair;
typedef std::pair<facesetmap_it, bool> facesetmap_ret_pair;


template<class T>
class IndexedValues {
public:
	std::string							name;
	std::vector<T>						values;
	std::vector<AbcA::uint32_t>	indices;

	IndexedValues() {
	}
};

class AlembicIntermediatePolyMesh
{
public:

	//AlembicIntermediatePolyMesh():nLargestMatId(0)
	//{}

	Abc::Box3d bbox;

	std::vector<Abc::V3f> posVec;

	IndexedValues<Abc::N3f> mIndexedNormals;
	
    //std::vector<Abc::N3f> normalVec;
    //std::vector<Abc::uint32_t> normalIndexVec;//will have size 0 if not using indexed normals

	std::vector<AbcA::int32_t> mFaceCountVec;
	std::vector<AbcA::int32_t> mFaceIndicesVec;  

	//std::vector<Abc::V2f> mUvVec;
	//std::vector<Abc::uint32_t> mUvIndexVec;//will have size 0 if not using indexed UVs

	std::vector<IndexedValues<Abc::V2f>> mIndexedUVSet;
	
	std::vector<Abc::uint32_t> mMatIdIndexVec;
	facesetmap mFaceSetsMap;

   //std::vector<Abc::V3f> mBindPoseVec;
   std::vector<Abc::V3f> mVelocitiesVec;
   //std::vector<float> mRadiusVec;
  
	LONG sampleCount;//TODO: do I need this?

	//TODO: add method to setup sizes for multiple merges

	bool mergeWith(const AlembicIntermediatePolyMesh& srcMesh);
};


#endif