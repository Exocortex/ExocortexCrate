#ifndef __ALEMBIC_CAMERA_MODIFIER__H
#define __ALEMBIC_CAMERA_MODIFIER__H

  

#include "resource.h"
#include "AlembicDefinitions.h"
#include "AlembicNames.h"


class AlembicCameraModifier : public Modifier {
public:
	IParamBlock2 *pblock;
	
	// Parameters in first block:
	enum 
	{ 
		ID_PATH,
		ID_IDENTIFIER,
		ID_TIME,
      ID_HFOV,
      ID_VFOV,
      ID_FOCAL,
      ID_HAPERATURE,
      ID_VAPERATURE,
      ID_HFILMOFFSET,
      ID_VFILMOFFSET,
      ID_LSRATIO,
      ID_OVERSCANL,
      ID_OVERSCANR,
      ID_OVERSCANT,
      ID_OVERSCANB,
      ID_FSTOP,
      ID_FOCUSDIST,
      ID_SHUTTEROPEN,
      ID_SHUTTERCLOSE,
      ID_NEARCLIPPINGPLANE,      ID_FARCLIPPINGPLANE
	};

	static IObjParam *ip;
	static AlembicCameraModifier *editMod;

	AlembicCameraModifier();
    ~AlembicCameraModifier();

	// From Animatable
	virtual Class_ID ClassID() { return ALEMBIC_CAMERA_MODIFIER_CLASSID; }		
	void GetClassName(TSTR& s) { s = _T(ALEMBIC_CAMERA_MODIFIER_NAME); }  
	CONST_2013 TCHAR *GetObjectName() { return _T(ALEMBIC_CAMERA_MODIFIER_NAME); }

	void DeleteThis() { delete this; }
	RefTargetHandle Clone(RemapDir& remap);

	void EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags);

	// From Modifier
	ChannelMask ChannelsUsed()  { return DISP_ATTRIB_CHANNEL; }
	ChannelMask ChannelsChanged() { return DISP_ATTRIB_CHANNEL; }
	Class_ID InputType() { return defObjectClassID;	 }
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

#if crate_Max_Version == 2015
	RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);
#else
	RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, PartID& partID, RefMessage message);
#endif

private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock = (IParamBlock2 *) rtarg; }
private:
     std::string m_CachedAbcFile;
};



class AlembicCameraModifierClassDesc : public ClassDesc2
{
public:
	AlembicCameraModifierClassDesc() {}
	~AlembicCameraModifierClassDesc() {}

	int 			IsPublic()		{ return TRUE; }
	const TCHAR *	ClassName()		{ return _T(ALEMBIC_CAMERA_MODIFIER_NAME); }
	SClass_ID		SuperClassID()	{ return OSM_CLASS_ID; }
	void *			Create(BOOL loading = FALSE) { return new AlembicCameraModifier; }
	Class_ID		ClassID()		{ return ALEMBIC_CAMERA_MODIFIER_CLASSID; }
	const TCHAR* 	Category()		{ return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName()	{ return _T(ALEMBIC_CAMERA_MODIFIER_SCRIPTNAME); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance()		{ return hInstance; }			// returns owning module handle
};

ClassDesc2 *GetAlembicCameraModifierClassDesc();

#endif
