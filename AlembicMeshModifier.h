/**********************************************************************
 *<
	FILE: Convert.h

	DESCRIPTION: Convert To type modifiers

	CREATED BY: Steve Anderson 

	HISTORY:

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __ALEMBIC_POLYMESH_MODIFIER__H
#define __ALEMBIC_POLYMESH_MODIFIER__H

#include "Foundation.h"  
#include <MNMath.h>
#include <iparamb2.h>
#include <PolyObj.h>
#include "resource.h"
#include "surf_api.h"
#include <string>
#include "AlembicDefinitions.h"
#include "AlembicMeshUtilities.h"
#include "AlembicNames.h"


class AlembicMeshModifier : public Modifier {
public:
	// Parameters in first block:
	enum 
	{ 
		MAX_PARAM_BLOCKS = 1
	};

	IParamBlock2 *pblock[MAX_PARAM_BLOCKS];
	static const BlockID PBLOCK_ID = 0;
	
	// Parameters in first block:
	enum 
	{ 
		ID_PATH,
		ID_IDENTIFIER,
		ID_CURRENTTIMEHIDDEN,
		ID_TIMEOFFSET,
		ID_TIMESCALE,
		ID_FACESET,
		ID_VERTICES,
		ID_NORMALS,
		ID_UVS,
		ID_CLUSTERS,
		ID_MUTED
	};

	static IObjParam *ip;
	static AlembicMeshModifier *editMod;

	AlembicMeshModifier();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Mesh Modifier"); }  
	virtual Class_ID ClassID() { return ALEMBIC_MESH_MODIFIER_CLASSID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Alembic Mesh Modifier"); }

	// From Modifier
	ChannelMask ChannelsUsed()  { return TOPO_CHANNEL|GEOM_CHANNEL|TEXMAP_CHANNEL; }
	ChannelMask ChannelsChanged() { return TOPO_CHANNEL|GEOM_CHANNEL|TEXMAP_CHANNEL; }
	Class_ID InputType() { return polyObjectClassID; }
	void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t) { return GetValidity(t); }
	Interval GetValidity (TimeValue t);
	BOOL DependOnTopology(ModContext &mc) { return TRUE; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 

	int NumParamBlocks () { return MAX_PARAM_BLOCKS; }
	IParamBlock2 *GetParamBlock (int i) { return pblock[i]; }
	IParamBlock2 *GetParamBlockByID (short id);

	int NumRefs() { return MAX_PARAM_BLOCKS; }
	RefTargetHandle GetReference(int i) { return pblock[i]; }

private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock[i] = (IParamBlock2 *) rtarg; }

public:
	int NumSubs() {return MAX_PARAM_BLOCKS;}
	Animatable* SubAnim(int i) { return GetReference(i); }
	TSTR SubAnimName(int i);

	RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		PartID& partID, RefMessage message);

};



class AlembicMeshModifierClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicMeshModifier; }
	const TCHAR *	ClassName() { return _T("Alembic Mesh Modifier"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return ALEMBIC_MESH_MODIFIER_CLASSID; }
	const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName() { return _T("AlembicMeshModifier"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};


ClassDesc2 *GetAlembicMeshModifierClassDesc();


#endif	// __ALEMBIC_POLYMESH_MODIFIER__H
