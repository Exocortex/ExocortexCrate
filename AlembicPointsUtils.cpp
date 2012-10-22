
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
#include <map>
#include <vector>
#include "AlembicParticles.h"
#include "AlembicIntermediatePolyMesh3DSMax.h" 
#include "AlembicWriteJob.h"
#include "MaxSceneTimeManager.h"

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

enum splitTypeT
{	kRender_splitType_single,
	kRender_splitType_multiple,
	kRender_splitType_particle,
	kRender_splitType_num=3 
};

splitTypeT setPerParticleMeshRenderSetting(Object* obj, TimeValue ticks, splitTypeT nNewValue)
{
	if(nNewValue == kRender_splitType_num){
		return kRender_splitType_num;
	}

	splitTypeT nOldValue = kRender_splitType_num;

	IPFActionList* particleActionList = GetPFActionListInterface(obj);
	if(!particleActionList){
		return nOldValue;
	}

	PFSimpleOperator *pSimpleOperator = NULL;

	int numActions = particleActionList->NumActions();
	for (int p = particleActionList->NumActions()-1; p >= 0; p -= 1)
	{
		INode *pActionNode = particleActionList->GetAction(p);
		Object *pActionObj = (pActionNode != NULL ? pActionNode->EvalWorldState(ticks).obj : NULL);

		if (pActionObj == NULL){
			continue;
		}
		MSTR name;
		pActionObj->GetClassName(name);

		if (pActionObj->ClassID() == PFOperatorRender_Class_ID){
			pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
			break;
		}
	}

	if(pSimpleOperator){
		
		//int nNumBlocks = pSimpleOperator->NumParamBlocks();
		//for(int j=0; j<nNumBlocks; j++){
		//	IParamBlock2 *pblock = pSimpleOperator->GetParamBlockByID(j);
		//	if(pblock){
		//		int nNumParams = pblock->NumParams();
		//		for(int i=0; i<nNumParams; i++){

		//			ParamID id = pblock->IndextoID(i);
		//			MSTR name = pblock->GetLocalName(id, 0);
		//			MSTR value = pblock->GetStr(id, 0);
		//			
		//			int n=0;
		//			n++;
		//		}
		//	}
		//}

		IParamBlock2 *pblock = pSimpleOperator->GetParamBlockByID(0);
		if(pblock){
			ParamID id = pblock->IndextoID(2);
			MSTR name = pblock->GetLocalName(id, 0);
			int nSplitType = pblock->GetInt(id);
			nOldValue = (splitTypeT)nSplitType;

			TimeValue zero(0);
			pblock->SetValue(id, zero, (int)nNewValue);
		}

	}

	return nOldValue;

}
	
bool particleGroupInterface::setCurrentParticle(TimeValue ticks, int i)
{
	m_currTicks = ticks;

	INode *particleGroupNode = m_pParticlesExt->GetParticleGroup(i);

	if(!particleGroupNode){
		ESS_LOG_WARNING("Error: cound retrieve particle group.");
		return false;
	}

	Object *particleGroupObj = (particleGroupNode != NULL) ? particleGroupNode->EvalWorldState(ticks).obj : NULL;
	m_pCurrParticleGroup = GetParticleGroupInterface(particleGroupObj);

	if( groupParticleCount.find(particleGroupNode) == groupParticleCount.end() ){
		groupParticleCount[particleGroupNode] = 0;
	}
	m_nCurrParticleGroupId = groupParticleCount[particleGroupNode];
	groupParticleCount[particleGroupNode]++;

	m_pCurrGroupContainer = m_pCurrParticleGroup->GetParticleContainer();
	m_pCurrGroupSystem = m_pCurrParticleGroup->GetParticleSystem();
	m_pCurrGroupMtl = m_pCurrParticleGroup->GetMaterial();
	m_pCurrRender = getIPFRender(particleGroupObj, ticks);

	return true;
}
	
