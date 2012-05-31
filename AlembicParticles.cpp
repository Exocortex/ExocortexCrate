#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicDefinitions.h"
#include "AlembicParticles.h"
#include "AlembicArchiveStorage.h"
#include "AlembicVisibilityController.h"
#include "utility.h"
#include "AlembicMAXScript.h"
#include "AlembicMetadataUtils.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

static AlembicParticlesClassDesc s_AlembicParticlesClassDesc;
ClassDesc2 *GetAlembicParticlesClassDesc() { return &s_AlembicParticlesClassDesc; }


//////////////////////////////////////////////////////////////////////////////////////////////////
// Alembic_XForm_Ctrl_Param_Blk
///////////////////////////////////////////////////////////////////////////////////////////////////

static ParamBlockDesc2 AlembicParticlesParams(
	0,
	_T(ALEMBIC_SIMPLE_PARTICLE_SCRIPTNAME),
	0,
	GetAlembicParticlesClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	0,

	// rollout description 
	IDD_ALEMBIC_PARTICLE_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
	AlembicParticles::ID_PATH, _T("path"), TYPE_FILENAME, P_RESET_DEFAULT, IDS_PATH,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PATH_EDIT,
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

	p_end
);

///////////////////////////////////////////////////////////////////////////////////////////////////

// static member variables
AlembicParticles *AlembicParticles::s_EditObject = NULL;
IObjParam *AlembicParticles::s_ObjParam = NULL;

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
    m_pConeMaker = NULL;
	m_pDiskMaker = NULL;
	m_pRectangleMaker = NULL;

    s_AlembicParticlesClassDesc.MakeAutoParamBlocks(this);
}

// virtual
AlembicParticles::~AlembicParticles()
{
    ALEMBIC_SAFE_DELETE(m_pBoxMaker);
    ALEMBIC_SAFE_DELETE(m_pSphereMaker);
    ALEMBIC_SAFE_DELETE(m_pCylinderMaker);
    ALEMBIC_SAFE_DELETE(m_pConeMaker);
    ALEMBIC_SAFE_DELETE(m_pDiskMaker);
    ALEMBIC_SAFE_DELETE(m_pRectangleMaker);
}

