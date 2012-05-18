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

class NullView: public View 
{
public:
    Point2 ViewToScreen(Point3 p) { return Point2(p.x,p.y); }
    NullView() { worldToView.IdentityMatrix(); screenW=640.0f; screenH = 480.0f; }
};

Mesh* getParticleSystemRenderMesh(TimeValue ticks, Object* obj, INode* node, BOOL& bNeedDelete)
{
	Mesh* pMesh = NULL;

	IPFActionList* particleActionList = GetPFActionListInterface(obj);
	if(!particleActionList){
		return pMesh;
	}

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

			NullView nullView;
			//IParticleContainer* pCont = GetChannelContainerInterface(obj);

			IParticleObjectExt* particlesExt = GetParticleObjectExtInterface(obj);
			INode *particleGroupNode = particlesExt->GetParticleGroup(0);
			Object *particleGroupObj = (particleGroupNode != NULL) ? particleGroupNode->EvalWorldState(ticks).obj : NULL;

		    IParticleGroup *particleGroup = GetParticleGroupInterface(particleGroupObj);
			if(!particleGroup){
				continue;
			}
			::IObject *pCont = particleGroup->GetParticleContainer();

			pMesh = particleRender->GetRenderMesh(pCont, ticks, obj, node, nullView, bNeedDelete);
			return pMesh;
		}

	}

	return pMesh;
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