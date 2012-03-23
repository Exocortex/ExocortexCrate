#ifndef _ALEMBIC_POINTS_H_
#define _ALEMBIC_POINTS_H_

#include "AlembicObject.h"

class IParticleObjectExt;

class AlembicPoints: public AlembicObject
{
public:
    enum ShapeType
    {
      ShapeType_Point,
      ShapeType_Box,
      ShapeType_Sphere,
      ShapeType_Cylinder,
      ShapeType_Cone,
      ShapeType_Disc,
      ShapeType_Rectangle,
      ShapeType_Instance,
      ShapeType_NbElements
    };
private:
    static void AlembicPoints::ConvertMaxEulerXYZToAlembicQuat(const Point3 &degrees, Alembic::Abc::Quatf &quat);
    static void AlembicPoints::ConvertMaxAngAxisToAlembicQuat(const AngAxis &angAxis, Alembic::Abc::Quatf &quat);
    static void AlembicPoints::GetShapeType(IParticleObjectExt *pExt, int particleId, TimeValue ticks, ShapeType &type, unsigned short &instanceId, float &animationTime, std::vector<std::string> &nameList);

    Alembic::AbcGeom::OXformSchema mXformSchema;
    Alembic::AbcGeom::OPointsSchema mPointsSchema;
    Alembic::AbcGeom::XformSample mXformSample;
    Alembic::AbcGeom::OPointsSchema::Sample mPointsSample;

    // instance lookups
    Alembic::Abc::ALEMBIC_VERSION_NS::OStringArrayProperty mInstanceNamesProperty;
    std::vector<std::string> mInstanceNames;
    std::map<unsigned long, size_t> mInstanceMap;

    // Additional particle attributes
    Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mScaleProperty;
    Alembic::Abc::ALEMBIC_VERSION_NS::OQuatfArrayProperty mOrientationProperty;
    Alembic::Abc::ALEMBIC_VERSION_NS::OQuatfArrayProperty mAngularVelocityProperty;
    Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mAgeProperty;
    Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mMassProperty;
    Alembic::Abc::ALEMBIC_VERSION_NS::OUInt16ArrayProperty mShapeTypeProperty;
    Alembic::Abc::ALEMBIC_VERSION_NS::OFloatArrayProperty mShapeTimeProperty;
    Alembic::Abc::ALEMBIC_VERSION_NS::OUInt16ArrayProperty mShapeInstanceIDProperty;
    Alembic::Abc::ALEMBIC_VERSION_NS::OC4fArrayProperty mColorProperty;

public:
    AlembicPoints(const SceneEntry & in_Ref, AlembicWriteJob * in_Job);
    ~AlembicPoints();

    virtual Alembic::Abc::OCompoundProperty GetCompound();
    virtual bool Save(double time);
};

#endif
