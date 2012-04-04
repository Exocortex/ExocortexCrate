#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicDefinitions.h"
#include "AlembicSimpleParticle.h"
#include "AlembicArchiveStorage.h"
#include "AlembicVisibilityController.h"
#include "utility.h"
#include "AlembicMAXScript.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

static AlembicSimpleParticleClassDesc s_AlembicSimpleParticleClassDesc;
ClassDesc2 *GetAlembicSimpleParticleClassDesc() { return &s_AlembicSimpleParticleClassDesc; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Alembic_XForm_Ctrl_Param_Blk
///////////////////////////////////////////////////////////////////////////////////////////////////

static ParamBlockDesc2 AlembicSimpleParticleParams(
	0,
	_T(ALEMBIC_SIMPLE_PARTICLE_SCRIPTNAME),
	0,
	GetAlembicSimpleParticleClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	0,

	// rollout description 
	IDD_ALEMBIC_PARTICLE_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
	AlembicSimpleParticle::ID_PATH, _T("path"), TYPE_FILENAME, P_RESET_DEFAULT, IDS_PATH,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PATH_EDIT,
	 	end,
        
	AlembicSimpleParticle::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, P_RESET_DEFAULT, IDS_IDENTIFIER,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_IDENTIFIER_EDIT,
	 	end,

	AlembicSimpleParticle::ID_TIME, _T("time"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIME,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_TIME_EDIT,    IDC_TIME_SPIN, 0.01f,
		end,
        
    AlembicSimpleParticle::ID_MUTED, _T("muted"), TYPE_BOOL, P_ANIMATABLE, IDS_MUTED,
		p_default,       FALSE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_MUTED_CHECKBOX,
		end,

	end
);

///////////////////////////////////////////////////////////////////////////////////////////////////

// static member variables
AlembicSimpleParticle *AlembicSimpleParticle::s_EditObject = NULL;
IObjParam *AlembicSimpleParticle::s_ObjParam = NULL;

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

AlembicSimpleParticle::AlembicSimpleParticle()
    : SimpleParticle(), m_TotalShapesToEnumerate(0)
{
    pblock = NULL;
    s_AlembicSimpleParticleClassDesc.MakeAutoParamBlocks(this);
}

// virtual
AlembicSimpleParticle::~AlembicSimpleParticle()
{
    ClearCurrentViewportMeshes();
}

void AlembicSimpleParticle::UpdateParticles(TimeValue t, INode *node)
{
	ESS_CPP_EXCEPTION_REPORTING_START
   
    Interval interval = FOREVER;//os->obj->ObjectValidity(t);
	//ESS_LOG_INFO( "Interval Start: " << interval.Start() << " End: " << interval.End() );

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicSimpleParticle::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicSimpleParticle::ID_IDENTIFIER, t, strIdentifier, interval);
 
	float fTime;
	this->pblock->GetValue( AlembicSimpleParticle::ID_TIME, t, fTime, interval);

	BOOL bMuted;
	this->pblock->GetValue( AlembicSimpleParticle::ID_MUTED, t, bMuted, interval);

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

    // Delete the old viewport meshes
    ClearCurrentViewportMeshes();

    m_InstanceShapeType.resize(numParticles);
    m_InstanceShapeTimes.resize(numParticles);
    m_InstanceShapeIds.resize(numParticles);
    m_ParticleOrientations.resize(numParticles);
    m_ParticleScales.resize(numParticles);
    m_ParticleViewportMeshes.resize(numParticles);
    
    m_TotalShapesToEnumerate = 0;

    for (int i = 0; i < numParticles; ++i)
    {
        parts.points[i] = GetParticlePosition(floorSample, ceilSample, sampleInfo, i);
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

    // Rebuild the viewport meshes
    NullView nullView;
    BOOL needdel;
    for (int i = 0; i < m_ParticleViewportMeshes.size(); i += 1)
    {
        SetAFlag(A_NOTREND);
        m_ParticleViewportMeshes[i] = GetMultipleRenderMesh(t, node, nullView, needdel, i);  
        
        if (m_ParticleViewportMeshes[i])
            m_ParticleViewportMeshes[i]->InvalidateStrips();

        ClearAFlag(A_NOTREND);
    }
    
    tvalid = t;
    valid = TRUE;

	ESS_CPP_EXCEPTION_REPORTING_END
}

void AlembicSimpleParticle::BuildEmitter(TimeValue t, Mesh &mesh)
{
    return;
/*
    Alembic::AbcGeom::IPoints iPoints;
    if (!GetAlembicIPoints(iPoints))
    {
        return;
    }

    Alembic::AbcGeom::IPointsSchema::Sample floorSample;
    Alembic::AbcGeom::IPointsSchema::Sample ceilSample;
    GetSampleAtTime(iPoints, t, floorSample, ceilSample);

    // Create a box the size of the bounding box
    Alembic::Abc::Box3d bbox = floorSample.getSelfBounds();
    Point3 alembicMinPt(bbox.min.x, bbox.min.y, bbox.min.z);
    Point3 alembicMaxPt(bbox.max.x, bbox.max.y, bbox.max.z);
    Point3 maxMinPt;
    Point3 maxMaxPt;
    ConvertAlembicPointToMaxPoint(alembicMinPt, maxMinPt);
    ConvertAlembicPointToMaxPoint(alembicMaxPt, maxMaxPt);

    const float EPSILON = 0.0001f;
    Point3 minPt(min(maxMinPt.x, maxMaxPt.x) - EPSILON, min(maxMinPt.y, maxMaxPt.y) - EPSILON, min(maxMinPt.z, maxMaxPt.z) - EPSILON);
    Point3 maxPt(max(maxMinPt.x, maxMaxPt.x) + EPSILON, max(maxMinPt.y, maxMaxPt.y) + EPSILON, max(maxMinPt.z, maxMaxPt.z) + EPSILON);

    mesh.setNumVerts(8);
    mesh.setVert(0, minPt.x, minPt.y, minPt.z);
    mesh.setVert(1, minPt.x, maxPt.y, minPt.z);
    mesh.setVert(2, maxPt.x, maxPt.y, minPt.z);
    mesh.setVert(3, maxPt.x, minPt.y, minPt.z);
    mesh.setVert(4, minPt.x, minPt.y, maxPt.z);
    mesh.setVert(5, minPt.x, maxPt.y, maxPt.z);
    mesh.setVert(6, maxPt.x, maxPt.y, maxPt.z);
    mesh.setVert(7, maxPt.x, minPt.y, maxPt.z);

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

    mvalid = Interval(t ,t);
*/
}

int AlembicSimpleParticle::RenderBegin(TimeValue t, ULONG flags)
{
    SetAFlag(A_RENDER);
    ParticleInvalid();		
    NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
    return 0;
}
    
int AlembicSimpleParticle::RenderEnd(TimeValue t)
{
    ClearAFlag(A_RENDER);
	ParticleInvalid();		
	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	return 0;
}

Interval AlembicSimpleParticle::GetValidity(TimeValue t)
{
    Interval interval(t, t);
    return interval;
}

MarkerType AlembicSimpleParticle::GetMarkerType()
{
    return POINT_MRKR;
}

bool AlembicSimpleParticle::GetAlembicIPoints(Alembic::AbcGeom::IPoints &iPoints, const char *strFile, const char *strIdentifier)
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

SampleInfo AlembicSimpleParticle::GetSampleAtTime(Alembic::AbcGeom::IPoints &iPoints, TimeValue t, Alembic::AbcGeom::IPointsSchema::Sample &floorSample, Alembic::AbcGeom::IPointsSchema::Sample &ceilSample) const
{
    double sampleTime = GetSecondsFromTimeValue(t);
    SampleInfo sampleInfo = getSampleInfo(sampleTime,
                                          iPoints.getSchema().getTimeSampling(),
                                          iPoints.getSchema().getNumSamples());

    iPoints.getSchema().get(floorSample, sampleInfo.floorIndex);
    iPoints.getSchema().get(ceilSample, sampleInfo.ceilIndex);

    return sampleInfo;
}

int AlembicSimpleParticle::GetNumParticles(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample) const
{
    // We assume that the number of position items is the number of particles
    return static_cast<int>(floorSample.getPositions()->size());
}

Point3 AlembicSimpleParticle::GetParticlePosition(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const
{
    float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

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

    return ConvertAlembicPointToMaxPoint(alembicP3f, masterScaleUnitMeters );
}

Point3 AlembicSimpleParticle::GetParticleVelocity(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const
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

    float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);
    return ConvertAlembicPointToMaxPoint(alembicP3f, masterScaleUnitMeters );
}

float AlembicSimpleParticle::GetParticleRadius(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
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

TimeValue AlembicSimpleParticle::GetParticleAge(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
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

Quat AlembicSimpleParticle::GetParticleOrientation(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
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
        if (floorAngVelSamples != NULL && floorAngVelSamples->size() > 0 && floorAngVelSamples->size() < index) 
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

Point3 AlembicSimpleParticle::GetParticleScale(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
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

AlembicPoints::ShapeType AlembicSimpleParticle::GetParticleShapeType(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
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

uint16_t AlembicSimpleParticle::GetParticleShapeInstanceId(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
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

void AlembicSimpleParticle::FillParticleShapeNodes(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo)
{
    m_TotalShapesToEnumerate = 0;
    m_InstanceShapeINodes.clear();

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

    m_TotalShapesToEnumerate = m_InstanceShapeNames->size();
    m_InstanceShapeINodes.resize(m_TotalShapesToEnumerate);
    
    for (int i = 0; i < m_TotalShapesToEnumerate; i += 1)
    {
        m_InstanceShapeINodes[i] = NULL;

        // Take out any blanks from our count
        if (m_InstanceShapeNames->get()[i].empty())
        {
            m_TotalShapesToEnumerate -= 1;
        }
    }

    IScene *pScene = GET_MAX_INTERFACE()->GetScene();
    pScene->EnumTree(this);
}

TimeValue AlembicSimpleParticle::GetParticleShapeInstanceTime(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const
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

int AlembicSimpleParticle::NumberOfRenderMeshes()
{
    return parts.Count();
}

Mesh* AlembicSimpleParticle::GetMultipleRenderMesh(TimeValue  t,  INode *inode,  View &view,  BOOL &needDelete,  int meshNumber)
{
    if (meshNumber > parts.Count() || !parts.Alive(meshNumber) || view.CheckForRenderAbort())
    {
        needDelete = NULL;
        return NULL;
    }

    Mesh *pMesh = NULL;
    switch (m_InstanceShapeType[meshNumber])
    {
    case AlembicPoints::ShapeType_Point:
        pMesh = BuildPointMesh();
        break;
    case AlembicPoints::ShapeType_Box:
        pMesh = BuildBoxMesh();
        break;
    case AlembicPoints::ShapeType_Sphere:
        pMesh = BuildSphereMesh();
        break;
    case AlembicPoints::ShapeType_Cylinder:
        pMesh = BuildCylinderMesh();
        break;
    case AlembicPoints::ShapeType_Cone:
        pMesh = BuildConeMesh();
        break;
    case AlembicPoints::ShapeType_Disc:
        pMesh = BuildDiscMesh();
        break;
    case AlembicPoints::ShapeType_Rectangle:
        pMesh = BuildRectangleMesh();
        break;
    case AlembicPoints::ShapeType_Instance:
        pMesh = BuildInstanceMesh(meshNumber);
        break;
    case AlembicPoints::ShapeType_NbElements:
        pMesh = BuildNbElementsMesh();
        break;
    default:
	    pMesh = BuildPointMesh();
	    break;
    }

    needDelete = pMesh != NULL;
    return pMesh;
}

void AlembicSimpleParticle::GetMultipleRenderMeshTM(TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid)
{
    if (meshNumber > parts.Count() || !parts.Alive(meshNumber) || view.CheckForRenderAbort())
    {
        meshTM.IdentityMatrix();
    }

    // Calculate the matrix
    Point3 pos = parts.points[meshNumber];
    Quat orient = m_ParticleOrientations[meshNumber];
    Point3 scaleVec = m_ParticleScales[meshNumber];
    meshTM.SetRotate(orient);
    meshTM.PreScale(scaleVec);
    meshTM.SetTrans(pos);
 
    meshTMValid = Interval(t, t);
}

INode* AlembicSimpleParticle::GetParticleMeshNode(int meshNumber, INode *displayNode)
{
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

int AlembicSimpleParticle::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
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
   gw->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY|/*GW_BACKCULL|*/ (rlim&GW_Z_BUFFER) );  //removed BC on 4/29/99 DB

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
      
   // Draw the particles
   NullView nullView;
   gw->setRndLimits(rlim);
   for (int i = 0; i < NumberOfRenderMeshes(); i += 1)
   {
       if (m_ParticleViewportMeshes[i])
       {
           Matrix3 meshTM;
           Interval meshTMValid = FOREVER;
           GetMultipleRenderMeshTM(t, inode, nullView, i, meshTM, meshTMValid);
           INode *meshNode = GetParticleMeshNode(i, inode);
           Material *mtls = meshNode->Mtls();
           int numMtls = meshNode->NumMtls();

           if (numMtls > 1)
               gw->setMaterial(mtls[0], 0);

           gw->setTransform(meshTM);
           m_ParticleViewportMeshes[i]->render(gw, mtls, (flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, COMP_ALL, numMtls);
       }
   }
   
   return 0;
}

int AlembicSimpleParticle::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
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

   // Hit test against the particles
   NullView nullView;
   gw->setRndLimits((savedLimits|GW_PICK) & ~ GW_ILLUM);
   for (int i = 0; i < NumberOfRenderMeshes(); i += 1)
   {
       if (m_ParticleViewportMeshes[i])
       {
           Matrix3 meshTM;
           Interval meshTMValid = FOREVER;
           GetMultipleRenderMeshTM(t, inode, nullView, i, meshTM, meshTMValid);
           INode *meshNode = GetParticleMeshNode(i, inode);
           Material *mtls = meshNode->Mtls();
           int numMtls = meshNode->NumMtls();

           gw->setTransform(meshTM);
           m_ParticleViewportMeshes[i]->select(gw, mtls, &hr, TRUE, numMtls);
       }
       else
       {
           Point3 pos = parts.points[i];
           gw->marker(&pos, POINT_MRKR);
       }
       
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

int AlembicSimpleParticle::callback( INode *node )
{
    int enumCode = TREE_CONTINUE;

    for (int i = 0; i < m_InstanceShapeNames->size(); i += 1)
    {
        const char *shapename = m_InstanceShapeNames->get()[i].c_str();
        if (!strcmp(node->GetName(), shapename))
        {
            m_InstanceShapeINodes[i] = node;
            m_TotalShapesToEnumerate -= 1;
            break;
        }        
    }

    enumCode = m_TotalShapesToEnumerate > 0 ?  TREE_CONTINUE : TREE_ABORT;

    return enumCode;
}

Mesh *AlembicSimpleParticle::BuildPointMesh()
{
    return NULL;
}

Mesh *AlembicSimpleParticle::BuildBoxMesh()
{
    return NULL;
}

Mesh *AlembicSimpleParticle::BuildSphereMesh()
{
    return NULL;
}

Mesh *AlembicSimpleParticle::BuildCylinderMesh()
{
    return NULL;
}

Mesh *AlembicSimpleParticle::BuildConeMesh()
{
    return NULL;
}

Mesh *AlembicSimpleParticle::BuildDiscMesh()
{
    return NULL;
}

Mesh *AlembicSimpleParticle::BuildRectangleMesh()
{
    return NULL;
}

Mesh *AlembicSimpleParticle::BuildInstanceMesh(int meshNumber)
{
   if (meshNumber > m_InstanceShapeIds.size())
       return NULL;

    uint16_t shapeid = m_InstanceShapeIds[meshNumber];

    if (shapeid > m_InstanceShapeINodes.size())
        return NULL;

    INode *pNode = m_InstanceShapeINodes[shapeid];
    TimeValue t = m_InstanceShapeTimes[meshNumber];

    bool deleteIt = false;
    TriObject *triObj = GetTriObjectFromNode(pNode, t, deleteIt);

    if (!triObj)
        return NULL;

    Mesh *pMesh = new Mesh;
    *pMesh = triObj->mesh;

    if (deleteIt)
        delete triObj;

    return pMesh;
}

Mesh *AlembicSimpleParticle::BuildNbElementsMesh()
{
    return NULL;
}

RefResult AlembicSimpleParticle::NotifyRefChanged(
    Interval iv, 
    RefTargetHandle hTarg, 
    PartID& partID, 
    RefMessage msg) 
{
    switch (msg) 
    {
        case REFMSG_CHANGE:
            if (hTarg == pblock) 
            {
                ParamID changing_param = pblock->LastNotifyParamID();
                switch(changing_param)
                {
                case ID_PATH:
                    {
                        delRefArchive(m_CachedAbcFile);
                        MCHAR const* strPath = NULL;
                        TimeValue t = GetCOREInterface()->GetTime();
                        pblock->GetValue( AlembicSimpleParticle::ID_PATH, t, strPath, iv);
                        m_CachedAbcFile = strPath;
                        addRefArchive(m_CachedAbcFile);
                    }
                    break;
                default:
                    break;
                }

                AlembicSimpleParticleParams.InvalidateUI(changing_param);
            }
            break;

        case REFMSG_OBJECT_CACHE_DUMPED:
            return REF_STOP;
            break;
    }

    return REF_SUCCEED;
}

void AlembicSimpleParticle::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    s_EditObject  = this;

    SimpleParticle::BeginEditParams(ip, flags, prev);
	s_AlembicSimpleParticleClassDesc.BeginEditParams(ip, this, flags, prev);
}

void AlembicSimpleParticle::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
    SimpleParticle::EndEditParams(ip, flags, next);
	s_AlembicSimpleParticleClassDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    s_EditObject  = NULL;
}

void AlembicSimpleParticle::SetReference(int i, ReferenceTarget* pTarget)
{ 
    switch(i) 
    { 
    case ALEMBIC_SIMPLE_PARTICLE_REF_PBLOCK:
        pblock = static_cast<IParamBlock2*>(pTarget);
    default:
        break;
    }
}

RefTargetHandle AlembicSimpleParticle::GetReference(int i)
{ 
    switch(i)
    {
    case ALEMBIC_SIMPLE_PARTICLE_REF_PBLOCK:
        return pblock;
    default:
        return NULL;
    }
}

RefTargetHandle AlembicSimpleParticle::Clone(RemapDir& remap) 
{
	AlembicSimpleParticle *particle = new AlembicSimpleParticle();
    particle->ReplaceReference (ALEMBIC_SIMPLE_PARTICLE_REF_PBLOCK, remap.CloneRef(pblock));
   	
    BaseClone(this, particle, remap);
	return particle;
}

void AlembicSimpleParticle::ClearCurrentViewportMeshes()
{
    for (int i = 0; i < m_ParticleViewportMeshes.size(); i += 1)
    {
        if (m_ParticleViewportMeshes[i])
        {
            delete m_ParticleViewportMeshes[i];
            m_ParticleViewportMeshes[i] = NULL;
        }
    }
}

BOOL AlembicSimpleParticle::OKtoDisplay( TimeValue t)
{
    if (parts.Count() == 0)
        return FALSE;

    if (!valid)
        return FALSE;

    Interval interval = FOREVER;

	BOOL bMuted;
	this->pblock->GetValue( AlembicSimpleParticle::ID_MUTED, t, bMuted, interval);

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

int AlembicImport_Points(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
    // Find the object in the archive
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file, identifier);
	if (!iObj.valid())
    {
		return alembic_failure;
    }

    bool isConstant = false;
	if( !isAlembicPoints( &iObj, isConstant ) ) 
    {
		return alembic_failure;
	}

	// Create the particle emitter object and place it in the scene
    AlembicSimpleParticle *pParticleObj = static_cast<AlembicSimpleParticle*>
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

    return 0;
}
