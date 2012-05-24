#ifndef __ALEMBIC_INTERMEDIATE_POLYMESH_H__
#define __ALEMBIC_INTERMEDIATE_POLYMESH_H__

//#include "Foundation.h"
#include "Alembic.h"

typedef std::map<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> > facesetmap;
typedef std::map<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> >::iterator facesetmap_it;
typedef std::pair<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> > facesetmap_insert_pair;
typedef std::pair<facesetmap_it, bool> facesetmap_ret_pair;


class AlembicIntermediatePolyMesh
{
public:
	Alembic::Abc::Box3d bbox;

	std::vector<Alembic::Abc::V3f> posVec;

    std::vector<Alembic::Abc::N3f> normalVec;
    std::vector<Alembic::Abc::uint32_t> normalIndexVec;

	std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceCountVec;
	std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceIndicesVec;  

	std::vector<Alembic::Abc::V2f> mUvVec;
	std::vector<Alembic::Abc::uint32_t> mUvIndexVec;

	std::vector<Alembic::Abc::uint32_t> mMatIdIndexVec;
	std::vector<std::string> mMatNames;
	facesetmap mFaceSetsMap;

   //std::vector<Alembic::Abc::V3f> mBindPoseVec;
   //std::vector<Alembic::Abc::V3f> mVelocitiesVec;
   //std::vector<float> mRadiusVec;
  
	LONG sampleCount;//TODO: do I need this?

	void mergeWith(const AlembicIntermediatePolyMesh& mesh2);
};


#endif