#include "Alembic.h"
#include "AlembicParticles.h"
#include "AlembicParticlesExtInterface.h"
#include "stdafx.h"

IAlembicParticlesExt::IAlembicParticlesExt(AlembicParticles* pAlembicParticles)
{
  m_pAlembicParticles = pAlembicParticles;
}

// Implemented by the Plug-In.
// Since particles may have different motion, the particle system should supply
// speed information
// on per vertex basis for a motion blur effect to be able to generate effect.
// the method is not exposed in maxscript
// returns true if the object supports the method
// Parameters:
//		TimeValue t
//			The time to get the mesh vertices speed.
//		INode *inode
//			The node in the scene
//		View& view
//			If the renderer calls this method it will pass the view
//information here.
//		Tab<Point3>& speed
//			speed per vertex in world coordinates
bool IAlembicParticlesExt::GetRenderMeshVertexSpeed(TimeValue t, INode* inode,
                                                    View& view,
                                                    Tab<Point3>& speed)
{
  ESS_PROFILE_FUNC();
  ESS_LOG_INFO("IAlembicParticlesExt::GetRenderMeshVertexSpeed() - t: "
               << t << "  currTick: " << m_pAlembicParticles->m_currTick);

  int numParticles = 0;

  Tab<Point3> perParticleVelocities;
  if (m_pAlembicParticles->m_currTick == t) {
    perParticleVelocities = m_pAlembicParticles->parts.vels;
    numParticles = perParticleVelocities.Count();
  }
  else {
    int iSampleTime = GetTimeValueFromSeconds(t);

    AbcG::IPointsSchema::Sample floorSample;
    AbcG::IPointsSchema::Sample ceilSample;
    SampleInfo sampleInfo = m_pAlembicParticles->GetSampleAtTime(
        m_pAlembicParticles->m_iPoints, iSampleTime, floorSample, ceilSample);

    numParticles = m_pAlembicParticles->GetNumParticles(floorSample);
    perParticleVelocities.SetCount(numParticles);
    // Matrix3 identityMat;
    // identityMat.IdentityMatrix();
    m_pAlembicParticles->GetParticleVelocities(
        floorSample, ceilSample, sampleInfo, m_pAlembicParticles->m_objToWorld,
        perParticleVelocities);
  }

  //   Matrix3 nodeWorldTM = inode->GetObjTMAfterWSM(t);
  //   Abc::M44d nodeWorldTrans;
  //   ConvertMaxMatrixToAlembicMatrix(nodeWorldTM, nodeWorldTrans);
  // Abc::M44d nodeWorldTransInv = nodeWorldTrans.inverse();

  ExoNullView nullView;

  // calculate the total number of vertices in the particle system render mesh
  int totalVerts = 0;
  for (int i = 0; i < numParticles; i++) {
    BOOL curNeedDelete = FALSE;
    Mesh* pMesh = m_pAlembicParticles->GetMultipleRenderMesh_Internal(
        t, inode, nullView, curNeedDelete, i);

    if (!pMesh) {
      continue;
    }

    int curNumVerts = pMesh->getNumVerts();
    if (curNumVerts == 0) {
      if (curNeedDelete) {
        pMesh->FreeAll();
        delete pMesh;
      }
      continue;
    }

    totalVerts += curNumVerts;
  }
  speed.SetCount(totalVerts);

  // fill the per render mesh vertex array
  int v = 0;
  for (int i = 0; i < numParticles; i++) {
    BOOL curNeedDelete = FALSE;
    Mesh* pMesh = m_pAlembicParticles->GetMultipleRenderMesh_Internal(
        t, inode, nullView, curNeedDelete, i);

    if (!pMesh) {
      continue;
    }

    int curNumVerts = pMesh->getNumVerts();
    if (curNumVerts == 0) {
      if (curNeedDelete) {
        pMesh->FreeAll();
        delete pMesh;
      }
      pMesh = NULL;
      continue;
    }

    for (int j = 0; j < curNumVerts; j++) {
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

// Implemented by the Plug-In.
// Particle system may supply multiple render meshes. If this method returns a
// positive number,
// then GetMultipleRenderMesh and GetMultipleRenderMeshTM will be called for
// each mesh,
// instead of calling GetRenderMesh. The method has a current time parameter
// which is not
// the case with the NumberOfRenderMeshes method of GeomObject class
// the method is not exposed in maxscript
// Parameters:
//		TimeValue t
//			Time for the number of render meshes request.
//		INode *inode
//			The node in the scene
//		View& view
//			If the renderer calls this method it will pass the view
//information here.
int IAlembicParticlesExt::NumberOfRenderMeshes(TimeValue t, INode* inode,
                                               View& view)
{
  // ESS_LOG_WARNING("IAlembicParticlesExt::NumberOfRenderMeshes");
  // return m_pAlembicParticles->NumberOfRenderMeshes();
  return 0;  // 0 means multiple render meshes not supported
}

// For multiple render meshes, if it supports vertex speed for motion blur, this
// method must be implemented
// Since particles may have different motion, the particle system should supply
// speed information
// on per vertex basis for a motion blur effect to be able to generate effect.
// the method is not exposed in maxscript
// returns true if the particular render mesh supports the method
// Parameters:
//		TimeValue t
//			The time to get the mesh vertices speed.
//		INode *inode
//			The node in the scene
//		View& view
//			If the renderer calls this method it will pass the view
//information here.
//		int meshNumber
//			Specifies which of the multiple meshes is being asked
//for.
//		Tab<Point3>& speed
//			speed per vertex in world coordinates
bool IAlembicParticlesExt::GetMultipleRenderMeshVertexSpeed(
    TimeValue t, INode* inode, View& view, int meshNumber, Tab<Point3>& speed)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetMultipleRenderMeshVertexSpeed not implmented.");
  return false;
}

// Implemented by the Plug-In.
// This method is called so the particle system can update its state to reflect
// the current time passed.  This may involve generating new particle that are
// born,
// eliminating old particles that have expired, computing the impact of
// collisions or
// force field effects, and modify properties of the particles.
// Parameters:
//		TimeValue t
//			The particles should be updated to reflect this time.
//		INode *node
//			This is the emitter node.
// the method is not exposed in maxscript
void IAlembicParticlesExt::UpdateParticles(INode* node, TimeValue t)
{
  // ESS_LOG_WARNING("IAlembicParticlesExt::UpdateParticles.");
  m_pAlembicParticles->UpdateParticles(t, node);
}

// Implemented by the Plug-in
// Use this method to retrieve time of the current update step. The update time
// maybe unrelated to
// the current time of the scene.
TimeValue IAlembicParticlesExt::GetUpdateTime()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetUpdateTime not implmented.");
  return 0;
}
// Implemented by the Plug-in
// Use this method to retrieve time interval of the current update step. The
// update time maybe unrelated to
// the current time of the scene. The GetUpdateTime method above retrieves the
// finish time.
void IAlembicParticlesExt::GetUpdateInterval(TimeValue& start,
                                             TimeValue& finish)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetUpdateInterval not implmented.");
}

