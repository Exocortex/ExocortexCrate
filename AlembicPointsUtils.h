#ifndef _ALEMBIC_POINTSUTILS_H_
#define _ALEMBIC_POINTSUTILS_H_


#include <vector>

struct particleMeshData{
	BOOL bNeedDelete;
	Mesh* pMesh;
	Mtl* pMtl;
	AnimHandle animHandle;//needed to uniquely identifiy particle groups
	Matrix3 meshTM;
};

class ExoNullView;
class IParticleObjectExt;
class IParticleGroup;
class IPFRender;
class particleGroupInterface
{
public:

	ExoNullView* m_pNullView;

	typedef std::map<INode*, int> groupParticleCountT;
	//I use this std::map to keep to obtain iteration index for each particle group.
	groupParticleCountT groupParticleCount;

	IParticleObjectExt* m_pParticlesExt;
	Object* m_pSystemObject;
	INode* m_pSystemNode;

	IParticleGroup* m_pCurrParticleGroup;
	int m_nCurrParticleGroupId;
	::IObject* m_pCurrGroupContainer;
	::INode* m_pCurrGroupSystem;
	Mtl* m_pCurrGroupMtl;
	IPFRender* m_pCurrRender;
	TimeValue m_currTicks;

	particleGroupInterface( IParticleObjectExt* particlesExt, Object* obj, INode* node, ExoNullView* pNullView ):
		m_pParticlesExt(particlesExt), m_pSystemObject(obj), m_pSystemNode(node), m_pNullView(pNullView),
		m_pCurrParticleGroup(NULL), m_nCurrParticleGroupId(-1),
		m_pCurrGroupContainer(NULL), m_pCurrGroupSystem(NULL), m_pCurrGroupMtl(NULL), m_pCurrRender(NULL)
	{}

	bool setCurrentParticle(TimeValue ticks, int i);
	bool getCurrentParticleMeshTM(Matrix3& meshTM);
	Mesh* getCurrentParticleMesh(BOOL& bNeedDelete);
	int getCurrentMtlId();
};

void getParticleSystemRenderMeshes(TimeValue ticks, Object* obj, INode* node, std::vector<particleMeshData>& meshes);
class IntermediatePolyMesh3DSMax;
struct materialsMergeStr;
class AlembicWriteJob;
bool getParticleSystemMesh(TimeValue ticks, Object* obj, INode* node, IntermediatePolyMesh3DSMax* mesh, 
						   materialsMergeStr* pMatMerge, AlembicWriteJob * mJob, int nNumSamples, bool bEnableVelocityExport);




#endif
