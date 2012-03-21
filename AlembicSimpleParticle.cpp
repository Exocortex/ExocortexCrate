#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicSimpleParticle.h"
#include "AlembicArchiveStorage.h"
#include "AlembicVisibilityController.h"
#include "utility.h"
#include "iparamb2.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

static AlembicSimpleParticleClassDesc s_AlembicSimpleParticleClassDesc;
ClassDesc2 *GetAlembicSimpleParticleClassDesc() { return &s_AlembicSimpleParticleClassDesc; }

// static member variables
AlembicSimpleParticle *AlembicSimpleParticle::s_EditObject = NULL;
IObjParam *AlembicSimpleParticle::s_ObjParam = NULL;

AlembicSimpleParticle::AlembicSimpleParticle()
    : SimpleParticle()
{
}

// virtual
AlembicSimpleParticle::~AlembicSimpleParticle()
{
}

void AlembicSimpleParticle::SetAlembicId(const std::string &file, const std::string &identifier)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

void AlembicSimpleParticle::UpdateParticles(TimeValue t, INode *node)
{
	ESS_CPP_EXCEPTION_REPORTING_START

	Alembic::AbcGeom::IPoints iPoints;
    if (!GetAlembicIPoints(iPoints))
    {
        return;
    }

    Alembic::AbcGeom::IPointsSchema::Sample floorSample;
    Alembic::AbcGeom::IPointsSchema::Sample ceilSample;
    SampleInfo sampleInfo = GetSampleAtTime(iPoints, t, floorSample, ceilSample);

    int numParticles = GetNumParticles(floorSample);
    parts.SetCount(numParticles, PARTICLE_VELS | PARTICLE_AGES | PARTICLE_RADIUS);
    parts.SetCustomDraw(NULL);

    for (int i = 0; i < numParticles; ++i)
    {
        parts.points[i] = GetParticlePosition(floorSample, ceilSample, sampleInfo, i);
        parts.vels[i] = GetParticleVelocity(floorSample, ceilSample, sampleInfo, i);
        parts.ages[i] = GetParticleAge(iPoints, sampleInfo, i);
        parts.radius[i] = GetParticleRadius(iPoints, sampleInfo, i);
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

Interval AlembicSimpleParticle::GetValidity(TimeValue t)
{
    Interval interval(t, t);
    return interval;
}

MarkerType AlembicSimpleParticle::GetMarkerType()
{
    return POINT_MRKR;
}

bool AlembicSimpleParticle::GetAlembicIPoints(Alembic::AbcGeom::IPoints &iPoints)
{
    // Find the object in the archive
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
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

	const Imath::V3f & alembicP3f = floorSample.getPositions()->get()[index];

	// TODO: Interpolate the position (using the velocity) if there is an alpha.
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


int AlembicImport_Points(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
    // Find the object in the archive
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file, identifier);
	if (!iObj.valid())
    {
		return alembic_failure;
    }

	// Create the particle emitter object and place it in the scene
    AlembicSimpleParticle *pParticleObj = new AlembicSimpleParticle();
    if (pParticleObj == NULL)
    {
        return alembic_failure;
    }

    // Set the alembic information
    pParticleObj->SetAlembicId(file, identifier);

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

    return 0;
}
