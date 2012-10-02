#ifndef __MESH_UTILITIES_H
#define __MESH_UTILITIES_H

bool isAlembicMeshValid( Alembic::AbcGeom::IObject *pIObj );
bool isAlembicMeshNormals( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshPositions( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshUVWs( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshTopoDynamic( Alembic::AbcGeom::IObject *pIObj );
bool isAlembicMeshPointCache( Alembic::AbcGeom::IObject *pIObj );


int validateAlembicMeshTopo(std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> faceCounts,
							std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> faceIndices,
							const std::string& meshName);


#endif // __MESH_UTILITIES_H