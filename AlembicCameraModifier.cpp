//#include "Alembic.h"
#include "AlembicCameraModifier.h"
#include "AlembicMax.h"
#include "AlembicDefinitions.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "AlembicXForm.h"
#include "AlembicVisibilityController.h"
#include "CommonProfiler.h"
#include "AlembicCameraUtilities.h"


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
	IDD_ALEMBIC_CAMERA_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

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

   //set UI for display of floats.
   //Notes:
   //You cannot display a float type as a textfield directly. You must use a spinner. Also, every entry
   //must have a unique param ID value, string table value, text field value (you CANNOT share spinners or string table entries)
   //Might be better to use MAXSCRIPT next time. Unless there is way to write a generic property display in C++.

	AlembicCameraModifier::ID_HFOV, _T("hfov"), TYPE_FLOAT, P_ANIMATABLE, IDS_HFOV,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_HFOV_EDIT,    IDC_HFOV_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_VFOV, _T("vfov"), TYPE_FLOAT, P_ANIMATABLE, IDS_VFOV,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_VFOV_EDIT,    IDC_VFOV_SPIN,  0.01f,
		p_end,


	AlembicCameraModifier::ID_FOCAL, _T("focallength"), TYPE_FLOAT, P_ANIMATABLE, IDS_FOCAL_LENGTH,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_FOCAL_LENGTH_EDIT,    IDC_FOCAL_LENGTH_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_HAPERATURE, _T("haperature"), TYPE_FLOAT, P_ANIMATABLE, IDS_HAPERATURE,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_HAPERATURE_EDIT,    IDC_HAPERATURE_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_VAPERATURE, _T("vaperature"), TYPE_FLOAT, P_ANIMATABLE, IDS_VAPERATURE,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_VAPERATURE_EDIT2,    IDC_VAPERATURE_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_HFILMOFFSET, _T("hfilmoffset"), TYPE_FLOAT, P_ANIMATABLE, IDS_HFILMOFFSET,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_HFILMOFFSET_EDIT,    IDC_HFILMOFFSET_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_VFILMOFFSET, _T("vfilmoffset"), TYPE_FLOAT, P_ANIMATABLE, IDS_VFILMOFFSET,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_VFILMOFFSET_EDIT,    IDC_VFILMOFFSET_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_LSRATIO, _T("lsratio"), TYPE_FLOAT, P_ANIMATABLE, IDS_LSRATIO,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_LSRATIO_EDIT,    IDC_LSRATIO_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_OVERSCANL, _T("loverscan"), TYPE_FLOAT, P_ANIMATABLE, IDS_OVERSCANL,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_OVERSCANL_EDIT,    IDC_OVERSCANL_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_OVERSCANR, _T("roverscan"), TYPE_FLOAT, P_ANIMATABLE, IDS_OVERSCANR,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_OVERSCANR_EDIT,    IDC_OVERSCANR_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_OVERSCANT, _T("toverscan"), TYPE_FLOAT, P_ANIMATABLE, IDS_OVERSCANT,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_OVERSCANT_EDIT,    IDC_OVERSCANT_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_OVERSCANB, _T("boverscan"), TYPE_FLOAT, P_ANIMATABLE, IDS_OVERSCANB,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_OVERSCANB_EDIT,    IDC_OVERSCANB_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_FSTOP, _T("fstop"), TYPE_FLOAT, P_ANIMATABLE, IDS_FSTOP,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_FSTOP_EDIT,    IDC_FSTOP_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_FOCUSDIST, _T("focusdistance"), TYPE_FLOAT, P_ANIMATABLE, IDS_FOCUSDIST,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_FOCUSDIST_EDIT,    IDC_FOCUSDIST_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_SHUTTEROPEN, _T("shutteropen"), TYPE_FLOAT, P_ANIMATABLE, IDS_SHUTTEROPEN,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_SHUTTEROPEN_EDIT,    IDC_SHUTTEROPEN_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_SHUTTERCLOSE, _T("shutterclose"), TYPE_FLOAT, P_ANIMATABLE, IDS_SHUTTERCLOSE,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_SHUTTERCLOSE_EDIT,    IDC_SHUTTERCLOSE_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_NEARCLIPPINGPLANE, _T("nclippingplane"), TYPE_FLOAT, P_ANIMATABLE, IDS_NEARCLIPPINGPLANE,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_NEARCLIPPINGPLANE_EDIT,    IDC_NEARCLIPPINGPLANE_SPIN,  0.01f,
		p_end,

	AlembicCameraModifier::ID_FARCLIPPINGPLANE, _T("fclippingplane"), TYPE_FLOAT, P_ANIMATABLE, IDS_FARCLIPPINGPLANE,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_FAR_CLIPPING_PLANE_EDIT,    IDC_FARCLIPPINGPLANE_SPIN,  0.01f,
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

extern bool getCameraSampleVal(Alembic::AbcGeom::ICamera& objCamera, SampleInfo& sampleInfo, Alembic::AbcGeom::CameraSample sample, const char* name, double& sampleVal);

void AlembicCameraModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
 //  Interval interval = FOREVER;

 //  MCHAR const* strPath = NULL;
	//this->pblock->GetValue( ID_PATH, t, strPath, interval);

	//MCHAR const* strIdentifier = NULL;
	//this->pblock->GetValue( ID_IDENTIFIER, t, strIdentifier, interval);
 //
	//float fTime;
	//this->pblock->GetValue( ID_TIME, t, fTime, interval);

	//std::string szPath = EC_MCHAR_to_UTF8( strPath );
	//std::string szIdentifier = EC_MCHAR_to_UTF8( strIdentifier );

	//if( szPath.size() == 0 ) {
	//   ESS_LOG_ERROR( "No filename specified." );
	//   return;
	//}
	//if( szIdentifier.size() == 0 ) {
	//   ESS_LOG_ERROR( "No path specified." );
	//   return;
	//}

 //  Alembic::AbcGeom::IObject iObj = getObjectFromArchive(szPath, szIdentifier);

 //  if(!iObj.valid() || !Alembic::AbcGeom::ICamera::matches(iObj.getMetaData())) {
 //     return;
 //  }

 //  Alembic::AbcGeom::ICamera objCamera = Alembic::AbcGeom::ICamera(iObj, Alembic::Abc::kWrapExisting);

 //  TimeValue dTicks = GetTimeValueFromSeconds( fTime );
 //  double sampleTime = GetSecondsFromTimeValue(dTicks);

 //  SampleInfo sampleInfo = getSampleInfo(sampleTime,
 //                             objCamera.getSchema().getTimeSampling(),
 //                             objCamera.getSchema().getNumSamples());
 //  Alembic::AbcGeom::CameraSample sample;
 //  objCamera.getSchema().get(sample, sampleInfo.floorIndex);

 //  double sampleVal = 0.0;
 //  if(::getCameraSampleVal(objCamera, sampleInfo, sample, "horizontalFOV", sampleVal)){
 //     
 //  }

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