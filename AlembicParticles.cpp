#include "stdafx.h"
#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicParticles.h"
#include "AlembicArchiveStorage.h"
#include "AlembicVisibilityController.h"
#include "utility.h"
#include "AlembicMAXScript.h"
#include "AlembicMetadataUtils.h"
#include "AlembicParticlesExtInterface.h"


static AlembicParticlesClassDesc s_AlembicParticlesClassDesc;
ClassDesc2 *GetAlembicParticlesClassDesc() { return &s_AlembicParticlesClassDesc; }

#ifdef min
#undef min
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Alembic_XForm_Ctrl_Param_Blk
///////////////////////////////////////////////////////////////////////////////////////////////////

static const int ALEMBIC_PARTICLES_VERSION = 2;

static ParamBlockDesc2 AlembicParticlesParams(
	0,
	_T(ALEMBIC_SIMPLE_PARTICLE_SCRIPTNAME),
	0,
	GetAlembicParticlesClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI | P_VERSION,
	ALEMBIC_PARTICLES_VERSION,
	0,

	// rollout description 
	IDD_ALEMBIC_PARTICLE_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
	AlembicParticles::ID_PATH, _T("path"), TYPE_FILENAME, P_RESET_DEFAULT, IDS_PATH,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PATH_EDIT,
		p_assetTypeID,	AssetManagement::kExternalLink,
		p_end,
        
	AlembicParticles::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, P_RESET_DEFAULT, IDS_IDENTIFIER,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_IDENTIFIER_EDIT,
		p_end,

	AlembicParticles::ID_TIME, _T("time"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIME,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_TIME_EDIT,    IDC_TIME_SPIN, 0.01f,
		p_end,
        
    AlembicParticles::ID_MUTED, _T("muted"), TYPE_BOOL, P_ANIMATABLE, IDS_MUTED,
		p_default,       FALSE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_MUTED_CHECKBOX,
		p_end,

	AlembicParticles::ID_VIEWPORT_PERCENT, _T("Viewport %"), TYPE_FLOAT, P_ANIMATABLE, IDS_VIEWPORT_PERCENT,
		p_default,       100.0f,
		p_range,         0.0f, 100.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_VIEWPORT_PERCENT_EDIT,    IDC_VIEWPORT_PERCENT_SPIN, 1.0f,
		p_end,
        

    AlembicParticles::ID_RENDER_AS_TICKS, _T("Render as ticks"), TYPE_BOOL, P_ANIMATABLE, IDS_RENDER_AS_TICKS,
		p_default,       FALSE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_RENDER_AS_TICKS_CHECKBOX,
		p_end,

	p_end
);

///////////////////////////////////////////////////////////////////////////////////////////////////

// static member variables
AlembicParticles *AlembicParticles::s_EditObject = NULL;
IObjParam *AlembicParticles::s_ObjParam = NULL;


void AlembicParticles::EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags)  {
	if ((flags&FILE_ENUM_CHECK_AWORK1)&&TestAFlag(A_WORK1)) return; // LAM - 4/11/03

	if (!(flags&FILE_ENUM_INACTIVE)) return; // not needed by renderer

	if(flags & FILE_ENUM_ACCESSOR_INTERFACE)	{
		IEnumAuxAssetsCallback* callback = static_cast<IEnumAuxAssetsCallback*>(&nameEnum);
		callback->DeclareAsset(AlembicPathAccessor(this));		
	}
	//else {
	//	IPathConfigMgr::GetPathConfigMgr()->RecordInputAsset( this->GetParamBlockByID( 0 )->GetAssetUser( GetParamIdByName( this, 0, "path" ), 0 ), nameEnum, flags);
	//}

	ReferenceTarget::EnumAuxFiles(nameEnum, flags);
} 

ParticleMtl::ParticleMtl():Material() 
{
   Kd[0] = PARTICLE_R;
   Kd[1] = PARTICLE_G;
   Kd[2] = PARTICLE_B;
   Ks[0] = PARTICLE_R;
   Ks[1] = PARTICLE_G;
   Ks[2] = PARTICLE_B;
   shininess = (float)0.0;
   shadeLimit = GW_WIREFRAME;
   selfIllum = (float)1.0;
}

AlembicParticles::AlembicParticles()
    : SimpleParticle()
{
    pblock2 = NULL;
    m_pBoxMaker = NULL;
    m_pSphereMaker = NULL;
	m_pCylinderMaker = NULL;
	m_pDiskMaker = NULL;
	m_pRectangleMaker = NULL;

    s_AlembicParticlesClassDesc.MakeAutoParamBlocks(this);

	m_outputOrientationMotionBlurWarning = true;

	 m_bRenderAsTicks = false;

	 m_fViewportPercent = 100.0f;

	 m_pAlembicParticlesExt = new IAlembicParticlesExt(this);
}

// virtual
AlembicParticles::~AlembicParticles()
{
    ALEMBIC_SAFE_DELETE(m_pBoxMaker);
    ALEMBIC_SAFE_DELETE(m_pSphereMaker);
    ALEMBIC_SAFE_DELETE(m_pCylinderMaker);
    ALEMBIC_SAFE_DELETE(m_pDiskMaker);
    ALEMBIC_SAFE_DELETE(m_pRectangleMaker);
	ALEMBIC_SAFE_DELETE(m_pAlembicParticlesExt);
    clearViewportMeshCache();
}

static const bool LOG = false;

void AlembicParticles::clearViewportMeshCache()
{
   ESS_PROFILE_FUNC();
    //clear the viewport meshes

   for(InstanceMeshCache::iterator it = m_InstanceMeshCache.begin(); it != m_InstanceMeshCache.end(); it++){
       if(it->second.needDelete){
          it->second.mesh->FreeAll();
          delete it->second.mesh;
          //ESS_LOG_WARNING("deleting mesh");
       }
   }
   m_InstanceMeshCache.clear();

}

