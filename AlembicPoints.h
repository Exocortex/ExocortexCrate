#ifndef _ALEMBIC_POINTS_H_
#define _ALEMBIC_POINTS_H_

#include "AlembicObject.h"
#include <Alembic/Util/Digest.h>
#include <ParticleFlow/IParticleChannelTime.h>

class IParticleObjectExt;
class PFSimpleOperator;
class IParticleGroup;

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
public:
    enum 
    {	
        PFlow_kInstanceShape_sepGroup,
		PFlow_kInstanceShape_sepHierarchy,
		PFlow_kInstanceShape_sepElements,
		PFlow_kInstanceShape_scale,
		PFlow_kInstanceShape_variation,
		PFlow_kInstanceShape_mapping,
		PFlow_kInstanceShape_material,
		PFlow_kInstanceShape_randomShape,
		PFlow_kInstanceShape_animatedShape,
		PFlow_kInstanceShape_acquireShape,
		PFlow_kInstanceShape_syncType,
		PFlow_kInstanceShape_syncRandom,
		PFlow_kInstanceShape_randomOffset,
		PFlow_kInstanceShape_randomSeed,
		PFlow_kInstanceShape_setScale,
		PFlow_kInstanceShape_objectMaxscript,
		PFlow_kInstanceShape_objectList,
		PFlow_kInstanceShape_fastShapeEvaluation
    };
public:
    enum 
    {	
        PFlow_kInstanceShape_syncBy_absoluteTime,
		PFlow_kInstanceShape_syncBy_particleAge,
		PFlow_kInstanceShape_syncBy_eventStart,
		PFlow_kInstanceShape_syncBy_num=3 
    };
public:
    enum 
    {	
        PFlow_kSimpleShape_shape, 
        PFlow_kSimpleShape_size,
        PFlow_kSimpleShape_useScale,
        PFlow_kSimpleShape_scale 
    };
public:
    enum 
    {	
        PFlow_kSimpleShape_shape_pyramid, 
        PFlow_kSimpleShape_shape_cube,
        PFlow_kSimpleShape_shape_sphere,
        PFlow_kSimpleShape_shape_vertex,
        PFlow_kSimpleShape_shape_num=4 
    };
public:
	enum
	{
		PFlow_kShapeLibary_dimensionType,
		PFlow_kShapeLibary_2DType,
		PFlow_kShapeLibary_3DType,
		PFlow_kShapeLibary_size,
		PFlow_kShapeLibary_useScale,
		PFlow_kShapeLibary_scale,
		PFlow_kShapeLibary_scaleVariation,
		PFlow_kShapeLibary_randomMultishapeOrder,
		PFlow_kShapeLibary_randomSeed,
		PFlow_kShapeLibary_generateMappingCoords,
		PFlow_kShapeLibary_FitMapping
	};
public:
	enum
	{
		PFlow_kShapeLibrary_dimensionType_2D,
		PFlow_kShapeLibrary_dimensionType_3D
	};
public:
	enum
	{
		PFlow_kShapeLibary_3DType_cube,
		PFlow_kShapeLibary_3DType_diamond,
		PFlow_kShapeLibary_3DType_long,
		PFlow_kShapeLibary_3DType_digitsArial,
		PFlow_kShapeLibary_3DType_digitsCourier,
		PFlow_kShapeLibary_3DType_digitsTimes,
		PFlow_kShapeLibary_3DType_Dodecahedron,
		PFlow_kShapeLibary_3DType_Heart,
		PFlow_kShapeLibary_3DType_LettersArial,
		PFlow_kShapeLibary_3DType_LettersCourier,
		PFlow_kShapeLibary_3DType_LettersTimes,
		PFlow_kShapeLibary_3DType_Notes,
		PFlow_kShapeLibary_3DType_Pyramid,
		PFlow_kShapeLibary_3DType_Special,
		PFlow_kShapeLibary_3DType_Sphere20sides,
		PFlow_kShapeLibary_3DType_Sphere80sides,
		PFlow_kShapeLibary_3DType_Star5point,
		PFlow_kShapeLibary_3DType_Star6point,
		PFlow_kShapeLibary_3DType_Tetra,
		PFlow_kShapeLibary_3DType_Torus
	};
public:
	enum
	{
		PFlow_kShapeLibrary_dimensionType_2D_circle12sides = 0,
		PFlow_kShapeLibrary_dimensionType_2D_circle24sides = 1,
		PFlow_kShapeLibrary_dimensionType_2D_square = 14
	};
public:
	enum
	{	
		kDisplay_type, 
		kDisplay_visible,
		kDisplay_color,
		kDisplay_showNumbering,
		kDisplay_selectedType,
	};
