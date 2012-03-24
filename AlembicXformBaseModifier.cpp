#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicDefinitions.h"
#include "AlembicXformBaseModifier.h"
#include "AlembicArchiveStorage.h"
#include "AlembicXForm.h"
#include "AlembicVisibilityController.h"
#include "AlembicXformUtilities.h"

using namespace MaxSDK::AssetManagement;

IObjParam *AlembicXformBaseModifier::ip              = NULL;
AlembicXformBaseModifier *AlembicXformBaseModifier::editMod         = NULL;

static AlembicXformBaseModifierClassDesc AlembicXformBaseModifierDesc;
ClassDesc2* GetAlembicXformBaseModifierClassDesc() {return &AlembicXformBaseModifierDesc;}

//--- Properties block -------------------------------

static ParamBlockDesc2 AlembicXformBaseModifierParams(
	0,
	_T("AlembicXformBaseModifier"),
	IDS_PROPS,
	GetAlembicXformBaseModifierClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	0,

	// rollout description
	IDD_EMPTY, IDS_PARAMS, 0, 0, NULL,

    // params
	AlembicXformBaseModifier::ID_PATH, _T("path"), TYPE_FILENAME, 0, IDS_PATH,
		end,
        
	AlembicXformBaseModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, 0, IDS_IDENTIFIER,
		end,

	AlembicXformBaseModifier::ID_CURRENTTIMEHIDDEN, _T("currentTimeHidden"), TYPE_FLOAT, 0, IDS_CURRENTTIMEHIDDEN,
		end,

	AlembicXformBaseModifier::ID_TIMEOFFSET, _T("timeOffset"), TYPE_FLOAT, 0, IDS_TIMEOFFSET,
		end,

	AlembicXformBaseModifier::ID_TIMESCALE, _T("timeScale"), TYPE_FLOAT, 0, IDS_TIMESCALE,
		end,

	AlembicXformBaseModifier::ID_CAMERATRANSFORM, _T("cameraTransform"), TYPE_BOOL, 0, IDS_CAMERATRANSFORM,
		end,

	AlembicXformBaseModifier::ID_MUTED, _T("muted"), TYPE_BOOL, 0, IDS_MUTED,
		end,

	end
);

//--- Modifier methods -------------------------------

AlembicXformBaseModifier::AlembicXformBaseModifier() 
{
    pblock = NULL;

	GetAlembicXformBaseModifierClassDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicXformBaseModifier::Clone(RemapDir& remap) 
{
	AlembicXformBaseModifier *mod = new AlembicXformBaseModifier();

    mod->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, mod, remap);
	return mod;
}

Interval AlembicXformBaseModifier::GetValidity (TimeValue t) {
	// Interval ret = FOREVER;
	// pblock->GetValidity (t, ret);

    // PeterM this will need to be rethought out
    Interval ret(t,t);
	return ret;
}

void AlembicXformBaseModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

   Interval interval =os->obj->ObjectValidity(t);

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicXformBaseModifier::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicXformBaseModifier::ID_IDENTIFIER, t, strIdentifier, interval);
 
	float fCurrentTimeHidden;
	this->pblock->GetValue( AlembicXformBaseModifier::ID_CURRENTTIMEHIDDEN, t, fCurrentTimeHidden, interval);

	float fTimeOffset;
	this->pblock->GetValue( AlembicXformBaseModifier::ID_TIMEOFFSET, t, fTimeOffset, interval);

	float fTimeScale;
	this->pblock->GetValue( AlembicXformBaseModifier::ID_TIMESCALE, t, fTimeScale, interval); 

	BOOL bCameraTransform;
	this->pblock->GetValue( AlembicXformBaseModifier::ID_CAMERATRANSFORM, t, bCameraTransform, interval);

	BOOL bMuted;
	this->pblock->GetValue( AlembicXformBaseModifier::ID_MUTED, t, bMuted, interval);

	float dataTime = fTimeOffset + fCurrentTimeHidden * fTimeScale;
	
	ESS_LOG_INFO( "AlembicXformBaseModifier::ModifyObject strPath: " << strPath << " strIdentifier: " << strIdentifier << " fCurrentTimeHidden: " << fCurrentTimeHidden << " fTimeOffset: " << fTimeOffset << " fTimeScale: " << fTimeScale << 
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
   // update the validity channel
   os->ApplyTM( &( options.maxMatrix ), interval );

   	ESS_CPP_EXCEPTION_REPORTING_END
}

RefResult AlembicXformBaseModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	ESS_CPP_EXCEPTION_REPORTING_START

	switch (message) 
    {
	case REFMSG_CHANGE:
		if (editMod!=this) break;
		
        AlembicXformBaseModifierParams.InvalidateUI(pblock->LastNotifyParamID());
		break;
 
    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return REF_SUCCEED;
}