void AlembicParticles::UpdateParticles(TimeValue t, INode *node)
{
	ESS_CPP_EXCEPTION_REPORTING_START

   ESS_PROFILE_FUNC();

	if(LOG){
		ESS_LOG_WARNING("UpdateParticles at time = "<<t);
	}

	Interval interval = FOREVER;//os->obj->ObjectValidity(t);
	//ESS_LOG_INFO( "Interval Start: " << interval.Start() << " End: " << interval.End() );

    MCHAR const* strPath = NULL;
	this->pblock2->GetValue( AlembicParticles::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock2->GetValue( AlembicParticles::ID_IDENTIFIER, t, strIdentifier, interval);
 
	float fTime;
	this->pblock2->GetValue( AlembicParticles::ID_TIME, t, fTime, interval);

	BOOL bMuted;
	this->pblock2->GetValue( AlembicParticles::ID_MUTED, t, bMuted, interval);

    if (bMuted)
    {
        valid = FALSE;
        return;
    }

	BOOL bRenderAsTicks;
	this->pblock2->GetValue( AlembicParticles::ID_RENDER_AS_TICKS, t, bRenderAsTicks, interval);
	m_bRenderAsTicks = bRenderAsTicks == TRUE;

	std::string szPath = EC_MCHAR_to_UTF8( strPath );
	std::string szIdentifier = EC_MCHAR_to_UTF8( strIdentifier );

	this->pblock2->GetValue( AlembicParticles::ID_VIEWPORT_PERCENT, t, m_fViewportPercent, interval);

	//AbcG::IPoints iPoints;
    if (!GetAlembicIPoints(m_iPoints, szPath, szIdentifier))
    {
        valid = FALSE;
        return;
    }

    if(tvalid == t && valid == TRUE)
    {
        return;
    }

	int iSampleTime = GetTimeValueFromSeconds( fTime );

	m_currTick = t;

	m_outputOrientationMotionBlurWarning = true;

    AbcG::IPointsSchema::Sample floorSample;
    AbcG::IPointsSchema::Sample ceilSample;
    SampleInfo sampleInfo = GetSampleAtTime(m_iPoints, iSampleTime, floorSample, ceilSample);

    int numParticles = GetNumParticles(floorSample);
    parts.SetCount(numParticles, PARTICLE_VELS | PARTICLE_AGES | PARTICLE_RADIUS);
    parts.SetCustomDraw(NULL);

    m_InstanceShapeType.resize(numParticles);
    m_InstanceShapeTimes.resize(numParticles);
    m_InstanceShapeIds.resize(numParticles);
    m_ParticleOrientations.resize(numParticles);
    m_ParticleScales.resize(numParticles);
    m_VCArray.resize(numParticles);
  


    m_objToWorld = node->GetObjTMAfterWSM(t);
 
	GetParticlePositions( m_iPoints, floorSample, ceilSample, sampleInfo, m_objToWorld, parts.points );
	GetParticleVelocities( floorSample, ceilSample, sampleInfo, m_objToWorld, parts.vels );
	GetParticleRadii( m_iPoints, sampleInfo, parts.radius);
	GetParticleAges( m_iPoints, sampleInfo, parts.ages);
	GetParticleOrientations( m_iPoints, sampleInfo, m_objToWorld,  m_ParticleOrientations );
	GetParticleScales( m_iPoints, sampleInfo, m_objToWorld,  m_ParticleScales );
	GetParticleColors(m_iPoints, sampleInfo, m_VCArray);

	GetParticleShapeTypes(m_iPoints, sampleInfo, m_InstanceShapeType );
	GetParticleShapeInstanceIds(m_iPoints, sampleInfo, m_InstanceShapeIds );
	GetParticleShapeInstanceTimes(m_iPoints, sampleInfo, m_InstanceShapeTimes );

    // Find the scene nodes for all our instances
    FillParticleShapeNodes(m_iPoints, sampleInfo);

    clearViewportMeshCache();
    
    tvalid = t;
    valid = TRUE;

	ESS_CPP_EXCEPTION_REPORTING_END
}

void AlembicParticles::BuildEmitter(TimeValue t, Mesh &mesh)
{
    /*Interval interval = FOREVER;//os->obj->ObjectValidity(t);
	//ESS_LOG_INFO( "Interval Start: " << interval.Start() << " End: " << interval.End() );

    MCHAR const* strPath = NULL;
	this->pblock2->GetValue( AlembicParticles::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock2->GetValue( AlembicParticles::ID_IDENTIFIER, t, strIdentifier, interval);

    AbcG::IPoints iPoints;
    if (!GetAlembicIPoints(iPoints, strPath, strIdentifier))
    {
        return;
    }

    AbcG::IPointsSchema::Sample floorSample;
    AbcG::IPointsSchema::Sample ceilSample;
    GetSampleAtTime(iPoints, t, floorSample, ceilSample);

    // Create a box the size of the bounding box
    Abc::Box3d bbox = floorSample.getSelfBounds();
    Abc::V3f alembicMinPt(float(bbox.min.x), float(bbox.min.y), float(bbox.min.z));
    Abc::V3f alembicMaxPt(float(bbox.max.x), float(bbox.max.y), float(bbox.max.z));
    Point3 maxMinPt;
    Point3 maxMaxPt;
    maxMinPt = ConvertAlembicPointToMaxPoint(alembicMinPt);
    maxMaxPt = ConvertAlembicPointToMaxPoint(alembicMaxPt);

    const float EPSILON = 0.0001f;
    Point3 minPt(min(maxMinPt.x, maxMaxPt.x) - EPSILON, min(maxMinPt.y, maxMaxPt.y) - EPSILON, min(maxMinPt.z, maxMaxPt.z) - EPSILON);
    Point3 maxPt(max(maxMinPt.x, maxMaxPt.x) + EPSILON, max(maxMinPt.y, maxMaxPt.y) + EPSILON, max(maxMinPt.z, maxMaxPt.z) + EPSILON);

    mesh.setNumVerts(8);
    mesh.setVert(0, minPt.x, minPt.y, 0);
    mesh.setVert(1, minPt.x, maxPt.y, 0);
    mesh.setVert(2, maxPt.x, maxPt.y, 0);
    mesh.setVert(3, maxPt.x, minPt.y, 0);
    mesh.setVert(4, minPt.x, minPt.y, 0);
    mesh.setVert(5, minPt.x, maxPt.y, 0);
    mesh.setVert(6, maxPt.x, maxPt.y, 0);
    mesh.setVert(7, maxPt.x, minPt.y, 0);

    mesh.setNumFaces(12);
    mesh.faces[0].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[0].setVerts(0, 1, 2);
    mesh.faces[1].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[1].setVerts(2, 3, 0);
    mesh.faces[2].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[2].setVerts(2, 6, 7);
    mesh.faces[3].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[3].setVerts(7, 3, 2);
    mesh.faces[4].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[4].setVerts(1, 5, 6);
    mesh.faces[5].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[5].setVerts(6, 2, 1);
    mesh.faces[6].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[6].setVerts(0, 4, 5);
    mesh.faces[7].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[7].setVerts(5, 1, 0);
    mesh.faces[8].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[8].setVerts(3, 7, 4);
    mesh.faces[9].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[9].setVerts(4, 0, 3);
    mesh.faces[10].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[10].setVerts(7, 6, 5);
    mesh.faces[11].setEdgeVisFlags(TRUE, TRUE, FALSE);
    mesh.faces[11].setVerts(5, 4, 7);

    mesh.buildNormals();
    mesh.InvalidateGeomCache();
    mesh.InvalidateTopologyCache();
    */

    mvalid = Interval(t ,t);
}

int AlembicParticles::RenderBegin(TimeValue t, ULONG flags)
{
    SetAFlag(A_RENDER);
    ParticleInvalid();		
    NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
    return 0;
}
    
int AlembicParticles::RenderEnd(TimeValue t)
{
    ClearAFlag(A_RENDER);
	ParticleInvalid();		
	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	return 0;
}

Interval AlembicParticles::GetValidity(TimeValue t)
{
    Interval interval(t, t);
    return interval;
}

MarkerType AlembicParticles::GetMarkerType()
{
    return POINT_MRKR;
}

Point3 AlembicParticles::ParticlePosition(TimeValue t,int i) {
	//ESS_LOG_INFO( "AlembicParticles::ParticlePosition( t: "<<t<<"   i: " << i << " )" );
	if( i < parts.points.Count() ){
		return parts.points[i];
	}
	else{
		ESS_LOG_INFO("ParticlePosition out of bounds at "<<i);
		return Point3(0.0f, 0.0f, 0.0f);
	}
}
/**		Returns the velocity of the specified particle at the time passed (in 3ds
Max units per tick). This is specified as a vector. The Particle Age
texture map and the Particle Motion Blur texture map use this method.
\param t The time to return the particle velocity.
\param i The index of the particle. */
Point3 AlembicParticles::ParticleVelocity(TimeValue t,int i) {
	//ESS_LOG_INFO( "AlembicParticles::ParticleVelocity( t: "<<t<<"   i: " << i << " )" );
	if( i < parts.vels.Count() ){
		return parts.vels[i];
	}
	else{
		ESS_LOG_INFO("ParticleVelocity out of bounds at "<<i);
		return Point3(0.0f, 0.0f, 0.0f);
	}
}

/**		Returns the world space size of the specified particle in at the time
passed. 
The Particle Age texture map and the Particle Motion Blur texture map use
this method.
\param t The time to return the particle size.
\param i The index of the particle. */
float AlembicParticles::ParticleSize(TimeValue t,int i) {
	if( i < parts.radius.Count()) {
 		return parts.radius[i];
	}
	else{
		ESS_LOG_INFO("ParticleSize out of bounds at "<<i);
		return 0.0f;
	}
}
/**		Returns a value indicating where the particle geometry (mesh) lies in
relation to the particle position. 
This is used by Particle Motion Blur for example. It gets the point in
world space of the point it is shading, the size of the particle from
ParticleSize(), and the position of the mesh from
ParticleCenter(). Given this information, it can know where the
point is, and it makes the head and the tail more transparent.
\param t The time to return the particle center.
\param i The index of the particle.
\return  One of the following: 
\ref PARTCENTER_HEAD 
The particle geometry lies behind the particle position. 
\ref PARTCENTER_CENTER 
The particle geometry is centered around particle position. 
\ref PARTCENTER_TAIL 
The particle geometry lies in front of the particle position. */
int AlembicParticles::ParticleCenter(TimeValue t,int i) {
	return PARTCENTER_CENTER;
}
/**	Returns the age of the specified particle -- the length of time it has been
'alive'. 
The Particle Age texture map and the Particle Motion Blur texture map use
this method.
\param t Specifies the time to compute the particle age.
\param i The index of the particle. */
TimeValue AlembicParticles::ParticleAge(TimeValue t, int i) {
	if( i < parts.ages.Count() ){
		return parts.ages[i];
	}
	else{
		ESS_LOG_INFO("ParticleAge out of bounds at "<<i);
		return TimeValue(0);
	}
}
/**		Returns the life of the particle -- the length of time the particle will be
'alive'. 
The Particle Age texture map and the Particle Motion Blur texture map use
this method.
\param t Specifies the time to compute the particle life span.
\param i The index of the particle. */
TimeValue AlembicParticles::ParticleLife(TimeValue t, int i) {
	return -1;
}

bool AlembicParticles::GetAlembicIPoints(AbcG::IPoints &iPoints, std::string strFile, std::string strIdentifier)
{
    ESS_PROFILE_FUNC();
    // Find the object in the archive
    AbcG::IObject iObj = getObjectFromArchive(strFile, strIdentifier);
	 if (!iObj.valid())
    {
		return false;
    }

    iPoints = AbcG::IPoints(iObj, Abc::kWrapExisting);
    if (!iPoints.valid())
    {
        return false;
    }

    return true;
}

SampleInfo AlembicParticles::GetSampleAtTime(AbcG::IPoints &iPoints, TimeValue t, AbcG::IPointsSchema::Sample &floorSample, AbcG::IPointsSchema::Sample &ceilSample) const
{
    ESS_PROFILE_FUNC();
    double sampleTime = GetSecondsFromTimeValue(t);
    SampleInfo sampleInfo = getSampleInfo(sampleTime,
                                          iPoints.getSchema().getTimeSampling(),
                                          iPoints.getSchema().getNumSamples());

    iPoints.getSchema().get(floorSample, sampleInfo.floorIndex);
    iPoints.getSchema().get(ceilSample, sampleInfo.ceilIndex);

    return sampleInfo;
}

int AlembicParticles::GetNumParticles(const AbcG::IPointsSchema::Sample &floorSample) const
{
    // We assume that the number of position items is the number of particles
    return static_cast<int>(floorSample.getPositions()->size());
}

void
AlembicParticles::GetParticlePositions(AbcG::IPoints &iPoints, const AbcG::IPointsSchema::Sample &floorSample, const AbcG::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, const Matrix3& objToWorld, Tab<Point3>& points) const {
	ESS_PROFILE_FUNC();
    Abc::P3fArraySamplePtr floorPositions = floorSample.getPositions();
	if( floorPositions != NULL && floorPositions->valid() && floorPositions->size() > 0 ) {
		std::vector<Abc::V3f> alembicPositions( points.Count() );
		int j = 0;
		int jIncrement = floorPositions->size() == points.Count() ? 1 : 0;
		for( int i = 0; i < floorPositions->size(); i ++ ) {
			alembicPositions[i] = (*floorPositions)[j];
			j += jIncrement;
		}


		//Get the velocity if there is an alpha
		if (sampleInfo.alpha != 0.0f)
		{
			float timeAlpha = getTimeOffsetFromObject( iPoints, sampleInfo );

			Abc::V3fArraySamplePtr floorVelocities = floorSample.getVelocities();
			if( floorVelocities != NULL && floorVelocities->size() > 0 ) {
				j = 0;
				jIncrement = floorVelocities->size() == points.Count() ? 1 : 0;
				for( int i = 0; i < alembicPositions.size(); i ++ ) {
					alembicPositions[i] += timeAlpha * (*floorVelocities)[j];	 	
					j += jIncrement;
				}
			}
		}

		const float fLimit = FLT_MAX/5;

		for( int i = 0; i < alembicPositions.size(); i ++ ) {
			//bool bOutOfBounds = false;
			//if(alembicPositions[i].x > fLimit){
			//	alembicPositions[i].x = fLimit;
			//	bOutOfBounds = true;
			//}
			//if(alembicPositions[i].y > fLimit){
			//	alembicPositions[i].y = fLimit;
			//	bOutOfBounds = true;
			//}	
			//if(alembicPositions[i].z > fLimit){
			//	alembicPositions[i].z = fLimit;
			//	bOutOfBounds = true;
			//}

			//if(bOutOfBounds){
			//	ESS_LOG_INFO("Warning: 3DS max grid rendering breaks when a point is too large. Point "<<i<<" is too large, and has been capped to a safe limit.");
			//}

			points[i] = ConvertAlembicPointToMaxPoint( alembicPositions[i] ) * objToWorld;
		}
	}
}
void
AlembicParticles::GetParticleVelocities(const AbcG::IPointsSchema::Sample &floorSample, const AbcG::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, Matrix3 objToWorld, Tab<Point3>& vels) const {
	//ESS_LOG_WARNING( "Particle velocities are not transformed into world space." );
    ESS_PROFILE_FUNC();
	objToWorld.SetTrans(Point3(0.0, 0.0, 0.0));

	bool useDefaultValues = true;
	Abc::V3fArraySamplePtr floorVelocities = floorSample.getVelocities();
	if( floorVelocities != NULL && floorVelocities->valid() && floorVelocities->size() > 0 ) {
		int j = 0;
		int jIncrement = floorVelocities->size() == vels.Count() ? 1 : 0;
		for( int i = 0; i < vels.Count(); i ++ ) {
			vels[i] = (ConvertAlembicPointToMaxPoint( (*floorVelocities)[j] ) / TIME_TICKSPERSEC ) * objToWorld;
			j += jIncrement;
		}
		useDefaultValues = false;
	}
	if( useDefaultValues ) {
  		for( int i = 0; i < vels.Count(); i ++ ) {
			vels[i] = Point3( 0, 0, 0 );
		}
	}
}


void
AlembicParticles::GetParticleRadii(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, Tab<float>& radius) const {
   ESS_PROFILE_FUNC();
	bool useDefaultValues = true;
    AbcG::IFloatGeomParam widthsParam = iPoints.getSchema().getWidthsParam();
   if( widthsParam != NULL && widthsParam.getNumSamples() > 0 ) {
		Abc::FloatArraySamplePtr floorSamples = widthsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
		if( floorSamples != NULL && floorSamples->valid() && floorSamples->size() > 0 ) {
			int j = 0;
			int jIncrement = floorSamples->size() == radius.Count() ? 1 : 0;
			for( int i = 0; i < radius.Count(); i ++ ) {
				radius[i] = (*floorSamples)[j];
				j += jIncrement;
			}
			useDefaultValues = false;
		}
	}
 	if( useDefaultValues ) {
	  	for( int i = 0; i < radius.Count(); i ++ ) {
			radius[i] = 1.0f;
		}
	}
}

void
AlembicParticles::GetParticleAges(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, Tab<TimeValue>& ages) const {
    ESS_PROFILE_FUNC();
	bool useDefaultValues = true;
	Abc::IFloatArrayProperty prop;
	if( getArbGeomParamPropertyAlembic( iPoints, "age", prop ) ) {
	    Abc::FloatArraySamplePtr floorSamples = prop.getValue(sampleInfo.floorIndex);
		if( floorSamples != NULL && floorSamples->size() > 0  && floorSamples->size() == ages.Count() ) {
			int j = 0;
			int jIncrement = floorSamples->size() == ages.Count() ? 1 : 0;
			for( int i = 0; i < ages.Count(); i ++ ) {
				ages[i] = GetTimeValueFromSeconds( (*floorSamples)[j] );
				j += jIncrement;
			}
			useDefaultValues = false;
		}
	}
	if( useDefaultValues ) {
		for( int i = 0; i < ages.Count(); i ++ ) {
			ages[i] = 0;
		}
	}
}

void
AlembicParticles::GetParticleOrientations(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, Matrix3 objToWorld, std::vector<Quat>& particleOrientations) const {
   ESS_PROFILE_FUNC();
	//ESS_LOG_WARNING( "Particle orientations are not transformed into world space." );
	bool useDefaultValues = true;

	objToWorld.SetTrans(Point3(0.0, 0.0, 0.0));

	Abc::IQuatfArrayProperty orientProperty;
	if( getArbGeomParamPropertyAlembic( iPoints, "orientation", orientProperty ) ) {
		Abc::QuatfArraySamplePtr floorSamples = orientProperty.getValue(sampleInfo.floorIndex);
		if( floorSamples != NULL && floorSamples->valid() && floorSamples->size() > 0 ) {

			std::vector<Quat> maxOrientations( particleOrientations.size() );
			int j = 0;
			int jIncrement = floorSamples->size() == particleOrientations.size() ? 1 : 0;
			for( int i = 0; i < maxOrientations.size(); i ++ ) {
				Quat q = ConvertAlembicQuatToMaxQuat( (*floorSamples)[j], true);
				j += jIncrement;
				q.Normalize();
				maxOrientations[i] = q;
			}

			//Get the velocity if there is an alpha
			if (sampleInfo.alpha != 0.0f)
			{
				Abc::IQuatfArrayProperty angVelProperty ;
				if( getArbGeomParamPropertyAlembic( iPoints, "angularvelocity", angVelProperty ) ) {
					float timeAlpha = getTimeOffsetFromObject( iPoints, sampleInfo );
							
					Abc::QuatfArraySamplePtr floorAngVelSamples = angVelProperty.getValue(sampleInfo.floorIndex);
					if( floorAngVelSamples != NULL && floorAngVelSamples->size() > 0 ) {
						j = 0;
						jIncrement = floorAngVelSamples->size() == particleOrientations.size() ? 1 : 0;
						for( int i = 0; i < maxOrientations.size(); i ++ ) {
							Quat q = maxOrientations[i];
							Quat v = ConvertAlembicQuatToMaxQuat( (*floorAngVelSamples)[j], false);
							j += jIncrement;
							v = v * timeAlpha;
							if (v.w != 0.0f) {
								q = v * q;
							}
							q.Normalize();
							maxOrientations[i] = q;
						}
					}
				}
			}

			for( int i = 0; i < maxOrientations.size(); i ++ ) {
				particleOrientations[i] = maxOrientations[i] * objToWorld;
			}
			useDefaultValues = false;
		}
	}
	if( useDefaultValues ) {
		for( int i = 0; i < particleOrientations.size(); i ++ ) {
			particleOrientations[i] = Quat( 0.0f, 0.0f, 0.0f, 1.0f );
		}
	}
}

void
AlembicParticles::GetParticleScales(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, const Matrix3& objToWorld, std::vector<Point3>& scales) const {
   ESS_PROFILE_FUNC();
	//ESS_LOG_WARNING( "Particle scales are not transformed into world space ??" );
	bool useDefaultValues = true;
	Abc::IV3fArrayProperty prop;
	if( getArbGeomParamPropertyAlembic( iPoints, "scale", prop ) ) {
	    Abc::V3fArraySamplePtr floorSamples = prop.getValue(sampleInfo.floorIndex);
		if( floorSamples != NULL && floorSamples->size() > 0 ) {
			int j = 0;
			int jIncrement = floorSamples->size() == scales.size() ? 1 : 0;
			for( int i = 0; i < scales.size(); i ++ ) {
				scales[i] = ConvertAlembicScaleToMaxScale( (*floorSamples)[j] );
				j += jIncrement;									
			}
			useDefaultValues = false;
		}
	}
	if( useDefaultValues ) {
		for( int i = 0; i < scales.size(); i ++ ) {
			scales[i] = Point3( 1, 1, 1 );
		}
	}
}


void
AlembicParticles::GetParticleShapeTypes(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, std::vector<AlembicPoints::ShapeType>& instanceShapeType ) const {
   ESS_PROFILE_FUNC();
	bool useDefaultValues = true;
	Abc::IUInt16ArrayProperty shapeTypeProperty;
	if( getArbGeomParamPropertyAlembic( iPoints, "shapetype", shapeTypeProperty ) ) {
	    Abc::UInt16ArraySamplePtr floorSamples = shapeTypeProperty.getValue(sampleInfo.floorIndex);
		if( floorSamples != NULL && floorSamples->size() > 0 ) {
			int j = 0;
			int jIncrement = floorSamples->size() == instanceShapeType.size() ? 1 : 0;
			for( int i = 0; i < instanceShapeType.size(); i ++ ) {
				instanceShapeType[i] =  static_cast<AlembicPoints::ShapeType>( (*floorSamples)[j] );
				j += jIncrement;
			}
			useDefaultValues = false;
		}
	}
	if( useDefaultValues ) {
		for( int i = 0; i < instanceShapeType.size(); i ++ ) {
			instanceShapeType[i] = AlembicPoints::ShapeType_Point;
		}
	}
}


void
AlembicParticles::GetParticleShapeInstanceIds(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, std::vector<unsigned short>& instanceShapeIds ) const {
   ESS_PROFILE_FUNC();
	bool useDefaultValues = true;
	Abc::IUInt16ArrayProperty shapeIdProperty;
	if( getArbGeomParamPropertyAlembic( iPoints, "shapeinstanceid", shapeIdProperty ) ) {
		Abc::IStringArrayProperty shapeNameProperty;
		if( getArbGeomParamPropertyAlembic( iPoints, "instancenames", shapeNameProperty ) ) {	
			Abc::UInt16ArraySamplePtr floorSamples = shapeIdProperty.getValue(sampleInfo.floorIndex);
			if( floorSamples != NULL && floorSamples->size() > 0 ) {
				int j = 0;
				int jIncrement = floorSamples->size() == instanceShapeIds.size() ? 1 : 0;
				for( int i = 0; i < instanceShapeIds.size(); i ++ ) {
					instanceShapeIds[i] =  (*floorSamples)[j];
					j += jIncrement;
				}
				useDefaultValues = false;
			}
		}
	}
	if( useDefaultValues ) {
		for( int i = 0; i < instanceShapeIds.size(); i ++ ) {
			instanceShapeIds[i] = 0;
		}
	}
}

 

void AlembicParticles::FillParticleShapeNodes(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo)
{
   ESS_PROFILE_FUNC();
    //m_TotalShapesToEnumerate = 0;
    m_InstanceShapeINodes.clear();

	Abc::IStringArrayProperty shapeInstanceNameProperty;
	if( getArbGeomParamPropertyAlembic( iPoints, "instancenames", shapeInstanceNameProperty ) ) {		
		const size_t sIndex = shapeInstanceNameProperty.getNumSamples()-1;
		m_InstanceShapeNames = shapeInstanceNameProperty.getValue(sIndex);//sampleInfo.floorIndex);

		if (m_InstanceShapeNames == NULL || m_InstanceShapeNames->size() == 0)
		{
			return;
		}

		//m_TotalShapesToEnumerate = m_InstanceShapeNames->size();
		m_InstanceShapeINodes.resize(m_InstanceShapeNames->size());

        INodeMap nodeMap;
        buildINodeMap(nodeMap);

		for (int i = 0; i < m_InstanceShapeINodes.size(); i += 1)
		{
			const std::string& path = m_InstanceShapeNames->get()[i];
            m_InstanceShapeINodes[i] = nodeMap[path];

            if(m_InstanceShapeINodes[i] == NULL){
               const std::string newPath = alembicPathToMaxPath(path);
               m_InstanceShapeINodes[i] = nodeMap[newPath];
            }
		}
	}
}


void
AlembicParticles::GetParticleShapeInstanceTimes(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, std::vector<TimeValue>& instanceShapeTimes) const {
   ESS_PROFILE_FUNC();
	bool useDefaultValues = true;
	Abc::IFloatArrayProperty shapeTimeProperty;
	if( getArbGeomParamPropertyAlembic( iPoints, "shapetime", shapeTimeProperty ) ) {
	    Abc::FloatArraySamplePtr floorSamples = shapeTimeProperty.getValue(sampleInfo.floorIndex);
		if( floorSamples != NULL && floorSamples->size() > 0 ) {
			int j = 0;
			int jIncrement = floorSamples->size() == instanceShapeTimes.size() ? 1 : 0;
			for( int i = 0; i < instanceShapeTimes.size(); i ++ ) {
				instanceShapeTimes[i] = GetTimeValueFromSeconds( (*floorSamples)[j] );
				j += jIncrement;
			}
			useDefaultValues = false;
		}
	}
	if( useDefaultValues ) {
		for( int i = 0; i < instanceShapeTimes.size(); i ++ ) {
			instanceShapeTimes[i] = TimeValue( 0 );
		}
	}
}

void
AlembicParticles::GetParticleColors(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, std::vector<VertColor>& colors ) const {
   ESS_PROFILE_FUNC();
	bool useDefaultValues = true;
	Abc::IC4fArrayProperty colorProperty;
	if( getArbGeomParamPropertyAlembic( iPoints, "color", colorProperty ) ) {
	    Abc::C4fArraySamplePtr floorSamples = colorProperty.getValue(sampleInfo.floorIndex);
		if( floorSamples != NULL && floorSamples->size() > 0 ) {
			int j = 0;
			int jIncrement = floorSamples->size() == colors.size() ? 1 : 0;
			for( int i = 0; i < colors.size(); i ++ ) {
				Abc::C4f color = (*floorSamples)[j];					
				j += jIncrement;
				colors[i] = VertColor( color.r, color.g, color.b );
			}
			useDefaultValues = false;
		}
	}
	if( useDefaultValues ) {
		for( int i = 0; i < colors.size(); i ++ ) {
			colors[i] = VertColor(0.5, 0.5, 0.5);
		}
	}
}


int AlembicParticles::NumberOfRenderMeshes()
{
	//ESS_LOG_INFO( "AlembicParticles::NumberOfRenderMeshes()" );
	int count = parts.Count();
	return count;
}

Mesh* AlembicParticles::GetMultipleRenderMesh(TimeValue  t,  INode *inode,  View &view,  BOOL &needDelete,  int meshNumber)
{
	//ESS_LOG_INFO( "AlembicParticles::GetMultipleRenderMesh( t: " << t << " meshNumber: " << meshNumber << ", t: " << t << " )" );
	return GetMultipleRenderMesh_Internal(t, inode, view, needDelete, meshNumber, false);
}

Mesh* AlembicParticles::GetMultipleRenderMesh_Internal(TimeValue  t,  INode *inode,  View &view,  BOOL &needDelete,  int meshNumber, bool bUseCache)
{
   ESS_PROFILE_FUNC();
    if (meshNumber >= parts.Count() || !parts.Alive(meshNumber) || view.CheckForRenderAbort())
    {
        needDelete = NULL;
        return NULL;
    }

    Mesh *pMesh = NULL;
    switch (m_InstanceShapeType[meshNumber])
    {
    case AlembicPoints::ShapeType_Point:
        pMesh = BuildPointMesh(meshNumber, t, inode, view, needDelete);
        break;
    case AlembicPoints::ShapeType_Box:
        pMesh = BuildBoxMesh(meshNumber, t, inode, view, needDelete);
        break;
    case AlembicPoints::ShapeType_Sphere:
        pMesh = BuildSphereMesh(meshNumber, t, inode, view, needDelete);
        break;
    case AlembicPoints::ShapeType_Cylinder:
        pMesh = BuildCylinderMesh(meshNumber, t, inode, view, needDelete);
        break;
    case AlembicPoints::ShapeType_Cone:
        pMesh = BuildConeMesh(meshNumber, t, inode, view, needDelete);
        break;
    case AlembicPoints::ShapeType_Disc:
        pMesh = BuildDiscMesh(meshNumber, t, inode, view, needDelete);
        break;
    case AlembicPoints::ShapeType_Rectangle:
        pMesh = BuildRectangleMesh(meshNumber, t, inode, view, needDelete);
        break;
    case AlembicPoints::ShapeType_Instance:
        pMesh = BuildInstanceMesh(meshNumber, t, inode, view, needDelete, bUseCache);
        break;
    case AlembicPoints::ShapeType_NbElements:
        pMesh = BuildNbElementsMesh(meshNumber, t, inode, view, needDelete);
        break;
    default:
	    pMesh = NULL;
        needDelete = FALSE;
	    break;
    }

	if(pMesh && m_VCArray.size() > 0 ){
		inode->SetVertexColorType(nvct_map_channel);
		inode->SetVertexColorMapChannel(0);
		pMesh->setVCDisplayData(inode->GetVertexColorMapChannel());

		VertColor color = m_VCArray[meshNumber];

		//TODO: clear first?
		pMesh->setMapSupport(0, TRUE);
		MeshMap& map = pMesh->Map(0);
		map.setNumVerts(1);
		map.tv[0] = color;
		const int nNumFaces = pMesh->getNumFaces();
		map.setNumFaces(nNumFaces);
		for(int i=0; i<nNumFaces; i++){
			map.tf[i].setTVerts(0, 0, 0);
		}
	}

    return pMesh;
}

Mesh* AlembicParticles::GetRenderMesh(TimeValue t, INode *inode, View &view, BOOL &needDelete)
{
    //ESS_LOG_WARNING("GetRenderMesh called.");

    ESS_PROFILE_FUNC();
	if (m_currTick != t){
		Update(t,inode);
	}

	ExoNullView nullView;

	//Based upon the PFOperatorRender.cpp code
	
	Mesh* renderMesh = new Mesh();
	needDelete = true;
	int vertNum = 0;
	int faceNum = 0;
    int normNum = 0;

    {

    ESS_PROFILE_SCOPE("GetRenderMesh vertNum and faceNum");

	for(int i=0; i<NumberOfRenderMeshes(); i++)
	{
		BOOL curNeedDelete = FALSE;
		Mesh* pMesh = GetMultipleRenderMesh_Internal(t, inode, nullView, curNeedDelete, i);

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

        MeshNormalSpec *pNormalSpec = pMesh->GetSpecifiedNormals();
        if(pNormalSpec){
           normNum += pNormalSpec->GetNumNormals();
        }

		vertNum += pMesh->getNumVerts();
		faceNum += pMesh->getNumFaces();
	}

    }

	if(!renderMesh->setNumVerts(vertNum)){
		return renderMesh;
	}
	if(!renderMesh->setNumFaces(faceNum)){
		return renderMesh;
	}

	//MeshNormalSpec* pMeshNormalSpec = renderMesh->GetSpecifiedNormals();
    MeshNormalSpec* pRenderMeshNormalSpec = NULL;
    {
    ESS_PROFILE_SCOPE("GetRenderMesh specifyNormals");

	renderMesh->SpecifyNormals();
	pRenderMeshNormalSpec = renderMesh->GetSpecifiedNormals();
	pRenderMeshNormalSpec->SetParent(renderMesh);

	pRenderMeshNormalSpec->SetAllExplicit(true);
	pRenderMeshNormalSpec->SetNumFaces(faceNum);
	pRenderMeshNormalSpec->SetNumNormals(normNum);
    }

    MeshNormalFace *mpFace = &pRenderMeshNormalSpec->Face(0);
	Point3 *mpNormal = &pRenderMeshNormalSpec->Normal(0);

	//the mesh should be relative to the particle frame
	Matrix3 inverseTM = Inverse(inode->GetObjectTM(t));
	//inverseTM.IdentityMatrix();

	int tvertOffset[MAX_MESHMAPS];
	for(int i=0; i<MAX_MESHMAPS; i++){
		tvertOffset[i] = 0;
	}

	TVFace tvFace;
	tvFace.setTVerts(0, 0, 0);

	int vertOffset=0;
	int faceOffset=0;
    int normOffset=0;
    int normIndex=0;
	for(int i=0; i<NumberOfRenderMeshes(); i++)
	{
		BOOL curNeedDelete = FALSE;
		Mesh* pMesh = GetMultipleRenderMesh_Internal(t, inode, nullView, curNeedDelete, i);

		if(!pMesh){
			continue;
		}

		int curNumVerts = pMesh->getNumVerts();
		if(curNumVerts == 0){
            ESS_PROFILE_SCOPE("GetRenderMesh currNumVerts == 0");
			if (curNeedDelete) {
				pMesh->FreeAll();
				delete pMesh;
			}
			pMesh = NULL;
			continue;
		}

		int curNumFaces = pMesh->getNumFaces();
		Matrix3 meshTM;
		meshTM.IdentityMatrix();
		Interval meshTMValid = FOREVER;
        {
        ESS_PROFILE_SCOPE("GetRenderMesh::GetMultipleRenderMesh");
		GetMultipleRenderMeshTM_Internal(t, inode, nullView, i, meshTM, meshTMValid);
        }

        {
           ESS_PROFILE_SCOPE("GetRenderMesh init verts and faces");

		for(int j=0, curIndex=vertOffset; j<curNumVerts; j++, curIndex++) {
			renderMesh->verts[curIndex] = pMesh->verts[j] * meshTM * inverseTM;
		}
		for(int j=0, curIndex=faceOffset; j<curNumFaces; j++, curIndex++) {
			renderMesh->faces[curIndex] = pMesh->faces[j];
			for(int k=0; k<3; k++) {
				renderMesh->faces[curIndex].v[k] += vertOffset;
				//pRenderMeshNormalSpec->Face(curIndex).SetNormalID(k, 0);
				//pRenderMeshNormalSpec->Face(curIndex).SpecifyAll(true);
			}
		}

        }

		//for transforming the normals
		Matrix3 meshTM_I_T = meshTM * inverseTM;
		meshTM_I_T.SetTrans(Point3(0.0, 0.0, 0.0));
		//the following two steps are necessary because meshTM can contain a scale factor
		meshTM_I_T = Inverse(meshTM_I_T);
		meshTM_I_T = TransposeRot(meshTM_I_T);

        {
        ESS_PROFILE_SCOPE("GetRenderMesh - Build and set normals");

		MeshNormalSpec *pNormalSpec = pMesh->GetSpecifiedNormals();
		if(pNormalSpec && curNumFaces == pNormalSpec->GetNumFaces() ){
            ESS_PROFILE_SCOPE("GetRenderMesh - Build and set normals - copy normals from normal spec");

			for(int j=0, curIndex=faceOffset; j<curNumFaces; j++, curIndex++){
                mpFace[curIndex].SetNormalID(0, pNormalSpec->Face(j).GetNormalID(0) + normOffset);
                mpFace[curIndex].SetNormalID(1, pNormalSpec->Face(j).GetNormalID(1) + normOffset);
                mpFace[curIndex].SetNormalID(2, pNormalSpec->Face(j).GetNormalID(2) + normOffset);
                mpFace[curIndex].SetSpecified(0, true);
                mpFace[curIndex].SetSpecified(1, true);
                mpFace[curIndex].SetSpecified(2, true);
			}
            
            normOffset += pNormalSpec->GetNumNormals();

            for(int j=0; j<pNormalSpec->GetNumNormals(); j++){
               mpNormal[normIndex] = pNormalSpec->Normal(j) * meshTM_I_T;
               normIndex++;
            }
		}
		else{

			//int n = pMesh->getNumVerts()
            ESS_PROFILE_SCOPE("GetRenderMesh - Build and set normals - compute from smoothing groups");

			SmoothGroupNormals sgNormals;
			sgNormals.BuildMeshSmoothingGroupNormals(*pMesh);
			for(int j=0, curIndex=faceOffset; j<curNumFaces; j++, curIndex++){
				for(int f=0; f<3; f++){
					pRenderMeshNormalSpec->SetNormal(curIndex, f, sgNormals.GetVertexNormal(pMesh, j, f) * meshTM_I_T);
				}
			}

			//ESS_LOG_WARNING("Particle mesh has invalid normals.");
			//for(int j=0, curIndex=faceOffset; j<curNumFaces; j++, curIndex++){
			//	for(int f=0; f<3; f++){
			//		pRenderMeshNormalSpec->SetNormal(curIndex, f, Point3(0.0, 0.0, 0.0));
			//	}
			//}
		}

        }


        {
        ESS_PROFILE_SCOPE("GetRenderMesh copy over mapping channels");

		int numMaps = pMesh->getNumMaps();
		for(int mp=0; mp<numMaps; mp++) {
			int tvertsToAdd = pMesh->mapSupport(mp) ? pMesh->getNumMapVerts(mp) : 0;
			if (tvertsToAdd == 0) {
				continue;
			}
			if (tvertOffset[mp] == 0) { // the map channel needs expansion
				renderMesh->setMapSupport(mp, TRUE);
				int numMapVerts = 3*faceNum+1; // triple number of faces covers the maximum + one extra
				renderMesh->setNumMapVerts(mp, numMapVerts);
				for (int j=0; j < numMapVerts; j++) { // zero out verts
					renderMesh->setMapVert(mp, j, Point3::Origin);	
				}
				renderMesh->setNumMapFaces(mp, faceNum);
				for(int j=0; j<faceNum; j++){  // zero out vertex indices
					renderMesh->mapFaces(mp)[j] = tvFace;
				}
				tvertOffset[mp] = 1;
			}
			// verify that the tverts array is in proper array range
			if (tvertOffset[mp] + tvertsToAdd > renderMesh->getNumMapVerts(mp)) {
				renderMesh->setNumMapVerts(mp, tvertOffset[mp] + tvertsToAdd, TRUE);
			}
			for(int j=0, curIndex=tvertOffset[mp]; j<tvertsToAdd; j++, curIndex++) {
				renderMesh->setMapVert(mp, curIndex, pMesh->mapVerts(mp)[j] );
			}
			for(int j=0, curIndex=faceOffset; j<curNumFaces; j++, curIndex++) {
				renderMesh->mapFaces(mp)[curIndex] = pMesh->mapFaces(mp)[j];
				for(int k=0; k<3; k++) {
					renderMesh->mapFaces(mp)[curIndex].t[k] += tvertOffset[mp];
				}
			}
			tvertOffset[mp] += tvertsToAdd;
		}

        }
		
		if (curNeedDelete) {
            ESS_PROFILE_SCOPE("GetRenderMesh free after copying");
			pMesh->FreeAll();
			delete pMesh;
		}

		vertOffset += curNumVerts;
		faceOffset += curNumFaces;
	}

   pRenderMeshNormalSpec->SetFlag(MESH_NORMAL_NORMALS_BUILT, TRUE);
   pRenderMeshNormalSpec->SetFlag(MESH_NORMAL_NORMALS_COMPUTED, TRUE);


	//pRenderMeshNormalSpec->CheckNormals();
 //   {
 //   ESS_PROFILE_SCOPE("GetRenderMesh renderMesh->buildNormals");
	////renderMesh->buildNormals();
 //   }

	return renderMesh;
}

void AlembicParticles::GetMultipleRenderMeshTM(TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid)
{
   ESS_PROFILE_FUNC();
	MSTR rendererName;
	GET_MAX_INTERFACE()->GetCurrentRenderer()->GetClassName( rendererName );
	std::string renderer( EC_MSTR_to_UTF8( rendererName ) );

	if(LOG){
		ESS_LOG_WARNING("GetMultipleRenderMeshTM_Internal at time = "<<t<<", mesh #:"<<meshNumber<<", numOfParticles: "<<parts.Count());
	}


	GetMultipleRenderMeshTM_Internal(t, inode, view, meshNumber, meshTM, meshTMValid);

	

	Matrix3 objectToWorld = inode->GetObjectTM( t );

	if( renderer.find( std::string( "mental ray Renderer" ) ) !=std::string::npos ) {
		//ESS_LOG_INFO( "MR renderer mode" );
		
		meshTM = meshTM * Inverse( objectToWorld );
	}
	else if( renderer.find( std::string( "iray" ) ) !=std::string::npos ) {
		//ESS_LOG_INFO( "MR renderer mode" );
		meshTM = meshTM * Inverse( objectToWorld );
	}
	else if(( renderer.find( std::string( "vray" ) ) != std::string::npos )||
		( renderer.find( std::string( "Vray" ) ) != std::string::npos )||
		( renderer.find( std::string( "VRay" ) ) != std::string::npos ) ) {
		//ESS_LOG_INFO( "Vray renderer mode" );
		meshTM = meshTM * Inverse( objectToWorld );
	} 
	else {
		//ESS_LOG_INFO( "Default renderer mode" );
		meshTM = Inverse( objectToWorld ) * meshTM;
	}

 
	meshTMValid.SetInstant( t );
}

void AlembicParticles::GetMultipleRenderMeshTM_Internal(TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid, bool bCalledFromViewport)
{
   ESS_PROFILE_FUNC();
	if(meshNumber >= parts.Count()){
		meshTM.IdentityMatrix();
		return;
	}
	
	//if(!bCalledFromViewport){
	//	ESS_LOG_INFO("IAlembicParticlesExt::GetMultipleRenderMeshTM_Internal() - t: "<<t<<"  currTick: "<<m_currTick);
	//}

    // Calculate the matrix
    Point3 pos = parts.points[meshNumber];
    Quat orient = m_ParticleOrientations[meshNumber];
	Point3 scaleVec = m_ParticleScales[meshNumber];

	//for scanline motion blur we need to be able sample at numerous times. So we can't read from the stored state directly
	//also, Mental Ray seems to sample at some offset before the current tick, but only once per particle
	//the velocity array is used instead of multiple calls to the MeshTm method
	if(t != m_currTick){
		const float tDiff = static_cast<float>(GetSecondsFromTimeValue(t) - GetSecondsFromTimeValue(m_currTick));
		
		pos += parts.vels[meshNumber] * TIME_TICKSPERSEC * tDiff;

		if(m_outputOrientationMotionBlurWarning){
			ESS_LOG_WARNING( "Not advancing orientation of particles for sub-sample motion blur." );
			m_outputOrientationMotionBlurWarning = false;
		}

		//orient *= GetParticleOrientation(m_iPoints, sampleInfo, meshNumber);
		//TODO: we could possibly do it this way as well:
		//Point3 pos2 = pos + static_cast<float>(sampleInfo.alpha) * parts.vels[meshNumber];
	}

	//TODO: why is the radius 0 when calling during export?
	if(parts.radius[meshNumber] != 0.0){
		scaleVec *= parts.radius[meshNumber];
	}

	meshTM.IdentityMatrix();

	if(	m_InstanceShapeType[meshNumber] == AlembicPoints::ShapeType_Box ||
		m_InstanceShapeType[meshNumber] == AlembicPoints::ShapeType_Cylinder ||
		m_InstanceShapeType[meshNumber] == AlembicPoints::ShapeType_Cone){
		Matrix3 localTrans;
		localTrans.IdentityMatrix();
		//the object size is 2, and we want it centered along z-axis of local frame
		localTrans.SetTrans(Point3(0.0, 0.0, -1.0));
		meshTM = meshTM * localTrans;
	}

	Matrix3 worldTrans;
	worldTrans.IdentityMatrix();
	worldTrans.SetRotate(orient);
	worldTrans.PreScale(scaleVec);
	worldTrans.SetTrans(pos);

	meshTM = meshTM * worldTrans;
}

INode* AlembicParticles::GetParticleMeshNode(int meshNumber, INode *displayNode)
{
   ESS_PROFILE_FUNC();
	//ESS_LOG_INFO( "AlembicParticles::GetParticleMeshNode( meshNumber: " << meshNumber << " )" );
    if (meshNumber > parts.Count() || !parts.Alive(meshNumber))
    {
        return displayNode;
    }

    if (m_InstanceShapeType[meshNumber] == AlembicPoints::ShapeType_Instance)
    {
        if (meshNumber > m_InstanceShapeIds.size())
            return displayNode;

        Abc::uint16_t shapeid = m_InstanceShapeIds[meshNumber];

        if (shapeid > m_InstanceShapeINodes.size())
            return displayNode;

        INode *pNode = m_InstanceShapeINodes[shapeid];
        return pNode;
    }
    else
    {
        return displayNode;
    }
}

//Note: the display and hittest methods called often, so we definitely should have caching
int AlembicParticles::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
   ESS_PROFILE_FUNC();
   if (!OKtoDisplay(t)) 
   {
       return 0;
   }

   BOOL doupdate = ((t!=tvalid)||!valid);
   if (doupdate)
   {
	   ESS_PROFILE_SCOPE("Display::Update(t,inode)");
       Update(t,inode);
   }

   //createViewportMeshCache(t, inode);

   GraphicsWindow *gw = vpt->getGW();
   DWORD rlim  = gw->getRndLimits();

   // Draw emitter
   gw->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY| (rlim&GW_Z_BUFFER) );  //removed BC on 4/29/99 DB

   if (EmitterVisible()) 
   {
       Material *mtl = gw->getMaterial();   
       if (!inode->Selected() && !inode->IsFrozen())
       {
           gw->setColor( LINE_COLOR, mtl->Kd[0], mtl->Kd[1], mtl->Kd[2]);
       }

       gw->setTransform(inode->GetObjTMBeforeWSM(t));  
       mesh.render(gw, &particleMtl, 
           (flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, COMP_ALL);
   }

    Matrix3 objToWorld = inode->GetObjTMAfterWSM(t);
      
	Material nullMaterial;

	//the particles fail to clear properly in 3DS MAX 2010 if the we do not invalidate the viewport
	Rect rect;
	rect.top = 0; rect.bottom = gw->getWinSizeY(); rect.left = 0; rect.right = gw->getWinSizeX();
	vpt->InvalidateRect(rect);

	if(m_fViewportPercent == 0.0){
		return 0;
	}

	const float fFraction = m_fViewportPercent / 100.0f;
	int nNumParticlesToRender = (int)ceil((float)NumberOfRenderMeshes() * fFraction);
	const float fStep = 1 / fFraction;

    

	if(m_bRenderAsTicks){
	   // Draw the particles
	   ExoNullView nullView;
	   //nullView.worldToView = objToWorld;
	   gw->setRndLimits(rlim);
	   for (int ii = 0; ; ii++)
	   {
			int i = (int)floor(ii * fStep);
			if( i >= NumberOfRenderMeshes() ){
				break;
			}

			gw->setColor(FILL_COLOR, m_VCArray[i].x, m_VCArray[i].y, m_VCArray[i].z);
			gw->setTransform(Matrix3(1));
			gw->marker(&parts.points[i], POINT_MRKR);  
	   }
	}
	else{
	   ESS_PROFILE_SCOPE("Display::RenderMeshLoop");

	   // Draw the particles
	   ExoNullView nullView;
	   //nullView.worldToView = objToWorld;
	   gw->setRndLimits(rlim);
	   for (int ii = 0; ; ii++ )
	   {
			int i = (int)floor(ii * fStep);
			if( i >= NumberOfRenderMeshes() ){
				break;
			}
			
			Matrix3 elemToObj;
			elemToObj.IdentityMatrix();

			Interval meshTMValid = FOREVER;

			BOOL deleteMesh = FALSE;

			Mesh *mesh = NULL;
			{
			   ESS_PROFILE_SCOPE("Display::GetRenderMeshTM");
			   GetMultipleRenderMeshTM_Internal(t, inode, nullView, i, elemToObj, meshTMValid, true);
			}
			{
			   ESS_PROFILE_SCOPE("Display::GetMultipleRenderMesh_Internal");
			   mesh = GetMultipleRenderMesh_Internal(t, inode, nullView, deleteMesh, i);
			}

			if(mesh && m_InstanceShapeType[i] != AlembicPoints::ShapeType_Point ){
			    ESS_PROFILE_SCOPE("Display::RenderInstance");

				Matrix3 elemToWorld = elemToObj;// * objToWorld; 

				INode *meshNode = GetParticleMeshNode(i, inode);
				Material *mtls = meshNode->Mtls();
				int numMtls = meshNode->NumMtls();
				if(!mtls){
					mtls = &nullMaterial;
					numMtls = 1;
				}

				Mtl* pMtl = inode->GetMtl();//apply the particle system material first
				//if(!pMtl){
				//	pMtl = meshNode->GetMtl();//apply the instance material otherwise
				//}

				if(numMtls > 0){
					if(pMtl){
						mtls[0].Kd = pMtl->GetDiffuse();
						mtls[0].Ks = pMtl->GetSpecular();
						mtls[0].Ka = pMtl->GetAmbient();
					}
					else{//if there is no material, set diffuse equal to the particle color
						mtls[0].Kd = m_VCArray[i];
					}
				}



				if (numMtls > 0){
					gw->setMaterial(mtls[0], 0);
				}
				gw->setTransform( elemToWorld );

				{
				   ESS_PROFILE_SCOPE("Display::mesh->render");
				  mesh->render(gw, mtls, (flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, COMP_ALL, numMtls);
				}

                if(deleteMesh){
                    ESS_LOG_WARNING("deleting mesh");
                    delete mesh;
                }
			}
			else
			{
			   ESS_PROFILE_SCOPE("Display::RenderParticle");
				gw->setColor(FILL_COLOR, m_VCArray[i].x, m_VCArray[i].y, m_VCArray[i].z);
				gw->setTransform(Matrix3(1));
				gw->marker(&parts.points[i], POINT_MRKR);  
			}
	   }
	}

   //clearViewportMeshCache();
   
   return 0;
}

int AlembicParticles::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
   ESS_PROFILE_FUNC();
   BOOL doupdate = ((t!=tvalid)||!valid);
   if (doupdate)
   {
       Update(t,inode);
   }

   //createViewportMeshCache(t, inode);

   DWORD savedLimits;
   Matrix3 gwTM;
   int res = 0;
   HitRegion hr;

   GraphicsWindow* gw = vpt->getGW();
   savedLimits = gw->getRndLimits();
   gwTM.IdentityMatrix();
   gw->setTransform(gwTM);
   MakeHitRegion(hr, type, crossing, 4, p);
   gw->setHitRegion(&hr);
   gw->clearHitCode();

   // Hit test against the emitter mesh
   if (EmitterVisible()) 
   {
       gw->setRndLimits((savedLimits|GW_PICK|GW_WIREFRAME) 
           & ~(GW_ILLUM|GW_BACKCULL|GW_FLAT|GW_SPECULAR));
       gw->setTransform(inode->GetObjTMBeforeWSM(t));
       res = mesh.select(gw, &particleMtl, &hr, flags & HIT_ABORTONHIT);
   }

   if (res)
   {
       gw->setRndLimits(savedLimits);
       return res;
   }

  
   Matrix3 objToWorld = inode->GetObjTMAfterWSM(t);
 
   // Hit test against the particles
   ExoNullView nullView;
   nullView.worldToView = objToWorld;
   gw->setRndLimits((savedLimits|GW_PICK) & ~ GW_ILLUM);

	const float fFraction = m_fViewportPercent / 100.0f;
	int nNumParticlesToRender = (int)ceil((float)NumberOfRenderMeshes() * fFraction);
	const float fStep = 1 / fFraction;

   for (int ii=0; ; ii++)
   {
		int i = (int)floor(ii * fStep);
		if( i >= NumberOfRenderMeshes() ){
			break;
		}

		Matrix3 elemToObj;
		elemToObj.IdentityMatrix();

		Interval meshTMValid = FOREVER;

		BOOL deleteMesh = FALSE;

		GetMultipleRenderMeshTM_Internal(t, inode, nullView, i, elemToObj, meshTMValid, true);
		Mesh* mesh = GetMultipleRenderMesh_Internal(t, inode, nullView, deleteMesh, i);
		if(!mesh){
			continue;
		}

		Matrix3 elemToWorld = elemToObj * objToWorld;

		INode *meshNode = GetParticleMeshNode(i, inode);
		Material *mtls = 0;
		int numMtls = 0;

		if (meshNode && meshNode != inode)
		{
		   mtls = meshNode->Mtls();
		   numMtls = meshNode->NumMtls();
		}
		else
		{
			mtls = &particleMtl;
			numMtls = 1;
		}

		gw->setTransform( elemToWorld );
		mesh->select(gw, mtls, &hr, TRUE, numMtls);

		if (gw->checkHitCode()) 
		{
			res = TRUE;
			gw->clearHitCode();
			break;
		}

        if(deleteMesh){
            //ESS_LOG_WARNING("deleting mesh.");
            delete mesh;
        }
   }

   gw->setRndLimits(savedLimits);

   //clearViewportMeshCache();

   return res;
}

