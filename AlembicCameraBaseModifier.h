#ifndef __ALEMBIC_CAMERA_MODIFIER__H
#define __ALEMBIC_CAMERA_MODIFIER__H

#include "Foundation.h"
#include "MNMath.h" 
#include "resource.h"
#include "surf_api.h"
#include "AlembicDefinitions.h"
#include <iparamb2.h>
#include "AlembicNames.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Camera Modifier object
class AlembicCameraBaseModifier : public Modifier 
{  
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
		ID_FACES,
		ID_VERTICES,
		ID_NORMALS,
		ID_UVS,
		ID_CLUSTERS,
		ID_MUTED
	};

	static IObjParam *ip;
	static AlembicCameraBaseModifier *editMod;

	AlembicCameraBaseModifier();

	// From Animatable
	RefTargetHandle Clone(RemapDir& remap);
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Camera Base Modifier"); }  
	virtual Class_ID ClassID() { return ALEMBIC_CAMERA_BASE_MODIFIER_CLASSID; }		
	TCHAR *GetObjectName() { return _T("Alembic Camera Base Modifier"); }

	// From modifier
	ChannelMask ChannelsUsed() { return DISP_ATTRIB_CHANNEL; }		// TODO: What channels do we actually need?
	ChannelMask ChannelsChanged() { return DISP_ATTRIB_CHANNEL; }
	Class_ID InputType() { return defObjectClassID; }
	void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t) { return GetValidity(t); }
	Interval GetValidity (TimeValue t);
	BOOL DependOnTopology(ModContext &mc) { return FALSE; }

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

	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
		                       PartID& partID, RefMessage message);

    //void SetCamera(GenCamera *pCam);

private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock = (IParamBlock2 *) rtarg; }


    //GenCamera        *m_pCamera;
};

class AlembicCameraBaseModifierClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading = FALSE) { return new AlembicCameraBaseModifier; }
	const TCHAR *	ClassName() { return _T("Alembic Camera Base Modifier"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return ALEMBIC_CAMERA_BASE_MODIFIER_CLASSID; }
	const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName() { return _T("AlembicCameraBaseModifier"); }  // returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }                       // returns owning module handle
};


// can be generated via gencid.exe in the help folder of the 3DS Max.

ClassDesc2 *GetAlembicCameraBaseModifierClassDesc();

// Alembic Functions
typedef struct _alembic_importoptions alembic_importoptions;
int AlembicImport_Camera(const std::string &file, const std::string &identifier, alembic_importoptions &options);

#endif	// __ALEMBIC_CAMERA_MODIFIER__H
