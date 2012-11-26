#ifndef __MESH_UTILITIES_H
#define __MESH_UTILITIES_H

#include "CommonAlembic.h"

template<class T>
class IndexedValues {
public:
	std::string							name;
	std::vector<T>						values;
	std::vector<AbcA::uint32_t>	indices;

	typedef AbcA::uint32_t index_type;
	typedef T value_type;

	IndexedValues() {
	}
};

typedef IndexedValues<Abc::N3f> IndexedNormals;
typedef IndexedValues<Abc::V2f> IndexedUVs;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicMeshBase
{
protected:
	Alembic::Abc::IInt32ArrayProperty faceCountProp;

	AlembicMeshBase(void): isTopoDynamic(false) {}
public:
	bool isTopoDynamic;		// value can be computed in function pointCache!
	virtual bool pointCache(void) = 0;
	//virtual bool topoDynamic(void) = 0;
};

typedef boost::shared_ptr<AlembicMeshBase> AlembicMeshBasePtr;
AlembicMeshBasePtr createAlembicMesh(Alembic::AbcGeom::IObject *pIObj, bool isMesh);
///////////////////////////////////////////////////////////////////////////////////////////////////////////

bool isAlembicMeshValid( Alembic::AbcGeom::IObject *pIObj );
bool isAlembicMeshNormals( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshPositions( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshUVWs( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshTopoDynamic( Alembic::AbcGeom::IObject *pIObj );
bool isAlembicMeshTopology( Alembic::AbcGeom::IObject *pIObj );
bool isAlembicMeshPointCache( Alembic::AbcGeom::IObject *pIObj );

int validateAlembicMeshTopo(std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> faceCounts,
							std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> faceIndices,
							const std::string& meshName);

bool getIndexAndValues( Alembic::Abc::Int32ArraySamplePtr faceIndices, Alembic::AbcGeom::IV2fGeomParam& param, AbcA::index_t sampleIndex, 
					   std::vector<Imath::V2f>& outputValues, std::vector<AbcA::uint32_t>& outputIndices );
bool getIndexAndValues( Alembic::Abc::Int32ArraySamplePtr faceIndices, Alembic::AbcGeom::IN3fGeomParam& param, AbcA::index_t sampleIndex,
					   std::vector<Imath::V3f>& outputValues, std::vector<AbcA::uint32_t>& outputIndices );


void saveIndexedUVs( AbcG::OPolyMeshSchema& meshSchema, AbcG::OPolyMeshSchema::Sample& meshSample,
					AbcG::OV2fGeomParam::Sample &uvSample, std::vector<AbcG::OV2fGeomParam>& uvParams,
					unsigned int animatedTs, int numSamples, std::vector<IndexedUVs>& indexUVSet );

#endif // __MESH_UTILITIES_H