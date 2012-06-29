#ifndef _ALEMBIC_POINTSUTILS_H_
#define _ALEMBIC_POINTSUTILS_H_

#include "AlembicMax.h"
#include <vector>

struct particleMeshData{
	BOOL bNeedDelete;
	Mesh* pMesh;
	Mtl* pMtl;
	AnimHandle animHandle;//needed to uniquely identifiy particle groups
	Matrix3 meshTM;
};

void getParticleSystemRenderMeshes(TimeValue ticks, Object* obj, INode* node, std::vector<particleMeshData>& meshes);
class IntermediatePolyMesh3DSMax;
struct materialsMergeStr;
class AlembicWriteJob;
bool getParticleSystemMesh(TimeValue ticks, Object* obj, INode* node, IntermediatePolyMesh3DSMax* mesh, 
						   materialsMergeStr* pMatMerge, AlembicWriteJob * mJob, int nNumSamples);


#endif