Mesh *AlembicParticles::BuildPointMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
   ESS_PROFILE_FUNC();
    //Mesh *pMesh = NULL;
    //needDelete = FALSE;
    //return pMesh;
	return BuildSphereMesh(meshNumber, t, node, view, needDelete);
}

Mesh *AlembicParticles::BuildBoxMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
   ESS_PROFILE_FUNC();
    Mesh *pMesh = NULL;

    if (!m_pBoxMaker)
    {
        m_pBoxMaker = static_cast<GenBoxObject*>
            (GET_MAX_INTERFACE()->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(BOXOBJ_CLASS_ID, 0)));
        float size = 2;
        m_pBoxMaker->SetParams(size, size, size);
        m_pBoxMaker->BuildMesh(0);
        m_pBoxMaker->UpdateValidity(TOPO_CHAN_NUM, FOREVER);
        m_pBoxMaker->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
        m_pBoxMaker->UpdateValidity(TEXMAP_CHAN_NUM, FOREVER);
    }

    pMesh = m_pBoxMaker->GetRenderMesh(t, node, view, needDelete);
	//pMesh->buildNormals();
    return pMesh;
}

Mesh *AlembicParticles::BuildSphereMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
   ESS_PROFILE_FUNC();
    Mesh *pMesh = NULL;
    needDelete = FALSE;

    if (!m_pSphereMaker)
    {
        m_pSphereMaker = static_cast<GenSphere*>
            (GET_MAX_INTERFACE()->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(SPHERE_CLASS_ID, 0)));

        float size = 1;
		int segments = 12;
        m_pSphereMaker->SetParams(size, segments);
        m_pSphereMaker->BuildMesh(0);
        m_pSphereMaker->UpdateValidity(TOPO_CHAN_NUM, FOREVER);
        m_pSphereMaker->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
        m_pSphereMaker->UpdateValidity(TEXMAP_CHAN_NUM, FOREVER);
    }

    pMesh = m_pSphereMaker->GetRenderMesh(t, node, view, needDelete);
	//pMesh->buildNormals();
    return pMesh;
}

