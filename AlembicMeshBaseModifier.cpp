#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicMeshBaseModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include <iparamb2.h>
#include <MeshNormalSpec.h>
#include <Assetmanagement\AssetType.h>
#include "alembic.h"
#include "AlembicXForm.h"
#include "AlembicVisCtrl.h"
#include "ParamBlockUtility.h"


using namespace MaxSDK::AssetManagement;

IObjParam *AlembicMeshBaseModifier::ip              = NULL;
AlembicMeshBaseModifier *AlembicMeshBaseModifier::editMod         = NULL;

static AlembicMeshBaseModifierClassDesc AlembicMeshBaseModifierDesc;
ClassDesc2* GetAlembicMeshBaseModifierClassDesc() {return &AlembicMeshBaseModifierDesc;}

//--- Properties block -------------------------------

static ParamBlockDesc2 AlembicMeshBaseModifierParams(
	0,
	_T("AlembicMeshBaseModifier"),
	IDS_PROPS,
	GetAlembicMeshBaseModifierClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	0,

	// rollout description
	IDD_EMPTY, IDS_PARAMS, 0, 0, NULL,

    // params
	AlembicMeshBaseModifier::ID_PATH, _T("path"), TYPE_FILENAME, 0, IDS_PATH,
		end,
        
	AlembicMeshBaseModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, 0, IDS_IDENTIFIER,
		end,

	AlembicMeshBaseModifier::ID_CURRENTTIMEHIDDEN, _T("currentTimeHidden"), TYPE_FLOAT, 0, IDS_CURRENTTIMEHIDDEN,
		end,

	AlembicMeshBaseModifier::ID_TIMEOFFSET, _T("timeOffset"), TYPE_FLOAT, 0, IDS_TIMEOFFSET,
		end,

	AlembicMeshBaseModifier::ID_TIMESCALE, _T("timeScale"), TYPE_FLOAT, 0, IDS_TIMESCALE,
		end,

	AlembicMeshBaseModifier::ID_FACESET, _T("faceSet"), TYPE_BOOL, 0, IDS_FACESET,
		end,

	AlembicMeshBaseModifier::ID_VERTICES, _T("vertices"), TYPE_BOOL, 0, IDS_VERTICES,
		end,

	AlembicMeshBaseModifier::ID_NORMALS, _T("normals"), TYPE_BOOL, 0, IDS_NORMALS,
		end,

	AlembicMeshBaseModifier::ID_UVS, _T("uvs"), TYPE_BOOL, 0, IDS_UVS,
		end,

	AlembicMeshBaseModifier::ID_CLUSTERS, _T("clusters"), TYPE_BOOL, 0, IDS_CLUSTERS,
		end,

	AlembicMeshBaseModifier::ID_MUTED, _T("muted"), TYPE_BOOL, 0, IDS_MUTED,
		end,

	end
);

//--- Modifier methods -------------------------------

AlembicMeshBaseModifier::AlembicMeshBaseModifier() 
{
    pblock = NULL;

	GetAlembicMeshBaseModifierClassDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicMeshBaseModifier::Clone(RemapDir& remap) 
{
	AlembicMeshBaseModifier *mod = new AlembicMeshBaseModifier();

    mod->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, mod, remap);
	return mod;
}

Interval AlembicMeshBaseModifier::GetValidity (TimeValue t) {
	// Interval ret = FOREVER;
	// pblock->GetValidity (t, ret);

    // PeterM this will need to be rethought out
    Interval ret(t,t);
	return ret;
}

void AlembicMeshBaseModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

   Interval ivalid=os->obj->ObjectValidity(t);

	TimeValue now =  GetCOREInterface12()->GetTime();

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_PATH, now, strPath, FOREVER);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_IDENTIFIER, now, strIdentifier, FOREVER);
 
	float fCurrentTimeHidden;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_CURRENTTIMEHIDDEN, now, fCurrentTimeHidden, FOREVER);

	float fTimeOffset;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_TIMEOFFSET, now, fTimeOffset, FOREVER);

	float fTimeScale;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_TIMESCALE, now, fTimeScale, FOREVER); 

	BOOL bFaceSet;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_FACESET, now, bFaceSet, FOREVER);

	BOOL bVertices;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_VERTICES, now, bVertices, FOREVER);

	BOOL bNormals;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_NORMALS, now, bNormals, FOREVER);

	BOOL bUVs;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_UVS, now, bUVs, FOREVER);

	BOOL bMuted;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_MUTED, now, bMuted, FOREVER);

	float dataTime = fTimeOffset + fCurrentTimeHidden * fTimeScale;
	
	ESS_LOG_INFO( "AlembicMeshBaseModifier::ModifyObject strPath: " << strPath << " strIdentifier: " << strIdentifier << " fCurrentTimeHidden: " << fCurrentTimeHidden << " fTimeOffset: " << fTimeOffset << " fTimeScale: " << fTimeScale << 
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
		ESS_LOG_ERROR( "Error reading mesh from Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
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

RefResult AlembicMeshBaseModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	ESS_CPP_EXCEPTION_REPORTING_START

	switch (message) 
    {
	case REFMSG_CHANGE:
		if (editMod!=this) break;
		
        AlembicMeshBaseModifierParams.InvalidateUI(pblock->LastNotifyParamID());
		break;
 
    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return REF_SUCCEED;
}
