
#include "AlembicPointsUtils.h"
#include <IParticleObjectExt.h>
#include <ParticleFlow/IParticleChannelID.h>
#include <ParticleFlow/IParticleChannelShape.h>
#include <ParticleFlow/IParticleContainer.h>
#include <ParticleFlow/IParticleGroup.h>
#include <ParticleFlow/IPFSystem.h>
#include <ParticleFlow/IPFActionList.h>
#include <ParticleFlow/PFSimpleOperator.h>
#include <ParticleFlow/IParticleChannels.h>
#include <ParticleFlow/IChannelContainer.h>
#include <ParticleFlow/IParticleChannelLifespan.h>
#include <ParticleFlow/IPFRender.h>
#include <ParticleFlow/IChannelContainer.h>
#include <ParticleFlow/IParticleContainer.h>
#include <map>
#include <vector>
#include "AlembicParticles.h"
#include "AlembicIntermediatePolyMesh3DSMax.h" 
#include "AlembicWriteJob.h"


IPFRender* getIPFRender(Object* obj, TimeValue ticks)
{
	IPFActionList* particleActionList = GetPFActionListInterface(obj);
	if(!particleActionList){
		return NULL;
	}

	IPFRender* particleRender = NULL;

	int numActions = particleActionList->NumActions();
	for (int p = particleActionList->NumActions()-1; p >= 0; p -= 1)
	{
		INode *pActionNode = particleActionList->GetAction(p);
		Object *pActionObj = (pActionNode != NULL ? pActionNode->EvalWorldState(ticks).obj : NULL);

		if (pActionObj == NULL){
			continue;
		}
		//MSTR name;
		//pActionObj->GetClassName(name);

		IPFRender* particleRender = GetPFRenderInterface(pActionObj);
		if(particleRender){
			return particleRender;
		}
	}

	return NULL;
}

typedef std::map<INode*, int> groupParticleCountT;

bool getParticleSystemMesh(TimeValue ticks, Object* obj, INode* node, IntermediatePolyMesh3DSMax* mesh, 
						   materialsMergeStr* pMatMerge, AlembicWriteJob * mJob, int nNumSamples)
{

	IParticleObjectExt* particlesExt = GetParticleObjectExtInterface(obj);
	particlesExt->UpdateParticles(node, ticks);

	static NullView nullView;

	//static Mesh nullMesh;
	groupParticleCountT groupParticleCount;

	for(int i=0; i< particlesExt->NumParticles(); i++){
		INode *particleGroupNode = particlesExt->GetParticleGroup(i);

		Object *particleGroupObj = (particleGroupNode != NULL) ? particleGroupNode->EvalWorldState(ticks).obj : NULL;
		IParticleGroup* particleGroup = GetParticleGroupInterface(particleGroupObj);

		if( groupParticleCount.find(particleGroupNode) == groupParticleCount.end() ){
			groupParticleCount[particleGroupNode] = 0;
		}

		::IObject *pCont = particleGroup->GetParticleContainer();
		::INode *pNode = particleGroup->GetParticleSystem();
		Mtl* pMtl = particleGroup->GetMaterial();

		pMatMerge->currUniqueHandle = Animatable::GetHandleByAnim(particleGroup->GetActionList());
		
		IPFRender* particleRender = getIPFRender(particleGroupObj, ticks);
		if(!particleRender){
			ESS_LOG_INFO("Error. Failed to obtain IPFRender interface.");
			return false;
		}

		Matrix3 meshTM;
		meshTM.IdentityMatrix();
		Interval meshValid;
		meshValid.SetInstant(ticks);

		int meshId = groupParticleCount[particleGroupNode];

		particleRender->GetMultipleRenderMeshTM(pCont, ticks, obj, pNode, nullView, meshId, meshTM, meshValid);

		BOOL bNeedDelete = FALSE;
		Mesh* pMesh = particleRender->GetMultipleRenderMesh(pCont, ticks, obj, pNode, nullView, bNeedDelete, meshId);

		groupParticleCount[particleGroupNode]++;

		if(!pMesh || (pMesh && pMesh->numVerts == 0) ){
			ESS_LOG_INFO("Error. Null render mesh. Tick: "<<ticks<<" pid: "<<i);
			//return false;
		}

		if(i == 0){
			mesh->Save(mJob, ticks, pMesh, NULL, meshTM, pMtl, nNumSamples, pMatMerge);
		}
		else{
			IntermediatePolyMesh3DSMax currPolyMesh;
			currPolyMesh.Save(mJob, ticks, pMesh, NULL, meshTM, pMtl, nNumSamples, pMatMerge);
			bool bSuccess = mesh->mergeWith(currPolyMesh);
			if(!bSuccess){
				if(bNeedDelete){
					delete pMesh;
				}
				continue;
			}
		}

		if(bNeedDelete){
			delete pMesh;
		}
	}

	return true;
}


typedef std::map<INode*, IParticleGroup*> groupMapT;

void getParticleGroups(TimeValue ticks, Object* obj, INode* node, std::vector<IParticleGroup*>& groups)
{
	IParticleObjectExt* particlesExt = GetParticleObjectExtInterface(obj);
	particlesExt->UpdateParticles(node, ticks);

	groupMapT groupMap;

	for(int i=0; i< particlesExt->NumParticles(); i++){
		INode *particleGroupNode = particlesExt->GetParticleGroup(i);

		Object *particleGroupObj = (particleGroupNode != NULL) ? particleGroupNode->EvalWorldState(ticks).obj : NULL;
		IParticleGroup *particleGroup = GetParticleGroupInterface(particleGroupObj);

		groupMap[particleGroupNode] = particleGroup;
	}

	for( groupMapT::iterator it=groupMap.begin(); it!=groupMap.end(); it++)
	{
		groups.push_back((*it).second);
	}

}


