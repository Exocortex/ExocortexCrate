#ifndef __ALEMBIC_SIMPLE_PARTICLE__H
#define __ALEMBIC_SIMPLE_PARTICLE__H

#include "Foundation.h"
#include "AlembicDefinitions.h"
#include "Utility.h"
#include <iparamb2.h>
#include <simpobj.h>
#include "AlembicNames.h"
#include "AlembicPoints.h"

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
extern int AlembicImport_Points(const std::string &file, const std::string &identifier, alembic_importoptions &options);

class AlembicSimpleParticle;

class AlembicSimpleParticle : public SimpleParticle, ITreeEnumProc
{
public:
	static AlembicSimpleParticle *s_EditObject;
	static IObjParam *s_ObjParam;

    AlembicSimpleParticle();
    virtual ~AlembicSimpleParticle();

    void SetAlembicId(const std::string &file, const std::string &identifier);

    // --- Derived class implementation of the virtual functions in SimpleParticle ---
    void UpdateParticles(TimeValue t, INode *node);
    void BuildEmitter(TimeValue t, Mesh &mesh);
    Interval GetValidity(TimeValue t);
    MarkerType GetMarkerType();

    // --- Derived class implementation of the virtual functions in Animatable ---
    void DeleteThis() { delete this; }
    Class_ID ClassID() { return ALEMBIC_SIMPLE_PARTICLE_CLASSID; } 
    int RenderBegin(TimeValue t, ULONG flags);		
    int RenderEnd(TimeValue t);

    // --- Derived class implementation of the virtual functions in GeomObject ---
    virtual int NumberOfRenderMeshes();
    virtual Mesh* GetMultipleRenderMesh(TimeValue  t,  INode *inode,  View &view,  BOOL &needDelete,  int meshNumber); 
    virtual void GetMultipleRenderMeshTM (TimeValue  t, INode *inode, View &view, int  meshNumber, Matrix3 &meshTM, Interval &meshTMValid); 

    // --- Derived class implementation of the virtual functions in BaseObject ---
    CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }
    TCHAR *GetObjectName() { return "Alembic Simple Particle"; }
    int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

private:
    bool            GetAlembicIPoints(Alembic::AbcGeom::IPoints &iPoints);
    SampleInfo      GetSampleAtTime(Alembic::AbcGeom::IPoints &iPoints, TimeValue t, Alembic::AbcGeom::IPointsSchema::Sample &floorSample, Alembic::AbcGeom::IPointsSchema::Sample &ceilSample) const;
    int             GetNumParticles(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample) const;
    Point3          GetParticlePosition(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const;
    Point3          GetParticleVelocity(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const;
    float           GetParticleRadius(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    TimeValue       GetParticleAge(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    Quat            GetParticleOrientation(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    Point3          GetParticleScale(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    AlembicPoints::ShapeType GetParticleShapeType(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    unsigned short  GetParticleShapeInstanceId(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    TimeValue       GetParticleShapeInstanceTime(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    void            FillParticleShapeNodes(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo);
    int			    callback( INode *node );
    INode*          GetParticleMeshNode(int meshNumber, INode *displayNode);
private:
    Mesh *BuildPointMesh();
    Mesh *BuildBoxMesh();
    Mesh *BuildSphereMesh();
    Mesh *BuildCylinderMesh();
    Mesh *BuildConeMesh();
    Mesh *BuildDiscMesh();
    Mesh *BuildRectangleMesh();
    Mesh *BuildInstanceMesh(int meshNumber);
    Mesh *BuildNbElementsMesh();
private:
    std::vector<Quat> m_ParticleOrientations;
    std::vector<Point3> m_ParticleScales;
    std::vector<AlembicPoints::ShapeType> m_InstanceShapeType;
    std::vector<unsigned short> m_InstanceShapeIds;
    std::vector<TimeValue> m_InstanceShapeTimes;
    Alembic::Abc::StringArraySamplePtr m_InstanceShapeNames;
    std::vector<INode*> m_InstanceShapeINodes;
    size_t m_TotalShapesToEnumerate;
    std::vector<Mesh*> m_ParticleViewportMeshes;
    alembic_nodeprops m_AlembicNodeProps;
};


class AlembicSimpleParticleClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new AlembicSimpleParticle(); }
	const TCHAR *	ClassName() { return _T("Alembic Particles"); }
	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() { return ALEMBIC_SIMPLE_PARTICLE_CLASSID; }
	const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName() { return _T("AlembicSimpleParticle"); }  // returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }                       // returns owning module handle
};


#endif	// __ALEMBIC_SIMPLE_PARTICLE__H
