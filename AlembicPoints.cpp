#include "Alembic.h"
#include "AlembicPoints.h"
#include "AlembicXform.h"
#include "SceneEntry.h"
#include "SimpObj.h"
#include "Utility.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;


AlembicPoints::AlembicPoints(const SceneEntry &in_Ref, AlembicWriteJob *in_Job)
    : AlembicObject(in_Ref, in_Job)
{
    std::string pointsName = in_Ref.node->GetName();
    std::string xformName = pointsName + "Xfo";

    Alembic::AbcGeom::OXform xform(GetOParent(), xformName.c_str(), GetCurrentJob()->GetAnimatedTs());
    Alembic::AbcGeom::OPoints points(xform, pointsName.c_str(), GetCurrentJob()->GetAnimatedTs());

    // create the generic properties
    mOVisibility = CreateVisibilityProperty(points, GetCurrentJob()->GetAnimatedTs());

    mXformSchema = xform.getSchema();
    mPointsSchema = points.getSchema();

    // create all properties
    mInstanceNamesProperty = OStringArrayProperty(mPointsSchema, ".instancenames", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );

    // particle attributes
    mScaleProperty = OV3fArrayProperty(mPointsSchema, ".scale", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mOrientationProperty = OQuatfArrayProperty(mPointsSchema, ".orientation", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mAngularVelocityProperty = OQuatfArrayProperty(mPointsSchema, ".angularvelocity", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mAgeProperty = OFloatArrayProperty(mPointsSchema, ".age", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mMassProperty = OFloatArrayProperty(mPointsSchema, ".mass", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeTypeProperty = OUInt16ArrayProperty(mPointsSchema, ".shapetype", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeTimeProperty = OFloatArrayProperty(mPointsSchema, ".shapetime", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeInstanceIDProperty = OUInt16ArrayProperty(mPointsSchema, ".shapeinstanceid", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mColorProperty = OC4fArrayProperty(mPointsSchema, ".color", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
}

AlembicPoints::~AlembicPoints()
{
    // we have to clear this prior to destruction this is a workaround for issue-171
    mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicPoints::GetCompound()
{
    return mPointsSchema;
}

bool AlembicPoints::Save(double time)
{
    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time);

    // Note: Should particles always be considered as animated?
    // If the points are not animated and this is not the first sample, we can just skip it
    if (mNumSamples > 0 && !GetRef().node->IsAnimated())
    {
        return true;
    }

    // Extract our particle emitter at the given time
    TimeValue ticks = GetTimeValueFromFrame(time);
    Object *obj = GetRef().node->EvalWorldState(ticks).obj;
    ParticleObject* particles = GetParticleInterface(obj);
    if (particles == NULL)
    {
        return false;
    }

    int numParticles = 0;

    // Set the visibility
    float flVisibility = GetRef().node->GetLocalVisibility(ticks);
    mOVisibility.set(flVisibility > 0 ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);

    // Store positions, velocity, width/size, scale, id, bounding box
    std::vector<Alembic::Abc::V3f> positionVec;
    std::vector<Alembic::Abc::V3f> velocityVec;
    std::vector<Alembic::Abc::V3f> scaleVec;
    std::vector<float> widthVec;
    std::vector<float> ageVec;
    std::vector<float> massVec;
    std::vector<float> shapeTimeVec;
    std::vector<uint64_t> idVec;
    std::vector<uint16_t> shapeTypeVec;
    std::vector<uint16_t> shapeInstanceIDVec;
    std::vector<Alembic::Abc::Quatf> orientationVec;
    std::vector<Alembic::Abc::Quatf> angularVelocityVec;
    std::vector<Alembic::Abc::C4f> colorVec;
    std::vector<std::string> instanceNamesVec;
    Alembic::Abc::Box3d bbox;
    Point3 pos;
    Point3 vel;
    Point3 scale;
    TimeValue age;
    TimeValue life;
    bool constantPos = true;
    bool constantVel = true;
    bool constantWidth = true;
    bool constantAge = true;
    int index = 0;

    do
    {
        age = particles->ParticleAge(ticks, index);
        life = particles->ParticleLife(ticks, index);
        if (age >= 0 && life > 0 && age < life)
        {
            ConvertMaxPointToAlembicPoint(particles->ParticlePosition(ticks, index), pos);
            ConvertMaxPointToAlembicPoint(particles->ParticleVelocity(ticks, index), vel);
            float width = ScaleFloatFromInchesToDecimeters(particles->ParticleSize(ticks, index));
            uint64_t id = index;    // This is not quite right

            positionVec.push_back(Alembic::Abc::V3f(pos.x, pos.y, pos.z));
            velocityVec.push_back(Alembic::Abc::V3f(vel.x, vel.y, vel.z));
            widthVec.push_back(width);
            ageVec.push_back(static_cast<float>(GetSecondsFromTimeValue(age)));
            idVec.push_back(id);
            bbox.extendBy(positionVec[numParticles]);

            constantPos &= (positionVec[numParticles] == positionVec[0]);
            constantVel &= (velocityVec[numParticles] == velocityVec[0]);
            constantWidth &= (widthVec[numParticles] == widthVec[0]);
            constantAge &= (ageVec[numParticles] == ageVec[0]);

            ++numParticles;
        }

        ++index;
    }
    while (life > 0);

    // Set constant properties that are not currently supported by Max
    scaleVec.push_back(Alembic::Abc::V3f(1.0f, 1.0f, 1.0f));
    massVec.push_back(1.0f);
    shapeTimeVec.push_back(1.0f);
    shapeTypeVec.push_back(ShapeType_Point);
    shapeInstanceIDVec.push_back(0);
    orientationVec.push_back(Alembic::Abc::Quatf(0.0f, 1.0f, 0.0f, 0.0f));
    angularVelocityVec.push_back(Alembic::Abc::Quatf(0.0f, 1.0f, 0.0f, 0.0f));
    colorVec.push_back(Alembic::Abc::C4f(0.0f, 0.0f, 0.0f, 1.0f));
    instanceNamesVec.push_back("");

    if (numParticles == 0)
    {
        positionVec.push_back(Alembic::Abc::V3f(FLT_MAX, FLT_MAX, FLT_MAX));
        velocityVec.push_back(Alembic::Abc::V3f(0.0f, 0.0f, 0.0f));
        widthVec.push_back(0.0f);
        ageVec.push_back(0.0f);
        idVec.push_back(static_cast<uint64_t>(-1));
    }
    else
    {
        if (constantPos)        { positionVec.resize(1); }
        if (constantVel)        { velocityVec.resize(1); }
        if (constantWidth)      { widthVec.resize(1); }
        if (constantAge)        { ageVec.resize(1); }
    }

    // Store the information into our properties and points schema
    Alembic::Abc::P3fArraySample positionSample(&positionVec.front(), positionVec.size());
    Alembic::Abc::P3fArraySample velocitySample(&velocityVec.front(), velocityVec.size());
    Alembic::Abc::P3fArraySample scaleSample(&scaleVec.front(), scaleVec.size());
    Alembic::Abc::FloatArraySample widthSample(&widthVec.front(), widthVec.size());
    Alembic::Abc::FloatArraySample ageSample(&ageVec.front(), ageVec.size());
    Alembic::Abc::FloatArraySample massSample(&massVec.front(), massVec.size());
    Alembic::Abc::FloatArraySample shapeTimeSample(&shapeTimeVec.front(), shapeTimeVec.size());
    Alembic::Abc::UInt64ArraySample idSample(&idVec.front(), idVec.size());
    Alembic::Abc::UInt16ArraySample shapeTypeSample(&shapeTypeVec.front(), shapeTypeVec.size());
    Alembic::Abc::UInt16ArraySample shapeInstanceIDSample(&shapeInstanceIDVec.front(), shapeInstanceIDVec.size());
    Alembic::Abc::QuatfArraySample orientationSample(&orientationVec.front(), orientationVec.size());
    Alembic::Abc::QuatfArraySample angularVelocitySample(&angularVelocityVec.front(), angularVelocityVec.size());
    Alembic::Abc::C4fArraySample colorSample(&colorVec.front(), colorVec.size());
    Alembic::Abc::StringArraySample instanceNamesSample(&instanceNamesVec.front(), instanceNamesVec.size());

    mScaleProperty.set(scaleSample);
    mAgeProperty.set(ageSample);
    mMassProperty.set(massSample);
    mShapeTimeProperty.set(shapeTimeSample);
    mShapeTypeProperty.set(shapeTypeSample);
    mShapeInstanceIDProperty.set(shapeInstanceIDSample);
    mOrientationProperty.set(orientationSample);
    mAngularVelocityProperty.set(angularVelocitySample);
    mColorProperty.set(colorSample);
    mInstanceNamesProperty.set(instanceNamesSample);

    mPointsSample.setPositions(positionSample);
    mPointsSample.setVelocities(velocitySample);
    mPointsSample.setWidths(Alembic::AbcGeom::OFloatGeomParam::Sample(widthSample, Alembic::AbcGeom::kVertexScope));
    mPointsSample.setIds(idSample);
    mPointsSample.setSelfBounds(bbox);

    mPointsSchema.set(mPointsSample);

    mNumSamples++;

    return true;
}
