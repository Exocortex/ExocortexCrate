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

	AlembicMeshBaseModifier::ID_TIME, _T("time"), TYPE_FLOAT, 0, IDS_TIME,
		end,

	AlembicMeshBaseModifier::ID_TOPOLOGY, _T("topology"), TYPE_BOOL, 0, IDS_TOPOLOGY,
		end,

	AlembicMeshBaseModifier::ID_GEOMETRY, _T("geometry"), TYPE_BOOL, 0, IDS_GEOMETRY,
		end,

	AlembicMeshBaseModifier::ID_UVS, _T("uvs"), TYPE_BOOL, 0, IDS_UVS,
		end,

	AlembicMeshBaseModifier::ID_NORMALS, _T("normals"), TYPE_BOOL, 0, IDS_NORMALS,
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

void AlembicMeshBaseModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

	Interval interval = os->obj->ObjectValidity(t);

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_IDENTIFIER, t, strIdentifier, interval);
 
	float fTime;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_TIME, t, fTime, interval);

	BOOL bTopology;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_TOPOLOGY, t, bTopology, interval);

	BOOL bGeometry;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_GEOMETRY, t, bGeometry, interval);

	BOOL bNormals;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_NORMALS, t, bNormals, interval);

	BOOL bUVs;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_UVS, t, bUVs, interval);

	BOOL bMuted;
	this->pblock->GetValue( AlembicMeshBaseModifier::ID_MUTED, t, bMuted, interval);
	
	ESS_LOG_INFO( "AlembicMeshBaseModifier::ModifyObject strPath: " << strPath << " strIdentifier: " << strIdentifier << " fTime: " << fTime << 
		" bTopology: " << bTopology << " bGeometry: " << bGeometry << " bNormals: " << bNormals << " bUVs: " << bUVs << " bMuted: " << bMuted );

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
   options.dTicks = GetTimeValueFromSeconds( fTime );
   options.nDataFillFlags = 0;
    if( bTopology ) {
	   options.nDataFillFlags |= ALEMBIC_DATAFILL_FACELIST;
		options.nDataFillFlags |= ALEMBIC_DATAFILL_FACESETS;
   }
   if( bGeometry ) {
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
  		ESS_LOG_ERROR( "Can not convert internal mesh data into a TriObject or PolyObject, confused." );
	     return;
   }

   try {
	   AlembicImport_FillInPolyMesh(options);
   }
   catch(std::exception exp ) {
		ESS_LOG_ERROR( "Error reading mesh from Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
		return;
   }

   // update the validity channel
    if( bTopology ) {
		os->obj->UpdateValidity(TOPO_CHAN_NUM, interval);
		os->obj->UpdateValidity(GEOM_CHAN_NUM, interval);
	}
    if( bGeometry ) {
		os->obj->UpdateValidity(GEOM_CHAN_NUM, interval);
	}
    if( bUVs ) {
		os->obj->UpdateValidity(TEXMAP_CHAN_NUM, interval);
   }

	ESS_LOG_INFO( "Interval Start: " << interval.Start() << " End: " << interval.End() );

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