bool particleGroupInterface::getCurrentParticleMeshTM(Matrix3& meshTM)
{
	meshTM.IdentityMatrix();
	Interval meshValid;
	meshValid.SetInstant(m_currTicks);

	if(!m_pCurrRender){
		return false;
	}

	m_pCurrRender->GetMultipleRenderMeshTM(
		m_pCurrGroupContainer, m_currTicks, m_pSystemObject, m_pCurrGroupSystem, *m_pNullView, m_nCurrParticleGroupId, meshTM, meshValid);

	return true;
}

Mesh* particleGroupInterface::getCurrentParticleMesh(BOOL& bNeedDelete){
	if(!m_pCurrRender){
		return NULL;
	}

	Mesh* pMesh = m_pCurrRender->GetMultipleRenderMesh(m_pCurrGroupContainer, m_currTicks, m_pSystemObject, m_pCurrGroupSystem, *m_pNullView, bNeedDelete, m_nCurrParticleGroupId);

	if(!pMesh){
		return NULL;
	}
	if(pMesh && pMesh->numVerts == 0){
		return NULL;	
	}
	return pMesh;
}

int particleGroupInterface::getCurrentMtlId(){
	IParticleChannelIntR* chMtlIndex = GetParticleChannelMtlIndexRInterface(m_pCurrGroupContainer);
	if(chMtlIndex){
		return chMtlIndex->GetValue(m_nCurrParticleGroupId);
	}
	return -1;
}


