#ifndef __ALEMBIC_SIMPLE_PARTICLE__H
#define __ALEMBIC_SIMPLE_PARTICLE__H

#include "Foundation.h"
#include "AlembicDefinitions.h"
#include "Utility.h"
#include <iparamb2.h>
#include <simpobj.h>

// can be generated via gencid.exe in the help folder of the 3DS Max.
#define EXOCORTEX_ALEMBIC_SIMPLE_PARTICLE_CLASS_ID  Class_ID(0x246b1c1e, 0x3b2f032f)

// Alembic functions to create the simple particle object
typedef struct _alembic_importoptions alembic_importoptions;
extern int AlembicImport_Points(const std::string &file, const std::string &identifier, alembic_importoptions &options);


class AlembicSimpleParticle : public SimpleParticle
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
    Class_ID ClassID() { return EXOCORTEX_ALEMBIC_SIMPLE_PARTICLE_CLASS_ID; } 

    // --- Derived class implementation of the virtual functions in GeomObject ---
    //Mesh* GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);

    // --- Derived class implementation of the virtual functions in BaseObject ---
    CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }
    TCHAR *GetObjectName() { return "Alembic Simple Particle"; }

private:
    bool            GetAlembicIPoints(Alembic::AbcGeom::IPoints &iPoints);
    SampleInfo      GetSampleAtTime(Alembic::AbcGeom::IPoints &iPoints, TimeValue t, Alembic::AbcGeom::IPointsSchema::Sample &floorSample, Alembic::AbcGeom::IPointsSchema::Sample &ceilSample) const;
    int             GetNumParticles(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample) const;
    Point3          GetParticlePosition(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const;
    Point3          GetParticleVelocity(const Alembic::AbcGeom::IPointsSchema::Sample &floorSample, const Alembic::AbcGeom::IPointsSchema::Sample &ceilSample, const SampleInfo &sampleInfo, int index) const;
    float           GetParticleRadius(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;
    TimeValue       GetParticleAge(Alembic::AbcGeom::IPoints &iPoints, const SampleInfo &sampleInfo, int index) const;

    alembic_nodeprops m_AlembicNodeProps;
};


class AlembicSimpleParticleClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicSimpleParticle(); }
	const TCHAR *	ClassName() { return _T("Particles"); }
	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_SIMPLE_PARTICLE_CLASS_ID; }
	const TCHAR* 	Category() { return _T("Alembic Objects"); }
	const TCHAR*	InternalName() { return _T("AlembicSimpleParticle"); }  // returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }                       // returns owning module handle
};


#endif	// __ALEMBIC_SIMPLE_PARTICLE__H
