#include "stdafx.h"
#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicNurbsModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "AlembicXForm.h"
#include "AlembicVisibilityController.h"
#include "AlembicNurbsUtilities.h"
#include <surf_api.h> 

using namespace MaxSDK::AssetManagement;

IObjParam *AlembicNurbsModifier::ip              = NULL;
AlembicNurbsModifier *AlembicNurbsModifier::editMod         = NULL;

static AlembicNurbsModifierClassDesc AlembicNurbsModifierDesc;
ClassDesc2* GetAlembicNurbsModifierClassDesc() {return &AlembicNurbsModifierDesc;}

//--- Properties block -------------------------------

const int ALEMBIC_NURBS_MODIFIER_VERSION = 1;

static ParamBlockDesc2 AlembicNurbsModifierParams(
	0,
	_T(ALEMBIC_NURBS_MODIFIER_SCRIPTNAME),
	0,
	GetAlembicNurbsModifierClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI | P_VERSION,
	ALEMBIC_NURBS_MODIFIER_VERSION,
	0,

	// rollout description 
	IDD_ALEMBIC_NURBS_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
	AlembicNurbsModifier::ID_PATH, _T("path"), TYPE_FILENAME, P_RESET_DEFAULT, IDS_PATH,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PATH_EDIT,
		p_assetTypeID,	kExternalLink,
		p_end,
        
	AlembicNurbsModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, P_RESET_DEFAULT, IDS_IDENTIFIER,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_IDENTIFIER_EDIT,
		p_end,

	AlembicNurbsModifier::ID_TIME, _T("time"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIME,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_TIME_EDIT,    IDC_TIME_SPIN, 0.01f,
		p_end,
		
	AlembicNurbsModifier::ID_MUTED, _T("muted"), TYPE_BOOL, P_ANIMATABLE, IDS_MUTED,
		p_default,       TRUE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_MUTED_CHECKBOX,
		p_end,

	AlembicNurbsModifier::ID_IGNORE_SUBFRAME_SAMPLES, _T("Ignore subframe samples"), TYPE_BOOL, P_ANIMATABLE, IDS_IGNORE_SUBFRAME_SAMPLES,
		p_default,       FALSE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_IGNORE_SUBFRAME_SAMPLES,
		p_end,

	p_end
);

//--- Modifier methods -------------------------------

AlembicNurbsModifier::AlembicNurbsModifier() 
{
    pblock = NULL;
    m_CachedAbcFile = "";

	GetAlembicNurbsModifierClassDesc()->MakeAutoParamBlocks(this);
}

AlembicNurbsModifier::~AlembicNurbsModifier() 
{
     delRefArchive(m_CachedAbcFile);
}

RefTargetHandle AlembicNurbsModifier::Clone(RemapDir& remap) 
{
	AlembicNurbsModifier *mod = new AlembicNurbsModifier();

    mod->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, mod, remap);
	return mod;
}


void AlembicNurbsModifier::EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags)  {
	if ((flags&FILE_ENUM_CHECK_AWORK1)&&TestAFlag(A_WORK1)) return; // LAM - 4/11/03

	if (!(flags&FILE_ENUM_INACTIVE)) return; // not needed by renderer

	if(flags & FILE_ENUM_ACCESSOR_INTERFACE)	{
		IEnumAuxAssetsCallback* callback = static_cast<IEnumAuxAssetsCallback*>(&nameEnum);
		callback->DeclareAsset(AlembicPathAccessor(this));		
	}
	//else {
	//	IPathConfigMgr::GetPathConfigMgr()->RecordInputAsset( this->GetParamBlockByID( 0 )->GetAssetUser( GetParamIdByName( this, 0, "path" ), 0 ), nameEnum, flags);
	//}

	ReferenceTarget::EnumAuxFiles(nameEnum, flags);
} 

void AlembicNurbsModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START
ESS_PROFILE_FUNC();

	Interval interval = FOREVER;//os->obj->ObjectValidity(t);
	//ESS_LOG_INFO( "Interval Start: " << interval.Start() << " End: " << interval.End() );

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicNurbsModifier::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicNurbsModifier::ID_IDENTIFIER, t, strIdentifier, interval);
 
	float fTime;
	this->pblock->GetValue( AlembicNurbsModifier::ID_TIME, t, fTime, interval);

	BOOL bMuted;
	this->pblock->GetValue( AlembicNurbsModifier::ID_MUTED, t, bMuted, interval);
	
	BOOL bIgnoreSubframeSamples;
	this->pblock->GetValue( AlembicNurbsModifier::ID_IGNORE_SUBFRAME_SAMPLES, t, bIgnoreSubframeSamples, interval);

	//ESS_LOG_INFO( "AlembicNurbsModifier::ModifyObject strPath: " << strPath << " strIdentifier: " << strIdentifier << " fTime: " << fTime << 
	//	" bTopology: " << bTopology << " bGeometry: " << bGeometry << " bNormals: " << bNormals << " bUVs: " << bUVs << " bMuted: " << bMuted );

	if( bMuted ) {
		return;
	}

	std::string szPath = EC_MCHAR_to_UTF8( strPath );
	std::string szIdentifier = EC_MCHAR_to_UTF8( strIdentifier );

	AbcG::IObject iObj;
	try {
		iObj = getObjectFromArchive(szPath, szIdentifier);
	} catch( std::exception exp ) {
		ESS_LOG_ERROR( "Can not open Alembic data stream.  Path: " << szPath << " identifier: " << szIdentifier << " reason: " << exp.what() );
		return;
	}

	if(!iObj.valid()) {
		ESS_LOG_ERROR( "Not a valid Alembic data stream.  Path: " << szPath << " identifier: " << szIdentifier );
		return;
	}


	
	alembic_NURBSload_options options;
    options.pIObj =  &iObj;
    options.dTicks = GetTimeValueFromSeconds( fTime );
 
	if(bIgnoreSubframeSamples){
		options.nDataFillFlags |= ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES;
	}

	//SClass_ID superClassID = os->obj->SuperClassID();
	Class_ID classID = os->obj->ClassID();

    if(classID == EDITABLE_SURF_CLASS_ID){
        options.pObject = os->obj;
    }
	else {
		ESS_LOG_ERROR( "Can not convert internal object data into a ShapeObject, confused. (2)" );
	}

   try {
		AlembicImport_LoadNURBS_Internal(options);
   }
   catch(std::exception exp ) {
		ESS_LOG_ERROR( "Error reading shape from Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
		return;
   }

   os->obj = options.pObject;
   os->obj->UnlockObject();

	os->obj->SetChannelValidity(TOPO_CHAN_NUM, interval);
	os->obj->SetChannelValidity(GEOM_CHAN_NUM, interval);
	os->obj->SetChannelValidity(TEXMAP_CHAN_NUM, interval);
	os->obj->SetChannelValidity(MTL_CHAN_NUM, interval);
	os->obj->SetChannelValidity(SELECT_CHAN_NUM, interval);
	os->obj->SetChannelValidity(SUBSEL_TYPE_CHAN_NUM, interval);
	os->obj->SetChannelValidity(DISP_ATTRIB_CHAN_NUM, interval);

   	ESS_CPP_EXCEPTION_REPORTING_END
}

RefResult AlembicNurbsModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	ESS_CPP_EXCEPTION_REPORTING_START

    ESS_PROFILE_FUNC();

    switch (message) 
    {
        case REFMSG_CHANGE:
            if (hTarget == pblock) 
            {
                ParamID changing_param = pblock->LastNotifyParamID();
                switch(changing_param)
                {
                case ID_PATH:
                    {
                        delRefArchive(m_CachedAbcFile);
                        MCHAR const* strPath = NULL;
                        TimeValue t = GetCOREInterface()->GetTime();
                        pblock->GetValue( AlembicNurbsModifier::ID_PATH, t, strPath, changeInt);
                        m_CachedAbcFile = EC_MCHAR_to_UTF8( strPath );
                        addRefArchive(m_CachedAbcFile);
                    }
                    break;
                default:
                    break;
                }

                AlembicNurbsModifierParams.InvalidateUI(changing_param);
            }
            break;
 
    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return REF_SUCCEED;
}


void AlembicNurbsModifier::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    editMod  = this;

	AlembicNurbsModifierDesc.BeginEditParams(ip, this, flags, prev);
}

void AlembicNurbsModifier::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
	AlembicNurbsModifierDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    editMod  = NULL;
}