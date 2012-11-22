#ifndef __ALEMBIC_SIMPLE_PARTICLE__H
#define __ALEMBIC_SIMPLE_PARTICLE__H


#include "AlembicDefinitions.h"
#include "Utility.h"
#include <simpobj.h>
#include "AlembicNames.h"
#include "AlembicPoints.h"

const int ALEMBIC_SIMPLE_PARTICLE_REF_PBLOCK2 = 0;

class ExoNullView: public View 
{
public:
    Point2 ViewToScreen(Point3 p) { return Point2(p.x,p.y); }
    ExoNullView() { 
		worldToView.IdentityMatrix(); 
		screenW = 640.0f; 
		screenH = 480.0f; 
		fov = 0.5f;
		pixelSize = 1.0;
		affineTM.IdentityMatrix();
	}
};

class ParticleMtl: public Material 
{
public:
    ParticleMtl();
};

static ParticleMtl particleMtl;

#define PARTICLE_R	float(1.0)
#define PARTICLE_G	float(1.0)
#define PARTICLE_B	float(0.0)

#define A_RENDER			A_PLUGIN1
#define A_NOTREND			A_PLUGIN2

// Alembic functions to create the simple particle object
typedef struct _alembic_importoptions alembic_importoptions;
extern int AlembicImport_Points(const std::string &file, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);

class AlembicParticles;

typedef struct _viewportmesh
{
    Mesh *mesh;
    BOOL needDelete;
    _viewportmesh() : mesh(NULL), needDelete(FALSE) {}
} viewportmesh;

class IAlembicParticlesExt;

class AlembicParticles : public SimpleParticle
{
	friend class IAlembicParticlesExt;
public:
    IParamBlock2* pblock2;
   
    // Parameters in first block:
	enum 
	{ 
		ID_PATH,
		ID_IDENTIFIER,
		ID_TIME,
        ID_MUTED,
		ID_VIEWPORT_PERCENT,
		ID_RENDER_AS_TICKS
	};

    static IObjParam *s_ObjParam;
    static AlembicParticles *s_EditObject;
public:
    AlembicParticles();
    virtual ~AlembicParticles();
public:
    void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
    void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

	void EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags);

	int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
    IParamBlock2* GetParamBlock(int i) { return pblock2; } // return i'th ParamBlock
    IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock2->ID() == id) ? pblock2 : NULL; } // return id'd ParamBlock

	int NumRefs() { return 1; }
	void SetReference(int i, ReferenceTarget* pTarget); 
	RefTargetHandle GetReference(int i); 
	RefResult NotifyRefChanged(Interval, RefTargetHandle, PartID&, RefMessage);

    int NumSubs()  {return 1;} //because it uses the paramblock
    Animatable* SubAnim(int i) {return GetReference(i);}
    TSTR SubAnimName(int i) { return _T("Parameters"); }
    int SubNumToRefNum(int subNum) {if (subNum==0) return 0; else return -1;}
public:
    // --- Derived class implementation of the virtual functions in SimpleParticle ---
    void UpdateParticles(TimeValue t, INode *node);
    void BuildEmitter(TimeValue t, Mesh &mesh);
    Interval GetValidity(TimeValue t);
    MarkerType GetMarkerType();

    // --- Derived class implementation of the virtual functions in Animatable ---
    void DeleteThis() { delete this; }
    RefTargetHandle Clone(RemapDir& remap);
    Class_ID ClassID() { return ALEMBIC_SIMPLE_PARTICLE_CLASSID; } 
    int RenderBegin(TimeValue t, ULONG flags);		
    int RenderEnd(TimeValue t);

    // --- Derived class implementation of the virtual functions in GeomObject ---

    virtual int NumberOfRenderMeshes();
    virtual Mesh* GetMultipleRenderMesh(TimeValue  t,  INode *inode,  View &view,  BOOL &needDelete,  int meshNumber); 
	Mesh* GetMultipleRenderMesh_Internal(TimeValue  t,  INode *inode,  View &view,  BOOL &needDelete,  int meshNumber); 
    virtual void GetMultipleRenderMeshTM (TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid); 
	void GetMultipleRenderMeshTM_Internal(TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid, bool bCalledFromViewport=false);
	virtual Mesh* GetRenderMesh(TimeValue t, INode *inode, View &view, BOOL &needDelete);

    // --- Derived class implementation of the virtual functions in BaseObject ---
    CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }
    CONST_2013 TCHAR *GetObjectName() { return EC_UTF8_to_TCHAR( "Alembic Particles" ); }
    virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
    virtual int HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
    virtual BOOL OKtoDisplay( TimeValue t);

	virtual Point3 ParticlePosition(TimeValue t,int i);
	virtual Point3 ParticleVelocity(TimeValue t,int i);
	virtual float ParticleSize(TimeValue t,int i);
	virtual int ParticleCenter(TimeValue t,int i);
	virtual TimeValue ParticleAge(TimeValue t, int i);
	virtual TimeValue ParticleLife(TimeValue t, int i);

	virtual BaseInterface* GetInterface(Interface_ID id);

