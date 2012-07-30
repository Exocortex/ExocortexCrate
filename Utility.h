#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"
#include "AlembicIntermediatePolyMesh3DSMax.h"
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
void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Matrix3 &alembicMatrix );
void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Alembic::Abc::M44d& iMatrix);
void ConvertAlembicMatrixToMaxMatrix( const Matrix3 &alembicMatrix, Matrix3 &maxMatrix );

Imath::M33d extractRotation(Imath::M44d& m);

inline Imath::V3f ConvertMaxPointToAlembicPoint( const Point3 &maxPoint )
{
	return Imath::V3f(maxPoint.x, maxPoint.z, -maxPoint.y);
}

inline Imath::V4f ConvertMaxPointToAlembicPoint4( const Point3 &maxPoint )
{
	return Imath::V4f(maxPoint.x, maxPoint.z, -maxPoint.y, 1.0);
}

inline Point3 ConvertAlembicPointToMaxPoint( const Imath::V3f &alembicPoint )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y);
}

inline Imath::V3f ConvertMaxVectorToAlembicVector( const Point3 &maxPoint )
{
	return Imath::V3f(maxPoint.x, maxPoint.z, -maxPoint.y);
}

inline Imath::V4f ConvertMaxVectorToAlembicVector4( const Point3 &maxPoint )
{
	return Imath::V4f(maxPoint.x, maxPoint.z, -maxPoint.y, 0.0);
}

inline Point3 ConvertAlembicVectorToMaxVector( const Imath::V3f &alembicPoint )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y);
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

inline void ConvertMaxAngAxisToAlembicQuat(const AngAxis &angAxis, Alembic::Abc::Quatd &quat)
{
    Imath::V3f alembicAxis = ConvertMaxNormalToAlembicNormal(angAxis.axis);
    quat.setAxisAngle(alembicAxis, angAxis.angle);
    quat.normalize();
}

// Utility functions for working on INodes
bool CheckIfNodeIsAnimated( INode *pNode );
bool CheckIfObjIsValidForever(Object *obj, TimeValue v);
bool IsModelTransformNode( INode *pNode );
INode *GetParentModelTransformNode( INode *pNode );
void LockNodeTransform(INode *pNode, bool bLock);

int GetParamIdByName( Animatable *pBaseObject, int pblockIndex, char const* pParamName );
TriObject* GetTriObjectFromNode(INode *iNode, const TimeValue t, bool &deleteIt);


INode* GetNodeFromHierarchyPath(const std::string& path);
INode* GetNodeFromName(const std::string& name);
INode* GetChildNodeFromName(const std::string& name, INode* pParent);
std::string getNodePath(const std::string& name);

Modifier* FindModifier(INode* node, char* name);
void printAnimatables(Animatable* pObj);

class AlembicPathAccessor : public  IAssetAccessor	{
	public:

	AlembicPathAccessor(Animatable* pAnimatable) : pAnimatable(pAnimatable) {}

	// path accessor functions
	MaxSDK::AssetManagement::AssetUser GetAsset() const {
		return pAnimatable->GetParamBlockByID( 0 )->GetAssetUser( GetParamIdByName( pAnimatable, 0, "path" ), 0 );
	}

	bool SetAsset(const MaxSDK::AssetManagement::AssetUser& aNewAssetUser) {
		pAnimatable->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pAnimatable, 0, "path" ), 0, aNewAssetUser );
		return true;
	}

	// asset client information
	MaxSDK::AssetManagement::AssetType GetAssetType() const 	{ return MaxSDK::AssetManagement::kExternalLink; }

protected:
	Animatable* pAnimatable;
};


template<class OBJTYPE, class DATATYPE>
bool getArbGeomParamPropertyAlembic( OBJTYPE obj, std::string name, Alembic::Abc::ITypedArrayProperty<DATATYPE> &pOut ) {
	// look for name with period on it.
	std::string nameWithDotPrefix = std::string(".") + name;
	if ( obj.getSchema().getPropertyHeader( nameWithDotPrefix ) != NULL ) {
		Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema(), nameWithDotPrefix );
		if( prop.valid() && prop.getNumSamples() > 0 ) {
			pOut = prop;
			return true;
		}
	}
	if( obj.getSchema().getArbGeomParams() != NULL ) {
		if ( obj.getSchema().getArbGeomParams().getPropertyHeader( name ) != NULL ) {
			Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema().getArbGeomParams(), name );
			if( prop.valid() && prop.getNumSamples() > 0 ) {
				pOut = prop;
				return true;
			}
		}
		if ( obj.getSchema().getArbGeomParams().getPropertyHeader( nameWithDotPrefix ) != NULL ) {
			Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema().getArbGeomParams(), nameWithDotPrefix );
			if( prop.valid() && prop.getNumSamples() > 0 ) {
				pOut = prop;
				return true;
			}
		}
	}

	return false;
}


#endif  // _FOUNDATION_H_
