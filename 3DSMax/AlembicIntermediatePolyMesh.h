#ifndef __ALEMBIC_INTERMEDIATE_POLYMESH_H__
#define __ALEMBIC_INTERMEDIATE_POLYMESH_H__

//
#include "Alembic.h"
#include "CommonMeshUtilities.h"

typedef std::vector<AbcA::int32_t> facesetmap_vec;

struct faceSetStr {
  std::string name;
  facesetmap_vec faceIds;
  int originalMatId;
};

typedef std::map<int, faceSetStr> facesetmap;
typedef std::map<int, faceSetStr>::iterator facesetmap_it;
typedef std::map<int, faceSetStr>::const_iterator facesetmap_cit;
typedef std::pair<int, faceSetStr> facesetmap_insert_pair;
typedef std::pair<facesetmap_it, bool> facesetmap_ret_pair;

class AlembicIntermediatePolyMesh {
 public:
  // AlembicIntermediatePolyMesh():nLargestMatId(0)
  //{}

  Abc::Box3d bbox;

  std::vector<Abc::V3f> posVec;

  IndexedNormals mIndexedNormals;

  // std::vector<Abc::N3f> normalVec;
  // std::vector<Abc::uint32_t> normalIndexVec;//will have size 0 if not using
  // indexed normals

  std::vector<AbcA::int32_t> mFaceCountVec;
  std::vector<AbcA::int32_t> mFaceIndicesVec;

  // std::vector<Abc::V2f> mUvVec;
  // std::vector<Abc::uint32_t> mUvIndexVec;//will have size 0 if not using
  // indexed UVs

  std::vector<std::string> mIndexedUVNames;
  std::vector<IndexedUVs> mIndexedUVSet;

  std::vector<Abc::uint32_t> mMatIdIndexVec;
  facesetmap mFaceSetsMap;

  // std::vector<Abc::V3f> mBindPoseVec;
  std::vector<Abc::V3f> mVelocitiesVec;
  // std::vector<float> mRadiusVec;

  LONG sampleCount;  // TODO: do I need this?

  // TODO: add method to setup sizes for multiple merges

  bool mergeWith(const AlembicIntermediatePolyMesh& srcMesh);
};

#endif