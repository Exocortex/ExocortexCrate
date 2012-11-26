#include "stdafx.h"
#include "Alembic.h"
#include "AlembicParticlesExtInterface.h"
#include "AlembicParticles.h"

IAlembicParticlesExt::IAlembicParticlesExt(AlembicParticles* pAlembicParticles)
{
	m_pAlembicParticles = pAlembicParticles;


}


bool IAlembicParticlesExt::GetRenderMeshVertexSpeed(TimeValue t, INode *inode, View& view, Tab<Point3>& speed)  
{ 
   ESS_PROFILE_FUNC();
	ESS_LOG_INFO("IAlembicParticlesExt::GetRenderMeshVertexSpeed() - t: "<<t<<"  currTick: "<<m_pAlembicParticles->m_currTick);

	int numParticles = 0;

	Tab<Point3> perParticleVelocities;
	if( m_pAlembicParticles->m_currTick == t ){
		perParticleVelocities = m_pAlembicParticles->parts.vels;
		numParticles = perParticleVelocities.Count();
	}
	else{
		int iSampleTime = GetTimeValueFromSeconds(t);

		AbcG::IPointsSchema::Sample floorSample;
		AbcG::IPointsSchema::Sample ceilSample;
		SampleInfo sampleInfo = m_pAlembicParticles->GetSampleAtTime(m_pAlembicParticles->m_iPoints, iSampleTime, floorSample, ceilSample);

		numParticles = m_pAlembicParticles->GetNumParticles(floorSample);
		perParticleVelocities.SetCount(numParticles);
		//Matrix3 identityMat;
		//identityMat.IdentityMatrix();
		m_pAlembicParticles->GetParticleVelocities( floorSample, ceilSample, sampleInfo, m_pAlembicParticles->m_objToWorld, perParticleVelocities);
	}

 //   Matrix3 nodeWorldTM = inode->GetObjTMAfterWSM(t);
 //   Abc::M44d nodeWorldTrans;
 //   ConvertMaxMatrixToAlembicMatrix(nodeWorldTM, nodeWorldTrans);
	//Abc::M44d nodeWorldTransInv = nodeWorldTrans.inverse();

	ExoNullView nullView;

	//calculate the total number of vertices in the particle system render mesh
	int totalVerts = 0;
	for(int i=0; i<numParticles; i++)
	{
		BOOL curNeedDelete = FALSE;
		Mesh* pMesh = m_pAlembicParticles->GetMultipleRenderMesh_Internal(t, inode, nullView, curNeedDelete, i);

		if(!pMesh){
			continue;
		}

		int curNumVerts = pMesh->getNumVerts();
		if(curNumVerts == 0){
			if (curNeedDelete) {
				pMesh->FreeAll();
				delete pMesh;
			}
			continue;
		}

		totalVerts += curNumVerts;
	}
	speed.SetCount(totalVerts);

	//fill the per render mesh vertex array
	int v = 0;
	for(int i=0; i<numParticles; i++){

		BOOL curNeedDelete = FALSE;
		Mesh* pMesh = m_pAlembicParticles->GetMultipleRenderMesh_Internal(t, inode, nullView, curNeedDelete, i);

		if(!pMesh){
			continue;
		}

		int curNumVerts = pMesh->getNumVerts();
		if(curNumVerts == 0){
			if (curNeedDelete) {
				pMesh->FreeAll();
				delete pMesh;
			}
			pMesh = NULL;
			continue;
		}

		for(int j=0; j<curNumVerts; j++){

			speed[v] = perParticleVelocities[i];
			v++;
		}
		if (curNeedDelete) {
			pMesh->FreeAll();
			delete pMesh;
		}
	}

	return true; 
}

int IAlembicParticlesExt::NumberOfRenderMeshes(TimeValue t, INode *inode, View& view) { 
	//ESS_LOG_WARNING("IAlembicParticlesExt::NumberOfRenderMeshes");
	//return m_pAlembicParticles->NumberOfRenderMeshes();
	return 0;
}

// Implemented by the Plug-In.
// The method returns how many particles are currently in the particle system. 
// Some of these particles may be dead or not born yet (indicated by GetAge(..) method =-1). 
int IAlembicParticlesExt::NumParticles(){
	//ESS_LOG_WARNING("IAlembicParticlesExt::NumParticles");
	return m_pAlembicParticles->NumberOfRenderMeshes();
	//return 0;
}