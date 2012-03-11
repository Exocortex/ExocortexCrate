#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicPolyMeshModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include <iparamb2.h>
#include <MeshNormalSpec.h>
#include <Assetmanagement\AssetType.h>
#include "alembic.h"
#include "AlembicXForm.h"
#include "AlembicVisCtrl.h"
#include "AlembicNames.h"

using namespace MaxSDK::AssetManagement;

#define MAX_PARAM_BLOCKS 1	// file local

class AlembicPolyMeshModifier : public Modifier {
public:
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
	static AlembicPolyMeshModifier *editMod;

	AlembicPolyMeshModifier();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic PolyMesh"); }  
	virtual Class_ID ClassID() { return EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Alembic PolyMesh"); }

	// From modifier
	ChannelMask ChannelsUsed()  { return TOPO_CHANNEL|GEOM_CHANNEL|TEXMAP_CHANNEL; }
	ChannelMask ChannelsChanged() { return TOPO_CHANNEL|GEOM_CHANNEL|TEXMAP_CHANNEL; }
	Class_ID InputType() { return polyObjectClassID; }
	void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t) { return GetValidity(t); }
	Interval GetValidity (TimeValue t);
	BOOL DependOnTopology(ModContext &mc) { return TRUE; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 
	void BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev);
	void EndEditParams(IObjParam *ip,ULONG flags,Animatable *next);		

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
//--- ClassDescriptor and class vars ---------------------------------

IObjParam *AlembicPolyMeshModifier::ip              = NULL;
AlembicPolyMeshModifier *AlembicPolyMeshModifier::editMod         = NULL;

class AlembicPolyMeshModifierClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicPolyMeshModifier; }
	const TCHAR *	ClassName() { return _T("Alembic PolyMesh"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID; }
	const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName() { return _T("AlembicPolyMeshModifier"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static AlembicPolyMeshModifierClassDesc AlembicPolyMeshModifierDesc;
ClassDesc2* GetAlembicPolyMeshModifierClassDesc() {return &AlembicPolyMeshModifierDesc;}

//--- Properties block -------------------------------

static ParamBlockDesc2 AlembicPolyMeshModifierParams(
	AlembicPolyMeshModifier::PBLOCK_ID,
	_T("AlembicPolyMeshModifier"),
	IDS_PROPS,
	GetAlembicPolyMeshModifierClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	REF_PBLOCK,

	// rollout description
	IDD_EMPTY, IDS_PARAMS, 0, 0, NULL,

    // params
	AlembicPolyMeshModifier::ID_PATH, _T("path"), TYPE_FILENAME, 0, IDS_PATH,
		end,
        
	AlembicPolyMeshModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, 0, IDS_IDENTIFIER,
		end,

	AlembicPolyMeshModifier::ID_CURRENTTIMEHIDDEN, _T("currentTimeHidden"), TYPE_FLOAT, 0, IDS_CURRENTTIMEHIDDEN,
		end,

	AlembicPolyMeshModifier::ID_TIMEOFFSET, _T("timeOffset"), TYPE_FLOAT, 0, IDS_TIMEOFFSET,
		end,

	AlembicPolyMeshModifier::ID_TIMESCALE, _T("timeScale"), TYPE_FLOAT, 0, IDS_TIMESCALE,
		end,

	AlembicPolyMeshModifier::ID_FACESET, _T("faceSet"), TYPE_BOOL, 0, IDS_FACESET,
		end,

	AlembicPolyMeshModifier::ID_VERTICES, _T("vertices"), TYPE_BOOL, 0, IDS_VERTICES,
		end,

	AlembicPolyMeshModifier::ID_NORMALS, _T("normals"), TYPE_BOOL, 0, IDS_NORMALS,
		end,

	AlembicPolyMeshModifier::ID_UVS, _T("uvs"), TYPE_BOOL, 0, IDS_UVS,
		end,

	AlembicPolyMeshModifier::ID_CLUSTERS, _T("clusters"), TYPE_BOOL, 0, IDS_CLUSTERS,
		end,

	AlembicPolyMeshModifier::ID_MUTED, _T("muted"), TYPE_BOOL, 0, IDS_MUTED,
		end,

	end
);

//--- Modifier methods -------------------------------

AlembicPolyMeshModifier::AlembicPolyMeshModifier() 
{
    for (int i = 0; i < MAX_PARAM_BLOCKS; i += 1)
	    pblock[i] = NULL;

	GetAlembicPolyMeshModifierDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicPolyMeshModifier::Clone(RemapDir& remap) 
{
	AlembicPolyMeshModifier *mod = new AlembicPolyMeshModifier();

    for (int i = 0; i < MAX_PARAM_BLOCKS; i += 1)
	    mod->ReplaceReference (i, remap.CloneRef(pblock[i]));
	
    BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicPolyMeshModifier::GetParamBlockByID (short id) 
{
    for (int i = 0; i < MAX_PARAM_BLOCKS; i += 1)
    {
        if (pblock[i] && pblock[i]->ID() == id)
            return pblock[i];
    }

    return NULL;
}

Interval AlembicPolyMeshModifier::GetValidity (TimeValue t) {
	// Interval ret = FOREVER;
	// pblock->GetValidity (t, ret);

    // PeterM this will need to be rethought out
    Interval ret(t,t);
	return ret;
}

void AlembicPolyMeshModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

   Interval ivalid=os->obj->ObjectValidity(t);

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
   if(!iObj.valid())
      return;

   alembic_fillmesh_options options;
   options.pIObj = &iObj;
   options.dTicks = t;
   options.nDataFillFlags = m_AlembicNodeProps.m_UpdateDataFillFlags;

   // Find out if we are modifying a poly object or a tri object
   if (os->obj->CanConvertToType(Class_ID(POLYOBJ_CLASS_ID, 0)))
   {
	   PolyObject *pPolyObj = reinterpret_cast<PolyObject *>(os->obj->ConvertToType(t, Class_ID(POLYOBJ_CLASS_ID, 0)));

	   options.pMNMesh = &( pPolyObj->GetMesh() );
    
	   if (os->obj != pPolyObj) {
          os->obj = pPolyObj;
	   }

   }
   else if (os->obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
   {
      TriObject *pTriObj = reinterpret_cast<TriObject *>(os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0)));
		options.pMesh = &( pTriObj->GetMesh() );

		if (os->obj != pTriObj) {
          os->obj = pTriObj;
		}
   }
   else
   {
       return;
   }

   AlembicImport_FillInPolyMesh(options);

   Interval alembicValid(t, t); 
   ivalid = alembicValid;

   // update the validity channel
   os->obj->UpdateValidity(TOPO_CHAN_NUM, ivalid);
   os->obj->UpdateValidity(GEOM_CHAN_NUM, ivalid);
   os->obj->UpdateValidity(TEXMAP_CHAN_NUM, ivalid);

   	ESS_CPP_EXCEPTION_REPORTING_END
}

void AlembicPolyMeshModifier::BeginEditParams (IObjParam  *ip, ULONG flags, Animatable *prev) {
	ESS_CPP_EXCEPTION_REPORTING_START

	this->ip = ip;	
	editMod  = this;

	// throw up all the appropriate auto-rollouts
	AlembicPolyMeshModifierDesc.BeginEditParams(ip, this, flags, prev);

    /*TimeValue t = GetCOREInterface()->GetTime();
    const char *strArchive = GetAlembicArchive().c_str();
    GetParamBlock(polymesh_preview)->SetValue(polymesh_preview_abc_archive, t, strArchive);

    const char *strObjectId = GetAlembicObjectId().c_str();
    GetParamBlock(polymesh_preview)->SetValue(polymesh_preview_abc_archive, t, strObjectId);
    */

	// Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

	ESS_CPP_EXCEPTION_REPORTING_END
}

void AlembicPolyMeshModifier::EndEditParams (IObjParam *ip,ULONG flags,Animatable *next) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

	AlembicPolyMeshModifierDesc.EndEditParams(ip, this, flags, next);
	this->ip = NULL;
	editMod  = NULL;

	ESS_CPP_EXCEPTION_REPORTING_END
}

RefResult AlembicPolyMeshModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	ESS_CPP_EXCEPTION_REPORTING_START

	switch (message) 
    {
	case REFMSG_CHANGE:
		if (editMod!=this) break;
		// if this was caused by a NotifyDependents from pblock, LastNotifyParamID()
		// will contain ID to update, else it will be -1 => inval whole rollout
		/*if (pblock->LastNotifyParamID() == turn_sel_level) {
			// Notify stack that subobject info has changed:
			NotifyDependents(changeInt, partID, message);
			NotifyDependents(FOREVER, 0, REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
			return REF_STOP;
		}
        */
        for (int i = 0; i < MAX_PARAM_BLOCKS; i += 1)
        {
		    polymesh_props_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
            polymesh_preview_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
            // polymesh_render_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
        }
		break;

    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return REF_SUCCEED;
}

void AlembicPolyMeshModifier::SetAlembicId(const std::string &file, const std::string &identifier)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

TSTR AlembicPolyMeshModifier::SubAnimName(int i)
{
    if ( i == 0)
        return _T("IDS_PROPS");
    else if (i == 1)
        return _T("IDS_PREVIEW");
    else if (i == 2)
        return _T("IDS_RENDER");
    else
        return "";
}

