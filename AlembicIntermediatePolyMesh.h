#ifndef __ALEMBIC_INTERMEDIATE_POLYMESH_H__
#define __ALEMBIC_INTERMEDIATE_POLYMESH_H__

//#include "Foundation.h"
#include "Alembic.h"


class AlembicIntermidiatePolyMesh
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

   //std::vector<Alembic::Abc::V3f> mBindPoseVec;

   //std::vector<Alembic::Abc::V3f> mVelocitiesVec;
   std::vector<Alembic::Abc::uint32_t> mMatIdIndexVec;

   //std::vector<float> mRadiusVec;
   //facesetmap mFaceSetsMap;

};


#endif