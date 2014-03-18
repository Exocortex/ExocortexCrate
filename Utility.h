#ifndef _UTILITY_H_
#define _UTILITY_H_


#include "AlembicIntermediatePolyMesh3DSMax.h"
#include "AlembicLicensing.h"
#include "CommonUtilities.h"


const double ALEMBIC_3DSMAX_TICK_VALUE = 4800;


class SceneEntry;

std::string buildIdentifierFromRef(const SceneEntry &in_Ref);
//std::string buildModelIdFromXFormId(const std::string &xformId);
std::string getIdentifierFromRef(const SceneEntry &in_Ref);
//std::string getModelFullName( const std::string &identifier );
double GetSecondsFromTimeValue(TimeValue t); 
int GetTimeValueFromSeconds( double seconds );
int GetTimeValueFromFrame( double frame );
void RoundTicksToNearestFrame( int& nTicks, float& fTimeAlpha );

// Debug functions
void AlembicDebug_PrintMeshData( Mesh &mesh, std::vector<VNormal> &sgVertexNormals );
void AlembicDebug_PrintTransform( Matrix3 &m );

// Conversion functions to Alembic Standards
void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Matrix3 &alembicMatrix );
void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Abc::M44d& iMatrix);
void ConvertAlembicMatrixToMaxMatrix( const Matrix3 &alembicMatrix, Matrix3 &maxMatrix );

inline Abc::V3f ConvertMaxPointToAlembicPoint( const Point3 &maxPoint )
{
	return Abc::V3f(maxPoint.x, maxPoint.z, -maxPoint.y);
}

inline Abc::V4f ConvertMaxPointToAlembicPoint4( const Point3 &maxPoint )
{
	return Abc::V4f(maxPoint.x, maxPoint.z, -maxPoint.y, 1.0);
}

inline Point3 ConvertAlembicPointToMaxPoint( const Abc::V3f &alembicPoint )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y);
}

inline Abc::V3f ConvertMaxVectorToAlembicVector( const Point3 &maxPoint )
{
	return Abc::V3f(maxPoint.x, maxPoint.z, -maxPoint.y);
}

inline Abc::V4f ConvertMaxVectorToAlembicVector4( const Point3 &maxPoint )
{
	return Abc::V4f(maxPoint.x, maxPoint.z, -maxPoint.y, 0.0);
}

inline Point3 ConvertAlembicVectorToMaxVector( const Abc::V3f &alembicPoint )
{
	return Point3(alembicPoint.x, -alembicPoint.z, alembicPoint.y);
}

inline Abc::V3f ConvertMaxNormalToAlembicNormal( const Abc::V3f &maxPoint )
{
   Abc::V3f maxPointNormalized = maxPoint.normalized();
	 return Abc::V3f( maxPoint.x, maxPoint.z, -maxPoint.y);
}

inline Abc::V3f ConvertMaxNormalToAlembicNormal( const Point3 &maxPoint )
{
     Point3 maxPointNormalized = maxPoint.Normalize();
	 return Abc::V3f( maxPoint.x, maxPoint.z, -maxPoint.y);
}

inline Point3 ConvertAlembicNormalToMaxNormal( const Abc::V3f &alembicPoint )
{
	return Point3( alembicPoint.x, -alembicPoint.z, alembicPoint.y );
}

inline Point3 ConvertAlembicNormalToMaxNormal_Normalized( const Abc::V3f &alembicPoint )
{
	return ConvertAlembicNormalToMaxNormal(alembicPoint).Normalize();
}

inline Abc::V3f ConvertMaxScaleToAlembicScale( const Point3 &maxScale )
{
	return Abc::V3f(maxScale.x, maxScale.z, maxScale.y);
}

inline Point3 ConvertAlembicScaleToMaxScale( const Abc::V3f &alembicScale )
{
	return Point3(alembicScale.x, alembicScale.z, alembicScale.y);
}

inline Quat ConvertAlembicQuatToMaxQuat( const Abc::Quatf &alembicQuat, bool bNormalize)
{
    Quat q(alembicQuat.v.x, -alembicQuat.v.z, alembicQuat.v.y, -alembicQuat.r);

    if (bNormalize)
        q.Normalize();

    return q;
}

//inline Abc::Quatf ConvertMaxQuatToAlembicQuat( const Abc::Quatf &alembicQuat, bool bNormalize)
//{
//	Abc::Quatf q(alembicQuat.v.x, alembicQuat.v.z, -alembicQuat.v.y, -alembicQuat.r);
//
//    if (bNormalize)
//		q.normalize();
//
//    return q;
//}

inline void ConvertMaxAngAxisToAlembicQuat(const AngAxis &angAxis, Abc::Quatd &quat)
{
    Abc::V3f alembicAxis = ConvertMaxNormalToAlembicNormal(angAxis.axis);
    quat.setAxisAngle(alembicAxis, angAxis.angle);
    quat.normalize();
}

// Utility functions for working on INodes
//bool CheckIfNodeIsAnimated( INode *pNode );
bool CheckIfObjIsValidForever(Object *obj, TimeValue v);
bool IsModelTransformNode( INode *pNode );
INode *GetParentModelTransformNode( INode *pNode );
void LockNodeTransform(INode *pNode, bool bLock);

int GetParamIdByName( Animatable *pBaseObject, int pblockIndex, char const* pParamName );
TriObject* GetTriObjectFromNode(INode *iNode, const TimeValue t, bool &deleteIt);


INode* GetNodeFromHierarchyPath(const std::string& path);
//INode* GetNodeFromName(const std::string& name);
INode* GetChildNodeFromName(const std::string& name, INode* pParent);
std::string getNodeAlembicPath(const std::string& name, bool bFlatten);

typedef std::map<std::string, INode*> INodeMap;
void buildINodeMap(INodeMap& nodeMap);

std::string alembicPathToMaxPath(const std::string& path);

Modifier* FindModifier(INode* node, char* name);
Modifier* FindModifier(INode* node, Class_ID obtype, const char* identifier);
Modifier* FindModifier(INode* node, Class_ID obtype);
void printControllers(Animatable* anim);
Matrix3 TransposeRot(const Matrix3& mat);

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




class SmoothGroupNormals
{//this class was refactored from AlembicIntermediatePolyMesh3DSMax, the save method still needs to be updated to use the new code
	std::vector<VNormal> m_MeshSmoothGroupNormals;
public:
	Point3 GetVertexNormal(Mesh *mesh, int faceNo, int faceVertNo);
	Point3 GetVertexNormal(MNMesh *mesh, int faceNo, int faceVertNo);
	void BuildMeshSmoothingGroupNormals(Mesh &mesh);
	void BuildMeshSmoothingGroupNormals(MNMesh &mesh);
	void ClearMeshSmoothingGroupNormals();
};

void printChannelIntervals(TimeValue t, Object* obj);

int createNode(AbcG::IObject& iObj, SClass_ID superID, Class_ID classID, INode** pMaxNode, bool& bReplaceExisting);

#define GET_MAXSCRIPT_NODE(pNode) "mynode2113 = maxOps.getNodeByHandle("<<pNode->GetHandle()<<");\n"

#endif  // _FOUNDATION_H_
