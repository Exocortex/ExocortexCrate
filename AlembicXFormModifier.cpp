/**********************************************************************
 *<
	FILE: ToMesh.cpp

	DESCRIPTION:  Convert to Mesh Modifier

	CREATED BY: Steve Anderson

	HISTORY: Created February 2000

 *>	Copyright (c) 2000 Autodesk, All Rights Reserved.
 **********************************************************************/
#include "Foundation.h"
#include "iparamm2.h"
#include "simpmod.h"
#include "AlembicXFormModifier.h"
#include "MeshNormalSpec.h"

static GenSubObjType SOT_Vertex(1);
static GenSubObjType SOT_Edge(2);
static GenSubObjType SOT_Face(4);

class AlembicXForm : public Modifier {
public:
	IParamBlock2 *pblock;

	static IObjParam *ip;
	static AlembicXForm *editMod;

	AlembicXForm();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Exocortex Alembic Xform"); }  
	virtual Class_ID ClassID() { return EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Exocortex Alembic Xform"); }

	// From modifier
	ChannelMask ChannelsUsed()  { return OBJ_CHANNELS; }
	ChannelMask ChannelsChanged() { return OBJ_CHANNELS; }
	Class_ID InputType() { return mapObjectClassID; }
	void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t) { return GetValidity(t); }
	Interval GetValidity (TimeValue t);
	BOOL DependOnTopology(ModContext &mc) { return FALSE; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 
	void BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev);
	void EndEditParams(IObjParam *ip,ULONG flags,Animatable *next);		

	int NumParamBlocks () { return 1; }
	IParamBlock2 *GetParamBlock (int i) { return pblock; }
	IParamBlock2 *GetParamBlockByID (short id);

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return pblock; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock = (IParamBlock2 *) rtarg; }
public:

	int NumSubs() {return 1;}
	Animatable* SubAnim(int i) { return GetReference(i); }
	TSTR SubAnimName(int i) {return _T("IDS_PARAMETERS");}

	RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		PartID& partID, RefMessage message);

	void Convert (PolyObject *obj, TimeValue t, Mesh & m, Interval & ivalid);
	void Convert (TriObject *obj, TimeValue t, Mesh & m, Interval & ivalid);
	void Convert (PatchObject *obj, TimeValue t, Mesh & m, Interval & ivalid);

	int UI2SelLevel(int selLevel);
};

//--- ClassDescriptor and class vars ---------------------------------

IObjParam *AlembicXForm::ip              = NULL;
AlembicXForm *AlembicXForm::editMod         = NULL;

class AlembicXFormClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicXForm; }
	const TCHAR *	ClassName() { return _T("ExoCortex Alembic XForm"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID; }
	const TCHAR* 	Category() { return _T("MAX STANDARD"); }
	const TCHAR*	InternalName() { return _T("AlembicXForm"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static AlembicXFormClassDesc convertToMeshDesc;
ClassDesc* GetAlembicXFormDesc() {return &convertToMeshDesc;}

// Parameter block IDs:
// Blocks themselves:
enum { turn_params };
// Parameters in first block:
enum { turn_use_invis, turn_sel_type, turn_softsel, turn_sel_level };

static ParamBlockDesc2 turn_param_desc ( turn_params, _T("ExoCortexAlembicXForm"),
									IDS_PARAMETERS, &convertToMeshDesc,
									P_AUTO_CONSTRUCT | P_AUTO_UI, REF_PBLOCK,
	//rollout description
	IDD_TO_MESH, IDS_PARAMETERS, 0, 0, NULL,

	// params
	turn_use_invis, _T("useInvisibleEdges"), TYPE_BOOL, P_RESET_DEFAULT|P_ANIMATABLE, IDS_USE_INVIS,
		p_default, TRUE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_USE_INVIS,
		end,

	turn_sel_type, _T("selectionConversion"), TYPE_INT, P_RESET_DEFAULT, IDS_SEL_TYPE,
		p_default, 0, // Preserve selection
		p_ui, TYPE_RADIO, 3, IDC_SEL_PRESERVE, IDC_SEL_CLEAR, IDC_SEL_INVERT,
		end,

	turn_softsel, _T("useSoftSelection"), TYPE_BOOL, P_RESET_DEFAULT, IDS_USE_SOFTSEL,
		p_default, TRUE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_USE_SOFTSEL,
		end,

	turn_sel_level, _T("selectionLevel"), TYPE_INT, P_RESET_DEFAULT, IDS_SEL_LEVEL,
		p_default, 0, // Object level.
		p_ui, TYPE_RADIO, 5, IDC_SEL_PIPELINE, IDC_SEL_OBJ, IDC_SEL_VERT, IDC_SEL_EDGE, IDC_SEL_FACE,
		end,
	end
);

//--- Modifier methods -------------------------------

AlembicXForm::AlembicXForm() {
	pblock = NULL;
	GetAlembicXFormDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicXForm::Clone(RemapDir& remap) {
	AlembicXForm *mod = new AlembicXForm();
	mod->ReplaceReference (0, remap.CloneRef(pblock));
	BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicXForm::GetParamBlockByID (short id) {
	return (pblock->ID() == id) ? pblock : NULL; 
}

Interval AlembicXForm::GetValidity (TimeValue t) {
	Interval ret = FOREVER;
	pblock->GetValidity (t, ret);
	return ret;
}

void AlembicXForm::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) {
	Interval ivalid=os->obj->ObjectValidity (t);

	// set matrix explicitly (this should be done only in the Alembic XForm)
	// os->SetTM( /* my matrix */, ivalid );

	// set mesh explicitly (this should actually be done in the Àlembic PolyMesh`modifier, not the Alembic XForm one)
	TriObject *tobj = CreateNewTriObject ();
	Mesh & m = tobj->GetMesh();
	os->obj = (Object *) tobj;
}

void AlembicXForm::BeginEditParams (IObjParam  *ip, ULONG flags, Animatable *prev) {
	this->ip = ip;	
	editMod  = this;

	// throw up all the appropriate auto-rollouts
	convertToMeshDesc.BeginEditParams(ip, this, flags, prev);

	// Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicXForm::EndEditParams (IObjParam *ip,ULONG flags,Animatable *next) {
	convertToMeshDesc.EndEditParams(ip, this, flags, next);
	this->ip = NULL;
	editMod  = NULL;
}

RefResult AlembicXForm::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	switch (message) {
	case REFMSG_CHANGE:
		if (editMod!=this) break;
		// if this was caused by a NotifyDependents from pblock, LastNotifyParamID()
		// will contain ID to update, else it will be -1 => inval whole rollout
		if (pblock->LastNotifyParamID() == turn_sel_level) {
			// Notify stack that subobject info has changed:
			NotifyDependents(changeInt, partID, message);
			NotifyDependents(FOREVER, 0, REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
			return REF_STOP;
		}
		turn_param_desc.InvalidateUI(pblock->LastNotifyParamID());
		break;
	}

	return REF_SUCCEED;
}

/*bool AlembicXForm::ChangesSelType()
{
	int selLevel;
	Interval ivalid = FOREVER;
	pblock->GetValue (turn_sel_level, GetCOREInterface()->GetTime(), selLevel, ivalid);
	
	if(selLevel == 0) 
		return false;
	else 
		return true;
}

*/