private:
    bool            GetAlembicIPoints(AbcG::IPoints &iPoints, std::string strFile, std::string strIdentifier);
    SampleInfo      GetSampleAtTime(AbcG::IPoints &iPoints, TimeValue t, AbcG::IPointsSchema::Sample &floorSample, AbcG::IPointsSchema::Sample &ceilSample) const;
    int             GetNumParticles(const AbcG::IPointsSchema::Sample &floorSample) const;

	void	GetParticlePositions(AbcG::IPoints &iPoints, const AbcG::IPointsSchema::Sample &floorSample, const AbcG::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, const Matrix3& objToWorld, Tab<Point3>& points) const;
    void	GetParticleVelocities(const AbcG::IPointsSchema::Sample &floorSample, const AbcG::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, Matrix3 objToWorld, Tab<Point3>& vels) const;
    void	GetParticleRadii(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, Tab<float>& radius) const;
    void	GetParticleAges(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, Tab<TimeValue>& ages) const;
	void	GetParticleOrientations(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, Matrix3 objToWorld, std::vector<Quat>& particleOrientations) const;
    void	GetParticleScales(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, const Matrix3& objToWorld, std::vector<Point3>& particleScales) const;
	void	GetParticleColors(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, std::vector<VertColor>& colors ) const;
    void	GetParticleShapeTypes(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, std::vector<AlembicPoints::ShapeType>& instanceShapeType) const;
    void	GetParticleShapeInstanceIds(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, std::vector<unsigned short>& instanceShapeIds) const;
    void	GetParticleShapeInstanceTimes(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo, std::vector<TimeValue>& instanceShapeTimes) const;

	void            FillParticleShapeNodes(AbcG::IPoints &iPoints, const SampleInfo &sampleInfo);
    INode*          GetParticleMeshNode(int meshNumber, INode *displayNode);
  //  void            ClearCurrentViewportMeshes();
	//void	ClearMeshCache();

private:
    Mesh *BuildPointMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
    Mesh *BuildBoxMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
    Mesh *BuildSphereMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
    Mesh *BuildCylinderMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
    Mesh *BuildConeMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
    Mesh *BuildDiscMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
    Mesh *BuildRectangleMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
    Mesh *BuildInstanceMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
    Mesh *BuildNbElementsMesh(int meshNumber, TimeValue t, INode *node, View& view, BOOL &needDelete);
private:
    std::vector<Quat> m_ParticleOrientations;
    std::vector<Point3> m_ParticleScales;
    std::vector<AlembicPoints::ShapeType> m_InstanceShapeType;
    std::vector<unsigned short> m_InstanceShapeIds;
    std::vector<TimeValue> m_InstanceShapeTimes;
    Abc::StringArraySamplePtr m_InstanceShapeNames;
    std::vector<INode*> m_InstanceShapeINodes;
	std::vector<VertColor> m_VCArray;

	//struct meshInfo{
	//	BOOL bMeshNeedDelete;
	//	Mesh* pMesh;

	//	meshInfo():bMeshNeedDelete(FALSE), pMesh(NULL)
	//	{}
	//};
	//typedef std::pair<INode*, TimeValue> nodeTimePair;
	//typedef std::map<nodeTimePair, meshInfo> nodeAndTimeToMeshMap;

	//nodeAndTimeToMeshMap meshCacheMap;

    //size_t m_TotalShapesToEnumerate;
   // std::vector<viewportmesh> m_ParticleViewportMeshes;
    std::string m_CachedAbcFile;
private:
    GenBoxObject *m_pBoxMaker;
    GenSphere *m_pSphereMaker;
	GenCylinder *m_pCylinderMaker;
	GenCylinder *m_pDiskMaker;
	GenBoxObject* m_pRectangleMaker;
	Matrix3  m_objToWorld;
	AbcG::IPoints m_iPoints;
	TimeValue m_currTick;

	bool m_outputOrientationMotionBlurWarning;
	bool m_bRenderAsTicks;
	float m_fViewportPercent;

	IAlembicParticlesExt* m_pAlembicParticlesExt;
};


class AlembicParticlesClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new AlembicParticles(); }
	const TCHAR *	ClassName() { return _T(ALEMBIC_SIMPLE_PARTICLE_NAME); }
	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() { return ALEMBIC_SIMPLE_PARTICLE_CLASSID; }
	const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName() { return _T(ALEMBIC_SIMPLE_PARTICLE_SCRIPTNAME); }  // returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }                       // returns owning module handle
};


#endif	// __ALEMBIC_SIMPLE_PARTICLE__H
