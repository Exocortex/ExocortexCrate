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


class AlembicTransformBaseModifier : public Modifier {
public:
	IParamBlock2 *pblock;
	
	// Parameters in first block:
	enum 
	{ 
		ID_PATH,
		ID_IDENTIFIER,
		ID_CURRENTTIMEHIDDEN,
		ID_TIMEOFFSET,
		ID_TIMESCALE,
		ID_CAMERATRANSFORM,
		ID_MUTED
	};

	static IObjParam *ip;
	static AlembicTransformBaseModifier *editMod;

	AlembicTransformBaseModifier();
 
	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Transform Base Modifier"); }  
	virtual Class_ID ClassID() { return ALEMBIC_TRANSFORM_BASE_MODIFIER_CLASSID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Alembic Transform Base Modifier"); }

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
};



class AlembicTransformBaseModifierClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading = FALSE) { return new AlembicTransformBaseModifier; }
	const TCHAR *	ClassName() { return _T("Alembic Transform Base Modifier"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return ALEMBIC_TRANSFORM_BASE_MODIFIER_CLASSID; }
	const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName() { return _T("AlembicTransformBaseModifier"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};


ClassDesc2 *GetAlembicTransformBaseModifierClassDesc();


#endif	// __ALEMBIC_POLYMESH_MODIFIER__H