Mesh *AlembicParticles::BuildCylinderMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
   ESS_PROFILE_FUNC();
    Mesh *pMesh = NULL;
    needDelete = FALSE;

    if (!m_pCylinderMaker)
    {
        m_pCylinderMaker = static_cast<GenCylinder*>
            (GET_MAX_INTERFACE()->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(CYLINDER_CLASS_ID, 0)));

        float radius = 1;
		float height = 2;
		int numSegments = 1;
		int numSides = 32;
        m_pCylinderMaker->SetParams( radius, height, numSegments, numSides );
        m_pCylinderMaker->BuildMesh(0);
        m_pCylinderMaker->UpdateValidity(TOPO_CHAN_NUM, FOREVER);
        m_pCylinderMaker->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
        m_pCylinderMaker->UpdateValidity(TEXMAP_CHAN_NUM, FOREVER);
   }

   pMesh = m_pCylinderMaker->GetRenderMesh(t, node, view, needDelete);
   //pMesh->buildNormals();
   return pMesh;
}


Mesh *AlembicParticles::BuildConeMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
   ESS_PROFILE_FUNC();
	//3DS Max doesn't seem to have a cone builder, so just return a cylinder fornow.
	return BuildCylinderMesh(meshNumber, t, node, view, needDelete);
}