void AlembicParticles::UpdateParticles(TimeValue t, INode *node)
{
	ESS_CPP_EXCEPTION_REPORTING_START
   
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

    if( strlen( strPath ) == 0 ) 
    {
       valid = FALSE;
	   ESS_LOG_ERROR( "No filename specified." );
	   return; 
	}
	if( strlen( strIdentifier ) == 0 ) 
    {
       valid = FALSE;
	   ESS_LOG_ERROR( "No path specified." );
	   return;
	}

	if( !fs::exists( strPath ) ) 
    {
        valid = FALSE;
		ESS_LOG_ERROR( "Can't file Alembic file.  Path: " << strPath );
		return;
	}

	Alembic::AbcGeom::IPoints iPoints;
    if (!GetAlembicIPoints(iPoints, strPath, strIdentifier))
    {
        valid = FALSE;
        return;
    }

    if(tvalid == t && valid == TRUE)
    {
        return;
    }

    TimeValue ticks = GetTimeValueFromSeconds( fTime );

    Alembic::AbcGeom::IPointsSchema::Sample floorSample;
    Alembic::AbcGeom::IPointsSchema::Sample ceilSample;
    SampleInfo sampleInfo = GetSampleAtTime(iPoints, ticks, floorSample, ceilSample);

    int numParticles = GetNumParticles(floorSample);
    parts.SetCount(numParticles, PARTICLE_VELS | PARTICLE_AGES | PARTICLE_RADIUS);
    parts.SetCustomDraw(NULL);

    m_InstanceShapeType.resize(numParticles);
    m_InstanceShapeTimes.resize(numParticles);
    m_InstanceShapeIds.resize(numParticles);
    m_ParticleOrientations.resize(numParticles);
    m_ParticleScales.resize(numParticles);
    
  


    m_objToWorld = node->GetObjTMAfterWSM(t);
 
    for (int i = 0; i < numParticles; ++i)
    {
        parts.points[i] = GetParticlePosition(floorSample, ceilSample, sampleInfo, i) * m_objToWorld;
        parts.vels[i] = GetParticleVelocity(floorSample, ceilSample, sampleInfo, i);
        parts.ages[i] = GetParticleAge(iPoints, sampleInfo, i);
		parts.radius[i] = GetParticleRadius(iPoints, sampleInfo, i);
        m_InstanceShapeType[i] = GetParticleShapeType(iPoints, sampleInfo, i);
        m_InstanceShapeIds[i] = GetParticleShapeInstanceId(iPoints, sampleInfo, i);
        m_InstanceShapeTimes[i] = GetParticleShapeInstanceTime(iPoints, sampleInfo, i);
        m_ParticleOrientations[i] = GetParticleOrientation(iPoints, sampleInfo, i);
        m_ParticleScales[i] = GetParticleScale(iPoints, sampleInfo, i);
    }

    // Find the scene nodes for all our instances
    FillParticleShapeNodes(iPoints, sampleInfo);

  /*  // Rebuild the viewport meshes
    NullView nullView;
    for (int i = 0; i < m_ParticleViewportMeshes.size(); i += 1)
    {
        SetAFlag(A_NOTREND);
        m_ParticleViewportMeshes[i].mesh = GetMultipleRenderMesh(t, node, nullView, m_ParticleViewportMeshes[i].needDelete, i);   
       
        ClearAFlag(A_NOTREND);
    }*/
    
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

    Alembic::AbcGeom::IPoints iPoints;
    if (!GetAlembicIPoints(iPoints, strPath, strIdentifier))
    {
        return;
    }

    Alembic::AbcGeom::IPointsSchema::Sample floorSample;
    Alembic::AbcGeom::IPointsSchema::Sample ceilSample;
    GetSampleAtTime(iPoints, t, floorSample, ceilSample);

    // Create a box the size of the bounding box
    Alembic::Abc::Box3d bbox = floorSample.getSelfBounds();
    Imath::V3f alembicMinPt(float(bbox.min.x), float(bbox.min.y), float(bbox.min.z));
    Imath::V3f alembicMaxPt(float(bbox.max.x), float(bbox.max.y), float(bbox.max.z));
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
	//ESS_LOG_INFO( "AlembicParticles::ParticlePosition( i: " << i << " )" );
	return parts.points[i];
}
/**		Returns the velocity of the specified particle at the time passed (in 3ds
Max units per tick). This is specified as a vector. The Particle Age
texture map and the Particle Motion Blur texture map use this method.
\param t The time to return the particle velocity.
\param i The index of the particle. */
Point3 AlembicParticles::ParticleVelocity(TimeValue t,int i) {
	//ESS_LOG_INFO( "AlembicParticles::ParticleVelocity( i: " << i << " )" );
	return parts.vels[i];
}

