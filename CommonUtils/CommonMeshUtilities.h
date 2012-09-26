#ifndef __MESH_UTILITIES_H
#define __MESH_UTILITIES_H

bool isAlembicMeshValid( Alembic::AbcGeom::IObject *pIObj );
bool isAlembicMeshNormals( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshPositions( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshUVWs( Alembic::AbcGeom::IObject *pIObj, bool& isConstant );
bool isAlembicMeshTopoDynamic( Alembic::AbcGeom::IObject *pIObj );

#endif // __MESH_UTILITIES_H