Mesh *AlembicParticles::BuildDiscMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
   ESS_PROFILE_FUNC();
    Mesh *pMesh = NULL;
    needDelete = FALSE;
    
	if (!m_pDiskMaker)
    {
        m_pDiskMaker = static_cast<GenCylinder*>
            (GET_MAX_INTERFACE()->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(CYLINDER_CLASS_ID, 0)));

        float radius = 1;
		float height = 0;
		int numSegments = 1;
		int numSides = 32;
        m_pDiskMaker->SetParams( radius, height, numSegments, numSides );
        m_pDiskMaker->BuildMesh(0);
        m_pDiskMaker->UpdateValidity(TOPO_CHAN_NUM, FOREVER);
        m_pDiskMaker->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
        m_pDiskMaker->UpdateValidity(TEXMAP_CHAN_NUM, FOREVER);
   }

   pMesh = m_pDiskMaker->GetRenderMesh(t, node, view, needDelete);
   //pMesh->buildNormals();
   return pMesh;
}
 
Mesh *AlembicParticles::BuildRectangleMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
   ESS_PROFILE_FUNC();
    Mesh *pMesh = NULL;
    needDelete = FALSE;

	if (!m_pRectangleMaker)
    {
        m_pRectangleMaker = static_cast<GenBoxObject*>
            (GET_MAX_INTERFACE()->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(BOXOBJ_CLASS_ID, 0)));
        float size = 2;
        m_pRectangleMaker->SetParams(size, 0, size);
        m_pRectangleMaker->BuildMesh(0);
        m_pRectangleMaker->UpdateValidity(TOPO_CHAN_NUM, FOREVER);
        m_pRectangleMaker->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
        m_pRectangleMaker->UpdateValidity(TEXMAP_CHAN_NUM, FOREVER);
   }

   pMesh = m_pRectangleMaker->GetRenderMesh(t, node, view, needDelete);
   //pMesh->buildNormals();
   return pMesh;
}