// Implemented by the Plug-In.
// The method returns how many particles are currently in the particle system.
// Some of these particles may be dead or not born yet (indicated by GetAge(..)
// method =-1).
int IAlembicParticlesExt::NumParticles()
{
  // ESS_LOG_WARNING("IAlembicParticlesExt::NumParticles");
  return m_pAlembicParticles->NumberOfRenderMeshes();
  // return 0;
}

// Implemented by the Plug-In.
// The method returns how many particles were born. Since particle systems have
// a tendency of reusing indices for newly born particles, sometimes it's
// necessary
// to keep a track for particular particles. This method and the methods that
// deal with
// particle IDs allow us to accomplish that.
int IAlembicParticlesExt::NumParticlesGenerated()
{
  return IAlembicParticlesExt::NumParticles();
}

// Implemented by the Plug-in
// The following four methods modify amount of particles in the particle system
// Returns true if the operation was completed successfully
//		Add a single particle
bool IAlembicParticlesExt::AddParticle()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::AddParticle not implmented.");
  return false;
}
//		Add "num" particles into the particle system
bool IAlembicParticlesExt::AddParticles(int num)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::AddParticles not implmented.");
  return false;
}
//		Delete a single particle with the given index
bool IAlembicParticlesExt::DeleteParticle(int index)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::DeleteParticle not implmented.");
  return false;
}
//		List-type delete of "num" particles starting with "start"
bool IAlembicParticlesExt::DeleteParticles(int start, int num)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::DeleteParticles not implmented.");
  return false;
}

