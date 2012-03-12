#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicTransformBaseModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include <iparamb2.h>
#include <MeshNormalSpec.h>
#include <Assetmanagement\AssetType.h>
#include "alembic.h"
#include "AlembicXForm.h"
#include "AlembicVisCtrl.h"
#include "ParamBlockUtility.h"
#include "AlembicTransformUtilities.h"

using namespace MaxSDK::AssetManagement;

IObjParam *AlembicTransformBaseModifier::ip              = NULL;
AlembicTransformBaseModifier *AlembicTransformBaseModifier::editMod         = NULL;

static AlembicTransformBaseModifierClassDesc AlembicTransformBaseModifierDesc;
ClassDesc2* GetAlembicTransformBaseModifierClassDesc() {return &AlembicTransformBaseModifierDesc;}

//--- Properties block -------------------------------

static ParamBlockDesc2 AlembicTransformBaseModifierParams(
	0,
	_T("AlembicTransformBaseModifier"),
	IDS_PROPS,
	GetAlembicTransformBaseModifierClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	0,

	// rollout description
	IDD_EMPTY, IDS_PARAMS, 0, 0, NULL,

    // params
	AlembicTransformBaseModifier::ID_PATH, _T("path"), TYPE_FILENAME, 0, IDS_PATH,
		end,
        
	AlembicTransformBaseModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, 0, IDS_IDENTIFIER,
		end,

	AlembicTransformBaseModifier::ID_CURRENTTIMEHIDDEN, _T("currentTimeHidden"), TYPE_FLOAT, 0, IDS_CURRENTTIMEHIDDEN,
		end,

	AlembicTransformBaseModifier::ID_TIMEOFFSET, _T("timeOffset"), TYPE_FLOAT, 0, IDS_TIMEOFFSET,
		end,

	AlembicTransformBaseModifier::ID_TIMESCALE, _T("timeScale"), TYPE_FLOAT, 0, IDS_TIMESCALE,
		end,

	AlembicTransformBaseModifier::ID_CAMERATRANSFORM, _T("cameraTransform"), TYPE_BOOL, 0, IDS_CAMERATRANSFORM,
		end,

	AlembicTransformBaseModifier::ID_MUTED, _T("muted"), TYPE_BOOL, 0, IDS_MUTED,
		end,

	end
);

//--- Modifier methods -------------------------------

AlembicTransformBaseModifier::AlembicTransformBaseModifier() 
{
    pblock = NULL;

	GetAlembicTransformBaseModifierClassDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicTransformBaseModifier::Clone(RemapDir& remap) 
{
	AlembicTransformBaseModifier *mod = new AlembicTransformBaseModifier();

    mod->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, mod, remap);
	return mod;
}

Interval AlembicTransformBaseModifier::GetValidity (TimeValue t) {
	// Interval ret = FOREVER;
	// pblock->GetValidity (t, ret);

    // PeterM this will need to be rethought out
    Interval ret(t,t);
	return ret;
}

void AlembicTransformBaseModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

   Interval ivalid=os->obj->ObjectValidity(t);

	TimeValue now =  GetCOREInterface12()->GetTime();

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicTransformBaseModifier::ID_PATH, now, strPath, FOREVER);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicTransformBaseModifier::ID_IDENTIFIER, now, strIdentifier, FOREVER);
 
	float fCurrentTimeHidden;
	this->pblock->GetValue( AlembicTransformBaseModifier::ID_CURRENTTIMEHIDDEN, now, fCurrentTimeHidden, FOREVER);

	float fTimeOffset;
	this->pblock->GetValue( AlembicTransformBaseModifier::ID_TIMEOFFSET, now, fTimeOffset, FOREVER);

	float fTimeScale;
	this->pblock->GetValue( AlembicTransformBaseModifier::ID_TIMESCALE, now, fTimeScale, FOREVER); 

	BOOL bCameraTransform;
	this->pblock->GetValue( AlembicTransformBaseModifier::ID_CAMERATRANSFORM, now, bCameraTransform, FOREVER);

	BOOL bMuted;
	this->pblock->GetValue( AlembicTransformBaseModifier::ID_MUTED, now, bMuted, FOREVER);

	float dataTime = fTimeOffset + fCurrentTimeHidden * fTimeScale;
	
	ESS_LOG_INFO( "AlembicTransformBaseModifier::ModifyObject strPath: " << strPath << " strIdentifier: " << strIdentifier << " fCurrentTimeHidden: " << fCurrentTimeHidden << " fTimeOffset: " << fTimeOffset << " fTimeScale: " << fTimeScale << 
		" bCameraTransform: " << bCameraTransform << " bMuted: " << bMuted );

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

   alembic_fillxform_options options;
   options.pIObj = &iObj;
   options.dTicks = GetTimeValueFromSeconds( dataTime );
   options.bIsCameraTransform = ( bCameraTransform == TRUE );
 
   try {
	   AlembicImport_FillInXForm(options);
   }
   catch(std::exception exp ) {
		ESS_LOG_ERROR( "Error reading transform from Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
		return;
   }

   //ESS_LOG_INFO( "NumFaces: " << options.->getNumFaces() << "  NumVerts: " << pMesh->getNumVerts() );

   Interval alembicValid(t, t); 
   ivalid = alembicValid;

   // update the validity channel
   os->ApplyTM( &( options.maxMatrix ), ivalid );

   	ESS_CPP_EXCEPTION_REPORTING_END
}

RefResult AlembicTransformBaseModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	ESS_CPP_EXCEPTION_REPORTING_START

	switch (message) 
    {
	case REFMSG_CHANGE:
		if (editMod!=this) break;
		
        AlembicTransformBaseModifierParams.InvalidateUI(pblock->LastNotifyParamID());
		break;
 
    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return REF_SUCCEED;
}
