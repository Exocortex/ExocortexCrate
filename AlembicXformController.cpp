#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "AlembicXformController.h"
#include "Utility.h"
#include "dummy.h"
#include <ILockedTracks.h>
#include "iparamb2.h"
#include <iparamm2.h>

#include "AlembicXformUtilities.h"

// This function returns a pointer to a class descriptor for our Utility
// This is the function that informs max that our plug-in exists and is 
// available to use
static AlembicXformControllerClassDesc sAlembicXformControllerClassDesc;
ClassDesc2* GetAlembicXformControllerClassDesc()
{
	return &sAlembicXformControllerClassDesc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Alembic_XForm_Ctrl_Param_Blk
///////////////////////////////////////////////////////////////////////////////////////////////////


static ParamBlockDesc2 AlembicXformControllerParams(
	0,
	_T(ALEMBIC_XFORM_CONTROLLER_SCRIPTNAME),
	0,
	GetAlembicXformControllerClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI,
	0,

	// rollout description 
	IDD_ALEMBIC_XFORM_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
	AlembicXformController::ID_PATH, _T("path"), TYPE_FILENAME, P_RESET_DEFAULT, IDS_PATH,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PATH_EDIT,
	 	end,
        
	AlembicXformController::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, P_RESET_DEFAULT, IDS_IDENTIFIER,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_IDENTIFIER_EDIT,
	 	end,

	AlembicXformController::ID_TIME, _T("time"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIME,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_TIME_EDIT,    IDC_TIME_SPIN, 0.01f,
		end,

	AlembicXformController::ID_CAMERA, _T("camera"), TYPE_BOOL, P_ANIMATABLE, IDS_CAMERA,
		p_default,       TRUE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_CAMERA_CHECKBOX,
		end,

	AlembicXformController::ID_MUTED, _T("muted"), TYPE_BOOL, P_ANIMATABLE, IDS_MUTED,
		p_default,       FALSE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_MUTED_CHECKBOX,
		end,

	end
);

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicXformController Methods
///////////////////////////////////////////////////////////////////////////////////////////////////

IObjParam *AlembicXformController::ip = NULL;
AlembicXformController *AlembicXformController::editMod = NULL;

void AlembicXformController::GetValueLocalTime(TimeValue t, void *ptr, Interval &valid, GetSetMethod method)
{
	ESS_CPP_EXCEPTION_REPORTING_START

	Interval interval = FOREVER;//os->obj->ObjectValidity(t);
	//ESS_LOG_INFO( "Interval Start: " << interval.Start() << " End: " << interval.End() );

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicXformController::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicXformController::ID_IDENTIFIER, t, strIdentifier, interval);
 
	float fTime;
	this->pblock->GetValue( AlembicXformController::ID_TIME, t, fTime, interval);

	BOOL bCamera;
	this->pblock->GetValue( AlembicXformController::ID_CAMERA, t, bCamera, interval);

	BOOL bMuted;
	this->pblock->GetValue( AlembicXformController::ID_MUTED, t, bMuted, interval);

    if (bMuted)
    {
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

    alembic_fillxform_options xformOptions;
    xformOptions.pIObj = &iObj;
    xformOptions.dTicks = GetTimeValueFromSeconds( fTime );
	xformOptions.bIsCameraTransform = ( bCamera ? true : false );

    AlembicImport_FillInXForm(xformOptions);

    valid = valid & interval;

	if (method == CTRL_ABSOLUTE)
	{
		Matrix3* m3InVal = (Matrix3*)ptr;
		*m3InVal = xformOptions.maxMatrix;
	}
	else // CTRL_RELATIVE
	{
		Matrix3* m3InVal = (Matrix3*)ptr;
		*m3InVal = xformOptions.maxMatrix * (*m3InVal);
	}

	ESS_CPP_EXCEPTION_REPORTING_END
}

AlembicXformController::AlembicXformController()
{
    pblock = NULL;
    sAlembicXformControllerClassDesc.MakeAutoParamBlocks(this);
}

AlembicXformController::~AlembicXformController()
{
    delRefArchive(m_CachedAbcFile);
}

RefTargetHandle AlembicXformController::Clone(RemapDir& remap) 
{
	AlembicXformController *ctrl = new AlembicXformController();
    ctrl->ReplaceReference (ALEMBIC_XFORM_CONTROLLER_REF_PBLOCK, remap.CloneRef(pblock));
   	
    BaseClone(this, ctrl, remap);
	return ctrl;
}

void AlembicXformController::SetValueLocalTime(TimeValue t, void *ptr, int commit, GetSetMethod method)
{

}

void* AlembicXformController::CreateTempValue()
{
    return new Matrix3(1);
}

void AlembicXformController::DeleteTempValue(void *val)
{
    delete (Matrix3*)val;
}

void AlembicXformController::ApplyValue(void *val, void *delta)
{
    Matrix3 &deltatm = *((Matrix3*)delta);
    Matrix3 &valtm = *((Matrix3*)val);
    valtm = deltatm * valtm;
}

void AlembicXformController::MultiplyValue(void *val, float m)
{
     Matrix3 *valtm = (Matrix3*)val;
     valtm->PreScale(Point3(m,m,m));
}

void AlembicXformController::Extrapolate(Interval range, TimeValue t, void *val, Interval &valid, int type)
{
}

#define LOCK_CHUNK		0x2535  //the lock value
IOResult AlembicXformController::Save(ISave *isave)
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

IOResult AlembicXformController::Load(ILoad *iload)
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

RefResult AlembicXformController::NotifyRefChanged(
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
                        pblock->GetValue( AlembicXformController::ID_PATH, t, strPath, iv);
                        m_CachedAbcFile = strPath;
                        addRefArchive(m_CachedAbcFile);
                    }
                    break;
                default:
                    break;
                }

                AlembicXformControllerParams.InvalidateUI(changing_param);
            }
            break;

        case REFMSG_OBJECT_CACHE_DUMPED:
            return REF_STOP;
            break;
    }

    return REF_SUCCEED;
}

void AlembicXformController::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    editMod  = this;

    LockableStdControl::BeginEditParams(ip, flags, prev);
	sAlembicXformControllerClassDesc.BeginEditParams(ip, this, flags, prev);
}

void AlembicXformController::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
    LockableStdControl::EndEditParams(ip, flags, next);
	sAlembicXformControllerClassDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    editMod  = NULL;
}

void AlembicXformController::SetReference(int i, ReferenceTarget* pTarget)
{ 
    switch(i) 
    { 
    case ALEMBIC_XFORM_CONTROLLER_REF_PBLOCK:
        pblock = static_cast<IParamBlock2*>(pTarget);
    default:
        break;
    }
}

RefTargetHandle AlembicXformController::GetReference(int i)
{ 
    switch(i)
    {
    case ALEMBIC_XFORM_CONTROLLER_REF_PBLOCK:
        return pblock;
    default:
        return NULL;
    }
}


