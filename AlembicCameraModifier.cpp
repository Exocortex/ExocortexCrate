//#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicDefinitions.h"
#include "AlembicCameraModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "AlembicXForm.h"
#include "AlembicVisibilityController.h"
#include "CommonProfiler.h"


using namespace MaxSDK::AssetManagement;

IObjParam *AlembicCameraModifier::ip              = NULL;
AlembicCameraModifier *AlembicCameraModifier::editMod         = NULL;

static AlembicCameraModifierClassDesc AlembicCameraModifierDesc;
ClassDesc2* GetAlembicCameraModifierClassDesc() {return &AlembicCameraModifierDesc;}

//--- Properties block -------------------------------

static const int ALEMBIC_CAMERA_MODIFIER_VERSION = 1;

static ParamBlockDesc2 AlembicCameraModifierParams(
	0,
	_T(ALEMBIC_CAMERA_MODIFIER_SCRIPTNAME),
	0,
	GetAlembicCameraModifierClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI | P_VERSION,
	ALEMBIC_CAMERA_MODIFIER_VERSION,
	0,

	// rollout description 
	IDD_ALEMBIC_MESH_GEOM_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
	AlembicCameraModifier::ID_PATH, _T("path"), TYPE_FILENAME, P_RESET_DEFAULT, IDS_PATH,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PATH_EDIT,
		p_assetTypeID,	kExternalLink,
		p_end,
        
	AlembicCameraModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, P_RESET_DEFAULT, IDS_IDENTIFIER,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_IDENTIFIER_EDIT,
		p_end,

	AlembicCameraModifier::ID_TIME, _T("time"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIME,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_TIME_EDIT,    IDC_TIME_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_IDENTIFIER, _T("hfov"), TYPE_STRING, P_RESET_DEFAULT, IDS_IDENTIFIER,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_IDENTIFIER_EDIT,
		p_end,



	p_end
);

//--- Modifier methods -------------------------------

AlembicCameraModifier::AlembicCameraModifier() 
{
    pblock = NULL;
    m_CachedAbcFile = "";

	GetAlembicCameraModifierClassDesc()->MakeAutoParamBlocks(this);
}

AlembicCameraModifier::~AlembicCameraModifier()
{
   delRefArchive(m_CachedAbcFile);
}


RefTargetHandle AlembicCameraModifier::Clone(RemapDir& remap) 
{
	AlembicCameraModifier *mod = new AlembicCameraModifier();

    mod->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, mod, remap);
	return mod;
}



void AlembicCameraModifier::EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags)  {
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


void AlembicCameraModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{



}

RefResult AlembicCameraModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	ESS_CPP_EXCEPTION_REPORTING_START

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
                    pblock->GetValue( AlembicCameraModifier::ID_PATH, t, strPath, changeInt);
                    m_CachedAbcFile = EC_MCHAR_to_UTF8( strPath );
                    addRefArchive(m_CachedAbcFile);
                }
                break;
            default:
                break;
            }

            AlembicCameraModifierParams.InvalidateUI(changing_param);
        }
        break;
 
    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return REF_SUCCEED;
}


void AlembicCameraModifier::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    editMod  = this;

	AlembicCameraModifierDesc.BeginEditParams(ip, this, flags, prev);
}

void AlembicCameraModifier::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
	AlembicCameraModifierDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    editMod  = NULL;
}