/**		Returns the world space size of the specified particle in at the time
passed. 
The Particle Age texture map and the Particle Motion Blur texture map use
this method.
\param t The time to return the particle size.
\param i The index of the particle. */
float AlembicParticles::ParticleSize(TimeValue t,int i) {
 	return parts.radius[i];
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
	return parts.ages[i];
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

bool AlembicParticles::GetAlembicIPoints(Alembic::AbcGeom::IPoints &iPoints, const char *strFile, const char *strIdentifier)
{
    // Find the object in the archive
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(strFile, strIdentifier);
	if (!iObj.valid())
    {
		return false;
    }

    iPoints = Alembic::AbcGeom::IPoints(iObj, Alembic::Abc::kWrapExisting);
    if (!iPoints.valid())
    {
        return false;
    }

    return true;
}

SampleInfo AlembicParticles::GetSampleAtTime(Alembic::AbcGeom::IPoints &iPoints, TimeValue t, Alembic::AbcGeom::IPointsSchema::Sample &floorSample, Alembic::AbcGeom::IPointsSchema::Sample &ceilSample) const
{
    double sampleTime = GetSecondsFromTimeValue(t);
    SampleInfo sampleInfo = getSampleInfo(sampleTime,
                                          iPoints.getSchema().getTimeSampling(),
                                          iPoints.getSchema().getNumSamples());

    iPoints.getSchema().get(floorSample, sampleInfo.floorIndex);
    iPoints.getSchema().get(ceilSample, sampleInfo.ceilIndex);

    return sampleInfo;
}

int AlembicParticles::GetNumParticles(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample) const
{
    // We assume that the number of position items is the number of particles
    return static_cast<int>(floorSample.getPositions()->size());
}

Point3 AlembicParticles::GetParticlePosition(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const
{
	
	Imath::V3f alembicP3f = floorSample.getPositions()->get()[index];

    // Get the velocity if there is an alpha
    if (sampleInfo.alpha != 0.0f)
    {
        Imath::V3f alembicVel3f;
        Alembic::Abc::V3fArraySamplePtr velocitiesArray = floorSample.getVelocities();
        if (velocitiesArray == NULL || velocitiesArray->size() == 0)
        {
            alembicVel3f = Imath::V3f(0.0f, 0.0f, 0.0f);
        }
        else
        {
            alembicVel3f = velocitiesArray->get()[(velocitiesArray->size() > index) ? index : 0];
        }

        float alpha = static_cast<float>(sampleInfo.alpha);
        alembicP3f.x = alembicP3f.x + alpha * alembicVel3f.x;
        alembicP3f.y = alembicP3f.y + alpha * alembicVel3f.y;
        alembicP3f.z = alembicP3f.z + alpha * alembicVel3f.z;
    }

    return ConvertAlembicPointToMaxPoint(alembicP3f );
}

Point3 AlembicParticles::GetParticleVelocity(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const
{
    Alembic::Abc::V3fArraySamplePtr velocitiesArray = floorSample.getVelocities();
    if (velocitiesArray == NULL || velocitiesArray->size() == 0)
    {
        return Point3(0.0f, 0.0f, 0.0f);
    }

	Imath::V3f alembicP3f = velocitiesArray->get()[(velocitiesArray->size() > index) ? index : 0];

    if (sampleInfo.alpha != 0.0)
    {
        velocitiesArray = ceilSample.getVelocities();
        if (velocitiesArray != NULL && velocitiesArray->size() > 0)
        {
            const Imath::V3f &alembicCeilP3f = velocitiesArray->get()[(velocitiesArray->size() > index) ? index : 0];
            alembicP3f = static_cast<float>(1.0 - sampleInfo.alpha) * alembicP3f + static_cast<float>(sampleInfo.alpha) * alembicCeilP3f;
        }
    }

    return ConvertAlembicPointToMaxPoint(alembicP3f );
}

float AlembicParticles::GetParticleRadius(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
{
    Alembic::AbcGeom::IFloatGeomParam widthsParam = iPoints.getSchema().getWidthsParam();
    if (widthsParam == NULL || widthsParam.getNumSamples() == 0)
    {
        return 0.0f;
    }

    Alembic::Abc::FloatArraySamplePtr floorSamples = widthsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
    if (floorSamples == NULL || floorSamples->size() == 0)
    {
        return 0.0f;
    }

    float size = floorSamples->get()[(floorSamples->size() > index) ? index : 0];

    if (sampleInfo.alpha != 0.0)
    {
        Alembic::Abc::FloatArraySamplePtr ceilSamples = widthsParam.getExpandedValue(sampleInfo.ceilIndex).getVals();
        if (ceilSamples != NULL && ceilSamples->size() > 0)
        {
            float ceilSize = ceilSamples->get()[(ceilSamples->size() > index) ? index : 0];
            size = static_cast<float>((1.0 - sampleInfo.alpha) * size + sampleInfo.alpha * ceilSize);
        }
    }

    return size;
}

TimeValue AlembicParticles::GetParticleAge(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
{
    if (iPoints.getSchema().getPropertyHeader(".age") == NULL)
    {
        return 0;
    }

    IFloatArrayProperty ageProperty = Alembic::Abc::IFloatArrayProperty(iPoints.getSchema(), ".age");
    if (!ageProperty.valid() || ageProperty.getNumSamples() == 0)
    {
        return 0;
    }

    Alembic::Abc::FloatArraySamplePtr floorSamples = ageProperty.getValue(sampleInfo.floorIndex);
    if (floorSamples == NULL || floorSamples->size() == 0)
    {
        return 0;
    }

    float age = floorSamples->get()[(floorSamples->size() > index) ? index : 0];

    if (sampleInfo.alpha != 0.0)
    {
        Alembic::Abc::FloatArraySamplePtr ceilSamples = ageProperty.getValue(sampleInfo.ceilIndex);
        if (ceilSamples != NULL && ceilSamples->size() > 0)
        {
            float ceilAge = ceilSamples->get()[(ceilSamples->size() > index) ? index : 0];
            age = static_cast<float>((1.0 - sampleInfo.alpha) * age + sampleInfo.alpha * ceilAge);
        }
    }

    return GetTimeValueFromSeconds(age);
}

Quat AlembicParticles::GetParticleOrientation(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
{
    Quat identity;
    identity.Identity();

    if (iPoints.getSchema().getPropertyHeader(".orientation") == NULL)
    {
        return identity;
    }

    IQuatfArrayProperty orientProperty = Alembic::Abc::IQuatfArrayProperty(iPoints.getSchema(), ".orientation");
    if (!orientProperty.valid() || orientProperty.getNumSamples() == 0)
    {
        return identity;
    }

    Alembic::Abc::QuatfArraySamplePtr floorSamples = orientProperty.getValue(sampleInfo.floorIndex);
    if (floorSamples == NULL || floorSamples->size() == 0)
    {
        return identity;
    }

    Alembic::Abc::Quatf quat = floorSamples->get()[(floorSamples->size() > index) ? index : 0];
    Quat q = ConvertAlembicQuatToMaxQuat(quat, true);
    q.Normalize();

    if (iPoints.getSchema().getPropertyHeader(".angularvelocity") == NULL)
    {
        return q;
    }

    IQuatfArrayProperty angVelProperty = Alembic::Abc::IQuatfArrayProperty(iPoints.getSchema(), ".angularvelocity");
    if (!angVelProperty.valid() || angVelProperty.getNumSamples() == 0 )
    {
        return q;
    }

    if (sampleInfo.alpha != 0.0)
    {
        Alembic::Abc::QuatfArraySamplePtr floorAngVelSamples = angVelProperty.getValue(sampleInfo.floorIndex);
        if (floorAngVelSamples != NULL && floorAngVelSamples->size() > 0 && floorAngVelSamples->size() > index) 
        {
            Alembic::Abc::Quatf velquat = floorAngVelSamples->get()[index];
            Quat v = ConvertAlembicQuatToMaxQuat(velquat, false);
            float alpha = static_cast<float>(sampleInfo.alpha);
            v = v * alpha;
            if (v.w != 0.0f)
                q = v * q;
            q.Normalize();
        }
    }

    return q;
}

Point3 AlembicParticles::GetParticleScale(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
{
    if (iPoints.getSchema().getPropertyHeader(".scale") == NULL)
    {
        return Point3(1,1,1);
    }

    IV3fArrayProperty scaleProperty = Alembic::Abc::IV3fArrayProperty(iPoints.getSchema(), ".scale");
    if (!scaleProperty.valid() || scaleProperty.getNumSamples() == 0)
    {
        return Point3(1,1,1);
    }

    Alembic::Abc::V3fArraySamplePtr floorSamples = scaleProperty.getValue(sampleInfo.floorIndex);
    if (floorSamples == NULL || floorSamples->size() == 0)
    {
        return Point3(1,1,1);
    }

    Alembic::Abc::V3f floorScale = floorSamples->get()[(floorSamples->size() > index) ? index : 0];

    if (sampleInfo.alpha != 0.0)
    {
        Alembic::Abc::V3fArraySamplePtr ceilSamples = scaleProperty.getValue(sampleInfo.ceilIndex);
        if (ceilSamples != NULL && ceilSamples->size() > 0)
        {
            float invalpha = static_cast<float>(1.0 - sampleInfo.alpha);
            float alpha = static_cast<float>(sampleInfo.alpha);
            Alembic::Abc::V3f ceilScale = ceilSamples->get()[(ceilSamples->size() > index) ? index : 0];
            floorScale.x = invalpha * floorScale.x + alpha * ceilScale.x;
            floorScale.y = invalpha * floorScale.y + alpha * ceilScale.y;
            floorScale.z = invalpha * floorScale.z + alpha * ceilScale.z;
        }
    }

    Point3 maxScale = ConvertAlembicScaleToMaxScale(floorScale);
    return maxScale;
}

AlembicPoints::ShapeType AlembicParticles::GetParticleShapeType(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
{
    if (iPoints.getSchema().getPropertyHeader(".shapetype") == NULL)
    {
        return AlembicPoints::ShapeType_Point;
    }

    IUInt16ArrayProperty shapeTypeProperty = Alembic::Abc::IUInt16ArrayProperty(iPoints.getSchema(), ".shapetype");
    if (!shapeTypeProperty.valid() || shapeTypeProperty.getNumSamples() == 0)
    {
        return AlembicPoints::ShapeType_Point;
    }

    Alembic::Abc::UInt16ArraySamplePtr floorSamples = shapeTypeProperty.getValue(sampleInfo.floorIndex);
    if (floorSamples == NULL || floorSamples->size() == 0)
    {
        return AlembicPoints::ShapeType_Point;
    }

    AlembicPoints::ShapeType shapeType = static_cast<AlembicPoints::ShapeType>(floorSamples->get()[(floorSamples->size() > index) ? index : 0]);

    return shapeType;
}

uint16_t AlembicParticles::GetParticleShapeInstanceId(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
{
    if (iPoints.getSchema().getPropertyHeader(".shapeinstanceid") == NULL || iPoints.getSchema().getPropertyHeader(".instancenames") == NULL)
    {
        return 0;
    }

    IUInt16ArrayProperty shapeIdProperty = Alembic::Abc::IUInt16ArrayProperty(iPoints.getSchema(), ".shapeinstanceid");
    if (!shapeIdProperty.valid() || shapeIdProperty.getNumSamples() == 0)
    {
        return 0;
    }

    Alembic::Abc::UInt16ArraySamplePtr floorSamples = shapeIdProperty.getValue(sampleInfo.floorIndex);
    if (floorSamples == NULL || floorSamples->size() == 0)
    {
        return 0;
    }

    uint16_t instanceId = floorSamples->get()[(floorSamples->size() > index) ? index : 0];

    return instanceId;
}

void AlembicParticles::FillParticleShapeNodes(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo)
{
    //m_TotalShapesToEnumerate = 0;
    m_InstanceShapeINodes.clear();

	if( iPoints.getSchema().getPropertyHeader( ".instancenames" ) == NULL ) {
		return;
	}

    IStringArrayProperty shapeInstanceNameProperty = Alembic::Abc::IStringArrayProperty(iPoints.getSchema(), ".instancenames");
    if (!shapeInstanceNameProperty.valid() || shapeInstanceNameProperty.getNumSamples() == 0)
    {
        return;
    }

    m_InstanceShapeNames = shapeInstanceNameProperty.getValue(sampleInfo.floorIndex);

    if (m_InstanceShapeNames == NULL || m_InstanceShapeNames->size() == 0)
    {
        return;
    }

    //m_TotalShapesToEnumerate = m_InstanceShapeNames->size();
    m_InstanceShapeINodes.resize(m_InstanceShapeNames->size());
    
    for (int i = 0; i < m_InstanceShapeINodes.size(); i += 1)
    {
		const std::string& path = m_InstanceShapeNames->get()[i];
		m_InstanceShapeINodes[i] = GetNodeFromHierarchyPath(path);
    }
}

TimeValue AlembicParticles::GetParticleShapeInstanceTime(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
{
    if (iPoints.getSchema().getPropertyHeader(".shapetime") == NULL)
    {
        return 0;
    }

    IFloatArrayProperty ageProperty = Alembic::Abc::IFloatArrayProperty(iPoints.getSchema(), ".shapetime");
    if (!ageProperty.valid() || ageProperty.getNumSamples() == 0)
    {
        return 0;
    }

    Alembic::Abc::FloatArraySamplePtr floorSamples = ageProperty.getValue(sampleInfo.floorIndex);
    if (floorSamples == NULL || floorSamples->size() == 0)
    {
        return 0;
    }

    float shapeTime = floorSamples->get()[(floorSamples->size() > index) ? index : 0];

    if (sampleInfo.alpha != 0.0)
    {
        Alembic::Abc::FloatArraySamplePtr ceilSamples = ageProperty.getValue(sampleInfo.ceilIndex);
        if (ceilSamples != NULL && ceilSamples->size() > 0)
        {
            float ceilShapeTime = ceilSamples->get()[(ceilSamples->size() > index) ? index : 0];
            shapeTime = static_cast<float>((1.0 - sampleInfo.alpha) * shapeTime + sampleInfo.alpha * ceilShapeTime);
        }
    }

    return GetTimeValueFromSeconds(shapeTime);
}

int AlembicParticles::NumberOfRenderMeshes()
{
	//ESS_LOG_INFO( "AlembicParticles::NumberOfRenderMeshes()" );
     return parts.Count();
}

Mesh* AlembicParticles::GetMultipleRenderMesh(TimeValue  t,  INode *inode,  View &view,  BOOL &needDelete,  int meshNumber)
{
	//ESS_LOG_INFO( "AlembicParticles::GetMultipleRenderMesh( t: " << t << " meshNumber: " << meshNumber << ", t: " << t << " )" );
    if (meshNumber > parts.Count() || !parts.Alive(meshNumber) || view.CheckForRenderAbort())
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
        pMesh = BuildInstanceMesh(meshNumber, t, inode, view, needDelete);
        break;
    case AlembicPoints::ShapeType_NbElements:
        pMesh = BuildNbElementsMesh(meshNumber, t, inode, view, needDelete);
        break;
    default:
	    pMesh = NULL;
        needDelete = FALSE;
	    break;
    }

    return pMesh;
}

void AlembicParticles::GetMultipleRenderMeshTM(TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid)
{
	MSTR rendererName;
	GET_MAX_INTERFACE()->GetCurrentRenderer()->GetClassName( rendererName );
	string renderer( rendererName );

	GetMultipleRenderMeshTM_Internal(t, inode, view, meshNumber, meshTM, meshTMValid);

	

	Matrix3 objectToWorld = inode->GetObjectTM( t );

	if( renderer.find( string( "mental ray Renderer" ) ) !=string::npos ) {
		//ESS_LOG_INFO( "MR renderer mode" );
		
		meshTM = meshTM * Inverse( objectToWorld );
	}
	else if( renderer.find( string( "iray" ) ) !=string::npos ) {
		//ESS_LOG_INFO( "MR renderer mode" );
		meshTM = meshTM * Inverse( objectToWorld );
	}
	else if(( renderer.find( string( "vray" ) ) != string::npos )||
		( renderer.find( string( "Vray" ) ) != string::npos )||
		( renderer.find( string( "VRay" ) ) != string::npos ) ) {
		//ESS_LOG_INFO( "Vray renderer mode" );
		meshTM = meshTM * Inverse( objectToWorld );
	} 
	else {
		//ESS_LOG_INFO( "Default renderer mode" );
		meshTM = Inverse( objectToWorld ) * meshTM;
	}
 
	meshTMValid.SetInstant( t );
}

void AlembicParticles::GetMultipleRenderMeshTM_Internal(TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid)
{
	
    // Calculate the matrix
    Point3 pos = parts.points[meshNumber];
    Quat orient = m_ParticleOrientations[meshNumber];
    Point3 scaleVec = m_ParticleScales[meshNumber];
    scaleVec *= parts.radius[meshNumber];

	//if(m_InstanceShapeType[meshNumber] == AlembicPoints::ShapeType_Box){
	//	pos.z -= 2;
	//}

	meshTM.IdentityMatrix();
	//meshTM.SetRotate(orient);//TODO: the orientation is wrong
	meshTM.PreScale(scaleVec);
	meshTM.SetTrans(pos);
}

INode* AlembicParticles::GetParticleMeshNode(int meshNumber, INode *displayNode)
{
	ESS_LOG_INFO( "AlembicParticles::GetParticleMeshNode( meshNumber: " << meshNumber << " )" );
    if (meshNumber > parts.Count() || !parts.Alive(meshNumber))
    {
        return displayNode;
    }

    if (m_InstanceShapeType[meshNumber] == AlembicPoints::ShapeType_Instance)
    {
        if (meshNumber > m_InstanceShapeIds.size())
            return displayNode;

        uint16_t shapeid = m_InstanceShapeIds[meshNumber];

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


int AlembicParticles::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
   if (!OKtoDisplay(t)) 
   {
       return 0;
   }

   BOOL doupdate = ((t!=tvalid)||!valid);
   if (doupdate)
   {
       Update(t,inode);
   }

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
      
   // Draw the particles
   NullView nullView;
   //nullView.worldToView = objToWorld;
   gw->setRndLimits(rlim);
   for (int i = 0; i < NumberOfRenderMeshes(); i += 1)
   {

		Matrix3 elemToObj;
		elemToObj.IdentityMatrix();

		Interval meshTMValid = FOREVER;

		BOOL deleteMesh = FALSE;

		GetMultipleRenderMeshTM_Internal(t, inode, nullView, i, elemToObj, meshTMValid);
		Mesh *mesh = GetMultipleRenderMesh(t, inode, nullView, deleteMesh, i);

		if(mesh){

			Matrix3 elemToWorld = elemToObj * objToWorld;

			INode *meshNode = GetParticleMeshNode(i, inode);
			Material *mtls = meshNode->Mtls();
			int numMtls = meshNode->NumMtls();

			if (numMtls > 1){
				gw->setMaterial(mtls[0], 0);
			}

			gw->setTransform( elemToWorld );
			mesh->render(gw, mtls, (flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, COMP_ALL, numMtls);
		}
		else
		{
			gw->setTransform(Matrix3(1));
			gw->marker(&parts.points[i], POINT_MRKR);  
		}
   }
   
   return 0;
}

int AlembicParticles::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
   BOOL doupdate = ((t!=tvalid)||!valid);
   if (doupdate)
   {
       Update(t,inode);
   }

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
   NullView nullView;
   nullView.worldToView = objToWorld;
   gw->setRndLimits((savedLimits|GW_PICK) & ~ GW_ILLUM);

   for (int i = 0; i < NumberOfRenderMeshes(); i += 1)
   {
		Matrix3 elemToObj;
		elemToObj.IdentityMatrix();

		Interval meshTMValid = FOREVER;

		BOOL deleteMesh = FALSE;

		GetMultipleRenderMeshTM_Internal(t, inode, nullView, i, elemToObj, meshTMValid);
		Mesh *mesh = GetMultipleRenderMesh(t, inode, nullView, deleteMesh, i);
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
   }

   gw->setRndLimits(savedLimits);
   return res;
}

Mesh *AlembicParticles::BuildPointMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
    //Mesh *pMesh = NULL;
    //needDelete = FALSE;
    //return pMesh;
	return BuildSphereMesh(meshNumber, t, node, view, needDelete);
}

Mesh *AlembicParticles::BuildBoxMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
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
    return pMesh;
}

Mesh *AlembicParticles::BuildSphereMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
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
    return pMesh;
}

Mesh *AlembicParticles::BuildCylinderMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
    Mesh *pMesh = NULL;
    needDelete = FALSE;

    if (!m_pCylinderMaker)
    {
        m_pCylinderMaker = static_cast<GenCylinder*>
            (GET_MAX_INTERFACE()->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(CYLINDER_CLASS_ID, 0)));

        float radius = 1;
		float height = 1;
		int numSegments = 1;
		int numSides = 32;
        m_pCylinderMaker->SetParams( radius, height, numSegments, numSides );
        m_pCylinderMaker->BuildMesh(0);
        m_pCylinderMaker->UpdateValidity(TOPO_CHAN_NUM, FOREVER);
        m_pCylinderMaker->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
        m_pCylinderMaker->UpdateValidity(TEXMAP_CHAN_NUM, FOREVER);
   }

   pMesh = m_pCylinderMaker->GetRenderMesh(t, node, view, needDelete);
   return pMesh;
}


Mesh *AlembicParticles::BuildConeMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
    Mesh *pMesh = NULL;
    needDelete = FALSE;

	if (!m_pConeMaker)
    {
        m_pConeMaker = static_cast<SimpleObject*>
            (GET_MAX_INTERFACE()->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(CONE_CLASS_ID, 0)));

        float radius1 = 1;
        float radius2 = 0;
		float height = 1;
		int numSegments = 1;
		int numCapSegments = 1;
		int numSides = 32;

		TimeValue zero( 0 );

/*#define PB_RADIUS1		0
#define PB_RADIUS2		1
#define PB_HEIGHT		2
#define PB_SEGMENTS		3
#define PB_CAPSEGMENTS	4
#define PB_SIDES		5*/

        m_pConeMaker->GetParamBlockByID( 0 )->SetValue( 0, zero, radius1 );
        m_pConeMaker->GetParamBlockByID( 0 )->SetValue( 1, zero, radius2 );
        m_pConeMaker->GetParamBlockByID( 0 )->SetValue( 2, zero, height );
        m_pConeMaker->GetParamBlockByID( 0 )->SetValue( 3, zero, numSegments );
        m_pConeMaker->GetParamBlockByID( 0 )->SetValue( 4, zero, numCapSegments );
        m_pConeMaker->GetParamBlockByID( 0 )->SetValue( 5, zero, numSides );
        m_pConeMaker->BuildMesh(0);
        m_pConeMaker->UpdateValidity(TOPO_CHAN_NUM, FOREVER);
        m_pConeMaker->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
        m_pConeMaker->UpdateValidity(TEXMAP_CHAN_NUM, FOREVER);
   }

   pMesh = m_pConeMaker->GetRenderMesh(t, node, view, needDelete);
   return pMesh;
}

Mesh *AlembicParticles::BuildDiscMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
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
   return pMesh;
}
 
Mesh *AlembicParticles::BuildRectangleMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
    Mesh *pMesh = NULL;
    needDelete = FALSE;

	if (!m_pRectangleMaker)
    {
        m_pRectangleMaker = static_cast<SimpleObject*>
            (GET_MAX_INTERFACE()->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(CONE_CLASS_ID, 0)));

        float width = 1;
        float length = 1;

		TimeValue zero( 0 );

/*// Parameter map indices
#define PB_LENGTH		0
#define PB_WIDTH		1*/

        m_pRectangleMaker->GetParamBlockByID( 0 )->SetValue( 0, zero, width );
        m_pRectangleMaker->GetParamBlockByID( 0 )->SetValue( 1, zero, length );
        m_pRectangleMaker->BuildMesh(0);
        m_pRectangleMaker->UpdateValidity(TOPO_CHAN_NUM, FOREVER);
        m_pRectangleMaker->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
        m_pRectangleMaker->UpdateValidity(TEXMAP_CHAN_NUM, FOREVER);
   }

   pMesh = m_pRectangleMaker->GetRenderMesh(t, node, view, needDelete);
    return pMesh;
}