void getParticleSystemRenderMeshes(TimeValue ticks, Object* obj, INode* node, std::vector<particleMeshData>& meshes)
{

	SimpleParticle* pSimpleParticle = (SimpleParticle*) obj->GetInterface(I_SIMPLEPARTICLEOBJ);
	if(pSimpleParticle){

		NullView nullView;

		AlembicParticles* pAlembicParticles = NULL;
		if(obj->CanConvertToType(ALEMBIC_SIMPLE_PARTICLE_CLASSID))
		{
			pAlembicParticles = reinterpret_cast<AlembicParticles*>(obj->ConvertToType(ticks, ALEMBIC_SIMPLE_PARTICLE_CLASSID));
		}

		if(pAlembicParticles){

			pSimpleParticle->Update(ticks, node);
			int numParticles = pSimpleParticle->parts.points.Count();

			for(int i=0; i<numParticles; i++){

				particleMeshData mdata;
				mdata.pMtl = NULL; //TODO: where get material?
				mdata.animHandle = 0; //TODO: where to get handle?
				Interval interval = FOREVER;
				pAlembicParticles->GetMultipleRenderMeshTM_Internal(ticks, NULL, nullView, i, mdata.meshTM, interval);

				//Matrix3 objectToWorld = inode->GetObjectTM( ticks );
				//mdata.meshTM = mdata.meshTM * Inverse(objectToWorld); 

				mdata.pMesh = pAlembicParticles->GetMultipleRenderMesh(ticks, node /*system node or particle node?*/ , nullView, mdata.bNeedDelete, i);

				if(mdata.pMesh){
					meshes.push_back(mdata);
				}
			}
		}
		else{
			particleMeshData mdata;
			mdata.pMtl = NULL;//TODO: where to get material?
			mdata.animHandle = 0;//TODO: where to get handle?
			mdata.meshTM.IdentityMatrix();
			mdata.pMesh = pSimpleParticle->GetRenderMesh(ticks, node, nullView, mdata.bNeedDelete);
			if(mdata.pMesh){
				meshes.push_back(mdata);
			}
		}
	}
	else{

		IPFActionList* particleActionList = GetPFActionListInterface(obj);
		if(!particleActionList){
			return;
		}

		std::vector<IParticleGroup*> groups;
		getParticleGroups(ticks, obj, node, groups);

		IPFRender* particleRender = NULL;

		int numActions = particleActionList->NumActions();
		for (int p = particleActionList->NumActions()-1; p >= 0; p -= 1)
		{
			INode *pActionNode = particleActionList->GetAction(p);
			Object *pActionObj = (pActionNode != NULL ? pActionNode->EvalWorldState(ticks).obj : NULL);

			if (pActionObj == NULL){
				continue;
			}
			//MSTR name;
			//pActionObj->GetClassName(name);

			particleRender = GetPFRenderInterface(pActionObj);
			if(particleRender){
				break;
			}
		}

		if(particleRender){

			NullView nullView;

			for(int g=0; g<groups.size(); g++){
				
				::IObject *pCont = groups[g]->GetParticleContainer();
				::INode *pNode = groups[g]->GetParticleSystem();

				particleMeshData mdata;
				mdata.pMtl = groups[g]->GetMaterial();
				
				mdata.animHandle = Animatable::GetHandleByAnim(groups[g]->GetActionList());

				mdata.meshTM.IdentityMatrix();

				mdata.pMesh = particleRender->GetRenderMesh(pCont, ticks, obj, pNode, nullView, mdata.bNeedDelete);

				if(mdata.pMesh){
					meshes.push_back(mdata);
				}
			}
		}

	}
}

const Mesh* GetShapeMesh(IParticleObjectExt *pExt, int particleId, TimeValue ticks)
{

     // Go into the particle's action list
     INode *particleGroupNode = pExt->GetParticleGroup(particleId);
     Object *particleGroupObj = (particleGroupNode != NULL) ? particleGroupNode->EvalWorldState(ticks).obj : NULL;

	if (!particleGroupObj){
         return NULL;
	}
 
     IParticleGroup *particleGroup = GetParticleGroupInterface(particleGroupObj);
    ::IObject *pCont = particleGroup->GetParticleContainer();
    if(!pCont){	
		return NULL;
	}

    IChannelContainer* chCont = GetChannelContainerInterface(pCont);
	if (!chCont){
		return NULL;
	}

	IParticleChannelMeshR* pShapeChannel = GetParticleChannelShapeRInterface(chCont);
	if(!pShapeChannel){
		return NULL;
	}

	//IParticleChannelINodeR* pNodeChannel = GetParticleChannelRefNodeRInterface(chCont);
	//if(!pNodeChannel){
	//	return;
	//}

	//int nSize = pShapeChannel->GetValueCount();
	//if(particleId >= nSize){
	//	return;
	//}

	//bool bIsShared = pShapeChannel->IsShared();//if true, particle meshes are not unique, and we should avoid writing out copies

	int nMeshIndex = pShapeChannel->GetValueIndex(particleId);
	const Mesh* pMesh = pShapeChannel->GetValueByIndex(nMeshIndex);

	if(!pMesh){
		int n=0;
		n++;
	}

	return pMesh;
}