// Implemented by the Plug-In.
// Each particle is given a unique ID (consecutive) upon its birth. The method
// allows us to distinguish physically different particles even if they are
// using
// the same particle index (because of the "index reusing").
// Parameters:
//		int i
//			index of the particle in the range of [0,
//NumParticles-1]
int IAlembicParticlesExt::GetParticleBornIndex(int i)
{
  // ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleBornIndex.");
  return (int)m_pAlembicParticles->m_idVec[i];
}

// Implemented by the Plug-In.
// the methods verifies if a particle with a given particle id (born index) is
// present
// in the particle system. The methods returns Particle Group node the particle
// belongs to,
// and index in this group. If there is no such particle, the method returns
// false.
// Parameters:
//		int bornIndex
//			particle born index
//		INode*& groupNode
//			particle group the particle belongs to
//		int index
//			particle index in the particle group or particle system
bool IAlembicParticlesExt::HasParticleBornIndex(int bornIndex, int& index)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::HasParticleBornIndex not implmented.");
  return false;
}
INode* IAlembicParticlesExt::GetParticleGroup(int index)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleGroup not implmented.");
  return NULL;
}
int IAlembicParticlesExt::GetParticleIndex(int bornIndex)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleIndex not implmented.");
  return 0;
}

// Implemented by the Plug-In.
// The following four methods define "current" index or bornIndex. This index is
// used
// in the property methods below to get the property without specifying the
// index.
int IAlembicParticlesExt::GetCurrentParticleIndex()
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetCurrentParticleIndex not implmented.");
  return 0;
}
int IAlembicParticlesExt::GetCurrentParticleBornIndex()
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetCurrentParticleBornIndex not implmented.");
  return 5;
}
void IAlembicParticlesExt::SetCurrentParticleIndex(int index)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetCurrentParticleIndex not implmented.");
}
void IAlembicParticlesExt::SetCurrentParticleBornIndex(int bornIndex)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetCurrentParticleBornIndex not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define age of the specified particle. Particle is
// specified by either its
// index in the particle group or particle system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		TimeValue age
//			new age value to set for a particle
TimeValue IAlembicParticlesExt::GetParticleAgeByIndex(int index)
{
  // ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleAgeByIndex.");
  return m_pAlembicParticles->parts.ages[index];
}
TimeValue IAlembicParticlesExt::GetParticleAgeByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleAgeByBornIndex not implmented.");
  return 0;
}
void IAlembicParticlesExt::SetParticleAgeByIndex(int index, TimeValue age)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleAgeByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleAgeByBornIndex(int id, TimeValue age)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleAgeByBornIndex not implmented.");
}
TimeValue IAlembicParticlesExt::GetParticleAge()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleAge not implmented.");
  return 0;
}
void IAlembicParticlesExt::SetParticleAge(TimeValue age)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleAge not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define lifespan of the specified particle. Particle
// is specified by either its
// index in the particle group or particle system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		TimeValue lifespan
//			new lifespan value to set for a particle
TimeValue IAlembicParticlesExt::GetParticleLifeSpanByIndex(int index)
{
  // ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleLifeSpanByIndex.");
  return TIME_PosInfinity;
}
TimeValue IAlembicParticlesExt::GetParticleLifeSpanByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleLifeSpanByBornIndex not implmented.");
  return TIME_PosInfinity;
}
void IAlembicParticlesExt::SetParticleLifeSpanByIndex(int index,
                                                      TimeValue LifeSpan)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleLifeSpanByBornIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleLifeSpanByBornIndex(int id,
                                                          TimeValue LifeSpan)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleLifeSpanByBornIndex not implmented.");
}
TimeValue IAlembicParticlesExt::GetParticleLifeSpan()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleLifeSpan not implmented.");
  return TIME_PosInfinity;
}
void IAlembicParticlesExt::SetParticleLifeSpan(TimeValue lifespan)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleLifeSpan not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define for how long the specified particle was
// staying in the current
// particle group. Particle is specified by either its
// index in the particle group or particle system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		TimeValue time
//			how long particle was staying in the current particle
//group
TimeValue IAlembicParticlesExt::GetParticleGroupTimeByIndex(int index)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleGroupTimeByIndex not implmented.");
  return 0;
}
TimeValue IAlembicParticlesExt::GetParticleGroupTimeByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleGroupTimeByBornIndex not implmented.");
  return 0;
}
void IAlembicParticlesExt::SetParticleGroupTimeByIndex(int index,
                                                       TimeValue time)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleGroupTimeByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleGroupTimeByBornIndex(int id,
                                                           TimeValue time)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleGroupTimeByBornIndex not implmented.");
}
TimeValue IAlembicParticlesExt::GetParticleGroupTime()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleGroupTime not implmented.");
  return 0;
}
void IAlembicParticlesExt::SetParticleGroupTime(TimeValue time)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleGroupTime not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define position of the specified particle in the
// current state.
// Particle is specified by either its index in the particle group or particle
// system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		Point3 pos
//			position of the particle
Point3* IAlembicParticlesExt::GetParticlePositionByIndex(int index)
{
  // ESS_LOG_WARNING("IAlembicParticlesExt::GetParticlePositionByIndex.");
  return &(m_pAlembicParticles->parts.points[index]);
}
Point3* IAlembicParticlesExt::GetParticlePositionByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticlePositionByBornIndex not implmented.");
  return &(m_pAlembicParticles->parts.points[0]);
}
void IAlembicParticlesExt::SetParticlePositionByIndex(int index, Point3 pos)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticlePositionByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticlePositionByBornIndex(int id, Point3 pos)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticlePositionByBornIndex not implmented.");
}
Point3* IAlembicParticlesExt::GetParticlePosition()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticlePosition not implmented.");
  return &(m_pAlembicParticles->parts.points[0]);
}
void IAlembicParticlesExt::SetParticlePosition(Point3 pos)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticlePosition not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define speed of the specified particle in the
// current state.
// Particle is specified by either its index in the particle group or particle
// system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		Point3 speed
//			speed of the particle in units per frame
Point3* IAlembicParticlesExt::GetParticleSpeedByIndex(int index)
{
  // ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleSpeedByIndex.");
  return &(m_pAlembicParticles->parts.vels[index]);
}
Point3* IAlembicParticlesExt::GetParticleSpeedByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleSpeedByBornIndex not implmented.");
  return &(m_pAlembicParticles->parts.vels[0]);
}
void IAlembicParticlesExt::SetParticleSpeedByIndex(int index, Point3 speed)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleSpeedByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleSpeedByBornIndex(int id, Point3 speed)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleSpeedByBornIndex not implmented.");
}
Point3* IAlembicParticlesExt::GetParticleSpeed()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleSpeed not implmented.");
  return &(m_pAlembicParticles->parts.vels[0]);
}
void IAlembicParticlesExt::SetParticleSpeed(Point3 speed)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleSpeed not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define orientation of the specified particle in the
// current state.
// Particle is specified by either its index in the particle group or particle
// system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		Point3 orient
//			orientation of the particle. The orientation is defined by
//incremental rotations
//			by world axes X, Y and Z. The rotation values are in
//degrees.
Point3* IAlembicParticlesExt::GetParticleOrientationByIndex(int index)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleOrientationByIndex not implmented.");
  return NULL;
}
Point3* IAlembicParticlesExt::GetParticleOrientationByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleOrientationByBornIndex not "
      "implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleOrientationByIndex(int index,
                                                         Point3 orient)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleOrientationByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleOrientationByBornIndex(int id,
                                                             Point3 orient)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleOrientationByBornIndex not "
      "implmented.");
}
Point3* IAlembicParticlesExt::GetParticleOrientation()
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleOrientation not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleOrientation(Point3 orient)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleOrientation not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define angular speed of the specified particle in
// the current state.
// Particle is specified by either its index in the particle group or particle
// system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		AngAxis spin
//			angular speed of the particle in rotation per frame
//			axis defines rotation axis, angle defines rotation amount
//per frame
AngAxis* IAlembicParticlesExt::GetParticleSpinByIndex(int index)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleSpinByIndex not implmented.");
  return NULL;
}
AngAxis* IAlembicParticlesExt::GetParticleSpinByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleSpinByBornIndex not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleSpinByIndex(int index, AngAxis spin)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleSpinByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleSpinByBornIndex(int id, AngAxis spin)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleSpinByBornIndex not implmented.");
}
AngAxis* IAlembicParticlesExt::GetParticleSpin()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleSpin not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleSpin(AngAxis spin)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleSpin not implmented.");
}

