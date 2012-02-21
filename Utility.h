#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"

const double ALEMBIC_3DSMAX_TICK_VALUE = 4800;

class SceneEntry;

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

SampleInfo getSampleInfo(double iFrame,Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps);
std::string buildIdentifierFromRef(const SceneEntry &in_Ref);
std::string buildModelIdFromXFormId(const std::string &xformId);
std::string getIdentifierFromRef(const SceneEntry &in_Ref);
std::string getModelName( const std::string &identifier );
std::string getModelFullName( const std::string &identifier );
double GetSecondsFromTimeValue(TimeValue t); 
int GetTimeValueFromSeconds( double seconds );
int GetTimeValueFromFrame( double frame );

// Debug functions
void ALEMBIC_DEBUG( char *debugmsg, ...);
void AlembicDebug_PrintMeshData( Mesh &mesh );
void AlembicDebug_PrintTransform(Matrix3 &m);

// Conversion functions to Alembic Standards
Point3 ScalePointFromInchesToDecimeters( const Point3 &inches );
Point3 ScalePointFromDecimetersToInches( const Point3 &decimeters );
void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Matrix3 &alembicMatrix);
void ConvertAlembicMatrixToMaxMatrix( const Matrix3 &alembicMatrix, Matrix3 &maxMatrix);
void ConvertMaxPointToAlembicPoint( const Point3 &maxPoint, Point3 &result);
void ConvertAlembicPointToMaxPoint( const Point3 &alembicPoint, Point3 &result);
void ConvertMaxNormalToAlembicNormal( const Point3 &maxPoint, Point3 &result);
void ConvertAlembicNormalToMaxNormal( const Point3 &alembicPoint, Point3 &result);


#endif  // _FOUNDATION_H_
