#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"
#include "AlembicPolyMsh.h"
#include "AlembicLicensing.h"

const double ALEMBIC_3DSMAX_TICK_VALUE = 4800;

#define ALEMBIC_SAFE_DELETE(p)  if(p) delete p; p = 0;

class SceneEntry;

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

SampleInfo getSampleInfo(double iFrame,Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps);
Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::IObject object);
std::string buildIdentifierFromRef(const SceneEntry &in_Ref);
std::string buildModelIdFromXFormId(const std::string &xformId);
std::string getIdentifierFromRef(const SceneEntry &in_Ref);
std::string getModelName( const std::string &identifier );
std::string getModelFullName( const std::string &identifier );
double GetSecondsFromTimeValue(TimeValue t); 
int GetTimeValueFromSeconds( double seconds );
int GetTimeValueFromFrame( double frame );

// Debug functions
void AlembicDebug_PrintMeshData( Mesh &mesh, std::vector<VNormal> &sgVertexNormals );
void AlembicDebug_PrintTransform( Matrix3 &m );

// Conversion functions to Alembic Standards
inline float GetMasterUnitToDecimeterRatio( const float& masterScaleUnitMeters )
{
    return masterScaleUnitMeters * 10;
}

inline float GetDecimeterToMasterUnitRatio( const float& masterScaleUnitMeters )
{
    return ( 1.0f / GetMasterUnitToDecimeterRatio( masterScaleUnitMeters ) );
}

inline void TestMasterUnit() {
	ESS_LOG_WARNING( "GetMasterScale(UNITS_CENTIMETERS): " << GetMasterScale(UNITS_CENTIMETERS) );
	ESS_LOG_WARNING( "GetMasterScale(UNITS_METERS): " << GetMasterScale(UNITS_METERS) );
	ESS_LOG_WARNING( "GetMasterScale(UNITS_INCHES): " << GetMasterScale(UNITS_INCHES) );
	ESS_LOG_WARNING( "GetMasterScale(UNITS_FEET): " << GetMasterScale(UNITS_FEET) );
}
void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, const float& masterScaleUnitMeters, Matrix3 &alembicMatrix );
void ConvertAlembicMatrixToMaxMatrix( const Matrix3 &alembicMatrix, const float& masterScaleUnitMeters, Matrix3 &maxMatrix );

inline Imath::V3f ConvertMaxPointToAlembicPoint( const Point3 &maxPoint, const float &masterScaleUnitMeters)
{
	return Imath::V3f(maxPoint.x, maxPoint.z, -maxPoint.y) * GetMasterUnitToDecimeterRatio( masterScaleUnitMeters );
}

inline Point3 ConvertAlembicPointToMaxPoint( const Imath::V3f &alembicPoint, const float &masterScaleUnitMeters )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y) * GetDecimeterToMasterUnitRatio( masterScaleUnitMeters );
}

inline Imath::V3f ConvertMaxVectorToAlembicVector( const Point3 &maxPoint, const float &masterScaleUnitMeters, bool scale )
{
	return Imath::V3f(maxPoint.x, maxPoint.z, -maxPoint.y) * (scale ? GetMasterUnitToDecimeterRatio( masterScaleUnitMeters ) : 1.0f);
}

inline Point3 ConvertAlembicVectorToMaxVector( const Imath::V3f &alembicPoint, const float &masterScaleUnitMeters, bool scale )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y) * (scale ? GetDecimeterToMasterUnitRatio( masterScaleUnitMeters ) : 1.0f);
}

inline Imath::V3f ConvertMaxNormalToAlembicNormal( const Point3 &maxPoint )
{
     Point3 maxPointNormalized = maxPoint.Normalize();
	 return Imath::V3f( maxPoint.x, maxPoint.z, -maxPoint.y);
}

inline Point3 ConvertAlembicNormalToMaxNormal( const Imath::V3f &alembicPoint )
{
	return Point3( alembicPoint.x, -alembicPoint.z, alembicPoint.y );
}

inline Point3 ConvertAlembicNormalToMaxNormal_Normalized( const Imath::V3f &alembicPoint )
{
	return ConvertAlembicNormalToMaxNormal(alembicPoint).Normalize();
}

inline Imath::V3f ConvertMaxScaleToAlembicScale( const Point3 &maxScale )
{
	return Imath::V3f(maxScale.x, maxScale.z, maxScale.y);
}

inline Point3 ConvertAlembicScaleToMaxScale( const Imath::V3f &alembicScale )
{
	return Point3(alembicScale.x, alembicScale.z, alembicScale.y);
}

inline Quat ConvertAlembicQuatToMaxQuat( const Imath::Quatf &alembicQuat, bool bNormalize)
{
    Quat q(alembicQuat.v.x, -alembicQuat.v.z, alembicQuat.v.y, -alembicQuat.r);

    if (bNormalize)
        q.Normalize();

    return q;
}



// Utility functions for working on INodes
bool CheckIfNodeIsAnimated( INode *pNode );
bool CheckIfObjIsValidForever(Object *obj, TimeValue v);
bool IsModelTransformNode( INode *pNode );
INode *GetParentModelTransformNode( INode *pNode );
void LockNodeTransform(INode *pNode, bool bLock);

int GetParamIdByName( Animatable *pBaseObject, int pblockIndex, char const* pParamName );
TriObject* GetTriObjectFromNode(INode *iNode, const TimeValue t, bool &deleteIt);

#endif  // _FOUNDATION_H_