// Implemented by the Plug-In.
// The following twelve methods define scale factor of the specified particle in
// the current state.
// The XYZ form is used for non-uniform scaling
// Particle is specified by either its index in the particle group or particle
// system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		float scale
//			uniform scale factor
//		Point3 scale
//			scale factor for each local axis of the particle
float IAlembicParticlesExt::GetParticleScaleByIndex(int index)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleScaleByIndex not implmented.");
  return 1.0f;
}
float IAlembicParticlesExt::GetParticleScaleByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleScaleByBornIndex not implmented.");
  return 1.0f;
}
void IAlembicParticlesExt::SetParticleScaleByIndex(int index, float scale) { ; }
void IAlembicParticlesExt::SetParticleScaleByBornIndex(int id, float scale)
{
  ;
}
float IAlembicParticlesExt::GetParticleScale() { return 1.0f; }
void IAlembicParticlesExt::SetParticleScale(float scale) { ; }
Point3* IAlembicParticlesExt::GetParticleScaleXYZByIndex(int index)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleScaleXYZByIndex not implmented.");
  return NULL;
}
Point3* IAlembicParticlesExt::GetParticleScaleXYZByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleScaleXYZByBornIndex not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleScaleXYZByIndex(int index, Point3 scale)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleScaleXYZByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleScaleXYZByBornIndex(int id, Point3 scale)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleScaleXYZByBornIndex not implmented.");
}
Point3* IAlembicParticlesExt::GetParticleScaleXYZ()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleScaleXYZ not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleScaleXYZ(Point3 scale)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleScaleXYZ not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define transformation matrix of the specified
// particle in the current state.
// Particle is specified by either its index in the particle group or particle
// system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		Matrix3 tm
//			transformation matrix of the particle
Matrix3* IAlembicParticlesExt::GetParticleTMByIndex(int index)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleTMByIndex not implmented.");
  return NULL;
}
Matrix3* IAlembicParticlesExt::GetParticleTMByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleTMByBornIndex not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleTMByIndex(int index, Matrix3 tm)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleTMByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleTMByBornIndex(int id, Matrix3 tm)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleTMByBornIndex not implmented.");
}
Matrix3* IAlembicParticlesExt::GetParticleTM()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleTM not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleTM(Matrix3 tm)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleTM not implmented.");
}