//void AlembicParticles::ClearMeshCache()
//{
//   ESS_PROFILE_FUNC();
//	for(nodeAndTimeToMeshMap::iterator it=meshCacheMap.begin(); it != meshCacheMap.end(); it++){
//		meshInfo& mi = it->second;
//		if(mi.bMeshNeedDelete){
//			delete mi.pMesh;
//		}
//	}
//	meshCacheMap.clear();
//}

Mesh* GetMeshFromNode(INode *iNode, const TimeValue t, BOOL& bNeedDelete)
{
   ESS_PROFILE_FUNC();
	bNeedDelete = FALSE;
	if (!iNode){
		ESS_LOG_INFO("GetMeshFromNode: iNode is null.");
		return NULL;
	}
	Object *obj = iNode->EvalWorldState(t).obj;
	if(!obj){
		ESS_LOG_INFO("GetMeshFromNode: obj is null.");
		return NULL;
	}

    if (obj->SuperClassID()==GEOMOBJECT_CLASS_ID) {
		ExoNullView nullView;
		GeomObject* geomObject = (GeomObject*)obj;
		Mesh* pMesh = geomObject->GetRenderMesh(t, iNode, nullView, bNeedDelete);
        //ESS_LOG_WARNING("Allocating mesh.");
		return pMesh;
	}
	else{
		return NULL;
	}
}