bool getParticleSystemMesh(TimeValue ticks, Object* obj, INode* node, IntermediatePolyMesh3DSMax* mesh, 
						   materialsMergeStr* pMatMerge, AlembicWriteJob * mJob, int nNumSamples)
{
	static ExoNullView nullView;
	static const bool ENABLE_VELOCITY_EXPORT = true;

    Matrix3 nodeWorldTM = node->GetObjTMAfterWSM(ticks);
    Abc::M44d nodeWorldTrans;
    ConvertMaxMatrixToAlembicMatrix(nodeWorldTM, nodeWorldTrans);
	Abc::M44d nodeWorldTransInv = nodeWorldTrans.inverse();

	ParticleObject* pParticleObject = (ParticleObject*)obj->GetInterface(I_PARTICLEOBJ);
	//SimpleParticle* pSimpleParticle = (SimpleParticle*)obj->GetInterface(I_SIMPLEPARTICLEOBJ);
	IPFSystem* iSystem = GetPFSystemInterface(obj);
	IParticleObjectExt* particlesExt = GetParticleObjectExtInterface(obj);
	
	if(pParticleObject && !iSystem){
	// for simple particles and thinking particles, else clause applies to pFlow
	// TODO: try using particlesExt->GetRenderMeshVertexSpeed with pFlow instead of calculating the velocities manually
		
		//AlembicParticles* pAlembicParticles = NULL;
		//if(obj->CanConvertToType(ALEMBIC_SIMPLE_PARTICLE_CLASSID))
		//{
		//	pAlembicParticles = reinterpret_cast<AlembicParticles*>(obj->ConvertToType(ticks, ALEMBIC_SIMPLE_PARTICLE_CLASSID));
		//}

		//if(pAlembicParticles){

			//pParticleObject->Update(ticks, node);
			//int numParticles = pParticleObject->parts.points.Count();

			//for(int i=0; i<numParticles; i++){

			//	particleMeshData mdata;
			//	mdata.pMtl = NULL; //TODO: where get material?
			//	mdata.animHandle = 0; //TODO: where to get handle?
			//	Interval interval = FOREVER;
			//	pAlembicParticles->GetMultipleRenderMeshTM_Internal(ticks, NULL, nullView, i, mdata.meshTM, interval);

			//	//Matrix3 objectToWorld = inode->GetObjectTM( ticks );
			//	//mdata.meshTM = mdata.meshTM * Inverse(objectToWorld); 

			//	mdata.pMesh = pAlembicParticles->GetMultipleRenderMesh(ticks, node /*system node or particle node?*/ , nullView, mdata.bNeedDelete, i);

			//	if(mdata.pMesh){
			//		meshes.push_back(mdata);
			//	}
			//}
		//}
		//else{

			Mtl* pMtl = NULL;//TODO: where to get material?
			pMatMerge->currUniqueHandle = 0;//TODO: where to get handle?
			pMatMerge->bPreserveIds = true;
	
			Matrix3 meshTM;
			meshTM.IdentityMatrix();
			//Interval meshValid;
			//meshValid.SetInstant(ticks);

			BOOL bNeedDelete = FALSE;
			Mesh* pMesh = pParticleObject->GetRenderMesh(ticks, node, nullView, bNeedDelete);

			if(!pMesh || (pMesh && pMesh->numVerts == 0) ){
				ESS_LOG_INFO("Error. Null render mesh. Tick: "<<ticks);
				//return false;
			}

			if(ENABLE_VELOCITY_EXPORT){//TODO...

				IParticleObjectExt* particlesExt = GetParticleObjectExtInterface(pParticleObject);
				if(particlesExt){
					Tab<Point3> perVertexVelocities;
					//particlesExt->UpdateParticles(node, ticks);
					particlesExt->GetRenderMeshVertexSpeed(ticks, node, nullView, perVertexVelocities);  

					if(perVertexVelocities.Count() != pMesh->getNumVerts()){
						ESS_LOG_WARNING("Warning: Mesh Vertices count and velocities count do not match.");
					}

					mesh->mVelocitiesVec.reserve(perVertexVelocities.Count());
					for(int j=0; j<perVertexVelocities.Count(); j++){
						Abc::V4f pVelocity4 = ConvertMaxVectorToAlembicVector4(perVertexVelocities[j] * TIME_TICKSPERSEC); 
						pVelocity4 = pVelocity4 * nodeWorldTransInv;
						mesh->mVelocitiesVec.push_back(Abc::V3f(pVelocity4.x, pVelocity4.y, pVelocity4.z));
					}
				}
				else{
					ESS_LOG_WARNING("Warning: could not obtain per vertex velocities.");
				}	
			}

			mesh->Save(mJob->mOptions, pMesh, NULL, meshTM, pMtl, -1, nNumSamples == 0, pMatMerge);

			if(bNeedDelete){
				delete pMesh;
			}
		//}

		return true;
	}

	//Export as particle flow if not simple particle

	splitTypeT oldSplitType = setPerParticleMeshRenderSetting(obj, ticks, kRender_splitType_particle);

	//We have to put the particle system into the renders state so that PFOperatorMaterialFrequency::Proceed will set the materialID channel
	bool bRenderStateForced = false;
	if(iSystem && !iSystem->IsRenderState()){
		iSystem->SetRenderState(true);
		bRenderStateForced = true;
	}

	//particlesExt->UpdateParticles(node, ticks);

	SetMaxSceneTime(ticks);

	particleGroupInterface groupInterface(particlesExt, obj, node, &nullView);

	for(int i=0; i< particlesExt->NumParticles(); i++){

		groupInterface.setCurrentParticle(ticks, i);

		Matrix3 meshTM;
		if(!groupInterface.getCurrentParticleMeshTM(meshTM)){
			continue;
		}

		BOOL bNeedDelete = FALSE;
		Mesh* pMesh = groupInterface.getCurrentParticleMesh(bNeedDelete);

		if(!pMesh){
			ESS_LOG_INFO("Error. Null render mesh. Tick: "<<ticks<<" pid: "<<i);
			continue;
		}

		//conpute vertex velocity, and save them immediately to final result
		//max meshes do not store per vertex velocities
		//we want the end result relative particle system space

		if(ENABLE_VELOCITY_EXPORT){
#if 1
			//float fps = (float)GetFrameRate();
			Abc::V4f pPosition4 = ConvertMaxPointToAlembicPoint4(*particlesExt->GetParticlePositionByIndex(i));
			Abc::V4f pVelocity4 = ConvertMaxVectorToAlembicVector4(*particlesExt->GetParticleSpeedByIndex(i) * TIME_TICKSPERSEC); 

			//Abc::V4f pVel = pPosition4 - prevPos;
			//Abc::V4f pVels = pVelocity4 * fps;
			//float factor = pVel.y / pVelocity4.y;
			//prevPos = pPosition4;

			Abc::Quatd pAngularVelocity(0.0, 0.0, 1.0, 0.0);
			AngAxis particleSpin = *particlesExt->GetParticleSpinByIndex(i);
			particleSpin.angle *= TIME_TICKSPERSEC;
			ConvertMaxAngAxisToAlembicQuat(particleSpin, pAngularVelocity);
			Abc::M44d mAngularVelocity44 = pAngularVelocity.toMatrix44();

			//AngAxis angAxis = *particlesExt->GetParticleSpinByIndex(i);
			//Abc::V3f pAngularVelocity = ConvertMaxNormalToAlembicNormal(angAxis.axis);
			//pAngularVelocity = pAngularVelocity * angAxis.angle * 200;

			//the values are returned in world space, so move them to particle space
			pPosition4 = pPosition4 * nodeWorldTransInv;
			pVelocity4 = pVelocity4 * nodeWorldTransInv;
			mAngularVelocity44 = mAngularVelocity44 * nodeWorldTransInv;
			//pAngularVelocity = pAngularVelocity * nodeWorldTransInv;

			Abc::V3f pPosition = Abc::V3f(pPosition4.x, pPosition4.y, pPosition4.z);
			Abc::V3f pVelocity = Abc::V3f(pVelocity4.x, pVelocity4.y, pVelocity4.z);
			Abc::M33d mAngularVelocity = extractRotation(mAngularVelocity44);
			Abc::M33d mIdentity;
			mIdentity.makeIdentity();
			mAngularVelocity -= mIdentity;

			mesh->mVelocitiesVec.reserve(pMesh->getNumVerts());
			for(int j=0; j<pMesh->getNumVerts(); j++){
				Abc::V3f meshVertex = ConvertMaxPointToAlembicPoint(pMesh->getVert(j) * meshTM);//the mesh vertex in particle system space
				Abc::V3f vVelocity = meshVertex * mAngularVelocity;
				mesh->mVelocitiesVec.push_back(pVelocity /*+ vVelocity*/);
			}
#else
			Tab<Point3> perVertexVelocities;
			//particlesExt->UpdateParticles(node, ticks);
			//Note: this call doesn't seem to be implemented for pflow
			particlesExt->GetRenderMeshVertexSpeed(ticks, node, nullView, perVertexVelocities);  

			if(perVertexVelocities.Count() != pMesh->getNumVerts()){
				ESS_LOG_WARNING("Warning: Mesh Vertices count and velocities count do not match.");
			}

			mesh->mVelocitiesVec.reserve(perVertexVelocities.Count());
			for(int j=0; j<perVertexVelocities.Count(); j++){
				Abc::V4f pVelocity4 = ConvertMaxVectorToAlembicVector4(perVertexVelocities[j] * TIME_TICKSPERSEC); 
				pVelocity4 = pVelocity4 * nodeWorldTransInv;
				mesh->mVelocitiesVec.push_back(Abc::V3f(pVelocity4.x, pVelocity4.y, pVelocity4.z));
			}
#endif
		}

		//IParticleChannelIntR* chMtlIndex = GetParticleChannelMtlIndexRInterface(pCont);
		//int nMatId = -1;
		//if(chMtlIndex){
		//	chMtlIndex->GetValue(meshId);
		//}

		pMatMerge->currUniqueHandle = Animatable::GetHandleByAnim(groupInterface.m_pCurrParticleGroup->GetActionList());
		pMatMerge->bPreserveIds = true;

		if(i == 0){
			mesh->Save(mJob->mOptions, pMesh, NULL, meshTM, groupInterface.m_pCurrGroupMtl, -1, nNumSamples == 0, pMatMerge);
		}
		else{
			IntermediatePolyMesh3DSMax currPolyMesh;
			currPolyMesh.Save(mJob->mOptions, pMesh, NULL, meshTM, groupInterface.m_pCurrGroupMtl, -1, nNumSamples == 0, pMatMerge);
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

	if(bRenderStateForced){
		iSystem->SetRenderState(false);
	}

	setPerParticleMeshRenderSetting(obj, ticks, oldSplitType);
	
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

		ExoNullView nullView;

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

			ExoNullView nullView;

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