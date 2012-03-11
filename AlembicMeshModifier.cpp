#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicMeshModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include <iparamb2.h>
#include <MeshNormalSpec.h>
#include <Assetmanagement\AssetType.h>
#include "alembic.h"
#include "AlembicXForm.h"
#include "AlembicVisCtrl.h"
#include "ParamBlockUtility.h"
#include "AlembicNames.h"

using namespace MaxSDK::AssetManagement;

#define MAX_PARAM_BLOCKS 1 // file local

class AlembicMeshModifier : public Modifier {
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
	static AlembicMeshModifier *editMod;

	AlembicMeshModifier();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Mesh Modifier"); }  
	virtual Class_ID ClassID() { return EXOCORTEX_ALEMBIC_MESH_MODIFIER_ID; }		
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
//--- ClassDescriptor and class vars ---------------------------------

IObjParam *AlembicMeshModifier::ip              = NULL;
AlembicMeshModifier *AlembicMeshModifier::editMod         = NULL;

class AlembicMeshModifierClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicMeshModifier; }
	const TCHAR *	ClassName() { return _T("Alembic Mesh Modifier"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_MESH_MODIFIER_ID; }
	const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY; }
	const TCHAR*	InternalName() { return _T("AlembicMeshModifier"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static AlembicMeshModifierClassDesc AlembicMeshModifierDesc;
ClassDesc2* GetAlembicMeshModifierClassDesc() {return &AlembicMeshModifierDesc;}

//--- Properties block -------------------------------

static ParamBlockDesc2 AlembicMeshModifierParams(
	AlembicMeshModifier::PBLOCK_ID,
	_T("AlembicMeshModifier"),
	IDS_PROPS,
	GetAlembicMeshModifierClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	REF_PBLOCK,

	// rollout description
	IDD_EMPTY, IDS_PARAMS, 0, 0, NULL,

    // params
	AlembicMeshModifier::ID_PATH, _T("path"), TYPE_FILENAME, 0, IDS_PATH,
		end,
        
	AlembicMeshModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, 0, IDS_IDENTIFIER,
		end,

	AlembicMeshModifier::ID_CURRENTTIMEHIDDEN, _T("currentTimeHidden"), TYPE_FLOAT, 0, IDS_CURRENTTIMEHIDDEN,
		end,

	AlembicMeshModifier::ID_TIMEOFFSET, _T("timeOffset"), TYPE_FLOAT, 0, IDS_TIMEOFFSET,
		end,

	AlembicMeshModifier::ID_TIMESCALE, _T("timeScale"), TYPE_FLOAT, 0, IDS_TIMESCALE,
		end,

	AlembicMeshModifier::ID_FACESET, _T("faceSet"), TYPE_BOOL, 0, IDS_FACESET,
		end,

	AlembicMeshModifier::ID_VERTICES, _T("vertices"), TYPE_BOOL, 0, IDS_VERTICES,
		end,

	AlembicMeshModifier::ID_NORMALS, _T("normals"), TYPE_BOOL, 0, IDS_NORMALS,
		end,

	AlembicMeshModifier::ID_UVS, _T("uvs"), TYPE_BOOL, 0, IDS_UVS,
		end,

	AlembicMeshModifier::ID_CLUSTERS, _T("clusters"), TYPE_BOOL, 0, IDS_CLUSTERS,
		end,

	AlembicMeshModifier::ID_MUTED, _T("muted"), TYPE_BOOL, 0, IDS_MUTED,
		end,

	end
);

//--- Modifier methods -------------------------------

AlembicMeshModifier::AlembicMeshModifier() 
{
    for (int i = 0; i < MAX_PARAM_BLOCKS; i += 1)
	    pblock[i] = NULL;

	GetAlembicMeshModifierClassDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicMeshModifier::Clone(RemapDir& remap) 
{
	AlembicMeshModifier *mod = new AlembicMeshModifier();

    for (int i = 0; i < MAX_PARAM_BLOCKS; i += 1)
	    mod->ReplaceReference (i, remap.CloneRef(pblock[i]));
	
    BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicMeshModifier::GetParamBlockByID (short id) 
{
    for (int i = 0; i < MAX_PARAM_BLOCKS; i += 1)
    {
        if (pblock[i] && pblock[i]->ID() == id)
            return pblock[i];
    }

    return NULL;
}

Interval AlembicMeshModifier::GetValidity (TimeValue t) {
	// Interval ret = FOREVER;
	// pblock->GetValidity (t, ret);

    // PeterM this will need to be rethought out
    Interval ret(t,t);
	return ret;
}

void AlembicMeshModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

   Interval ivalid=os->obj->ObjectValidity(t);

	TimeValue now =  GetCOREInterface12()->GetTime();

    MCHAR const* strPath = NULL;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_PATH, now, strPath, FOREVER);

	MCHAR const* strIdentifier = NULL;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_IDENTIFIER, now, strIdentifier, FOREVER);
 
	float fCurrentTimeHidden;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_CURRENTTIMEHIDDEN, now, fCurrentTimeHidden, FOREVER);

	float fTimeOffset;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_TIMEOFFSET, now, fTimeOffset, FOREVER);

	float fTimeScale;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_TIMESCALE, now, fTimeScale, FOREVER); 

	BOOL bFaceSet;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_FACESET, now, bFaceSet, FOREVER);

	BOOL bVertices;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_VERTICES, now, bVertices, FOREVER);

	BOOL bNormals;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_NORMALS, now, bNormals, FOREVER);

	BOOL bUVs;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_UVS, now, bUVs, FOREVER);

	BOOL bMuted;
	this->pblock[0]->GetValue( AlembicMeshModifier::ID_MUTED, now, bMuted, FOREVER);

	float dataTime = fTimeOffset + fCurrentTimeHidden * fTimeScale;
	
	ESS_LOG_INFO( "strPath: " << strPath << " strIdentifier: " << strIdentifier << " fCurrentTimeHidden: " << fCurrentTimeHidden << " fTimeOffset: " << fTimeOffset << " fTimeScale: " << fTimeScale << 
		" bFaceSet: " << bFaceSet << " bVertices: " << bVertices << " bNormals: " << bNormals << " bUVs: " << bUVs << " bMuted: " << bMuted );

	if( bMuted ) {
		return;
	}

	if( strlen( strPath ) == 0 ) {
	   ESS_LOG_ERROR( "No filename specified." );
	   return;
	}
	if( strlen( strIdentifier ) == 0 ) {
	   ESS_LOG_ERROR( "No path specified." );
	   return;
	}

	if( ! fs::exists( strPath ) ) {
		ESS_LOG_ERROR( "Can't file Alembic file.  Path: " << strPath );
		return;
	}

	Alembic::AbcGeom::IObject iObj;
	try {
		iObj = getObjectFromArchive(strPath, strIdentifier);
	} catch( std::exception exp ) {
		ESS_LOG_ERROR( "Can not open Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
		return;
	}

	if(!iObj.valid()) {
		ESS_LOG_ERROR( "Not a valid Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier );
		return;
	}

   alembic_fillmesh_options options;
   options.pIObj = &iObj;
   options.dTicks = GetTimeValueFromSeconds( dataTime );
   options.nDataFillFlags = 0;
    if( bFaceSet ) {
	   options.nDataFillFlags |= ALEMBIC_DATAFILL_FACELIST;
   }
   if( bVertices ) {
	   options.nDataFillFlags |= ALEMBIC_DATAFILL_VERTEX;
   }
   if( bNormals ) {
	   options.nDataFillFlags |= ALEMBIC_DATAFILL_NORMALS;
   }
   if( bUVs ) {
		options.nDataFillFlags |= ALEMBIC_DATAFILL_UVS;
   }
  
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

   try {
	   AlembicImport_FillInPolyMesh(options);
   }
   catch(std::exception exp ) {
		ESS_LOG_ERROR( "Error creating mesh from Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
		return;
   }

   //ESS_LOG_INFO( "NumFaces: " << options.->getNumFaces() << "  NumVerts: " << pMesh->getNumVerts() );

   Interval alembicValid(t, t); 
   ivalid = alembicValid;

   // update the validity channel
   os->obj->UpdateValidity(TOPO_CHAN_NUM, ivalid);
   os->obj->UpdateValidity(GEOM_CHAN_NUM, ivalid);
   os->obj->UpdateValidity(TEXMAP_CHAN_NUM, ivalid);

   	ESS_CPP_EXCEPTION_REPORTING_END
}

RefResult AlembicMeshModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
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
			AlembicMeshModifierParams.InvalidateUI(pblock[i]->LastNotifyParamID());
        }
		break;
 
    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return REF_SUCCEED;
}

TSTR AlembicMeshModifier::SubAnimName(int i)
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