Mesh *AlembicParticles::BuildInstanceMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete, bool bUseCache)
{
   ESS_PROFILE_FUNC()
	needDelete = FALSE;

	if (meshNumber >= m_InstanceShapeIds.size()){
		return NULL;
	}

	Abc::uint16_t shapeid = m_InstanceShapeIds[meshNumber];

	if (shapeid >= m_InstanceShapeINodes.size()){
	   return NULL;
	}

	INode *pNode = m_InstanceShapeINodes[shapeid];
	TimeValue shapet = m_InstanceShapeTimes[meshNumber];

    InstanceMeshCache::iterator it = m_InstanceMeshCache.find(EC_MCHAR_to_UTF8(pNode->GetName()));

    if( it != m_InstanceMeshCache.end() ){
       return it->second.mesh;
    }
    else{
       //ESS_LOG_WARNING("NODE: "<<pNode->GetName());
       InstanceMesh iMesh;
       iMesh.mesh = GetMeshFromNode(pNode, shapet, iMesh.needDelete);
       m_InstanceMeshCache[EC_MCHAR_to_UTF8(pNode->GetName())] = iMesh;
       return iMesh.mesh;
    }


    //return GetMeshFromNode(pNode, shapet, needDelete);


	//nodeAndTimeToMeshMap::iterator it = meshCacheMap.find(nodeTimePair(pNode, shapet));
	//if( it != meshCacheMap.end() ){
	//	meshInfo& mi = it->second;
	//	return mi.pMesh;
	//}
	//else{
	//	meshInfo& mi = meshCacheMap[nodeTimePair(pNode, shapet)];
	//	mi.pMesh = GetMeshFromNode(pNode, shapet, mi.bMeshNeedDelete);
	//	return mi.pMesh;
	//}

 //  bool deleteTriObj = false;
 //  TriObject *triObj = GetTriObjectFromNode(pNode, shapet, deleteTriObj);

 //  if (!triObj)
 //      return NULL;

//   Mesh *pMesh = triObj->GetRenderMesh(t, node, view, needDelete);

 //  Mesh *pMesh = triObj->GetRenderMesh(shapet, node, view, needDelete);

 //  if (deleteTriObj && !needDelete)
 //  {
 //      Mesh *pTempMesh = new Mesh;
 //      *pTempMesh = *pMesh;
 //      pMesh = pTempMesh;
 //      pMesh->InvalidateGeomCache();
 //      pMesh->InvalidateTopologyCache();
 //      needDelete = TRUE;
 //  }

 //  if (deleteTriObj)
 //      delete triObj;

	//return pMesh;
}

