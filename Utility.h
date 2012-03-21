#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"
#include "AlembicPolyMsh.h"

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
inline float GetInchesToDecimetersRatio( const float& masterScaleUnitMeters )
{
    float flDecimetersPerInch = masterScaleUnitMeters;
    flDecimetersPerInch *= 10.0f;
    return flDecimetersPerInch;
}

inline float GetDecimetersToInchesRatio( const float& masterScaleUnitMeters )
{
    float flDecimetersPerInch = masterScaleUnitMeters;
    flDecimetersPerInch *= 10.0f;
    return 1.0f / flDecimetersPerInch;
}

void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, const float& masterScaleUnitMeters, Matrix3 &alembicMatrix );
void ConvertAlembicMatrixToMaxMatrix( const Matrix3 &alembicMatrix, const float& masterScaleUnitMeters, Matrix3 &maxMatrix );

inline Imath::V3f ConvertMaxPointToAlembicPoint( const Point3 &maxPoint, const float &masterScaleUnitMeters)
{
	return Imath::V3f(maxPoint.x, maxPoint.z, -maxPoint.y) * GetInchesToDecimetersRatio( masterScaleUnitMeters );
}

inline Point3 ConvertAlembicPointToMaxPoint( const Imath::V3f &alembicPoint, const float &masterScaleUnitMeters )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y) * GetDecimetersToInchesRatio( masterScaleUnitMeters );
}

inline Imath::V3f ConvertMaxVectorToAlembicVector( const Point3 &maxPoint, const float &masterScaleUnitMeters, bool scale )
{
	return Imath::V3f(maxPoint.x, maxPoint.z, -maxPoint.y) * (scale ? GetInchesToDecimetersRatio( masterScaleUnitMeters ) : 1.0f);
}

inline Point3 ConvertAlembicPointToMaxPoint( const Imath::V3f &alembicPoint, const float &masterScaleUnitMeters, bool scale )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y) * (scale ? GetDecimetersToInchesRatio( masterScaleUnitMeters ) : 1.0f);
}

inline Imath::V3f ConvertMaxNormalToAlembicNormal( const Point3 &maxPoint )
{
     Point3 maxPointNormalized = maxPoint.Normalize();
	 return Imath::V3f( maxPoint.x, maxPoint.z, -maxPoint.y);
}

inline Point3 ConvertAlembicNormalToMaxNormal( const Imath::V3f &alembicPoint )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y);
}

inline Point3 ConvertAlembicNormalToMaxNormal_Normalized( const Imath::V3f &alembicPoint )
{
	return ConvertAlembicNormalToMaxNormal(alembicPoint).Normalize();
}



// Utility functions for working on INodes
bool CheckIfNodeIsAnimated( INode *pNode );
bool IsModelTransformNode( INode *pNode );
INode *GetParentModelTransformNode( INode *pNode );
void LockNodeTransform(INode *pNode, bool bLock);

int GetParamIdByName( Animatable *pBaseObject, int pblockIndex, char const* pParamName );

#endif  // _FOUNDATION_H_
