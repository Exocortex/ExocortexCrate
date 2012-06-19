#ifndef __ALEMBIC_SIMPLE_PARTICLE__H
#define __ALEMBIC_SIMPLE_PARTICLE__H

#include "Foundation.h"
#include "AlembicDefinitions.h"
#include "Utility.h"
#include <simpobj.h>
#include "AlembicNames.h"
#include "AlembicPoints.h"

const int ALEMBIC_SIMPLE_PARTICLE_REF_PBLOCK2 = 0;

class NullView: public View 
{
public:
    Point2 ViewToScreen(Point3 p) { return Point2(p.x,p.y); }
    NullView() { worldToView.IdentityMatrix(); screenW=640.0f; screenH = 480.0f; }
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
extern int AlembicImport_Points(const std::string &file, Alembic::AbcGeom::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);

class AlembicParticles;

typedef struct _viewportmesh
{
    Mesh *mesh;
    BOOL needDelete;
    _viewportmesh() : mesh(NULL), needDelete(FALSE) {}
} viewportmesh;

class AlembicParticles : public SimpleParticle
{
public:
    IParamBlock2* pblock2;
   
    // Parameters in first block:
	enum 
	{ 
		ID_PATH,
		ID_IDENTIFIER,
		ID_TIME,
        ID_MUTED,
	};

    static IObjParam *s_ObjParam;
    static AlembicParticles *s_EditObject;
public:
    AlembicParticles();
    virtual ~AlembicParticles();
public:
    void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
    void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

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
    virtual void GetMultipleRenderMeshTM (TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid); 
	void GetMultipleRenderMeshTM_Internal(TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid);

    // --- Derived class implementation of the virtual functions in BaseObject ---
    CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }
    CONST_2013 TCHAR *GetObjectName() { return "Alembic Particles"; }
    virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
    virtual int HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
    virtual BOOL OKtoDisplay( TimeValue t);

	virtual Point3 ParticlePosition(TimeValue t,int i);
	virtual Point3 ParticleVelocity(TimeValue t,int i);
	virtual float ParticleSize(TimeValue t,int i);
	virtual int ParticleCenter(TimeValue t,int i);
	virtual TimeValue ParticleAge(TimeValue t, int i);
	virtual TimeValue ParticleLife(TimeValue t, int i);

private:
    bool            GetAlembicIPoints(Alembic::AbcGeom::IPoints &iPoints, const char *strFile, const char *strIdentifier);
    SampleInfo      GetSampleAtTime(Alembic::AbcGeom::IPoints &iPoints, TimeValue t, Alembic::AbcGeom::IPointsSchema::Sample &floorSample, Alembic::AbcGeom::IPointsSchema::Sample &ceilSample) const;
    int             GetNumParticles(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample) const;
    Point3          GetParticlePosition(Alembic::AbcGeom::IPoints &iPoints, const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const;
    Point3          GetParticleVelocity(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const;
    float           GetParticleRadius(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    TimeValue       GetParticleAge(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    Quat            GetParticleOrientation(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    Point3          GetParticleScale(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    AlembicPoints::ShapeType GetParticleShapeType(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    unsigned short  GetParticleShapeInstanceId(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    TimeValue       GetParticleShapeInstanceTime(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    void            FillParticleShapeNodes(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo);
    INode*          GetParticleMeshNode(int meshNumber, INode *displayNode);
  //  void            ClearCurrentViewportMeshes();
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
    Alembic::Abc::StringArraySamplePtr m_InstanceShapeNames;
    std::vector<INode*> m_InstanceShapeINodes;
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
	Alembic::AbcGeom::IPoints m_iPoints;
	TimeValue m_currTick;
};


class AlembicParticlesClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new AlembicParticles(); }
	const TCHAR *	ClassName() { return ALEMBIC_SIMPLE_PARTICLE_NAME; }
	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() { return ALEMBIC_SIMPLE_PARTICLE_CLASSID; }
	const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName() { return ALEMBIC_SIMPLE_PARTICLE_SCRIPTNAME; }  // returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }                       // returns owning module handle
};


#endif	// __ALEMBIC_SIMPLE_PARTICLE__H