Mesh *AlembicParticles::BuildNbElementsMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
    Mesh *pMesh = NULL;
    needDelete = FALSE;
    return pMesh;
}

RefResult AlembicParticles::NotifyRefChanged(
    Interval iv, 
    RefTargetHandle hTarg, 
    PartID& partID, 
    RefMessage msg) 
{
   ESS_PROFILE_FUNC();
    switch (msg) 
    {
        case REFMSG_CHANGE:
            if (hTarg == pblock2) 
            {
                ParamID changing_param = pblock2->LastNotifyParamID();
                switch(changing_param)
                {
                case ID_PATH:
                    {
                        delRefArchive(m_CachedAbcFile);
                        MCHAR const* strPath = NULL;
                        TimeValue t = GetCOREInterface()->GetTime();
                        pblock2->GetValue( AlembicParticles::ID_PATH, t, strPath, iv);
                        m_CachedAbcFile = EC_MCHAR_to_UTF8( strPath );
                        addRefArchive(m_CachedAbcFile);
                    }
                    break;
                default:
                    break;
                }

                AlembicParticlesParams.InvalidateUI(changing_param);
            }
            break;

        case REFMSG_OBJECT_CACHE_DUMPED:
            return REF_STOP;
            break;
    }

    return REF_SUCCEED;
}

void AlembicParticles::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    s_EditObject  = this;

    SimpleParticle::BeginEditParams(ip, flags, prev);
	s_AlembicParticlesClassDesc.BeginEditParams(ip, this, flags, prev);
}

void AlembicParticles::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
    SimpleParticle::EndEditParams(ip, flags, next);
	s_AlembicParticlesClassDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    s_EditObject  = NULL;
}

void AlembicParticles::SetReference(int i, ReferenceTarget* pTarget)
{ 
    switch(i) 
    { 
    case ALEMBIC_SIMPLE_PARTICLE_REF_PBLOCK2:
        pblock2 = static_cast<IParamBlock2*>(pTarget);
    default:
        break;
    }
}

RefTargetHandle AlembicParticles::GetReference(int i)
{ 
    switch(i)
    {
    case ALEMBIC_SIMPLE_PARTICLE_REF_PBLOCK2:
        return pblock2;
    default:
        return NULL;
    }
}

RefTargetHandle AlembicParticles::Clone(RemapDir& remap) 
{
	AlembicParticles *particle = new AlembicParticles();
    particle->ReplaceReference (ALEMBIC_SIMPLE_PARTICLE_REF_PBLOCK2, remap.CloneRef(pblock2));
   	
    BaseClone(this, particle, remap);
	return particle;
}


BOOL AlembicParticles::OKtoDisplay( TimeValue t)
{
    if (parts.Count() == 0)
        return FALSE;

    if (!valid)
        return FALSE;

    Interval interval = FOREVER;

	BOOL bMuted;
	this->pblock2->GetValue( AlembicParticles::ID_MUTED, t, bMuted, interval);

    return !bMuted;
}

bool isAlembicPoints( AbcG::IObject *pIObj, bool& isConstant ) 
{
   ESS_PROFILE_FUNC();
	AbcG::IPoints objPoints;

	isConstant = true; 

	if(AbcG::IPoints::matches((*pIObj).getMetaData())) {
		objPoints = AbcG::IPoints(*pIObj,Abc::kWrapExisting);
		if( objPoints.valid() ) {
			isConstant = objPoints.getSchema().isConstant();
		}
	}

	return objPoints.valid();
}

int AlembicImport_Points(const std::string &file, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode)
{
   ESS_PROFILE_FUNC();
	const std::string &identifier = iObj.getFullName();

    bool isConstant = false;
	if( !isAlembicPoints( &iObj, isConstant ) ) 
    {
		return alembic_failure;
	}

	// Create the particle emitter object and place it in the scene
    AlembicParticles *pParticleObj = static_cast<AlembicParticles*>
		(GetCOREInterface()->CreateInstance(GEOMOBJECT_CLASS_ID, ALEMBIC_SIMPLE_PARTICLE_CLASSID));
   
    if (pParticleObj == NULL)
    {
        return alembic_failure;
    }

    // Set the alembic information
    TimeValue zero( 0 );
    pParticleObj->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pParticleObj, 0, "path" ), zero, EC_UTF8_to_TCHAR( file.c_str() ) );
	pParticleObj->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pParticleObj, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifier.c_str() ) );
	pParticleObj->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pParticleObj, 0, "time" ), zero, 0.0f );
	pParticleObj->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pParticleObj, 0, "muted" ), zero, FALSE );

    // Create the object node
   Abc::IObject parent = iObj.getParent();
   std::string name = removeXfoSuffix(parent.getName().c_str());
	INode *pNode = GET_MAX_INTERFACE()->CreateObjectNode(pParticleObj, EC_UTF8_to_TCHAR( name.c_str() ));
	if (pNode == NULL)
    {
		return alembic_failure;
    }
	*pMaxNode = pNode;

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, pParticleObj, OBTYPE_POINTS, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl( file.c_str(), identifier.c_str(), iObj, pNode, options);

    //if( !isConstant )
	//our isConstant only takes into account point size, but other things such as the shape mesh can change
    {
        GET_MAX_INTERFACE()->SelectNode( pNode );
        char szControllerName[10000];	
        sprintf_s( szControllerName, 10000, "$.time" );
        AlembicImport_ConnectTimeControl( szControllerName, options );
    }

	GET_MAX_INTERFACE()->SelectNode( pNode );
	importMetadata(iObj);

    return 0;
}



BaseInterface* AlembicParticles::GetInterface(Interface_ID id)
{ 
	if(id == PARTICLEOBJECTEXT_INTERFACE){
		//ESS_LOG_WARNING("ParticleObject Extended interface being retrieved.");
		return m_pAlembicParticlesExt;
		//return NULL;
	}
	else return SimpleParticle::GetInterface(id);
}