private:

	struct shapeInfo
	{
		ShapeType type;
		float animationTime;
		std::string instanceName;
		INode* pParentActionList;

		shapeInfo():type(ShapeType_NbElements), animationTime(-1.0), instanceName(), pParentActionList(NULL)
		{}
	};

	//maps born actionList to shape type
	//needed so that we can find the parent shape in the case that there is no shape assignment
	typedef std::map<INode*, shapeInfo> perActionListShapeMap;
	typedef std::map<INode*, shapeInfo>::iterator perActionListShapeMap_it;
	typedef std::map<INode*, shapeInfo>::const_iterator perActionListShapeMap_cit;

	struct meshInfo{
		Mesh* pMesh;
		std::string name;
		int nMatId;
		//AbcG::OXformSchema xformSchema;
		//AbcG::OPolyMeshSchema meshSchema;
      Abc::Box3d bbox;

		BOOL bNeedDelete;
		Matrix3 meshTM;

        int nMeshInstanceId;

		meshInfo(): pMesh(NULL), nMatId(-1), bNeedDelete(FALSE), nMeshInstanceId(-1)
		{
         bbox.min = Abc::V3d(0.0, 0.0, 0.0);
         bbox.max = Abc::V3d(0.0, 0.0, 0.0);
      }
	};
	//typedef std::pair<Alembic::Util::Digest, Alembic::Util::Digest> faceVertexHashPair;
	struct meshDigests{
		Alembic::Util::Digest Vertices;
		Alembic::Util::Digest Faces;
		Alembic::Util::Digest MatIds;
		Alembic::Util::Digest UVWs;

		
		bool operator<( const meshDigests &iRhs ) const
		{
			if(Vertices < iRhs.Vertices ) return true;
			if(Vertices > iRhs.Vertices ) return false;
			if(Faces < iRhs.Faces ) return true;
			if(Faces > iRhs.Faces ) return false;
			if(MatIds < iRhs.MatIds ) return true;
			if(MatIds > iRhs.MatIds ) return false;
			if(UVWs < iRhs.UVWs ) return true;
			return false;
    	}
	};
	typedef std::map<meshDigests, meshInfo> faceVertexHashToShapeMap;
	faceVertexHashToShapeMap mShapeMeshCache;
	int mNumShapeMeshes;
	int mTotalShapeMeshes;

	//typedef std::map<PreciseTimeValue, bool> timeValueMap;
	//timeValueMap mTimeValueMap;
	//int mTimeSamplesCount;

	std::vector<meshInfo*> mMeshesToSaveForCurrentFrame;

    static void AlembicPoints::ConvertMaxEulerXYZToAlembicQuat(const Point3 &degrees, Abc::Quatd &quat); //double check
    static void AlembicPoints::ConvertMaxAngAxisToAlembicQuat(const AngAxis &angAxis, Abc::Quatd &quat); //double check
    void AlembicPoints::GetShapeType(IParticleObjectExt *pExt, int particleId, TimeValue ticks, ShapeType &type, unsigned short &instanceId, float &animationTime);
	void AlembicPoints::ReadShapeFromOperator( IParticleGroup *particleGroup, PFSimpleOperator *pSimpleOperator, int particleId, TimeValue ticks, ShapeType &type, unsigned short &instanceId, float &animationTime);
	Abc::C4f AlembicPoints::GetColor(IParticleObjectExt *pExt, int particleId, TimeValue ticks);
	//unsigned short FindInstanceName(const std::string& name);

	meshInfo CacheShapeMesh(Mesh* pShapeMesh, BOOL bNeedDelete, Matrix3 meshTM, int nMatId, int particleId, TimeValue ticks, ShapeType &type, unsigned short &instanceId, float &animationTime);

	void saveCurrentFrameMeshes();

    AbcG::OPointsSchema mPointsSchema;
    AbcG::OPointsSchema::Sample mPointsSample;

    // instance lookups
    Abc::OStringArrayProperty mInstanceNamesProperty;
    //std::vector<std::string> mInstanceNames;
    //std::map<unsigned long, size_t> mInstanceMap;
	perActionListShapeMap mPerActionListShapeMap;

    // Additional particle attributes
    Abc::OV3fArrayProperty mScaleProperty;
    Abc::OQuatfArrayProperty mOrientationProperty;
    Abc::OQuatfArrayProperty mAngularVelocityProperty;
    Abc::OFloatArrayProperty mAgeProperty;
    Abc::OFloatArrayProperty mMassProperty;
    Abc::OUInt16ArrayProperty mShapeTypeProperty;
    Abc::OFloatArrayProperty mShapeTimeProperty;
    Abc::OUInt16ArrayProperty mShapeInstanceIDProperty;
    Abc::OC4fArrayProperty mColorProperty;

public:
    AlembicPoints(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
    ~AlembicPoints();

    virtual Abc::OCompoundProperty GetCompound();
    virtual bool Save(double time, bool bLastFrame);
};

#endif