Mesh *AlembicParticles::BuildInstanceMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete)
{
   needDelete = FALSE;

   if (meshNumber > m_InstanceShapeIds.size())
       return NULL;

   uint16_t shapeid = m_InstanceShapeIds[meshNumber];

   if (shapeid > m_InstanceShapeINodes.size())
       return NULL;

   INode *pNode = m_InstanceShapeINodes[shapeid];
   TimeValue shapet = m_InstanceShapeTimes[meshNumber];

   bool deleteTriObj = false;
   TriObject *triObj = GetTriObjectFromNode(pNode, shapet, deleteTriObj);

   if (!triObj)
       return NULL;

   if (!deleteTriObj)
   {
       triObj->UpdateValidity(TOPO_CHAN_NUM, Interval(t, t));
       triObj->UpdateValidity(GEOM_CHAN_NUM, Interval(t, t));
       triObj->UpdateValidity(TEXMAP_CHAN_NUM, Interval(t, t));
   }

   Mesh *pMesh = triObj->GetRenderMesh(t, node, view, needDelete);

   if (deleteTriObj && !needDelete)
   {
       Mesh *pTempMesh = new Mesh;
       *pTempMesh = *pMesh;
       pMesh = pTempMesh;
       pMesh->InvalidateGeomCache();
       pMesh->InvalidateTopologyCache();
       needDelete = TRUE;
   }

   if (deleteTriObj)
       delete triObj;

   return pMesh;
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
                        m_CachedAbcFile = strPath;
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

bool isAlembicPoints( Alembic::AbcGeom::IObject *pIObj, bool& isConstant ) 
{
	Alembic::AbcGeom::IPoints objPoints;

	isConstant = true; 

	if(Alembic::AbcGeom::IPoints::matches((*pIObj).getMetaData())) {
		objPoints = Alembic::AbcGeom::IPoints(*pIObj,Alembic::Abc::kWrapExisting);
		if( objPoints.valid() ) {
			isConstant = objPoints.getSchema().isConstant();
		}
	}

	return objPoints.valid();
}

int AlembicImport_Points(const std::string &file, Alembic::AbcGeom::IObject& iObj, alembic_importoptions &options, INode** pMaxNode)
{
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
    pParticleObj->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pParticleObj, 0, "path" ), zero, file.c_str());
	pParticleObj->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pParticleObj, 0, "identifier" ), zero, identifier.c_str() );
	pParticleObj->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pParticleObj, 0, "time" ), zero, 0.0f );
	pParticleObj->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pParticleObj, 0, "muted" ), zero, FALSE );

    // Create the object node
	INode *pNode = GET_MAX_INTERFACE()->CreateObjectNode(pParticleObj, iObj.getName().c_str());
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

    if( !isConstant ) 
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