// Implemented by the Plug-In.
// The following six methods define selection status of the specified particle
// in the current state.
// Particle is specified by either its index in the particle group or particle
// system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		bool selected
//			selection status of the particle
bool IAlembicParticlesExt::GetParticleSelectedByIndex(int index)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleSelectedByIndex not implmented.");
  return true;
}
bool IAlembicParticlesExt::GetParticleSelectedByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleSelectedByBornIndex not implmented.");
  return true;
}
void IAlembicParticlesExt::SetParticleSelectedByIndex(int index, bool selected)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleSelectedByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleSelectedByBornIndex(int id, bool selected)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleSelectedByBornIndex not implmented.");
}
bool IAlembicParticlesExt::GetParticleSelected()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleSelected not implmented.");
  return true;
}
void IAlembicParticlesExt::SetParticleSelected(bool selected)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleSelected not implmented.");
}

// Implemented by the Plug-In.
// The following seven methods define shape of the specified particle in the
// current state.
// Particle is specified by either its index in the particle group or particle
// system, or by its born index
// if no index is specified then the "current" index is used
// Parameters:
//		int id
//			particle born index
//		int index
//			particle index in the particle group
//		Mesh* shape
//			shape of the particle
Mesh* IAlembicParticlesExt::GetParticleShapeByIndex(int index)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleShapeByIndex not implmented.");
  return NULL;
}
Mesh* IAlembicParticlesExt::GetParticleShapeByBornIndex(int id)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::GetParticleShapeByBornIndex not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleShapeByIndex(int index, Mesh* shape)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleShapeByIndex not implmented.");
}
void IAlembicParticlesExt::SetParticleShapeByBornIndex(int id, Mesh* shape)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetParticleShapeByBornIndex not implmented.");
}
Mesh* IAlembicParticlesExt::GetParticleShape()
{
  ESS_LOG_WARNING("IAlembicParticlesExt::GetParticleShape not implmented.");
  return NULL;
}
void IAlembicParticlesExt::SetParticleShape(Mesh* shape)
{
  ESS_LOG_WARNING("IAlembicParticlesExt::SetParticleShape not implmented.");
}
// set the same shape for all particles
void IAlembicParticlesExt::SetGlobalParticleShape(Mesh* shape)
{
  ESS_LOG_WARNING(
      "IAlembicParticlesExt::SetGlobalParticleShape not implmented.");
}