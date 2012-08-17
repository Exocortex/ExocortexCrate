#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "AlembicFloatController.h"
#include "Utility.h"
#include "AlembicNames.h"
#include "resource.h"
#include "AlembicMAXScript.h"
#include <cmath>

// This function returns a pointer to a class descriptor for our Utility
// This is the function that informs max that our plug-in exists and is 
// available to use
static AlembicFloatControllerClassDesc sAlembicFloatControllerClassDesc;
ClassDesc2* GetAlembicFloatControllerClassDesc()
{
	return &sAlembicFloatControllerClassDesc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Alembic_vis_Ctrl_Param_Blk
///////////////////////////////////////////////////////////////////////////////////////////////////

static ParamBlockDesc2 AlembicFloatControllerParams(
	0,
	_T(ALEMBIC_FLOAT_CONTROLLER_SCRIPTNAME),
	0,
	GetAlembicFloatControllerClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	0,

	// rollout description 
	IDD_ALEMBIC_FLOAT_CTRL_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
	AlembicFloatController::ID_PATH, _T("path"), TYPE_FILENAME, P_RESET_DEFAULT, IDS_PATH,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PATH_EDIT,
		p_assetTypeID,	AssetManagement::kExternalLink,
	 	p_end,
        
	AlembicFloatController::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, P_RESET_DEFAULT, IDS_IDENTIFIER,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_IDENTIFIER_EDIT,
	 	p_end,

	AlembicFloatController::ID_PROPERTY, _T("property"), TYPE_STRING, P_RESET_DEFAULT, IDC_PROPERTY_EDIT,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PROPERTY_EDIT,
		p_end,

	AlembicFloatController::ID_TIME, _T("time"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIME,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_TIME_EDIT,    IDC_TIME_SPIN, 0.01f,
		p_end,

	AlembicFloatController::ID_MUTED, _T("muted"), TYPE_BOOL, P_ANIMATABLE, IDS_MUTED,
		p_default,       FALSE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_MUTED_CHECKBOX,
		p_end,

	p_end
);

bool iequals(const char* a, const char* b)
{
    size_t sa = strlen(a);
	size_t sb = strlen(b);
	if (sa != sb){
        return false;
	}

	size_t minsize = sa;
	if(sb < minsize) minsize = sb;

	for (size_t i = 0; i < minsize; ++i){
		if (tolower(a[i]) != tolower(b[i])){
            return false;
		}
	}
    return true;
}

bool getCameraSampleVal(Alembic::AbcGeom::ICamera& objCamera, SampleInfo& sampleInfo, Alembic::AbcGeom::CameraSample sample, const char* name, double& sampleVal)
{
    objCamera.getSchema().get(sample, sampleInfo.floorIndex);

	if(iequals(name, "horizontalFOV")){
		double aperature = sample.getHorizontalAperture()*10.0; //convert from cm to mm
		double focalLength = sample.getFocalLength();
		if(focalLength < 0.000001){
			focalLength = 1.0;
		}
		sampleVal = 2.0 * atan(aperature/(2.0*focalLength));
	}
	else if(iequals(name, "verticalFOV")){
		double aperature = sample.getVerticalAperture()*10.0; //convert from cm to mm	
		double focalLength = sample.getFocalLength();
		if(focalLength < 0.000001){
			focalLength = 1.0;
		}
		sampleVal = 2.0 * atan(aperature/(2.0*focalLength));
	}
	else if(iequals(name, "FocalLength")){
		sampleVal = sample.getFocalLength();
	}
	else if(iequals(name, "HorizontalAperture")){
		sampleVal = sample.getHorizontalAperture();
	}
	else if(iequals(name, "HorizontalFilmOffset")){
		sampleVal = sample.getHorizontalFilmOffset();
	}
	else if(iequals(name, "VerticalAperture")){
		sampleVal = sample.getVerticalAperture();
	}
	else if(iequals(name, "VerticalFilmOffset")){
		sampleVal = sample.getVerticalFilmOffset();
	}
	else if(iequals(name, "LensSqueezeRatio")){
		sampleVal = sample.getLensSqueezeRatio();
	}
	else if(iequals(name, "OverScanLeft")){
		sampleVal = sample.getOverScanLeft();
	}
	else if(iequals(name, "OverScanRight")){
		sampleVal = sample.getOverScanRight();
	}
	else if(iequals(name, "OverScanTop")){
		sampleVal = sample.getOverScanTop(); 
	}
	else if(iequals(name, "OverScanBottom")){
		sampleVal = sample.getOverScanBottom();
	}
	else if(iequals(name, "FStop")){
		sampleVal = sample.getFStop();
	}
	else if(iequals(name, "FocusDistance")){
		sampleVal = sample.getFocusDistance();
	}
	else if(iequals(name, "ShutterOpen")){
		sampleVal = sample.getShutterOpen();
	}
	else if(iequals(name, "ShutterClose")){
		sampleVal = sample.getShutterClose();
	}
	else if(iequals(name, "NearClippingPlane")){
		sampleVal = sample.getNearClippingPlane();
	}
	else if(iequals(name, "FarClippingPlane")){
		sampleVal = sample.getFarClippingPlane();
	}
	else{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicFloatController Methods
///////////////////////////////////////////////////////////////////////////////////////////////////
IObjParam *AlembicFloatController::ip = NULL;
AlembicFloatController *AlembicFloatController::editMod = NULL;

void AlembicFloatController::GetValueLocalTime(TimeValue t, void *ptr, Interval &valid, GetSetMethod method)
{
 	ESS_CPP_EXCEPTION_REPORTING_START

	Interval interval = FOREVER;

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicFloatController::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicFloatController::ID_IDENTIFIER, t, strIdentifier, interval);
 
	MCHAR const* strProperty = NULL;
	this->pblock->GetValue( AlembicFloatController::ID_PROPERTY, t, strProperty, interval);

	float fTime;
	this->pblock->GetValue( AlembicFloatController::ID_TIME, t, fTime, interval);

	BOOL bMuted;
	this->pblock->GetValue( AlembicFloatController::ID_MUTED, t, bMuted, interval);

    if (bMuted || !strProperty)
    {
        return;
    }
	
	std::string szPath = EC_MCHAR_to_UTF8( strPath );
	std::string szIdentifier = EC_MCHAR_to_UTF8( strIdentifier );
	std::string szProperty = EC_MCHAR_to_UTF8( strProperty );	

	if( szPath.size() == 0 ) {
	   ESS_LOG_ERROR( "No filename specified." );
	   return;
	}
	if( szIdentifier.size() == 0 ) {
	   ESS_LOG_ERROR( "No path specified." );
	   return;
	}
	if( szProperty.size() == 0 ) {
	   ESS_LOG_ERROR( "No property specified." );
	   return;
	}
	if( ! fs::exists( szPath.c_str() ) ) {
		ESS_LOG_ERROR( "Can't find Alembic file.  Path: " << szPath );
		return;
	}

	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(szPath, szIdentifier);
    
	if(!iObj.valid() || !Alembic::AbcGeom::ICamera::matches(iObj.getMetaData())) {
        return;
	}

    Alembic::AbcGeom::ICamera objCamera = Alembic::AbcGeom::ICamera(iObj, Alembic::Abc::kWrapExisting);

	TimeValue dTicks = GetTimeValueFromSeconds( fTime );
    double sampleTime = GetSecondsFromTimeValue(dTicks);

	SampleInfo sampleInfo = getSampleInfo(sampleTime,
                                          objCamera.getSchema().getTimeSampling(),
                                          objCamera.getSchema().getNumSamples());
    Alembic::AbcGeom::CameraSample sample;
    objCamera.getSchema().get(sample, sampleInfo.floorIndex);

	double sampleVal = 0.0;
	if(!getCameraSampleVal(objCamera, sampleInfo, sample, szProperty.c_str(), sampleVal)){
		return;
	}

    // Blend the camera values, if necessary
    if (sampleInfo.alpha != 0.0)
    {
        objCamera.getSchema().get(sample, sampleInfo.ceilIndex);
		double sampleVal2 = 0.0;
		if(getCameraSampleVal(objCamera, sampleInfo, sample, szProperty.c_str(), sampleVal2)){
			sampleVal = (1.0 - sampleInfo.alpha) * sampleVal + sampleInfo.alpha * sampleVal2;
		}
    }

	const float fSampleVal = (float)sampleVal;

	valid = interval;

	if (method == CTRL_ABSOLUTE)
	{
		float* fInVal = (float*)ptr;
		*fInVal = fSampleVal;
	}
	else // CTRL_RELATIVE
	{
		float* fInVal = (float*)ptr;
		*fInVal = fSampleVal * (*fInVal);
	}

	ESS_CPP_EXCEPTION_REPORTING_END
}

AlembicFloatController::AlembicFloatController()
{
    pblock = NULL;
    sAlembicFloatControllerClassDesc.MakeAutoParamBlocks(this);
}

AlembicFloatController::~AlembicFloatController()
{
    delRefArchive(m_CachedAbcFile);
}

RefTargetHandle AlembicFloatController::Clone(RemapDir& remap) 
{
	AlembicFloatController *ctrl = new AlembicFloatController();
    ctrl->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, ctrl, remap);
	return ctrl;
}



void AlembicFloatController::EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags)  {
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

void AlembicFloatController::SetValueLocalTime(TimeValue t, void *ptr, int commit, GetSetMethod method)
{
}

void* AlembicFloatController::CreateTempValue()
{
    return new float;
}

void AlembicFloatController::DeleteTempValue(void *val)
{
    delete (float*)val;
}

void AlembicFloatController::ApplyValue(void *val, void *delta)
{
    float &fdelta = *((float*)delta);
    float &fval = *((float*)val);
    fval = fdelta * fval;
}

void AlembicFloatController::MultiplyValue(void *val, float m)
{
     float *fVal = (float*)val;
     *fVal = (*fVal) * m;
}

void AlembicFloatController::Extrapolate(Interval range, TimeValue t, void *val, Interval &valid, int type)
{
}

#define LOCK_CHUNK		0x2535  //the lock value
IOResult AlembicFloatController::Save(ISave *isave)
{
	Control::Save(isave);

	// note: if you add chunks, it must follow the LOCK_CHUNK chunk due to Renoir error in 
	// placement of Control::Save(isave);
	ULONG nb;
	int on = (mLocked==true) ? 1 :0;
	isave->BeginChunk(LOCK_CHUNK);
	isave->Write(&on,sizeof(on),&nb);	
	isave->EndChunk();	

	return IO_OK;
}

IOResult AlembicFloatController::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res;

	res = Control::Load(iload);
	if (res!=IO_OK)  return res;

	// We can't do the standard 'while' loop of opening chunks and checking ID
	// since that will eat the Control ORT chunks that were saved improperly in Renoir
	USHORT next = iload->PeekNextChunkID();
	if (next == LOCK_CHUNK) 
	{
		iload->OpenChunk();
		int on;
		res=iload->Read(&on,sizeof(on),&nb);
		if(on)
			mLocked = true;
		else
			mLocked = false;
		iload->CloseChunk();
		if (res!=IO_OK)  return res;
	}

	// Only do anything if this is the control base classes chunk
	next = iload->PeekNextChunkID();
	if (next == CONTROLBASE_CHUNK) 
		res = Control::Load(iload);  // handle improper Renoir Save order
	return res;	
}

RefResult AlembicFloatController::NotifyRefChanged(
    Interval iv, 
    RefTargetHandle hTarg, 
    PartID& partID, 
    RefMessage msg) 
{
    switch (msg) 
    {
    case REFMSG_CHANGE:
        if (hTarg == pblock) 
        {
            ParamID changing_param = pblock->LastNotifyParamID();
            switch(changing_param)
            {
            case ID_PATH:
                {
                    delRefArchive(m_CachedAbcFile);
                    MCHAR const* strPath = NULL;
                    TimeValue t = GetCOREInterface()->GetTime();
                    pblock->GetValue( AlembicFloatController::ID_PATH, t, strPath, iv);
                    m_CachedAbcFile = EC_MCHAR_to_UTF8( strPath );
                    addRefArchive(m_CachedAbcFile);
                }
                break;
            default:
                break;
            }

            AlembicFloatControllerParams.InvalidateUI(changing_param);
        }
        break;

    case REFMSG_OBJECT_CACHE_DUMPED:
        return REF_STOP;
        break;
    }

    return REF_SUCCEED;
}

void AlembicFloatController::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    editMod  = this;

	sAlembicFloatControllerClassDesc.BeginEditParams(ip, this, flags, prev);

    // Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicFloatController::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
	sAlembicFloatControllerClassDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    editMod  = NULL;
}


