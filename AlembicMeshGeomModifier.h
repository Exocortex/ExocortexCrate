#ifndef __ALEMBIC_MESH_GEOM_MODIFIER__H
#define __ALEMBIC_MESH_GEOM_MODIFIER__H

#include "Foundation.h"  
#include "AlembicMax.h"
#include "resource.h"
#include "AlembicDefinitions.h"
#include "AlembicMeshUtilities.h"
#include "AlembicNames.h"


class AlembicMeshGeomModifier : public Modifier {
public:
	IParamBlock2 *pblock;
	
	// Parameters in first block:
	enum 
	{ 
		ID_PATH,
		ID_IDENTIFIER,
		ID_TIME,
/*		ID_TOPOLOGY,
		ID_GEOMETRY,*/
		ID_ADDITIVE,
		ID_GEOALPHA,
/*		ID_NORMALS,
		ID_UVS,*/
		ID_MUTED
	};

	static IObjParam *ip;
	static AlembicMeshGeomModifier *editMod;

	AlembicMeshGeomModifier();
    ~AlembicMeshGeomModifier();

	// From Animatable
	virtual Class_ID ClassID() { return ALEMBIC_MESH_GEOM_MODIFIER_CLASSID; }		
	void GetClassName(TSTR& s) { s = _T(ALEMBIC_MESH_GEOM_MODIFIER_NAME); }  
	CONST_2013 TCHAR *GetObjectName() { return _T(ALEMBIC_MESH_GEOM_MODIFIER_NAME); }

	void DeleteThis() { delete this; }
	RefTargetHandle Clone(RemapDir& remap);

	void EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags);

	// From Modifier
	ChannelMask ChannelsUsed()  { return GEOM_CHANNEL; }
	ChannelMask ChannelsChanged() { return GEOM_CHANNEL; }
	Class_ID InputType() { return polyObjectClassID; }
	void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	//Interval LocalValidity(TimeValue t) { return GetValidity(t); }
	//Interval GetValidity (TimeValue t);
	BOOL DependOnTopology(ModContext &mc) { return FALSE; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 

	void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
    void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

	int NumParamBlocks () { return 1; }
	IParamBlock2 *GetParamBlock (int i) { return pblock; }
	IParamBlock2 *GetParamBlockByID (short id) { return (pblock->ID() == id) ? pblock : NULL; }

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return pblock; }

	int NumSubs() { return 1; }
	Animatable* SubAnim(int i) { return GetReference(i); }
	TSTR SubAnimName(int i) { return _T("IDS_PROPS"); }

	RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		PartID& partID, RefMessage message);

private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock = (IParamBlock2 *) rtarg; }
private:
     std::string m_CachedAbcFile;
};



class AlembicMeshGeomModifierClassDesc : public ClassDesc2
{
public:
	AlembicMeshGeomModifierClassDesc() {}
	~AlembicMeshGeomModifierClassDesc() {}

	int 			IsPublic()		{ return TRUE; }
	const TCHAR *	ClassName()		{ return _T(ALEMBIC_MESH_GEOM_MODIFIER_NAME); }
	SClass_ID		SuperClassID()	{ return OSM_CLASS_ID; }
	void *			Create(BOOL loading = FALSE) { return new AlembicMeshGeomModifier; }
	Class_ID		ClassID()		{ return ALEMBIC_MESH_GEOM_MODIFIER_CLASSID; }
	const TCHAR* 	Category()		{ return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName()	{ return _T(ALEMBIC_MESH_GEOM_MODIFIER_SCRIPTNAME); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance()		{ return hInstance; }			// returns owning module handle
};

ClassDesc2 *GetAlembicMeshGeomModifierClassDesc();

#endif
